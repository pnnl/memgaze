/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: static_memory_analysis.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Computes generic formulas for memory references' addresses
 * and strides, using data flow analysis.
 */

#ifndef _STATIC_MEMORY_ANALYSIS_H
#define _STATIC_MEMORY_ANALYSIS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <string>

#include "miami_types.h"
#include "slice_references.h"
#include "base_slice.h"
#include "uipair.h"
#include "instr_info.H"
#include "InstructionDecoder.hpp"
#include "PrivateCFG.h"
#include "bucket_hashmap.h"

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

//using namespace std;
namespace MIAMI  // add everything to namespace MIAMI
{

class RegFormula
{
public:
   RegFormula(const register_info& _reg, const GFSliceVal &_formula)
        : pc(0), idx(-1), reg(_reg), formula(_formula)
   {}
   RegFormula(addrtype _pc, int _idx, const GFSliceVal &_formula)
        : pc(_pc), idx(_idx), formula(_formula)
   {}

   addrtype pc;
   int idx;
   register_info reg;
   GFSliceVal formula;
};

typedef std::vector<RegFormula*> RFPArray;

static int deleteFormulaInMap(void *arg0, addrtype pc, void *value)
{
//   fprintf(stderr, "Deleting atg0=%p, pc=0x%" PRIxaddr ", value=%p\n", arg0, pc, (void*)(*(addrtype*)value)
   GFSliceVal *gf = static_cast<GFSliceVal*>((void*)(*(addrtype*)value));
   if (gf)
      delete (gf);
   return (0);
}

extern GFSliceVal* defaultGFSvP;
// create an interface for caching reference formulas based on pc.
// One x86 instruction may have multiple micro-ops and instructions can
// be 1 byte long. We must be able to refer to any of the micro-ops.
// The hashmap key is a 64-bit integer. To create the key, we shift 
// the instruction's address left by 3 bits, and use the lowest 3 bits 
// for the micro-op index. No instruction has more than 8 micro-ops
// the way we decode them (1 primary uop + 1 uop for each memory operand
// + 1 uop for a rep prefix).
//
class GFSvPCache
{
   typedef MIAMIU::BucketHashMap<addrtype, GFSliceVal*, &defaultGFSvP> AddrGFSvPMap;

public:

   GFSliceVal& operator() (addrtype _pc, int idx)
   {
      addrtype key = (_pc<<3) + (idx&7);
      GFSliceVal* &ff = storage[key];
      if (!ff)
         ff = new GFSliceVal();
      return (*ff);
   }
   bool hasKey(addrtype _pc, int idx)
   {
      addrtype key = (_pc<<3) + (idx&7);
      return (storage.is_member(key));
   }

   virtual ~GFSvPCache()
   {
      storage.map(deleteFormulaInMap, NULL);
   }
  
private:
   AddrGFSvPMap storage;
};


class ReferenceSlice : public BaseSlice
{
public:

   // Constructor:
   //
   ReferenceSlice(const PrivateCFG* g, RFormulasMap& _rFormulas)
      : BaseSlice(g, _rFormulas)
   {
   }
  
   virtual ~ReferenceSlice()
   {
      for (RFPArray::iterator rit=_values.begin() ; rit!=_values.end() ; ++rit)
         delete (*rit);
      _values.clear();
   }
  
   RegFormula* getValueForReg(const register_info& regarg /*, addrtype pc*/)
   {
      RFPArray::iterator rit = _values.end();
      for( ; rit!=_values.begin() ; )
      {
         -- rit;
         if ((*rit)->reg.Overlaps (regarg))
         {
            // sanity check first; what should I do if the found register does not
            // define the full bit range for requested register
            if (! (*rit)->reg.IncludesRange(regarg))
            {
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(3,
                  cerr << "ATTENTION: ReferenceSlice::getValueForReg with request for register "
                       << regarg << " found value for register with non-inclusive range "
                       << (*rit)->reg << ", value formula=" << (*rit)->formula << endl;
               )
#endif
//               assert(!"Non-inclusive range in ReferenceSlice::getValueForReg. Check this case.");
            }
            return (*rit);
         }
      }
      return NULL;
   }
  
