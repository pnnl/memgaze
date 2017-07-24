// -*-Mode: C++;-*-

//*BeginPNNLCopyright********************************************************
//
// $HeadURL$
// $Id$
//
//**********************************************************EndPNNLCopyright*

//***************************************************************************
//
// Translate ISA Instructions to MIAMI's "internal representation" via
// DynInst's "internal representation".
//
//***************************************************************************

//***************************************************************************
// 
//***************************************************************************


//***************************************************************************
// DynInst includes
//***************************************************************************

#include "BPatch.h"
#include "Absloc.h"
#include "Instruction.h"
#include "DynAST.h"
#include "slicing.h"

#include "CFG.h" //parseAPI function,block
#include "CodeObject.h"
#include "InstructionDecoder.h"
#include "slicing.h"
#include "SymEval.h"

//***************************************************************************
// MIAMI includes
//***************************************************************************

#include "instr_info.H"

//***************************************************************************
// 
//***************************************************************************

int dyninst_decode(void* pc, int len, MIAMI::DecodedInstruction *dInst, BPatch_function *f, BPatch_basicBlock* blk);

//***************************************************************************
// 
//***************************************************************************

void dynXlate_dumpInsn(instruction_data& insn_data);
void dynXlate_dumpAssignmentAST(Dyninst::AST::Ptr ast, std::string index, int num);
void dynXlate_getAssignments(instruction_data& insn_data);

//---------------------------------------------------------------------------

void dynXlate_leave(instruction_data& insn_data);
void dynXlate_nop(instruction_data& insn_data);
void dynXlate_or(instruction_data& insn_data);
void dynXlate_divide(instruction_data& insn_data);
void dynXlate_call(instruction_data& insn_data);
void dynXlate_return(instruction_data& insn_data);
void dynXlate_jump(instruction_data& insn_data);
void dynXlate_compare(instruction_data& insn_data);
void dynXlate_enter(instruction_data& insn_data);
void dynXlate_sysCall(instruction_data& insn_data);
void dynXlate_prefetch(instruction_data& insn_data);
void dynXlate_test(instruction_data& insn_data);

//---------------------------------------------------------------------------

void dynXlate_assignments(instruction_data& insn_data);
void dynXlate_assignment(Dyninst::Assignment::Ptr aptr, instruction_data& insn_data);
void dynXlate_assignmentAST(Dyninst::Assignment::Ptr aptr, MIAMI::instruction_info* uop, Dyninst::AST::Ptr ast, std::vector<Dyninst::DataflowAPI::RoseAST::Ptr> & insn_opVec, instruction_data& insn_data);

//---------------------------------------------------------------------------

void dynXlate_jump_ast(MIAMI::instruction_info* uop, Dyninst::AST::Ptr ast, Dyninst::Assignment::Ptr aptr, instruction_data& insn_data);


//***************************************************************************

void traverse_or(MIAMI::instruction_info* uop, Dyninst::AST::Ptr ast);
int traverse_options(MIAMI::instruction_info* uop, Dyninst::DataflowAPI::RoseAST::Ptr ast);
void traverse_rose(Dyninst::AST::Ptr ast, std::vector<Dyninst::DataflowAPI::RoseAST::Ptr> & insn_opVec);

void convert_operator(MIAMI::instruction_info* uop, Dyninst::DataflowAPI::ROSEOperation::Op op);
void get_operator(MIAMI::instruction_info* uop, Dyninst::AST::Ptr ast, Dyninst::Assignment::Ptr aptr, std::vector<Dyninst::DataflowAPI::RoseAST::Ptr> & insn_opVec, instruction_data& insn_data);

int calculate_jump_val(Dyninst::AST::Ptr ast,int insn_jump_tgt,int & insn_jump_tgt2);

unsigned int get_register_index(Dyninst::Absloc a, std::vector<Dyninst::Absloc>& insn_locVec);
int get_src_reg_lsb(Dyninst::MachRegister mr);
Dyninst::MachRegister get_machine_flag_reg(Dyninst::Architecture arch);

bool sourceOpIsFlag(Dyninst::DataflowAPI::VariableAST::Ptr ast_var, Dyninst::Architecture arch);
bool destOpIsFlag(Dyninst::Assignment::Ptr aptr, Dyninst::Architecture arch);

void update_dest_with_flag(instruction_data& insn_data);

bool check_flags_registers(Dyninst::MachRegister mr, Dyninst::Architecture arch);
int check_stack_register(Dyninst::MachRegister mr, Dyninst::Architecture arch);
int check_vector_regs(Dyninst::InstructionAPI::Instruction::Ptr insn, Dyninst::Assignment::Ptr aptr, Dyninst::Architecture arch);

void append_src_regs(MIAMI::instruction_info* uop, MIAMI::OperandType opt, Dyninst::InstructionAPI::Instruction::Ptr insn, Dyninst::MachRegister mr, int size, Dyninst::Architecture arch);
void append_dest_regs(MIAMI::instruction_info* uop, MIAMI::OperandType opt, Dyninst::InstructionAPI::Instruction::Ptr insn, Dyninst::MachRegister mr, int size, Dyninst::Architecture arch);
void append_all_dest_registers(MIAMI::instruction_info* uop, Dyninst::InstructionAPI::Instruction::Ptr insn, Dyninst::Assignment::Ptr aptr, Dyninst::Architecture arch);

void get_width_from_tree(MIAMI::instruction_info* uop, Dyninst::AST::Ptr ast);

void create_add_micro(Dyninst::Assignment::Ptr aptr, instruction_data& insn_data);
void create_load_micro(Dyninst::Assignment::Ptr aptr, int idx, instruction_data& insn_data);
void create_store_micro(Dyninst::Assignment::Ptr aptr, int idx, instruction_data& insn_data);
void create_jump_micro(int jump_val, Dyninst::Assignment::Ptr assign, bool mem, instruction_data& insn_data);
void create_return_micro(Dyninst::Assignment::Ptr aptr, instruction_data& insn_data);
void create_compare_micro(instruction_data& insn_data);


bool check_is_zf(Dyninst::MachRegister mr, Dyninst::Architecture arch);
int get_vec_len(Dyninst::InstructionAPI::Instruction::Ptr insn);

MIAMI::ExecUnitType get_execution_type(Dyninst::InstructionAPI::Instruction::Ptr insn);

void get_dest_field(Dyninst::Assignment::Ptr aptr, MIAMI::instruction_info* uop, std::vector<Dyninst::Absloc>& insn_locVec);

void parse_assign(Dyninst::Assignment::Ptr aptr, MIAMI::instruction_info* uop, instruction_data& insn_data);
