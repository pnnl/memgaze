/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: instruction_decoding.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A generic interface for accessing architectural specific
 * information about instructions, operands, registers, branch targets, etc.
 */

#ifndef _INSTRUCTION_DECODING_H
#define _INSTRUCTION_DECODING_H

#include "miami_types.h"
#include "instr_info.H"
#include "register_class.h"
#include "canonical_ops.h"
#include <list>

#define WITH_REFERENCE_SLICING 1

#if WITH_REFERENCE_SLICING
# include "slice_references.h"
#endif

namespace MIAMI
{


typedef std::list<RegName> RegNameList;

void instruction_decoding_init(void *arg0);

/* I need an architecture independent instruction interator interface.
 * Note that instructions may have variable length (e.g. x86)
 */
int length_instruction_at_pc(void *pc, int len);
int decode_instruction_at_pc(void *pc, int len, DecodedInstruction *dInst);
int debug_decode_instruction_at_pc(void *pc, int len);

/* Check if the instruction at the given memory location is a control
 * transfer instruction (terminating a basic block), and return also
 * the length in bytes of the encoded machine instruction.
 */
int instruction_at_pc_transfers_control(void *pc, int len, int& ilen);

/* Check if the given instruction can execute multiple times.
 * This is the case with REP prefixed instructions on x86.
 */
int mach_inst_stutters(const DecodedInstruction *dInst);

/* Check if the instruction at the given pc can execute multiple times.
 * This is the case with REP prefixed instructions on x86.
 */
int instruction_at_pc_stutters(void *pc, int len, int& ilen);

/* Return the canonical operation performed by this micro-op.
 * Support only a limited number of micro-ops for starter. Add more as deemed
 * necessary for register slicing.
 */
CanonicalOp uop_canonical_decode(const DecodedInstruction *dInst, const instruction_info *ii);

/* return unaliased register name 
 */
RegName register_name_actual(RegName reg);

/* return the possible return registers for the given mico-op.
 * The micro-op information can be ignored if it does not encode the
 * return register infor. For Linux x86-64, return values can be stored in
 * RAX, RDX, XMM0-1, YMM0-1, depending on return type. I can include all of
 * them because these are scratch registers. They are not supposed to be
 * live across function calls (unless very specialized code).
 * @append - specified that registers should be pushed onto the list without
 * clearing previous content.
 */
int mach_inst_return_registers(const DecodedInstruction *dInst, const instruction_info *ii, 
                RInfoList &regs, bool append = false);

/* return the register used for the param-th parameter of type int or fp.
 * Some ABIs have separate counts for interger and fp parameters, so one
 * can have 6 int and 8 fp register parameters for a total of 14. These are
 * the values for Linux x86-64. On the other hand, some ABIs define a total
 * limit and a parameter can be in one register or another depending on its
 * type. This is the case for Win-64.
 * Counting starts from 1.
 */
register_info register_for_int_func_parameter(int param);
register_info register_for_fp_func_parameter(int param);

/* return true if the ABI defines that params of both fp and int types
 * are counted together to determine the parameter register.
 * return false if the ABI defines that int and fp parameters must be
 * separately counted.
 */
bool combined_int_fp_param_register_count();

/* return the machine register name for the given register name.
 */
const char* register_name_to_string(RegName rn);

/* return true if the register name is an instruction pointer
 * register.
 */
bool is_instruction_pointer_register(const register_info& reg);

/* return true if the register name is a stack pointer register.
 */
bool is_stack_pointer_register(const register_info& reg);

/* return true if the register name is a flags register.
 */
bool is_flag_or_status_register(const register_info& reg);

/* return true if the register is hardwired to a specific value.
 */
bool is_hardwired_register(const register_info& reg, const DecodedInstruction *dInst, long& value);

addrtype mach_inst_branch_target(const DecodedInstruction *dInst);

/* return true if this is a conditional move instruction. 
   These instructions have difficult to predict effects on
   the value of the destination register, since they can either
   modify it or not. For this reason, I will classify them as
   having an impossible effect on slicing.
 */
bool mach_inst_performs_conditional_move (const DecodedInstruction *dInst, const instruction_info *ii = 0);

/* return true if this is a compare instruction. 
 */
bool mach_inst_performs_compare(const instruction_info *ii);

/* get register associated with a register operand
 */
register_info mach_inst_reg_operand(const DecodedInstruction *dInst, int op_num);

/* get i-th register associated with a memory operand
 */
register_info mach_inst_mem_operand_reg(const DecodedInstruction *dInst, int op_num, int i);

/* build a list with all registers read by micro-op ii part of decoded instruction dInst.
 * @internal - specifies if internal registers should be included in the list.
 */
int mach_inst_src_registers(const DecodedInstruction *dInst, const instruction_info *ii, RInfoList &regs,
         bool internal = false);

/* build a list with all registers written by micro-op ii part of decoded instruction dInst
 */
int mach_inst_dest_registers(const DecodedInstruction *dInst, const instruction_info *ii, RInfoList &regs,
         bool internal = false);

/* return length of delay slot for this instruction
 */
int mach_inst_delay_slot_length(const DecodedInstruction *dInst, const instruction_info *ii);

/* return the number of register stacks on this architecture.
   I have to manage the stacks explcitly to translate the typical
   stack offset value into a fixed name register.
 */
int number_of_register_stacks();

/* translate the passed register name into the proper operation
 * on the stack top. The stack index has been identified as part 
 * of the register decode step.
 */
int stack_operation_top_increment_value(int reg);

/* return the maximum possible stack top value (if applicable),
 * for the given stack index.
 * Note that this value is used to initialize the stack top at the start
 * of a path and we increment the top on a push. The initial value should
 * act as a buffer in case we can read/write registers besides the top of
 * the stack. absolute_reg_name = stack_top - relative_reg_name;
 * If absolute_reg_name is negative, we ignore the register.
 */
int max_stack_top_value(int stack);

#if WITH_REFERENCE_SLICING
int generic_formula_for_memory_operand(const DecodedInstruction *dInst, int uop_idx, 
                int op_num, addrtype pc, GFSliceVal& formula);

int generic_formula_for_branch_target(const DecodedInstruction *dInst, const instruction_info *ii, 
                int uop_idx, addrtype pc, GFSliceVal& formula);
#endif

/* Temporary hack to determine if a register can have only part of its bits modified.
 * I think that XMM/YMM registers can have lower and upper parts modified independently.
 * However, I've not seen any clear proof that this is the case for other general purpose
 * registers on x86. For example, if you write to EAX, it should reset the high 32-bits of
 * the full 64-bit register (RAX) to 0, not keep them unmodified. 
 * I will assume that this is the case until proven wrong.
 * Parameters: Pass the ranges of both the new and the previous register definitions
 */
bool can_have_unmodified_register_range(const register_info& rnew, const register_info& rold);

} /* namespace MIAMI */

#endif
