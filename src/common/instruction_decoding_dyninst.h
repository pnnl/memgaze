//----------------MIAMI includes-----------------
#include "instr_info.H"
//-----------------------------------------------

//---------------DYNINST includes----------------
#include "BPatch.h"
#include "Absloc.h"
#include "Instruction.h"
//-----------------------------------------------

int dyninst_decode_instruction_at_pc(void* pc, int len, MIAMI::DecodedInstruction *dInst, BPatch_function *f, BPatch_basicBlock* blk);

void get_instruction_assignments(Dyninst::Address pc, BPatch_function *f, BPatch_basicBlock* blk, std::vector<Dyninst::Assignment::Ptr> & assignments, Dyninst::InstructionAPI::Instruction::Ptr & insn);

void dyninst_dump_instruction_at_pc(MIAMI::addrtype pc, Dyninst::InstructionAPI::Instruction::Ptr & insn, std::vector<Dyninst::Assignment::Ptr> &assignments);
void dump_assignment_AST(Dyninst::AST::Ptr ast, std::string index, int num);