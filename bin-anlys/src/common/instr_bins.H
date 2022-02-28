/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: instr_bins.H
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Defines the generic MIAMI micro-operation types.
 */

#ifndef INSTR_BINS_H
#define INSTR_BINS_H

namespace MIAMI
{

// some sections of code used the order in which the instruction types are 
// defined.
// In general try to group together instructions from the same family.
// Memory instructions are grouped at the beginning (yes, I use this for now);
// float operations are grouped between fp_add and fp_madd (conversion and
// register copy are not considered "real" float instructions)
enum InstrBin
{
 IB_dg_exit = -2,
 IB_dg_entry = -1,
 IB_INVALID = 0,
 
 IB_inner_loop,

 IB_load,
 IB_store,
 IB_load_store,
 IB_mem_fence,

 IB_privl_op,
 IB_branch,
 IB_br_CC,
 IB_jump,   /* this should be used only for inter-procedural jump: call,
               jmpl to a different procedure */
 IB_ret,    /* for returns */
 IB_cvt,
 IB_cvt_prec,
 IB_copy,
 IB_move,
 IB_move_cc,
 IB_shuffle,
 IB_blend,
 IB_misc,  /* 18 */

 IB_cmp,
 IB_add,
 IB_lea,
 IB_add_cc,
 IB_sub,
 IB_mult,
 IB_div,
 IB_sqrt,
 IB_madd,   /* 27 */

 IB_xor,
 IB_logical,
 IB_shift,
 IB_nop,
 IB_prefetch,

 IB_trap,

 IB_load_conf,
 IB_store_conf,  /* 35 */
 IB_fn,

 IB_getf,
 IB_setf,
 IB_popcnt,
 IB_port_rd,
 IB_port_wr,
 IB_reciprocal, /* 42 */
 IB_insert,
 IB_extract,
 
 IB_xchg,
 IB_cmp_xchg,
 IB_rotate,
 IB_rotate_cc,  /* 48 */
  
 // have some special instruction for now to use for calls to fortran 
 // intrinsics
 IB_intrinsic,
 IB_unknown,

 IB_TOP_MARKER  // this must be always on the last position
};

const char*    Convert_InstrBin_to_string(InstrBin ibin);

inline int InstrBinIsNOP(InstrBin type)
{
   return (type == IB_nop);
}

inline int InstrBinIsPrefetch(InstrBin type)
{
   return (type == IB_prefetch);
}

// this function was initially named "IsMemoryInstruction"
int InstrBinIsMemoryType(InstrBin type);

int InstrBinIsLoadType(InstrBin type);
int InstrBinIsStoreType(InstrBin type);

int InstrBinIsRegisterMove(InstrBin type);

int InstrBinIsBranchType(InstrBin type);

} /* namespace MIAMI */

#endif
