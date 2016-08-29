/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: canonical_ops.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Define a set of canonical operation types.
 */

#ifndef _CANONICAL_OPS_H
#define _CANONICAL_OPS_H

namespace MIAMI
{

// Operations that an instruction can perform.
//
enum CanonicalOp
{
  OP_INVALID = 0,
  OP_CALL,  // subroutine call
  OP_PC_SET_CALL, // pc setting instruction for PIC code
  OP_ICALL, // subroutine call, (register) indirect
  OP_RET,   // subroutine return 

  OP_BR,    // branch
  OP_BR_CC, // branch on condition codes
  OP_JMP,   // jump
  OP_TRAP,  // trap

  OP_LIMM,  // load immediate
  OP_CMP,   // comparison (either primary or secondary (e.g. condition code))
  OP_ADD,   // add
  OP_AND,   // and
  OP_ANDN,  // andn
  OP_MUL,   // multiply
  OP_UMUL,  // multiply, unsigned
  OP_DIV,   // divide
  OP_UDIV,  // divide, unsigned
  OP_OR,    // inclusive-or
  OP_ORN,   // inclusive-orn
  OP_SLL,   // shift left logical
  OP_SRL,   // shift right logical
  OP_SRA,   // shift right arithmetic
  OP_SUB,   // subtract
  OP_XNOR,  // exclusive-nor
  OP_XOR,   // exclusive-or 
  OP_NOT,   // NOT
  OP_MOV,   // move or register copy
  OP_LEA,   // compute effective address LEA

  OP_FMOV,  // FP move
  OP_FNEG,  // FP negate
  OP_FABS,  // FP absolute value
  OP_FSQRT, // FP square root   
  OP_FADD,  // FP add
  OP_FSUB,  // FP subtract
  OP_FMUL,  // FP multiply
  OP_FDIV,  // FP divide  
  OP_FCMP,  // FP compare 
  OP_FCNVT, // FP 'type-conversion'

  OP_LD,    // load
  OP_ST,    // store
  OP_LDST,  // load-store
  OP_SWAP,  // swap register and memory

  OP_PREFETCH, // prefetch instruction
  OP_MISC,
  OP_LAST
};

const char*    Convert_CanonicalOp_to_string(CanonicalOp cop);

}  /* namespace MIAMI */

#endif
