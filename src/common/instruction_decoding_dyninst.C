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

int dyninst_decode_instruction_at_pc(void* pc, int len, MIAMI::DecodedInstruction *dInst,  BPatch_function *f, BPatch_basicBlock* blk){
  instruction_data insn_data;
  insn_data.pc = (Dyninst::Address)pc;
  insn_data.dInst = dInst;
  insn_data.parse_func = Dyninst::ParseAPI::convert(f);
  insn_data.parse_blk = Dyninst::ParseAPI::convert(blk);
  insn_data.arch = insn_data.parse_blk->obj()->cs()->getArch();

  get_instruction_assignments(insn_data);
  dyninst_dump_instruction_at_pc(insn_data);

  insn_data.dInst->len = (int) insn_data.dyn_insn->size(); 
  insn_data.dInst->pc = insn_data.pc; 
  insn_data.dInst->is_call = false;

  // The instruction's pointer in dyninst may not point to the machine data which MIAMI wants.
  // Haven't encounter any situations which mach_data is used. Thus, leave it like this for now.
  insn_data.dInst->mach_data = const_cast<void*>( static_cast<const void*>(insn_data.dyn_insn->ptr()) ); // CHECK
  bool x86 = (insn_data.arch == Dyninst::Architecture::Arch_x86
          || insn_data.arch == Dyninst::Architecture::Arch_x86_64);

  if (x86 && insn_data.dyn_insn->getOperation().getID() == e_leave) {
    dynXlate_leave(insn_data);
  }
  else if (x86 && insn_data.dyn_insn->getOperation().getID() == e_nop) {
    dynXlate_nop(insn_data); // x86 nop
  }
  else if (x86 && insn_data.dyn_insn->getOperation().getID() >= e_or && insn_data.dyn_insn->getOperation().getID() <= e_orps) {
    dynXlate_or(insn_data);
  }
  else if (x86 && insn_data.dyn_insn->getOperation().getID() >= e_div && insn_data.dyn_insn->getOperation().getID() <= e_divss) {
    dynXlate_divide(insn_data);
  }
  else {
    switch(insn_data.dyn_insn->getCategory()) {
      case Dyninst::InstructionAPI::c_CallInsn:
        insn_data.dInst->is_call = true;
        dynXlate_call(insn_data);
        break;
      case Dyninst::InstructionAPI::c_ReturnInsn:
        dynXlate_return(insn_data); 
        break;
      case Dyninst::InstructionAPI::c_BranchInsn:
       dynXlate_jump(insn_data);
        break;
      case Dyninst::InstructionAPI::c_CompareInsn:
        dynXlate_compare(insn_data);
        break;
      case Dyninst::InstructionAPI::c_SysEnterInsn:
        dynXlate_enter(insn_data); // this function hasn't been implemented yet.
        break;
      case Dyninst::InstructionAPI::c_SyscallInsn:
        dInst->is_call = true;
        dynXlate_sysCall(insn_data);
        break;
      case Dyninst::InstructionAPI::c_PrefetchInsn:
        dynXlate_prefetch(insn_data);
        break;
      case Dyninst::InstructionAPI::c_NoCategory:{
        dynXlate_assignments(insn_data); // default choice is always dynXlate_assignments.
        break;
      }
      default:
        assert("Impossible!\n");
        break;
    }
  }  
  return 0;
}

//void get_instruction_assignments(Dyninst::Address pc, Dyninst::ParseAPI::Function* parse_func, Dyninst::ParseAPI::Block* parse_blk, std::vector<Dyninst::Assignment::Ptr> & assignments, 
//    Dyninst::InstructionAPI::Instruction::Ptr insn, Dyninst::Architecture arch)
void get_instruction_assignments(instruction_data & insn_data){ 
  const void* buf = (const void*)insn_data.pc;
  Dyninst::InstructionAPI::InstructionDecoder dec(buf, Dyninst::InstructionAPI::InstructionDecoder::maxInstructionLength, insn_data.arch);
  insn_data.dyn_insn = dec.decode();

  Dyninst::AssignmentConverter ac(true);
  ac.convert(insn_data.dyn_insn, insn_data.pc, insn_data.parse_func, insn_data.parse_blk, insn_data.assignments);
  std::cout<<"dyninst: number of assignments "<<insn_data.assignments.size()<<std::endl;
}


//void dyninst_dump_instruction_at_pc(Dyninst::Address pc, Dyninst::InstructionAPI::Instruction::Ptr insn, std::vector<Dyninst::Assignment::Ptr> & assignments)
void dyninst_dump_instruction_at_pc(instruction_data & insn_data){
  using std::cerr;
  cerr << "========== DynInst Instruction "
       << "(" << std::hex << (void*)insn_data.pc << std::dec << "): " << insn_data.dyn_insn->format()
       << " [" << insn_data.dyn_insn->size() << " bytes] "
       << " ==========\n";
  
  for (unsigned int i = 0; i < insn_data.assignments.size(); i++) {
    cerr << "assignment " << i << ":" << insn_data.assignments.at(i)->format() << "\n";
    std::pair<Dyninst::AST::Ptr, bool> assignPair = Dyninst::DataflowAPI::SymEval::expand(insn_data.assignments.at(i), false);
    // if (assignPair.second && (NULL != assignPair.first)) {
    //   dump_assignment_AST(assignPair.first, "0", assignPair.first->numChildren());
    // }
  }
}

void dump_assignment_AST(Dyninst::AST::Ptr ast, std::string index, int num){
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

// Translate leave instruction.Usually contains 3 uops: Copy, Load, Add.
//void dynXlate_leave(MIAMI::DecodedInstruction* dInst, std::vector<Dyninst::Assignment::Ptr> & assignments, Dyninst::InstructionAPI::Instruction::Ptr insn, 
//    std::vector<Dyninst::Absloc>& insn_locVec, Dyninst::Architecture arch)
void dynXlate_leave(instruction_data & insn_data){
  for (auto ait = insn_data.assignments.begin(); ait != insn_data.assignments.end(); ++ait) {
    insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
    MIAMI::instruction_info * uop = &insn_data.dInst->micro_ops.back();
    get_dest_field(*ait, uop, insn_data.insn_locVec);
    parse_assign(*ait, uop, insn_data);
    if (uop->type == MIAMI::IB_copy)
      uop->primary = true;
    if ((*ait)->out().absloc().type() == Dyninst::Absloc::Register)
    {
       append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, (*ait)->out().absloc().reg(), uop->width * uop->vec_len, insn_data.arch);
    }
  }
  create_add_micro(insn_data.assignments.at(0),insn_data);
}

// Translate the nop instruction 
void dynXlate_nop(instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back();
  uop->type = MIAMI::IB_nop;
  uop->width = 0;
  uop->vec_len = 0;
  uop->exec_unit = MIAMI::ExecUnit_SCALAR;
  uop->exec_unit_type = MIAMI::ExecUnitType_INT;
}

// Translate or instruction. Make it into a special case for simplicity.
void dynXlate_or(instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back();
  uop->type = MIAMI::IB_logical;
  uop->primary = true;
  uop->vec_len = get_vec_len(insn_data.dyn_insn);
  uop->exec_unit = (uop->vec_len == 1) ? MIAMI::ExecUnit_SCALAR : MIAMI::ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn_data.dyn_insn);

  uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_INTERNAL, 1);
  append_src_regs(uop, MIAMI::OperandType_INTERNAL, insn_data.dyn_insn, Dyninst::MachRegister(), uop->width * uop->vec_len, insn_data.arch);

  uop->dest_reg_list.clear();
  uop->dest_opd[uop->num_dest_operands++] = make_operand(MIAMI::OperandType_INTERNAL, 0);
  append_dest_regs(uop, MIAMI::OperandType_INTERNAL, insn_data.dyn_insn, Dyninst::MachRegister(), uop->width * uop->vec_len, insn_data.arch);
  uop->dest_opd[uop->num_dest_operands] = make_operand(MIAMI::OperandType_REGISTER, uop->num_dest_operands);
  uop->num_dest_operands++;
  append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, get_machine_flag_reg(insn_data.arch), 1, insn_data.arch);

  for (unsigned int i = 0; i < insn_data.assignments.size(); ++i)
  {
    // if output field is not flag, we traverse its AST to get constant values and its width
    Dyninst::Absloc aloc = insn_data.assignments.at(i)->out().absloc();
    if (aloc.type() == Dyninst::Absloc::Register)
    {
      if (!check_flags_registers(aloc.reg(),insn_data.arch))
        {
          std::pair<Dyninst::AST::Ptr, bool> astPair;
          astPair = Dyninst::DataflowAPI::SymEval::expand(insn_data.assignments.at(i), false);
          if (astPair.second && NULL != astPair.first) // 
          {
            if (!uop->width)
              get_width_from_tree(uop, astPair.first);
            
            traverse_or(uop, astPair.first); // get value from AST
            create_load_micro(insn_data.assignments.at(i), 1, insn_data);
            create_store_micro(insn_data.assignments.at(i), 0, insn_data);
            return;
          }
        }  
    }
  }

  // Have to create load an store micro. Take default choice 0.
  create_load_micro(insn_data.assignments.at(0), 1, insn_data);
  uop->width = insn_data.dInst->micro_ops.front().width;
  create_store_micro( insn_data.assignments.at(0), 0, insn_data);
  return;
}

// Translate the divide instruction. Make it a special case because it contains 8 assignments 
// and this way is easier. 
void dynXlate_divide(instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back();
  uop->type = MIAMI::IB_div;
  if(!uop->vec_len) uop->vec_len = get_vec_len(insn_data.dyn_insn);
  uop->primary = true;
  uop->exec_unit = (uop->vec_len == 1) ? MIAMI::ExecUnit_SCALAR : MIAMI::ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn_data.dyn_insn);

  // get all the source operands from one assignment
  Dyninst::Assignment::Ptr aptr = insn_data.assignments.at(0);
  for (unsigned int i = 0; i < aptr->inputs().size(); ++i)
  {
    if (aptr->inputs().at(i).absloc().type() == Dyninst::Absloc::Register)
    {
      if (!check_flags_registers(aptr->inputs().at(i).absloc().reg(),insn_data.arch))
      {
        Dyninst::MachRegister mr = aptr->inputs().at(i).absloc().reg();
        uop->src_opd[uop->num_src_operands] = make_operand(MIAMI::OperandType_REGISTER, uop->num_src_operands);
        uop->num_src_operands++;
        append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, mr, mr.size() * 8, insn_data.arch);
        insn_data.insn_locVec.push_back(aptr->inputs().at(i).absloc());
        if (!uop->width)
          uop->width = mr.size() * 8;
      }
    }
  }

  // get all the destination operands from all assignments
  for (unsigned int i = 0; i < insn_data.assignments.size(); ++i)
  {
    Dyninst::Absloc aloc = insn_data.assignments.at(i)->out().absloc();
    if (insn_data.assignments.at(i)->out().absloc().type() == Dyninst::Absloc::Register)
    {
      if (!check_flags_registers(insn_data.assignments.at(i)->out().absloc().reg(),insn_data.arch))
      {
        Dyninst::MachRegister mr = insn_data.assignments.at(i)->out().absloc().reg();
        uop->dest_opd[uop->num_dest_operands++] = make_operand(MIAMI::OperandType_REGISTER, get_register_index(aloc,insn_data.insn_locVec));
        append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, mr, mr.size() * 8, insn_data.arch);
      }
    }
  }

  // for x86 machines, we clapse all the flag output regions into one: x86::flags or x86_64::flags
  if (insn_data.arch == Dyninst::Architecture::Arch_x86 || insn_data.arch == Dyninst::Architecture::Arch_x86_64)
  {
    uop->dest_opd[uop->num_dest_operands] = make_operand(MIAMI::OperandType_REGISTER, uop->num_dest_operands);
    uop->num_dest_operands++;
    append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, get_machine_flag_reg(insn_data.arch), 1, insn_data.arch);
  }
}

