/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: static_branch_analysis.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Computes generic formulas for branch targets. Attempts to 
 * resolve targets of indirect branches using data flow analysis.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <iostream>
#include "static_branch_analysis.h"
#include "base_slice.h"
#include "uipair.h"
#include "InstructionDecoder.hpp"
#include "static_memory_analysis.h"
#include "debug_miami.h"
#include "private_routine.h"
#include "miami_containers.h"

#ifdef NO_STD_CHEADERS
# include <stddef.h>
# include <stdlib.h>
#else
# include <cstddef> // for 'NULL'
# include <cstdlib>
using std::abort; // For compatibility with non-std C headers
using std::exit;
#endif

using std::cerr;
using std::endl;
using std::hex;
using std::dec;


namespace MIAMI
{

/* Compute a branch target formula. The method evaluates only hardwired registers
 * (like the IP register) and constant values, of course. For indirect jumps, it will
 * leave the target registers unresolved. The second method, below, tries to slice
 * on the target registers to resolve the target of most indirect jumps.
 */
int
ComputeBranchTargetFormula(const DecodedInstruction *dInst, const instruction_info *ii,
                int uop_idx, addrtype pc, GFSliceVal& targetF)
{
   GFSliceVal tempF;
   int res = generic_formula_for_branch_target(dInst, ii, uop_idx, pc, tempF);
   if (res)
   {
      // this should never happen, assert
      cerr << "Could not get generic formula for branch target at pc 0x"
           << hex << pc << dec << ", uop index " << uop_idx << endl;
      assert (!"We should be able to compute a generic formula always.");
      return (res);
   }
   
   // We got the generic formula. 
   // Iterate over the terms of the formula and evaluate any hardwired registers
   targetF = GFSliceVal(sliceVal(0, TY_CONSTANT));
   GFSliceVal::iterator fit;
   for (fit=tempF.begin() ; fit!=tempF.end() ; ++fit)
   {
      if (fit->TermType() == TY_REGISTER)
      {
         const register_info& reg = fit->Register();
         long value = 0;
         if (is_hardwired_register(reg, dInst, value))
         {
            targetF += sliceVal(value, TY_CONSTANT);
         } else
            targetF += *fit;
      } else  // if it is not a register formula term
         targetF += *fit;
   }  // for each term of the generic formula
   return (0);
}


// Create a derived class from ReferenceSlice, mainly to deal with partial
// graphs for which we do not have loop back edge information.
// I need to use a different approach when I decide what edges I should take.
// I will probabaly use the marker method for visited blocks.
class PartialCFGSlice : public ReferenceSlice
{
public:
   // define a map from Nodes to Edges in which to store precomputed
   // paths that must be taken when looking for the next block to slice.
   typedef std::map<PrivateCFG::Node*, PrivateCFG::Edge*> NodeEdgeMap;

protected:
   using ReferenceSlice::SliceIn;
//   using ReferenceSlice::SliceOut;

