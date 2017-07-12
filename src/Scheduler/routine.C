/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: routine.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Data bookkeeping and analysis at the routine level for the
 * MIAMI scheduler. Extends the PrivateRoutine implementation with 
 * functionality specific to the MIAMI post-processing tool.
 */

#include <assert.h>
#include <string.h>
#include <fnmatch.h>

#include "miami_types.h"
#include "routine.h"
#include "load_module.h"
#include "CRCHash.h"
#include "source_file_mapping.h"
#include "InstructionDecoder.hpp"
#include "debug_scheduler.h"
#include "DGBuilder.h"
#include "report_time.h"
#include "MiamiDriver.h"
#include "tarjans/MiamiRIFG.h"
#include "base_slice.h"
#include "static_branch_analysis.h"
#include "Assertion.h"

//#include "dyninst-insn-xlate.hpp"
//#include "miami_options.h"

#include <BPatch_flowGraph.h>
#include <Function.h>
#include <CodeObject.h>
#include <CodeSource.h>


using namespace std;
using namespace MIAMI;

StringList MIAMI::Routine::includePatterns;
StringList MIAMI::Routine::excludePatterns;


#define MARKER_CODE 0xabcd
#define DEBUG_PROG_SCOPE 0

namespace MIAMI {
  extern MIAMI_Driver mdriver;
}

Routine::Routine(LoadModule *_img, addrtype _start, usize_t _size, 
        const std::string& _name, addrtype _offset, addrtype _reloc)
    : PrivateRoutine(_img, _start, _size, _name, _offset)
{
   cout <<"routine: "<<_name<<" "<< (unsigned int*)_start<<" "<<(unsigned int*)_offset<<" "<<(unsigned int*)_reloc<<endl;
   reloc_offset = _reloc;
   valid_for_analysis = 0;
   validity_computed = 0;
   g = new CFG(this, _name);

   rFormulas = new RFormulasMap(0);
   dyn_addrToBlock = new DynBlkMap;
   dyn_blkNoToMiamiBlk = new BlkNoMap;
   dyn_addrToBlkNo = new BlkAddrMap;
   createDyninstFunction();
}

Routine::~Routine()
{
  delete g;
  delete rFormulas;
  delete dyn_addrToBlock;
  delete dyn_func;
  // delete dyn_blkNoToAddr;
  delete dyn_addrToBlkNo;
}

LoadModule* 
Routine::InLoadModule() const
{
   return (static_cast<LoadModule*>(img));
}

int
Routine::loadCFGFromFile(FILE *fd)
{
   std::cout<<" in loadCFGFromFIle"<<std::endl;
#define CHECK_COND(cond, err, ...) if (cond) \
   {fprintf(stderr, "ERROR: " err "\n", __VA_ARGS__); goto load_error; }
#define CHECK_COND0(cond, err) if (cond) \
   {fprintf(stderr, "ERROR: " err "\n"); goto load_error; }

   size_t res;
   int ires;
   int32_t len, i;  // to read number of entries
   addrtype entry;
   uint64_t count;
   
   // read routine entry points
   res = fread(&len, 4, 1, fd);
   // if a routine never executes for a particular thread, we save
   // 0 entries, 0 trace starts, and 0 CFG nodes. Thus, 0 is a valid value here.
   CHECK_COND(res!=1 || len<0 || len>1024, "reading number of entries for routine %s, res=%ld, nEntries=%u", 
            name.c_str(), res, len)
   for (i=0 ; i<len ; ++i)
   {
      res = fread(&entry, sizeof(addrtype), 1, fd);
      CHECK_COND(res!=1, "reading entry %u/%u for routine %s", i, len, name.c_str())
      res = fread(&count, sizeof(uint64_t), 1, fd);
      CHECK_COND(res!=1, "reading count of entry %u/%u for routine %s", i, len, name.c_str())
      entries[entry] = count;
#if DEBUG_CFG_COUNTS
      DEBUG_CFG(2,
         fprintf (stderr, "Found entry %u/%u at %p with count %lu\n", i, len,
             (void*)entry, count);
      )
#endif
   }

   // no more call-site returns; we store trace starts
   res = fread(&len, 4, 1, fd);
   CHECK_COND(res!=1 || len<0 || len>65536, "reading number of trace starts for routine %s, res=%ld, nTraceStarts=%u", 
            name.c_str(), res, len)
   for (i=0 ; i<len ; ++i)
   {
      res = fread(&entry, sizeof(addrtype), 1, fd);
      CHECK_COND(res!=1, "reading trace start %u/%u for routine %s", i, len, name.c_str())
      res = fread(&count, sizeof(uint64_t), 1, fd);
      CHECK_COND(res!=1, "reading count of trace start %u/%u for routine %s", i, len, name.c_str())
      traces[entry] = count;
#if DEBUG_CFG_COUNTS
      DEBUG_CFG(2,
         fprintf (stderr, "Found trace start %u/%u at %p with count %lu\n", i, len,
             (void*)entry, count);
      )
#endif
   }
   
   if (g == NULL)
   {
      g = new CFG(this, name);
      CHECK_COND(g==NULL, "allocating CFG object for routine %s", name.c_str())
   }
   
   ires = static_cast<CFG*>(g)->loadFromFile(fd, entries, traces, reloc_offset);
   return (ires);
   
load_error:
   assert(!"Error while loading the CFG data");
   return (-1);
}

// determine if the routine should be analyzed based on the use
// of incude/exclude files
int 
Routine::parse_include_exclude_files(const std::string& include_file, 
            const std::string& exclude_file)
{
   char buf[512];
   if (include_file.length() > 0)
   {
      FILE* fd = fopen(include_file.c_str(), "rt");
      if (fd == NULL)
      {
         fprintf(stderr, "Cannot open the include file %s for reading. Ignore it.\n",
               include_file.c_str());
      }
      else
      {
         while(!feof(fd))
         {
            if (fscanf(fd, "%s", buf) < 1)
               break;
            // we have the name of one pattern in buf
            includePatterns.push_back(std::string(buf));
         }
         fclose(fd);
      }
   }

   if (exclude_file.length() > 0)
   {
      FILE* fd = fopen(exclude_file.c_str(), "rt");
      if (fd == NULL)
      {
         fprintf(stderr, "Cannot open the exclude file %s for reading. Ignore it.\n",
               exclude_file.c_str());
      }
      else
      {
         while(!feof(fd))
         {
            if (fscanf(fd, "%s", buf) < 1)
               break;
            // we have the name of one routine in buf
            excludePatterns.push_back(std::string(buf));
         }
         fclose(fd);
      }
   }
   return (0);
}

bool 
Routine::is_valid_for_analysis()
{
   if (!validity_computed)
   {
      // traverse the include/exclude patterns to determine if 
      // routine is valid
      bool is_valid = true;
      if (!includePatterns.empty())
      {
         // make default false. If we find pattern that matches, then 
         // it will be set to true
         is_valid = false;
         
         StringList::iterator lit = includePatterns.begin();
         for ( ; lit!=includePatterns.end() ; ++lit) {
            if (!fnmatch(lit->c_str(), name.c_str(), 0) )  // pattern matches
            {
               is_valid = true;
               break;
            }
         }
      }
      if (!excludePatterns.empty())
      {
         StringList::iterator lit = excludePatterns.begin();
         for ( ; lit!=excludePatterns.end() ; ++lit) {
            if (!fnmatch(lit->c_str(), name.c_str(), 0) )  // pattern matches
            {
               is_valid = false;
               break;
            }
         }
      }
   
      validity_computed = 1;
      valid_for_analysis = is_valid;
   }
   return (valid_for_analysis);
}

uint64_t 
Routine::SaveStaticData(FILE *fd)
{
   return (SaveStaticAnalysisData(fd, *rFormulas));
}

void 
Routine::FetchStaticData(FILE *fd, uint64_t offset)
{
   LoadStaticAnalysisData(fd, offset, *rFormulas);
}

void
Routine::computeBaseFormulas (ReferenceSlice *rslice, CFG *g, RFormulasMap& refFormulas)
{
   // Use BFS to traverse the graph and compute base formulas 
   // for instructions in each block.
   // Maintain a queue of nodes that still have to be processed.
   CFG::NodeList nqueue;
   
   // start from the cfg entry node; push it on the queue
   unsigned int mm = g->new_marker();
   CFG::Node *node = g->CfgEntry();
   assert (! node->visited(mm));
   node->visit(mm);
   nqueue.push_back(node);
   
   while (! nqueue.empty())
   {
      node = nqueue.front();
      nqueue.pop_front();
      
      // Add all its unprocessed successors to the queue.
      // Before adding a node, I want to make sure that all its
      // predecessors along forward edges are already in the queue.
      // I want to process the nodes in topological order.
      CFG::OutgoingEdgesIterator oeit(node);
      while((bool)oeit)
      {
         CFG::Node *b = oeit->sink();
         if (! b->visited(mm))
         {
            bool topo_ready = true;
            CFG::IncomingEdgesIterator ieit(b);
            while(topo_ready && (bool)ieit)
            {
               if (! ieit->isLoopBackEdge() && !ieit->source()->visited(mm))
                  // node has an unprocessed predecessor along a forward edge
                  // skip this node; we'll get to it again from that predecessor
                  topo_ready = false;
               ++ ieit;
            }
            if (topo_ready)
            {
               b->visit(mm);
               nqueue.push_back(b);
            }
         }
         ++ oeit;
      }
      
      if (node->Size()==0)
         continue;     // nothing to do for nodes of size 0
         
      // decode and process all the micro-ops in this block
      UopCodeCache *ucc = node->MicroOpCode();
      int num_uops = ucc->NumberOfUops();
      for (int i=0 ; i<num_uops ; ++i)
      {
         const uOp_t& uop = ucc->UopAtIndex(i);
         addrtype pc = uop.dinst->pc;
#if DEBUG_STATIC_MEMORY
         DEBUG_STMEM(3,
            if (uop.idx == 0)
               fprintf(stderr, "-----------------------------------\n");
         )
#endif
         if (InstrBinIsMemoryType(uop.iinfo->type))
         {
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(2,
               fprintf(stderr, ">> Found memory micro-op at pc 0x%" PRIxaddr ", idx %d, type %s. Start slicing.\n",
                     pc, uop.idx, Convert_InstrBin_to_string(uop.iinfo->type));
            )
#endif

            int opidx = uop.getMemoryOperandIndex();  // memory operand index in original DecodedInstruction
            if (opidx<0)  // dump the instruction decoding
            {
               DumpInstrList(uop.dinst);
               assert (opidx >= 0);
            }
            RefFormulas* &rf = refFormulas(pc, opidx);
            if (! rf)
               rf = new RefFormulas();
            else
            {
               // if the reference is at a different level then we were expecting 
               // (it has a different number of stride formulas), clear all the
               // strides so that they are recomputed.
               if ((int)(rf->NumberOfStrides()) != node->getLevel())
               {
#if DEBUG_STATIC_MEMORY
                  DEBUG_STMEM(1,
                     cerr << "WARNING: Reference at address 0x" << hex << pc << dec 
                          << " memop index " << opidx << " is at level " 
                          << node->getLevel() << " but has " << rf->NumberOfStrides()
                          << " stride formulas." << endl;
                  )
#endif
                  rf->strides.clear(); // clear the existing strides
               }
            }
            
            // check first if we have the formulas already
            GFSliceVal &_loadedFormula = rf->base;
            // compute formula if not found in file
            if (_loadedFormula.is_uninitialized())
            {
               // we have to compute
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(3,
                  cerr << "Start slice for memory micro-op at address 0x" << hex 
                       << pc << dec << " mem operand index " << opidx << endl;
               )
#endif
               _loadedFormula = rslice->ComputeFormulaForMemoryOperand(node, uop.dinst, i, opidx, pc);
               
               GFSliceVal adjustFormula(sliceVal(0, TY_CONSTANT));
               bool fchanged = false;
               GFSliceVal::iterator sit;
               for (sit=_loadedFormula.begin() ; sit!=_loadedFormula.end() ; ++sit)
               {
                  int tt = sit->TermType();
                  addrtype tpc = sit->Address();
                  int uop_idx = sit->UopIndex();
                  if (tt == TY_LOAD)
                  {
                     coeff_t tval = sit->ValueNum() / sit->ValueDen();
                     // find block containing load instruction
                     // then find the uop and its index in the original instruction
                     PrivateCFG::Node *find_b = g->findNodeContainingAddress(tpc);
                     assert (find_b != NULL);
                     UopCodeCache *bucc = find_b->MicroOpCode();
                     assert (uop_idx < bucc->NumberOfUops());
                     const uOp_t& buop = bucc->UopAtIndex(uop_idx);
                     int bopidx = buop.getMemoryOperandIndex();  // memory operand index in original DecodedInstruction
                     
                     RefFormulas *orf = refFormulas(tpc, bopidx);
                     assert (orf != NULL);  // we should have processed this uop before
                     coeff_t soffset;
                     if (FormulaIsStackReference(orf->base, soffset))
                     {
                        adjustFormula += sliceVal(tval, TY_STACK, tpc, uop_idx, (addrtype)soffset);
                        adjustFormula -= (*sit);
                        fchanged = true;
                     }
                  }
               }
               if (fchanged)
                  _loadedFormula += adjustFormula;
            }  // formula is not already loaded
               
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(1,
               cerr << "* Memory micro-op at address 0x" << hex << pc << dec
                    << ", index " << uop.idx << " accesses memory location: " 
                    << _loadedFormula << endl;
            )
#endif
         }  // uop is memory reference
      }  // for each micro-op
   }  // while queue not empty
   
}


// returns zero if the reference seems not to be indirect
// and one if the reference looks like an indirect access
bool
Routine::clarifyIndirectAccessForRef(RefStrideId& rsi, GFSliceVal& _formula, 
             RFormulasMap& refFormulas, RefInfoMap& memRefs)
{
   if (rsi.visited==1)  // we have seen this reference before
   {
#if DEBUG_STATIC_MEMORY
      DEBUG_STMEM(1,
         cerr << "WARNING: we have a dependency cylcle when computing indirect "
              << "access that contains memory operand at 0x" << hex << rsi.pc 
              << dec << " / " << rsi.opidx << endl;
      )
#endif
      // mark it as indirect and return 1
      _formula.set_indirect_access(GUARANTEED_INDIRECT, 0);
      return (true);
   }
   rsi.visited = 1;
   AddrIntSet& dependOnRefs = _formula.get_indirect_pcs();
   AddrIntSet::iterator sit = dependOnRefs.begin();
   for( ; sit!=dependOnRefs.end() ; ++sit )
   {
      RefInfoMap::iterator rit = memRefs.find(*sit);
      assert (rit != memRefs.end());
      
      RefFormulas *rf = refFormulas(rit->second.pc, rit->second.opidx);
      assert (rf && (int)(rf->strides.size())>rit->second.level);

      GFSliceVal& _tempF = rf->strides[rit->second.level];
      if (_tempF.has_indirect_access()==1)
      {
         // found an indirect ref that I depend on
         // set myself to indirect and return 1
         _formula.set_indirect_access(GUARANTEED_INDIRECT, 0);
         rsi.visited = 0;
         return (true);
      } else
      if (_tempF.has_indirect_access()==2)
      {
         // found a possibly indirect ref that I depend on
         // must recursively analyze this reference
         bool rez = clarifyIndirectAccessForRef(rit->second, _tempF, refFormulas, memRefs);
#if DEBUG_STATIC_MEMORY
         DEBUG_STMEM(2,
            cerr << ">$ (" << rit->second.level << ")* Memory instruction at address 0x" 
                 << hex << rit->second.pc << dec << " / " << rit->second.opidx
                 << " has clarified stride: " << _tempF << endl;
         )
#endif
         if (rez)
         {
            _formula.set_indirect_access(GUARANTEED_INDIRECT, 0);
            rsi.visited = 0;
            return (true);
         } 
      } else
      {
         // this reference is not marked as indirect
         // if its stride is different than zero though, then my reference
         // is going to be indirect
         coeff_t valueNum;
         ucoeff_t valueDen;
         if ( (!IsConstantFormula(_tempF, valueNum, valueDen)) ||
                   valueNum!=0 )
         {
            _formula.set_indirect_access(GUARANTEED_INDIRECT, 0);
            rsi.visited = 0;
            return (true);
         }
      }
   }
   
   // if I reached this point it means that no dependence was causing 
   // this access to be indirect; clear all dependences and return 0
   dependOnRefs.clear();
   rsi.visited = 0;
   return (false);
}

void
Routine::computeStrideFormulasForScope(StrideSlice &sslice, ReferenceSlice *rslice, 
        RIFGNodeId node, TarjanIntervals *tarj, MiamiRIFG* mCfg, int level,
        RFormulasMap& refFormulas, RefInfoMap& memRefs)
{
   // I need to compute the strides for references in this scope and all
   // its inner loops. I can set a limit for how deep I go if this analysis
   // takes too long and the strides relative to a far outer loop are never used.
   int kid;
   CFG *cfg = ControlFlowGraph();
   CFG::NodeList nqueue;
   CFG::Node *root_b = static_cast<CFG::Node*>(mCfg->GetRIFGNode(node));
//   addrtype loopStart = root_b->getStartAddress();
   if (root_b->Size() > 0)
      nqueue.push_back(root_b);
      
   
   // I need to iterate over the nodes twice. First time process only acyclic
   // blocks, computing strides for all memory operands at this level. 
   // In the second pass process interval and irreducible nodes recursively.
   // I use this two step process to process together references that are at 
   // the same level.
   for (kid = tarj->TarjInners(node) ; kid != RIFG_NIL ; 
                kid = tarj->TarjNext(kid))
   {
      if (! tarj->NodeIsLoopHeader(kid))
      {
         CFG::Node *b = static_cast<CFG::Node*>(mCfg->GetRIFGNode(kid));
         if (b->Size() > 0)
            nqueue.push_back(b);
      }
   }

#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(1,
      fprintf(stderr, "$$ Computing stride formulas for blocks with marker %d, at relative depth %d\n",
              root_b->getMarkedWith(), level);
   )