// Translate call instruction (two different kinds, with load (5 uops) or without load (4 uops)).
void dynXlate_call(instruction_data & insn_data){
  for (unsigned int i = 0; i < insn_data.assignments.size(); i++) {
    if (insn_data.assignments.at(i)->inputs().size() > 0) {
      insn_data.dInst->micro_ops.push_front(MIAMI::instruction_info());  // we push front because Dyninst reverses the order 
                                                        // of the assignments in this case of call. 
      MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.front();
      get_dest_field(insn_data.assignments.at(i), uop,insn_data.insn_locVec);
      parse_assign(insn_data.assignments.at(i), uop, insn_data);
      if (insn_data.dInst->micro_ops.front().type == MIAMI::IB_load) // special call with load usually call [??? + ???]
      {
        uop = &insn_data.dInst->micro_ops.front();
        uop->num_dest_operands = 1; // Reassign the destination value of load uop in call insn.
        uop->dest_opd[0] = make_operand(MIAMI::OperandType_INTERNAL, 0);
        append_dest_regs(uop, MIAMI::OperandType_INTERNAL, insn_data.dyn_insn, Dyninst::MachRegister(), uop->width * uop->vec_len, insn_data.arch);
      } else {
        append_all_dest_registers(uop, insn_data.dyn_insn, insn_data.assignments.at(i), insn_data.arch);
      }
    }
  }
  int insn_jump_tgt = 0;
  int insn_jump_tgt2 = 0;
  // Find the asssignment with AST and create jump uop using the value retrieved from AST.
  for (unsigned int i = 0; i < insn_data.assignments.size(); ++i) {
    std::pair<Dyninst::AST::Ptr, bool> astPair;
    astPair = Dyninst::DataflowAPI::SymEval::expand(insn_data.assignments.at(i), false);
    if (astPair.second && NULL != astPair.first) {
      insn_jump_tgt = calculate_jump_val(astPair.first,insn_jump_tgt,insn_jump_tgt2); // We need AST to find the value which the call jump to.
      if (insn_jump_tgt != insn_jump_tgt2){
        std::cout<<"error: jump targets do not match: "<<insn_jump_tgt<<" "<<insn_jump_tgt2<<std::endl;
        exit(0);
      }

      if (insn_data.dInst->micro_ops.size() == 3) { // If we have load uop, change the jump's src operand to INTERNAL
    create_jump_micro(0, insn_data.assignments.at(0), true, insn_data);
      }
      else { // no load operation
    insn_jump_tgt = insn_data.dInst->len;
    create_jump_micro(insn_jump_tgt, insn_data.assignments.at(0), false, insn_data);
      }

      insn_jump_tgt = 0; // this is static
      break;
      
    }
  }  

  // the first add is usually the second assignment, so we pass the second one over. CHECK to make sure.
  create_add_micro(insn_data.assignments.at(1), insn_data); 
}

// Translate 'return' instruction into 3 uops: Load, Add, Return.
void dynXlate_return(instruction_data & insn_data){
  if (insn_data.assignments.size() != 2) 
    assert("Leave instruction should contain two and only two assignments.\n"); 
  
  // only translate the first assignments
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back();
  get_dest_field(insn_data.assignments.at(0), uop, insn_data.insn_locVec);
  parse_assign(insn_data.assignments.at(0), uop, insn_data);

  if (insn_data.assignments.at(0)->out().absloc().type() == Dyninst::Absloc::Register)
     append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, insn_data.assignments.at(0)->out().absloc().reg(), uop->width * uop->vec_len, insn_data.arch);   
  
  create_add_micro(insn_data.assignments.at(1),insn_data); // use the second assignments to create add uop
  create_return_micro(insn_data.assignments.at(0),insn_data);
}

// Translate the jump instruction
void dynXlate_jump(instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info *uop = &insn_data.dInst->micro_ops.back();

  get_dest_field(insn_data.assignments.at(0), uop, insn_data.insn_locVec);
  std::pair<Dyninst::AST::Ptr, bool> astPair;
  astPair = Dyninst::DataflowAPI::SymEval::expand(insn_data.assignments.at(0), false);
  int insn_jump_tgt =0;
  int insn_jump_tgt2 =0;
  if (astPair.second && NULL != astPair.first) {
    dynXlate_jump_ast(uop, astPair.first, insn_data.assignments.at(0),insn_data); 
    insn_jump_tgt += calculate_jump_val(astPair.first,insn_jump_tgt,insn_jump_tgt2);
    if (insn_jump_tgt != insn_jump_tgt2){
        std::cout<<"error dynXlate_jump jump targets do not match: "<<insn_jump_tgt<<" "<<insn_jump_tgt<<std::endl;
    }
  }
  
  if (insn_jump_tgt) { // If the AST gives the constant operand
    insn_jump_tgt = insn_jump_tgt - insn_data.dInst->len;
    uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; //FIXME
    uop->imm_values[uop->num_imm_values].value.s = insn_jump_tgt;
    uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_IMMED, uop->num_imm_values++); 
    insn_jump_tgt = 0;  
  }

  Dyninst::Absloc out_aloc = insn_data.assignments.at(0)->out().absloc();
  if (out_aloc.type() == Dyninst::Absloc::Register)
     append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, out_aloc.reg(), out_aloc.reg().size() * 8, insn_data.arch); 
}

// Translate jump by traversing its AST.
void dynXlate_jump_ast( MIAMI::instruction_info* uop, Dyninst::AST::Ptr ast, Dyninst::Assignment::Ptr aptr, instruction_data & insn_data){
  if (uop->vec_len == 0) {
    uop->vec_len = get_vec_len(insn_data.dyn_insn);
  }
    
  if (uop->exec_unit == MIAMI::ExecUnit_LAST) uop->exec_unit = (uop->vec_len == 1) ? MIAMI::ExecUnit_SCALAR : MIAMI::ExecUnit_VECTOR;
  if (uop->exec_unit_type == MIAMI::ExecUnitType_LAST) uop->exec_unit_type = get_execution_type(insn_data.dyn_insn);
  if (!uop->width) get_width_from_tree(uop, ast);
  std::vector<Dyninst::DataflowAPI::RoseAST::Ptr>  insn_opVec;
  unsigned int i;
  switch(ast->getID()){
    case Dyninst::AST::V_RoseAST:{
      Dyninst::DataflowAPI::RoseAST::Ptr ast_r = Dyninst::DataflowAPI::RoseAST::convert(ast);

      if (uop->type == MIAMI::IB_unknown) { 
        get_operator(uop, ast, aptr, insn_opVec, insn_data);

        if (uop->type == MIAMI::IB_add) 
          uop->type = MIAMI::IB_branch; // CHECK

        if (uop->type == MIAMI::IB_load) { // deal with jump qword ptr [rip+0x200682]
          create_load_micro(aptr, 0, insn_data);
          uop->type = MIAMI::IB_branch;
          uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_INTERNAL, 0);
          append_src_regs(uop, MIAMI::OperandType_INTERNAL, insn_data.dyn_insn, Dyninst::MachRegister(), \
                                    insn_data.dInst->micro_ops.front().width * insn_data.dInst->micro_ops.front().vec_len, insn_data.arch);
          return;
        }
        if (uop->type == MIAMI::IB_unknown) 
          uop->type = MIAMI::IB_branch;
      }

      if (ast_r->val().op == Dyninst::DataflowAPI::ROSEOperation::ifOp || ast_r->val().op == Dyninst::DataflowAPI::ROSEOperation::derefOp || uop->type == MIAMI::IB_branch) { 
      // if conditional and operator is 'if' || unconditional branching --> might get more imms, fine with dg.
        for (i = 0; i < ast->numChildren(); ++i) {
          dynXlate_jump_ast(uop, ast->child(i),aptr,insn_data);
        }  
      } else {
        dynXlate_jump_ast(uop, ast->child(0),aptr,insn_data);
      }
      break;
    }

    case Dyninst::AST::V_ConstantAST:{ // can only get width here

      if (uop->num_imm_values < 2) 
      {
        Dyninst::DataflowAPI::ConstantAST::Ptr ast_c = Dyninst::DataflowAPI::ConstantAST::convert(ast);
        if (uop->width == 0 && uop->vec_len) {
          uop->width = ast_c->val().size / uop->vec_len;
        }
      }
      break;
    }
      

    case Dyninst::AST::V_VariableAST:{ // can get type, width and source registers.

      Dyninst::DataflowAPI::VariableAST::Ptr ast_v = Dyninst::DataflowAPI::VariableAST::convert(ast);

      if (uop->type == MIAMI::IB_unknown)
        uop->type = MIAMI::IB_branch;
         
      if (ast_v->val().reg.absloc().type() == Dyninst::Absloc::Register){
        if (uop->width == 0 && uop->vec_len) {
          uop->width = ast_v->val().reg.absloc().reg().size() * 8 / uop->vec_len;
        }
            
        unsigned int idx = get_register_index(ast_v->val().reg.absloc(), insn_data.insn_locVec);
        uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx);

        int size = 0;
        if (insn_opVec.size() && insn_opVec.back()->val().op == Dyninst::DataflowAPI::ROSEOperation::extractOp)
          size = insn_opVec.back()->val().size; // don't need to time it by 8, its already in bit
        else 
          size = ast_v->val().reg.absloc().reg().size() * 8;
          append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, ast_v->val().reg.absloc().reg(), size, insn_data.arch);
      }
      break;
    }
    default:{
    assert("Wrong type of AST node.\n");

    }
  }
}

// Translate compare uop
void dynXlate_compare(instruction_data & insn_data){
  Dyninst::Assignment::Ptr assign = insn_data.assignments.at(0);
  std::vector<Dyninst::AbsRegion> inputs = assign->inputs();
  for (unsigned int i = 0; i < inputs.size(); ++i)
  {
    Dyninst::Absloc aloc = inputs.at(i).absloc();
    if (aloc.type() == Dyninst::Absloc::Stack || aloc.type() == Dyninst::Absloc::Heap || aloc.type() == Dyninst::Absloc::Unknown) {
      create_load_micro(assign, 0, insn_data); // Create load uop if we have mem as input field.
      insn_data.insn_locVec.push_back(aloc);
    }
  }

  create_compare_micro(insn_data);
  //update_dest_with_flag(dInst, insn); // Compare instruction usually has destination fields being flags.
}

// Haven't encounter any enter yet. Don't really know how MIAMI translate it.
void dynXlate_enter(instruction_data & insn_data)
{
  return;
}

// System call is translated into jump uop, with two destination operands:
// program counter and flags.
void dynXlate_sysCall(instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back();
  uop->type = MIAMI::IB_jump;
  uop->width = 64;
  uop->primary = true;
  if(!uop->vec_len) uop->vec_len = get_vec_len(insn_data.dyn_insn);
  uop->exec_unit = (uop->vec_len == 1) ? MIAMI::ExecUnit_SCALAR : MIAMI::ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn_data.dyn_insn);

  Dyninst::MachRegister pc = Dyninst::Absloc::makePC(insn_data.arch).reg();
  uop->dest_opd[uop->num_dest_operands] = make_operand(MIAMI::OperandType_REGISTER, uop->num_dest_operands);
  uop->num_dest_operands++;
  append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, pc, pc.size() * 8, insn_data.arch);
  
  uop->dest_opd[uop->num_dest_operands] = make_operand(MIAMI::OperandType_REGISTER, uop->num_dest_operands);
  uop->num_dest_operands++;
  append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, get_machine_flag_reg(insn_data.arch), 1, insn_data.arch);
}