   virtual bool SliceIn(PrivateCFG::Edge *e, const register_info& reg)
   {
      // Follow only forward edges, no loop back edges
      // We do not know which edges are forward or not.
      // In this case, just make sure we do not go back to a previously
      // visited block??
//      PrivateCFG::Node* bp = e->source();
//      return (bp->getEndAddress()>to_pc && !bp->visited(marker));

      // New version. I precompute acyclic paths that include the starting block.
      // I only follow edges that are part of this acyclic path.
      // Just check that the sink of this edge is in the map and that
      // this edge is its selected incoming Edge.

#if DEBUG_INDIRECT_TRANSFER
      DEBUG_INDIRECT(4,
         PrivateCFG::Node* bp = e->source();
         cerr << endl << "  --- PartialCFGSlice::SliceIn(Edge)"
              << ", with source node " << *bp 
              << ", start_pc=0x" << hex << start_pc << dec << endl;
      )
#endif

      PrivateCFG::Node* bn = e->sink();
      NodeEdgeMap::iterator neit = incomingEdges.find(bn);
      if (neit!=incomingEdges.end() && neit->second == e)
      {
#if DEBUG_INDIRECT_TRANSFER
         DEBUG_INDIRECT(4,
            PrivateCFG::Node* bs = e->source();
            fprintf (stderr, "SliceIn: taking edge E%d from B%d [0x%" PRIxaddr ",0x%" PRIxaddr "] to B%d [0x%"
                     PRIxaddr ",0x%" PRIxaddr "]\n", e->getId(), bs->getId(), bs->getStartAddress(),
                     bs->getEndAddress(), bn->getId(), bn->getStartAddress(), bn->getEndAddress());
         )
#endif
         return (true);
      }
      else
         return (false);

   }
   
public:
   PartialCFGSlice(const PrivateCFG* g, RFormulasMap& _rFormulas) 
       : ReferenceSlice(g, _rFormulas)
   {
   }
   
   void setIncomingEdgesMap(const NodeEdgeMap &_incEdges)
   {
      incomingEdges = _incEdges;
   }

   const NodeEdgeMap& getIncomingEdgesMap() const
   {
      return (incomingEdges);
   }

   // opType represents the op_type of the micro-op that must be resolved. I use the 
   // instruction pc and its op_type to identify its index relative to the start of
   // the block.
   void StartSlice(PrivateCFG::Node *b, const register_info& reg, InstrBin opType, 
                   addrtype _start_pc, addrtype _to_pc = 0)
   {
      to_pc = _to_pc;
#if DEBUG_INDIRECT_TRANSFER
      DEBUG_INDIRECT(3,
         cerr << "PartialCFGSlice::StartSlice, block [0x" << hex << b->getStartAddress()
              << ",0x" << b->getEndAddress() << dec << "]" << ", on reg " << reg 
              << ", start_pc=0x" << hex << _start_pc << dec << " micro-op type "
              << opType << " (" << Convert_InstrBin_to_string(opType) << ")" << endl;
      )
#endif
      assert(_start_pc>=b->getStartAddress() && _start_pc<b->getEndAddress());
      
      // test if we have a precomputed acyclic path that includes the starting block
      // if we do not, then we must clear the current one and compute a new path
      NodeEdgeMap::iterator nemit = incomingEdges.find(b);
      if (nemit == incomingEdges.end())  // did not find it
      {
         incomingEdges.clear();
         BuildACyclicPath(b);
      }

      // Find the starting uop index inside the block
      UopCodeCache *ucc = b->MicroOpCode();
      int num_uops = ucc->NumberOfUops();
      int iu = 0;
      for ( ; iu<num_uops ; ++iu)
      {
         const uOp_t& uop = ucc->UopAtIndex(iu);
         addrtype pc = uop.dinst->pc;
         // Hopefully the start_pc will match one of the instructions in block
         // If it doesn't, we exit on the first micro-op of an instruction at a 
         // larger PC anyway.
         if (pc > _start_pc) 
            break;
         if (pc==_start_pc && uop.iinfo->type==opType)
            break;
      }
      
      Slice(b, iu, reg, _start_pc, _to_pc);
   }

private:
   void BuildACyclicPath(PrivateCFG::Node *startb)
   {
      PrivateCFG::NodeList ba;
      PrivateCFG::EdgeList ea;

      PrivateCFG *g = startb->inCfg();
      unsigned int mm = g->new_marker();
      
      if (RecursiveBuildACyclicPath(startb, ba, ea, mm))
      {
         // now add the blocks and edges to the incomingEdges map
         assert (ba.size() == ea.size());
         // Next assert is not always true. It fails for .plt stubs
         // that typically have a single basic block
//         assert (ba.size() > 0);
         
         PrivateCFG::NodeList::iterator nit = ba.begin();
         PrivateCFG::EdgeList::iterator eit = ea.begin();
         for ( ; nit!=ba.end() && eit!=ea.end() ; ++nit, ++eit)
         {
            incomingEdges.insert(NodeEdgeMap::value_type(*nit, *eit));
         }
         assert(nit==ba.end() && eit==ea.end());
         
#if DEBUG_INDIRECT_TRANSFER
         DEBUG_INDIRECT(4,
            fprintf(stderr, "NEW PartialCFGSlice::BuildACyclicPath found acyclic path: ");
            for (nit=ba.begin() ; nit!=ba.end() ; ++nit)
               fprintf(stderr, "-> B%d [0x%" PRIxaddr ",0x%" PRIxaddr "] ", 
                   (*nit)->getId(), (*nit)->getStartAddress(), (*nit)->getEndAddress());
            fprintf(stderr, "\n");
         )
#endif
      } else  // could not find an acyclic path from the start block
      {
         // for now print an error message and assert
         fprintf(stderr, "ERROR: Could not build an acyclic path starting from block %d [0x%" 
                 PRIxaddr ",0x%" PRIxaddr "]. I do not know what to do.\n",
              startb->getId(), startb->getStartAddress(), startb->getEndAddress());
         assert(!"Chech the CFG of this routine.");
      }
   }
   