#endif

   // iterate over all the blocks in the queue and compute strides for memory operands
   while (! nqueue.empty())
   {
      CFG::Node *b = nqueue.front();
      nqueue.pop_front();
      
      // decode and process all the micro-ops in this block
      UopCodeCache *ucc = b->MicroOpCode();
      int num_uops = ucc->NumberOfUops();
      for (int i=0 ; i<num_uops ; ++i)
      {
         const uOp_t& uop = ucc->UopAtIndex(i);
         addrtype pc = uop.dinst->pc;
#if DEBUG_STATIC_MEMORY
         DEBUG_STMEM(3,
            if (uop.idx == 0)
               fprintf(stderr, "-----------------------------------\n");
         )
#endif
         if (InstrBinIsMemoryType(uop.iinfo->type))
         {
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(2,
               fprintf(stderr, "$(%d) Found memory micro-op at pc 0x%" PRIxaddr ", idx %d, type %s, block Id %d. Start slicing for stride.\n",
                     level, pc, uop.idx, Convert_InstrBin_to_string(uop.iinfo->type), b->getId());
            )
#endif

            int opidx = uop.getMemoryOperandIndex();  // memory operand index in original DecodedInstruction
            if (opidx<0)  // dump the instruction decoding
            {
               DumpInstrList(uop.dinst);
               assert (opidx >= 0);
            }
            
            // record all the memory references that we found while computing the
            // strides, and record their level relative to the striding loop
            memRefs.insert(RefInfoMap::value_type(AddrIntPair(pc, i),
                      RefStrideId(pc, opidx, level)));
            
            RefFormulas* &rf = refFormulas(pc, opidx);
            if (! rf)
               rf = new RefFormulas();
            
            if ((int)(rf->strides.size()) < root_b->getLevel())
               rf->strides.resize(root_b->getLevel());
               
            // check first if we have the formulas already
            GFSliceVal &_loadedFormula = rf->strides[level];
            if (_loadedFormula.is_uninitialized())  // we have to compute
            {
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(3,
                  cerr << "Start stride slice for memory micro-op at address 0x" << hex 
                       << pc << dec << " mem operand index " << opidx << endl;
               )
#endif
               GFSliceVal tempFormula = sslice.ComputeFormulaForMemoryOperand(b, uop.dinst, i, opidx, pc);

#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(2,
                  cerr << "TempIterFormula: " << tempFormula << endl;
               )
#endif
   
               // iterate over the temp formula to slice its register terms up to the
               // start of the routine
               _loadedFormula = GFSliceVal(sliceVal (0, TY_CONSTANT));
               GFSliceVal::iterator sit;
               for (sit=tempFormula.begin() ; sit!=tempFormula.end() ; ++sit)
               {
                  int tt = sit->TermType();
                  addrtype tpc = sit->Address();
                  int uop_idx = sit->UopIndex();
                  // I have to slice back on all register terms, no matter the
                  // uop index; This is in order to deal with the cases of one strideF == 1
                  // with the other == 2. In such a case I have to add the register of the 
                  // operand of type 2, and hope that this expansion will compute the right 
                  // value for it.
                  /**** TODO *****/
                  if (tt == TY_REGISTER) // && uop_idx==(-1))
                  {
                     const register_info &reg = sit->Register();
                     coeff_t tval = sit->ValueNum() / sit->ValueDen();
                     PrivateCFG::Node *find_b = cfg->findNodeContainingAddress(tpc);
                     assert (find_b != NULL);
                     rslice->clear();
                     rslice->Slice(find_b, uop_idx+1, reg, tpc);
                     RegFormula *rf = rslice->getValueForRegAndDelete(reg);
                     if (rf == NULL)
                        _loadedFormula += (*sit);
                     else
                     {
                        rf->formula *= tval;
                        _loadedFormula += rf->formula;
                        delete (rf);
                     }
                  } else
                  if (tt == TY_LOAD)
                  {
                     const register_info &reg = sit->Register();
                     coeff_t tval = sit->ValueNum() / sit->ValueDen();
                     // try to find if there is a store to the same address
                     // I need to start slicing right from the uop that does
                     // the loading. In fact, it should be the uop immediately
                     // after.
                     // find block containing load instruction
                     PrivateCFG::Node *find_b = cfg->findNodeContainingAddress(tpc);
                     assert (find_b != NULL);
                     rslice->clear();
                     rslice->Slice(find_b, uop_idx+1, reg, tpc);
                     RegFormula *rf = rslice->getValueForRegAndDelete(reg);
                     if (rf == NULL)
                        _loadedFormula += (*sit);
                     else
                     {
                        rf->formula *= tval;
                        _loadedFormula += rf->formula;
                        delete (rf);
                     }
                  } else  // not a REGISTER or LOAD, just add the term unchanged
                  {
                     _loadedFormula += (*sit);
                  }
               }
               if (tempFormula.has_indirect_access() == 1)
                  _loadedFormula.set_indirect_access(GUARANTEED_INDIRECT, 0);
               else
                  if (tempFormula.has_indirect_access() == 2)
                     _loadedFormula.set_indirect_pcs(tempFormula.get_indirect_pcs());
               if (tempFormula.has_irregular_access())
                  _loadedFormula.set_irregular_access();
               
               // iterate over the formula again to find any stack reference terms
               GFSliceVal adjustFormula(sliceVal(0, TY_CONSTANT));
               bool fchanged = false;
               for (sit=_loadedFormula.begin() ; sit!=_loadedFormula.end() ; ++sit)
               {
                  int tt = sit->TermType();
                  addrtype tpc = sit->Address();
                  int uop_idx = sit->UopIndex();
                  if (tt == TY_LOAD)
                  {
                     coeff_t tval = sit->ValueNum() / sit->ValueDen();
                     // find block containing load instruction
                     // then find the uop and its index in the original instruction
                     PrivateCFG::Node *find_b = cfg->findNodeContainingAddress(tpc);
                     assert (find_b != NULL);
                     UopCodeCache *bucc = find_b->MicroOpCode();
                     assert (uop_idx < bucc->NumberOfUops());
                     const uOp_t& buop = bucc->UopAtIndex(uop_idx);
                     int bopidx = buop.getMemoryOperandIndex();  // memory operand index in original DecodedInstruction
                     
                     RefFormulas *orf = refFormulas(tpc, bopidx);
                     assert (orf != NULL);  // we should have processed this uop before
                     coeff_t soffset;
                     if (FormulaIsStackReference(orf->base, soffset))
                     {
                        adjustFormula += sliceVal(tval, TY_STACK, tpc, uop_idx, (addrtype)soffset);
                        adjustFormula -= (*sit);
                        fchanged = true;
                     }
                  }
               }
               if (fchanged)
                  _loadedFormula += adjustFormula;
               
               /* TODO:
                * When we instrument memory ops, we may want to collect also values
                * for any unknown terms used in the stride formulas, like registers
                * and LOADs. See Sparc version, profile_analysis.C:1543
                */
            }  // formula is not already loaded
            
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(1,
               cerr << "*(" << level << ") Memory micro-op at address 0x" 
                    << hex << pc << dec << ", index " << uop.idx 
                    << " has stride: " << _loadedFormula << endl;
            )
#endif
         }  // uop is memory reference
      }  // for each micro-op
   }  // while nqueue not empty

   for (kid = tarj->TarjInners(node) ; kid != RIFG_NIL ; 
                kid = tarj->TarjNext(kid))
   {
      if (tarj->NodeIsLoopHeader(kid))
      {
         computeStrideFormulasForScope(sslice, rslice, kid, tarj, mCfg, level+1,
                refFormulas, memRefs);
      }
   }
}

void
Routine::computeStrideFormulasForRoutine(RIFGNodeId node, TarjanIntervals *tarj, 
            MiamiRIFG* mCfg, int marker, int level, ReferenceSlice *rslice)
{
   CFG::Node* b;
   int kid;

   b = static_cast<CFG::Node*>(mCfg->GetRIFGNode(node));
   b->markWith(marker);
   
   // I need to iterate over the nodes twice. First time process only acyclic
   // blocks. Mark them with current marker, but do not 
   // go recursivly inside loops or irreducible intervals (?).
   for (kid = tarj->TarjInners(node) ; kid != RIFG_NIL ; 
                kid = tarj->TarjNext(kid))
   {
      if (! tarj->NodeIsLoopHeader(kid))
      {
         b = static_cast<CFG::Node*>(mCfg->GetRIFGNode(kid));
         // nothing to do here
         b->markWith(marker);
      }

      if (tarj->NodeIsLoopHeader(kid))
      {
         int tmarker = tarj->LoopIndex(kid);
         computeStrideFormulasForRoutine(kid, tarj, mCfg, tmarker, level+1, rslice);
      }
   }

   if (level>0)
   {
      // build stride formulas for this scope
      // do not compute strides if scope is a routine body
      CFG *cfg = ControlFlowGraph();
      RFormulasMap &refFormulas = *rFormulas;
      StrideSlice sslice(cfg, refFormulas, marker);
      RefInfoMap memRefs;
#if DEBUG_STATIC_MEMORY
      DEBUG_STMEM(1,
         CFG::Node *root_b = static_cast<CFG::Node*>(mCfg->GetRIFGNode(node));
         fprintf(stderr, "\n**** Computing stride formulas relative to scope at level %d, marker %d, head block"
              " %d [0x%" PRIxaddr ",0x%" PRIxaddr "] ****\n", 
                    level, marker, root_b->getId(), 
                    root_b->getStartAddress(), root_b->getEndAddress());
      )
#endif
      computeStrideFormulasForScope(sslice, rslice, node, tarj, mCfg, 0, refFormulas, memRefs);
      
      // Now, I have to traverse the list of memory references, and for those that have 
      // the potential of being indirect, clarify if the strde is truly indirect
      RefInfoMap::iterator rit = memRefs.begin();
      for ( ; rit!=memRefs.end() ; ++rit)
      {
         RefFormulas* rf = refFormulas(rit->second.pc, rit->second.opidx);
         assert (rf || !"A RefFormulas entry should have been allocated already for this memory operand");
         assert ((int)(rf->strides.size()) > rit->second.level);
         GFSliceVal& formula = rf->strides[rit->second.level];
         assert (! formula.is_uninitialized());
         if (formula.has_indirect_access() == 2)
         {
            clarifyIndirectAccessForRef(rit->second, formula, refFormulas, memRefs);
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(2,
               cerr << ">$ (" << rit->second.level << ") Memory instruction at address 0x" 
                    << hex << rit->second.pc << dec << " / " << rit->second.opidx
                    << " has clarified stride: " << formula << endl;
            )
#endif
         }
      }
   }  // not zero
}

int
Routine::main_analysis(ImageScope *prog, const MiamiOptions *_mo)
{
   int ires;
   // compute execution frequency of every edge and block
   CFG *cfg = ControlFlowGraph();
//   computeSchedule = _computeSchedule;
   mo = _mo;
   
   std::cerr << "[INFO]Routine::main_analysis(): '" << name << "'\n";
   if (name.compare(mo->debug_routine) == 0)  // they are equal
   {
      // draw CFG
      cfg->draw_CFG(name.c_str(), 0, mo->debug_parts);
   }
   
   /* compute BB ad edge frequencies */
   if (mo->do_cfgcounts)
   {
      ires = cfg->computeBBAndEdgeCounts();
      if (ires < 0)
      {
         fprintf (stderr, "Error while computing the basic block and edge counts\n");
         return (-1);
      }
   }

   createBlkNoToMiamiBlkMap(cfg);
   
   MiamiRIFG mCfg(cfg);
   TarjanIntervals tarj(mCfg);  // computes tarjan intervals
   RIFGNodeId root = mCfg.GetRootNode();
   cfg->set_node_levels(root, &tarj, &mCfg, 0);
   
   // for DEBUGGING
#if 0 // begin debug1
   BaseSlice bslice(cfg, *rFormulas);
   register_info ri;
   CFG::NodesIterator nit(*cfg);
   while ((bool)nit)
   {
      CFG::Node *b = (CFG::Node*)nit;
      UopCodeCache *ucc = b->MicroOpCode();
      int num_uops = ucc->NumberOfUops();
      for (int i=0 ; i<num_uops ; ++i)
      {
         const uOp_t& uop = ucc->UopAtIndex(i);
         if (uop.idx == 0)
            fprintf(stderr, "-----------------------------------\n");
         
         switch(bslice.SliceEffect(b, uop, ri))
         {
            case SliceEffect_EASY:
               fprintf (stderr, "*$#@ EASY");
               break;
               
            case SliceEffect_NORMAL:
               fprintf (stderr, "*$#@ NORMAL");
               break;
            
            case SliceEffect_HARD:
               fprintf (stderr, "*$#@ HARD");
               break;
            
            case SliceEffect_IMPOSSIBLE:
               fprintf (stderr, "*$#@ IMPOSSIBLE");
               break;
            
            default:
               fprintf (stderr, "*$#@ UNKNOWN");
               break;
         }
         fprintf (stderr, " instruction found, at pc 0x%" PRIxaddr ", uop %d of type %s, primary? %d:\n", 
                       uop.dinst->pc, uop.idx, Convert_InstrBin_to_string(uop.iinfo->type), uop.iinfo->primary);
         if (uop.iinfo->primary)
         {
            debug_decode_instruction_at_pc((void*)(uop.dinst->pc+InLoadModule()->RelocationOffset()), 
                        uop.dinst->len);
            DumpInstrList(uop.dinst);
         }
      }
      fprintf(stderr, "====================================\n");
      
      ++ nit;
   }
#endif // end debug1
#if 0  // begin debug2
   CFG::NodesIterator nit(*cfg);
   while ((bool)nit)
   {
      CFG::Node *b = (CFG::Node*)nit;
      UopCodeCache *ucc = b->MicroOpCode();
      int num_uops = ucc->NumberOfUops();
      for (int i=0 ; i<num_uops ; ++i)
      {
         const uOp_t& uop = ucc->UopAtIndex(i);
         if (uop.idx == 0)
            fprintf(stderr, "-----------------------------------\n");
         fprintf (stderr, " instruction found, at pc 0x%" PRIxaddr ", uop %d of type %s, primary? %d:\n", 
                       uop.dinst->pc, uop.idx, Convert_InstrBin_to_string(uop.iinfo->type), uop.iinfo->primary);
         if (uop.iinfo->primary)
         {
            debug_decode_instruction_at_pc((void*)(uop.dinst->pc+InLoadModule()->RelocationOffset()), 
                        uop.dinst->len);
            DumpInstrList(uop.dinst);
         }
      }
      fprintf(stderr, "====================================\n");
      
      ++ nit;
   }
#endif // end debug2
#if 0  // begin debug3
   CFG::NodesIterator nit(*cfg);
   while ((bool)nit)
   {
      CFG::Node *b = (CFG::Node*)nit;
      UopCodeCache *ucc = b->MicroOpCode();
      int num_uops = ucc->NumberOfUops();
      for (int i=0 ; i<num_uops ; ++i)
      {
         const uOp_t& uop = ucc->UopAtIndex(i);
         if (uop.idx == 0)
            fprintf(stderr, "-----------------------------------\n");
         InstrBin itype = uop.iinfo->type;
         if (itype==IB_branch || itype==IB_br_CC || itype==IB_jump)
         {
            fprintf (stderr, ">> Branch found, at pc 0x%" PRIxaddr ", uop %d of type %s, primary? %d:\n", 
                          uop.dinst->pc, uop.idx, Convert_InstrBin_to_string(itype), uop.iinfo->primary);
            debug_decode_instruction_at_pc((void*)(uop.dinst->pc+InLoadModule()->RelocationOffset()), 
                        uop.dinst->len);
            DumpInstrList(uop.dinst);
            GFSliceVal targetF;
            if (!ComputeBranchTargetFormula(uop.dinst, uop.iinfo, i, uop.dinst->pc, targetF))
            {
               cerr << "Branch target: " << targetF << endl;
            }
         }
      }
      fprintf(stderr, "====================================\n");
      ++ nit;
   }
#endif // end debug3
   
   if (mo->do_scopetree)
   {
      // call mark_loop_back_edges after computing the tarjan intervals
      CFG::AddrEdgeMMap *entryEdges = new CFG::AddrEdgeMMap ();
      CFG::AddrEdgeMMap *callEntryEdges = new CFG::AddrEdgeMMap ();
      cfg->mark_loop_back_edges(root, &tarj, &mCfg, entryEdges, callEntryEdges);
      cfg->build_loops_scope_tree(&tarj, &mCfg);
      
      // I should add the edges from the cfg_entry block to the entryEdges map
      CFG::Node *root_b = cfg->CfgEntry();
      addrtype startAddr = root_b->getStartAddress();
      CFG::OutgoingEdgesIterator oeit(root_b);
      while ((bool)oeit)
      {
         CFG::Edge *oute = (CFG::Edge*)oeit;
         entryEdges->insert (CFG::AddrEdgeMMap::value_type (startAddr, oute));
         ++oeit;
      }
      
      // (re)compute node ranks after tarjan analysis to get consistent rank numbers
      // for irreducible intervals
      cfg->ComputeNodeRanks();

      std::string file, func;
      int32_t lineNumber1 = 0, lineNumber2 = 0;
      if (mo->do_linemap)
         InLoadModule()->GetSourceFileInfo(start_addr, start_addr+size,
                     file, func, lineNumber1, lineNumber2);
      
      // check if we have a file scope created already
      int maxname = MIAMIP::get_max_demangled_name_size();
      assert(maxname < 100000);
      char *outbuf = new char[maxname];
      std::string rName = std::string(MIAMIP::get_best_function_name(name.c_str(), outbuf, maxname));
      std::string fName = (file.length()>0)?file:std::string("unknown-file");
      delete[] outbuf;

      addrtype _key;
      if (sizeof(addrtype)==8)
         _key = MIAMIU::CRC64Hash::Evaluate(fName.c_str(), fName.length());
      else
         _key = MIAMIU::CRC32Hash::Evaluate(fName.c_str(), fName.length());
         
#if DEBUG_PROG_SCOPE
      fprintf (stderr, "File %s has CRC hash %p, demangled routine name %s, lines [%d,%d]\n", 
            fName.c_str(), (void*)_key, rName.c_str(), lineNumber1, lineNumber2);
      cerr << "Parent scope: " << prog->ToString() << " has following children:" << endl;
      CodeScope::iterator tmpit = prog->begin();
      for ( ; tmpit!=prog->end() ; ++tmpit)
         cerr << "    - " << tmpit->second->ToString() << " with key " 
              << hex << tmpit->first << dec << endl;
#endif

      CodeScope::iterator siit = prog->find(_key);
      FileScope *fscope;
      if (siit != prog->end ())  // found a file with this CRC
         fscope = static_cast<FileScope*> (siit->second);
      else
      {
         fscope = new FileScope (prog, fName, _key);
#if DEBUG_PROG_SCOPE
         fprintf (stderr, "New FileScope, fname=%s, xml=%s\n", fName.c_str(), 
                fscope->XMLScopeHeader().c_str());
#endif
      }
      assert (fscope != NULL);
      RoutineScope *rscope = new RoutineScope (fscope, rName, start_addr);
      if (rscope == 0) {
          fprintf (stderr, "Cannot allocate new scope (routine)!!!\n");
          exit (-11);
      }
      unsigned int findex = mdriver.GetFileIndex(fName);
#if DEBUG_PROG_SCOPE
      cerr << "Routine " << rName << " is in file >" << fName
           << "< with index " << findex << endl;
#endif
      rscope->addSourceFileInfo(findex, func, lineNumber1, lineNumber2);
      rscope->setMarkedWith(tarj.LoopIndex(root));
      // add lines in the global map as well
      mdriver.AddSourceFileInfo(findex, lineNumber1, lineNumber2);
#if PROFILE_SCHEDULER
      MIAMIP::report_time (stderr, "Initialize data for routine %s", rName.c_str());
#endif


      if (name.compare(mo->debug_routine) == 0)  // they are equal
      {
         CfgToDot(&mCfg, &tarj, 0, mo->debug_parts);
      }
      
      if (mo->do_staticmem)
      {
         // build base location formulas first; use topological traversal of the CFG, 
         // so that when I analyze an instruction, all references on all paths to that 
         // instruction are already analyzed (have symbolic formulas)
         ReferenceSlice *rslice = 0;
#if DEBUG_STATIC_MEMORY
         DEBUG_STMEM(1,
            fprintf(stderr, "\n**** Computing base formulas for routine [%s / %d] ****\n",
                    rscope->ToString().c_str(), 0);
         )
#endif
         RFormulasMap &refFormulas = *rFormulas;
         rslice = new ReferenceSlice(cfg, refFormulas);
         computeBaseFormulas(rslice, cfg, refFormulas);
         
         // build stride formulas now
         computeStrideFormulasForRoutine(root, &tarj, &mCfg, tarj.LoopIndex(root), 0, rslice);
         if (rslice) {
            delete (rslice);
            rslice = 0;
         }
         
#if PROFILE_SCHEDULER
         MIAMIP::report_time (stderr, "Compute symbolic formulas for routine %s", rName.c_str());
#endif
      }
      // pass level info as well.
      if (build_paths_for_interval (rscope, root, &tarj, &mCfg, tarj.LoopIndex(root), 0,
                1 /*no_fpga_acc */, entryEdges, callEntryEdges))
      {
          fprintf (stderr, "==>>> ERROR: ROUTINE_SCOPE has been deleted by build_path_for_intervals, but program continues with bad pointer!!! <<<==\n");
          fflush (stderr);
      }
         
#if BUILD_PATHS

#if NOT_NOW  /* cannot do this until I have static analysis of memory references */
      // compute hot foot print information for each routine
      computeHotFootPrintRecursively (rscope);
#endif
      // deallocate unnecessary data structures
      recursivelyTrimFatInPaths (rscope);
#endif   // NOT_NOW

      delete (entryEdges);
      delete (callEntryEdges);
   } else  // do_scopetree == false
   {
      // do not build scope trees. However, test if we have to do
      // some other type of static analysis: instruction mix, stream reuse bining ...
      if (mo->do_idecode || mo->do_streams || mo->do_ref_scope_index)
      {
         AddrIntSet scopeMemRefs;
         
         // Get an index for this "scope" first
         // Since we are not building a scope tree, I will break down counts
         // at the load module level at best. I just use an ID of 0 as the 
         // scope Id, and then record the image ScopeImplementation* with the 0 ID??
         int32_t scopeId = 0;
         LoadModule *lm = InLoadModule();
         lm->SetSIForScope(scopeId, prog);
         prog->setScopeId(scopeId);
         addrtype reloc = lm->RelocationOffset();
         
         // traverse all blocks and decode the instructions in them
         // I have to iterate over all the blocks
         CFG::NodesIterator nnit(*cfg);
         while ((bool)nnit)
         {
            RefIClassMap refsClass;  // I am not using this information, but 
                                     // I need to pass an appropriate parameter
            CFG::Node *b = (CFG::Node*)nnit;
            if (b->Size()>0 && (b->ExecCount()>0 || mo->do_ref_scope_index)) {
               decode_instructions_for_block(prog, b, b->ExecCount(), scopeMemRefs, refsClass);
            }
            ++ nnit;
         }
         if (mo->do_ref_scope_index)
         {
            AddrIntSet::iterator ais = scopeMemRefs.begin();
            for ( ; ais!=scopeMemRefs.end() ; ++ais)
            {
               // Get a unique index for this memory micro-op
               int32_t iidx = lm->AllocateIndexForInstPC(ais->first+reloc, ais->second);
               // set also the scope index for this reference
               lm->SetScopeIndexForReference(iidx, scopeId);
            }
            // I should also group references into sets, based on their strides
            // However, I cannot do that when the scope tree is not built
         }
         
         if (mo->do_streams)
         {
            // I need to get the set of memory refs in this scope
            LoadModule *img = InLoadModule();
            RFormulasMap &refFormulas = *rFormulas;
            mdriver.compute_stream_reuse_for_scope(mdriver.fstream, prog, img->ImageID(), 
                  scopeMemRefs, refFormulas, lm->BaseAddr()+lm->LowOffsetAddr()-reloc);
         }
      }
   }  // if/else do_scopetree

   DeleteControlFlowGraph();
   return (0);
}