   RegFormula* getValueForRegAndDelete(const register_info& regarg/*, addrtype pc*/)
   {
      RFPArray::iterator rit = _values.end();
      for( ; rit!=_values.begin() ; )
      {
         -- rit;
         if ((*rit)->reg.Overlaps (regarg))
         {
            // sanity check first; what should I do if the found register does not
            // define the full bit range for requested register
            if (! (*rit)->reg.IncludesRange(regarg))
            {
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(3,
                  cerr << "ATTENTION: ReferenceSlice::getValueForRegAndDelete with request for register "
                       << regarg << " found value for register with non-inclusive range "
                       << (*rit)->reg << ", value formula=" << (*rit)->formula << endl;
               )
#endif
//               assert(!"Non-inclusive range in ReferenceSlice::getValueForRegAndDelete. Check this case.");
            }
            RegFormula *temp = (*rit);
            _values.erase(rit);
            return (temp);
         }
      }
      return NULL;
   }

   RegFormula* getValueForReg(addrtype pc, int idx)
   {
      RFPArray::iterator rit = _values.end();
      for( ; rit!=_values.begin() ; )
      {
         -- rit;
         if ((*rit)->pc==pc && (*rit)->idx==idx)
         {
            return (*rit);
         }
      }
      return NULL;
   }
  
   RegFormula* getValueForRegAndDelete(addrtype pc, int idx)
   {
      RFPArray::iterator rit = _values.end();
      for( ; rit!=_values.begin() ; )
      {
         -- rit;
         if ((*rit)->pc==pc && (*rit)->idx==idx)
         {
            RegFormula *temp = (*rit);
            _values.erase(rit);
            return (temp);
         }
      }
      return NULL;
   }

protected:
   using BaseSlice::SliceIn;
   using BaseSlice::SliceOut;

   // save the value of a register at a given PC
   virtual void RecordRegisterValue(const register_info& reg, const uOp_t& uop, coeff_t value);

   // Slice back until encountering a function call, a load operation, or start
   // of function;
   // another stopping condition can be a unsupported instruction;
   // in general, go back along add, sub, mov, shift, sethi, 
   // (or, and, not, xor - for constant values only);
   // When slicing back, several paths may exist. Avoid backedges in order to
   // avoid an infinite loop. Stop at the first value.
   //
   virtual bool SliceIn(PrivateCFG::Node *b, int from_uop, const register_info& reg);
   virtual void SliceOut(PrivateCFG::Node *b, int from_uop, const register_info& reg);

   // called at the beginning of SliceNext, check if we reached a stopping condition
   virtual bool SliceNextIn(PrivateCFG::Node *b, const register_info& reg);
   virtual void SliceNextOut(PrivateCFG::Node *b, const register_info& reg);

   virtual bool SliceIn(PrivateCFG::Edge *e, const register_info& reg);
   virtual void SliceOut(PrivateCFG::Edge *e, const register_info& reg);

public:
   void clear()
   {
      for (RFPArray::iterator rit=_values.begin() ; rit!=_values.end() ; ++rit)
         delete (*rit);
      _values.clear();
   }

   GFSliceVal ComputeFormulaForMemoryOperand(PrivateCFG::Node *node, const DecodedInstruction *dInst, 
             int uop_idx, int op_num, addrtype pc);

protected:
   void printAllValues()
   {
      cerr << "ReferenceSlice contains the following stack of values (" 
           << _values.size() << "):" << endl;
      RFPArray::iterator rit = _values.end();
      for( ; rit!=_values.begin() ; )
      {
         -- rit;
         cerr << "  - " << (*rit)->reg << "  =>  " << (*rit)->formula << endl;
      }
   }

   RegFormula* RegFormulaForOperand(const DecodedInstruction *dinst, 
                     const instruction_info *ii, int op_num);