   bool RecursiveBuildACyclicPath(PrivateCFG::Node *b, PrivateCFG::NodeList &ba, 
                PrivateCFG::EdgeList &ea, unsigned int mm)
   {
#if DEBUG_INDIRECT_TRANSFER
      DEBUG_INDIRECT(5,
         cerr << "RecursivePath: path marker=" << mm << ", to_pc="
              << hex << to_pc << dec
              << ", consider block " << *b << "[0x" << hex 
              << b->getStartAddress() << ",0x" << b->getEndAddress()
              << dec << "] visited with " << b->getVisitedWith() << endl;
      )
#endif

      if (b->visited(mm))  // found node already in the path, we closed a cycle
         return (false);
      if (b->isCfgEntry())
         return (true);
      
      b->visit(mm);
      ba.push_back(b);
      
      if (b->getStartAddress()<=to_pc)  // we do not have Cfg Entry nodes in partial graphs
      {
         ea.push_back(NULL);
         return (true);
      }
      
      PrivateCFG::IncomingEdgesIterator ieit(b);
      int num_incoming = 0;
      while ((bool)ieit)
      {
         PrivateCFG::Edge *e = ieit;
         PrivateCFG::Node* bp = e->source();

         ea.push_back(e);
         if (RecursiveBuildACyclicPath(bp, ba, ea, mm))
            return (true);
         // otherwise, we did not find a path
         // remove current edge and try a new one
         ea.pop_back();
         ++ num_incoming;  // count how many incoming edges we saw
         
         ++ ieit;
      }
      
      // did not find an edge
      // if the block has no incoming edges, it is likely a routine entry point
      // return success in this case
      if (num_incoming == 0)
      {
         ea.push_back(NULL);
         return (true);
      }

      ba.pop_back();
      // b->visit(mm-1);
      
      return (false);
   }
   

   NodeEdgeMap incomingEdges;
};


/* 
 * This class extends the normal slicing class, adding an additional stopping
 * condition for slicing. Slicing on a register stops if the register is used 
 * in a compare operation. At that point, the other operand of the compare
 * operation is saved and associated with the sliced register. The value it is
 * compared against represents the upper size of an eventual dispatch table.
 */
class CompareSlice : public PartialCFGSlice
{
   typedef PartialCFGSlice SUPER;
   
public:
   CompareSlice(const PrivateCFG* g, RFormulasMap& _rFormulas) 
          : PartialCFGSlice(g, _rFormulas)
   {
   }

   ~CompareSlice()
   {
      for (RFPArray::iterator rit=cmpValues.begin() ; rit!=cmpValues.end() ; ++rit)
         delete (*rit);
      cmpValues.clear();
   }