/* recursively recover executed paths through loops and loop nests 
 */
int
Routine::build_paths_for_interval (ScopeImplementation *pscope, RIFGNodeId node, 
            TarjanIntervals *tarj, MiamiRIFG* mCfg, int marker, int level, 
            int no_fpga_acc, CFG::AddrEdgeMMap *entryEdges, CFG::AddrEdgeMMap *callEntryEdges)
{
   CFG::Node* b;
   int kid;
   int is_zero = 1;

   std::cerr << "[INFO]Routine::build_paths_for_interval(): '" << name << "'\n";
   b = static_cast<CFG::Node*>(mCfg->GetRIFGNode(node));
   b->markWith(marker);
   
   // Assign a unique ID to each scope in a LoadModule
   // I identify scopes by their starting offset and level.
   // This informations should be repeatable from to run. In the absence 
   // of fully resolving indirect jumps, the CFGs may be incomplete, but
   // I will try to find a way of choosing an appropriate scope in those
   // situations. 
   // However, another problem is caused by irreducible intervals. Irreducible
   // intervals are characterized by having multiple nodes that can act as
   // entry points. The Tarjan analysis picks one of the entry nodes to
   // act as loop header, and I use that node's starting address to identify
   // the scope. This may cause irreducible intervals to be identified by
   // different starting addresses. I need to assign an ID to each possible
   // starting address, and associate the interval's ScopeImplementation 
   // object to each ID.
   LoadModule *img = InLoadModule();
   addrtype saddress = 0, reloc = img->RelocationOffset();
   int rlevel = 0;
   if (b->isCfgEntry())
      saddress = Start();
   else
   {
      saddress = b->getStartAddress();
      rlevel = tarj->GetLevel(node);
   }
   int32_t nodeScopeId = img->AllocateIndexForScopePC(saddress+reloc, rlevel);
   // In memreuse we also set the parent of a scope. Do we need this info??
   // Decide later. However, I want to record the pscope associated with a scope ID.
   img->SetSIForScope(nodeScopeId, pscope);  // set ScopeImplementation for this scope
   pscope->setScopeId(nodeScopeId);
//   img->SetParentForScope(nodeScopeId, parentId);

   if (b->ExecCount() > 0)
      is_zero = 0;
   if (mo->do_linemap && b->Size()>0 && (b->ExecCount()>0 || b->Size()>5))
      compute_lineinfo_for_block(pscope, b);
      
   // I need to iterate over the nodes twice. First time process only acyclic
   // blocks. Mark them with current marker and compute line info, but do not 
   // go recursivly inside loops or irreducible intervals (?).
   // In the second pass process interval and irreducible nodes recursively.
   // I need this two pass process because I want to compute the line info
   // for outer blocks first. Inside loops I use the end line number of
   // routines, therefore I need to compute it before I process any inner loop.
   for (kid = tarj->TarjInners(node) ; kid != RIFG_NIL ; 
                kid = tarj->TarjNext(kid))
   {
//      if (tarj->IntervalType(kid) == RI_TARJ_ACYCLIC)   // not a loop head
      if (! tarj->NodeIsLoopHeader(kid))
      {
         b = static_cast<CFG::Node*>(mCfg->GetRIFGNode(kid));
         // nothing to do here
         b->markWith(marker);
         if (mo->do_linemap && b->Size()>0 && (b->ExecCount()>0 || b->Size()>5))
            compute_lineinfo_for_block(pscope, b);
         if (b->ExecCount() > 0)
            is_zero = 0;
      
         // If the kid is part of an Irreducible interval, then check to
         // see if it has any entry edges. In that case, assign a new
         // scope ID for this scope using the block's starting address
         if (tarj->getNodeType(node) == TarjanIntervals::NODE_IRREDUCIBLE)  // (natural) loop head
         {
            // iterate over the block's incoming edges
            RIFGEdgeId inEdge;
            RIFGEdgeIterator* ei = mCfg->GetEdgeIterator(kid, ED_INCOMING);
            for (inEdge = ei->Current(); inEdge != RIFG_NIL; inEdge = (*ei)++)
            {
               RIFGNodeId src = mCfg->GetEdgeSrc (inEdge);
               int lEnter = 0, lExit = 0;   // store number of levels crossed
               tarj->tarj_entries_exits (src, kid, lEnter, lExit);
               if (lEnter > 0)  // it is an interval entry edge
               {
                  saddress = b->getStartAddress();
                  int32_t altScopeId = img->AllocateIndexForScopePC(saddress+reloc, rlevel);
                  // In memreuse we also set the parent of a scope. Do we need this info??
                  // Decide later. However, I want to record the pscope associated with a scope ID.
                  img->SetSIForScope(altScopeId, pscope);  // assign ScopeImplementation for this scope
                  //img->SetParentForScope(altScopeId, parentId);
               }
            }
            delete ei;
         }
      }
   }

   for (kid = tarj->TarjInners(node) ; kid != RIFG_NIL ; 
                kid = tarj->TarjNext(kid))
   {
      if (tarj->NodeIsLoopHeader(kid))
      {
         int tmarker = tarj->LoopIndex(kid);
         LoopScope *lscope = new LoopScope (pscope, tarj->GetLevel(kid), 
                         tmarker, tmarker);
         if (lscope == 0) {
            fprintf (stderr, "Cannot allocate new scope (loop)!!!\n");
            exit (-11);
         }
         lscope->setMarkedWith(tmarker);
//         assert (lscope->LoopNode() != 0);
//         lscope->LoopNode()->markWith(tmarker);
         
         if (!build_paths_for_interval (lscope, kid, tarj, mCfg, tmarker, level+1,
                   no_fpga_acc, entryEdges, callEntryEdges))
         {
            RoutineScope *rscope = (RoutineScope*)lscope->Ancestor(ROUTINE_SCOPE);
            assert (rscope!=NULL || !"Interval does not have ROUTINE ancestor");
            pscope->addSourceFileInfo(lscope->getFileIndex(), rscope->RoutineName(),
                 lscope->getStartLine(), lscope->getEndLine());
            // these lines come from the inner loop. I should have already
            // added them in the global map. Do not add them again.
//            pscope->setStartLine (lscope->getStartLine());
//            pscope->setEndLine (lscope->getEndLine());
         }
      }
   }
   
   CFG::Node *root_b = static_cast<CFG::Node*>(mCfg->GetRIFGNode(node));
   if (!is_zero || mo->do_ref_scope_index)
   {
      if (mo->do_streams || mo->do_idecode || mo->do_ref_scope_index)
      {
         AddrIntSet scopeMemRefs;
         RefIClassMap refClasses;
         
         // traverse all blocks and decode the instructions in them
         if (root_b->Size()>0 && (root_b->ExecCount()>0 || mo->do_ref_scope_index)) {
            decode_instructions_for_block(pscope, root_b, root_b->ExecCount(), 
                       scopeMemRefs, refClasses);
         //, nodeScopeId);
	 }
         for (kid = tarj->TarjInners(node) ; kid != RIFG_NIL ; 
                  kid = tarj->TarjNext(kid))
            if (! tarj->NodeIsLoopHeader(kid))
            {
               b = static_cast<CFG::Node*>(mCfg->GetRIFGNode(kid));
               if (b->Size()>0 && (b->ExecCount()>0 || mo->do_ref_scope_index))
                  decode_instructions_for_block(pscope, b, b->ExecCount(), 
                         scopeMemRefs, refClasses);
            }
            
         if (mo->do_ref_scope_index)
         {
            // use a temporary data-structure in which we include the first reference
            // of each set. When we try to find equivalencies for a new ref, we
            // only have to compare against the representatives of each set. All
            // refs in a set must have the same type, name, strides, ...
            // Use a map, where the value associated with a key is the ref's index.
            AddrIntSet setRepresentatives;
            addrtype reloc = img->RelocationOffset();
            AddrIntSet::iterator ais = scopeMemRefs.begin(), ris;
            for ( ; ais!=scopeMemRefs.end() ; ++ais)
            {
               // Get a unique index for this memory micro-op
               int32_t iidx = img->AllocateIndexForInstPC(ais->first+reloc, ais->second);
               // set also the scope index for this reference
               img->SetScopeIndexForReference(iidx, nodeScopeId);
               
               // I should also group references into sets, based on their strides
               // Aggregate references based on their type (load/store, int/fp,
               // 32/64/128/... bits), their strides, and their corresponding 
               // source code names.
               // Should I also aggregate references with 0 strides? Yeah, why not.
               const char *var_name = ComputeObjectNameForRef(ais->first, ais->second);
               
               // I need to get the full type of a reference. Use an InstructionClass?
               // I have the refClasses map now. Use it to compare instruction types.
               InstructionClass &iclass = refClasses[*ais];
               assert (iclass.isValid());  // we should have class information for this reference
               
               // Get the symbolic forulas for the reference
               RefFormulas* rf = rFormulas->hasFormulasFor(ais->first, ais->second);
               int numStrides = 0;
               if (rf)
                  numStrides = rf->NumberOfStrides();
                  
               bool found = false;
               // iterate over all representatives previously found
               for (ris=setRepresentatives.begin() ; 
                    !found && ris!=setRepresentatives.end() ; 
                    ++ris)
               {
                  InstructionClass o_iclass = refClasses[*ris];
                  // check types first, skip remaining tests if not equal
                  if (! iclass.equals(o_iclass))
                     continue;
                     
                  // Get the symbolic forulas for the reference
                  RefFormulas* orf = rFormulas->hasFormulasFor(ris->first, ris->second);
                  int o_numStrides = 0;
                  if (orf)
                     o_numStrides = orf->NumberOfStrides();
                  // they should have equal numbers of strides, because they come
                  // from the same loop
                  if (numStrides != o_numStrides)
                  {
                     fprintf(stderr, "ERROR: References at locations <0x%" PRIxaddr ",%d> and <0x%"
                            PRIxaddr ",%d> from the same loop, have different number of strides, %d and %d, respectivelly."
                            " Problem likely caused by cached symbolic formulas from a different input, and variations in the CFGs between the inputs."
                            " This loop is located at offset 0x%" PRIxaddr ", level %d in routine %s.\n",
                            ais->first, ais->second, ris->first, ris->second,
                            numStrides, o_numStrides, 
                            (root_b->isCfgEntry() ? Start() : root_b->getStartAddress()),
                            tarj->GetLevel(node), Name().c_str());
                     fprintf(stderr, "Delete the associated <imageName-imageHash.stAn> file for this Image and run again.\n");
                            
                     assert (numStrides == o_numStrides);
                  }
                  bool eq_strides = true;
                  for (int ss=0 ; ss<numStrides && eq_strides ; ++ss)
                     if  (! FormulasAreIdentical(rf->strides[ss], 
                                                orf->strides[ss]))
                        eq_strides = false;
                  if (! eq_strides)
                     continue;
                  
                  // Finally, compare their names
                  // Names are associated with set indecies, oops
                  int32_t oidx = img->AllocateIndexForInstPC(ris->first+reloc, ris->second);
                  int32_t setIdx = img->GetSetIndexForReference(oidx);
                  const char* oname = img->GetNameForSetIndex(setIdx);
                  if (! strcmp(var_name, oname))  // names are equal, hurrah
                  {
                     found = true;  // we'll exit the loop on this flag
                     // add this ref to the found set
                     img->AddReferenceToSet(iidx, setIdx);
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(2,
                        fprintf(stderr, ">> Memory refer at 0x%" PRIxaddr " / %d has the same type (%s), name (%s) and pattern as ref at 0x%" PRIxaddr " / %d\n",
                            ais->first, ais->second, iclass.ToString().c_str(), 
                            var_name, ris->first, ris->second);
                     )
#endif
                  }
               }  // for each of previously found sets
               
               if (! found)  // no equivalencies, start a new set
               {
                  // add reference info to the set of representatives
                  setRepresentatives.insert(*ais);
                  // start a new set
                  int32_t newset = img->AddReferenceToSet(iidx);
                  // set variable name for this set
                  img->SetNameForSetIndex(newset, var_name);
#if DEBUG_STATIC_MEMORY
                  DEBUG_STMEM(2,
                     fprintf(stderr, ">> Memory refer at 0x%" PRIxaddr " / %d of type (%s), name (%s), starts a new set %d\n",
                         ais->first, ais->second, iclass.ToString().c_str(), 
                         var_name, newset);
                  )
#endif
                  
                  // for new sets, I need to record irregular access information
                  // for later use.
                  for (int ss=0 ; ss<numStrides ; ++ss)
                     if  (rf->strides[ss].has_irregular_access() ||
                          rf->strides[ss].has_indirect_access())
                        img->SetIrregularAccessForSet(newset, ss);
               }
            }  // for each mem ref
         }  // if do_ref_scope_index
         
         if (mo->do_streams)
         {
            // I need to get the set of memory refs in this scope
            // the address field of the memrefs array is modified by the 
            // compute_stream_reuse routine
            RFormulasMap &refFormulas = *rFormulas;
            mdriver.compute_stream_reuse_for_scope(mdriver.fstream, pscope, img->ImageID(), 
                  scopeMemRefs, refFormulas, img->BaseAddr()+img->LowOffsetAddr()-reloc);
         }
      }
      
      if (mo->do_cfgpaths){
         constructPaths(pscope, root_b, marker, no_fpga_acc, entryEdges,
			callEntryEdges);
      }
      if(mo->do_mycfgpaths){
         myConstructPaths(pscope,no_fpga_acc,mo->block_path);
      }

      return (0);
   } else
   {
#if DEBUG_PROG_SCOPE
      fprintf (stderr, "DELETING SCOPE\n");
#endif
      if (mo->do_cfgpaths){
         constructPaths(pscope, root_b, marker, no_fpga_acc, entryEdges,
			callEntryEdges);
      }
      if(mo->do_mycfgpaths){
         myConstructPaths(pscope,no_fpga_acc,mo->block_path);
      }
      return (0);
   }
   return (0);
}

const char * 
Routine::ComputeObjectNameForRef(addrtype pc, int32_t memop)
{
   // This functionality is not implemented yet. I need to get certain
   // information about variables from Dwarf. FIXME
   return ("~-TODO-~");
}