void dynXlate_prefetch(instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back();
  uop->type = MIAMI::IB_prefetch;
  uop->width = 64;
  uop->primary = true;
  uop->vec_len = 8; // Is this always true?
  uop->exec_unit = MIAMI::ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn_data.dyn_insn);
  uop->src_opd[uop->num_src_operands] = make_operand(MIAMI::OperandType_MEMORY, uop->num_src_operands);
  uop->num_src_operands++;
  
  for (unsigned int i = 0; i < insn_data.assignments.size(); ++i)
  {
     if (insn_data.assignments.at(i)->inputs().at(0).absloc().type() == Dyninst::Absloc::Register)
     {
        append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, insn_data.assignments.at(i)->inputs().at(0).absloc().reg(), \
                insn_data.assignments.at(i)->inputs().at(0).absloc().reg().size() * 8, insn_data.arch);
        break;
     }
  }
}

// dynXlate_all the assignments of an instruction
void dynXlate_assignments(instruction_data & insn_data){
  int flag = 0;
  for (unsigned int i = 0; i < insn_data.assignments.size(); i++) {
    if (destOpIsFlag(insn_data.assignments.at(i),insn_data.arch)) {
      flag = 1;
    } 
    else {
      // If it's not flag, we start processing the assignment.
      dynXlate_assignment(insn_data.assignments.at(i), insn_data);
    }
  }
  if (insn_data.dInst->micro_ops.size() == 1) {
    insn_data.dInst->micro_ops.back().primary = true; // only one uop, it must be primary.
  }

  if (!flag && insn_data.dInst->micro_ops.size() == 1 && insn_data.dInst->micro_ops.back().type == MIAMI::IB_add && insn_data.dInst->micro_ops.back().vec_len <= 1) { // not including vector operand
    insn_data.dInst->micro_ops.back().type = MIAMI::IB_lea;
    insn_data.dInst->micro_ops.back().src_opd[insn_data.dInst->micro_ops.back().num_src_operands++] = make_operand(MIAMI::OperandType_MEMORY, 0);

    if (insn_data.assignments.at(0)->inputs().size()){ // lea get address not from rip
      Dyninst::Absloc aloc = insn_data.assignments.at(0)->inputs().at(0).absloc();
      if (aloc.type() == Dyninst::Absloc::Register){
        append_src_regs(&insn_data.dInst->micro_ops.back(), MIAMI::OperandType_MEMORY, insn_data.dyn_insn, aloc.reg(), aloc.reg().size() * 8, insn_data.arch);
      }
    }
    else {
      append_src_regs(&insn_data.dInst->micro_ops.back(), MIAMI::OperandType_MEMORY, insn_data.dyn_insn, Dyninst::MachRegister(), 64, insn_data.arch);
    }

  /* Special case: processing push and pop.
   * It seems that dynXlate_insnectAST cannot cannot get the CONSTANT values in 'add' uop, so we append it here.
   * The reason it cannot get the constant value is because the traverse_options function do not traverse
   * all the children of 'add' ROSEOperations' in most of the cases (but only the first one).
   * We do it that way to stop the recursion sooner for the general uops. Many uops contains 'add' ROSEOperation.
   * They are usually large and usually only the first branch of add's children is meaningful. CHECK*/

  }
  else if (!flag && insn_data.dInst->micro_ops.size() == 2 && insn_data.dInst->micro_ops.back().type == MIAMI::IB_add && insn_data.dInst->micro_ops.front().type == MIAMI::IB_store){ // push
    MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back(); // get the 'add' uop
    uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; 
    uop->imm_values[uop->num_imm_values].value.s = -8; 
    uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_IMMED, uop->num_imm_values++);
    insn_data.dInst->micro_ops.reverse(); // reverse the order of store and add. Dyninst order does not corresponds
                                // well with MIAMI's order sometimes.

  } 
  else if (!flag && insn_data.dInst->micro_ops.size() == 2 && insn_data.dInst->micro_ops.back().type == MIAMI::IB_add && insn_data.dInst->micro_ops.front().type == MIAMI::IB_load){ // pop
    MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back(); // get the 'add' uop
    uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; 
    uop->imm_values[uop->num_imm_values].value.s = 8; 
    uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_IMMED, uop->num_imm_values++);
  }

  if (flag) {
    update_dest_with_flag(insn_data); // update the primary uop with a flag destination. (x86/x86_64)
  }
  if (flag && insn_data.dInst->micro_ops.size() == 0){ 
    dynXlate_test(insn_data); // all dest ops are flags-->test uop
  }
  else if (flag && insn_data.dInst->micro_ops.size() == 0 && insn_data.dInst->micro_ops.begin()->vec_len > 1){

  }
}

void dynXlate_assignment(Dyninst::Assignment::Ptr aptr, instruction_data & insn_data){
  // start a new micro operation
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info *uop = &insn_data.dInst->micro_ops.back();

  std::pair<Dyninst::AST::Ptr, bool> astPair;
  astPair = Dyninst::DataflowAPI::SymEval::expand(aptr, false);

  //need to check special xmm/ymm case
  int len = check_vector_regs(insn_data.dyn_insn, aptr, insn_data.arch);
  if (len) {
   uop->vec_len = len;
   uop->exec_unit = MIAMI::ExecUnit_VECTOR; 
  }

  get_dest_field(aptr, uop,insn_data.insn_locVec);

  // If the assignment contains an abstract syntax tree
  std::vector<Dyninst::DataflowAPI::RoseAST::Ptr>  insn_opVec;
  if (astPair.second && NULL != astPair.first) {
    //check whether there are any load assignments
    get_operator(uop, astPair.first, aptr,insn_opVec, insn_data);

    dynXlate_insnectAST( aptr, uop, astPair.first, insn_opVec, insn_data);

    // These special cases are quite messy. Even "add [R14 + 8], 1" and "add [R15 + RAX * 4], 1" are translated 
    // differently, with the first one having 3 uops: load, add, store, and the second one having 2 uops (wrong)
    // load and store. (Let along even more special cases like "xadd"--exchange and add, 
    // which dyninst gives little information and no AST. So it will be processed in parse_assign.
    // Thus, for now, xadd can only be translate into a load and a store as well. )

    // add [RAX], AL and or [RIP+xxx], EAX
    Dyninst::Absloc aloc = aptr->out().absloc();
    if ((aloc.type() == Dyninst::Absloc::Heap || aloc.type() == Dyninst::Absloc::Stack || aloc.type() == Dyninst::Absloc::Unknown) && insn_data.dInst->micro_ops.size() == 1) {

      if (uop->type != MIAMI::IB_store) //uop->type == IB_add
      {
        create_store_micro(aptr, 1, insn_data);
        uop->primary = true; // add the is primary micro
        uop->dest_opd[0] = make_operand(MIAMI::OperandType_INTERNAL, 1);
        append_dest_regs(uop, MIAMI::OperandType_INTERNAL, insn_data.dyn_insn, Dyninst::MachRegister(), uop->width * uop->vec_len, insn_data.arch);// FIXME
      }
    }


    // If inputs fields being memory.
    for (unsigned int i = 1; i < aptr->inputs().size(); ++i) { 
      Dyninst::Absloc aloc = aptr->inputs().at(i).absloc();
      if (aloc.type() == Dyninst::Absloc::Stack || aloc.type() == Dyninst::Absloc::Heap || aloc.type() == Dyninst::Absloc::Unknown) {
        if (uop->type != MIAMI::IB_load){ // the in case of add [...]. which dynXlate_insnectAST will return only an add uop,
                                   // since AST does not contain information of memory field, but we need
                                   // to create another load uop. 
          create_load_micro(aptr, 0, insn_data);
          insn_data.insn_locVec.push_back(aptr->inputs().at(i).absloc());

          // Change the REGISTER operand by INTERNAL operand since we have another load uop. 
          // Not the best practice to change the last element of source operands like the
          // following. But leave it like this for now. 
          if(uop->num_src_operands) uop->src_opd[uop->num_src_operands-1] = make_operand(MIAMI::OperandType_INTERNAL, 0); //FIXME, not the right way to do it
          append_src_regs(uop, MIAMI::OperandType_INTERNAL, insn_data.dyn_insn, Dyninst::MachRegister(), uop->width * uop->vec_len, insn_data.arch);
          // Similarly, change the destination memory space into internal too. 
          if (aloc.type() == Dyninst::Absloc::Register)
          {
            uop->dest_opd[0] = make_operand(MIAMI::OperandType_REGISTER, 1);
            uop->dest_reg_list.clear();
            append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, aloc.reg(), aloc.reg().size() * 8, insn_data.arch);  
          }
          
          break;
        } 
      }
    }


  } else { // The assignment doesn't contain ast
    parse_assign(aptr, uop, insn_data);
  }

  if (uop->num_dest_operands > 4) // This is a very bad bug I haven't figure out yet. Some times the program gives
                                  // > 100 destination operand for instructions like fild or fld. I haven't been 
                                  // able to figure out yet. So I just arbitrarily assign the value to be 1 so 
                                  // append_all_dest_registers won't segfault. 
  {
    uop->num_dest_operands = 1;
  }
  append_all_dest_registers(uop, insn_data.dyn_insn, aptr,insn_data.arch); // processe all destination register information.
}