   // Value of slice.
   //
   RFPArray _values;
   GFSvPCache cacheValues;
};  // ReferenceSlice


//#define MIAMI_NO_DFS_INDEX   ((unsigned long)-1)

class RegCache
{
public:
   RegCache(const register_info& _reg, addrtype _pc, int _idx, unsigned long _dfs=0)
        : reg(_reg), defPc(_pc), idx(_idx), targetDFSIndex(_dfs)
   {}

   const register_info reg;
   addrtype defPc;
   int idx;
   unsigned long targetDFSIndex;
};

typedef std::vector<RegCache*> RCPArray;


class StrideInst
{
public:
   StrideInst(unsigned long _dfs = 0)
   {
      loopCnt = 0; loopReg = MIAMI_NO_REG; loopHb = 0;
      formula = NULL; final_formula = NULL;
      is_strideF = final_strideF = 0;
      dfsIndex = _dfs; useCached = false;
      dfsTarget = 0; cacheKey = 0;
   }
   
   StrideInst(const StrideInst& si)
   {
      loopCnt = si.loopCnt; loopReg = si.loopReg;
      is_strideF = si.is_strideF; loopHb = si.loopHb;
      dfsIndex = si.dfsIndex;
      useCached = si.useCached;
      dfsTarget = si.dfsTarget;
      cacheKey = si.cacheKey;
      final_strideF = si.final_strideF;
      if (si.formula==NULL)
         formula = NULL;
      else
         formula = new GFSliceVal(*(si.formula));
      if (si.final_formula==NULL)
         final_formula = NULL;
      else
         final_formula = new GFSliceVal(*(si.final_formula));
   }
   
   ~StrideInst()
   {
      if (formula)
         delete formula;
      if (final_formula)
         delete final_formula;
   }
   
   StrideInst& operator= (const StrideInst& si)
   {
      loopCnt = si.loopCnt; loopReg = si.loopReg;
      is_strideF = si.is_strideF; loopHb = si.loopHb;
      dfsIndex = si.dfsIndex;
      useCached = si.useCached;
      dfsTarget = si.dfsTarget;
      cacheKey = si.cacheKey;
      final_strideF = si.final_strideF;
      if (si.formula==NULL)
         formula = NULL;
      else
         formula = new GFSliceVal(*(si.formula));
      if (si.final_formula==NULL)
         final_formula = NULL;
      else
         final_formula = new GFSliceVal(*(si.final_formula));
      return (*this);
   }
   
   int loopCnt;
   unsigned long dfsIndex;
   register_info loopReg;
   int loopHb;      // keep track of the high_bits value for the loopReg
   int is_strideF;  // specifies if the formula is a formula for the stride
                    // 0 - is not
                    // 1 - it is a partial formula; did not complete the cycle
                    // 2 - it is a stride formula with the cycle finished
   int final_strideF;  // same meaning as above
   GFSliceVal *formula;
   GFSliceVal *final_formula;
   unsigned long dfsTarget;
   unsigned long cacheKey;  // a strideF=1 formula is considered cached only
                            // if its cacheKey == the current sliceKey
   bool useCached;
};


static int deleteStrideInMap(void *arg0, addrtype pc, void *value)
{
//   fprintf(stderr, "Deleting atg0=%p, pc=0x%" PRIxaddr ", value=%p\n", arg0, pc, (void*)(*(addrtype*)value)
   StrideInst *si = static_cast<StrideInst*>((void*)(*(addrtype*)value));
   if (si)
      delete (si);
   return (0);
}

extern StrideInst* defaultStrideInstP;
// create an interface for caching reference formulas based on pc.
// One x86 instruction may have multiple micro-ops and instructions can
// be 1 byte long. We must be able to refer to any of the micro-ops.
// The hashmap key is a 64-bit integer. To create the key, we shift 
// the instruction's address left by 3 bits, and use the lowest 3 bits 
// for the micro-op index. No instruction has more than 8 micro-ops
// the way we decode them (1 primary uop + 1 uop for each memory operand
// + 1 uop for a rep prefix).
//
class SIPCache
{
//   typedef MIAMIU::HashMap<addrtype, StrideInst*, &defaultStrideInstP> AddrSIPMap;
   typedef MIAMIU::BucketHashMap<addrtype, StrideInst*, &defaultStrideInstP> AddrSIPMap;

public:

