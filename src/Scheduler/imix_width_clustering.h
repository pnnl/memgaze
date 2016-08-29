/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: imix_width_clustering.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines a set of instruction categories for computing instruction mixes
 * based on operation type and data width.
 */

#ifndef _IMIX_WIDTH_CLUSTERING_H
#define _IMIX_WIDTH_CLUSTERING_H

#include "miami_types.h"
#include "instruction_class.h"

namespace MIAMI 
{

/* Define a basic set of micro-operations. We will cluster a program's 
 * micro-ops into these categories. Moreover, for the types between
 * IMIXW_WIDTH_FIRST and IMIXW_WIDTH_LAST, we will cluster separately
 * operations based on their result's bit length: <=32, 64, 128, 256.
 */
enum ImixWType {
   IMIXW_INVALID = 0,
   IMIXW_Load, 
   IMIXW_Store,
   IMIXW_IntOp,
   IMIXW_FpOp,
   IMIXW_Move,
   IMIXW_Branch, 
   IMIXW_Misc,
   IMIXW_Nop,
   IMIXW_Prefetch,
   IMIXW_LAST,
   IMIXW_WIDTH_FIRST = IMIXW_Load,
   IMIXW_WIDTH_LAST = IMIXW_Move
};

enum IWidthType {
   IWIDTH_INVALID = 0,
   IWIDTH_32,
   IWIDTH_64,
   IWIDTH_128,
   IWIDTH_256,
   IWIDTH_LAST
};

extern const char* ImixWTypeToString(ImixWType it);
extern const char* IWidthTypeToString(IWidthType it);

extern ImixWType IClassToImixWType(const InstructionClass& iclass, int& count);
extern IWidthType IClassToIWidthType(const InstructionClass& iclass);

#define NUM_IMIX_WIDTH_BINS  ((IMIXW_WIDTH_LAST-IMIXW_WIDTH_FIRST+1)*(IWIDTH_LAST-IWIDTH_INVALID-2) \
 + IMIXW_LAST-IMIXW_INVALID)

extern int NumImixWidthBins();
extern int IClassToImixWidthBin(const InstructionClass& iclass, int& count);
extern ImixWType ImixWidthBinToImixWidthType(int b, IWidthType &width);

}  /* namespace MIAMI */

#endif
