/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: canonical_ops.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Define a set of canonical operation types.
 */

#include "canonical_ops.h"
#include <assert.h>

namespace MIAMI
{

const char*    
Convert_CanonicalOp_to_string(CanonicalOp cop)
{
   switch (cop) 
   {
      case OP_INVALID:        return("OpInvalid");
      case OP_CALL:           return("OpCall");
      case OP_PC_SET_CALL:    return("OpPcSetCall");
      case OP_ICALL:          return("OpICall");
      case OP_RET:            return("OpRet");
      case OP_BR:             return("OpBr");
      case OP_BR_CC:          return("OpBrCC");
      case OP_JMP:            return("OpJmp");
      case OP_TRAP:           return("OpTrap");
      case OP_LIMM:           return("OpLoadImm");
      case OP_CMP:            return("OpCmp");
      case OP_ADD:            return("OpAdd");
      case OP_AND:            return("OpAND");
      case OP_ANDN:           return("OpANDN");
      case OP_MUL:            return("OpMul");
      case OP_UMUL:           return("OpUMul");
      case OP_DIV:            return("OpDiv");
      case OP_UDIV:           return("OpUDiv");
      case OP_OR:             return("OpOR");
      case OP_ORN:            return("OpORN");
      case OP_SLL:            return("OpSLL");
      case OP_SRL:            return("OpSRL");
      case OP_SRA:            return("OpSRA");
      case OP_SUB:            return("OpSub");
      case OP_XNOR:           return("OpXNOR");
      case OP_XOR:            return("OpXOR");
      case OP_NOT:            return("OpNOT");
      case OP_MOV:            return("OpMov");
      case OP_LEA:            return("OpLEA");
      case OP_FMOV:           return("OpFMov");
      case OP_FNEG:           return("OpFNeg");
      case OP_FABS:           return("OpFAbs");
      case OP_FSQRT:          return("OpFSqrt");
      case OP_FADD:           return("OpFAdd");
      case OP_FSUB:           return("OpFSub");
      case OP_FMUL:           return("OpFMul");
      case OP_FDIV:           return("OpFDiv");
      case OP_FCMP:           return("OpFCmp");
      case OP_FCNVT:          return("OpFCnvt");
      case OP_LD:             return("OpLoad");
      case OP_ST:             return("OpStore");
      case OP_LDST:           return("OpLoadStore");
      case OP_SWAP:           return("OpSwap");
      case OP_PREFETCH:       return("OpPrefetch");
      case OP_MISC:           return("OpMisc");
      case OP_LAST:           return("OpLAST");
      default:                assert(!"unknown Canonical Op");
   };
   return("notReachable");
}

}  /* namespace MIAMI */