   StrideInst*& operator() (addrtype _pc, int idx)
   {
      addrtype key = (_pc<<3) + (idx&7);
      StrideInst* &ff = storage[key];
//      if (!ff)
//         ff = new StrideInst();
      return (ff);
   }
   bool hasKey(addrtype _pc, int idx)
   {
      addrtype key = (_pc<<3) + (idx&7);
      return (storage.is_member(key));
   }

   virtual ~SIPCache()
   {
      storage.map(deleteStrideInMap, NULL);
   }
  
private:
   AddrSIPMap storage;
};


class CycleInfo
{
public:
   CycleInfo(addrtype _cycle_addr=0, int op_idx=0, int _cycle_cnt=0, 
             const register_info& _cycle_reg=MIAMI_NO_REG, int _cycle_hb=0)
          : cycle_addr(_cycle_addr), cycle_op_idx(op_idx), cycle_cnt(_cycle_cnt), 
            cycle_reg(_cycle_reg), cycle_hb(_cycle_hb)
   { 
   }
   

   addrtype cycle_addr;
   int cycle_op_idx;
   int cycle_cnt;
   register_info cycle_reg;
   int cycle_hb;       // keep track of the high_bits value for the cycle_reg
};

typedef std::multimap<unsigned long, CycleInfo, std::greater<unsigned long> > CycleMap;

#define IMARK(pc, idx)  ((*iMarkers)(pc,idx))

class StrideSlice : public BaseSlice
{
   // define a map from Nodes to Edges in which to store precomputed
   // paths that must be taken when looking for the next block to slice.
   typedef std::map<PrivateCFG::Node*, PrivateCFG::Edge*> NodeEdgeMap;

   using BaseSlice::SliceIn;
   using BaseSlice::SliceOut;
public:
   // Do I need all the addresses that I pass here? 
   // If I follow only edges and blocks marked with the correct marker,
   // I should be fine. *** FIX THIS ***
   StrideSlice(const PrivateCFG *g, RFormulasMap& _rFormulas, 
#if USE_ADDR
           addrtype _loop_start, addrtype _loop_end,
           addrtype _min_addr, addrtype _max_addr, 
#endif
           unsigned int _marker)
       : BaseSlice(g, _rFormulas)
   {
      back_edged = 0;
      start_be = 0;
      strideDFSCount = 0;
      sliceKey = 0;

#if 0  // try without override mechanism
      overrideForm2 = 0;
#endif
#if USE_ADDR
      start_addr = _loop_start;
      end_addr = _loop_end;
      min_addr = _min_addr;
      max_addr = _max_addr;
#endif
      slice_marker = _marker;
   }
   
   virtual ~StrideSlice()
   {
      // I must delete the iMarkers
      for (RCPArray::iterator rit=_values.begin() ; rit!=_values.end() ; ++rit)
         delete (*rit);
      _values.clear();
   }

   void clear()
   {
      for (RCPArray::iterator rit=_values.begin() ; rit!=_values.end() ; ++rit)
         delete (*rit);
      _values.clear();
      sCycles.clear();

      back_edged = 0;
      start_pc = 0;
      start_be = 0;
      strideDFSCount = 0;
   }

   void StartSlice(PrivateCFG::Node *b, int from_uop, const register_info& reg, 
                addrtype _start_pc, addrtype _to_pc=0);