// Parse the AST of an assignment.
void dynXlate_insnectAST(Dyninst::Assignment::Ptr aptr, MIAMI::instruction_info* uop, Dyninst::AST::Ptr ast, std::vector<Dyninst::DataflowAPI::RoseAST::Ptr> & insn_opVec, instruction_data & insn_data){
  Dyninst::Absloc aloc;
  if (uop->vec_len == 0){ 
    uop->vec_len = get_vec_len(insn_data.dyn_insn);
  }
  if (uop->exec_unit == MIAMI::ExecUnit_LAST){ 
    uop->exec_unit = (uop->vec_len == 1) ? MIAMI::ExecUnit_SCALAR : MIAMI::ExecUnit_VECTOR;
  }
  if (uop->exec_unit_type == MIAMI::ExecUnitType_LAST) {
    uop->exec_unit_type = get_execution_type(insn_data.dyn_insn);
  }
  // special cases when memory field does not shown in ast --> CHECK
  if (uop->type == MIAMI::IB_load && !uop->num_src_operands) {
    for (unsigned int i = 0; i < aptr->inputs().size(); ++i) {
      std::pair<Dyninst::AST::Ptr, bool> astPair;
      astPair = Dyninst::DataflowAPI::SymEval::expand(aptr, false);

      if (!uop->width && astPair.second && NULL != astPair.first){
        get_width_from_tree(uop, astPair.first);
      }

      aloc = aptr->inputs().at(i).absloc();
      if (aloc.type() == Dyninst::Absloc::Stack || aloc.type() == Dyninst::Absloc::Heap || aloc.type() == Dyninst::Absloc::Unknown) {
        uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_MEMORY, 0);  // Does the index matter in the case of mem?
        insn_data.insn_locVec.push_back(aloc);

        // building the source register information with the right register to represent it
        if (i != 0) { // The memory field is not is first input field. We choose the register field before it to represent mem field.
          Dyninst::MachRegister mr = aptr->inputs().at(i-1).absloc().reg();
          append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, mr, mr.size() * 8, insn_data.arch);
        }
        else { // Using default program counter to represent the memory field.
          Dyninst::MachRegister pc = Dyninst::Absloc::makePC(insn_data.arch).reg();
          append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, pc, pc.size() * 8, insn_data.arch); 
        }
        return;
      }
    }
  // If load already has a src operand (must be memory), ignore all the others memory fields 
  // in the same assignment.
  }
  else if (uop->type == MIAMI::IB_load && uop->num_src_operands) {
    return;
  }
  switch(ast->getID()){
    case Dyninst::AST::V_ConstantAST : {

      Dyninst::DataflowAPI::ConstantAST::Ptr ast_c = Dyninst::DataflowAPI::ConstantAST::convert(ast);
      Dyninst::Absloc aloc_out = aptr->out().absloc();

      // If get_operator function fail to provide a type for IB, then we check whether it
      // is the special case of IB_copy or IB_store. 
      if (uop->type == MIAMI::IB_unknown && aloc_out.type() == Dyninst::Absloc::Register){ // copy from imm to reg
        uop->type = MIAMI::IB_copy;
      } 
      else if (uop->type == MIAMI::IB_unknown && aptr->inputs().size()) { // store into [rip+x]??
         // uop->type = IB_store; CHECK!!!
        uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_REGISTER, 1);

        if (aloc_out.type() == Dyninst::Absloc::Register) {
          append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, aloc_out.reg(), aloc_out.reg().size() * 8, insn_data.arch);  
        } 
        else if (aptr->inputs().at(0).absloc().type() == Dyninst::Absloc::Register){
          Dyninst::MachRegister input = aptr->inputs().at(0).absloc().reg();
          append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, input, input.size() * 8, insn_data.arch);
        }
        return;
      }

      if (uop->width == 0 && uop->vec_len) {
        uop->width = (MIAMI::width_t) ast_c->val().size / uop->vec_len;
      }

      if (uop->num_imm_values < 2) {
        uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; //CHECK
        uop->imm_values[uop->num_imm_values].value.s = ast_c->val().val; 
        // std::cerr << "inside dynXlate_insnectAST, constant value is " << ast_c->val().val;
        uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_IMMED, uop->num_imm_values++); // no need to find dependency of immediates
        insn_data.insn_locVec.push_back(Dyninst::Absloc());
      }
      return;
    }

    case Dyninst::AST::V_VariableAST : {
      Dyninst::DataflowAPI::VariableAST::Ptr ast_var = Dyninst::DataflowAPI::VariableAST::convert(ast);

      // If we do not have a type yet, then update type with either store or copy.
      if (uop->type == MIAMI::IB_unknown) {
         aloc = aptr->out().absloc();
        if (aloc.type() == Dyninst::Absloc::Stack || aloc.type() == Dyninst::Absloc::Heap || aloc.type() == Dyninst::Absloc::Unknown) {
          uop->type = MIAMI::IB_store; // Is it possible? function get_dest_field should already assign it. 
        }
        else if (aloc.type() == Dyninst::Absloc::Register) {
          uop->type = MIAMI::IB_copy;
        }
      } 

      // update width 
      if (uop->width == 0) {
        if (aptr->out().absloc().type() == Dyninst::Absloc::Register) { // update the width according to the output Register operand's size
          if(uop->vec_len) {
            uop->width = aptr->out().absloc().reg().size() * 8 / uop->vec_len;
          }
        } 
        else if (aptr->inputs().size()) { // update the width according to the input Register operand's size
          for (unsigned int i = 0; i < aptr->inputs().size(); ++i) {
            Dyninst::Absloc input_loc = aptr->inputs().at(i).absloc();
            if (input_loc.type() == Dyninst::Absloc::Register) {
              if (uop->vec_len) {
                uop->width = input_loc.reg().size() * 8 / uop->vec_len;
              }
              break;
            }
          }
        } 
        else { // operands only concerning memory
          uop->width = 64; // default 64 for Memory (load / store)--> FIXME
        }
      }

      // Update source operand list when:
      // 1. either the variable is not a flag
      // 2. or if the flag operand has not been listed yet
      if (sourceOpIsFlag(ast_var,insn_data.arch)) {
        if (uop->flag == false) {
          uop->flag = true;
          uop->src_opd[uop->num_src_operands] = make_operand(MIAMI::OperandType_REGISTER, uop->num_src_operands); // flags shall not appear already
          uop->num_src_operands++;
          append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, get_machine_flag_reg(insn_data.arch), 1, insn_data.arch);
        }
      }
      else {
        // Special cases when memory field does not shown in ast 
        if (uop->type == MIAMI::IB_load) {
          for (unsigned int i = 0; i < aptr->inputs().size(); ++i) {
            aloc = aptr->inputs().at(i).absloc();
            if (aloc.type() == Dyninst::Absloc::Stack || aloc.type() == Dyninst::Absloc::Heap || aloc.type() == Dyninst::Absloc::Unknown) {
              uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_MEMORY, 0);  // !!!!!!!!!!!!!!!!!
              insn_data.insn_locVec.push_back(aloc);

              // Building the source register information.
              if (i != 0) {
                 Dyninst::MachRegister mr = aptr->inputs().at(i-1).absloc().reg();
                 append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, mr, mr.size() * 8, insn_data.arch);
              }
              else {
                 Dyninst::MachRegister pc = Dyninst::Absloc::makePC(insn_data.arch).reg();
                 append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, pc, pc.size() * 8, insn_data.arch); 
              }
              return;
            }
          }
        }

        // Update register source operand.
        if (ast_var->val().reg.absloc().type() == Dyninst::Absloc::Register) {
          unsigned int idx = get_register_index(ast_var->val().reg.absloc(),insn_data.insn_locVec);
          uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx); 

          // building the source register information
          int size = 0;
          // 'extract' part of the value. eg: 32 from 64 or 16 from 64.
          if (insn_opVec.size() && insn_opVec.back()->val().op == Dyninst::DataflowAPI::ROSEOperation::extractOp) 
            size = insn_opVec.back()->val().size;
          else 
            size = ast_var->val().reg.absloc().reg().size();

          append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, ast_var->val().reg.absloc().reg(), size,insn_data.arch);
        }
      }
      return;
    }

    case Dyninst::AST::V_RoseAST : {
      Dyninst::DataflowAPI::RoseAST::Ptr ast_r = Dyninst::DataflowAPI::RoseAST::convert(ast);

      if (uop->width == 0)
         get_width_from_tree(uop, ast);
      
      // Depending on different operations, we traverse different choices of their children.
      // Some we just ignore its children (0), some we traverse the first child (1), some 
      // we traverse all of its children (2). 
      int option = traverse_options(uop, ast_r);

      switch(option){
        case 2:
          for (unsigned int i = 0; i < ast->numChildren(); i++) {
            dynXlate_insnectAST(aptr, uop, ast->child(i), insn_opVec, insn_data);
          }
          return;
        case 1:
          dynXlate_insnectAST(aptr, uop, ast->child(0), insn_opVec, insn_data);
          return;
        case 0:
          // special case to deal with sub instruction
          if (ast_r->val().op == Dyninst::DataflowAPI::ROSEOperation::extractOp && ast->child(0)->getID() == Dyninst::AST::V_RoseAST && uop->type != MIAMI::IB_load) { 
              uop->type = MIAMI::IB_unknown;
              dynXlate_insnectAST(aptr, uop, ast->child(0), insn_opVec, insn_data);
          }
            return;
        default:
          return;
      }
      return;
    }

    default: {
      assert("Wrong type of AST node\n");
    }
  }
  return;
}

// Translate test instruction. Make it into special cases for 'simplity'.
// Test instruction is translated into one LogicalOp uop. 
void dynXlate_test(instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back();
  uop->type = MIAMI::IB_logical; 
  if(!uop->vec_len) uop->vec_len = get_vec_len(insn_data.dyn_insn);
  uop->exec_unit = (uop->vec_len == 1) ? MIAMI::ExecUnit_SCALAR : MIAMI::ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn_data.dyn_insn);

  // We take the first assignment and all of them have the same input fields 
  // and different output fields with different flags, and we don't care
  // about the differences. 
  if (insn_data.assignments.at(0)->inputs().at(0).absloc().type() == Dyninst::Absloc::Register) {
    Dyninst::MachRegister reg = insn_data.assignments.at(0)->inputs().at(0).absloc().reg();
    if (!uop->width && uop->vec_len) // get width
      uop->width =  reg.size() * 8 / uop->vec_len; 

    // get souce operands information and the source registers.
    unsigned int idx = get_register_index(insn_data.assignments.at(0)->inputs().at(0).absloc(),insn_data.insn_locVec);
    uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx); 
    append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, reg, reg.size() * 8, insn_data.arch);
    if (insn_data.assignments.at(0)->inputs().size() > 1){
      Dyninst::Absloc aloc = insn_data.assignments.at(0)->inputs().at(1).absloc();
      if (aloc.type() == Dyninst::Absloc::Register)
      {
       uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx+1); 
       append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, aloc.reg(), aloc.reg().size() * 8, insn_data.arch); 

      // If it contains memory field.
      } else if (aloc.type() == Dyninst::Absloc::Heap || aloc.type() == Dyninst::Absloc::Stack || aloc.type() == Dyninst::Absloc::Unknown) 
      {
        uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_MEMORY, idx+1); 
        append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, reg, reg.size() * 8, insn_data.arch); 
      }
    }

    // test instruction always contains flags output region.
    uop->dest_opd[uop->num_dest_operands] = make_operand(MIAMI::OperandType_REGISTER, uop->num_dest_operands);
    uop->num_dest_operands++;
    append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, get_machine_flag_reg(insn_data.arch), 1, insn_data.arch);
  }
}


// Traverse the AST in or instruction to get constant values (if any).
void traverse_or(MIAMI::instruction_info* uop, Dyninst::AST::Ptr ast){
  if (ast->getID() == Dyninst::AST::V_ConstantAST)
  {
    Dyninst::DataflowAPI::ConstantAST::Ptr ast_c = Dyninst::DataflowAPI::ConstantAST::convert(ast);
    uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_IMMED, uop->num_imm_values++);
    uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; //FIXME
    uop->imm_values[uop->num_imm_values].value.s = ast_c->val().val; 
    uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_IMMED, uop->num_imm_values++);
    return;

  } else if(ast->getID() == Dyninst::AST::V_RoseAST) {
    Dyninst::DataflowAPI::RoseAST::Ptr ast_r = Dyninst::DataflowAPI::RoseAST::convert(ast);
    int option = traverse_options(uop, ast_r);

    switch(option){
      case 3:
        traverse_or(uop, ast->child(1));
        return;
      case 2: // traverse all the children
        for (unsigned int i = 0; i < ast->numChildren(); ++i)
        {
          traverse_or(uop, ast->child(i));
        }
        return;
      case 1: // traverse the first child, ignore the others (not necessary ones)
        traverse_or(uop, ast->child(0));
      case 0:
        return;
    }
  }
}

