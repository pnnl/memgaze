#include <iostream>
//----------------MIAMI includes-----------------

//-----------------------------------------------

//---------------DYNINST includes----------------
#include "CFG.h" //parseAPI function,block
#include "BPatch_basicBlock.h"
#include "BPatch_function.h"
#include "CodeObject.h"
#include "InstructionDecoder.h"
#include "slicing.h"
#include "SymEval.h"
//-----------------------------------------------

#include "instruction_decoding_dyninst.h"

int 
dyninst_decode_instruction_at_pc(void* pc, int len, MIAMI::DecodedInstruction *dInst,  BPatch_function *f, BPatch_basicBlock* blk)
{
    Dyninst::InstructionAPI::Instruction::Ptr insn;
    std::vector<Dyninst::Assignment::Ptr> assignments;
    get_instruction_assignments((Dyninst::Address)pc,f,blk,assignments,insn);
    dyninst_dump_instruction_at_pc((Dyninst::Address)pc,insn,assignments);


   //  xed_decoded_inst_t xedd;
   
   // int res = decode_instruction(pc, len, &xedd);
   // if (res < 0)
   //    return (res);
   
   // dInst->len = xed_decoded_inst_get_length(&xedd);
   // dInst->pc = (addrtype)pc;
   // dInst->is_call = (xed_decoded_inst_get_category(&xedd)==XED_CATEGORY_CALL);
   // dInst->mach_data = malloc(sizeof(xed_decoded_inst_t));
   // if (! dInst->mach_data)  // cannot allocate memory ??
   // {
   //    fprintf(stderr, "ERROR: Instruction decode: cannot allocate memory for machine dependent data\n");
   //    return (-1);
   // }
   // memcpy(dInst->mach_data, &xedd, sizeof(xed_decoded_inst_t));
   // // decide if we need to store anything in mach_data2
   
   // res = XedInstToUopList(&xedd, &(dInst->micro_ops), pc);
   // // traverse all the micro-ops and change ExecUnit from VECTOR to SCALAR if vec_len==1
   // InstrList::iterator ilit = dInst->micro_ops.begin();
   // for ( ; ilit!=dInst->micro_ops.end() ; ++ilit)
   //    if (ilit->exec_unit==ExecUnit_VECTOR && ilit->vec_len==1)
   //       ilit->exec_unit = ExecUnit_SCALAR;
   // return (res);
    return 0;
}

void get_instruction_assignments(Dyninst::Address pc, BPatch_function *f, BPatch_basicBlock* blk, std::vector<Dyninst::Assignment::Ptr> & assignments, Dyninst::InstructionAPI::Instruction::Ptr& insn)
{
  
  Dyninst::ParseAPI::Block* parse_blk = Dyninst::ParseAPI::convert(blk);

  Dyninst::Architecture arch = parse_blk->obj()->cs()->getArch();

  Dyninst::ParseAPI::Function* parse_func = Dyninst::ParseAPI::convert(f);

#if 0
  // FIXME: tallent: This is O(|instructions in block|).
  // FIXME: tallent: Furthermore, it does not actually decode 'pc'
  
  void* buf = lm_codeSource->getPtrToInstruction(insn_myBlock->start());
  InstructionDecoder dec(buf, insn_myBlockEndAddr - insn_myBlockStartAddr, insn_myBlock->obj()->cs()->getArch());
  (*insn) = dec.decode();
  unsigned long insn_addr = insn_myBlockStartAddr;
    
  // Decode the given instruction. I might make a mistake here. 
  // The loop might decode all the instructions in the given block.
  int k = 0;
  while (NULL != (*insn) && insn_addr < pc) {
    insn_addr += (*insn)->size();
    (*insn) = dec.decode();
    k++;
  }

  AssignmentConverter ac(true);
  
  if (Dyninst::InstructionAPI::Instruction::Ptr() != (*insn))
    ac.convert((*insn), (uint8_t) insn_addr, func, insn_myBlock, *assignments);
  else 
    assert("Where is the instruction??\n");
  
#else

  const void* buf = (const void*)pc;
  Dyninst::InstructionAPI::InstructionDecoder dec(buf, Dyninst::InstructionAPI::InstructionDecoder::maxInstructionLength, arch);
  insn = dec.decode();

  Dyninst::AssignmentConverter ac(true);
  ac.convert(insn, pc, parse_func, parse_blk, assignments);
  std::cout<<"dyninst: number of assignments "<<assignments.size()<<std::endl;
#endif
}


void dyninst_dump_instruction_at_pc(Dyninst::Address pc, Dyninst::InstructionAPI::Instruction::Ptr & insn, std::vector<Dyninst::Assignment::Ptr> & assignments)
{
  using std::cerr;
  cerr << "========== DynInst Instruction "
       << "(" << std::hex << (void*)pc << std::dec << "): " << insn->format()
       << " [" << insn->size() << " bytes] "
       << " ==========\n";
  
  for (unsigned int i = 0; i < assignments.size(); i++) {
    cerr << "assignment " << i << ":" << assignments.at(i)->format() << "\n";
    std::pair<Dyninst::AST::Ptr, bool> assignPair = Dyninst::DataflowAPI::SymEval::expand(assignments.at(i), false);
    // if (assignPair.second && (NULL != assignPair.first)) {
    //   dump_assignment_AST(assignPair.first, "0", assignPair.first->numChildren());
    // }
  }
}

void dump_assignment_AST(Dyninst::AST::Ptr ast, std::string index, int num)
{
  using std::cerr;
  cerr << index << ": ";
  
  if (ast->getID() == Dyninst::AST::V_RoseAST) {
    Dyninst::DataflowAPI::RoseAST::Ptr self = Dyninst::DataflowAPI::RoseAST::convert(ast);
    cerr << "RoseAST <op:size>: " << self->format() << "\n";
    //cerr << "RoseOperation val is " << self->val().format() << "\n";
    //cerr << "RoseOperation size is " << self->val().size << "\n";
    //cerr << "RoseOperation op is " << self->val().op << "\n";
  }
  else if (ast->getID() == Dyninst::AST::V_ConstantAST) {
    Dyninst::DataflowAPI::ConstantAST::Ptr self = Dyninst::DataflowAPI::ConstantAST::convert(ast);
    cerr << "ConstantAST <val:size>: " << self->format() << "\n";
    //cerr << "Constant val is " << self->val().val << "\n";
    //cerr << "bit size of the val is " << self->val().size << "\n";

  }
  else if (ast->getID() == Dyninst::AST::V_VariableAST) {
    Dyninst::DataflowAPI::VariableAST::Ptr self = Dyninst::DataflowAPI::VariableAST::convert(ast);
    cerr << "VariableAST <region:addr>: " << self->format() << "\n";
    //cerr << "Variable region is " << self->val().reg.format() << "\n";
    //cerr << "variable address is " << self->val().addr << "\n";
  }

  int i = 0;
  while (i < num) {
    std::string index_str = std::to_string((long long)i+1);
    if (index.compare("0") != 0) {
      index_str = index + "." + index_str;
    }
    dump_assignment_AST(ast->child(i), index_str, ast->child(i)->numChildren());
    i++; 
  }
}