void
Routine::decode_instructions_for_block (ScopeImplementation *pscope, CFG::Node *b, int64_t count,
      AddrIntSet& memRefs, RefIClassMap& refsClass)
{
   MIAMI::DecodedInstruction dInst;
   MIAMI::DecodedInstruction dInst2;   
   LoadModule *lm = InLoadModule();
   addrtype reloc = lm->RelocationOffset();
   // iterate over variable length instructions. Define an architecture independent API
   // for common instruction decoding tasks. That API can be implemented differently for
   // each architecture type
   ICIMap& scopeImix = pscope->getInstructionMixInfo();
   CFG::ForwardInstructionIterator iit(b);
   while ((bool)iit)
   {
      addrtype pc = iit.Address();
      int res = InstructionXlate::xlate_dbg(pc+reloc, b->getEndAddress()-pc, &dInst, "Routine::decode_instructions_for_block");

      // DynInst-based decoding
#if 0
      BPatch_basicBlock* dyn_blk = getBlockFromAddr(b->getStartAddress(), b->getEndAddress());
      if (!dyn_blk) {
	fprintf(stdout,"dyninst: blk not found\n");
      }

      MIAMI::DecodedInstruction dInst2;
      res = InstructionXlate::xlate_dyninst(pc+reloc, b->getEndAddress()-pc, &dInst2, dyn_func, dyn_blk);
#endif

#if 0//FIXME: deprecated
      MIAMI::DecodedInstruction* dInst2 = new MIAMI::DecodedInstruction(); // FIXME:tallent memory leak
      int res1 = isaXlate_insn_old(pc+reloc, dInst2);
      if (res1 != 0 || dInst2->micro_ops.size() == 0) {
         std::cout << "Routine::decode_instructions_for_block:error!\n";
      }
#endif

      if (res < 0) { // error while decoding
         return;
      }

      // Now iterate over the micro-ops of the decoded instruction
      MIAMI::InstrList::iterator lit = dInst.micro_ops.begin();
      for (int i=0 ; lit!=dInst.micro_ops.end() ; ++lit, ++i)
      {
         MIAMI::instruction_info& iiobj = (*lit);
         InstructionClass iclass;

         if ( (mo->do_ibins && count>0) || 
              (mo->do_ref_scope_index && InstrBinIsMemoryType(iiobj.type))
            )
         {
            iclass.Initialize(iiobj.type, iiobj.exec_unit, 
                    iiobj.width*iiobj.vec_len, iiobj.exec_unit_type,
                    iiobj.width);
            if (mo->do_ibins)
            {
               ICIMap::iterator imit = scopeImix.find(iclass);
               if (imit != scopeImix.end())
                  imit->second += count;
               else
                  scopeImix.insert(ICIMap::value_type(iclass, count));
            }
         } else
            iclass.Initialize();  // default initialization; not used, but to appease the compiler
         
         if (InstrBinIsMemoryType(iiobj.type))
         {
            if (mo->do_streams || mo->do_ref_scope_index)
            {
               int opidx = iiobj.get_memory_operand_index();
               assert(opidx >= 0);
               memRefs.insert(AddrIntPair(pc, opidx));
               
               if (mo->do_ref_scope_index)
                  refsClass.insert(RefIClassMap::value_type(
                        AddrIntPair(pc, opidx), iclass));
            }
         }

      }
      ++ iit;
   }
}

void
Routine::compute_lineinfo_for_block (ScopeImplementation *pscope, CFG::Node *b)
{
   std::string func, file;
   int32_t lineNumber1 = 0, lineNumber2 = 0;
   
#if DEBUG_COMPUTE_LINEINFO
   std::cerr << "LineInfo for block [" << std::hex << b->getStartAddress() 
             << "," << b->getEndAddress() << "]" << std::dec << " exec-count " 
             << b->ExecCount() << std::endl;
#endif
   // iterate over variable length instructions. Define an architecture independent API
   // for common instruction decoding tasks. That API can be implemented differently for
   // each architecture type
   CFG::ForwardInstructionIterator iit(b);
   while ((bool)iit)
   {
      addrtype pc = iit.Address();
      InLoadModule()->GetSourceFileInfo(pc, 0, file, func, lineNumber1, lineNumber2);
      unsigned int findex = mdriver.GetFileIndex(file);
#if DEBUG_PROG_SCOPE
      cerr << "Scope in routine " << name << " is in file >"
           << file << "< with index " << findex << endl;
#endif
      mdriver.AddSourceFileInfo(findex, lineNumber1, lineNumber2);
      pscope->addSourceFileInfo(findex, func, lineNumber1, lineNumber2);
      ++ iit;
   }
}


static PathID targetPath(0x4309a0, 2, 1);


void split(const std::string &s, char delim, std::vector<int> &elems) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(stoi(item));
    }
}

void
Routine::myConstructPaths(ScopeImplementation *pscope, int no_fpga_acc, const string& spath)
{
   std::cerr << "[INFO]Routine::constructPaths(): '" << name << "'\n";
   CFG::NodeList ba;
   CFG::EdgeList ea;
   RSIListList rl;
   //unused BMSet& entries = pscope->InnerEntries();
   //unused BMSet& exits = pscope->InnerExits();
   PairRSIMap &pathRegs = pscope->PathRegisters();
   BPMap *bpmtemp = new BPMap();
   LatencyType thisLoopLatency = 0;
//ozgurS   
   float thisLoopMemoryLatency = 0.0;
   float thisLoopCPULatency = 0.0;
//ozgurE
   uint64_t thisLoopUops = 0;
   double thisLoopMemLatency = 0;
//   LatencyType thisLoopInef = 0;

   const Machine *tmach = 0;
   if (mo->has_mdl)
      tmach = mdriver.targets.front();
   //unused CFG *cfg = ControlFlowGraph();

   vector<int> path;
   split(spath,'>',path);
   MIAMIU::FloatArray fa;
   for (unsigned int i = 0; i < path.size(); i++) {
      CFG::Node* blk = (CFG::Node*)(*dyn_blkNoToMiamiBlk)[path[i]];
      ba.push_back(blk);
      fa.push_back(1.0);
   }


   BlockPath *bp = new BlockPath(ba, (CFG::Node*)(*dyn_blkNoToMiamiBlk)[path[path.size()-1]], fa, rl, false);
   bpmtemp->insert(BPMap::value_type(bp, new PathInfo(1)));
   
   /* Temporary method to account for memory stalls and such.
    * Keep track of all memory references in this scope. After all paths
    * are scheduled, I will compute an average IPC for the scope.
    * Next, use the mechanistic model to compute memory stalls and
    * memory overlap.
    */
   AddrSet scopeMemRefs;
   int loopIdx = 1;
   for (BPMap::iterator bpit=bpmtemp->begin() ; bpit!=bpmtemp->end() ; 
	++bpit, ++loopIdx)
   {
      // pathId is 64 bits; Use 32 bits for head block address,
      // 16 bits for loop index, 16 bits for path index in loop
      PathID pathId(bpit->first->blocks[0]->getStartAddress());
      if (pscope->getScopeType() == LOOP_SCOPE)
         pathId.scopeIdx = dynamic_cast<LoopScope*>(pscope)->Index();
      else
      {
         assert(pscope->getScopeType()==ROUTINE_SCOPE);
         pathId.scopeIdx = 1;  // routines have index 1 I think
      }
      pathId.pathIdx = loopIdx;
      bpit->second->pathId = pathId;

      // FIXME: must compute the average number of times the backedge 
      // is taken everytime we enter the loop; Use a dummy value now
      float exit_count = pscope->getExitCount();
      assert (exit_count>0.f || bpmtemp->size()==1);
      if (exit_count<0.0001)
         exit_count = bpit->second->count;

//         float avgCount = pscope->getTotalCount() / pscope->getExitCount();
      float avgCount = bpit->second->count / exit_count; //pscope->getExitCount();
      if (! bpit->first->isExitPath ())
         avgCount += 1.0;

      avgCount = 1.0; //num of iterations 

      // for now just dump the path to check that we can construct them correctly
#if DEBUG_BLOCK_PATHS_PRINT
      DEBUG_PATHS (3,
         bpit->first->Dump(stderr);
      )
#endif
      
      MIAMI_DG::DGBuilder *sch = NULL;
      
#if DEBUG_BLOCK_PATHS_PRINT
      DEBUG_PATHS (2,
         fprintf(stderr, " *** Scheduling pathId %s\n", bpit->second->pathId.Name());
      )
#endif

      if (mo->do_build_dg)
      {
    std::cerr << "[INFO]Routine::constructPaths:schedule: '" << name << "'\n";
         RFormulasMap &refFormulas = *rFormulas;
#if 1
         //sch = new MIAMI_DG::DGBuilder(name.c_str(), pathId, 
         //     1 /*args.optimistic_memory_dep*/, refFormulas,
         //     InLoadModule(),
         // /*        mdriver.RefNames(), mdriver.RefsTable(), */
         //     bpit->first->size, bpit->first->blocks, 
         //     bpit->first->probabs, bpit->first->innerRegs,
         //     bpit->second->count, avgCount);
         sch = new MIAMI_DG::DGBuilder(this, pathId, 
              1 /*args.optimistic_memory_dep*/, refFormulas,
              InLoadModule(),
      /*        mdriver.RefNames(), mdriver.RefsTable(), */
              bpit->first->size, bpit->first->blocks, 
              bpit->first->probabs, bpit->first->innerRegs,
              avgCount, avgCount);
              //bpit->second->count, avgCount);
#if PROFILE_SCHEDULER
         MIAMIP::report_time (stderr, "Graph construction for path %s", pathId.Name());
#endif

         bpit->second->latency = 1;
//ozgurS
         bpit->second->cpulatency = 1;
//ozgurE
         bpit->second->num_uops = 1;
         
         if (mo->do_scheduling)
         {
            sch->updateGraphForMachine (tmach);
            // compute the scheduling, return a pair containing the latency and the number
            // of micro-ops in the path; we can compute IPCs and CPIs from this;
//ozgurS comment out old computeScheduleLatency and use myComputeScheduleLatency 
//            MIAMI_DG::schedule_result_t res = sch->computeScheduleLatency(0 /* args.graph_level */,
//                     mo->DetailedMetrics());
//
            MIAMI_DG::schedule_result_t res = sch->myComputeScheduleLatency(0,mo->DetailedMetrics(), thisLoopMemoryLatency,thisLoopCPULatency);
            bpit->second->cpulatency = thisLoopCPULatency;
            bpit->second->memlatency = thisLoopMemoryLatency;
//ozgurE
            bpit->second->latency = res.first;
            bpit->second->num_uops = res.second;


#if PROFILE_SCHEDULER
            MIAMIP::report_time (stderr, "Compute schedule for path %s", pathId.Name());
#endif
            // if it is an exit path, compute the input/output registers
            if (avgCount<=1)
            {
               MIAMIU::UIPair pathKey (bpit->first->blocks[0]->getStartAddress(), 
                         bpit->first->next_block->getStartAddress());
               PairRSIMap::iterator prit = pathRegs.find (pathKey);
               if (prit == pathRegs.end())  // did not see an exit path with this key
               {
                  prit = pathRegs.insert (pathRegs.begin(), 
                         PairRSIMap::value_type (pathKey, RSIList()) );
                  sch->computeRegisterScheduleInfo (prit->second);
#if PROFILE_SCHEDULER
                  MIAMIP::report_time (stderr, "Compute register mapping for path %s", pathId.Name());
#endif
               }
            }

#if NOT_NOW
            CliqueList &myCliques = bpit->second->cliques;
            HashMapUI &refsDist2Use = bpit->second->dist2use;
            RGList &rGroups = bpit->second->reuseGroups;
            sch->computeMemoryInformationForPath (refsTable, 
                     rGroups, myCliques, refsDist2Use);
            // traverse all the reuse groups and match them with the sets in each
            // memory level
            processReuseGroups (tmach, bpit->second);
            
            // compute the parallelism factor; 
            // initially compute a loose upper bound
            // set the miss rate for each reference to be equal to the execution
            // frequency of the path
            HashMapFP missCounts;
            CliqueList::iterator clit = myCliques.begin();
            for ( ; clit!=myCliques.end() ; ++clit)
               for (int ll=0 ; ll<(*clit)->num_nodes ; ++ll)
               {
                  unsigned int crt_pc = (*clit)->vertices[ll];
                  float &miss_val = missCounts [crt_pc];
                  if (miss_val < 0)
                     miss_val = bpit->second->count; 
               }
            float pFactor = compute_parallelism_factor (myCliques, missCounts,
                  refsDist2Use, bpit->second->serialMemLat, 
                  bpit->second->exposedMemLat);
            cerr << "Path " << hex << pathId << dec << " has parallel factor "
                 << pFactor << endl;
#if PROFILE_SCHEDULER
            MIAMIP::report_time (stderr, "Compute reuse groups and mem parallelism for path %s", pathId.Name());
#endif

#endif  // NOT_NOW
         }  // if do_scheduling
         
#else  // if 1/0
         bpit->second->latency = 1;
         bpit->second->num_uops = 1;
         if (pathId==targetPath)
         {
            sch = new MIAMI_DG::DGBuilder(name.c_str(), pathId, 
                 1 /*args.optimistic_memory_dep*/, refFormulas,
                 InLoadModule(),
     /*            mdriver.RefNames(), mdriver.RefsTable(), */
                 bpit->first->size, bpit->first->blocks, 
                 bpit->first->probabs, bpit->first->innerRegs,
                 bpit->second->count, avgCount);
#if PROFILE_SCHEDULER
            MIAMIP::report_time (stderr, "Graph construction w/ debug for path %s", pathId.Name());
#endif

            char bbuf1[64], bbuf2[64];
            sprintf (bbuf1, "q-%s", pathId.Filename());
            sprintf (bbuf2, "r-%s", pathId.Filename());
            sch->draw_debug_graphics (bbuf1);
            
            if (mo->do_scheduling)
            {
               sch->updateGraphForMachine (tmach);
               sch->draw_debug_graphics (bbuf2);
            
            // draw the dominator trees
//            sch->draw_dominator_trees ("x6");

               // compute the scheduling
               MIAMI_DG::schedule_result_t res = sch->computeScheduleLatency(0 /* args.graph_level */,
                        mo->DetailedMetrics());
               bpit->second->latency = res.first;
               bpit->second->num_uops = res.second;

#if PROFILE_SCHEDULER
               MIAMIP::report_time (stderr, "Compute schedule w/ debug for path %s", pathId.Name());
#endif

               // if it is an exit path, compute the input/output registers
               if (avgCount<=1)
               {
                  UIPair pathKey (bpit->first->blocks[0]->start(), 
                            bpit->first->next_block->start());
                  PairRSIMap::iterator prit = pathRegs.find (pathKey);
                  if (prit == pathRegs.end())  // did not see an exit path with this key
                  {
                     prit = pathRegs.insert (pathRegs.begin(),
                            PairRSIMap::value_type (pathKey, RSIList()) );
                     sch->computeRegisterScheduleInfo (prit->second);
#if PROFILE_SCHEDULER
                     MIAMIP::report_time (stderr, "Compute register mapping w/ debug for path %s", pathId.Name());
#endif
                  }
               }
#if NOT_NOW
               CliqueList &myCliques = bpit->second->cliques;
               HashMapUI &refsDist2Use = bpit->second->dist2use;
               RGList &rGroups = bpit->second->reuseGroups;
               sch->computeMemoryInformationForPath (refsTable, 
                        rGroups, myCliques, refsDist2Use);
               // traverse all the reuse groups and match them with the sets 
               // in each memory level
               processReuseGroups (tmach, bpit->second);

               // compute the parallelism factor; 
               // initially compute a loose upper bound
               // set the miss rate for each reference to be equal to the execution
               // frequency of the path
               HashMapFP missCounts;
               CliqueList::iterator clit = myCliques.begin();
               for ( ; clit!=myCliques.end() ; ++clit)
                  for (int ll=0 ; ll<(*clit)->num_nodes ; ++ll)
                  {
                     unsigned int crt_pc = (*clit)->vertices[ll];
                     float &miss_val = missCounts [crt_pc];
                     if (miss_val < 0)
                        miss_val = bpit->second->count; 
                  }
               float pFactor = compute_parallelism_factor (myCliques, missCounts,
                          refsDist2Use, bpit->second->serialMemLat, 
                          bpit->second->exposedMemLat);
               cerr << "Path " << hex << pathId << dec << " has parallel factor "
                    << pFactor << endl;
#if PROFILE_SCHEDULER
               MIAMIP::report_time (stderr, "Compute reuse groups and mem parallelism (debug) for path %s", 
                        pathId.Name());
#endif

#endif  // NOT_NOW
            } // if do_scheduling
         } // if path == targetPath
#endif   // if 1/0
         double memLat = 0;
         if (sch)
         {
            bpit->second->timeStats = sch->getTimeStats();
//            bpit->second->timeStats.addRetiredUops(bpit->second->num_uops);
            bpit->second->unitUsage = sch->getUnitUsage();
            
            const AddrSet& pathRefs = sch->getMemoryReferences();
            cout<<"memrefs: ";
            
            for (auto i : pathRefs){
               cout<<(unsigned int*)i<<":"<<InLoadModule()->getMemLoadLatency(i)<<" ";
               memLat +=InLoadModule()->getMemLoadLatency(i);
            }
            cout<<endl;
            scopeMemRefs.insert(pathRefs.begin(), pathRefs.end());
            //sch->dump(cout);
            if(mo->palm_dump_flag_set){ 
                  std::ofstream sched_file;
                  sched_file.open(mo->palm_dump_file);
                  sch->draw_scheduling(sched_file, mo->palm_dump_file.c_str());
            }
            sch->printTimeAccount();
            delete sch;
#if PROFILE_SCHEDULER
            MIAMIP::report_time (stderr, "Get time stats and deallocate scheduler for path %s", pathId.Name());
#endif
         }

         std::cout<<"PALM: path lat: "<<bpit->second->latency<<" path uops: "<<bpit->second->num_uops<<" path count: "<<bpit->second->count<<" "<<bpit->second->serialMemLat<<" "<<bpit->second->exposedMemLat<<std::endl;
         thisLoopLatency += (bpit->second->count)*(bpit->second->latency);
//ozgurS
//TODO ask about bit->second->count
//ozgurE
         thisLoopUops += (bpit->second->count)*(bpit->second->num_uops);
         thisLoopMemLatency += memLat;
      }  // if do_build_dg
   }
   
   if (!no_fpga_acc)
   {
#if NOT_NOW
      decodeInstructionsInPath (pscope, bpmtemp);
#endif
#if PROFILE_SCHEDULER
      MIAMIP::report_time (stderr, "Decode instructions for scope %s", 
               pscope->ToString().c_str());
#endif
   }

   pscope->Paths() = bpmtemp;
   std::cout <<"lat: "<< thisLoopLatency << " numuops: "<<thisLoopUops<<" memLat: "<<thisLoopMemLatency<<" memoryLatency: "<<thisLoopMemoryLatency<<" cpuLatency: "<<thisLoopCPULatency<<std::endl;
   pscope->Latency() = thisLoopLatency;
   pscope->NumUops() = thisLoopUops;

   if (mo->do_memlat)
   {
      // once all the paths are processed, compute exclusive memory stalls for this scope
      mdriver.compute_memory_stalls_for_scope(pscope, scopeMemRefs);
   }
}


