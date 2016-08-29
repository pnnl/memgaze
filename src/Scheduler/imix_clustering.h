/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: imix_clustering.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines a set of instruction categories for computing simple instruction 
 * mixes based on static instruction decoding.
 */

#ifndef _IMIX_CLUSTERING_H
#define _IMIX_CLUSTERING_H

#include "miami_types.h"
#include "instruction_class.h"

namespace MIAMI 
{

enum ImixType {
   IMIX_INVALID,
   IMIX_Mem, 
   IMIX_MemVec,
   IMIX_IntArithm,
   IMIX_IntArithmVec,
   IMIX_FpArithm, 
   IMIX_FpArithmVec,
   IMIX_Move,
   IMIX_Branch, 
   IMIX_Misc,
   IMIX_Nop,
   IMIX_Prefetch,
   IMIX_LAST
};

extern const char* ImixTypeToString(ImixType it);
extern ImixType IClassToImixType(const InstructionClass& iclass, int& count);

}  /* namespace MIAMI */

#endif
