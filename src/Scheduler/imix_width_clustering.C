/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: imix_width_clustering.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements clustering of instructions based on their operation types and
 * data processing width. For SIMD operations, the width is computed as the
 * product between the number of elements and width of an element.
 */

#include <assert.h>
#include <stdio.h>
#include "imix_width_clustering.h"

namespace MIAMI
{

const char* ImixWTypeToString(ImixWType it)
{
   switch (it)
   {
      case IMIXW_INVALID:     return ("Invalid");
      case IMIXW_Load:        return ("Load");
      case IMIXW_Store:       return ("Store");
      case IMIXW_IntOp:       return ("IntOp");
      case IMIXW_FpOp:        return ("FpOp");
      case IMIXW_Move:        return ("Move");
      case IMIXW_Branch:      return ("Branch");
      case IMIXW_Misc:        return ("Misc");
      case IMIXW_Nop:         return ("NOP");
      case IMIXW_Prefetch:    return ("Prefetch");
      default: assert(! "Unknown ImixWType");
   }
   return("notReachable");
}

const char* IWidthTypeToString(IWidthType it)
{
   switch (it)
   {
      case IWIDTH_INVALID:    return ("Invalid");
      case IWIDTH_32:         return ("032");
      case IWIDTH_64:         return ("064");
      case IWIDTH_128:        return ("128");
      case IWIDTH_256:        return ("256");
      default: assert(! "Unknown IWidthType");
   }
   return("notReachable");
}

int NumImixWidthBins()
{
   return ((IMIXW_WIDTH_LAST-IMIXW_WIDTH_FIRST+1)*(IWIDTH_LAST-IWIDTH_INVALID-2) \
             + IMIXW_LAST-IMIXW_INVALID);
}

ImixWType ImixWidthBinToImixWidthType(int b, IWidthType &width)
{
   int itype;
   if (b < IMIXW_WIDTH_FIRST) {
      width = IWIDTH_INVALID;
      itype = b;
   } else if (b < ((IMIXW_WIDTH_LAST-IMIXW_WIDTH_FIRST+1) * (IWIDTH_LAST-IWIDTH_INVALID-1)
                       + IMIXW_WIDTH_FIRST))
   {
      itype = b - IMIXW_WIDTH_FIRST;
      width = (IWidthType)(itype % (IWIDTH_LAST-IWIDTH_INVALID-1) + IWIDTH_INVALID + 1);
      itype = itype / (IWIDTH_LAST-IWIDTH_INVALID-1) + IMIXW_WIDTH_FIRST;
   } else  // b > ...
   {
      width = IWIDTH_INVALID;
      itype = b - ((IMIXW_WIDTH_LAST-IMIXW_WIDTH_FIRST+1) * (IWIDTH_LAST-IWIDTH_INVALID-2));
   }
   if (itype<=IMIXW_INVALID || itype>=IMIXW_LAST)
      return (IMIXW_INVALID);
   else
      return ((ImixWType)itype);
}

ImixWType 
IClassToImixWType(const InstructionClass& iclass, int& count)
{
   count = 1;
   switch (iclass.type)
   {
      case IB_INVALID:
         return (IMIXW_INVALID);
         
      case IB_load:
      case IB_load_store:
         return (IMIXW_Load);
 
      case IB_store:
         return (IMIXW_Store);
         
      case IB_branch:
      case IB_br_CC:
      case IB_jump:
      case IB_ret:
         return (IMIXW_Branch);
         
      case IB_cvt:
      case IB_cvt_prec:
      case IB_copy:
      case IB_move:
      case IB_move_cc:
      case IB_shuffle:
      case IB_blend:
      case IB_insert:
      case IB_extract:
         return (IMIXW_Move);
         
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
         if (iclass.eu_type==ExecUnitType_FP)
            return (IMIXW_FpOp);
         else
            return (IMIXW_IntOp);

      case IB_nop:
         return (IMIXW_Nop);
         
      case IB_prefetch:
         return (IMIXW_Prefetch);
      
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
         return (IMIXW_Misc);
      
      case IB_inner_loop:
      case IB_unknown:
         return (IMIXW_INVALID);
      
      default:                assert(!"unknown Instruction Bin");
   }
   return (IMIXW_INVALID);
}

IWidthType
IClassToIWidthType(const InstructionClass& iclass)
{
   int width = iclass.width;
   if (iclass.eu_style == ExecUnit_VECTOR)
      width = iclass.vec_width;
   if (width <= 32)
      return (IWIDTH_32);
   else if (width <= 64)
      return (IWIDTH_64);
   else if (width <= 128)
      return (IWIDTH_128);
   else if (width <= 256)
      return (IWIDTH_256);
   else 
   {
      fprintf(stderr, "Unexpected InstructionClass of type %s with width of %d larger than 256\n", 
             Convert_InstrBin_to_string(iclass.type), width);
      assert(! "Bad instruction width??");
   }
   return (IWIDTH_INVALID);
}

int 
IClassToImixWidthBin(const InstructionClass& iclass, int& count)
{
   ImixWType itype = IClassToImixWType(iclass, count);
   if (itype<IMIXW_WIDTH_FIRST)
      return ((int)itype);
   else if (itype <= IMIXW_WIDTH_LAST)
   {
      IWidthType wtype = IClassToIWidthType(iclass);
      return ((itype-IMIXW_WIDTH_FIRST)*(IWIDTH_LAST-IWIDTH_INVALID-1) +
                 (wtype-IWIDTH_INVALID-1) + IMIXW_WIDTH_FIRST);
   } else
   {
      return (itype + ((IMIXW_WIDTH_LAST-IMIXW_WIDTH_FIRST+1) * (IWIDTH_LAST-IWIDTH_INVALID-2)));
   }
}


}  /* namespace MIAMI */