// Useful for the recursion of AST.
// For certain operations, all of its children need to be traversed (2). In other
// cases, maybe only the first child (1) or the second child (3) or even none
// of its children will be traversed.
int traverse_options(MIAMI::instruction_info* uop, Dyninst::DataflowAPI::RoseAST::Ptr ast){
  switch(ast->val().op){
    
    case Dyninst::DataflowAPI::ROSEOperation::invertOp:{
      return 1;
    }
    case Dyninst::DataflowAPI::ROSEOperation::uModOp:
    case Dyninst::DataflowAPI::ROSEOperation::extendOp:
    case Dyninst::DataflowAPI::ROSEOperation::nullOp:
    case Dyninst::DataflowAPI::ROSEOperation::negateOp:
    case Dyninst::DataflowAPI::ROSEOperation::equalToZeroOp:
    case Dyninst::DataflowAPI::ROSEOperation::generateMaskOp:
    case Dyninst::DataflowAPI::ROSEOperation::LSBSetOp:
    case Dyninst::DataflowAPI::ROSEOperation::MSBSetOp:
    case Dyninst::DataflowAPI::ROSEOperation::writeRepOp:
    case Dyninst::DataflowAPI::ROSEOperation::writeOp:
      return 0; 
    case Dyninst::DataflowAPI::ROSEOperation::extractOp:{
        if (uop->type == MIAMI::IB_unknown || uop->type == MIAMI::IB_xor || uop->type == MIAMI::IB_shift ) {
          return 1;
        } else if (uop->num_src_operands == 0) {
          return 1;
        } else {
          return 0;
        }
      }
    case Dyninst::DataflowAPI::ROSEOperation::concatOp:
    case Dyninst::DataflowAPI::ROSEOperation::extendMSBOp:
    case Dyninst::DataflowAPI::ROSEOperation::orOp:
    case Dyninst::DataflowAPI::ROSEOperation::xorOp:
    case Dyninst::DataflowAPI::ROSEOperation::rotateLOp:
    case Dyninst::DataflowAPI::ROSEOperation::rotateROp:
    case Dyninst::DataflowAPI::ROSEOperation::derefOp:
    case Dyninst::DataflowAPI::ROSEOperation::signExtendOp:
      return 1;
    case Dyninst::DataflowAPI::ROSEOperation::addOp:{ // add has two and only two children
      //  transverse the second child if we have 'or' ROSEOperation
      if (uop->type == MIAMI::IB_logical) {
         return 3;
      // transverse both children if the second one is a rose
      } else if (ast->child(1)->getID() == Dyninst::AST::V_RoseAST) 
      {
         return 2;
      } else {
         return 1;
      }
      break;
    }
    case Dyninst::DataflowAPI::ROSEOperation::shiftLOp:
    case Dyninst::DataflowAPI::ROSEOperation::shiftROp:
    case Dyninst::DataflowAPI::ROSEOperation::shiftRArithOp:
    case Dyninst::DataflowAPI::ROSEOperation::andOp:
    case Dyninst::DataflowAPI::ROSEOperation::ifOp:
    case Dyninst::DataflowAPI::ROSEOperation::sMultOp: // signed multiplication
    case Dyninst::DataflowAPI::ROSEOperation::uMultOp:
    case Dyninst::DataflowAPI::ROSEOperation::sDivOp:
    case Dyninst::DataflowAPI::ROSEOperation::sModOp:
    case Dyninst::DataflowAPI::ROSEOperation::uDivOp:
      return 2;
    default:
      fprintf(stderr, "operation %d is invalid.\n", ast->val().op);
      return -1;
  }
}



// Get all the operators from a Dyninst AST, and let get_operator to
// process it later.
void traverse_rose(Dyninst::AST::Ptr ast, std::vector<Dyninst::DataflowAPI::RoseAST::Ptr> & insn_opVec){
  if (ast->getID() == Dyninst::AST::V_RoseAST)
  {
    Dyninst::DataflowAPI::RoseAST::Ptr ast_r = Dyninst::DataflowAPI::RoseAST::convert(ast);
    insn_opVec.push_back(ast_r);
    for (unsigned int i = 0; i < ast->numChildren(); ++i)
    {
      if (ast->child(i)->getID() == Dyninst::AST::V_RoseAST)
      {
        traverse_rose(ast->child(i),insn_opVec);
      }
    }
  }
}

// Translate operations from dyninst representation to MIAMI representation.
// All the ones being commented out are processed in get_operator.
void convert_operator(MIAMI::instruction_info* uop, Dyninst::DataflowAPI::ROSEOperation::Op op) {
  switch(op){
    case Dyninst::DataflowAPI::ROSEOperation::nullOp:
    //case Dyninst::DataflowAPI::ROSEOperation::extractOp:
    //case Dyninst::DataflowAPI::ROSEOperation::invertOp:
    case Dyninst::DataflowAPI::ROSEOperation::negateOp:
    case Dyninst::DataflowAPI::ROSEOperation::generateMaskOp:
    case Dyninst::DataflowAPI::ROSEOperation::LSBSetOp:
    case Dyninst::DataflowAPI::ROSEOperation::MSBSetOp:
    case Dyninst::DataflowAPI::ROSEOperation::extendOp:
    //case Dyninst::DataflowAPI::ROSEOperation::extendMSBOp:
      uop->type = MIAMI::IB_unknown;
      break;
    case Dyninst::DataflowAPI::ROSEOperation::signExtendOp:
      uop->type = MIAMI::IB_cvt_prec;
      break;
    case Dyninst::DataflowAPI::ROSEOperation::uModOp:
    case Dyninst::DataflowAPI::ROSEOperation::sModOp:
      uop->type = MIAMI::IB_fn;
      break;
    case Dyninst::DataflowAPI::ROSEOperation::sMultOp: // signed multiplication
    case Dyninst::DataflowAPI::ROSEOperation::uMultOp:
      uop->type = MIAMI::IB_mult;
      uop->primary = true;
      break;
    case Dyninst::DataflowAPI::ROSEOperation::sDivOp:
    case Dyninst::DataflowAPI::ROSEOperation::uDivOp:
      uop->type = MIAMI::IB_div;
      uop->primary = true;
      break;
    //case Dyninst::DataflowAPI::ROSEOperation::concatOp:{
      //uop->type = IB_copy;
      //uop->primary = true;
      //break;
    //}
    case Dyninst::DataflowAPI::ROSEOperation::equalToZeroOp: //??compare?? is it logical
    case Dyninst::DataflowAPI::ROSEOperation::andOp:
    case Dyninst::DataflowAPI::ROSEOperation::orOp:
    case Dyninst::DataflowAPI::ROSEOperation::xorOp: //IB_xor
      uop->type = MIAMI::IB_logical;
      break;
    case Dyninst::DataflowAPI::ROSEOperation::rotateLOp:
    case Dyninst::DataflowAPI::ROSEOperation::rotateROp:
      uop->type = MIAMI::IB_rotate;
      break;
    case Dyninst::DataflowAPI::ROSEOperation::shiftLOp:
    case Dyninst::DataflowAPI::ROSEOperation::shiftROp:
    case Dyninst::DataflowAPI::ROSEOperation::shiftRArithOp:
      uop->type = MIAMI::IB_shift;
      break;
    case Dyninst::DataflowAPI::ROSEOperation::addOp:
      uop->type = MIAMI::IB_add;
      break;
    
    //case Dyninst::DataflowAPI::ROSEOperation::derefOp:{
    //  uop->type = MIAMI::IB_load;
    //  uop->primary = true;
    //  break;
    //}
    case Dyninst::DataflowAPI::ROSEOperation::writeRepOp:
    case Dyninst::DataflowAPI::ROSEOperation::writeOp:
      break;
    //case Dyninst::DataflowAPI::ROSEOperation::ifOp:{
    //  uop->type = MIAMI::IB_br_CC;
    //  uop->primary = true;
    //  break;
    //}
  
    default:{break;}
      //assert(!"get_operator\n");
  }
}

// Get the right operator from the AST (if exists) of an assignment. Stack all the operators in a vector and 
// choosing the operator according to certain rules.
void get_operator(MIAMI::instruction_info* uop, Dyninst::AST::Ptr ast, Dyninst::Assignment::Ptr aptr,std::vector<Dyninst::DataflowAPI::RoseAST::Ptr> & insn_opVec, instruction_data & insn_data){
  
  //if (uop->type != IB_unknown) return;
  
  traverse_rose(ast,insn_opVec);
  if (insn_opVec.size() == 0) 
    return;

  for (auto it = insn_opVec.begin(); it != insn_opVec.end(); it++) {
    Dyninst::DataflowAPI::ROSEOperation::Op op = (*it)->val().op;

    if (op == Dyninst::DataflowAPI::ROSEOperation::ifOp) {
      uop->type = MIAMI::IB_br_CC;
      break;

    } else if (op == Dyninst::DataflowAPI::ROSEOperation::extractOp) // Usually extract is not the important operation. (Also, no corresponding IB.)
      continue;

    else if (op == Dyninst::DataflowAPI::ROSEOperation::extendMSBOp) // As above.
      continue;

    else if (op == Dyninst::DataflowAPI::ROSEOperation::invertOp) {

      if (uop->type == MIAMI::IB_add){
        uop->type = MIAMI::IB_sub;
        break;    
      } else {
        continue;
      }
    
    } else if (op == Dyninst::DataflowAPI::ROSEOperation::xorOp)
    {
      uop->type = MIAMI::IB_xor;
      continue; // It can be sub, because we might find invert later, so continue searching.

    } else if (op == Dyninst::DataflowAPI::ROSEOperation::concatOp)
    {
      uop->type = MIAMI::IB_copy;
      continue; // It might be sub, add, xor. Keep searching.

    } else if (op == Dyninst::DataflowAPI::ROSEOperation::derefOp)
    {
      if (uop->type != MIAMI::IB_add){ // A lot of special cases!
         uop->type = MIAMI::IB_load;
         break;
      }

    } else {
      convert_operator(uop, op); // Direct translate other cases and assess them later.
      continue;
    }

  }
}

// Traversing through AST of the jump assignment and calculate the value of its constant operand.
int calculate_jump_val(Dyninst::AST::Ptr ast,int insn_jump_tgt,int & insn_jump_tgt2 ){
  if (ast->getID() == Dyninst::AST::V_RoseAST) {
    Dyninst::DataflowAPI::RoseAST::Ptr ast_r = Dyninst::DataflowAPI::RoseAST::convert(ast);
    if (ast_r->val().op == Dyninst::DataflowAPI::ROSEOperation::addOp) {

      for (unsigned int i = 0; i < ast->numChildren(); ++i) {
        if (ast->child(i)->getID() == Dyninst::AST::V_ConstantAST) {
          Dyninst::DataflowAPI::ConstantAST::Ptr ast_c = Dyninst::DataflowAPI::ConstantAST::convert(ast->child(i));
          insn_jump_tgt += ast_c->val().val;
          insn_jump_tgt2 += ast_c->val().val;
      
        }
    else if (ast->child(i)->getID() == Dyninst::AST::V_VariableAST){
          Dyninst::DataflowAPI::VariableAST::Ptr ast_v = Dyninst::DataflowAPI::VariableAST::convert(ast->child(i));
          insn_jump_tgt += ast_v->val().addr; // CHECK --> not all variables are rip
          insn_jump_tgt2 += ast_v->val().addr;
        }
    else {
          insn_jump_tgt += calculate_jump_val(ast->child(i),insn_jump_tgt,insn_jump_tgt2);
        }
      }
    }
    else if (ast_r->val().op == Dyninst::DataflowAPI::ROSEOperation::ifOp) {
      insn_jump_tgt += calculate_jump_val(ast->child(1),insn_jump_tgt,insn_jump_tgt2); // need to CHECK

    }
    else {
      //return;
      //assert(!"Must be add operation for address calculation\n");
    }
  }
  return insn_jump_tgt;
}


unsigned int get_register_index(Dyninst::Absloc a, std::vector<Dyninst::Absloc>& insn_locVec){
  
  for (unsigned int i = 0; i < insn_locVec.size(); ++i) {
    if (insn_locVec.at(i).format().compare(a.format()) == 0){
      return i;
    }
  }
  insn_locVec.push_back(a);
  return insn_locVec.size() - 1;
}

int get_src_reg_lsb(Dyninst::MachRegister mr) { 
   return 0;
}

// If x86 or x86_64, return the representation of all the flags.
Dyninst::MachRegister get_machine_flag_reg(Dyninst::Architecture arch){
  //if (NULL == insn_myBlock)
  //  return Dyninst::MachRegister();
  
  if (arch == Dyninst::Architecture::Arch_x86_64)
    return Dyninst::x86_64::flags;  

  else if (arch == Dyninst::Architecture::Arch_x86)
    return Dyninst::x86::flags; 

  else 
    return Dyninst::MachRegister();
}


// Return if the variable ast is a flag register
bool sourceOpIsFlag(Dyninst::DataflowAPI::VariableAST::Ptr ast_var, Dyninst::Architecture arch){
  Dyninst::Absloc aloc = ast_var->val().reg.absloc();
  if (aloc.type() == Dyninst::Absloc::Register) {
    Dyninst::InstructionAPI::RegisterAST reg = Dyninst::InstructionAPI::RegisterAST(aloc.reg());
    Dyninst::MachRegister mr = reg.getID().getBaseRegister();
    return check_flags_registers(mr,arch);
  }
  return false;
}

