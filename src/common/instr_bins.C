/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: instr_bins.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Defines the generic MIAMI micro-operation types.
 */

#include <assert.h>
#include "instr_bins.H"

namespace MIAMI
{

unsigned int defaultIB = IB_unknown;

const char*    
Convert_InstrBin_to_string(InstrBin ibin)
{
   switch (ibin) 
   {
      case IB_dg_entry:       return("DG_Entry");
      case IB_dg_exit:        return("DG_Exit");
      case IB_INVALID:        return("Invalid");
      
      case IB_load:           return("Load");
      case IB_load_store:     return("LoadStore");
      case IB_mem_fence:      return("MemoryFence");
      case IB_privl_op:       return("PrivilegedOp");
      case IB_branch:         return("UncondBranch");
      case IB_br_CC:          return("CondBranch");
      case IB_jump:           return("Jump");
      case IB_ret:            return("Return");
      case IB_cvt:            return("Convert");
      case IB_cvt_prec:       return("PrecisionConvert");
      case IB_copy:           return("Copy");
      case IB_move:           return("Move");
      case IB_move_cc:        return("MoveCC");
      case IB_shuffle:        return("Shuffle");
      case IB_blend:          return("Blend");
      case IB_misc:           return("Misc");
      case IB_cmp:            return("Cmp");
      case IB_add:            return("Add");
      case IB_lea:            return("LEA");
      case IB_add_cc:         return("AddCC");
      case IB_sub:            return("Sub");
      case IB_mult:           return("Mult");
      case IB_div:            return("Div");
      case IB_sqrt:           return("Sqrt");
      case IB_madd:           return("MultAdd");
      case IB_xor:            return("Xor");
      case IB_logical:        return("LogicalOp");
      case IB_shift:          return("Shift");
      case IB_nop:            return("NOP");
      case IB_prefetch:       return("Prefetch");
      case IB_trap:           return("Trap");
      case IB_store:          return("Store");
      case IB_inner_loop:     return("InnerLoop");
      case IB_intrinsic:      return("IntrinsicCall");

      case IB_load_conf:      return("LoadState");
      case IB_store_conf:     return("StoreState");
      case IB_popcnt:         return("PopCount");
      case IB_port_rd:        return("PortRead");
      case IB_port_wr:        return("PortWrite");
      case IB_reciprocal:     return("Reciprocal");
      case IB_insert:         return("Insert");
      case IB_extract:        return("Extract");

      case IB_xchg:           return("Xchg");
      case IB_cmp_xchg:       return("CmpXchg");
      case IB_rotate:         return("Rotate");
      case IB_rotate_cc:      return("RotateCC");
      
      case IB_fn:             return("MathFunc");
      case IB_unknown:        return("UnknownOp");
      default:                assert(!"unknown Instruction Bin");
   };
   return("notReachable");
}


int
InstrBinIsMemoryType(InstrBin type)
{
   switch (type) 
   {
      case IB_load:
      case IB_store:
      case IB_load_conf:
      case IB_store_conf:
      case IB_prefetch:
      case IB_load_store:
         return (1);
        
      case IB_mem_fence:
      case IB_privl_op:
      case IB_branch:
      case IB_br_CC:
      case IB_jump:
      case IB_ret:
      case IB_cvt:
      case IB_cvt_prec:
      case IB_copy:
      case IB_move:
      case IB_move_cc:
      case IB_shuffle:
      case IB_blend:
      case IB_misc:
      case IB_cmp:
      case IB_add:
      case IB_lea:
      case IB_mult:
      case IB_div:
      case IB_sqrt:
      case IB_madd:
      case IB_logical:
      case IB_shift:
      case IB_nop:
      case IB_trap:
      case IB_inner_loop:
      case IB_intrinsic:
      case IB_add_cc:
      case IB_xor:
      case IB_sub:
      case IB_popcnt:
      case IB_port_rd:
      case IB_port_wr:
      case IB_reciprocal:
      case IB_insert:
      case IB_extract:
      case IB_xchg:
      case IB_cmp_xchg:
      case IB_rotate:
      case IB_rotate_cc:
      case IB_fn:
      case IB_unknown:
         return (0);
      default:               assert(!"unknown Instruction Bin");
   };
   return (0);
}

int
InstrBinIsLoadType(InstrBin type)
{
   switch (type) 
   {
      case IB_load:
      case IB_load_conf:
      case IB_prefetch:
      case IB_load_store:
         return (1);
        
      case IB_store:
      case IB_store_conf:
      case IB_mem_fence:
      case IB_privl_op:
      case IB_branch:
      case IB_br_CC:
      case IB_jump:
      case IB_ret:
      case IB_cvt:
      case IB_cvt_prec:
      case IB_copy:
      case IB_move:
      case IB_move_cc:
      case IB_shuffle:
      case IB_blend:
      case IB_misc:
      case IB_cmp:
      case IB_add:
      case IB_lea:
      case IB_mult:
      case IB_div:
      case IB_sqrt:
      case IB_madd:
      case IB_logical:
      case IB_shift:
      case IB_nop:
      case IB_trap:
      case IB_inner_loop:
      case IB_intrinsic:
      case IB_add_cc:
      case IB_xor:
      case IB_sub:
      case IB_popcnt:
      case IB_port_rd:
      case IB_port_wr:
      case IB_reciprocal:
      case IB_insert:
      case IB_extract:
      case IB_xchg:
      case IB_cmp_xchg:
      case IB_rotate:
      case IB_rotate_cc:
      case IB_fn:
      case IB_unknown:
         return (0);
      default:                assert(!"unknown Instruction Bin");
   };
   return (0);
}

int
InstrBinIsStoreType(InstrBin type)
{
   switch (type) 
   {
      case IB_store:
      case IB_store_conf:
      case IB_load_store:
         return (1);
        
      case IB_load:
      case IB_load_conf:
      case IB_prefetch:
      case IB_mem_fence:
      case IB_privl_op:
      case IB_branch:
      case IB_br_CC:
      case IB_jump:
      case IB_ret:
      case IB_cvt:
      case IB_cvt_prec:
      case IB_copy:
      case IB_move:
      case IB_move_cc:
      case IB_shuffle:
      case IB_blend:
      case IB_misc:
      case IB_cmp:
      case IB_add:
      case IB_lea:
      case IB_mult:
      case IB_div:
      case IB_sqrt:
      case IB_madd:
      case IB_logical:
      case IB_shift:
      case IB_nop:
      case IB_trap:
      case IB_inner_loop:
      case IB_intrinsic:
      case IB_add_cc:
      case IB_xor:
      case IB_sub:
      case IB_popcnt:
      case IB_port_rd:
      case IB_port_wr:
      case IB_reciprocal:
      case IB_insert:
      case IB_extract:
      case IB_xchg:
      case IB_cmp_xchg:
      case IB_rotate:
      case IB_rotate_cc:
      case IB_fn:
      case IB_unknown:
         return (0);
      default:                assert(!"unknown Instruction Bin");
   };
   return (0);
}

int
InstrBinIsBranchType(InstrBin type)
{
   switch (type) 
   {
      case IB_branch:
      case IB_br_CC:
      case IB_jump:
      case IB_ret:
         return (1);
        
      case IB_load:
      case IB_store:
      case IB_load_conf:
      case IB_store_conf:
      case IB_prefetch:
      case IB_load_store:
      case IB_mem_fence:
      case IB_privl_op:
      case IB_cvt:
      case IB_cvt_prec:
      case IB_copy:
      case IB_move:
      case IB_move_cc:
      case IB_shuffle:
      case IB_blend:
      case IB_misc:
      case IB_cmp:
      case IB_add:
      case IB_lea:
      case IB_mult:
      case IB_div:
      case IB_sqrt:
      case IB_madd:
      case IB_logical:
      case IB_shift:
      case IB_nop:
      case IB_trap:
      case IB_inner_loop:
      case IB_intrinsic:
      case IB_add_cc:
      case IB_xor:
      case IB_sub:
      case IB_popcnt:
      case IB_port_rd:
      case IB_port_wr:
      case IB_reciprocal:
      case IB_insert:
      case IB_extract:
      case IB_xchg:
      case IB_cmp_xchg:
      case IB_rotate:
      case IB_rotate_cc:
      case IB_fn:
      case IB_unknown:
         return (0);
      default:                assert(!"unknown Instruction Bin");
   };
   return (0);
}

int 
InstrBinIsRegisterMove(InstrBin type)
{
   switch (type) 
   {
      case IB_cvt:
      case IB_cvt_prec:
      case IB_copy:
      case IB_move:
      case IB_move_cc:
      case IB_shuffle:
      case IB_blend:
      case IB_insert:
      case IB_extract:
         return (1);
        
      case IB_load:
      case IB_store:
      case IB_load_conf:
      case IB_store_conf:
      case IB_branch:
      case IB_br_CC:
      case IB_jump:
      case IB_ret:
      case IB_prefetch:
      case IB_load_store:
      case IB_mem_fence:
      case IB_privl_op:
      case IB_misc:
      case IB_cmp:
      case IB_add:
      case IB_lea:
      case IB_mult:
      case IB_div:
      case IB_sqrt:
      case IB_madd:
      case IB_logical:
      case IB_shift:
      case IB_nop:
      case IB_trap:
      case IB_inner_loop:
      case IB_intrinsic:
      case IB_add_cc:
      case IB_xor:
      case IB_sub:
      case IB_popcnt:
      case IB_port_rd:
      case IB_port_wr:
      case IB_reciprocal:
      case IB_xchg:
      case IB_cmp_xchg:
      case IB_rotate:
      case IB_rotate_cc:
      case IB_fn:
      case IB_unknown:
         return (0);
      default:                assert(!"unknown Instruction Bin");
   };
   return (0);
}


}