void
Routine::constructPaths(ScopeImplementation *pscope, CFG::Node *b, int marker,
			int no_fpga_acc, CFG::AddrEdgeMMap *entryEdges, 
			CFG::AddrEdgeMMap *callEntryEdges)
{
   std::cerr << "[INFO]Routine::constructPaths(): '" << name << "'\n";
   CFG::NodeList ba;
   CFG::EdgeList ea;
   RSIListList rl;
   BMSet& entries = pscope->InnerEntries();
   BMSet& exits = pscope->InnerExits();
   PairRSIMap &pathRegs = pscope->PathRegisters ();
   BPMap *bpmtemp = new BPMap();
   LatencyType thisLoopLatency = 0;
   uint64_t thisLoopUops = 0;
//   LatencyType thisLoopInef = 0;

   const Machine *tmach = 0;
   if (mo->has_mdl)
      tmach = mdriver.targets.front();
   CFG *cfg = ControlFlowGraph();
   
   // b should be the loop head; For irreducible intervals we can have more
   // than one head. For this I created the entryEdges data structure which
   // stores all entry edges for an interval. We will use this more general 
   // method. We iterate over all entry edges of a loop and attempt to recover
   // the executed paths. Function call can also result in entry points for
   // some intervals. This happens if I use instrumentation by request. The
   // hooks that I use to start and stop instrumentation are in fact function
   // calls. I do not know if it can happen naturally to have function calls
   // as entry point, but perhaps a call to setjmp can act as an entry 
   // point if we longjmp to it.
   // gmarin 2009/11/06: For irreducible intervals I can have also edges going 
   // into the middle of the loop, which can be the header of a normal loop
   // inside the irreducible interval (the way I construct them). So I must 
   // make the entry / exit blocks of all loops one level inside an irreducible
   // interval avaialble as entry/exit blocks for the parent interval of the
   // irreducible loop.
   
   std::pair <CFG::AddrEdgeMMap::iterator, CFG::AddrEdgeMMap::iterator> _entries = 
           entryEdges->equal_range (b->getStartAddress());
   CFG::AddrEdgeMMap::iterator eit;
   for (eit=_entries.first ; eit!=_entries.second ; ++eit)
   {
      CFG::Edge *in_e = dynamic_cast<CFG::Edge*>(eit->second);
      int64_t inCount = in_e->secondExecCount();
#if DEBUG_BLOCK_PATHS_PRINT
      DEBUG_PATHS (3, 
         fprintf (stderr, "Try entry edge b[%08lx,%08lx] -> b[%08lx,%08lx], with count %ld\n",
               in_e->source()->getStartAddress(), in_e->source()->getEndAddress(),
               in_e->sink()->getStartAddress(), in_e->sink()->getEndAddress(), inCount);
      )
#endif
      if (inCount > 0)
      {
         CFG::Node *in_b = in_e->sink();
         entries.insert (BMarker (in_b, in_b, marker, 0));
         addBlock (pscope, in_b, in_b, in_e, ba, ea, rl, marker, bpmtemp, inCount);
      } 
   }

   // it is possible to have some blocks with non-zero frequency ????
   assert (ba.empty());  // I pop back anything I push.
   assert (ea.empty());
   assert (rl.empty());

   // try also the entry points due to function calls.
   // I wonder how should I process these? I would like to start with the
   // actual function call that is the entry point. It may happen to have
   // several function calls along the path. What if I start from some
   // intermediary one?
   _entries = callEntryEdges->equal_range (b->getStartAddress());
   for (eit=_entries.first ; eit!=_entries.second ; ++eit)
   {
      CFG::Edge *in_e = dynamic_cast<CFG::Edge*>(eit->second);
      int64_t inCount = in_e->ExecCount();
      CFG::Node *in_b = in_e->sink();
      CFG::Node *surrogb = in_e->source();
      CFG::Edge *surrogE = surrogb->firstIncoming();
      int64_t surrogCount = surrogE->ExecCount();
      inCount -= surrogCount;

      if (inCount>0 && in_b->isMarkedWith(marker) && in_b->Size()>0 && 
                 in_b->ExecCount()>0)
      {
#if DEBUG_BLOCK_PATHS_PRINT
         DEBUG_PATHS (3, 
             fprintf (stderr, "Try call return entry edge b[%08lx,%08lx] -> b[%08lx,%08lx], with count %ld, tail block count %ld\n",
                  in_e->source()->getStartAddress(), in_e->source()->getEndAddress(),
                  in_b->getStartAddress(), in_b->getEndAddress(), inCount, in_b->ExecCount());
         )
#endif
         entries.insert (BMarker (in_b, in_b, marker, 0));
         addBlock (pscope, in_b, in_b, in_e, ba, ea, rl, marker, bpmtemp, inCount);
      }
   }

   // it is possible to have some blocks with non-zero frequency ????
   assert (ba.empty());  // I pop back anything I push.
   assert (ea.empty());
   assert (rl.empty());
   
   // There is one case which is not covered by the normal path detection approach
   // When the Tarjan identified loop head has also a self cycling back-edge which 
   // typically would create an inner loop, but the same node cannot be loop head
   // at two different levels. I need to try forming a path from the loop head.
   // This path may not even exit the main loop. We'll improvise something.
   if (b->ExecCount()>0 && !b->isCfgEntry())  // try building paths from this node
   {
      int64_t lhCount = b->ExecCount();
#if DEBUG_BLOCK_PATHS_PRINT
      DEBUG_PATHS (1, 
         fprintf(stderr, "\n**WARNING ConstructPaths**: Found loop head [0x%" PRIxaddr ",0x%" PRIxaddr "] with count %ld not accounted for. Try building a path from the loop head.\n",
                b->getStartAddress(), b->getEndAddress(), lhCount);
      )
#endif
      addBlock (pscope, 0, b, NULL, ba, ea, rl, marker, bpmtemp, lhCount);
   }

   BMSet::iterator it;
   // Check all the exits. If we exit with an edge that enters into an inner loop
   // we must check the exits of that inner loop. We should find an exit from that
   // inner loop that crosses multiple levels
   for (it=exits.begin() ; it!=exits.end() ; )
   {
      ScopeImplementation *inScope = 0;
      int lEntered = 0;
      if ((lEntered=it->dummyE->getNumLevelsEntered())>0 &&
            (inScope=pscope->Descendant (it->b->getMarkedWith()))!=0
         )  // I must check exits from all those levels
      {
#if DEBUG_BLOCK_PATHS_PRINT
         DEBUG_PATHS (3, 
         {
            fprintf(stderr, "\n## Found exit at block 0x%lx (%" PRIu64 ") [marker %u, freq %" PRIu64 " / dummyE freq %" PRIu64 ", id %d], that enters %d levels of inner loops ** ", 
                  it->b->getStartAddress(), it->b->ExecCount(),
                  it->marker, it->freq, it->dummyE->ExecCount(),
                  it->dummyE->getId(), it->dummyE->getNumLevelsEntered() );
            fprintf(stderr, "Attempt to find the actual exit to an outer scope.\n");
         })
#endif
      
         typedef std::list<ScopeImplementation*> ScopeList;
         ScopeList iScopes;   // build a list of entered scopes, sorted from outer to inner most
         int64_t exit_counter = it->dummyE->ExecCount();  // how much count we have to replace
         
#if DEBUG_BLOCK_PATHS_PRINT
         DEBUG_PATHS (5, 
            CFG::Edge* lastE = it->dummyE;
            fprintf(stderr, " Exit edge %p enters %d loop levels\n", 
                lastE, lEntered);
            if (lastE)
               fprintf(stderr, "Exit edge id=%d from B%d[%" PRIxaddr "] to B%d[%" PRIxaddr "]\n",
                     lastE->getId(), lastE->source()->getId(), lastE->source()->getStartAddress(),
                     lastE->sink()->getId(), lastE->sink()->getStartAddress());
         )
#endif
         
         while(inScope)
         {
            // I should not include the current scope. When a loop entry edge enters
            // additional inner loops and I am looking at all the exits, I want to
            // exclude the outermost loop, the one in which we are trying to find paths.
            // Check by marker. If inScope's marker is current loop's marker, then do not
            // include it and stop. We should not consider any loops which are outer this one.
            if ((int)inScope->getMarkedWith() <= marker)
               break;
            iScopes.push_front(inScope);
            if (lEntered > 1)
            {
               lEntered -= 1;
               inScope = inScope->Parent();
            } else
               inScope = 0;
         }
         ScopeList::iterator slit = iScopes.begin();
         for ( ; slit!=iScopes.end() ; )
         {
            inScope = *slit;
            
            BMSet& oldExits = inScope->InnerExits();

            // first, add all exits to a map, ordered by the following criteria:
            // - exist to a node marked with a smaller marker (key = 10)
            // - exits to a node marked with marker  (key = 20)
            // - exits that get me to a node marked with marker ^ MARKER_CODE (key = 50)
            // - exist to a node marked with a larger marker  (key = 50)
            typedef std::multimap<unsigned int, const BMarker*> ExitsMap;
            ExitsMap sExits;  // scope exits
            for (BMSet::iterator oit=oldExits.begin() ; oit!=oldExits.end() ; ++oit)
            {
               const BMarker *bm = &(*oit);
               unsigned int key = 50;
               int bmMarker = bm->b->getMarkedWith();
               if (bmMarker < marker)
                  key = 10;
               else if (bmMarker == marker)
                  key = 20;
               // otherwise, the marker stays at 50
               sExits.insert(ExitsMap::value_type(key, bm));
            }
            
            for (ExitsMap::iterator emit=sExits.begin() ; 
                 emit!=sExits.end() && exit_counter>0 ; 
                 ++emit)
            {
               const BMarker *bm = emit->second;
               if (bm->freq==0 || bm->dummyE->ExecCount()<=0)
               {
#if DEBUG_BLOCK_PATHS_PRINT
                  DEBUG_PATHS (3, 
                     fprintf (stderr, "Cannot consider exit (key %d) from inner loop at 0x%lx (%" PRId64 "), it freq %" PRId64 ", dummE freq %" PRId64 ", mark %u\n",
                         emit->first,
                         bm->b->getStartAddress(), bm->b->ExecCount(), bm->freq, 
                         bm->dummyE->ExecCount(), bm->b->getMarkedWith());
                  )
#endif
                  continue;
               }
#if DEBUG_BLOCK_PATHS_PRINT
               DEBUG_PATHS (3, 
                  fprintf (stderr, "Use exit (key %d) from inner loop at 0x%lx (%" PRId64 "), mark %u\n",
                      emit->first,
                      bm->b->getStartAddress(), bm->b->ExecCount(), bm->b->getMarkedWith() );
               )
#endif
               // add this exit to the current scope.
               CFG::Node *lnode = pscope->hasLoopNode(it->entry);
               assert(lnode);
               int64_t counter = bm->dummyE->ExecCount();
               if (counter > exit_counter)
                  counter = exit_counter;  // we only need to replace that many exit paths
               BMarker tempM (bm->entry, bm->b, marker, 
                       new inner_loop_edge (lnode, bm->b),
                       counter);
               // mark that we are using counter exits
               bm->dummyE->setExecCount(bm->dummyE->ExecCount() - counter);
               BMSet::iterator bit = exits.find (tempM);
               if (bit != exits.end())
               {
                  // I cannot modify in place because it is a const, bah
                  tempM.freq += bit->freq;
                  exits.erase (bit);
               }
               tempM.dummyE->setExecCount (tempM.freq);
               unsigned int lEntered2 = bm->dummyE->getNumLevelsEntered();
               tempM.dummyE->setNumLevelsEntered(lEntered2);
#if DEBUG_BLOCK_PATHS_PRINT
               DEBUG_PATHS (3, 
                  fprintf(stderr, "Setting entered levels to %d for loop exit dummy edge at b 0x%" 
                          PRIxaddr "\n", lEntered2, bm->b->getStartAddress());
               )
#endif
               exits.insert (tempM);
               exit_counter -= counter;
            }
            
            // if this was the last scope to try, return; I do not want to go finalize a path 
            // at the same time
            ++slit;
         }  // for each scope in iScopes
      
         // see if we used all the exit frequency
         if (exit_counter > 0)  // BAAAAD
         {
            fprintf(stderr, "BAAAD: I have %" PRId64 " out of %" PRId64 " exit path counts that were not converted.\n", 
                       exit_counter, it->dummyE->ExecCount());
            it->dummyE->setExecCount(exit_counter);
         } else   // it is zero
         {
            // Erase the current exit from the exits map
            BMSet::iterator it2 = it;
            ++ it;
            exits.erase(it2);
            continue;
         }
      }  // exit edge enters inner loop
      
      ++ it;
   }
   
#if DEBUG_BLOCK_PATHS_PRINT
     DEBUG_PATHS (4, 
     {
        fprintf(stderr, "\n* * * * * * * * * * BEFORE * * * * * * * * * * * *\n");
        fprintf(stderr, "\n**** [%s / %d] Found %ld paths in this loop ****\n",
             pscope->ToString().c_str(), marker, bpmtemp->size());
        for (BPMap::iterator bpit=bpmtemp->begin() ; bpit!=bpmtemp->end() ; ++bpit)
        {
           fprintf(stderr, "FREQ %" PRIu64 " : ", bpit->second->count);
           for (unsigned int i=0 ; i<bpit->first->size ; i++) {
              fprintf(stderr, " 0x%lx    ", bpit->first->blocks[i]->getStartAddress());
	   }
           fprintf(stderr, " *** \n");
        }
        fprintf(stderr, "**** [%s / %d] List of all blocks in this loop ****\n",
              pscope->ToString().c_str(), marker);
        CFG::NodesIterator nit(*cfg);
        while ((bool)nit)
        {
           CFG::Node *db = nit;
           if (db->isMarkedWith(marker) && db->Size()>0)  // it is one of our blocks
           {
              fprintf(stderr, "0x%lx (%" PRIu64 ") ** ", db->getStartAddress(), db->ExecCount() );
           }
           ++ nit;
        }
        fprintf(stderr, "\n ****** next are the entries blocks: ******\n");
        for (it=entries.begin() ; it!=entries.end() ; it++)
           fprintf(stderr, "0x%lx (%" PRIu64 ") ** ", it->b->getStartAddress(), it->b->ExecCount() );
        fprintf(stderr, "\n ****** next are the exit blocks: ******\n");
        for (it=exits.begin() ; it!=exits.end() ; it++)
           fprintf(stderr, "0x%lx (%" PRIu64 ") [marker %u, freq %" PRIu64 " / dummyE freq %" PRIu64 ", id %d, enter %d] ** ", 
                   it->b->getStartAddress(), it->b->ExecCount(),
                   it->marker, it->freq, it->dummyE->ExecCount(),
                   it->dummyE->getId(), it->dummyE->getNumLevelsEntered() );
        fprintf(stderr, "\n ****** @@@@@@@ ******\n");
     })
#endif

   // check that all blocks exhausted their frequency
   // if any block has non-zero frequency, try to insert it in some path
   // this is a laborious process

   bool notready;
   bool changed = false;
   do
   {
      notready = false;
      CFG::NodesIterator nit(*cfg);
      while ((bool)nit)
      {
         CFG::Node *db = nit;
         if (db->isMarkedWith(marker) && db->Size()>0 && db->ExecCount()>0)  // a lost block
         {  // try to fix paths so they include it
            fprintf(stderr, "WARNING: found block 0x%lx (%" PRIu64 "); Go hack the paths\n",
                  db->getStartAddress(), db->ExecCount() );
            assert(!"I need adjustPaths");
 //           notready = adjustPaths(pscope, marker, bpmtemp);
            changed = changed || notready;
         }
         ++ nit;
      }
   } while(notready);

#if DEBUG_BLOCK_PATHS_PRINT
   DEBUG_PATHS (4, 
      if (changed)
      {
        fprintf(stderr, "\n* * * * * * * * * * AFTER * * * * * * * * * * * *\n");
        fprintf(stderr, "\n**** [%s / %d] Found %lu paths in this loop ****\n",
             pscope->ToString().c_str(), marker, bpmtemp->size());
        for (BPMap::iterator bpit=bpmtemp->begin() ; bpit!=bpmtemp->end() ; ++bpit)
        {
           fprintf(stderr, "FREQ %" PRIu64 " : ", bpit->second->count);
           for (unsigned int i=0 ; i<bpit->first->size ; i++)
              fprintf(stderr, " 0x%lx    ", bpit->first->blocks[i]->getStartAddress());
           fprintf(stderr, " *** \n");
        }
        fprintf(stderr, "**** [%s / %d] List of all blocks in this loop ****\n",
             pscope->ToString().c_str(), marker);
        CFG::NodesIterator nit(*cfg);
        while ((bool)nit)
        {
           CFG::Node *db = nit;
           if (db->isMarkedWith(marker) && db->Size()>0)  // it is one of our blocks
           {
              fprintf(stderr, "0x%lx (%" PRIu64 ") ** ", db->getStartAddress(), db->ExecCount() );
              if (db->ExecCount() != 0)
                 fprintf(stderr, "\n$$$$ ");
           }
           ++ nit;
        }
        fprintf(stderr, "\n ****** next are the entries blocks: ******\n");
        for (it=entries.begin() ; it!=entries.end() ; it++)
           fprintf(stderr, "0x%lx (%" PRIu64 ") ** ", it->b->getStartAddress(), it->b->ExecCount() );
        fprintf(stderr, "\n ****** next are the exit blocks: ******\n");
        for (it=exits.begin() ; it!=exits.end() ; it++)
           fprintf(stderr, "0x%lx (%" PRIu64 ") [marker %u, freq %" PRIu64 " / dummyE freq %" PRIu64 ", id %d, enter %d] ** ", 
                   it->b->getStartAddress(), it->b->ExecCount(),
                   it->marker, it->freq, it->dummyE->ExecCount(),
                   it->dummyE->getId(), it->dummyE->getNumLevelsEntered() );
        fprintf(stderr, "\n ****** THAT WAS ONE LOOP ONLY ******\n");
      }
   )
//#else
#endif

#if PROFILE_SCHEDULER || DEBUG_BLOCK_PATHS_PRINT
   long num_found_paths = bpmtemp->size();
#endif
#if PROFILE_SCHEDULER
   MIAMIP::report_time (stderr, "Recover %ld paths for scope %s", num_found_paths,
        pscope->ToString().c_str());
#endif
#if DEBUG_BLOCK_PATHS_PRINT
   DEBUG_PATHS (1,
      fprintf(stderr, "\n**** [%s / %d] Found %ld paths in this loop ****\n",
           pscope->ToString().c_str(), marker, num_found_paths);
      )
#endif


   /* Temporary method to account for memory stalls and such.
    * Keep track of all memory references in this scope. After all paths
    * are scheduled, I will compute an average IPC for the scope.
    * Next, use the mechanistic model to compute memory stalls and
    * memory overlap.
    */
   AddrSet scopeMemRefs;
   int loopIdx = 1;
   for (BPMap::iterator bpit=bpmtemp->begin() ; bpit!=bpmtemp->end() ; 
                ++bpit, ++loopIdx)
   {
      // pathId is 64 bits; Use 32 bits for head block address,
      // 16 bits for loop index, 16 bits for path index in loop
      PathID pathId(bpit->first->blocks[0]->getStartAddress());
      if (pscope->getScopeType()==LOOP_SCOPE)
         pathId.scopeIdx = dynamic_cast<LoopScope*>(pscope)->Index();
      else
      {
         assert (pscope->getScopeType()==ROUTINE_SCOPE);
         pathId.scopeIdx = 1;  // routines have index 1 I think
      }
      pathId.pathIdx = loopIdx;
      bpit->second->pathId = pathId;

      // FIXME: must compute the average number of times the backedge 
      // is taken everytime we enter the loop; Use a dummy value now
      float exit_count = pscope->getExitCount();
      assert (exit_count>0.f || bpmtemp->size()==1);
      if (exit_count<0.0001)
         exit_count = bpit->second->count;

//         float avgCount = pscope->getTotalCount() / pscope->getExitCount();
      float avgCount = bpit->second->count / exit_count; //pscope->getExitCount();
      if (! bpit->first->isExitPath ())
         avgCount += 1.0;

      // for now just dump the path to check that we can construct them correctly
#if DEBUG_BLOCK_PATHS_PRINT
      DEBUG_PATHS (3,
         bpit->first->Dump(stderr);
      )
#endif
      
      MIAMI_DG::DGBuilder *sch = NULL;
      
#if DEBUG_BLOCK_PATHS_PRINT
      DEBUG_PATHS (2,
         fprintf(stderr, " *** Scheduling pathId %s\n", bpit->second->pathId.Name());
      )
#endif

      if (mo->do_build_dg)
      {
	 std::cerr << "[INFO]Routine::constructPaths:schedule: '" << name << "'\n";
         RFormulasMap &refFormulas = *rFormulas;
#if 1
         //sch = new MIAMI_DG::DGBuilder(name.c_str(), pathId, 
         //     1 /*args.optimistic_memory_dep*/, refFormulas,
         //     InLoadModule(),
         // /*        mdriver.RefNames(), mdriver.RefsTable(), */
         //     bpit->first->size, bpit->first->blocks, 
         //     bpit->first->probabs, bpit->first->innerRegs,
         //     bpit->second->count, avgCount);
         sch = new MIAMI_DG::DGBuilder(this, pathId,
              1 /*args.optimistic_memory_dep*/, refFormulas,
              InLoadModule(),
      /*        mdriver.RefNames(), mdriver.RefsTable(), */
              bpit->first->size, bpit->first->blocks, 
              bpit->first->probabs, bpit->first->innerRegs,
              bpit->second->count, avgCount);
#if PROFILE_SCHEDULER
         MIAMIP::report_time (stderr, "Graph construction for path %s", pathId.Name());
#endif

         bpit->second->latency = 1;
         bpit->second->num_uops = 1;
         
         if (mo->do_scheduling)
         {
            sch->updateGraphForMachine (tmach);
            // compute the scheduling, return a pair containing the latency and the number
            // of micro-ops in the path; we can compute IPCs and CPIs from this;
            MIAMI_DG::schedule_result_t res = sch->computeScheduleLatency(0 /* args.graph_level */,
                     mo->DetailedMetrics());
            bpit->second->latency = res.first;
            bpit->second->num_uops = res.second;


#if PROFILE_SCHEDULER
            MIAMIP::report_time (stderr, "Compute schedule for path %s", pathId.Name());
#endif
            // if it is an exit path, compute the input/output registers
            if (avgCount<=1)
            {
               MIAMIU::UIPair pathKey (bpit->first->blocks[0]->getStartAddress(), 
                         bpit->first->next_block->getStartAddress());
               PairRSIMap::iterator prit = pathRegs.find (pathKey);
               if (prit == pathRegs.end())  // did not see an exit path with this key
               {
                  prit = pathRegs.insert (pathRegs.begin(), 
                         PairRSIMap::value_type (pathKey, RSIList()) );
                  sch->computeRegisterScheduleInfo (prit->second);
#if PROFILE_SCHEDULER
                  MIAMIP::report_time (stderr, "Compute register mapping for path %s", pathId.Name());
#endif
               }
            }

#if NOT_NOW
            CliqueList &myCliques = bpit->second->cliques;
            HashMapUI &refsDist2Use = bpit->second->dist2use;
            RGList &rGroups = bpit->second->reuseGroups;
            sch->computeMemoryInformationForPath (refsTable, 
                     rGroups, myCliques, refsDist2Use);
            // traverse all the reuse groups and match them with the sets in each
            // memory level
            processReuseGroups (tmach, bpit->second);
            
            // compute the parallelism factor; 
            // initially compute a loose upper bound
            // set the miss rate for each reference to be equal to the execution
            // frequency of the path
            HashMapFP missCounts;
            CliqueList::iterator clit = myCliques.begin();
            for ( ; clit!=myCliques.end() ; ++clit)
               for (int ll=0 ; ll<(*clit)->num_nodes ; ++ll)
               {
                  unsigned int crt_pc = (*clit)->vertices[ll];
                  float &miss_val = missCounts [crt_pc];
                  if (miss_val < 0)
                     miss_val = bpit->second->count; 
               }
            float pFactor = compute_parallelism_factor (myCliques, missCounts,
                  refsDist2Use, bpit->second->serialMemLat, 
                  bpit->second->exposedMemLat);
            cerr << "Path " << hex << pathId << dec << " has parallel factor "
                 << pFactor << endl;
#if PROFILE_SCHEDULER
            MIAMIP::report_time (stderr, "Compute reuse groups and mem parallelism for path %s", pathId.Name());
#endif

#endif  // NOT_NOW
         }  // if do_scheduling
         
#else  // if 1/0
         bpit->second->latency = 1;
         bpit->second->num_uops = 1;
         if (pathId==targetPath)
         {
            sch = new MIAMI_DG::DGBuilder(name.c_str(), pathId, 
                 1 /*args.optimistic_memory_dep*/, refFormulas,
                 InLoadModule(),
     /*            mdriver.RefNames(), mdriver.RefsTable(), */
                 bpit->first->size, bpit->first->blocks, 
                 bpit->first->probabs, bpit->first->innerRegs,
                 bpit->second->count, avgCount);
#if PROFILE_SCHEDULER
            MIAMIP::report_time (stderr, "Graph construction w/ debug for path %s", pathId.Name());
#endif

            char bbuf1[64], bbuf2[64];
            sprintf (bbuf1, "q-%s", pathId.Filename());
            sprintf (bbuf2, "r-%s", pathId.Filename());
            sch->draw_debug_graphics (bbuf1);
            
            if (mo->do_scheduling)
            {
               sch->updateGraphForMachine (tmach);
               sch->draw_debug_graphics (bbuf2);
            
            // draw the dominator trees
//            sch->draw_dominator_trees ("x6");

               // compute the scheduling
               MIAMI_DG::schedule_result_t res = sch->computeScheduleLatency(0 /* args.graph_level */,
                        mo->DetailedMetrics());
               bpit->second->latency = res.first;
               bpit->second->num_uops = res.second;

#if PROFILE_SCHEDULER
               MIAMIP::report_time (stderr, "Compute schedule w/ debug for path %s", pathId.Name());
#endif

               // if it is an exit path, compute the input/output registers
               if (avgCount<=1)
               {
                  UIPair pathKey (bpit->first->blocks[0]->start(), 
                            bpit->first->next_block->start());
                  PairRSIMap::iterator prit = pathRegs.find (pathKey);
                  if (prit == pathRegs.end())  // did not see an exit path with this key
                  {
                     prit = pathRegs.insert (pathRegs.begin(),
                            PairRSIMap::value_type (pathKey, RSIList()) );
                     sch->computeRegisterScheduleInfo (prit->second);
#if PROFILE_SCHEDULER
                     MIAMIP::report_time (stderr, "Compute register mapping w/ debug for path %s", pathId.Name());
#endif
                  }
               }
#if NOT_NOW
               CliqueList &myCliques = bpit->second->cliques;
               HashMapUI &refsDist2Use = bpit->second->dist2use;
               RGList &rGroups = bpit->second->reuseGroups;
               sch->computeMemoryInformationForPath (refsTable, 
                        rGroups, myCliques, refsDist2Use);
               // traverse all the reuse groups and match them with the sets 
               // in each memory level
               processReuseGroups (tmach, bpit->second);

               // compute the parallelism factor; 
               // initially compute a loose upper bound
               // set the miss rate for each reference to be equal to the execution
               // frequency of the path
               HashMapFP missCounts;
               CliqueList::iterator clit = myCliques.begin();
               for ( ; clit!=myCliques.end() ; ++clit)
                  for (int ll=0 ; ll<(*clit)->num_nodes ; ++ll)
                  {
                     unsigned int crt_pc = (*clit)->vertices[ll];
                     float &miss_val = missCounts [crt_pc];
                     if (miss_val < 0)
                        miss_val = bpit->second->count; 
                  }
               float pFactor = compute_parallelism_factor (myCliques, missCounts,
                          refsDist2Use, bpit->second->serialMemLat, 
                          bpit->second->exposedMemLat);
               cerr << "Path " << hex << pathId << dec << " has parallel factor "
                    << pFactor << endl;
#if PROFILE_SCHEDULER
               MIAMIP::report_time (stderr, "Compute reuse groups and mem parallelism (debug) for path %s", 
                        pathId.Name());
#endif

#endif  // NOT_NOW
            } // if do_scheduling
         } // if path == targetPath
#endif   // if 1/0
         
         if (sch)
         {
            bpit->second->timeStats = sch->getTimeStats();
//            bpit->second->timeStats.addRetiredUops(bpit->second->num_uops);
            bpit->second->unitUsage = sch->getUnitUsage();
            
            const AddrSet& pathRefs = sch->getMemoryReferences();
            scopeMemRefs.insert(pathRefs.begin(), pathRefs.end());
            delete sch;
#if PROFILE_SCHEDULER
            MIAMIP::report_time (stderr, "Get time stats and deallocate scheduler for path %s", pathId.Name());
#endif
         }

         std::cout<<"PALM: path lat: "<<bpit->second->latency<<" path uops: "<<bpit->second->num_uops<<" path count: "<<bpit->second->count<<" "<<bpit->second->serialMemLat<<" "<<bpit->second->exposedMemLat<<std::endl;
         thisLoopLatency += (bpit->second->count)*(bpit->second->latency);
         thisLoopUops += (bpit->second->count)*(bpit->second->num_uops);
      }  // if do_build_dg
   }
   
   if (!no_fpga_acc)
   {
#if NOT_NOW
      decodeInstructionsInPath (pscope, bpmtemp);
#endif
#if PROFILE_SCHEDULER
      MIAMIP::report_time (stderr, "Decode instructions for scope %s", 
			   pscope->ToString().c_str());
#endif
   }

   pscope->Paths() = bpmtemp;
   std::cout <<"lat: "<< thisLoopLatency << " numuops: "<<thisLoopUops<<std::endl;
   pscope->Latency() = thisLoopLatency;
   pscope->NumUops() = thisLoopUops;

   if (mo->do_memlat)
   {
      // once all the paths are processed, compute exclusive memory stalls for this scope
      mdriver.compute_memory_stalls_for_scope(pscope, scopeMemRefs);
   }
}