   GFSliceVal ComputeFormulaForMemoryOperand(PrivateCFG::Node *node, const DecodedInstruction *dInst, 
             int uop_idx, int op_num, addrtype pc);


protected:
   void BuildCyclicPath(PrivateCFG::Node *b);
   bool RecursiveBuildCyclicPath(PrivateCFG::Node *startb, PrivateCFG::Node *b,
            PrivateCFG::NodeList &ba, PrivateCFG::EdgeList &ea, int back_edges, 
            unsigned int mm, const unsigned int* loopTree, unsigned int treeSize);
   unsigned int loops_LCA(const unsigned int* loopTree, unsigned int treeSize, 
            unsigned int marker1, unsigned int marker2, unsigned int marker3);
   
   virtual void Slice(PrivateCFG::Node *b, int from_uop, const register_info& reg, 
                addrtype _start_pc, addrtype _to_pc=0);

   void printRegCacheStack()
   {
      RCPArray::iterator rit = _values.end();
      int i=0;
      for( ; rit!=_values.begin() ; )
      {
         --rit;
         ++i;
         cerr << "Stack " << i << ": Reg: " << (*rit)->reg
              << ", pc=0x" << hex << (*rit)->defPc << dec 
              << ", idx=" << (*rit)->idx
              << ", target DFS=" << (*rit)->targetDFSIndex << endl;
      }
   }

   bool findTargetInStack(unsigned long _dfs)
   {
      // look only for non-zero DFS targets
      if (_dfs == 0)
         return (false);

      RCPArray::iterator rit = _values.end();
      for( ; rit!=_values.begin() ; )
      {
         --rit;
         if ((*rit)->targetDFSIndex == _dfs)
            return (true);
      }
      return (false);
   }
  
   RegCache* getValueForReg(const register_info& regarg)
   {
      RCPArray::iterator rit = _values.end();
      for( ; rit!=_values.begin() ; )
      {
         --rit;
         if ((*rit)->reg.Overlaps (regarg))
         {
            // sanity check first; what should I do if the found register does not
            // define the full bit range for requested register
            if (! (*rit)->reg.IncludesRange(regarg))
            {
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(3,
                  cerr << "ATTENTION: StrideSlice::getValueForReg with request for register "
                       << regarg << " found value for register with non-inclusive range "
                       << (*rit)->reg << ", target DFS index=" << (*rit)->targetDFSIndex << endl;
               )
#endif
//               assert(!"Non-inclusive range in StrideSlice::getValueForReg. Check this case.");
            }
            return (*rit);
         }
      }
      return NULL;
   }
  
   RegCache* getValueForRegAndDelete(const register_info& regarg)
   {
      RCPArray::iterator rit = _values.end();
      for( ; rit!=_values.begin() ; )
      {
         --rit;
         if ((*rit)->reg.Overlaps (regarg))
         {
            // sanity check first; what should I do if the found register does not
            // define the full bit range for requested register
            if (! (*rit)->reg.IncludesRange(regarg))
            {
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(3,
                  cerr << "ATTENTION: StrideSlice::getValueForRegAndDelete with request for register "
                       << regarg << " found value for register with non-inclusive range "
                       << (*rit)->reg << ", target DFS index=" << (*rit)->targetDFSIndex << endl;
               )
#endif
//               assert(!"Non-inclusive range in StrideSlice::getValueForRegAndDelete. Check this case.");
            }
            RegCache *temp = (*rit);
            _values.erase(rit);
            return (temp);
         }
      }
      return NULL;
   }
   
   RegCache* getValueForReg(addrtype pc, int idx)
   {
      RCPArray::iterator rit = _values.end();
      for( ; rit!=_values.begin() ; )
      {
         --rit;
         if ((*rit)->defPc==pc && (*rit)->idx==idx)
         {
            return (*rit);
         }
      }
      return NULL;
   }
  
   RegCache* getValueForRegAndDelete(addrtype pc, int idx)
   {
      RCPArray::iterator rit = _values.end();
      for( ; rit!=_values.begin() ; )
      {
         --rit;
         if ((*rit)->defPc==pc && (*rit)->idx==idx)
         {
            RegCache *temp = (*rit);
            _values.erase(rit);
            return (temp);
         }
      }
      return NULL;
   }
   