   void cmp_clear()
   {
      for (RFPArray::iterator rit=cmpValues.begin() ; rit!=cmpValues.end() ; ++rit)
         delete (*rit);
      cmpValues.clear();
   }

   RegFormula* getCmpValueForReg(const register_info& regarg /*, addrtype pc*/)
   {
      RFPArray::iterator rit = cmpValues.end();
      for( ; rit!=cmpValues.begin() ; )
      {
         -- rit;
         if ((*rit)->reg.Overlaps (regarg))
         {
            // sanity check first; what should I do if the found register does not
            // define the full bit range for requested register
            if (! (*rit)->reg.IncludesRange(regarg))
            {
#if DEBUG_INDIRECT_TRANSFER
               DEBUG_INDIRECT(3,
                  cerr << "ATTENTION: CompareSlice::getCmpValueForReg with request for register "
                       << regarg << " found value for register with non-inclusive range "
                       << (*rit)->reg << ", value formula=" << (*rit)->formula << endl;
               )
#endif
            }
            return (*rit);
         }
      }
      return NULL;
   }
  
   RegFormula* getCmpValueForRegAndDelete(const register_info& regarg/*, addrtype pc*/)
   {
      RFPArray::iterator rit = cmpValues.end();
      for( ; rit!=cmpValues.begin() ; )
      {
         -- rit;
         if ((*rit)->reg.Overlaps (regarg))
         {
            // sanity check first; what should I do if the found register does not
            // define the full bit range for requested register
            if (! (*rit)->reg.IncludesRange(regarg))
            {
#if DEBUG_INDIRECT_TRANSFER
               DEBUG_INDIRECT(3,
                  cerr << "ATTENTION: CompareSlice::getCmpValueForRegAndDelete with request for register "
                       << regarg << " found value for register with non-inclusive range "
                       << (*rit)->reg << ", value formula=" << (*rit)->formula << endl;
               )
#endif
            }
            RegFormula *temp = (*rit);
            cmpValues.erase(rit);
            return (temp);
         }
      }
      return NULL;
   }
   
   void printAllCompareValues()
   {
      cerr << "CompareSlice found the following compare mappings:" << endl;
      RFPArray::iterator rit = cmpValues.begin();
      for( ; rit!=cmpValues.end() ; ++rit)
      {
         cerr << "  - " << (*rit)->reg << "  =>  " << (*rit)->formula << endl;
      }
   }

protected:
   virtual bool ShouldSliceUop(const uOp_t& uop, const register_info& reg, register_info& areg)
   {
      // stop if this instruction does a compare, or a binary AND operation with a 
      // constant value that is a (power of 2) - 1
      CanonicalOp cop = uop_canonical_decode(uop.dinst, uop.iinfo);
      if ( (cop==OP_CMP || 
              (cop==OP_AND && uop.iinfo->num_imm_values==1 &&
               (uop.iinfo->imm_values[0].value.s & (uop.iinfo->imm_values[0].value.s + 1))==0
              )
           ) && uop.Reads(reg, false, areg) )
         return (false);
      else
         return (SUPER::ShouldSliceUop(uop, reg, areg));
   }
   