// pentry is the path entry block
// - it can be NULL when I attempt to build a path from the loop head without an explicit
//   non-zero incoming edge into the node
// - in this case, I should not be able to find an exit path ... more corner cases.
CFG::NodeList::iterator
Routine::addBlock(ScopeImplementation *pscope, CFG::Node *pentry, CFG::Node *thisb, 
           CFG::Edge *lastE, CFG::NodeList& ba, CFG::EdgeList &ea, RSIListList &rl, 
           int marker, BPMap *bpm, int64_t& iCount)
{
//   CFG *cfg = ControlFlowGraph();
   // if this block is an entry into an inner loop, store the scope of the loop 
   // in the lScope variable
   ScopeImplementation *lScope = 0;
   std::pair<addrtype,addrtype> myaddrs(thisb->getStartAddress(),thisb->getEndAddress());
   if (dyn_addrToBlkNo->count(myaddrs) > 0){
      std::cout<<"my block num is: "<<(*dyn_addrToBlkNo)[myaddrs]<<" "<<std::hex<<thisb->getStartAddress()<<" "<<thisb->getEndAddress()<<std::dec
      <<" "<<rl.size()<<" "<<(*dyn_blkNoToMiamiBlk)[(*dyn_addrToBlkNo)[myaddrs]]<<" "<<thisb<<std::endl;
   }
   
#if DEBUG_BLOCK_PATHS_PRINT
   DEBUG_PATHS (6, 
      fprintf(stderr, "\nConsider block 0x%lx (%d), count %" PRIu64 " ", 
           thisb->getStartAddress(), thisb->getMarkedWith(), thisb->ExecCount());
   )
#endif

   if (thisb->Size()>0 && thisb->isMarkedWith(marker) && thisb->ExecCount()<=0)
      return (ba.end());
   // if we close the loop on some other node than the first node in the path,
   // it means that something is wrong. Do not allow it.
   if (thisb->isMarkedWith(marker ^ MARKER_CODE) && thisb != *(ba.begin()))
      return (ba.end());
   
   // consider also the block in delay slot for the exit jump
   if ((thisb->isMarkedWith(marker) || thisb->is_delay_block()) && 
          !thisb->isCfgExit())
//           !(thisb->isCfgExit() || (thisb->isCallSurrogate() && 
//           thisb->firstOutgoingOfType(CFG::MIAMI_FALLTHRU_EDGE)->sink()->ExecCount()<=0)))
   {
#if DEBUG_BLOCK_PATHS_PRINT
      DEBUG_PATHS (6, 
         fprintf(stderr, " | Added block 0x%lx size %ld", thisb->getStartAddress(), thisb->Size());
      )
#endif
      // add also the cfg entry block
      if (thisb->Size()>0 || thisb->isCfgEntry() || thisb->isCallSurrogate())
      {
         ba.push_back(thisb);
         // set in_use flag only on blocks right after an inner loop exit
//         thisb->setInUse();
      }
//      else
         // a block with size 0 can be either a call surrogate or a
         // cfg entry. I found there are routines with multiple entry
         // points, and as such with multiple outgoing edges from the
         // cfg entry block.
         // call surrogate can have multiple outgoing edges now, one
         // fall-thru edges, and a return edge for cases when the call
         // does not return
//         assert (thisb->num_outgoing()==1);
//         assert (thisb->isCallSurrogate());
                                            
      thisb->markWith(marker ^ MARKER_CODE);  // mark that we saw this node once
      CFG::OutgoingEdgesIterator oeit(thisb);
      CFG::NodeList::iterator retit = ba.end();
      while ((bool)oeit)
      {
         CFG::Edge *e = oeit;
         ++ oeit;
#if DEBUG_BLOCK_PATHS_PRINT
         DEBUG_PATHS (7, 
            fprintf(stderr, " | Consider edge [0x%lx,0x%lx] count %" PRId64 " ...", 
               thisb->getStartAddress(), e->sink()->getStartAddress(), e->ExecCount());
         )
#endif
         if (e->ExecCount() == 0)
            continue;
#if DEBUG_BLOCK_PATHS_PRINT
         DEBUG_PATHS (7, 
            fprintf(stderr, " taken!");
         )
#endif
            
         // otherwise, this edge was taken for sure
         if (thisb->Size()>0 || thisb->isCfgEntry() || thisb->isCallSurrogate())
            ea.push_back(e);
         retit = addBlock(pscope, pentry, e->sink(), e, ba, ea, rl, marker, bpm, iCount);
         if (thisb->Size()>0 || thisb->isCfgEntry() || thisb->isCallSurrogate())
            ea.pop_back();

#if DEBUG_BLOCK_PATHS_PRINT
         DEBUG_PATHS (7, 
         {
            fprintf(stderr, "\nRemove edge [0x%lx,0x%lx] count %" PRId64 " ", 
               thisb->getStartAddress(), e->sink()->getStartAddress(), e->ExecCount());
            fprintf(stderr, " | Current block is 0x%lx size %ld, count %" PRId64, 
               thisb->getStartAddress(), thisb->Size(), thisb->ExecCount());
         })
#endif
         if (retit != ba.end())
            break;
         if (thisb->Size()>0 && thisb->ExecCount()<=0)
         {
            break;
         }
      }
#if DEBUG_BLOCK_PATHS_PRINT
      DEBUG_PATHS (7, 
         fprintf(stderr, " | Remove block 0x%lx size %ld, count %" PRId64, 
                thisb->getStartAddress(), thisb->Size(), thisb->ExecCount());
      )
#endif
      thisb->markWith(marker);
      CFG::NodeList::iterator crtit = ba.end();
      --crtit;
      if (crtit == retit)
         retit = ba.end();
      if (thisb->Size()>0 || thisb->isCfgEntry() || thisb->isCallSurrogate())
      {
         ba.pop_back();
//         assert(thisb->InUse());
//         thisb->unsetInUse();
      }
      return (retit);
   }
   else // maybe it is an entrance into an inner loop -> jump to its exit
        // search not only immediately inner loops, but also grand-children loops
        // Hmm, I should test all loops entered through this incoming edge.
   {
      typedef std::list<ScopeImplementation*> ScopeList;
      ScopeList iScopes;   // build a list of entered scopes, sorted from outer to inner most
      
      ScopeImplementation *inScope = 0;
      int lEntered = 0;
      if (!lastE || (lEntered = lastE->getNumLevelsEntered())>0)
         inScope = pscope->Descendant (thisb->getMarkedWith());
#if DEBUG_BLOCK_PATHS_PRINT
      DEBUG_PATHS (5, 
         fprintf(stderr, " Last edge %p enters %d loop levels\n", 
             lastE, lEntered);
         if (lastE)
            fprintf(stderr, "Last edge id=%d from B%d[%" PRIxaddr "] to B%d[%" PRIxaddr "]\n",
                  lastE->getId(), lastE->source()->getId(), lastE->source()->getStartAddress(),
                  lastE->sink()->getId(), lastE->sink()->getStartAddress());
      )
#endif
      
      while(inScope)
      {
         // I should not include the current scope. When a loop entry edge enters
         // additional inner loops and I am looking at all the exits, I want to
         // exclude the outermost loop, the one in which we are trying to find paths.
         // Check by marker. If inScope's marker is current loop's marker, then do not
         // include it and stop. We should not consider any loops which are outer this one.
         if ((int)inScope->getMarkedWith() <= marker)
            break;
         iScopes.push_front(inScope);
         if (lEntered > 1)
         {
            lEntered -= 1;
            inScope = inScope->Parent();
         } else
            inScope = 0;
      }
      ScopeList::iterator slit = iScopes.begin();
      for ( ; slit!=iScopes.end() ; )
      {
         inScope = *slit;
         
         // I am using the loop_node associated with a scope to detect if I am
         // reentering a particular loop. When I start my path with an inner loop
         // I want to be able to detect a cycle if I end up reentering the same
         // inner loop. I can see a problem with the current implementation if 
         // there are more than 1 level of inner-loops that I enter because I only
         // set and test the marker on one loop level, the innermost level at the 
         // entry point. The innermost level may be different on start and reentry,
         // so I may end up not detecting the loop.
         const BMarker *eMarker;
         if (inScope && (eMarker = inScope->HasEntry(thisb)))  // we had an entry at block thisb
         {
            CFG::Node *lnode = inScope->LoopNode(thisb);
            unsigned int themark = inScope->getMarkedWith();
            assert (lnode->isMarkedWith(themark) || lnode->isMarkedWith(themark^MARKER_CODE));
            if (lnode->isMarkedWith(themark))  // first time seeing this entry on this path
            {
               assert(! lnode->InUse());
#if DEBUG_BLOCK_PATHS_PRINT
               DEBUG_PATHS (3, 
                  fprintf(stderr, " | I have found an entry into an inner loop 0x%lx (%" PRId64 "), mark %u\n",
                      thisb->getStartAddress(), thisb->ExecCount(), themark);
               )
#endif
               BMSet& oldExits = inScope->InnerExits();

               lnode->markWith(themark ^ MARKER_CODE);
               ba.push_back (lnode);
               lnode->setInUse();
               PairRSIMap &pathRegs = inScope->PathRegisters();
               CFG::NodeList::iterator retit = ba.end();
               
               // I will not sort the exits for now. I am just going to 
               // traverse the exits associated with this entry, and 
               // pick the first one that has non-zero count and I haven't
               // used on this path yet (must check for this somehow).
               BMarker tempm(thisb, 0, 0, 0);
               BMSet::iterator it = oldExits.lower_bound(tempm);
               for ( ; it!=oldExits.end() && it->entry==thisb ; ++it)
               {
                  const BMarker *bm = &(*it);
#if 0
                  unsigned int key = 50;
                  int bmMarker = bm->b->getMarkedWith();
                  if (bmMarker == (marker^MARKER_CODE))
                     key = 10;
                  else if (bmMarker == marker)
                     key = 20;
                  // otherwise, the marker stays at 50 (be it larger or smaller marker)
                  sExits.insert(ExitsMap::value_type(key, bm));
#endif

                  if (bm->freq==0 || bm->dummyE->ExecCount()<=0 || bm->b->InUse())
                  {
#if DEBUG_BLOCK_PATHS_PRINT
                     DEBUG_PATHS (4, 
                        fprintf (stderr, "Cannot consider exit (in_use=%d) from inner loop at 0x%" PRIxaddr " (%" PRId64 "), BM freq %" PRId64 ", dummE freq %" PRId64 ", mark %u\n",
                            bm->b->InUse(),
                            bm->b->getStartAddress(), bm->b->ExecCount(), bm->freq, 
                            bm->dummyE->ExecCount(), bm->b->getMarkedWith());
                     )
#endif
                     continue;
                  }
                  ea.push_back (bm->dummyE);
                  bm->b->setInUse();
//               innerE->setExecCount (bm->freq);
                  BriefAssertion (bm->marker == themark);  // is this always true now that we can go into
                             // a grandchild loop?
                  PairRSIMap::iterator prit = pathRegs.find 
                        (MIAMIU::UIPair (thisb->getStartAddress(), bm->b->getStartAddress()));
                  if (prit == pathRegs.end())  // did not find the info
                  {
#if DEBUG_BLOCK_PATHS_PRINT
                  DEBUG_PATHS (3, 
                     fprintf(stderr, "Did not find a register map for this entry-exit pair.\n");
                  )
#endif
                     rl.push_back (RSIList());
                  }
                  else
                     rl.push_back (prit->second);
#if DEBUG_BLOCK_PATHS_PRINT
                  DEBUG_PATHS (3, 
                     fprintf (stderr, "Use exit from inner loop at 0x%" PRIxaddr " (%" PRId64 "), mark %u\n",
                         bm->b->getStartAddress(), bm->b->ExecCount(), bm->b->getMarkedWith() );
                  )
#endif
                  retit = addBlock (pscope, pentry, bm->b, bm->dummyE, ba, ea, rl, marker, bpm, iCount);
                  bm->b->unsetInUse();
                  rl.pop_back ();
                  ea.pop_back ();
                  if (retit != ba.end())
                     break;
               }
               
               CFG::NodeList::iterator crtit = ba.end();
               --crtit;
               if (crtit == retit)
                  retit = ba.end();
               ba.pop_back ();
               assert(lnode->InUse());
               lnode->unsetInUse();
               lnode->markWith(themark);
               
               if (retit != ba.end())
                  return (retit);
            } else   // reentry at the same point, we close a cyle
            {
               thisb = lnode;
               lScope = inScope;
               break;
            }
         }
#if 0
         if (inScope && inScope->LoopNode() && 
                    // check we do not reenter the same inner loop again
                    inScope->LoopNode()->isMarkedWith(inScope->getMarkedWith()))
         {
            BMSet& oldEntries = inScope->InnerEntries();
            if (oldEntries.find (BMarker (thisb, inScope->getMarkedWith(), 0)) != 
                          oldEntries.end())
            {
#if DEBUG_BLOCK_PATHS_PRINT
               DEBUG_PATHS (3, 
                  fprintf(stderr, " | I have found an entry into an inner loop 0x%lx (%" PRId64 "), mark %u\n",
                      thisb->getStartAddress(), thisb->ExecCount(), inScope->getMarkedWith());
               )
#endif
               unsigned int themark = inScope->getMarkedWith();
               BMSet& oldExits = inScope->InnerExits();
               // I found cases where a routine starts with an inner loop, thus,
               // the edge array is empty when I take the exit edge from an inner loop.
               // I do not know why I had the assert in the first place. I do not know if
               // any of the following code assumes the ea array is not empty, or if it
               // was just a sanity check. Comment for now.
               //            assert (! ea.empty());

               CFG::Node *loop_node = inScope->LoopNode();
               assert(loop_node);
               loop_node->markWith(themark ^ MARKER_CODE);
               ba.push_back (loop_node);
               PairRSIMap &pathRegs = inScope->PathRegisters();
               CFG::NodeList::iterator retit = ba.end();
               
               // first, add all exits to a map, ordered by the following criteria:
               // - exits that get me to a node marked with marker ^ MARKER_CODE (key = 10)
               // - exits to a node marked with marker  (key = 20)
               // - exist to a node marked with a larger marker  (key = 50)
               // - exist to a node marked with a smaller marker
               typedef std::multimap<unsigned int, const BMarker*> ExitsMap;
               ExitsMap sExits;  // scope exits
               for (BMSet::iterator it=oldExits.begin() ; it!=oldExits.end() ; ++it)
               {
                  const BMarker *bm = &(*it);
                  unsigned int key = 50;
                  int bmMarker = bm->b->getMarkedWith();
                  if (bmMarker == (marker^MARKER_CODE))
                     key = 10;
                  else if (bmMarker == marker)
                     key = 20;
                  // otherwise, the marker stays at 50 (be it larger or smaller marker)
                  sExits.insert(ExitsMap::value_type(key, bm));
               }
               
               for (ExitsMap::iterator emit=sExits.begin() ; emit!=sExits.end() ; ++emit)
               {
                  const BMarker *bm = emit->second;
                  if (bm->freq==0 || bm->dummyE->ExecCount()<=0)
                  {
#if DEBUG_BLOCK_PATHS_PRINT
                     DEBUG_PATHS (4, 
                        fprintf (stderr, "Cannot consider exit (key %d) from inner loop at 0x%lx (%" PRId64 "), it freq %" PRId64 ", dummE freq %" PRId64 ", mark %u\n",
                            emit->first,
                            bm->b->getStartAddress(), bm->b->ExecCount(), bm->freq, 
                            bm->dummyE->ExecCount(), bm->b->getMarkedWith());
                     )
#endif
                     continue;
                  }
                  ea.push_back (bm->dummyE);
//               innerE->setExecCount (bm->freq);
                  BriefAssertion (bm->marker == themark);  // is this always true now that we can go into
                             // a grandchild loop?
                  PairRSIMap::iterator prit = pathRegs.find 
                        (MIAMIU::UIPair (thisb->getStartAddress(), bm->b->getStartAddress()));
                  if (prit == pathRegs.end())  // did not find the info
                  {
#if DEBUG_BLOCK_PATHS_PRINT
                  DEBUG_PATHS (3, 
                     fprintf(stderr, "Did not find a register map for this entry-exit pair.\n");
                  )
#endif
                     rl.push_back (RSIList());
                  }
                  else
                     rl.push_back (prit->second);
#if DEBUG_BLOCK_PATHS_PRINT
                  DEBUG_PATHS (3, 
                     fprintf (stderr, "Use exit (key %d) from inner loop at 0x%lx (%" PRId64 "), mark %u\n",
                         emit->first,
                         bm->b->getStartAddress(), bm->b->ExecCount(), bm->b->getMarkedWith() );
                  )
#endif
                  retit = addBlock (pscope, pentry, bm->b, bm->dummyE, ba, ea, rl, marker, bpm, iCount);
                  rl.pop_back ();
                  ea.pop_back ();
                  if (retit != ba.end())
                     break;
               }
               CFG::NodeList::iterator crtit = ba.end();
               --crtit;
               if (crtit == retit)
                  retit = ba.end();
               ba.pop_back ();
               loop_node->markWith(themark);
               
               if (retit != ba.end())
                  return (retit);
            }
         } else  // if (inScope && inScope->LoopNode() && ...)
         if (inScope && inScope->LoopNode())  // this must be a reentry, test
         {
            assert(inScope->LoopNode()->isMarkedWith(inScope->getMarkedWith()^MARKER_CODE));
            thisb = inScope->LoopNode();
            lScope = inScope;
            break;
         }
#endif   // #if 0
         
         // if this was the last scope to try, return; I do not want to go finalize a path 
         // at the same time
         ++slit;
         if (slit == iScopes.end())
            return (ba.end());
      }  // for each scope in iScopes

      // we have found a path; determine its frequency
      CFG::NodeList::iterator it, minit = ba.end();
      CFG::EdgeList::iterator eit;
      // if this is actually an exit path, use methods getFrequency, setFrequency
      // to check how much of the frequency is available.
      // We do not want to modify the ExecCount value because that value is
      // needed when finding paths in the outer loop.
      // So just use this field I added in the block to acknowledge that we 
      // used part or all of the available frequency for this block;
      // mgabi: 8/12/2004: I should not change the exec frequency of the 
      // ending blocks because this is actually the first block which is not
      // part of the path. I do not think there is a danger to consider this 
      // block again if I do not mark it as used because I decrement the 
      // available exec count of the edge leading to this node, and such the
      // nodes becomes unreachable from the inner loop, but it could be reached
      // from an outer loop through the mechanism of entry/exit nodes.
      
      int64_t counter = 0;
      // get the counter of the firt node that is not an inner loop dummy
      for (it=ba.begin() ; it!=ba.end() ; ++it)
      {
         if (!(*it)->is_inner_loop())
         {
            counter = (*it)->ExecCount();
            break;
         }
      }
      
      // it is possible to have a path were there are no nodes besides the inner loop
      // check the edge counters if counter is 0
      if (counter==0 && !ea.empty())
      {
         counter = ea.front()->ExecCount();
      }

#if DEBUG_BLOCK_PATHS_PRINT
      DEBUG_PATHS (7, 
         fprintf (stderr, "Check block counts for this path, current counter=%" PRId64 ":.\n", counter);
      )
#endif
      for (it=ba.begin() ; it!=ba.end() ; ++it)
      {
         if ((*it)->is_inner_loop ())   // an inner loop marker
            continue;
#if DEBUG_BLOCK_PATHS_PRINT
         DEBUG_PATHS (7, 
            fprintf (stderr, "B[0x%lx,0x%lx] (%" PRId64 ") - ", (*it)->getStartAddress(), 
                  (*it)->getEndAddress(), (*it)->ExecCount());
         )
#endif
         if (counter == 0)
            break;
         if (counter > (*it)->ExecCount())
         {
            counter = (*it)->ExecCount();
            minit = it;
         }
      }
      if (counter == 0)  // return the iterator to the block with zero count
      {
         assert (it!=ba.end());
#if DEBUG_BLOCK_PATHS_PRINT
         DEBUG_PATHS (6, 
            fprintf (stderr, "\nFound block with zero counter value, B[0x%" PRIxaddr ",0x%" PRIxaddr "]\n", 
                    (*it)->getStartAddress(), (*it)->getEndAddress());
         )
#endif
         return (it);
      }
#if DEBUG_BLOCK_PATHS_PRINT
      DEBUG_PATHS (7, 
         fprintf (stderr, "\nCurrent counter value %" PRId64 ", Check edge counts for this path:.\n", 
                  counter);
      )
#endif

      for (eit=ea.begin(), it=ba.begin() ; eit!=ea.end() ; ++eit, ++it)
      {
         // Now I create a dummy edge instead of a NULL pointer and set its 
         // execution frequency to the execution frequency of the exit path 
         // taken
//         if ((*eit) == NULL)   // should never happen
//            continue;
#if DEBUG_BLOCK_PATHS_PRINT
         DEBUG_PATHS (7, 
            fprintf (stderr, "E[0x%lx,0x%lx] (%" PRId64 ") - ", 
                  (*eit)->source()->getStartAddress(), (*eit)->sink()->getStartAddress(), 
                  (*eit)->ExecCount());
         )
#endif
         if (counter == 0)
            break;
         if (counter > (*eit)->ExecCount())
         {
            counter = (*eit)->ExecCount();
            minit = it;
            ++ minit;
         }
      }
      if (counter == 0)  // return the iterator to the block with zero count
      {
         assert (eit!=ea.end());
         assert (it!=ba.end());
#if DEBUG_BLOCK_PATHS_PRINT
         DEBUG_PATHS (6, 
            fprintf (stderr, "\nFound edge with zero counter value, E[0x%" PRIxaddr ",0x%" PRIxaddr 
                   "], Biterator[0x%" PRIxaddr ",0x%" PRIxaddr "\n", 
                    (*eit)->source()->getStartAddress(), (*eit)->sink()->getStartAddress(),
                    (*it)->getStartAddress(), (*it)->getEndAddress());
         )
#endif
         ++ it;
         // I think iterator can point to the end of the array now, still fine to return
         // assert (it!=ba.end());
         return (it);
      }
#if DEBUG_BLOCK_PATHS_PRINT
      DEBUG_PATHS (7, 
         fprintf (stderr, "\nCurrent counter value %" PRId64 "\n", counter);
      )
#endif

      int is_exit_path = 0;
      // if it is not a loop head visited again, then it is an exit node
      // 10/31/2013, gmarin: it can also be an inner loop if I start my path
      // by entering an inner loop; check for this case
      if ((! thisb->isMarkedWith(marker ^ MARKER_CODE)) &&
          (!thisb->is_inner_loop() || !lScope || !thisb->isMarkedWith(lScope->getMarkedWith()^MARKER_CODE))
          && (counter>0) )
      {
         is_exit_path = 1;
         // take into account iCount for exit paths
         if (counter > iCount)
         {
#if DEBUG_BLOCK_PATHS_PRINT
            DEBUG_PATHS (7, 
               fprintf (stderr, "\nCurrent counter value %" PRId64 
                   " is greater than iCount %" PRId64 ", set it to the latter value\n", 
                   counter, iCount);
      )
#endif
            counter = iCount;
            // limited by the loop entry edge, set minit to first block
            // NOOO; we may have other paths that take the loop back edge. I should not 
            // touch minit. Comment next line.
            //minit = ba.begin();
         }
      }

      // update the counter of each block and edge after considering current path
      if (counter > 0)
      {
         for (it=ba.begin() ; it!=ba.end() ; ++it)
         {
            if (!(*it)->is_inner_loop ())
               (*it)->setExecCount ((*it)->ExecCount() - counter);
         }

         int ii = 0;
         for (eit=ea.end() ; eit!=ea.begin() ; ++ii)
         {
            -- eit;
            // 05/16/2012, gmarin: Right now I am including the call surrogate
            // blocks and any of their edges as first class citizens in my paths
            // The following test is not needed any more
#if 0
            CFG::Node *nb = (*eit)->sink();
            // if next block is call_surrogate, decrement also the count of 
            // the edge after it, which is not included in the ea array
            // make sure however that this is not the last edge (ii!=0)
            if (nb->isCallSurrogate () && ii>0)
            {
               CFG::Edge *nedge = nb->firstOutgoingOfType(CFG::MIAMI_FALLTHRU_EDGE);
               int64_t ncount = nedge->ExecCount();
               assert (ncount >= counter);
               nedge->setExecCount (ncount - counter);
            }
#endif
            if (*eit)
               (*eit)->setExecCount ((*eit)->ExecCount() - counter);
         }
         if (is_exit_path)
            iCount -= counter;
      }
      
      if (counter>0)
      {
         if (is_exit_path)
         {
            // pentry (the path entry block) should not be NULL if this is an
            // exit path. I hope it does not fail
            assert(pentry != 0);
            pscope->addExitCount(counter);
            // also add this node to the exit nodes;
            // I cannot just use insert. I have to check if I already have
            // that (block,marker) combination in exits. It is possible to
            // have two or more paths through an inner loop that both exit
            // at the same node. In such cases I have to sum the exit 
            // counts of all exit paths.
            // gmarin, 11/06/2013: keep track of exits separately for each
            // entry node. To implement this, I will also keep separate loop
            // nodes for each entry point into a loop.
//            assert(pscope->LoopNode());
            // can ba be empty?? I guess if I enter into an inner loop and
            // then exit out of both loops. In such a case, the entry block 
            // should be the entry point into the inner loop.
            CFG::Node *lnode = pscope->LoopNode(pentry);
            BMarker tempM (pentry, thisb, marker, 
                    new inner_loop_edge (lnode, thisb),
                    counter);
            BMSet& crtExits = pscope->InnerExits ();
            BMSet::iterator bit = crtExits.find (tempM);
            if (bit != crtExits.end())
            {
               // I cannot modify in place because it is a const, bah
               tempM.freq += bit->freq;
               crtExits.erase (bit);
            }
            tempM.dummyE->setExecCount (tempM.freq);
            if (!ea.empty()) {
               unsigned int lEntered = ea.back()->getNumLevelsEntered();
               tempM.dummyE->setNumLevelsEntered(lEntered);
#if DEBUG_BLOCK_PATHS_PRINT
               DEBUG_PATHS (5, 
                  fprintf(stderr, "Setting entered levels to %d for loop exit dummy edge at b 0x%" 
                          PRIxaddr "\n", lEntered, thisb->getStartAddress());
               )
#endif
            }
            crtExits.insert (tempM);
         }
         
         pscope->addExecCount(counter);
         // the arrays of blocks and edges should be equal now
         assert(ba.size() == ea.size());
         // compute the probability array
         MIAMIU::FloatArray fa;
         for (eit=ea.begin() ; eit!=ea.end() ; ++eit)
         {
            assert ((*eit == NULL) || (*eit)->source()->Size()>0
                     || (*eit)->source()->isCfgEntry() 
                     || (*eit)->source()->isCallSurrogate() 
                     || (*eit)->source()->is_inner_loop());
            // if this edge leads to an inner loop that was scheduled separately,
            // set the probability to 0, which makes the edge act as a barrier 
            // between the instructions placed before the inner loop and the 
            // instructions found after the loop.
            // If the last Node is a delay block, then I have to mark the edge
            // corresponding to the last branch.
//            if ((*eit)->isInnerLoopEntry ())
            if (*eit == NULL)
               fa.push_back (0.0);  //- (*eit)->Probability ());  //(0.0);
            else
               fa.push_back ((*eit)->Probability ());
         }
         BriefAssertion(ba.size() == fa.size());

         // Include the next block after the path into the BlockPath, so I schedule
         // the exit path separately. In addition, I avoid the problem with what edge
         // probabilities to consider when the regular and exit paths have the same
         // blocks (same start addresses - they may contain delay blocks on different
         // edges, but the delay blocks would have the same start address).
         BlockPath *bp = new BlockPath(ba, thisb, fa, rl, is_exit_path);
         BPMap::iterator bit = bpm->find(bp);
         if (bit == bpm->end())   // a new path
         {
            bpm->insert(BPMap::value_type(bp, new PathInfo(counter)));
         }
         else  // i have a path with the same blocks already -> update frequency
         {
            bit->second->count += counter;
            delete bp;
         }

#if DEBUG_BLOCK_PATHS_PRINT
      // DEBUG; print the path on the screen
      DEBUG_PATHS (1, 
      {
         fprintf(stderr, "Scope %s, found path with freq %" PRId64 ":\n", 
                pscope->ToString().c_str(), counter);
         CFG::NodeList::iterator bit_=ba.begin();
         MIAMIU::FloatArray::iterator fit_=fa.begin();
         for ( ; bit_!=ba.end() ; bit_++, fit_++)
         {
            fprintf(stderr, " 0x%lx (prob %g) #*#", (*bit_)?(*bit_)->getStartAddress():0, (*fit_));
         }
         fprintf(stderr, " +++ 0x%lx (%" PRId64 ")\n", thisb->getStartAddress(), thisb->ExecCount());
      })
#endif
         fa.clear();
         return (minit);
      }
   }
   return (ba.end());
}