   GFSliceVal getStrideForReg(const register_info& reg, int high_bits=0)
   {
      RegCache* rc = getValueForRegAndDelete(reg);
      assert( rc != NULL );
      addrtype def_pc = rc->defPc;
      int idx = rc->idx;
      delete rc;
      if (def_pc == MIAMI_NO_ADDRESS)  // it is a register that is constant 
                                       // in the loop. Return 0.
         return (GFSliceVal(sliceVal(0, TY_CONSTANT)));
      else
      {
         SIPCache *iMarkers = &iMarkersLow;
//         if (high_bits)
//            iMarkers = iMarkersHi;
         StrideInst *si = (*iMarkers)(def_pc, idx);
         assert(si || !"Could not find a cached value for specified register");
        // assert(si->is_strideF != 1);//OZGURFIXME{
         if(si->is_strideF != 1)
            std::cout<<"OZGURERROR:: sii->is_strideF != 1 it is:"<<si->is_strideF<<std::endl;
         //}
         if (si->is_strideF == 0) // does not depend on an
                   // index var. Return 0, but check if it is indirect access
         {
            GFSliceVal temp(sliceVal(0, TY_CONSTANT));
            int indA = si->formula->has_indirect_access();
            if (indA==1)
               temp.set_indirect_access(GUARANTEED_INDIRECT, 0);
            else if (indA==2)
               temp.set_indirect_pcs(si->formula->get_indirect_pcs());
            if (si->formula->has_irregular_access())
               temp.set_irregular_access();
            return (temp);
         }
         else  // is_strideF must be 2. Return the computed formula.
            return (*(si->formula));
      }
   }


   // save the value of a register at a given PC
   virtual void RecordRegisterValue(const register_info& reg, const uOp_t& uop, coeff_t value);

   // Slice back until encountering a function call, a load operation, or start
   // of function;
   // another stopping condition can be a unsupported instruction;
   // in general, go back along add, sub, mov, shift, sethi, 
   // (or, and, not, xor - for constant values only);
   // When slicing back, several paths may exist. Avoid backedges in order to
   // avoid an infinite loop. Stop at the first value.
   //
   virtual bool SliceIn(PrivateCFG::Node *b, int from_uop, const register_info& reg);
   virtual void SliceOut(PrivateCFG::Node *b, int from_uop, const register_info& reg);

   virtual bool SliceIn(PrivateCFG::Edge *e, const register_info& reg);
   virtual void SliceOut(PrivateCFG::Edge *e, const register_info& reg);
   
   // called at the beginning of SliceNext, check if we reached a stopping condition
   virtual bool SliceNextIn(PrivateCFG::Node *b, const register_info& reg);
   virtual void SliceNextOut(PrivateCFG::Node *b, const register_info& reg);

   int GetFormulaInfoForOperand(const DecodedInstruction *dinst, const instruction_info *ii, 
              int op_num, GFSliceVal& formula, int& strideF, 
              addrtype& defPc, unsigned long& dfsTarget);

   SIPCache iMarkersLow;
//   SIPCache iMarkersHi;

   static unsigned long strideDFSCount;
   static unsigned long sliceKey;
   
#if USE_ADDR
   addrtype start_addr;
   addrtype end_addr;
   addrtype min_addr;
   addrtype max_addr;
#endif
   unsigned int slice_marker;
   int start_rank;
   int start_be;   // start_back_edged
   int back_edged;

#if 0  // try without override
   addr overrideForm2;
   UIList overridedPCs;  // list of instructions whose formula was overrided
#endif
   // incoming Edges to take for a given block
   NodeEdgeMap incomingEdges;

   // Value of slice.
   //
   RCPArray _values;
   
   // Map of cycles in progress
   //
   CycleMap sCycles;  // stride cycles
};


}  /* namespace MIAMI */

#endif