bool destOpIsFlag(Dyninst::Assignment::Ptr aptr, Dyninst::Architecture arch){
  Dyninst::Absloc aloc = aptr->out().absloc();
  if (aloc.type() == Dyninst::Absloc::Register) {
    Dyninst::InstructionAPI::RegisterAST reg = Dyninst::InstructionAPI::RegisterAST(aloc.reg());
    Dyninst::MachRegister mr = reg.getID().getBaseRegister();
    return check_flags_registers(mr,arch);
  }
  return false;
}

// Update the primary micro-op with a new destination operand--flags
void update_dest_with_flag(instruction_data & insn_data){
  for (auto oit = insn_data.dInst->micro_ops.begin(); oit != insn_data.dInst->micro_ops.end(); oit++) {
    if (oit->primary) {
      oit->dest_opd[oit->num_dest_operands++] = make_operand(MIAMI::OperandType_REGISTER, oit->num_dest_operands); // which index should I use
      append_dest_regs(&(*oit), MIAMI::OperandType_REGISTER, insn_data.dyn_insn, get_machine_flag_reg(insn_data.arch), 1, insn_data.arch);
    }
  }
}

// Check whether a machine register is flag in x86 or x86_64
bool check_flags_registers(Dyninst::MachRegister mr, Dyninst::Architecture arch){
  if (arch == Dyninst::Architecture::Arch_x86_64){
    if (mr == Dyninst::x86_64::cf 
    || mr == Dyninst::x86_64::pf || mr == Dyninst::x86_64::af || mr == Dyninst::x86_64::zf
    || mr == Dyninst::x86_64::sf || mr == Dyninst::x86_64::tf || mr == Dyninst::x86_64::df
    || mr == Dyninst::x86_64::of
    || mr == Dyninst::x86_64::nt_ || mr == Dyninst::x86_64::if_ || mr == Dyninst::x86_64::flags) {
      return true; 
    }
  }
  else if (arch == Dyninst::Architecture::Arch_x86){
    if (mr == Dyninst::x86::cf || mr == Dyninst::x86::pf || mr == Dyninst::x86::af || mr == Dyninst::x86::zf
        || mr == Dyninst::x86::sf || mr == Dyninst::x86::tf || mr == Dyninst::x86::df || mr == Dyninst::x86::of
        || mr == Dyninst::x86::nt_ || mr == Dyninst::x86::if_ || mr == Dyninst::x86::flags) {
      return true; 
    }
  } 
  return false;
}


// Return if a register is a 'stack' register (used for stack)
int check_stack_register(Dyninst::MachRegister mr, Dyninst::Architecture arch) {
  if (arch == Dyninst::Architecture::Arch_x86_64)
  { 
    if ((Dyninst::x86_64::st0 <= mr && mr <= Dyninst::x86_64::st7) || mr == Dyninst::x86_64::mm0) {
      return 1;
   } 
  } else if (arch == Dyninst::Architecture::Arch_x86){
    if (mr == Dyninst::x86::mm0)
    {
      return 1;
    }
  }
    

   return 0;
}

// If the output (which usually determine the vector length) is vector register operand, 
// then return its vector length. Otherwise, return vector length as 1.
// The following list is not complete and havent' include cases in other architectures.
int check_vector_regs(Dyninst::InstructionAPI::Instruction::Ptr insn, Dyninst::Assignment::Ptr aptr, Dyninst::Architecture arch){
  if (aptr->out().absloc().type() == Dyninst::Absloc::Register) 
  {
    Dyninst::InstructionAPI::RegisterAST reg = Dyninst::InstructionAPI::RegisterAST(aptr->out().absloc().reg());
    Dyninst::MachRegister mr = reg.getID().getBaseRegister();
    if (arch == Dyninst::Architecture::Arch_x86)
    {
      if (mr == Dyninst::x86::xmm0 || mr == Dyninst::x86::xmm1 || mr == Dyninst::x86::xmm2 || mr == Dyninst::x86::xmm3 \
      || mr == Dyninst::x86::xmm4 || mr == Dyninst::x86::xmm5 || mr == Dyninst::x86::xmm6 || mr == Dyninst::x86::xmm7)
        return 4;

    } else if (arch == Dyninst::Architecture::Arch_x86_64)
    {
      if ( mr == Dyninst::x86_64::xmm0 || mr == Dyninst::x86_64::xmm1 || mr == Dyninst::x86_64::xmm2 || mr == Dyninst::x86_64::xmm3 \
      || mr == Dyninst::x86_64::xmm4 || mr == Dyninst::x86_64::xmm5 || mr == Dyninst::x86_64::xmm6 || mr == Dyninst::x86_64::xmm7 ) 
        return 4;  
    }
  }
  return 0;
}

// Append source registers to the src_reg_list. 
// Register information not used right now. If we want to build dependency graph,
// we simply just pass these to internal field over instead use MIAMI's original function.
void append_src_regs(MIAMI::instruction_info* uop, MIAMI::OperandType opt, Dyninst::InstructionAPI::Instruction::Ptr insn, Dyninst::MachRegister mr, int size, Dyninst::Architecture arch) {
   int s = 0; // stack
   switch(opt){
      case MIAMI::OperandType_REGISTER:{
         MIAMI::RegisterClass rclass = MIAMI::RegisterClass_REG_OP;
         if (check_stack_register(mr,arch)) {
            rclass = MIAMI::RegisterClass_STACK_REG;
            s = 1;
         } 
         uop->src_reg_list.push_back(MIAMI::register_info(mr, rclass, get_src_reg_lsb(mr), size - 1, s));
         return;
      }
      case MIAMI::OperandType_INTERNAL:{
         uop->src_reg_list.push_back(MIAMI::register_info(mr, MIAMI::RegisterClass_TEMP_REG, get_src_reg_lsb(mr), size - 1));
         return;
      }
      case MIAMI::OperandType_MEMORY: {
         MIAMI::RegisterClass rclass = MIAMI::RegisterClass_MEM_OP;
         if (insn->getOperation().getID() == e_lea)
         {
            rclass = MIAMI::RegisterClass_LEA_OP;
         }  else if (check_stack_register(mr,arch))
         {
            rclass = MIAMI::RegisterClass_STACK_REG;
            s = 1;
         }
         uop->src_reg_list.push_back(MIAMI::register_info(mr, rclass, get_src_reg_lsb(mr), size - 1, s));
         return;
      }
      default: {
        assert("Not possible!\n");
        return;
      }
   }
}

// Append destination registers to the dest_reg_list. 
// Register information not used right now. If we want to build dependency graph,
// we simply just pass these to internal field over instead use MIAMI's original function.
void append_dest_regs(MIAMI::instruction_info* uop, MIAMI::OperandType opt, Dyninst::InstructionAPI::Instruction::Ptr insn, Dyninst::MachRegister mr, int size, Dyninst::Architecture arch){
   int s = 0;
   switch(opt){
      case MIAMI::OperandType_REGISTER:{
        MIAMI::RegisterClass rclass = MIAMI::RegisterClass_REG_OP;
        if (check_stack_register(mr,arch)) {
          rclass = MIAMI::RegisterClass_STACK_REG;
          s = 1;
        } 
        uop->dest_reg_list.push_back(MIAMI::register_info(mr, rclass, get_src_reg_lsb(mr), size - 1, s)); // fix me-->need to add in
                                                  
        return;
      }
      case MIAMI::OperandType_INTERNAL:{
        uop->dest_reg_list.push_back(MIAMI::register_info(mr, MIAMI::RegisterClass_TEMP_REG, get_src_reg_lsb(mr), size - 1));
        return;
      }
      case MIAMI::OperandType_MEMORY: {
        assert("Not allowed, memory register information is passed to source register list.\n");
        return;
      }
      default: {
        assert("Not possible.\n");
        return;
      }
   }
}

// Append all the destination registers 
void append_all_dest_registers(MIAMI::instruction_info* uop, Dyninst::InstructionAPI::Instruction::Ptr insn, Dyninst::Assignment::Ptr aptr, Dyninst::Architecture arch){
   for (unsigned int i = 0; i < uop->num_dest_operands; ++i)
   {
      switch(extract_op_type(uop->dest_opd[i])){ // extract the operation type.

         case MIAMI::OperandType_INTERNAL: 
            uop->dest_reg_list.clear();
            append_dest_regs(uop, MIAMI::OperandType_INTERNAL, insn, Dyninst::MachRegister(), uop->width * uop->vec_len, arch);
            break;
         case MIAMI::OperandType_REGISTER: {
            if (i == 0) // This is not flags, we will append flag later 
            {
              if (aptr->out().absloc().type() == Dyninst::Absloc::Register){
                append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn, aptr->out().absloc().reg(), uop->width * uop->vec_len, arch); 
              }else {
                Dyninst::MachRegister pc = Dyninst::Absloc::makePC(arch).reg();
                append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn, pc, pc.size() * 8, arch);  
              }  
            }
            break;
         }
         case MIAMI::OperandType_MEMORY: {
            if (aptr->inputs().size())
            {
               Dyninst::Absloc aloc = aptr->inputs().at(0).absloc(); // I actually don't know how to find the corresponding register 
                                                            // related to the memory field. The xed function related to this 
                                                            // is xed_decoded_inst_get_?*?_reg.
               if (aloc.type() == Dyninst::Absloc::Register)
                  append_src_regs(uop, MIAMI::OperandType_MEMORY, insn, aloc.reg(), uop->width * uop->vec_len, arch);  
               else                                         // Find no register field, use default program counter
                  append_src_regs(uop, MIAMI::OperandType_MEMORY, insn, Dyninst::Absloc::makePC(arch).reg(), uop->width * uop->vec_len, arch);
            }
            break;
         }
         default: {
            assert("No other type of operands.\n");
            break;
         } 
      }
  }
}

// Get the width of a uop from AST
void get_width_from_tree(MIAMI::instruction_info* uop, Dyninst::AST::Ptr ast){

   if (uop->width)
      return;
   
   switch(ast->getID()){

      case Dyninst::AST::V_RoseAST:{ // if node is an operation, only special ones can give the size. Normally, we keep recursing.
         Dyninst::DataflowAPI::RoseAST::Ptr ast_r = Dyninst::DataflowAPI::RoseAST::convert(ast);
         if (ast_r->val().op == Dyninst::DataflowAPI::ROSEOperation::derefOp || (ast_r->val().op == Dyninst::DataflowAPI::ROSEOperation::extractOp && ast_r->val().size != 1))
         {
            uop->width = ast_r->val().size;
         } else {
            for (unsigned int i = 0; i < ast->numChildren(); i++) {
              get_width_from_tree(uop, ast->child(i));
            }
         }
         return;
      } 

      case Dyninst::AST::V_ConstantAST:{ // get the size of the constant
         Dyninst::DataflowAPI::ConstantAST::Ptr ast_c = Dyninst::DataflowAPI::ConstantAST::convert(ast);
         uop->width = ast_c->val().size;
         return;
      }

      case Dyninst::AST::V_VariableAST:{ // get the size of the register
         Dyninst::DataflowAPI::VariableAST::Ptr ast_v = Dyninst::DataflowAPI::VariableAST::convert(ast);
         if (ast_v->val().reg.absloc().type() == Dyninst::Absloc::Register)
         {
            uop->width = ast_v->val().reg.absloc().reg().size() * 8;
         }
         return;
      }

      default: { assert("Not possible in this context.\n"); }
   }
}