   virtual bool UopTerminatesSlice(const uOp_t& uop, int uopidx, const register_info& reg, 
                  register_info& areg)
   {
      // do not continue slicing if this instruction does a compare, or a 
      // binary AND operation with a constant value that is a (power of 2)-1
      CanonicalOp cop = uop_canonical_decode(uop.dinst, uop.iinfo);
      if ( (cop==OP_CMP || 
              (cop==OP_AND && uop.iinfo->num_imm_values==1 &&
               (uop.iinfo->imm_values[0].value.s & (uop.iinfo->imm_values[0].value.s + 1))==0
              )
           ) && uop.Reads(reg, false, areg) )
      {
#if DEBUG_INDIRECT_TRANSFER
         DEBUG_INDIRECT(5,
            cerr << "CompareSlice::UopTerminatesSlice at pc 0x" << hex 
                 << uop.dinst->pc << dec << ", uopidx=" << uopidx
                 << " on register " << reg << endl;
         )
#endif
         // However, before returning, save the value of the other operand
         // We may even need to slice on it later if it is not a constant
         const instruction_info *ii = uop.iinfo;
         assert(ii->num_src_operands == 2);  // we should always have 2, right?
         // Iterate over all the source operands and try to find which one 
         // is the one represented by this register
         // I must consider all the possible cases
         sliceVal otherOperand;
         int regIndex = -1;
         int i;
         for (i=0 ; i<ii->num_src_operands ; ++i)
         {
            int op_num = extract_op_index(ii->src_opd[i]);
            int otype = extract_op_type(ii->src_opd[i]);
            
            // operand cannot be of type memory in my decoding
            // Only memory micro-ops can have memory operands, and this 
            // is a compare micro-op
            assert (otype != OperandType_MEMORY);
            switch (otype)
            {
               case OperandType_REGISTER:
               {
                  register_info crtReg = mach_inst_reg_operand(uop.dinst, op_num);
                  // if registers match, and we did not see a match yet, this is it
                  if (reg.Overlaps(crtReg) && regIndex<0)
                     regIndex = i;
                  else
                     otherOperand = sliceVal(1, TY_REGISTER, uop.dinst->pc, uopidx, crtReg);
               }
               break;

               case OperandType_INTERNAL:
               {
                  int rwidth = ii->width;
                  if (ii->exec_unit==ExecUnit_VECTOR)
                     rwidth *= ii->vec_len;
                     
                  register_info crtReg = register_info(op_num, RegisterClass_TEMP_REG, 0, rwidth-1);
                  // if registers match, and we did not see a match yet, this is it
                  if (reg.Overlaps(crtReg) && regIndex<0)
                     regIndex = i;
                  else
                     otherOperand = sliceVal(1, TY_REGISTER, uop.dinst->pc, uopidx, crtReg);
               }
               break;

               case OperandType_IMMED:
               {
                  // this must be the other operand, since the sliced 
                  // operand should be a register
                  otherOperand = sliceVal(ii->imm_values[op_num].value.s, TY_CONSTANT);
               }
               break;
            }
         }
         // we should do some sanity checking here
         assert(regIndex >= 0);  // we should have seen the sliced register
         assert(otherOperand.IsDefined());
         // Save otherOperand into a map indexed by slicing register
         cmpValues.push_back(new RegFormula(reg, GFSliceVal(otherOperand)));
         
         // push also a value for the register that we are not slicing
         _values.push_back (new RegFormula (reg, 
               GFSliceVal(sliceVal (1, TY_REGISTER, uop.dinst->pc, uopidx, reg)) ));
         
         return (true);
      } else // otherwise, use the default condition
         return (SUPER::UopTerminatesSlice(uop, uopidx, reg, areg));
   }


private:
   RFPArray cmpValues;  // save the formulas found for the compare operands
};

static bool
get_formula_terms(GFSliceVal& formula, sliceVal& load_term, sliceVal& reg_term, 
              addrtype& base_addr)
{
   GFSliceVal::iterator sit;
   bool has_other = false;
   for (sit=formula.begin() ; sit!=formula.end() && !has_other ; ++sit)
   {
      int tt = sit->TermType();
      switch (tt)
      {
         case TY_LOAD:
            if (load_term.IsDefined())  // we saw another load before, bad case
               has_other = true;
            else
               load_term = *sit;
            break;
            
         case TY_REGISTER:
            if (reg_term.IsDefined())  // we saw another reg before, bad case
               has_other = true;
            else
               reg_term = *sit;
            break;
            
         case TY_CONSTANT:
            base_addr = sit->ValueNum() / sit->ValueDen();
            break;
            
         default:
            has_other = true;
            break;
      }
   }
   return (has_other);
}

static coeff_t 
read_value_at_address(addrtype mem_addr, int read_width, PrivateLoadModule *img)
{
   coeff_t value = 0;
   switch (read_width)
   {
      case 8:
         value = *((int8_t*)mem_addr);
         break;
      
      case 16:
         value = *((int16_t*)mem_addr);
         break;
      
      case 32:
         value = *((int32_t*)mem_addr);
         break;
      
      case 64:
         value = *((int64_t*)mem_addr);
         break;
      
      default:
         cerr << "ERROR: Unexpected read width value: " << read_width << endl;
         exit(-1);
         break;
   }
   return (value);
}


/* Compute a branch target formula. For indirect jumps, resolve the target registers
 * using data flow analysis.
 */
int
ResolveBranchTargetFormula(const GFSliceVal& targetF, PrivateCFG *cfg, 
                PrivateCFG::Node *brB, addrtype lpc, InstrBin brType, 
                AddrSet& targets)
{
   GFSliceVal derivedF(sliceVal(0, TY_CONSTANT));
   // create a ReferenceSlice and try to get the value of the registers
   RFormulasMap *rformulas = new RFormulasMap(0);
   PartialCFGSlice *rslice = new PartialCFGSlice(cfg, *rformulas);
   PrivateRoutine *rout = cfg->InRoutine();
   PrivateLoadModule *img = rout->InLoadModule();
   addrtype rstart = rout->Start();
   // iterate over the temp formula to slice its register terms up to the
   // start of the routine
   
   GFSliceVal::iterator sit;
   for (sit=targetF.begin() ; sit!=targetF.end() ; ++sit)
   {
      int tt = sit->TermType();
      addrtype tpc = sit->Address();
      std::cout<<"OZGURDBG sliceVal: "<<sit->toString()<<std::endl;
#if DEBUG_INDIRECT_TRANSFER
      DEBUG_INDIRECT(3,
         int uop_idx = sit->UopIndex();
         cerr << "Resolving formula: tt=" << tt << ", tpc=" << hex 
              << tpc << ", rstart=" << rstart << dec 
              << ", uop_idx=" << uop_idx << endl;
      )
#endif
      // I have to slice back on all register terms, no matter the
      // uop index;
      if (tt == TY_REGISTER) // && uop_idx==(-1))
      {
         const register_info &reg = sit->Register();
         coeff_t tval = sit->ValueNum() / sit->ValueDen();
         rslice->clear();
         rslice->StartSlice(brB, reg, brType, tpc, rstart);
         RegFormula *rf = rslice->getValueForRegAndDelete(reg);
         if (rf == NULL)
            derivedF += (*sit);
         else
         {
            rf->formula *= tval;
            derivedF += rf->formula;
            delete (rf);
         }
      } else  // not a REGISTER or LOAD, just add the term unchanged
      {
         derivedF += (*sit);
      }
   }

#if DEBUG_INDIRECT_TRANSFER
   DEBUG_INDIRECT(3,
      cerr << "ResolvedFormula(" 
           << Convert_InstrBin_to_string(brType)
           << " at 0x" << hex << lpc << dec << "): " << derivedF 
           << " / OrigF: " << targetF << endl;
   )
#endif
   // Now check for various indirect branch patterns
   // 1) we could resolve the formula to a constant -> set the value as target
   // 2) the formula contains a LOAD  and an optional constant term only.
   //    - try to resolve the address of the load
   coeff_t valueNum;
   ucoeff_t valueDen;
   if (IsConstantFormula(derivedF, valueNum, valueDen) && valueDen==1)
   {
#if DEBUG_INDIRECT_TRANSFER
      DEBUG_INDIRECT(1,
         cerr << "Indirect " << Convert_InstrBin_to_string(brType)
              << " at 0x" << hex << lpc << dec << " resolves to a CONSTANT: 0x"
              << hex << valueNum << dec << endl;
      )
#endif
      targets.insert((addrtype)valueNum);
   } else
   {
      addrtype base_addr = 0;  // store here the value of an eventual constant term
      sliceVal load_term;      // store here the load term if found
      sliceVal reg_term;       // store here the reg term if found
      bool has_other = get_formula_terms(derivedF, load_term, reg_term, base_addr);
      
      if (has_other || (load_term.IsDefined() && reg_term.IsDefined()))
      {
#if DEBUG_INDIRECT_TRANSFER
         DEBUG_INDIRECT(2,
            cerr << "Indirect " << Convert_InstrBin_to_string(brType)
                 << " at 0x" << hex << lpc << dec 
                 << " does not match a known pattern. Abort." << endl;
         )
#endif
      } else
      {
         // we are looking for several possible patterns
         // I'm not sure if they are all possible in practice
         // For a LOAD, get the address of the load first. We are looking
         // for an address that contains a constant base (start of dispatch table)
         // + a reg index scaled by a scaling factor.
         // Can the LOAD have a coefficient other than 1? It may be an
         // additional scaling factor; For now, try to infer as much info as possible
         addrtype table_addr = 0;
         int num_entries = 1;
//         register_info index = MIAMI_NO_REG;
         width_t table_entry_width = 0; // how large is an entry into the table (in bits)
         coeff_t index_scale = 1;
         coeff_t load_scale = 1;
         bool hasErrors = false;
         
         if (load_term.IsDefined())  // formula constains a load; try to resolve its address
         {
            addrtype tpc = load_term.Address();
            int uop_idx = load_term.UopIndex();
            load_scale = load_term.ValueNum() / load_term.ValueDen();
            // I need to find the block that contains instruction at tpc
            PrivateCFG::Node *node = rout->FindCFGNodeAtAddress(tpc);
            if (! node)
            {
               // we do not have a block for tpc, cry some more
#if DEBUG_INDIRECT_TRANSFER
               DEBUG_INDIRECT(0,
                  cerr << "ERROR: Cannot find a block that includes load instruction at 0x"
                       << hex << tpc << dec << ". Cannot check if indirect jump matches known pattern." << endl;
               )
#endif
               hasErrors = true;
            }
            if (! hasErrors)
            {
               // I need to get the DecodedInstruction for this instructions, bleah
               UopCodeCache *ucc = node->MicroOpCode();
               int num_uops = ucc->NumberOfUops();
               assert (uop_idx>=0 && uop_idx<num_uops);
               const uOp_t& uop = ucc->UopAtIndex(uop_idx);
               assert(InstrBinIsLoadType(uop.iinfo->type));
               table_entry_width = uop.iinfo->width;
               if (uop.iinfo->exec_unit==ExecUnit_VECTOR)
                  table_entry_width *= uop.iinfo->vec_len;
               
               int opidx = uop.getMemoryOperandIndex();  // memory operand index in original DecodedInstruction
               if (opidx<0)  // dump the instruction decoding
               {
                  DumpInstrList(uop.dinst);
                  assert (opidx >= 0);
               }

#if DEBUG_INDIRECT_TRANSFER
               DEBUG_INDIRECT(3,
                  cerr << "Resolving address for LOAD term: " << load_term 
                       << " at pc=" << hex << tpc << ", rstart=" << rstart << dec 
                       << ", uop_idx=" << uop_idx << endl;
               )
#endif

               // try a full solution first
               rslice->clear();
               GFSliceVal loadF = rslice->ComputeFormulaForMemoryOperand(node, uop.dinst, 
                                uop_idx, opidx, tpc);
               if (IsConstantFormula(loadF, valueNum, valueDen) && valueDen==1)
               {
#if 0
                  DEBUG_INDIRECT(1,
                     cerr << "LoadTerm(" << load_term
                          << " at 0x" << hex << tpc << dec 
                          << ") resolves to a constant address 0x" 
                          << hex << valueNum << dec << endl;
                  )
#endif
                  table_addr = valueNum;
                  num_entries = 1;
                  index_scale = 0;
               } else
               {
                  CompareSlice cmpslice(cfg, *rformulas);
                  cmpslice.setIncomingEdgesMap(rslice->getIncomingEdgesMap());
                  loadF = cmpslice.ComputeFormulaForMemoryOperand(node, uop.dinst, 
                                   uop_idx, opidx, tpc);

                  sliceVal load_in_load;   // store here the load term if found
                  sliceVal reg_in_load;    // store here the reg term if found
                  table_addr = 0;
                  hasErrors = get_formula_terms(loadF, load_in_load, reg_in_load, table_addr);
                  if (load_in_load.IsDefined())
                     hasErrors = true;   // the dispatch table address cannot depend on any loads
                     
                  if (! hasErrors)  // so far, so good
                  {
                     // we do not have a load_in_load, but we must have a reg_in_load
                     // Otherwise, the load formula would be a constant, and we would have
                     // caught this case above
                     assert(reg_in_load.IsDefined());  // we have a reg index
                     
                     // we must check that we have a compare term that gives us the
                     // table size; in that case, get the index scaling factor
                     RegFormula *cmpf = cmpslice.getCmpValueForReg(reg_in_load.Register());
                     if (cmpf && IsConstantFormula(cmpf->formula, valueNum, valueDen) && 
                          valueDen==1 && valueNum>=0)
                     {
                        num_entries = valueNum+1;
                     } else
                     {
                        hasErrors = true;  // we do not know the number of entries
                        num_entries = -1;
                     }
                     // I should not delete cmpf if I do not remove the pointer from 
                     // the slice vector; crazy stuff is going on.
                     //if (cmpf) delete (cmpf);
                     index_scale = reg_in_load.ValueNum() / reg_in_load.ValueDen();
                  }
                  
                  if (hasErrors)
                  {
#if DEBUG_INDIRECT_TRANSFER
                     DEBUG_INDIRECT(2,
                        cerr << "LoadTermCmpFormula(" << load_term
                             << " at 0x" << hex << tpc << dec 
                             << ") does not match known pattern: " << loadF 
                             << endl;
                        cmpslice.printAllCompareValues();
                     )
#endif
                  }
               }  // load address is not a constant
               
               if (! hasErrors)
               {
#if DEBUG_INDIRECT_TRANSFER
                  DEBUG_INDIRECT(1,
                     cerr << "LoadTerm(" << load_term
                          << " at 0x" << hex << tpc << dec 
                          << ") resolves to LOAD" << table_entry_width
                          << " [0x" << hex << table_addr << dec 
                          << " + " << index_scale << "*i], where i<" 
                          << num_entries << endl;
                  )
#endif
               
                  for (int e=0 ; e<num_entries ; ++e)
                  {
                     coeff_t new_offset = read_value_at_address(table_addr+e*index_scale, table_entry_width, img);
                     targets.insert(base_addr + new_offset*load_scale);
                  }
               }
            }  // I could find the CFG block for the load term
         
         }  // has LOAD
         else  // it must have a reg term
         {
            assert(reg_term.IsDefined());
#if DEBUG_INDIRECT_TRANSFER
            DEBUG_INDIRECT(2,
               cerr << "Indirect " << Convert_InstrBin_to_string(brType)
                    << " at 0x" << hex << lpc << dec 
                    << " resolves to an index based pattern: 0x" << hex
                    << base_addr << " + " << reg_term << dec 
                    << " that MIAMI cannot handle yet." << endl;
            )
#endif
            
         }  // it must have a reg term
      
      } // not an unknown pattern
      
   }  // not a CONSTANT target
   
   delete (rslice);
   delete (rformulas);
   
   return (0);
}

}  /* namespace MIAMI */