void
Routine::createDyninstFunction() // todo: better error checking
{
   cout << "createDyninstFunction"<<endl;
   dyn_func = NULL;
   BPatch_image* dyn_img = InLoadModule()->getDyninstImage();
   std::vector<BPatch_function*> funcs;
   bool found = dyn_img->findFunction(start_addr,funcs);
   if (!found){
      fprintf(stderr, "Unable find dyninst function from address 0x%lx, searching based on name: %s \n",start_addr,name.c_str());
      dyn_img->findFunction(name.c_str(),funcs,true,true,true); 
   }
   if (funcs.size() != 1){
      if (funcs.size() == 0){
         fprintf(stderr, "Unable to create dyninst function: %s \n",name.c_str());
         return;
      }
      else {
         fprintf(stderr, "Multiple dyninst functions for %s 0x%lx found: \n",name.c_str(),start_addr);
         for (unsigned int i = 0; i < funcs.size(); i++) {
            fprintf(stderr,"%s 0x%p\n",funcs[i]->getMangledName().c_str(),funcs[i]->getBaseAddr());
            if(funcs[i]->getMangledName().compare(name) == 0 && funcs[i]->getBaseAddr() == (void*)start_addr){
               dyn_func = funcs[i];
               fprintf(stderr,"%s matched with address 0x%p\n",funcs[i]->getMangledName().c_str(),funcs[i]->getBaseAddr());
               break;
            }
         }
      }
   }
   if (funcs.size() == 1){

      dyn_func = funcs[0];
   }
   if (dyn_func != NULL){

      Dyninst::ParseAPI::CodeSource* codeSrc = Dyninst::ParseAPI::convert(dyn_func)->obj()->cs();
      BPatch_flowGraph *fg = dyn_func->getCFG();
      std::set<BPatch_basicBlock*> blks;
      fg->getAllBasicBlocks(blks);
      int blkCnt = 0;
      for (auto blk : blks){
         cout<<blkCnt<<" "<< (unsigned int*)blk->getStartAddress()<<" "<<(unsigned int*)blk->getEndAddress()<<" "<< (unsigned int*)codeSrc->getPtrToInstruction(blk->getStartAddress())<<endl;
         (*dyn_addrToBlock)[std::make_pair(blk->getStartAddress(),blk->getEndAddress())]=blk;
         (*dyn_addrToBlkNo)[std::make_pair(blk->getStartAddress(),blk->getEndAddress())]=blkCnt++;

         uint8_t* addr =(uint8_t*)codeSrc->getPtrToInstruction(blk->getStartAddress());
         int i =0;
         while ((unsigned int*)(blk->getStartAddress() + i) < (unsigned int*)blk->getEndAddress()){
            int len = 15;
            cout<<(unsigned int*)(blk->getStartAddress() + i)<<" "<<(unsigned int*)blk->getEndAddress()<<" "<<(unsigned int*)(addr+i)<<" "<<endl;
            //len = dump_instruction_at_pc((void*)(addr+i),len);
            cout<<i<<" "<<len<<endl;
            i+= len; 
            cout<<(unsigned int*)(blk->getStartAddress() + i)<<" "<<(unsigned int*)blk->getEndAddress()<<" "<<(unsigned int*)(addr+i)<<" "<<endl;
         }         
      }
   }
   //exit(0);
}

