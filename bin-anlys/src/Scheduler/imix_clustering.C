/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: imix_clustering.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements clustering of instructions based on their operation types and
 * if they are vector or scalar (for a few categories).
 */

#include <assert.h>
#include "imix_clustering.h"

namespace MIAMI
{

const char* ImixTypeToString(ImixType it)
{
   switch (it)
   {
      case IMIX_INVALID:      return ("Invalid");
      case IMIX_Mem:          return ("MemOps");
      case IMIX_MemVec:       return ("MemSIMD");
      case IMIX_IntArithm:    return ("IntOps");
      case IMIX_IntArithmVec: return ("IntSIMD");
      case IMIX_FpArithm:     return ("FpOps");
      case IMIX_FpArithmVec:  return ("FpSIMD");
      case IMIX_Move:         return ("Moves");
      case IMIX_Branch:       return ("BrOps");
      case IMIX_Misc:         return ("Misc");
      case IMIX_Nop:          return ("NOP");
      case IMIX_Prefetch:     return ("Prefetch");
      default: assert(! "Unknown ImixType");
   }
   return("notReachable");
}

ImixType 
IClassToImixType(const InstructionClass& iclass, int& count)
{
   count = 1;
   switch (iclass.type)
   {
      case IB_INVALID:
         return (IMIX_INVALID);
         
      case IB_load:
      case IB_store:
      case IB_load_store:
         if (iclass.eu_style==ExecUnit_VECTOR && iclass.vec_width>iclass.width)
            return (IMIX_MemVec);
         else
            return (IMIX_Mem);
         
      case IB_branch:
      case IB_br_CC:
      case IB_jump:
      case IB_ret:
         return (IMIX_Branch);
         
      case IB_cvt:
      case IB_cvt_prec:
      case IB_copy:
      case IB_move:
      case IB_move_cc:
      case IB_shuffle:
      case IB_blend:
      case IB_insert:
      case IB_extract:
         return (IMIX_Move);
         
      case IB_cmp:
      case IB_add:
      case IB_lea:
      case IB_add_cc:
      case IB_sub:
      case IB_mult:
      case IB_div:
      case IB_sqrt:
      case IB_madd:
      case IB_xor:
      case IB_logical:
      case IB_shift:
      case IB_reciprocal:
      case IB_rotate:
      case IB_rotate_cc:
      case IB_fn:
         if (iclass.eu_style==ExecUnit_VECTOR && iclass.vec_width>iclass.width)
         {
            if (iclass.eu_type==ExecUnitType_FP)
               return (IMIX_FpArithmVec);
            else
               return (IMIX_IntArithmVec);
         } else
         {
            if (iclass.eu_type==ExecUnitType_FP)
               return (IMIX_FpArithm);
            else
               return (IMIX_IntArithm);
         }

      case IB_nop:
         return (IMIX_Nop);
         
      case IB_prefetch:
         return (IMIX_Prefetch);
      
      case IB_trap:
      case IB_intrinsic:
      case IB_misc:
      case IB_mem_fence:
      case IB_privl_op:
      case IB_load_conf:
      case IB_store_conf:
      case IB_popcnt:
      case IB_port_rd:
      case IB_port_wr:
      case IB_xchg:
      case IB_cmp_xchg:
         return (IMIX_Misc);
      
      case IB_inner_loop:
      case IB_unknown:
         return (IMIX_INVALID);
      
      default:                assert(!"unknown Instruction Bin");
   }
   return (IMIX_INVALID);
}

}  /* namespace MIAMI */