// Create add micro that appear together with other micros in functions like call or pop
// We set the default value as 8 to indicate the increment of the stack pointer (?rsp).
//void create_add_micro(MIAMI::DecodedInstruction* dInst, Dyninst::InstructionAPI::Instruction::Ptr insn, Dyninst::Assignment::Ptr aptr,
//    std::vector<Dyninst::Absloc>& insn_locVec, Dyninst::Architecture arch){
void create_add_micro(Dyninst::Assignment::Ptr aptr, instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back();
  uop->type = MIAMI::IB_add;
  uop->width = (insn_data.dInst->micro_ops.begin())->width;
  uop->vec_len = (insn_data.dInst->micro_ops.begin())->vec_len;
  uop->exec_unit = (insn_data.dInst->micro_ops.begin())->exec_unit;
  uop->exec_unit_type = (insn_data.dInst->micro_ops.begin())->exec_unit_type;
  int idx = 0;
  if (aptr->out().absloc().type() == Dyninst::Absloc::Register){
    idx = get_register_index(aptr->out().absloc(),insn_data.insn_locVec);
    Dyninst::MachRegister mr = aptr->out().absloc().reg();
    uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx);
    append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, mr, mr.size() * 8,insn_data.arch); 
    uop->dest_opd[uop->num_dest_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx); // -->using the index of the first Absloc::Register
    append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, mr, mr.size() * 8,insn_data.arch); 
 
  }
  
  uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; 
  uop->imm_values[uop->num_imm_values].value.s = 8; // 
  uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_IMMED, uop->num_imm_values++);
}

// Create load micro operation.
//void create_load_micro(MIAMI::DecodedInstruction* dInst, Dyninst::InstructionAPI::Instruction::Ptr insn, Dyninst::Assignment::Ptr aptr, int idx,
//    std::vector<Dyninst::Absloc>& insn_locVec, Dyninst::Architecture arch)
void create_load_micro(Dyninst::Assignment::Ptr aptr, int idx, instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_front(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.front();
  uop->type = MIAMI::IB_load;

  std::pair<Dyninst::AST::Ptr, bool> astPair;
  astPair = Dyninst::DataflowAPI::SymEval::expand(aptr, false);
  // getting the width 
  if (astPair.second && NULL != astPair.first){
     get_width_from_tree(uop, astPair.first); // get width from AST

  } else {
    // if no AST --> get width from assignment's output or input register field
    if (aptr->out().absloc().type() == Dyninst::Absloc::Register) {
      uop->width = aptr->out().absloc().reg().size() * 8;
    } else if (aptr->inputs().size()) {

      for (unsigned int i = 0; i < aptr->inputs().size(); ++i)
      {
         if (aptr->inputs().at(i).absloc().type() == Dyninst::Absloc::Register) { 
            uop->width = aptr->inputs().at(i).absloc().reg().size() * 8;
            break;
         }
      }
    }
  } 
   
  if(!uop->vec_len){
    uop->vec_len = get_vec_len(insn_data.dyn_insn);
  }
  uop->exec_unit = (uop->vec_len == 1) ? MIAMI::ExecUnit_SCALAR : MIAMI::ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn_data.dyn_insn);

  // load uop's source must be memory field (idx here not important)
  uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_MEMORY, 0); 

  if (insn_data.assignments.size() > 1 || insn_data.dInst->micro_ops.size() > 1) // Store is not the primary operation, then it load into internal
    uop->dest_opd[uop-> num_dest_operands++] = make_operand(MIAMI::OperandType_INTERNAL, idx);  
  else // Load into register
    uop->dest_opd[uop-> num_dest_operands++] = make_operand(MIAMI::OperandType_REGISTER, 0);
  
  // Like memtioned above, I actually don't know how to find the corresponding register 
  // related to the memory field. The xed function related to this is xed_decoded_inst_get_?*?_reg.
  // Usually, chances are the first/second register in the inputs field represents the memory. 
  // If cannot find any not, use the default choice--program counter.
  Dyninst::MachRegister pc = Dyninst::Absloc::makePC(insn_data.arch).reg();
  if (aptr->inputs().size() && aptr->inputs().at(0).absloc().type() == Dyninst::Absloc::Register) 
  {
     Dyninst::MachRegister mr = aptr->inputs().at(0).absloc().reg();
     append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, mr, mr.size() * 8,insn_data.arch);
  } else if (aptr->inputs().size() > 1 && aptr->inputs().at(1).absloc().type() == Dyninst::Absloc::Register) // CHECK OTHER CASES
  {
     Dyninst::MachRegister mr = aptr->inputs().at(1).absloc().reg();
     append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, mr, mr.size() * 8,insn_data.arch);
  } else 
  {
     append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, pc, pc.size() * 8,insn_data.arch); 
  }

  if (insn_data.assignments.size() > 1 || insn_data.dInst->micro_ops.size() > 1) // Load info into internal
    append_dest_regs(uop, MIAMI::OperandType_INTERNAL, insn_data.dyn_insn, Dyninst::MachRegister(), uop->width * uop->vec_len,insn_data.arch);
  else { 
    // special case,usually with st0 which comes from parse_assign. We will append registers later
  }   
}

// If output field is memory, we create a store uop (Dyninst AST usually shows no information of load/store).
void create_store_micro(Dyninst::Assignment::Ptr aptr, int idx, instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back();
  uop->type = MIAMI::IB_store;
  std::pair<Dyninst::AST::Ptr, bool> astPair;
   astPair = Dyninst::DataflowAPI::SymEval::expand(aptr, false);

  if (astPair.second && NULL != astPair.first)
     get_width_from_tree(uop, astPair.first);
  else 
     uop->width = insn_data.dInst->micro_ops.front().width;
  
  uop->vec_len = get_vec_len(insn_data.dyn_insn);
  uop->exec_unit =  (uop->vec_len == 1) ? MIAMI::ExecUnit_SCALAR : MIAMI::ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn_data.dyn_insn);

  // If the instruction has more than one assignment, the store/load usually interact with temporary internal operand.
  uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_INTERNAL, idx); 
  append_src_regs(uop, MIAMI::OperandType_INTERNAL, insn_data.dyn_insn, Dyninst::MachRegister(), 64, insn_data.arch); // Does the size for internal field matter?

  uop->dest_opd[uop->num_dest_operands++] = make_operand(MIAMI::OperandType_MEMORY, 0); // FIXME--index
  for (unsigned int i = 0; i < aptr->inputs().size(); ++i)
  {
    Dyninst::Absloc aloc = aptr->inputs().at(i).absloc();
     if (aloc.type() == Dyninst::Absloc::Register && aptr->inputs().size() > 1)
     {
        append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, aloc.reg(), uop->width * uop->vec_len, insn_data.arch); // CHECK!!!! Not the previous one?
        return;
     }
  }
  // If no register information found for the memory field, use default choice PC register --> check if this is the case
  append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, Dyninst::Absloc::makePC(insn_data.arch).reg(), 64, insn_data.arch);
  return;
}

// Create jump micro.
void create_jump_micro(int jump_val, Dyninst::Assignment::Ptr assign, bool mem, instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back();
  uop->type = MIAMI::IB_jump;
  uop->primary = true; // whether in cond/uncond branch or call, jump is always the primary uop
  uop->width = (insn_data.dInst->micro_ops.begin())->width;
  uop->vec_len = (insn_data.dInst->micro_ops.begin())->vec_len;
  uop->exec_unit = (insn_data.dInst->micro_ops.begin())->exec_unit;
  uop->exec_unit_type = (insn_data.dInst->micro_ops.begin())->exec_unit_type;

  unsigned int idx = 0;
  if (assign->inputs().at(0).absloc().type() == Dyninst::Absloc::Register){
    idx = get_register_index(assign->inputs().at(0).absloc(), insn_data.insn_locVec); // use the index of register used in store
  }

  Dyninst::MachRegister pc = Dyninst::Absloc::makePC(insn_data.arch).reg();
  if (mem) { // If we have a load uop, then the source of jump comes from INTERNAL operand.
    uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_INTERNAL, 0);
    append_src_regs(uop, MIAMI::OperandType_INTERNAL, insn_data.dyn_insn, pc, pc.size() * 8, insn_data.arch);

  } else { // If we only have a jump uop, then the source operand is register.
    uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx);
    append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, pc, pc.size() * 8, insn_data.arch);
    uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; 
    uop->imm_values[uop->num_imm_values].value.s = jump_val; 
    uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_IMMED, uop->num_imm_values++);
  } 

  uop->dest_opd[uop->num_dest_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx);
  append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, pc, pc.size() * 8, insn_data.arch); 
}

// Create Return micro for instruction return
void create_return_micro(Dyninst::Assignment::Ptr aptr, instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back();
  uop->type = MIAMI::IB_ret;
  uop->primary = true;

  unsigned int idx = 0;
  if (aptr->out().absloc().type() == Dyninst::Absloc::Register){
    idx = get_register_index(aptr->out().absloc(),insn_data.insn_locVec);
  }

  // everything corresponds to the first uop IB_load in return case. (?)
  uop->width = (insn_data.dInst->micro_ops.begin())->width;
  uop->vec_len = (insn_data.dInst->micro_ops.begin())->vec_len;
  uop->exec_unit = (insn_data.dInst->micro_ops.begin())->exec_unit;
  uop->exec_unit_type = (insn_data.dInst->micro_ops.begin())->exec_unit_type;
  uop->dest_opd[uop->num_dest_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx); 

  // no source fields needed for return uop
  if (aptr->out().absloc().type() == Dyninst::Absloc::Register) {
     append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, aptr->out().absloc().reg(), uop->width * uop->vec_len, insn_data.arch);
  }
} 

// Create compare micro.
void create_compare_micro(instruction_data & insn_data){
  insn_data.dInst->micro_ops.push_back(MIAMI::instruction_info());
  MIAMI::instruction_info* uop = &insn_data.dInst->micro_ops.back();
  uop->type = MIAMI::IB_cmp;
  uop->primary = true;

  if(!uop->vec_len) uop->vec_len = get_vec_len(insn_data.dyn_insn);
  uop->exec_unit = (uop->vec_len == 1) ? MIAMI::ExecUnit_SCALAR : MIAMI::ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn_data.dyn_insn);

  for (unsigned int i = 0; i < insn_data.assignments.at(0)->inputs().size(); i++) { 
    Dyninst::Absloc aloc = insn_data.assignments.at(0)->inputs().at(i).absloc();

    if (aloc.type() == Dyninst::Absloc::Register){ // append all the register source operand
      if (uop->width == 0 && uop->vec_len) {

        if (check_is_zf(aloc.reg(),insn_data.arch))  // In Dyninst, only the assignment with zf register as the output field
                                      // shows the width in its AST. (in compare cases)
        {
          std::pair<Dyninst::AST::Ptr, bool> astPair;
          astPair = Dyninst::DataflowAPI::SymEval::expand(insn_data.assignments.at(i), false);

          if (astPair.second && NULL != astPair.first)
            get_width_from_tree(uop, astPair.first);
        }

        uop->width = aloc.reg().size() * 8 / uop->vec_len;
      }

      unsigned int idx = get_register_index(aloc,insn_data.insn_locVec);
      uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx);
      append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, aloc.reg(), aloc.reg().size() * 8, insn_data.arch);
    } 
  }

  // If we have another uop (load), the compare's source operand changes to INTERNAL
  // Replace REGISTER operand by INTERNAL operand.
  if (insn_data.dInst->micro_ops.size() > 1) {
    uop->src_opd[0] = make_operand(MIAMI::OperandType_INTERNAL, 0); 
    uop->src_reg_list.clear();
    append_src_regs(uop, MIAMI::OperandType_INTERNAL, insn_data.dyn_insn, Dyninst::MachRegister(), 64, insn_data.arch);
  }

  // We append constant values from the operands (if any).
  std::vector<Dyninst::InstructionAPI::Operand> operands;
  insn_data.dyn_insn->getOperands(operands);
  for (unsigned int i = 0; i < operands.size(); ++i) {
    const Dyninst::InstructionAPI::Result *r = &operands.at(i).getValue()->eval();

    if (r->defined) {
      uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; 
      uop->imm_values[uop->num_imm_values].value.s = r->convert<long int>();
      uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_IMMED, uop->num_imm_values++);

    } else { // If the operand is not a constant value, we get the width value from it (if we need it).

      if(!uop->width && uop->vec_len) uop->width = r->size() * 8 / uop->vec_len;
    }

  }
  uop->dest_opd[uop->num_dest_operands++] = make_operand(MIAMI::OperandType_REGISTER, 2); // indexing of flag register
  append_dest_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, get_machine_flag_reg(insn_data.arch), 1, insn_data.arch);
}