void Routine::createBlkNoToMiamiBlkMap(CFG* cfg){
   for (auto it = dyn_addrToBlkNo->begin();it!= dyn_addrToBlkNo->end(); ++it){
      (*dyn_blkNoToMiamiBlk)[it->second] = cfg->findNodeContainingAddress(it->first.first);
      std::cout<<it->second<<" "<<cfg->findNodeContainingAddress(it->first.first)<<" "<<(unsigned int*)it->first.first<<" "<<(unsigned int*)it->first.second<<std::endl;
   }
         
}

BPatch_basicBlock* 
Routine::getBlockFromAddr(addrtype startAddr,addrtype endAddr){
   std::pair<addrtype,addrtype> addrs(startAddr,endAddr);
   if (dyn_addrToBlock->count(addrs) != 0){
      return (*dyn_addrToBlock)[addrs];
   }
   else{
      return NULL;
   }
}

int Routine::getBlockNoFromAddr(addrtype startAddr,addrtype endAddr){
   std::pair<addrtype,addrtype> addrs(startAddr,endAddr);
   if (dyn_addrToBlkNo->count(addrs) != 0){
      return (*dyn_addrToBlkNo)[addrs];
   }
   else{
      return -1;
   }
}

BPatch_function* 
Routine::getDynFunction(){
   return dyn_func;
}

int Routine::dyninst_discover_CFG(addrtype pc){
   
   // check if we have a block created for pc already
   BBMap::iterator mits = _blocks.upper_bound(pc);
   BBMap::iterator mite;  // = _blocks.upper_bound(_end-1);
#if DEBUG_BBLs
   if (name.compare(debugName)==0)
   {
      fprintf(stderr, "==> Routine %s, static check for block at address 0x%" PRIxaddr "\n", 
             name.c_str(), pc);
   }
#endif
   if (mits == _blocks.end() || pc<mits->second->getStartAddress())  // must create a new block
   // did not find block, must decode instructions
   {
      addrtype saddr = pc, eaddr;  // how far I have to decode
      if (mits == _blocks.end())
         eaddr = End();  // up to the end of the routine
      else
         eaddr = mits->second->getStartAddress();  // up to the following known block
      
      int res = 0, res2 = 0;
      int len = 0;    // get the instruction length in bytes
      addrtype reloc = 0;//InLoadModule()->RelocationOffset();
      while (pc<eaddr && !res && !res2)
      {

         // check if instruction is a branch
         res = instruction_at_pc_transfers_control((void*)(pc+reloc), eaddr-pc, len);
         dump_instruction_at_pc((void*)(pc+reloc),15);
         cout<<name<<" pc: "<<(unsigned int*)pc<<" "<<len<<endl;
         if (res<0)  // error while decoding
         {
            fprintf(stderr, " - ERROR: Bad decoding (%d) while discovering CFG for routine %s, at address 0x%" PRIxaddr 
                    " (+reloc=0x%" PRIxaddr "), eaddr-pc=%" PRIaddr ", len=%d\n", 
                    res, name.c_str(), pc, pc+reloc, eaddr-pc, len);
            // Should I do anything on a bad instruction? I must wait to see 
            // some concrete cases before making a decision.
            return (res);
         }
         
         // check if instruction has REP prefix
         if (!res)
            res2 = instruction_at_pc_stutters((void*)(pc+reloc), len, len);
         pc += len;
      }
      
#if DEBUG_BBLs
      if (name.compare(debugName)==0)
      {
         fprintf(stderr, "--> Routine %s, static check from address 0x%" PRIxaddr " ended at address 0x%"
              PRIxaddr ", res=%d, res2=%d\n", name.c_str(), saddr, pc, res, res2);
      }
#endif
      // we exit the loop when either we reached eaddr, or we found a transfer control instruction
      // which we consider to terminate the basic block
      if (0 && pc>eaddr)
      {
         // This case is actually possible when an instruction has a prefix, but there is
         // a path that jumps to the instruction itself, while another path falls through
         // the prefix.
         // Set an assert to catch if it happens in practice, but once verified, we can
         // remove the assert.
         // Found this case with a lock prefix. 
         fprintf(stderr, "ERROR: Instruction decoding of bounded binary code stopped at address 0x%" PRIxaddr
                 " which is beyond the earlier detected end address 0x%" PRIxaddr ".\n", pc, eaddr);
         assert (pc <= eaddr);
      }
      assert (pc > saddr);  // we must have advanced at least one instruction
      addrtype lpc = pc - len;  // PC address of last instruction
      bool create_fallthru_edge = true, discover_fallthru_code = false;
      assert (res || res2 || pc>=eaddr);
      
      // I must create a block from saddr to pc
      // If last instruction is a REP prefixed instruction, then create a separate
      // block for it. I will need to create another block for instructions up to
      // the REP instruction in this case.
      if (res2)  // it was a rep instruction
      {
         if (lpc > saddr)  // non-empty block before the REP instruction
         {
            AddBlock(saddr, lpc, PrivateCFG::MIAMI_CODE_BLOCK, 0);
            // connect the two blocks with a fall-through edge
            AddPotentialEdge(lpc, lpc, PrivateCFG::MIAMI_FALLTHRU_EDGE);
         }
         
         // A REP prefixed instruction should take more than 1 byte
         // The REP prefix is 1 byte, + the length of the actual instruction
         AddBlock(lpc, pc, PrivateCFG::MIAMI_REP_BLOCK, 0);
//         AddPotentialEdge(lpc+1, lpc+1, PrivateCFG::MIAMI_FALLTHRU_EDGE);
//         AddBlock(lpc+1, pc, PrivateCFG::MIAMI_CODE_BLOCK, 0);
         
         // add a loop edge for the REP instruction
         AddPotentialEdge(pc, lpc, PrivateCFG::MIAMI_DIRECT_BRANCH_EDGE);
         // I also need to create a bypass edge to go around the REP instruction
         // in case that its count is set to zero.
         AddPotentialEdge(lpc, pc, PrivateCFG::MIAMI_BYPASS_EDGE);
      } else  // no REP instruction found, add a single block
         AddBlock(saddr, pc, PrivateCFG::MIAMI_CODE_BLOCK, 0);
      
      if (res)  // last decoded instruction was a transfer control instruction
      {
         assert (!res2);  // should not find a REP prefixed branch
         
         MIAMI::DecodedInstruction dInst;
         addrtype target = 0;
         res = decode_instruction_at_pc((void*)(lpc+reloc), len, &dInst);
         assert (res>=0); // I just decoded this instruction earlier
         
         // I need to determine what type of branch instruction it is, direct or indirect
         // and if we can compute the branch target
         GFSliceVal targetF(sliceVal (0, TY_CONSTANT));
         
         InstrList::iterator lit = dInst.micro_ops.begin();
         InstrBin branch_type = IB_INVALID;
         for ( ; lit!=dInst.micro_ops.end() ; ++lit)
         {
            instruction_info *ii = &(*lit);
            if (ii->type==IB_br_CC || ii->type==IB_branch || ii->type==IB_jump || ii->type==IB_ret)
            {
               branch_type = ii->type;
               
               res = ComputeBranchTargetFormula(&dInst, ii, -1, dInst.pc, targetF); 
               coeff_t valueNum;
               ucoeff_t valueDen;
               if (!res && IsConstantFormula(targetF, valueNum, valueDen) && valueDen==1)
                  target = valueNum;
               break;
            }
         }
         if (branch_type==IB_branch || branch_type==IB_ret)
            // unconditonal branches and returns have no fall-through
            create_fallthru_edge = false;
         
         // If I have the target address, discover the CFG starting from the target
         PrivateCFG::EdgeType edge_type = PrivateCFG::MIAMI_DIRECT_BRANCH_EDGE;
         if (branch_type==IB_jump)
         {
            edge_type = PrivateCFG::MIAMI_FALLTHRU_EDGE;
            create_fallthru_edge = false;  // do not create fall-thru here, 
            // it will be created by AddCallSurrogate from the call-site block
            discover_fallthru_code = true;  // discover fall-thru code still
         }
         else if (branch_type==IB_ret)
            edge_type = PrivateCFG::MIAMI_RETURN_EDGE;
         
         if (branch_type == IB_jump)  // indirect function call
         {
            // add an unresolved call surrogate
            AddCallSurrogateBlock(lpc, pc, target, edge_type, true);
            // AddCallSurrogateBlock function creates the fall-through edge
            create_fallthru_edge = false;  // do not create it again, or it will be missplaced
            // but I still need to attempt code discovery from the instruction 
            // following the call
            discover_fallthru_code = true;
         }
         else if (branch_type == IB_ret)
            AddReturnEdge(pc);

         if (! target)  // target could not be resolved
         {
            // write something about it
#if DEBUG_INDIRECT_TRANSFER
            DEBUG_INDIRECT(3,
               fprintf(stderr, "STATIC CFG Discovery for routine %s, PC 0x%" PRIxaddr " (0x%" PRIxaddr ") is branch of type %d (%s) with unresolved target 0x%" PRIxaddr 
                         ", some sort of INDIRECT control transfer. ",
                        Name().c_str(), lpc-InLoadModule()->BaseAddr(), lpc, 
                        branch_type, Convert_InstrBin_to_string(branch_type), 
                        target);
               cerr << "DecodedFormula: " << targetF << endl;
            )
#endif
            
            // do not attempt to resolve Returns
            if (branch_type!=IB_ret && branch_type!=IB_jump)
            {
               // Find the block that contains the indirect jump
               // if _blocks is not empty, then we must be building the CFG 
               // as we speak; use the _blocks map to find the block.
               // Otherwise, it may mean the CFG is completed (why are we trying 
               // to resolve indirect jumps??), so use the provided findBlock method
               PrivateCFG::Node *brB = FindCFGNodeAtAddress(lpc);
               if (! brB)
               {
                  // we do not have a block for lpc, cry me a river
                  cerr << "ERROR: Cannot find a block that includes control transfer instruction at 0x"
                       << hex << lpc << dec << ". Cannot resolve indirect target." << endl;
               } else
               {
#if DEBUG_INDIRECT_TRANSFER
                  DEBUG_INDIRECT(1,
                     cerr << endl << "Indirect " << Convert_InstrBin_to_string(branch_type)
                          << " found at pc 0x" << hex << lpc << dec
                          << " during static CFG discovery for routine "
                          << Name() << ". Attempting to resolve it." << endl;
                  )
#endif
                  AddrSet targets;
                  ResolveBranchTargetFormula(targetF, g, brB, lpc, (InstrBin)branch_type, targets);
                  if (! targets.empty())  // uhu, we found some targets
                  {
                     AddrSet::iterator ait;
#if DEBUG_INDIRECT_TRANSFER
                     DEBUG_INDIRECT(1,
                        cerr << "INDIRECT " << Convert_InstrBin_to_string(branch_type)
                             << " at 0x" << hex << lpc 
                             << " resolves to the following possible targets:";
                        for (ait=targets.begin() ; ait!=targets.end() ; ++ait)
                           cerr << " 0x" << *ait;
                        cerr << dec << endl;
                     )
#endif
                  
                     for (ait=targets.begin() ; ait!=targets.end() ; ++ait)
                     {
                        AddPotentialEdge(pc, *ait, PrivateCFG::MIAMI_INDIRECT_BRANCH_EDGE);
                        discover_CFG(*ait);
                     }
                  }
               }
            }
            // this must be an indirect jump
            // branch must be instrumented to find address at run-time;
            // hmm, it can leave holes in my precious CFG
         } else
//         if (target && branch_type!=IB_jump)
         {
            
            // check if it is an intra-procedural branch
            // gmarin, 2013/10/16: Apparently, pin provides bad routine bounds,
            // I don't know why. Create call surrogate only on true calls.
            // Otherwise, create a branch, and keep decoding.

            // Add counter edge from this block to the newly discovered block
            if (branch_type!=IB_jump && branch_type!=IB_ret)
            {
               AddPotentialEdge(pc, target, PrivateCFG::MIAMI_DIRECT_BRANCH_EDGE);
            }
               
            discover_CFG(target);
         } 

         // if instruction has fall-through execution, create also a fal-through edge
         // only unconditional branches do not have fall-through
         // In PIN, calls are considered not to have fall-through, however, I create a
         // fall-through edge, but these edges are not profilable, bummer
#if DEBUG_BBLs
         if (name.compare(debugName)==0)
         {
            fprintf(stderr, "STATIC Routine %s, PC 0x%" PRIxaddr " is branch of type %d (%s) with target 0x%" PRIxaddr 
                      ", and fallthrough=%d\n",
                     Name().c_str(), lpc, branch_type, Convert_InstrBin_to_string((InstrBin)branch_type), 
                     target, create_fallthru_edge);
         }
#endif
      }
      
      if (create_fallthru_edge)
      {
         // create a fall-through edge to the next block.
         // What if the end address was the end of the routine? Should I create
         // a fall-through edge to the exit node, even if no return?
         if (pc < End())
         {
            AddPotentialEdge(pc, pc, PrivateCFG::MIAMI_FALLTHRU_EDGE);
         } else  // we are at the end of the routine and we have a fall-through
            // create a fall-through edge to the Exit node
            AddReturnEdge(pc, PrivateCFG::MIAMI_FALLTHRU_EDGE);
      }
      if ((create_fallthru_edge || discover_fallthru_code) && pc<End())
      {
         discover_CFG(pc);
      }
   } else if (pc>mits->second->getStartAddress())
   {
      // check if we enter the middle of a block; I must split the block
      AddBlock(pc, mits->second->getEndAddress(), PrivateCFG::MIAMI_CODE_BLOCK, 0);
      
      // If I created a code cache for this block, I need to separate it for the
      // two new blocks
   }
   return (0);         
}
