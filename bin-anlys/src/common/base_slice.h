/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: base_slice.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements support for backwards slicing on a register name 
 * through a CFG. Enables various data flow analyses to be built on top of 
 * it.
 */

#ifndef _BASE_SLICE_H
#define _BASE_SLICE_H


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define HAVE_MEMORY_FORMULAS  1

#if HAVE_MEMORY_FORMULAS
#include "slice_references.h"
#endif

#include "miami_types.h"
#include "PrivateCFG.h"
#include "register_class.h"

namespace MIAMI
{

enum SliceEffect_t {
   SliceEffect_EASY        // output can be computed directly
   ,SliceEffect_NORMAL     // depends on input registers
   ,SliceEffect_HARD       // depends on loaded value, must follow save
   ,SliceEffect_IMPOSSIBLE // cannot compute, depends on result of function call, etc.
   ,SliceEffect_LAST
};
   
// Implements the basic functionality for a backwards register slice.
// It should define methods to go slice the next block, check if a CFG edge
// should be followed, if a block should be sliced, etc.
// Derived classes should overwrite the medthods that choose which edge to 
// follow, or if a block should be sliced. They should also overwrite
// the slice_in and slice_out methods that compute the result of the slice.
class BaseSlice
{
public:
#if  HAVE_MEMORY_FORMULAS
   BaseSlice(const PrivateCFG *_cfg, RFormulasMap& _rf) : refFormulas(_rf)
   {
      cfg = _cfg;
   }
#else
   BaseSlice(const PrivateCFG *_cfg) {
      cfg = _cfg;
   }
#endif
   
   virtual void Slice(PrivateCFG::Node *b, int from_uop, const register_info& reg, 
                addrtype _start_pc, addrtype _to_pc=0);
   
   SliceEffect_t SliceEffect(const PrivateCFG::Node *b, const uOp_t& uop, const register_info& reg);
protected:
   virtual ~BaseSlice() {}
   
   virtual bool SliceIn(PrivateCFG::Node *b, int from_uop, const register_info& reg)
   {
      return (true);
   }
   virtual void SliceOut(PrivateCFG::Node *b, int from_uop, const register_info& reg)
   {
   }
   
   virtual bool SliceIn(PrivateCFG::Node *b, const register_info& reg)
   {
      return (true);
   }
   virtual void SliceOut(PrivateCFG::Node *b, const register_info& reg)
   {
   }

   virtual bool SliceIn(PrivateCFG::Edge *e, const register_info& reg)
   {
      return (true);
   }
   virtual void SliceOut(PrivateCFG::Edge *e, const register_info& reg)
   {
   }
   // This method is called from the same places as SliceIn(e,reg) and
   // SliceOut(e,reg), so a derived class can overwrite them to implement 
   // different behavior like trying 1, 2, N, or all the incoming edges.
   virtual bool SliceMultiplePaths(PrivateCFG::Edge *e, const register_info& reg)
   {
      return (false);   // by default stop after first taken edge
   }
   
   // called at the beginning of SliceNext, check if we reached a stopping condition
   virtual bool SliceNextIn(PrivateCFG::Node *b, const register_info& reg)
   {
      return (true);
   }
   virtual void SliceNextOut(PrivateCFG::Node *b, const register_info& reg)
   {
   }
   virtual void SliceNext(PrivateCFG::Node *b, const register_info& reg);
   
   virtual bool ShouldSliceUop(const uOp_t& uop, const register_info& reg, register_info& areg)
   {
      return (uop.Writes(reg, false, areg) &&
           ! mach_inst_performs_conditional_move(uop.dinst, uop.iinfo));
   }
   virtual bool UopTerminatesSlice(const uOp_t& uop, int opidx, const register_info& reg, 
                   register_info& areg)
   {
      /* Ideally, I should keep track of the bit ranges written for each register.
       * If only some of the fields are written by an instruction, I must keep
       * slicing. While I implemented this part, I need to also keep track of
       * the actual bit ranges and have a way of combining the values of different,
       * overlapped writes. For now, terminate even at a partial Write.
       */
      return (uop.Writes(reg, /*true*/ false, areg) &&
           ! mach_inst_performs_conditional_move(uop.dinst, uop.iinfo));
   }

   virtual bool ShouldSliceRegister(const register_info& reg, addrtype pc)
   {
      return (!is_flag_or_status_register(reg));
   }

   virtual void RecordRegisterValue(const register_info& reg, const uOp_t& uop, coeff_t value)
   {
       // do nothing; derived classes must overwrite this method
   }
   
#if HAVE_MEMORY_FORMULAS
   virtual bool SliceFollowSave(PrivateCFG::Node *b, int from_uop, const GFSliceVal& _formula,
                register_info& spilled_reg);
#endif
         

protected:
   const PrivateCFG *cfg;
   addrtype start_pc;
   addrtype to_pc;
#if  HAVE_MEMORY_FORMULAS
   RFormulasMap& refFormulas;
#endif
};

}  /* namespace MIAMI */

#endif
