/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: static_branch_analysis.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Computes generic formulas for branch targets. Attempts to 
 * resolve targets of indirect branches using data flow analysis.
 */

#ifndef _STATIC_BRANCH_ANALYSIS_H
#define _STATIC_BRANCH_ANALYSIS_H


#include "miami_types.h"
#include "instr_info.H"
#include "slice_references.h"
#include "PrivateCFG.h"


//using namespace std;
namespace MIAMI  // add everything to namespace MIAMI
{

/* Compute a branch target formula. The method evaluates only hardwired registers
 * (like the IP register) and constant values, of course. For indirect jumps, it will
 * leave the target registers unresolved. The second method, below, tries to slice
 * on the target registers to resolve the target of most indirect jumps.
 */
int
ComputeBranchTargetFormula(const DecodedInstruction *dInst, const instruction_info *ii,
                int uop_idx, addrtype pc, GFSliceVal& targetF);

/* Compute a branch target formula. For indirect jumps, resolve the target registers
 * using data flow analysis.
 */
int
ResolveBranchTargetFormula(const GFSliceVal& targetF, PrivateCFG *cfg, 
             PrivateCFG::Node *brB, addrtype lpc, InstrBin brType, 
             AddrSet& targets);

}  /* namespace MIAMI */

#endif