// For x86 or x86_64, check whether a register is zf flag register.
bool check_is_zf(Dyninst::MachRegister mr, Dyninst::Architecture arch) {
  if (arch == Dyninst::Architecture::Arch_x86) 
  {
    if (mr == Dyninst::x86::zf)
      return true;
    
  } else if (arch == Dyninst::Architecture::Arch_x86_64)
  {
    if (mr == Dyninst::x86_64::zf)
      return true;
    
  }
  return false;
}

// Check operands size to see whether we are dealing with vector operand
int get_vec_len(Dyninst::InstructionAPI::Instruction::Ptr insn){
  std::vector<Dyninst::InstructionAPI::Operand> operands;
  insn->getOperands(operands);
  for (unsigned int i = 0; i < operands.size(); ++i) {
    const Dyninst::InstructionAPI::Result* r = &operands.at(i).getValue()->eval();
    if (r->size() > 8) {
      return (int) r->size() / 8;
    }
  }
  return 1;
}

// Get the execution type of an instruction, depending on its operands' type.
MIAMI::ExecUnitType get_execution_type(Dyninst::InstructionAPI::Instruction::Ptr insn){
  std::vector<Dyninst::InstructionAPI::Operand> operands;
  insn->getOperands(operands);
  for (unsigned int i = 0; i < operands.size(); ++i) {
    const Dyninst::InstructionAPI::Result* r = &operands.at(i).getValue()->eval();
    //if (r->defined) { // don't need the result to be defined to get the type
      switch(r->type){
        case Dyninst::InstructionAPI::bit_flag:
        case Dyninst::InstructionAPI::s8:
        case Dyninst::InstructionAPI::u8:
        case Dyninst::InstructionAPI::s16:
        case Dyninst::InstructionAPI::u16:
        case Dyninst::InstructionAPI::s32:
        case Dyninst::InstructionAPI::u32:
        case Dyninst::InstructionAPI::s48:
        case Dyninst::InstructionAPI::u48:
        case Dyninst::InstructionAPI::s64:
        case Dyninst::InstructionAPI::u64:
        // 48-bit pointers
        case Dyninst::InstructionAPI::m512:
        case Dyninst::InstructionAPI::m14:
          return MIAMI::ExecUnitType_INT;
        case Dyninst::InstructionAPI::dbl128:
        case Dyninst::InstructionAPI::sp_float:
        case Dyninst::InstructionAPI::dp_float:
        default:
          return MIAMI::ExecUnitType_FP;
      }
    //}
  }
  return MIAMI::ExecUnitType_INT; // FIXME 
}

// Processing the output region of a given assignment. 
void get_dest_field(Dyninst::Assignment::Ptr aptr, MIAMI::instruction_info* uop, std::vector<Dyninst::Absloc>& insn_locVec){
  Dyninst::Absloc out_aloc = aptr->out().absloc();
  if (out_aloc.type() == Dyninst::Absloc::Register) {
    unsigned int idx = get_register_index(out_aloc,insn_locVec);
    uop->dest_opd[uop->num_dest_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx);

  } else if (out_aloc.type() == Dyninst::Absloc::Stack || out_aloc.type() == Dyninst::Absloc::Heap || out_aloc.type() == Dyninst::Absloc::Unknown) {
    uop->dest_opd[uop->num_dest_operands++] = make_operand(MIAMI::OperandType_MEMORY, 0); // index for memory FIXME
    uop->type = MIAMI::IB_store;
  }
  insn_locVec.push_back(out_aloc);
}

// If assignment has no AST, parse it according to it's input and output field --> multiple cases untranslatable. 
//void parse_assign(MIAMI::DecodedInstruction* dInst, Dyninst::Assignment::Ptr aptr, MIAMI::instruction_info* uop, Dyninst::InstructionAPI::Instruction::Ptr insn, 
//    std::vector<Dyninst::Absloc>& insn_locVec, Dyninst::Architecture arch)
void parse_assign(Dyninst::Assignment::Ptr aptr, MIAMI::instruction_info* uop, instruction_data & insn_data){
  
  std::cerr << "parse_assign\n";

  if (uop->vec_len == 0) 
    uop->vec_len = get_vec_len(insn_data.dyn_insn);
  
  if (uop->exec_unit == MIAMI::ExecUnit_LAST) 
    uop->exec_unit = (uop->vec_len == 1) ? MIAMI::ExecUnit_SCALAR : MIAMI::ExecUnit_VECTOR;
  
  if (uop->exec_unit_type == MIAMI::ExecUnitType_LAST) 
    uop->exec_unit_type = get_execution_type(insn_data.dyn_insn);
  
  if (uop->width == 0 && uop->vec_len) {

    if (aptr->out().absloc().type() == Dyninst::Absloc::Register) {
      uop->width = aptr->out().absloc().reg().size() * 8 / uop->vec_len;
    } else {
      if (aptr->inputs().size() && aptr->inputs().at(0).absloc().type() == Dyninst::Absloc::Register) {
        uop->width = aptr->inputs().at(0).absloc().reg().size() * 8 / uop->vec_len;
      } else {
        uop->width = 64; // no width information available from input/output field, use default value for now
      }
    }
  }

  // start GUESSING what is the operation with limited information--inputs and output fields.
  Dyninst::Absloc aloc_out = aptr->out().absloc();
  std::vector<Dyninst::AbsRegion> inputs = aptr->inputs();

  if (uop->type == MIAMI::IB_unknown) {

    if (aloc_out.type() == Dyninst::Absloc::Register && inputs.size()) {
      Dyninst::Absloc inputs_aloc1 = inputs.at(0).absloc();

      // processing special vector operand
      if (uop->vec_len > 1) { 
        if (inputs.size() > 1) {

          Dyninst::Absloc inputs_aloc2 = inputs.at(1).absloc();
          if (inputs_aloc1.format().compare(inputs_aloc2.format()) != 0) { // two different input fields, usually add
            uop->type = MIAMI::IB_add;
          }
          if (inputs_aloc1.format().compare(inputs_aloc2.format()) == 0) { // two same input fields, usually xor
            uop->type = MIAMI::IB_xor;
          }  
        } else if (inputs.size() == 1) {
          if (inputs_aloc1.type() == Dyninst::Absloc::Register && inputs_aloc1.reg().size() <= 8) { // precision convert from 32 to 64
            uop->type = MIAMI::IB_cvt_prec;
          } 
        }

      // processing scalar operand with the first input region being register
      } else if (uop->vec_len == 1 && inputs_aloc1.type() == Dyninst::Absloc::Register) {

        if (uop->num_src_operands == 0 && aloc_out.format().compare(inputs_aloc1.format()) != 0 && inputs.size() <= 2) { // two different Registers
          
          for (unsigned int i = 0; i < inputs.size(); ++i)  // check (guess) which operation it is
          {
             Dyninst::Absloc tmp = inputs.at(i).absloc();
             if (tmp.type() == Dyninst::Absloc::Heap || tmp.type() == Dyninst::Absloc::Stack || tmp.type() == Dyninst::Absloc::Unknown) {
              uop->type = MIAMI::IB_load;

              // In the case of call, delete the original uop to build a new load uop
              if (insn_data.dInst->micro_ops.front().type == MIAMI::IB_load && insn_data.dInst->micro_ops.size() > 1) 
                  insn_data.dInst->micro_ops.pop_front(); // we pop the front because only we translate call's assignment in reverse order.
              
              // Not in the case of call.
              else if (insn_data.dInst->micro_ops.back().type == MIAMI::IB_load && insn_data.dInst->micro_ops.size() == 1) 
                  insn_data.dInst->micro_ops.pop_back(); 
              

              create_load_micro(aptr, 0,insn_data);
              return;
             }
          }
          if (uop->type == MIAMI::IB_unknown) uop->type = MIAMI::IB_copy; // default: IB_copy 

        } else if (uop->num_src_operands == 0 && aloc_out.format().compare(inputs_aloc1.format()) == 0 \
                      && inputs.size() == 1) { // Two same Register. The 'add' before the 'store' decrements the pc by -8
                                               // If certain 'push' instructions has no AST
          uop->type = MIAMI::IB_add;
          uop->imm_values[uop->num_imm_values].is_signed = true; 
          uop->imm_values[uop->num_imm_values].value.s = -8; 
          uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_IMMED, uop->num_imm_values++);
      
        }
      }

      int flags = 0;
      // processing the inputs field
      for (unsigned int i = 0; i < inputs.size(); i++) {
        Dyninst::Absloc aloc = inputs.at(i).absloc();
        if (aloc.type() == Dyninst::Absloc::Stack || aloc.type() == Dyninst::Absloc::Heap || aloc.type() == Dyninst::Absloc::Unknown) {
          if (uop->type == MIAMI::IB_unknown) {
            uop->type = MIAMI::IB_load; // Simply a load uop, different from the above, which creates a new load uop.
            uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_MEMORY, 0);
            insn_data.insn_locVec.push_back(aloc);
            if (i == inputs.size() - 1) { // If the last field is memory, we use default program counter to represent it.
               Dyninst::MachRegister pc = Dyninst::Absloc::makePC(insn_data.arch).reg();
               append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, pc, pc.size(), insn_data.arch);

            } else if (i < inputs.size() - 1) { // If not, the memory is represented by the register field after itself. 
               if (inputs.at(i+1).absloc().type() == Dyninst::Absloc::Register) {
                  Dyninst::MachRegister mr = inputs.at(i+1).absloc().reg();
                  append_src_regs(uop, MIAMI::OperandType_MEMORY, insn_data.dyn_insn, mr, mr.size(), insn_data.arch); 
                  break;
               }
            }
          }
        } else if (aloc.type() == Dyninst::Absloc::Register) {
          if (check_flags_registers(aloc.reg(),insn_data.arch)) {
            flags = 1;
          } else {
            unsigned int idx = get_register_index(aloc,insn_data.insn_locVec);
            uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx); // no extract to check
            append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, aloc.reg(), aloc.reg().size() * 8, insn_data.arch);
          }
        }
      }
      if (flags) {
        uop->src_opd[uop->num_src_operands] = make_operand(MIAMI::OperandType_REGISTER, uop->num_src_operands - 1);
        uop->num_src_operands++;
        append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, get_machine_flag_reg(insn_data.arch), 1, insn_data.arch); // not sure the size of the flag yet
      }
    } 
    
  // Already know from get_dest_field() that it is a store
  } else if (uop->type == MIAMI::IB_store) { 
    Dyninst::Absloc inputs_aloc = inputs.at(0).absloc();
    if (inputs_aloc.type() == Dyninst::Absloc::Register) {
      unsigned int idx = get_register_index(inputs_aloc,insn_data.insn_locVec);
      uop->src_opd[uop->num_src_operands++] = make_operand(MIAMI::OperandType_REGISTER, idx); 
      append_src_regs(uop, MIAMI::OperandType_REGISTER, insn_data.dyn_insn, inputs_aloc.reg(), inputs_aloc.reg().size() * 8, insn_data.arch);
    }
  }
}