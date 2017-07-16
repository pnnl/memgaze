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

#include "instr_bins.H"

#include "dyninst-insn-xlate.hpp"


//***************************************************************************

// FIXME: tallent: static data. Should be part of translation context
static Dyninst::ParseAPI::Block* insn_myBlock;

static int insn_numAssignments = 0;
static std::vector<Absloc> insn_locVec;
static std::vector<RoseAST::Ptr> insn_opVec;
static int insn_jump_tgt = 0;

//***************************************************************************

unsigned long get_start_address(std::vector<BPatch_object*> * objs, std::string file_name);

int isaXlate_getFunction(unsigned long pc);
Dyninst::ParseAPI::Block* isaXlate_getBlock(int f, addrtype pc);
void isaXlate_getDyninstInsn(addrtype pc, BPatch_function *f, std::vector<Assignment::Ptr> * assignments, Dyninst::InstructionAPI::Instruction::Ptr* insp);
void dynXlate_insn(Dyninst::InstructionAPI::Instruction::Ptr insp, std::vector<Assignment::Ptr> assignments, addrtype pc, MIAMI::DecodedInstruction* dInst);


void dynXlate_dumpInsn(Dyninst::InstructionAPI::Instruction::Ptr insn, std::vector<Assignment::Ptr> assignments, addrtype pc);


//***************************************************************************
// 
//***************************************************************************


// Initialize all the blocks for all the functions for a given module.
void isaXlate_init(const char* progName)
{
#if 0 // FIXME:tallent
  std::cerr << "binary: " << progName << endl;

  lm_codeSource = new Dyninst::ParseAPI::SymtabCodeSource((char*)progName);

  BPatch bpatch;
  BPatch_addressSpace* app = bpatch.openBinary(progName,false);
  
  BPatch_image *lm_image = app->getImage();
  lm_functions = *(lm_image->getProcedures(true));
#endif

#if 0 // FIXME:tallent: for one routine this computes the CFG for *all* routines
  for (unsigned int i = 0; i < lm_functions->size(); ++i) {
    BPatch_flowGraph *fg = (*lm_functions)[i]->getCFG();
    std::set<BPatch_basicBlock*> blks;
    fg->getAllBasicBlocks(blks);
    lm_func2blockMap[i] = blks;
  }
#endif
}


int isaXlate_insn(void* pc, int len, MIAMI::DecodedInstruction* dInst,
		  BPatch_function* dyn_func, BPatch_basicBlock* dyn_blk)
{
  Dyninst::ParseAPI::Block* dyn_blk1 = Dyninst::ParseAPI::convert(dyn_blk);
  insn_myBlock = dyn_blk1; // FIXME
  
  if (insn_myBlock) {
    Dyninst::InstructionAPI::Instruction::Ptr insn;
    std::vector<Assignment::Ptr> assignments;
    isaXlate_getDyninstInsn(pc, dyn_func, &assignments, &insn);

    if (!insn) {
      assert("Cannot find instruction.\n");
      return 0;
    }

    insn_numAssignments = assignments.size();

    dynXlate_dumpInsn(insn, assignments, pc);
    std::cerr << "\n";

    // testing
    dynXlate_insn(insn, assignments, pc, dInst);

    DumpInstrList(dInst);
    std::cerr << "\n";
  }
  else {
    return -1;
  }

  return 0;
}


// tallent: Requires 'Function' and 'Block' context.

// It translate the an instruction's dyninst IR into MIAMI IR and 
// return the length of the translated instruction to increment the pc.
int isaXlate_insn_old(unsigned long pc, MIAMI::DecodedInstruction* dInst)
{
  int f = isaXlate_getFunction(pc); // FIXME: use (Routine*)->[blocks] map
  insn_myBlock = isaXlate_getBlock(f, pc);
  
  if (insn_myBlock) {
    Dyninst::InstructionAPI::Instruction::Ptr insn;
    std::vector<Assignment::Ptr> assignments;
    isaXlate_getDyninstInsn(pc, (*lm_functions)[f], &assignments, &insn);

    if (!insn) {
      assert("Cannot find instruction.\n");
      return 0;
    }

    insn_numAssignments = assignments.size();

    dynXlate_dumpInsn(insn, assignments, pc);
    std::cerr << "\n";

    // testing
    dynXlate_insn(insn, assignments, pc, dInst);

    DumpInstrList(dInst);
    std::cerr << "\n";
  }
  else {
    return -1;
  }

  return 0;
}


//***************************************************************************
// 
//***************************************************************************

void dynXlate_assignments(Dyninst::InstructionAPI::Instruction::Ptr insn, MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments);
void dynXlate_assignment(MIAMI::DecodedInstruction* dInst, Assignment::Ptr aptr, Dyninst::InstructionAPI::Instruction::Ptr insn);

void dynXlate_insnectAST(MIAMI::DecodedInstruction* dInst, Assignment::Ptr aptr,
			 instruction_info* uop, AST::Ptr ast,
			 Dyninst::InstructionAPI::Instruction::Ptr insn);


void dynXlate_leave(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn);
void dynXlate_nop(MIAMI::DecodedInstruction* dInst, Dyninst::InstructionAPI::Instruction::Ptr insn);
void dynXlate_or(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn);
void dynXlate_divide(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn);
void dynXlate_call(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn);
void dynXlate_return(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn);
void dynXlate_jump(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn);
void dynXlate_compare(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn);
void dynXlate_enter(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn);
void dynXlate_sysCall(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn);
void dynXlate_prefectch(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn);

  
//***************************************************************************


// Translate each instructions 
void dynXlate_insn(Dyninst::InstructionAPI::Instruction::Ptr insn, std::vector<Assignment::Ptr> assignments, addrtype pc, MIAMI::DecodedInstruction* dInst)
{
  dInst->len = (int) insn->size(); 
  dInst->pc = (addrtype) pc; 
  dInst->is_call = false;

  // The instruction's pointer in dyninst may not point to the machine data which MIAMI wants.
  // Haven't encounter any situations which mach_data is used. Thus, leave it like this for now.
  dInst->mach_data = const_cast<void*>( static_cast<const void*>(insn->ptr()) ); // CHECK

  bool x86 = (insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86
	      || insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86_64);
  if (x86 && insn->getOperation().getID() == e_leave) {
    dynXlate_leave(dInst, assignments, insn); // x86 leave
  }
  else if (x86 && insn->getOperation().getID() == e_nop) {
    dynXlate_nop(dInst, insn); // x86 nop
  }
  else if (x86 && insn->getOperation().getID() >= e_or && insn->getOperation().getID() <= e_orps) {
    dynXlate_or(dInst, assignments, insn); // x86 or
  }
  else if (x86 && insn->getOperation().getID() >= e_div && insn->getOperation().getID() <= e_divss) {
    dynXlate_divide(dInst, assignments, insn); // x86 divide
  }
  else {
    switch(insn->getCategory()) {
      case c_CallInsn:
        dInst->is_call = true;
        dynXlate_call(dInst, assignments, insn);
        break;
      case c_ReturnInsn:
        dynXlate_return(dInst, assignments, insn);
        break;
      case c_BranchInsn:
        dynXlate_jump(dInst, assignments, insn);
        break;
      case c_CompareInsn:
        dynXlate_compare(dInst, assignments, insn);
        break;
      case c_SysEnterInsn:
        dynXlate_enter(dInst, assignments, insn); // this function hasn't been implemented yet.
        break;
      case c_SyscallInsn:
        dInst->is_call = true;
        dynXlate_sysCall(dInst, assignments, insn);
        break;
      case c_PrefetchInsn:
        dynXlate_prefectch(dInst, assignments, insn);
        break;
      case c_NoCategory:{
        dynXlate_assignments(insn, dInst, assignments); // default choice is always dynXlate_assignments.
        break;
      }
      default:
        assert("Impossible!\n");
        break;
    }
  }

  insn_locVec.clear();
  
  return;
}



//**************************************************p*************************
// 
//***************************************************************************







// Most reg has lsb 0. Don't know the special case yet.
int get_src_reg_lsb(MachRegister mr) { 
   return 0;
}






//***************************************************************************

// Translate leave instruction.Usually contains 3 uops: Copy, Load, Add.
void dynXlate_leave(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  for (auto ait = assignments.begin(); ait != assignments.end(); ++ait) {
    dInst->micro_ops.push_back(instruction_info());
    instruction_info* uop = &dInst->micro_ops.back();
    get_dest_field(dInst, *ait, uop);
    parse_assign(dInst, *ait, uop, insn);
    if (uop->type == IB_copy)
      uop->primary = true;
    if ((*ait)->out().absloc().type() == Absloc::Register)
    {
       append_dest_regs(uop, OperandType_REGISTER, insn, (*ait)->out().absloc().reg(), uop->width * uop->vec_len);
    }
  }

  create_add_micro(dInst, insn, assignments.at(0));
}

// Translate the nop instruction 
void dynXlate_nop(MIAMI::DecodedInstruction* dInst, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  dInst->micro_ops.push_back(instruction_info());
  instruction_info* uop = &dInst->micro_ops.back();
  uop->type = IB_nop;
  uop->width = 0;
  uop->vec_len = 0;
  uop->exec_unit = ExecUnit_SCALAR;
  uop->exec_unit_type = ExecUnitType_INT;
}

// Translate or instruction. Make it into a special case for simplicity.
void dynXlate_or(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  dInst->micro_ops.push_back(instruction_info());
  instruction_info* uop = &dInst->micro_ops.back();
  uop->type = IB_logical;
  uop->primary = true;
  uop->vec_len = get_vec_len(insn);
  uop->exec_unit = (uop->vec_len == 1) ? ExecUnit_SCALAR : ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn);

  uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_INTERNAL, 1);
  append_src_regs(uop, OperandType_INTERNAL, insn, Dyninst::MachRegister(), uop->width * uop->vec_len);

  uop->dest_reg_list.clear();
  uop->dest_opd[uop->num_dest_operands++] = make_operand(OperandType_INTERNAL, 0);
  append_dest_regs(uop, OperandType_INTERNAL, insn, Dyninst::MachRegister(), uop->width * uop->vec_len);
  uop->dest_opd[uop->num_dest_operands] = make_operand(OperandType_REGISTER, uop->num_dest_operands);
  uop->num_dest_operands++;
  append_dest_regs(uop, OperandType_REGISTER, insn, get_machine_flag_reg(), 1);

  for (unsigned int i = 0; i < assignments.size(); ++i)
  {
    // if output field is not flag, we traverse its AST to get constant values and its width
    Absloc aloc = assignments.at(i)->out().absloc();
    if (aloc.type() == Absloc::Register)
    {
      if (!check_flags_registers(aloc.reg()))
        {
          static std::pair<AST::Ptr, bool> astPair;
          astPair = SymEval::expand(assignments.at(i), false);
          if (astPair.second && NULL != astPair.first) // 
          {
            if (!uop->width)
              get_width_from_tree(uop, astPair.first);
            
            traverse_or(uop, astPair.first); // get value from AST
            create_load_micro(dInst, insn, assignments.at(i), 1);
            create_store_micro(dInst, insn, assignments.at(i), 0);
            return;
          }
        }  
    }
  }

  // Have to create load an store micro. Take default choice 0.
  create_load_micro(dInst, insn, assignments.at(0), 1);
  uop->width = dInst->micro_ops.front().width;
  create_store_micro(dInst, insn, assignments.at(0), 0);
  return;
}

// Translate the divide instruction. Make it a special case because it contains 8 assignments 
// and this way is easier. 
void dynXlate_divide(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  dInst->micro_ops.push_back(instruction_info());
  instruction_info* uop = &dInst->micro_ops.back();
  uop->type = IB_div;
  if(!uop->vec_len) uop->vec_len = get_vec_len(insn);
  uop->primary = true;
  uop->exec_unit = (uop->vec_len == 1) ? ExecUnit_SCALAR : ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn);

  // get all the source operands from one assignment
  Assignment::Ptr aptr = assignments.at(0);
  for (unsigned int i = 0; i < aptr->inputs().size(); ++i)
  {
    if (aptr->inputs().at(i).absloc().type() == Absloc::Register)
    {
      if (!check_flags_registers(aptr->inputs().at(i).absloc().reg()))
      {
        MachRegister mr = aptr->inputs().at(i).absloc().reg();
        uop->src_opd[uop->num_src_operands] = make_operand(OperandType_REGISTER, uop->num_src_operands);
	uop->num_src_operands++;
        append_src_regs(uop, OperandType_REGISTER, insn, mr, mr.size() * 8);
        insn_locVec.push_back(aptr->inputs().at(i).absloc());
        if (!uop->width)
          uop->width = mr.size() * 8;
      }
    }
  }

  // get all the destination operands from all assignments
  for (unsigned int i = 0; i < assignments.size(); ++i)
  {
    Absloc aloc = assignments.at(i)->out().absloc();
    if (assignments.at(i)->out().absloc().type() == Absloc::Register)
    {
      if (!check_flags_registers(assignments.at(i)->out().absloc().reg()))
      {
        MachRegister mr = assignments.at(i)->out().absloc().reg();
        uop->dest_opd[uop->num_dest_operands++] = make_operand(OperandType_REGISTER, get_register_index(aloc));
        append_dest_regs(uop, OperandType_REGISTER, insn, mr, mr.size() * 8);
      }
    }
  }

  // for x86 machines, we clapse all the flag output regions into one: x86::flags or x86_64::flags
  if (insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86 || insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86_64)
  {
    uop->dest_opd[uop->num_dest_operands] = make_operand(OperandType_REGISTER, uop->num_dest_operands);
    uop->num_dest_operands++;
    append_dest_regs(uop, OperandType_REGISTER, insn, get_machine_flag_reg(), 1);
  }
}

// Translate call instruction (two different kinds, with load (5 uops) or without load (4 uops)).
void dynXlate_call(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  for (unsigned int i = 0; i < assignments.size(); i++) {
    if (assignments.at(i)->inputs().size() > 0) {
      dInst->micro_ops.push_front(instruction_info());  // we push front because Dyninst reverses the order 
                                                        // of the assignments in this case of call. 
      instruction_info* uop = &dInst->micro_ops.front();
      get_dest_field(dInst, assignments.at(i), uop);
      parse_assign(dInst, assignments.at(i), uop, insn);
      if (dInst->micro_ops.front().type == IB_load) // special call with load usually call [??? + ???]
      {
        uop = &dInst->micro_ops.front();
        uop->num_dest_operands = 1; // Reassign the destination value of load uop in call insn.
        uop->dest_opd[0] = make_operand(OperandType_INTERNAL, 0);
        append_dest_regs(uop, OperandType_INTERNAL, insn, Dyninst::MachRegister(), uop->width * uop->vec_len);
      } else {
        append_all_dest_registers(uop, insn, assignments.at(i));
      }
    }
  }

  // Find the asssignment with AST and create jump uop using the value retrieved from AST.
  for (unsigned int i = 0; i < assignments.size(); ++i) {
    static std::pair<AST::Ptr, bool> astPair;
    astPair = SymEval::expand(assignments.at(i), false);
    if (astPair.second && NULL != astPair.first) {
      calculate_jump_val(astPair.first); // We need AST to find the value which the call jump to.

      if (dInst->micro_ops.size() == 3) { // If we have load uop, change the jump's src operand to INTERNAL
	create_jump_micro(dInst, 0, insn, assignments.at(0), true);
      }
      else { // no load operation
	insn_jump_tgt = dInst->len;
	create_jump_micro(dInst, insn_jump_tgt, insn, assignments.at(0), false);
      }

      insn_jump_tgt = 0; // this is static
      break;
      
    }
  }  

  // the first add is usually the second assignment, so we pass the second one over. CHECK to make sure.
  create_add_micro(dInst, insn, assignments.at(1)); 
}

// Translate 'return' instruction into 3 uops: Load, Add, Return.
void dynXlate_return(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments,
                                            Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  if (assignments.size() != 2) 
    assert("Leave instruction should contain two and only two assignments.\n"); 
  
  // only translate the first assignments
  dInst->micro_ops.push_back(instruction_info());
  instruction_info* uop = &dInst->micro_ops.back();
  get_dest_field(dInst, assignments.at(0), uop);
  parse_assign(dInst, assignments.at(0), uop, insn);

  if (assignments.at(0)->out().absloc().type() == Absloc::Register)
     append_dest_regs(uop, OperandType_REGISTER, insn, assignments.at(0)->out().absloc().reg(), uop->width * uop->vec_len);   
  
  create_add_micro(dInst, insn, assignments.at(1)); // use the second assignments to create add uop
  create_return_micro(dInst, insn, assignments.at(0));
}

// Translate the jump instruction
void dynXlate_jump(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  dInst->micro_ops.push_back(instruction_info());
  instruction_info *uop = &dInst->micro_ops.back();

  get_dest_field(dInst, assignments.at(0), uop);
  static std::pair<AST::Ptr, bool> astPair;
  astPair = SymEval::expand(assignments.at(0), false);
  if (astPair.second && NULL != astPair.first) {
    dynXlate_jump_ast(dInst, uop, astPair.first, insn, assignments.at(0)); 
    calculate_jump_val(astPair.first);
  }
  
  if (insn_jump_tgt) { // If the AST gives the constant operand
    insn_jump_tgt = insn_jump_tgt - dInst->len;
    uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; //FIXME
    uop->imm_values[uop->num_imm_values].value.s = insn_jump_tgt;
    uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_IMMED, uop->num_imm_values++); 
    insn_jump_tgt = 0;  
  }

  Absloc out_aloc = assignments.at(0)->out().absloc();
  if (out_aloc.type() == Absloc::Register)
     append_dest_regs(uop, OperandType_REGISTER, insn, out_aloc.reg(), out_aloc.reg().size() * 8); 
}

// Translate jump by traversing its AST.
void dynXlate_jump_ast(MIAMI::DecodedInstruction* dInst, instruction_info* uop, AST::Ptr ast, Dyninst::InstructionAPI::Instruction::Ptr insn, Assignment::Ptr aptr)
{
  if (uop->vec_len == 0) {
    uop->vec_len = get_vec_len(insn);
  }
    
  if (uop->exec_unit == ExecUnit_LAST) uop->exec_unit = (uop->vec_len == 1) ? ExecUnit_SCALAR : ExecUnit_VECTOR;
  if (uop->exec_unit_type == ExecUnitType_LAST) uop->exec_unit_type = get_execution_type(insn);
  if (!uop->width) get_width_from_tree(uop, ast);

  unsigned int i;
  switch(ast->getID()){
    case AST::V_RoseAST:{
      RoseAST::Ptr ast_r = RoseAST::convert(ast);

      if (uop->type == IB_unknown) { 
        get_operator(uop, ast, aptr, insn, dInst);

        if (uop->type == IB_add) 
          uop->type = IB_branch; // CHECK

        if (uop->type == IB_load) { // deal with jump qword ptr [rip+0x200682]
          create_load_micro(dInst, insn, aptr, 0);
          uop->type = IB_branch;
          uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_INTERNAL, 0);
          append_src_regs(uop, OperandType_INTERNAL, insn, Dyninst::MachRegister(), \
                                    dInst->micro_ops.front().width * dInst->micro_ops.front().vec_len);
          return;
        }
        if (uop->type == IB_unknown) 
          uop->type = IB_branch;
      }

      if (ast_r->val().op == ROSEOperation::ifOp || ast_r->val().op == ROSEOperation::derefOp || uop->type == IB_branch) { 
      // if conditional and operator is 'if' || unconditional branching --> might get more imms, fine with dg.
        for (i = 0; i < ast->numChildren(); ++i) {
          dynXlate_jump_ast(dInst, uop, ast->child(i), insn, aptr);
        }  
      } else {
        dynXlate_jump_ast(dInst, uop, ast->child(0), insn, aptr);
      }
      break;
    }

    case AST::V_ConstantAST:{ // can only get width here

      if (uop->num_imm_values < 2) 
      {
        ConstantAST::Ptr ast_c = ConstantAST::convert(ast);
        if (uop->width == 0 && uop->vec_len) {
          uop->width = ast_c->val().size / uop->vec_len;
        }
      }
      break;
    }
      

    case AST::V_VariableAST:{ // can get type, width and source registers.

      VariableAST::Ptr ast_v = VariableAST::convert(ast);

      if (uop->type == IB_unknown)
        uop->type = IB_branch;
         
      if (ast_v->val().reg.absloc().type() == Absloc::Register){
        if (uop->width == 0 && uop->vec_len) {
          uop->width = ast_v->val().reg.absloc().reg().size() * 8 / uop->vec_len;
        }
            
        unsigned int idx = get_register_index(ast_v->val().reg.absloc());
        uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_REGISTER, idx);

        int size = 0;
        if (insn_opVec.size() && insn_opVec.back()->val().op == ROSEOperation::extractOp)
          size = insn_opVec.back()->val().size; // don't need to time it by 8, its already in bit
        else 
          size = ast_v->val().reg.absloc().reg().size() * 8;
          append_src_regs(uop, OperandType_REGISTER, insn, ast_v->val().reg.absloc().reg(), size);
      }
      break;
    }
    default:{
    assert("Wrong type of AST node.\n");

    }
  }
}

// Translate compare uop
void dynXlate_compare(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  Assignment::Ptr assign = assignments.at(0);
  std::vector<AbsRegion> inputs = assign->inputs();
  for (unsigned int i = 0; i < inputs.size(); ++i)
  {
    Absloc aloc = inputs.at(i).absloc();
    if (aloc.type() == Absloc::Stack || aloc.type() == Absloc::Heap || aloc.type() == Absloc::Unknown) {
      create_load_micro(dInst, insn, assign, 0); // Create load uop if we have mem as input field.
      insn_locVec.push_back(aloc);
    }
  }

  create_compare_micro(dInst, assignments, insn);
  //update_dest_with_flag(dInst, insn); // Compare instruction usually has destination fields being flags.
}

// Haven't encounter any enter yet. Don't really know how MIAMI translate it.
void dynXlate_enter(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  return;
}


// System call is translated into jump uop, with two destination operands:
// program counter and flags.
void dynXlate_sysCall(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  dInst->micro_ops.push_back(instruction_info());
  instruction_info* uop = &dInst->micro_ops.back();
  uop->type = IB_jump;
  uop->width = 64;
  uop->primary = true;
  if(!uop->vec_len) uop->vec_len = get_vec_len(insn);
  uop->exec_unit = (uop->vec_len == 1) ? ExecUnit_SCALAR : ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn);

  MachRegister pc = Absloc::makePC(insn_myBlock->obj()->cs()->getArch()).reg();
  uop->dest_opd[uop->num_dest_operands] = make_operand(OperandType_REGISTER, uop->num_dest_operands);
  uop->num_dest_operands++;
  append_dest_regs(uop, OperandType_REGISTER, insn, pc, pc.size() * 8);
  
  uop->dest_opd[uop->num_dest_operands] = make_operand(OperandType_REGISTER, uop->num_dest_operands);
  uop->num_dest_operands++;
  append_dest_regs(uop, OperandType_REGISTER, insn, get_machine_flag_reg(), 1);
}

void dynXlate_prefectch(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  dInst->micro_ops.push_back(instruction_info());
  instruction_info* uop = &dInst->micro_ops.back();
  uop->type = IB_prefetch;
  uop->width = 64;
  uop->primary = true;
  uop->vec_len = 8; // Is this always true?
  uop->exec_unit = ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn);
  uop->src_opd[uop->num_src_operands] = make_operand(OperandType_MEMORY, uop->num_src_operands);
  uop->num_src_operands++;
  
  for (unsigned int i = 0; i < assignments.size(); ++i)
  {
     if (assignments.at(i)->inputs().at(0).absloc().type() == Absloc::Register)
     {
        append_src_regs(uop, OperandType_MEMORY, insn, assignments.at(i)->inputs().at(0).absloc().reg(), \
                assignments.at(i)->inputs().at(0).absloc().reg().size() * 8);
        break;
     }
  }
}

// Translate test instruction. Make it into special cases for 'simplity'.
// Test instruction is translated into one LogicalOp uop. 
void dynXlate_test(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  dInst->micro_ops.push_back(instruction_info());
  instruction_info* uop = &dInst->micro_ops.back();
  uop->type = IB_logical; 
  if(!uop->vec_len) uop->vec_len = get_vec_len(insn);
  uop->exec_unit = (uop->vec_len == 1) ? ExecUnit_SCALAR : ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn);

  // We take the first assignment and all of them have the same input fields 
  // and different output fields with different flags, and we don't care
  // about the differences. 
  if (assignments.at(0)->inputs().at(0).absloc().type() == Absloc::Register) {
    MachRegister reg = assignments.at(0)->inputs().at(0).absloc().reg();
    if (!uop->width && uop->vec_len) // get width
      uop->width =  reg.size() * 8 / uop->vec_len; 

    // get souce operands information and the source registers.
    unsigned int idx = get_register_index(assignments.at(0)->inputs().at(0).absloc());
    uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_REGISTER, idx); 
    append_src_regs(uop, OperandType_REGISTER, insn, reg, reg.size() * 8);
    if (assignments.at(0)->inputs().size() > 1){
      Absloc aloc = assignments.at(0)->inputs().at(1).absloc();
      if (aloc.type() == Absloc::Register)
      {
       uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_REGISTER, idx+1); 
       append_src_regs(uop, OperandType_REGISTER, insn, aloc.reg(), aloc.reg().size() * 8); 

      // If it contains memory field.
      } else if (aloc.type() == Absloc::Heap || aloc.type() == Absloc::Stack || aloc.type() == Absloc::Unknown) 
      {
        uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_MEMORY, idx+1); 
        append_src_regs(uop, OperandType_MEMORY, insn, reg, reg.size() * 8); 
      }
    }

    // test instruction always contains flags output region.
    uop->dest_opd[uop->num_dest_operands] = make_operand(OperandType_REGISTER, uop->num_dest_operands);
    uop->num_dest_operands++;
    append_dest_regs(uop, OperandType_REGISTER, insn, get_machine_flag_reg(), 1);
  }
}


//***************************************************************************
// 
//***************************************************************************

// dynXlate_all the assignments of an instruction
void dynXlate_assignments(Dyninst::InstructionAPI::Instruction::Ptr insn, MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments)
{
  int flag = 0;
  for (unsigned int i = 0; i < assignments.size(); i++) {
    if (destOpIsFlag(assignments.at(i))) {
      flag = 1;
    } else {
      // If it's not flag, we start processing the assignment.
      dynXlate_assignment(dInst, assignments.at(i), insn);
    }
  }
  if (dInst->micro_ops.size() == 1) dInst->micro_ops.back().primary = true; // only one uop, it must be primary.

  if (!flag && dInst->micro_ops.size() == 1 && dInst->micro_ops.back().type == IB_add \
                                              && dInst->micro_ops.back().vec_len <= 1) { // not including vector operand
     dInst->micro_ops.back().type = IB_lea;
     dInst->micro_ops.back().src_opd[dInst->micro_ops.back().num_src_operands++] = make_operand(OperandType_MEMORY, 0);

    if (assignments.at(0)->inputs().size()) // lea get address not from rip
    {
      Absloc aloc = assignments.at(0)->inputs().at(0).absloc();
      if (aloc.type() == Absloc::Register)
      {
         append_src_regs(&dInst->micro_ops.back(), OperandType_MEMORY, insn, aloc.reg(), aloc.reg().size() * 8);
      }
    }else {
       append_src_regs(&dInst->micro_ops.back(), OperandType_MEMORY, insn, Dyninst::MachRegister(), 64);
     }

  /* Special case: processing push and pop.
   * It seems that dynXlate_insnectAST cannot cannot get the CONSTANT values in 'add' uop, so we append it here.
   * The reason it cannot get the constant value is because the traverse_options function do not traverse
   * all the children of 'add' ROSEOperations' in most of the cases (but only the first one).
   * We do it that way to stop the recursion sooner for the general uops. Many uops contains 'add' ROSEOperation.
   * They are usually large and usually only the first branch of add's children is meaningful. CHECK*/

  } else if (!flag && dInst->micro_ops.size() == 2 \
    && dInst->micro_ops.back().type == IB_add && dInst->micro_ops.front().type == IB_store) // push
  {
    instruction_info* uop = &dInst->micro_ops.back(); // get the 'add' uop
    uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; 
    uop->imm_values[uop->num_imm_values].value.s = -8; 
    uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_IMMED, uop->num_imm_values++);
    dInst->micro_ops.reverse(); // reverse the order of store and add. Dyninst order does not corresponds
                                // well with MIAMI's order sometimes.

  } else if (!flag && dInst->micro_ops.size() == 2 \
    && dInst->micro_ops.back().type == IB_add && dInst->micro_ops.front().type == IB_load) // pop
  {
    instruction_info* uop = &dInst->micro_ops.back(); // get the 'add' uop
    uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; 
    uop->imm_values[uop->num_imm_values].value.s = 8; 
    uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_IMMED, uop->num_imm_values++);
  }

  if (flag) 
    update_dest_with_flag(dInst, insn); // update the primary uop with a flag destination. (x86/x86_64)
  if (flag && dInst->micro_ops.size() == 0) { 
    dynXlate_test(dInst, assignments, insn); // all dest ops are flags-->test uop
  } else if (flag && dInst->micro_ops.size() == 0 && dInst->micro_ops.begin()->vec_len > 1)
  {

  }

}

void dynXlate_assignment(MIAMI::DecodedInstruction* dInst, Assignment::Ptr aptr, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  // start a new micro operation
  dInst->micro_ops.push_back(instruction_info());
  instruction_info *uop = &dInst->micro_ops.back();

  static std::pair<AST::Ptr, bool> astPair;
  astPair = SymEval::expand(aptr, false);

  //need to check special xmm/ymm case
  int len = check_vector_regs(insn, aptr);
  if (len) {
   uop->vec_len = len;
   uop->exec_unit = ExecUnit_VECTOR; 
  }

  get_dest_field(dInst, aptr, uop);

  // If the assignment contains an abstract syntax tree
  if (astPair.second && NULL != astPair.first) {
    //check whether there are any load assignments
    get_operator(uop, astPair.first, aptr, insn, dInst);

    dynXlate_insnectAST(dInst, aptr, uop, astPair.first, insn);

    // These special cases are quite messy. Even "add [R14 + 8], 1" and "add [R15 + RAX * 4], 1" are translated 
    // differently, with the first one having 3 uops: load, add, store, and the second one having 2 uops (wrong)
    // load and store. (Let along even more special cases like "xadd"--exchange and add, 
    // which dyninst gives little information and no AST. So it will be processed in parse_assign.
    // Thus, for now, xadd can only be translate into a load and a store as well. )

    // add [RAX], AL and or [RIP+xxx], EAX
    Absloc aloc = aptr->out().absloc();
    if ((aloc.type() == Absloc::Heap || aloc.type() == Absloc::Stack || aloc.type() == Absloc::Unknown) \
                                     && dInst->micro_ops.size() == 1) {

      if (uop->type != IB_store) //uop->type == IB_add
      {
        create_store_micro(dInst, insn, aptr, 1);
        uop->primary = true; // add the is primary micro
        uop->dest_opd[0] = make_operand(OperandType_INTERNAL, 1);
        append_dest_regs(uop, OperandType_INTERNAL, insn, Dyninst::MachRegister(), uop->width * uop->vec_len);// FIXME
      }
    }


    // If inputs fields being memory.
    for (unsigned int i = 1; i < aptr->inputs().size(); ++i) { 
      Absloc aloc = aptr->inputs().at(i).absloc();
      if (aloc.type() == Absloc::Stack || aloc.type() == Absloc::Heap || aloc.type() == Absloc::Unknown) {
        if (uop->type != IB_load){ // the in case of add [...]. which dynXlate_insnectAST will return only an add uop,
                                   // since AST does not contain information of memory field, but we need
                                   // to create another load uop. 
          create_load_micro(dInst, insn, aptr, 0);
          insn_locVec.push_back(aptr->inputs().at(i).absloc());

          // Change the REGISTER operand by INTERNAL operand since we have another load uop. 
          // Not the best practice to change the last element of source operands like the
          // following. But leave it like this for now. 
          if(uop->num_src_operands) uop->src_opd[uop->num_src_operands-1] = make_operand(OperandType_INTERNAL, 0); //FIXME, not the right way to do it
          append_src_regs(uop, OperandType_INTERNAL, insn, Dyninst::MachRegister(), uop->width * uop->vec_len);
          // Similarly, change the destination memory space into internal too. 
          if (aloc.type() == Absloc::Register)
          {
            uop->dest_opd[0] = make_operand(OperandType_REGISTER, 1);
            uop->dest_reg_list.clear();
            append_dest_regs(uop, OperandType_REGISTER, insn, aloc.reg(), aloc.reg().size() * 8);  
          }
          
          break;
        } 
      }
    }


  } else { // The assignment doesn't contain ast
    parse_assign(dInst, aptr, uop, insn);
  }

  if (uop->num_dest_operands > 4) // This is a very bad bug I haven't figure out yet. Some times the program gives
                                  // > 100 destination operand for instructions like fild or fld. I haven't been 
                                  // able to figure out yet. So I just arbitrarily assign the value to be 1 so 
                                  // append_all_dest_registers won't segfault. 
  {
    uop->num_dest_operands = 1;
  }
  append_all_dest_registers(uop, insn, aptr); // processe all destination register information.

}

// Parse the AST of an assignment.
void dynXlate_insnectAST(MIAMI::DecodedInstruction* dInst, Assignment::Ptr aptr,
			 instruction_info* uop, AST::Ptr ast,
			 Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  Absloc aloc;
  if (uop->vec_len == 0) 
    uop->vec_len = get_vec_len(insn);
  if (uop->exec_unit == ExecUnit_LAST) 
    uop->exec_unit = (uop->vec_len == 1) ? ExecUnit_SCALAR : ExecUnit_VECTOR;
  if (uop->exec_unit_type == ExecUnitType_LAST) 
    uop->exec_unit_type = get_execution_type(insn);

  // special cases when memory field does not shown in ast --> CHECK
  if (uop->type == IB_load && !uop->num_src_operands) {
    for (unsigned int i = 0; i < aptr->inputs().size(); ++i) {
      static std::pair<AST::Ptr, bool> astPair;
      astPair = SymEval::expand(aptr, false);

      if (!uop->width && astPair.second && NULL != astPair.first) 
        get_width_from_tree(uop, astPair.first);

      aloc = aptr->inputs().at(i).absloc();
      if (aloc.type() == Absloc::Stack || aloc.type() == Absloc::Heap || aloc.type() == Absloc::Unknown) {
        uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_MEMORY, 0);  // Does the index matter in the case of mem?
        insn_locVec.push_back(aloc);

        // building the source register information with the right register to represent it
        if (i != 0) { // The memory field is not is first input field. We choose the register field before it to represent mem field.
          MachRegister mr = aptr->inputs().at(i-1).absloc().reg();
          append_src_regs(uop, OperandType_MEMORY, insn, mr, mr.size() * 8);
        }
	else { // Using default program counter to represent the memory field.
          MachRegister pc = Absloc::makePC(insn_myBlock->obj()->cs()->getArch()).reg();
          append_src_regs(uop, OperandType_MEMORY, insn, pc, pc.size() * 8); 
        }
        return;
      }
    }
  // If load already has a src operand (must be memory), ignore all the others memory fields 
  // in the same assignment.
  }
  else if (uop->type == IB_load && uop->num_src_operands) {
    return;
  }


  switch(ast->getID()){

    case AST::V_ConstantAST : {

      ConstantAST::Ptr ast_c = ConstantAST::convert(ast);
      Absloc aloc_out = aptr->out().absloc();

      // If get_operator function fail to provide a type for IB, then we check whether it
      // is the special case of IB_copy or IB_store. 
      if (uop->type == IB_unknown && aloc_out.type() == Absloc::Register) // copy from imm to reg
      {
        uop->type = IB_copy;
      } else if (uop->type == IB_unknown && aptr->inputs().size()) { // store into [rip+x]??
  
       // uop->type = IB_store; CHECK!!!
        uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_REGISTER, 1);

        if (aloc_out.type() == Absloc::Register) {
          append_src_regs(uop, OperandType_REGISTER, insn, aloc_out.reg(), aloc_out.reg().size() * 8);  
        } else if (aptr->inputs().at(0).absloc().type() == Absloc::Register){
          MachRegister input = aptr->inputs().at(0).absloc().reg();
          append_src_regs(uop, OperandType_REGISTER, insn, input, input.size() * 8);
        }

        return;
      }

      if (uop->width == 0 && uop->vec_len) {
        uop->width = (width_t) ast_c->val().size / uop->vec_len;
      }

      if (uop->num_imm_values < 2) {
        uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; //CHECK
        uop->imm_values[uop->num_imm_values].value.s = ast_c->val().val; 
        // std::cerr << "inside dynXlate_insnectAST, constant value is " << ast_c->val().val;
        uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_IMMED, uop->num_imm_values++); // no need to find dependency of immediates
        insn_locVec.push_back(Absloc());
      }
      return;
    }

    case AST::V_VariableAST : {

      VariableAST::Ptr ast_var = VariableAST::convert(ast);

      // If we do not have a type yet, then update type with either store or copy.
      if (uop->type == IB_unknown) {
         aloc = aptr->out().absloc();
        if (aloc.type() == Absloc::Stack || aloc.type() == Absloc::Heap || aloc.type() == Absloc::Unknown) {
          uop->type = IB_store; // Is it possible? function get_dest_field should already assign it. 
        }
	else if (aloc.type() == Absloc::Register) {
          uop->type = IB_copy;
        }
      } 

      // update width 
      if (uop->width == 0) {
        if (aptr->out().absloc().type() == Absloc::Register) { // update the width according to the output Register operand's size
          if(uop->vec_len) uop->width = aptr->out().absloc().reg().size() * 8 / uop->vec_len;

        } else if (aptr->inputs().size()) { // update the width according to the input Register operand's size
          for (unsigned int i = 0; i < aptr->inputs().size(); ++i) {
            Absloc input_loc = aptr->inputs().at(i).absloc();
            if (input_loc.type() == Absloc::Register) {
              if (uop->vec_len) uop->width = input_loc.reg().size() * 8 / uop->vec_len;
              break;
            }
          }
        } else { // operands only concerning memory
            uop->width = 64; // default 64 for Memory (load / store)--> FIXME
        }
      }

      // Update source operand list when:
      // 1. either the variable is not a flag
      // 2. or if the flag operand has not been listed yet
      if (sourceOpIsFlag(ast_var)) {
        if (uop->flag == false) {
          uop->flag = true;
          uop->src_opd[uop->num_src_operands] = make_operand(OperandType_REGISTER, uop->num_src_operands); // flags shall not appear already
	  uop->num_src_operands++;
          append_src_regs(uop, OperandType_REGISTER, insn, get_machine_flag_reg(), 1);
        }
	else {} // do nothing if source flag is already updated.

      }
      else {
        // Special cases when memory field does not shown in ast 
        if (uop->type == IB_load) {
          for (unsigned int i = 0; i < aptr->inputs().size(); ++i) {
            aloc = aptr->inputs().at(i).absloc();
            if (aloc.type() == Absloc::Stack || aloc.type() == Absloc::Heap || aloc.type() == Absloc::Unknown) {
              uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_MEMORY, 0);  // !!!!!!!!!!!!!!!!!
              insn_locVec.push_back(aloc);

              // Building the source register information.
              if (i != 0) {
                 MachRegister mr = aptr->inputs().at(i-1).absloc().reg();
                 append_src_regs(uop, OperandType_MEMORY, insn, mr, mr.size() * 8);
              }
	      else {
                 MachRegister pc = Absloc::makePC(insn_myBlock->obj()->cs()->getArch()).reg();
                 append_src_regs(uop, OperandType_MEMORY, insn, pc, pc.size() * 8); 
              }
              return;
            }
          }
        }

        // Update register source operand.
        if (ast_var->val().reg.absloc().type() == Absloc::Register) {
          unsigned int idx = get_register_index(ast_var->val().reg.absloc());
          uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_REGISTER, idx); 

          // building the source register information
          int size = 0;
          // 'extract' part of the value. eg: 32 from 64 or 16 from 64.
          if (insn_opVec.size() && insn_opVec.back()->val().op == ROSEOperation::extractOp) 
            size = insn_opVec.back()->val().size;
          else 
            size = ast_var->val().reg.absloc().reg().size();

          append_src_regs(uop, OperandType_REGISTER, insn, ast_var->val().reg.absloc().reg(), size);
        }
      }
      return;
    }


    case AST::V_RoseAST : {
      RoseAST::Ptr ast_r = RoseAST::convert(ast);

      if (uop->width == 0)
         get_width_from_tree(uop, ast);
      
      // Depending on different operations, we traverse different choices of their children.
      // Some we just ignore its children (0), some we traverse the first child (1), some 
      // we traverse all of its children (2). 
      int option = traverse_options(uop, ast_r);

      switch(option){
        case 2:
          for (unsigned int i = 0; i < ast->numChildren(); i++) {
            dynXlate_insnectAST(dInst, aptr, uop, ast->child(i), insn);
          }
          return;
        case 1:
          dynXlate_insnectAST(dInst, aptr, uop, ast->child(0), insn);
          return;
        case 0:
          // special case to deal with sub instruction
          if (ast_r->val().op == ROSEOperation::extractOp && ast->child(0)->getID() == AST::V_RoseAST && uop->type != IB_load) { 
              uop->type = IB_unknown;
              dynXlate_insnectAST(dInst, aptr, uop, ast->child(0), insn);
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


// Traverse the AST in or instruction to get constant values (if any).
void traverse_or(instruction_info* uop, AST::Ptr ast)
{
  if (ast->getID() == AST::V_ConstantAST)
  {
    ConstantAST::Ptr ast_c = ConstantAST::convert(ast);
    uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_IMMED, uop->num_imm_values++);
    uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; //FIXME
    uop->imm_values[uop->num_imm_values].value.s = ast_c->val().val; 
    uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_IMMED, uop->num_imm_values++);
    return;

  } else if(ast->getID() == AST::V_RoseAST) {
    RoseAST::Ptr ast_r = RoseAST::convert(ast);
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
int traverse_options(instruction_info* uop, RoseAST::Ptr ast)
{
  switch(ast->val().op){
    
    case ROSEOperation::invertOp:{
      return 1;
    }
    case ROSEOperation::uModOp:
    case ROSEOperation::extendOp:
    case ROSEOperation::nullOp:
    case ROSEOperation::negateOp:
    case ROSEOperation::equalToZeroOp:
    case ROSEOperation::generateMaskOp:
    case ROSEOperation::LSBSetOp:
    case ROSEOperation::MSBSetOp:
    case ROSEOperation::writeRepOp:
    case ROSEOperation::writeOp:
      return 0; 
    case ROSEOperation::extractOp:{
        if (uop->type == IB_unknown || uop->type == IB_xor || uop->type == IB_shift ) {
          return 1;
        } else if (uop->num_src_operands == 0) {
          return 1;
        } else {
          return 0;
        }
      }
    case ROSEOperation::concatOp:
    case ROSEOperation::extendMSBOp:
    case ROSEOperation::orOp:
    case ROSEOperation::xorOp:
    case ROSEOperation::rotateLOp:
    case ROSEOperation::rotateROp:
    case ROSEOperation::derefOp:
    case ROSEOperation::signExtendOp:
      return 1;
    case ROSEOperation::addOp:{ // add has two and only two children
      //  transverse the second child if we have 'or' ROSEOperation
      if (uop->type == IB_logical) {
         return 3;
      // transverse both children if the second one is a rose
      } else if (ast->child(1)->getID() == AST::V_RoseAST) 
      {
         return 2;
      } else {
         return 1;
      }
      break;
    }
    case ROSEOperation::shiftLOp:
    case ROSEOperation::shiftROp:
    case ROSEOperation::shiftRArithOp:
    case ROSEOperation::andOp:
    case ROSEOperation::ifOp:
    case ROSEOperation::sMultOp: // signed multiplication
    case ROSEOperation::uMultOp:
    case ROSEOperation::sDivOp:
    case ROSEOperation::sModOp:
    case ROSEOperation::uDivOp:
      return 2;
    default:
      fprintf(stderr, "operation %d is invalid.\n", ast->val().op);
      return -1;
  }
}


// Get all the operators from a Dyninst AST, and let get_operator to
// process it later.
void traverse_rose(AST::Ptr ast)
{
  if (ast->getID() == AST::V_RoseAST)
  {
    RoseAST::Ptr ast_r = RoseAST::convert(ast);
    insn_opVec.push_back(ast_r);
    for (unsigned int i = 0; i < ast->numChildren(); ++i)
    {
      if (ast->child(i)->getID() == AST::V_RoseAST)
      {
        traverse_rose(ast->child(i));
      }
    }
  }
}

// Translate operations from dyninst representation to MIAMI representation.
// All the ones being commented out are processed in get_operator.
void convert_operator(instruction_info* uop, ROSEOperation::Op op)
{
  switch(op){
    case ROSEOperation::nullOp:
    //case ROSEOperation::extractOp:
    //case ROSEOperation::invertOp:
    case ROSEOperation::negateOp:
    case ROSEOperation::generateMaskOp:
    case ROSEOperation::LSBSetOp:
    case ROSEOperation::MSBSetOp:
    case ROSEOperation::extendOp:
    //case ROSEOperation::extendMSBOp:
      uop->type = IB_unknown;
      break;
    case ROSEOperation::signExtendOp:
      uop->type = IB_cvt_prec;
      break;
    case ROSEOperation::uModOp:
    case ROSEOperation::sModOp:
      uop->type = IB_fn;
      break;
    case ROSEOperation::sMultOp: // signed multiplication
    case ROSEOperation::uMultOp:
      uop->type = IB_mult;
      uop->primary = true;
      break;
    case ROSEOperation::sDivOp:
    case ROSEOperation::uDivOp:
      uop->type = IB_div;
      uop->primary = true;
      break;
    //case ROSEOperation::concatOp:{
      //uop->type = IB_copy;
      //uop->primary = true;
      //break;
    //}
    case ROSEOperation::equalToZeroOp: //??compare?? is it logical
    case ROSEOperation::andOp:
    case ROSEOperation::orOp:
    case ROSEOperation::xorOp: //IB_xor
      uop->type = IB_logical;
      break;
    case ROSEOperation::rotateLOp:
    case ROSEOperation::rotateROp:
      uop->type = IB_rotate;
      break;
    case ROSEOperation::shiftLOp:
    case ROSEOperation::shiftROp:
    case ROSEOperation::shiftRArithOp:
      uop->type = IB_shift;
      break;
    case ROSEOperation::addOp:
      uop->type = IB_add;
      break;
    
    //case ROSEOperation::derefOp:{
    //  uop->type = IB_load;
    //  uop->primary = true;
    //  break;
    //}
    case ROSEOperation::writeRepOp:
    case ROSEOperation::writeOp:
      break;
    //case ROSEOperation::ifOp:{
    //  uop->type = IB_br_CC;
    //  uop->primary = true;
    //  break;
    //}
  
    default:{break;}
      //assert(!"get_operator\n");
  }

}

// Get the right operator from the AST (if exists) of an assignment. Stack all the operators in a vector and 
// choosing the operator according to certain rules.
void get_operator(instruction_info* uop, AST::Ptr ast, Assignment::Ptr aptr, Dyninst::InstructionAPI::Instruction::Ptr insn, MIAMI::DecodedInstruction* dInst)
{
  insn_opVec.clear();
  //if (uop->type != IB_unknown) return;

  traverse_rose(ast);
  if (insn_opVec.size() == 0) 
    return;

  for (auto it = insn_opVec.begin(); it != insn_opVec.end(); it++) {
    ROSEOperation::Op op = (*it)->val().op;

    if (op == ROSEOperation::ifOp) {
      uop->type = IB_br_CC;
      break;

    } else if (op == ROSEOperation::extractOp) // Usually extract is not the important operation. (Also, no corresponding IB.)
      continue;

    else if (op == ROSEOperation::extendMSBOp) // As above.
      continue;

    else if (op == ROSEOperation::invertOp) {

      if (uop->type == IB_add){
        uop->type = IB_sub;
        break;    
      } else {
        continue;
      }
    
    } else if (op == ROSEOperation::xorOp)
    {
      uop->type = IB_xor;
      continue; // It can be sub, because we might find invert later, so continue searching.

    } else if (op == ROSEOperation::concatOp)
    {
      uop->type = IB_copy;
      continue; // It might be sub, add, xor. Keep searching.

    } else if (op == ROSEOperation::derefOp)
    {
      if (uop->type != IB_add){ // A lot of special cases!
         uop->type = IB_load;
         break;
      }

    } else {
      convert_operator(uop, op); // Direct translate other cases and assess them later.
      continue;
    }

  }
}

// Traversing through AST of the jump assignment and calculate the value of its constant operand.
void calculate_jump_val(AST::Ptr ast)
{
  if (ast->getID() == AST::V_RoseAST) {
    RoseAST::Ptr ast_r = RoseAST::convert(ast);
    if (ast_r->val().op == ROSEOperation::addOp) {

      for (unsigned int i = 0; i < ast->numChildren(); ++i) {
        if (ast->child(i)->getID() == AST::V_ConstantAST) {
          ConstantAST::Ptr ast_c = ConstantAST::convert(ast->child(i));
          insn_jump_tgt += ast_c->val().val;
	  
        }
	else if (ast->child(i)->getID() == AST::V_VariableAST){
          VariableAST::Ptr ast_v = VariableAST::convert(ast->child(i));
          insn_jump_tgt += ast_v->val().addr; // CHECK --> not all variables are rip

        }
	else {
          calculate_jump_val(ast->child(i));
        }
      }
    }
    else if (ast_r->val().op == ROSEOperation::ifOp) {
      calculate_jump_val(ast->child(1)); // need to CHECK

    }
    else {
      //return;
      //assert(!"Must be add operation for address calculation\n");
    }
  }
}

// Return register index relative to registers in the instruction.
unsigned int get_register_index(Absloc a)
{
  
  for (unsigned int i = 0; i < insn_locVec.size(); ++i) {
    if (insn_locVec.at(i).format().compare(a.format()) == 0){
      return i;
    }
  }
  insn_locVec.push_back(a);
  return insn_locVec.size() - 1;
}

// If x86 or x86_64, return the representation of all the flags.
MachRegister get_machine_flag_reg(){
  if (NULL == insn_myBlock)
    return Dyninst::MachRegister();
  
  if (insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86_64)
    return x86_64::flags;  

  else if (insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86)
    return x86::flags; 

  else 
    return Dyninst::MachRegister();

}


// Return if the variable ast is a flag register
bool sourceOpIsFlag(VariableAST::Ptr ast_var)
{
  Absloc aloc = ast_var->val().reg.absloc();
  if (aloc.type() == Absloc::Register) {
    RegisterAST reg = RegisterAST(aloc.reg());
    MachRegister mr = reg.getID().getBaseRegister();
    return check_flags_registers(mr);
  }
  return false;
}


// Return if the output field of a assignment is a flag register
bool destOpIsFlag(Assignment::Ptr aptr)
{
  Absloc aloc = aptr->out().absloc();
  if (aloc.type() == Absloc::Register) {
    RegisterAST reg = RegisterAST(aloc.reg());
    MachRegister mr = reg.getID().getBaseRegister();
    return check_flags_registers(mr);
  }
  

  return false;
}


// Update the primary micro-op with a new destination operand--flags
void update_dest_with_flag(MIAMI::DecodedInstruction* dInst, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  for (auto oit = dInst->micro_ops.begin(); oit != dInst->micro_ops.end(); oit++) {
    if (oit->primary) {
      oit->dest_opd[oit->num_dest_operands++] = make_operand(OperandType_REGISTER, oit->num_dest_operands); // which index should I use
      append_dest_regs(&(*oit), OperandType_REGISTER, insn, get_machine_flag_reg(), 1);
    }
  }
}

// Check whether a machine register is flag in x86 or x86_64
bool check_flags_registers(MachRegister mr)
{
  if (insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86_64){
    if (mr == x86_64::cf 
	|| mr == x86_64::pf || mr == x86_64::af || mr == x86_64::zf
	|| mr == x86_64::sf || mr == x86_64::tf || mr == x86_64::df
	|| mr == x86_64::of
	|| mr == x86_64::nt_ || mr == x86_64::if_ || mr == x86_64::flags) {
      return true; 
    }
  }
  else if (insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86){
    if (mr == x86::cf || mr == x86::pf || mr == x86::af || mr == x86::zf
        || mr == x86::sf || mr == x86::tf || mr == x86::df || mr == x86::of
        || mr == x86::nt_ || mr == x86::if_ || mr == x86::flags) {
      return true; 
    }
  } 
  return false;
}

// Return if a register is a 'stack' register (used for stack)
int check_stack_register(MachRegister mr)
{
  if (insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86_64)
  { 
    if ((x86_64::st0 <= mr && mr <= x86_64::st7) || mr == x86_64::mm0) {
      return 1;
   } 
  } else if (insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86){
    if (mr == x86::mm0)
    {
      return 1;
    }
  }
    

   return 0;
}

// If the output (which usually determine the vector length) is vector register operand, 
// then return its vector length. Otherwise, return vector length as 1.
// The following list is not complete and havent' include cases in other architectures.
int check_vector_regs(Dyninst::InstructionAPI::Instruction::Ptr insn, Assignment::Ptr aptr)
{
  if (aptr->out().absloc().type() == Absloc::Register) 
  {
    RegisterAST reg = RegisterAST(aptr->out().absloc().reg());
    MachRegister mr = reg.getID().getBaseRegister();
    if (insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86)
    {
      if (mr == x86::xmm0 || mr == x86::xmm1 || mr == x86::xmm2 || mr == x86::xmm3 \
      || mr == x86::xmm4 || mr == x86::xmm5 || mr == x86::xmm6 || mr == x86::xmm7)
        return 4;

    } else if (insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86_64)
    {
      if ( mr == x86_64::xmm0 || mr == x86_64::xmm1 || mr == x86_64::xmm2 || mr == x86_64::xmm3 \
      || mr == x86_64::xmm4 || mr == x86_64::xmm5 || mr == x86_64::xmm6 || mr == x86_64::xmm7 ) 
        return 4;  
    }
  }
  return 0;
}

// Append source registers to the src_reg_list. 
// Register information not used right now. If we want to build dependency graph,
// we simply just pass these to internal field over instead use MIAMI's original function.
void append_src_regs(instruction_info* uop, OperandType opt, Dyninst::InstructionAPI::Instruction::Ptr insn, MachRegister mr, int size)
{
   int s = 0; // stack
   switch(opt){
      case OperandType_REGISTER:{
         RegisterClass rclass = RegisterClass_REG_OP;
         if (check_stack_register(mr)) {
            rclass = RegisterClass_STACK_REG;
            s = 1;
         } 
         uop->src_reg_list.push_back(register_info(mr, rclass, get_src_reg_lsb(mr), size - 1, s));
         return;
      }
      case OperandType_INTERNAL:{
         uop->src_reg_list.push_back(register_info(mr, RegisterClass_TEMP_REG, get_src_reg_lsb(mr), size - 1));
         return;
      }
      case OperandType_MEMORY: {
         RegisterClass rclass = RegisterClass_MEM_OP;
         if (insn->getOperation().getID() == e_lea)
         {
            rclass = RegisterClass_LEA_OP;
         }  else if (check_stack_register(mr))
         {
            rclass = RegisterClass_STACK_REG;
            s = 1;
         }
         uop->src_reg_list.push_back(register_info(mr, rclass, get_src_reg_lsb(mr), size - 1, s));
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
void append_dest_regs(instruction_info* uop, OperandType opt, Dyninst::InstructionAPI::Instruction::Ptr insn, MachRegister mr, int size)
{
   int s = 0;
   switch(opt){
      case OperandType_REGISTER:{
        RegisterClass rclass = RegisterClass_REG_OP;
        if (check_stack_register(mr)) {
          rclass = RegisterClass_STACK_REG;
          s = 1;
        } 
        uop->dest_reg_list.push_back(register_info(mr, rclass, get_src_reg_lsb(mr), size - 1, s)); // fix me-->need to add in
                                                  
        return;
      }
      case OperandType_INTERNAL:{
        uop->dest_reg_list.push_back(register_info(mr, RegisterClass_TEMP_REG, get_src_reg_lsb(mr), size - 1));
        return;
      }
      case OperandType_MEMORY: {
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
void append_all_dest_registers(instruction_info* uop, Dyninst::InstructionAPI::Instruction::Ptr insn, Assignment::Ptr aptr)
{
   for (unsigned int i = 0; i < uop->num_dest_operands; ++i)
   {
      switch(extract_op_type(uop->dest_opd[i])){ // extract the operation type.

         case OperandType_INTERNAL: 
            uop->dest_reg_list.clear();
            append_dest_regs(uop, OperandType_INTERNAL, insn, Dyninst::MachRegister(), uop->width * uop->vec_len);
            break;
         case OperandType_REGISTER: {
            if (i == 0) // This is not flags, we will append flag later 
            {
              if (aptr->out().absloc().type() == Absloc::Register){
                append_dest_regs(uop, OperandType_REGISTER, insn, aptr->out().absloc().reg(), uop->width * uop->vec_len); 
              }else {
                MachRegister pc = Absloc::makePC(insn_myBlock->obj()->cs()->getArch()).reg();
                append_dest_regs(uop, OperandType_REGISTER, insn, pc, pc.size() * 8);  
              }  
            }
            break;
         }
         case OperandType_MEMORY: {
            if (aptr->inputs().size())
            {
               Absloc aloc = aptr->inputs().at(0).absloc(); // I actually don't know how to find the corresponding register 
                                                            // related to the memory field. The xed function related to this 
                                                            // is xed_decoded_inst_get_?*?_reg.
               if (aloc.type() == Absloc::Register)
                  append_src_regs(uop, OperandType_MEMORY, insn, aloc.reg(), uop->width * uop->vec_len);  
               else                                         // Find no register field, use default program counter
                  append_src_regs(uop, OperandType_MEMORY, insn, Absloc::makePC(insn_myBlock->obj()->cs()->getArch()).reg(), uop->width * uop->vec_len);
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
void get_width_from_tree(instruction_info* uop, AST::Ptr ast)
{

   if (uop->width)
      return;
   
   switch(ast->getID()){

      case AST::V_RoseAST:{ // if node is an operation, only special ones can give the size. Normally, we keep recursing.
         RoseAST::Ptr ast_r = RoseAST::convert(ast);
         if (ast_r->val().op == ROSEOperation::derefOp || (ast_r->val().op == ROSEOperation::extractOp && ast_r->val().size != 1))
         {
            uop->width = ast_r->val().size;
         } else {
            for (unsigned int i = 0; i < ast->numChildren(); i++) {
              get_width_from_tree(uop, ast->child(i));
            }
         }
         return;
      } 

      case AST::V_ConstantAST:{ // get the size of the constant
         ConstantAST::Ptr ast_c = ConstantAST::convert(ast);
         uop->width = ast_c->val().size;
         return;
      }

      case AST::V_VariableAST:{ // get the size of the register
         VariableAST::Ptr ast_v = VariableAST::convert(ast);
         if (ast_v->val().reg.absloc().type() == Absloc::Register)
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
void create_add_micro(MIAMI::DecodedInstruction* dInst, Dyninst::InstructionAPI::Instruction::Ptr insn, Assignment::Ptr aptr)
{
  dInst->micro_ops.push_back(instruction_info());
  instruction_info* uop = &dInst->micro_ops.back();
  uop->type = IB_add;
  uop->width = (dInst->micro_ops.begin())->width;
  uop->vec_len = (dInst->micro_ops.begin())->vec_len;
  uop->exec_unit = (dInst->micro_ops.begin())->exec_unit;
  uop->exec_unit_type = (dInst->micro_ops.begin())->exec_unit_type;
  int idx = 0;
  if (aptr->out().absloc().type() == Absloc::Register){
    idx = get_register_index(aptr->out().absloc());
    MachRegister mr = aptr->out().absloc().reg();
    uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_REGISTER, idx);
    append_src_regs(uop, OperandType_REGISTER, insn, mr, mr.size() * 8); 
    uop->dest_opd[uop->num_dest_operands++] = make_operand(OperandType_REGISTER, idx); // -->using the index of the first Absloc::Register
    append_dest_regs(uop, OperandType_REGISTER, insn, mr, mr.size() * 8); 
 
  }
  
  uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; 
  uop->imm_values[uop->num_imm_values].value.s = 8; // 
  uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_IMMED, uop->num_imm_values++);


}

// Create load micro operation.
void create_load_micro(MIAMI::DecodedInstruction* dInst, Dyninst::InstructionAPI::Instruction::Ptr insn, Assignment::Ptr aptr, int idx)
{
  dInst->micro_ops.push_front(instruction_info());
  instruction_info* uop = &dInst->micro_ops.front();
  uop->type = IB_load;

  static std::pair<AST::Ptr, bool> astPair;
  astPair = SymEval::expand(aptr, false);
  // getting the width 
  if (astPair.second && NULL != astPair.first){
     get_width_from_tree(uop, astPair.first); // get width from AST

  } else {
    // if no AST --> get width from assignment's output or input register field
    if (aptr->out().absloc().type() == Absloc::Register) {
      uop->width = aptr->out().absloc().reg().size() * 8;
    } else if (aptr->inputs().size()) {

      for (unsigned int i = 0; i < aptr->inputs().size(); ++i)
      {
         if (aptr->inputs().at(i).absloc().type() == Absloc::Register) { 
            uop->width = aptr->inputs().at(i).absloc().reg().size() * 8;
            break;
         }
      }
    }
  } 
   
  if(!uop->vec_len) uop->vec_len = get_vec_len(insn);
  uop->exec_unit = (uop->vec_len == 1) ? ExecUnit_SCALAR : ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn);

  // load uop's source must be memory field (idx here not important)
  uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_MEMORY, 0); 

  if (insn_numAssignments > 1 || dInst->micro_ops.size() > 1) // Store is not the primary operation, then it load into internal
    uop->dest_opd[uop-> num_dest_operands++] = make_operand(OperandType_INTERNAL, idx);  
  else // Load into register
    uop->dest_opd[uop-> num_dest_operands++] = make_operand(OperandType_REGISTER, 0);
  
  // Like memtioned above, I actually don't know how to find the corresponding register 
  // related to the memory field. The xed function related to this is xed_decoded_inst_get_?*?_reg.
  // Usually, chances are the first/second register in the inputs field represents the memory. 
  // If cannot find any not, use the default choice--program counter.
  MachRegister pc = Absloc::makePC(insn_myBlock->obj()->cs()->getArch()).reg();
  if (aptr->inputs().size() && aptr->inputs().at(0).absloc().type() == Absloc::Register) 
  {
     MachRegister mr = aptr->inputs().at(0).absloc().reg();
     append_src_regs(uop, OperandType_MEMORY, insn, mr, mr.size() * 8);
  } else if (aptr->inputs().size() > 1 && aptr->inputs().at(1).absloc().type() == Absloc::Register) // CHECK OTHER CASES
  {
     MachRegister mr = aptr->inputs().at(1).absloc().reg();
     append_src_regs(uop, OperandType_MEMORY, insn, mr, mr.size() * 8);
  } else 
  {
     append_src_regs(uop, OperandType_MEMORY, insn, pc, pc.size() * 8); 
  }

  if (insn_numAssignments > 1 || dInst->micro_ops.size() > 1) // Load info into internal
    append_dest_regs(uop, OperandType_INTERNAL, insn, Dyninst::MachRegister(), uop->width * uop->vec_len);
  else { 
    // special case,usually with st0 which comes from parse_assign. We will append registers later
  }
     
}

// If output field is memory, we create a store uop (Dyninst AST usually shows no information of load/store).
void create_store_micro(MIAMI::DecodedInstruction* dInst, Dyninst::InstructionAPI::Instruction::Ptr insn, Assignment::Ptr aptr, int idx)
{
  dInst->micro_ops.push_back(instruction_info());
  instruction_info* uop = &dInst->micro_ops.back();
  uop->type = IB_store;
  static std::pair<AST::Ptr, bool> astPair;
   astPair = SymEval::expand(aptr, false);

  if (astPair.second && NULL != astPair.first)
     get_width_from_tree(uop, astPair.first);
  else 
     uop->width = dInst->micro_ops.front().width;
  
  uop->vec_len = get_vec_len(insn);
  uop->exec_unit =  (uop->vec_len == 1) ? ExecUnit_SCALAR : ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn);

  // If the instruction has more than one assignment, the store/load usually interact with temporary internal operand.
  uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_INTERNAL, idx); 
  append_src_regs(uop, OperandType_INTERNAL, insn, Dyninst::MachRegister(), 64); // Does the size for internal field matter?

  uop->dest_opd[uop->num_dest_operands++] = make_operand(OperandType_MEMORY, 0); // FIXME--index
  for (unsigned int i = 0; i < aptr->inputs().size(); ++i)
  {
    Absloc aloc = aptr->inputs().at(i).absloc();
     if (aloc.type() == Absloc::Register && aptr->inputs().size() > 1)
     {
        append_src_regs(uop, OperandType_MEMORY, insn, aloc.reg(), uop->width * uop->vec_len); // CHECK!!!! Not the previous one?
        return;
     }
  }
  // If no register information found for the memory field, use default choice PC register --> check if this is the case
  append_src_regs(uop, OperandType_MEMORY, insn, Absloc::makePC(insn_myBlock->obj()->cs()->getArch()).reg(), 64);
  return;

}

// Create jump micro.
void create_jump_micro(MIAMI::DecodedInstruction* dInst, int jump_val, Dyninst::InstructionAPI::Instruction::Ptr insn, Assignment::Ptr assign, bool mem)
{
  dInst->micro_ops.push_back(instruction_info());
  instruction_info* uop = &dInst->micro_ops.back();
  uop->type = IB_jump;
  uop->primary = true; // whether in cond/uncond branch or call, jump is always the primary uop
  uop->width = (dInst->micro_ops.begin())->width;
  uop->vec_len = (dInst->micro_ops.begin())->vec_len;
  uop->exec_unit = (dInst->micro_ops.begin())->exec_unit;
  uop->exec_unit_type = (dInst->micro_ops.begin())->exec_unit_type;

  unsigned int idx = 0;
  if (assign->inputs().at(0).absloc().type() == Absloc::Register){
    idx = get_register_index(assign->inputs().at(0).absloc()); // use the index of register used in store
  }

  MachRegister pc = Absloc::makePC(insn_myBlock->obj()->cs()->getArch()).reg();
  if (mem) { // If we have a load uop, then the source of jump comes from INTERNAL operand.
    uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_INTERNAL, 0);
    append_src_regs(uop, OperandType_INTERNAL, insn, pc, pc.size() * 8);

  } else { // If we only have a jump uop, then the source operand is register.
    uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_REGISTER, idx);
    append_src_regs(uop, OperandType_REGISTER, insn, pc, pc.size() * 8);
    uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; 
    uop->imm_values[uop->num_imm_values].value.s = jump_val; 
    uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_IMMED, uop->num_imm_values++);
  } 

  uop->dest_opd[uop->num_dest_operands++] = make_operand(OperandType_REGISTER, idx);
  append_dest_regs(uop, OperandType_REGISTER, insn, pc, pc.size() * 8); 

}

// Create Return micro for instruction return
void create_return_micro(MIAMI::DecodedInstruction* dInst, Dyninst::InstructionAPI::Instruction::Ptr insn, Assignment::Ptr aptr)
{
  dInst->micro_ops.push_back(instruction_info());
  instruction_info* uop = &dInst->micro_ops.back();
  uop->type = IB_ret;
  uop->primary = true;

  unsigned int idx = 0;
  if (aptr->out().absloc().type() == Absloc::Register){
    idx = get_register_index(aptr->out().absloc());
  }

  // everything corresponds to the first uop IB_load in return case. (?)
  uop->width = (dInst->micro_ops.begin())->width;
  uop->vec_len = (dInst->micro_ops.begin())->vec_len;
  uop->exec_unit = (dInst->micro_ops.begin())->exec_unit;
  uop->exec_unit_type = (dInst->micro_ops.begin())->exec_unit_type;
  uop->dest_opd[uop->num_dest_operands++] = make_operand(OperandType_REGISTER, idx); 

  // no source fields needed for return uop
  if (aptr->out().absloc().type() == Absloc::Register) 
     append_dest_regs(uop, OperandType_REGISTER, insn, aptr->out().absloc().reg(), uop->width * uop->vec_len);
}

// Create compare micro.
void create_compare_micro(MIAMI::DecodedInstruction* dInst, std::vector<Assignment::Ptr> assignments, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  dInst->micro_ops.push_back(instruction_info());
  instruction_info* uop = &dInst->micro_ops.back();
  uop->type = IB_cmp;
  uop->primary = true;

  if(!uop->vec_len) uop->vec_len = get_vec_len(insn);
  uop->exec_unit = (uop->vec_len == 1) ? ExecUnit_SCALAR : ExecUnit_VECTOR;
  uop->exec_unit_type = get_execution_type(insn);

  for (unsigned int i = 0; i < assignments.at(0)->inputs().size(); i++) { 
    Absloc aloc = assignments.at(0)->inputs().at(i).absloc();

    if (aloc.type() == Absloc::Register){ // append all the register source operand
      if (uop->width == 0 && uop->vec_len) {

        if (check_is_zf(aloc.reg()))  // In Dyninst, only the assignment with zf register as the output field
                                      // shows the width in its AST. (in compare cases)
        {
          static std::pair<AST::Ptr, bool> astPair;
          astPair = SymEval::expand(assignments.at(i), false);

          if (astPair.second && NULL != astPair.first)
            get_width_from_tree(uop, astPair.first);
        }

        uop->width = aloc.reg().size() * 8 / uop->vec_len;
      }

      unsigned int idx = get_register_index(aloc);
      uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_REGISTER, idx);
      append_src_regs(uop, OperandType_REGISTER, insn, aloc.reg(), aloc.reg().size() * 8);
    } 
  }

  // If we have another uop (load), the compare's source operand changes to INTERNAL
  // Replace REGISTER operand by INTERNAL operand.
  if (dInst->micro_ops.size() > 1) {
    uop->src_opd[0] = make_operand(OperandType_INTERNAL, 0); 
    uop->src_reg_list.clear();
    append_src_regs(uop, OperandType_INTERNAL, insn, Dyninst::MachRegister(), 64);
  }

  // We append constant values from the operands (if any).
  std::vector<Operand> operands;
  insn->getOperands(operands);
  for (unsigned int i = 0; i < operands.size(); ++i) {
    const Result *r = &operands.at(i).getValue()->eval();

    if (r->defined) {
      uop->imm_values[uop->num_imm_values].is_signed = (uop->width == 64) ? true : false; 
      uop->imm_values[uop->num_imm_values].value.s = r->convert<long int>();
      uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_IMMED, uop->num_imm_values++);

    } else { // If the operand is not a constant value, we get the width value from it (if we need it).

      if(!uop->width && uop->vec_len) uop->width = r->size() * 8 / uop->vec_len;
    }

  }
  uop->dest_opd[uop->num_dest_operands++] = make_operand(OperandType_REGISTER, 2); // indexing of flag register
  append_dest_regs(uop, OperandType_REGISTER, insn, get_machine_flag_reg(), 1);
}

//***************************************************************************

// For x86 or x86_64, check whether a register is zf flag register.
bool check_is_zf(MachRegister mr)
{
  if (insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86) 
  {
    if (mr == x86::zf)
      return true;
    
  } else if (insn_myBlock->obj()->cs()->getArch() == Dyninst::Architecture::Arch_x86_64)
  {
    if (mr == x86_64::zf)
      return true;
    
  }
  return false;
}

// Check operands size to see whether we are dealing with vector operand
int get_vec_len(Dyninst::InstructionAPI::Instruction::Ptr insn){
  std::vector<Operand> operands;
  insn->getOperands(operands);
  for (unsigned int i = 0; i < operands.size(); ++i) {
    const Result* r = &operands.at(i).getValue()->eval();
    if (r->size() > 8) {
      return (int) r->size() / 8;
    }
  }
  return 1;
}


// Get the execution type of an instruction, depending on its operands' type.
ExecUnitType get_execution_type(Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  std::vector<Operand> operands;
  insn->getOperands(operands);
  for (unsigned int i = 0; i < operands.size(); ++i) {
    const Result* r = &operands.at(i).getValue()->eval();
    //if (r->defined) { // don't need the result to be defined to get the type
      switch(r->type){
        case bit_flag:
        case s8:
        case u8:
        case s16:
        case u16:
        case s32:
        case u32:
        case s48:
        case u48:
        case s64:
        case u64:
        // 48-bit pointers
        case m512:
        case m14:
          return ExecUnitType_INT;
        case dbl128:
        case sp_float:
        case dp_float:
        default:
          return ExecUnitType_FP;
      }
    //}
  }
  return ExecUnitType_INT; // FIXME 
}


// Processing the output region of a given assignment. 
void get_dest_field(MIAMI::DecodedInstruction* dInst, Assignment::Ptr aptr, instruction_info* uop)
{
  Absloc out_aloc = aptr->out().absloc();
  if (out_aloc.type() == Absloc::Register) {
    unsigned int idx = get_register_index(out_aloc);
    uop->dest_opd[uop->num_dest_operands++] = make_operand(OperandType_REGISTER, idx);

  } else if (out_aloc.type() == Absloc::Stack || out_aloc.type() == Absloc::Heap || out_aloc.type() == Absloc::Unknown) {
    uop->dest_opd[uop->num_dest_operands++] = make_operand(OperandType_MEMORY, 0); // index for memory FIXME
    uop->type = IB_store;
  }
  insn_locVec.push_back(out_aloc);


}

// If assignment has no AST, parse it according to it's input and output field --> multiple cases untranslatable. 
void parse_assign(MIAMI::DecodedInstruction* dInst, Assignment::Ptr aptr, instruction_info* uop, Dyninst::InstructionAPI::Instruction::Ptr insn)
{
  
  std::cerr << "parse_assign\n";

  if (uop->vec_len == 0) 
    uop->vec_len = get_vec_len(insn);
  
  if (uop->exec_unit == ExecUnit_LAST) 
    uop->exec_unit = (uop->vec_len == 1) ? ExecUnit_SCALAR : ExecUnit_VECTOR;
  
  if (uop->exec_unit_type == ExecUnitType_LAST) 
    uop->exec_unit_type = get_execution_type(insn);
  
  if (uop->width == 0 && uop->vec_len) {

    if (aptr->out().absloc().type() == Absloc::Register) {
      uop->width = aptr->out().absloc().reg().size() * 8 / uop->vec_len;
    } else {
      if (aptr->inputs().size() && aptr->inputs().at(0).absloc().type() == Absloc::Register) {
        uop->width = aptr->inputs().at(0).absloc().reg().size() * 8 / uop->vec_len;
      } else {
        uop->width = 64; // no width information available from input/output field, use default value for now
      }
    }
  }

  // start GUESSING what is the operation with limited information--inputs and output fields.
  Absloc aloc_out = aptr->out().absloc();
  std::vector<AbsRegion> inputs = aptr->inputs();

  if (uop->type == IB_unknown) {

    if (aloc_out.type() == Absloc::Register && inputs.size()) {
      Absloc inputs_aloc1 = inputs.at(0).absloc();

      // processing special vector operand
      if (uop->vec_len > 1) { 
        if (inputs.size() > 1) {

          Absloc inputs_aloc2 = inputs.at(1).absloc();
          if (inputs_aloc1.format().compare(inputs_aloc2.format()) != 0) { // two different input fields, usually add
            uop->type = IB_add;
          }
          if (inputs_aloc1.format().compare(inputs_aloc2.format()) == 0) { // two same input fields, usually xor
            uop->type = IB_xor;
          }  
        } else if (inputs.size() == 1) {
          if (inputs_aloc1.type() == Absloc::Register && inputs_aloc1.reg().size() <= 8) { // precision convert from 32 to 64
            uop->type = IB_cvt_prec;
          } 
        }

      // processing scalar operand with the first input region being register
      } else if (uop->vec_len == 1 && inputs_aloc1.type() == Absloc::Register) {

        if (uop->num_src_operands == 0 && aloc_out.format().compare(inputs_aloc1.format()) != 0 && inputs.size() <= 2) { // two different Registers
          
          for (unsigned int i = 0; i < inputs.size(); ++i)  // check (guess) which operation it is
          {
             Absloc tmp = inputs.at(i).absloc();
             if (tmp.type() == Absloc::Heap || tmp.type() == Absloc::Stack || tmp.type() == Absloc::Unknown) {
              uop->type = IB_load;

              // In the case of call, delete the original uop to build a new load uop
              if (dInst->micro_ops.front().type == IB_load && dInst->micro_ops.size() > 1) 
                  dInst->micro_ops.pop_front(); // we pop the front because only we translate call's assignment in reverse order.
              
              // Not in the case of call.
              else if (dInst->micro_ops.back().type == IB_load && dInst->micro_ops.size() == 1) 
                  dInst->micro_ops.pop_back(); 
              

              create_load_micro(dInst, insn, aptr, 0);
              return;
             }
          }
          if (uop->type == IB_unknown) uop->type = IB_copy; // default: IB_copy 

        } else if (uop->num_src_operands == 0 && aloc_out.format().compare(inputs_aloc1.format()) == 0 \
                      && inputs.size() == 1) { // Two same Register. The 'add' before the 'store' decrements the pc by -8
                                               // If certain 'push' instructions has no AST
          uop->type = IB_add;
          uop->imm_values[uop->num_imm_values].is_signed = true; 
          uop->imm_values[uop->num_imm_values].value.s = -8; 
          uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_IMMED, uop->num_imm_values++);
      
        }
      }

      int flags = 0;
      // processing the inputs field
      for (unsigned int i = 0; i < inputs.size(); i++) {
        Absloc aloc = inputs.at(i).absloc();
        if (aloc.type() == Absloc::Stack || aloc.type() == Absloc::Heap || aloc.type() == Absloc::Unknown) {
          if (uop->type == IB_unknown) {
            uop->type = IB_load; // Simply a load uop, different from the above, which creates a new load uop.
            uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_MEMORY, 0);
            insn_locVec.push_back(aloc);
            if (i == inputs.size() - 1) { // If the last field is memory, we use default program counter to represent it.
               MachRegister pc = Absloc::makePC(insn_myBlock->obj()->cs()->getArch()).reg();
               append_src_regs(uop, OperandType_MEMORY, insn, pc, pc.size());

            } else if (i < inputs.size() - 1) { // If not, the memory is represented by the register field after itself. 
               if (inputs.at(i+1).absloc().type() == Absloc::Register) {
                  MachRegister mr = inputs.at(i+1).absloc().reg();
                  append_src_regs(uop, OperandType_MEMORY, insn, mr, mr.size()); 
                  break;
               }
            }
          }
        } else if (aloc.type() == Absloc::Register) {
          if (check_flags_registers(aloc.reg())) {
            flags = 1;
          } else {
            unsigned int idx = get_register_index(aloc);
            uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_REGISTER, idx); // no extract to check
            append_src_regs(uop, OperandType_REGISTER, insn, aloc.reg(), aloc.reg().size() * 8);
          }
        }
      }
      if (flags) {
        uop->src_opd[uop->num_src_operands] = make_operand(OperandType_REGISTER, uop->num_src_operands - 1);
	uop->num_src_operands++;
        append_src_regs(uop, OperandType_REGISTER, insn, get_machine_flag_reg(), 1); // not sure the size of the flag yet
      }
    } 
    
  // Already know from get_dest_field() that it is a store
  } else if (uop->type == IB_store) { 
    Absloc inputs_aloc = inputs.at(0).absloc();
    if (inputs_aloc.type() == Absloc::Register) {
      unsigned int idx = get_register_index(inputs_aloc);
      uop->src_opd[uop->num_src_operands++] = make_operand(OperandType_REGISTER, idx); 
      append_src_regs(uop, OperandType_REGISTER, insn, inputs_aloc.reg(), inputs_aloc.reg().size() * 8);
    }
  }
}


//***************************************************************************

void dynXlate_dumpAssignmentAST(AST::Ptr ast, std::string index, int num);


void dynXlate_dumpInsn(Dyninst::InstructionAPI::Instruction::Ptr insn, std::vector<Assignment::Ptr> assignments, addrtype pc)
{
  using std::cerr;
  cerr << "========== DynInst Instruction "
       << "(" << std::hex << (void*)pc << std::dec << "): " << insn->format()
       << " [" << insn->size() << " bytes] "
       << " ==========\n";
  
  for (unsigned int i = 0; i < assignments.size(); i++) {
    cerr << "assignment " << i << ":" << assignments.at(i)->format() << "\n";
    std::pair<AST::Ptr, bool> assignPair = SymEval::expand(assignments.at(i), false);
    if (assignPair.second && (NULL != assignPair.first)) {
      dynXlate_dumpAssignmentAST(assignPair.first, "0", assignPair.first->numChildren());
    }
  }
}


void dynXlate_dumpAssignmentAST(AST::Ptr ast, std::string index, int num)
{
  using std::cerr;
  cerr << index << ": ";
  
  if (ast->getID() == AST::V_RoseAST) {
    RoseAST::Ptr self = RoseAST::convert(ast);
    cerr << "RoseAST <op:size>: " << self->format() << "\n";
    //cerr << "RoseOperation val is " << self->val().format() << "\n";
    //cerr << "RoseOperation size is " << self->val().size << "\n";
    //cerr << "RoseOperation op is " << self->val().op << "\n";
  }
  else if (ast->getID() == AST::V_ConstantAST) {
    ConstantAST::Ptr self = ConstantAST::convert(ast);
    cerr << "ConstantAST <val:size>: " << self->format() << "\n";
    //cerr << "Constant val is " << self->val().val << "\n";
    //cerr << "bit size of the val is " << self->val().size << "\n";

  }
  else if (ast->getID() == AST::V_VariableAST) {
    VariableAST::Ptr self = VariableAST::convert(ast);
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
    dynXlate_dumpAssignmentAST(ast->child(i), index_str, ast->child(i)->numChildren());
    i++; 
  }
}


//***************************************************************************
// 
//***************************************************************************

#if 0

// cf. Routine::compute_lineinfo_for_block()
void compute_lineinfo_for_block_dyninst(LoadModule *lm, ScopeImplementation *pscope, CFG::Node *b) {
   std::vector<unsigned long> addressVec = get_instructions_address_from_block(b);
   //std::cout << "routine: compute_lineinfo_for_block_dyninst: 8\n";
   if (!addressVec.size())
   {
      assert("No instructions available");
      return;
   }
   std::string func, file;
   int32_t lineNumber1 = 0, lineNumber2 = 0;
   
#if DEBUG_COMPUTE_LINEINFO
   std::cerr << "LineInfo for block [" << std::hex << b->getStartAddress() 
             << "," << b->getEndAddress() << "]" << std::dec << " exec-count " 
             << b->ExecCount() << std::endl;
#endif
   // iterate over variable length instructions. Define an architecture independent API
   // for common instruction decoding tasks. That API can be implemented differently for
   // each architecture type
   //CFG::ForwardInstructionIterator iit(b);
   for (unsigned int i = 0; i < addressVec.size(); ++i)
   {
      addrtype pc = addressVec.at(i);
      lm->GetSourceFileInfo(pc, 0, file, func, lineNumber1, lineNumber2);
      unsigned int findex = mdriver.GetFileIndex(file);
      mdriver.AddSourceFileInfo(findex, lineNumber1, lineNumber2);
      pscope->addSourceFileInfo(findex, func, lineNumber1, lineNumber2);
     // std::cout << "routine: lineMappings size is " << pscope->GetLineMappings().size() << endl;
   }
}
#endif


//***************************************************************************
// 
//***************************************************************************

#if 0 // FIXME:delete


// FIXME: tallent: static data. Should be part of LoadModule
static Dyninst::ParseAPI::SymtabCodeSource* lm_codeSource; // MIAMI::LoadModule
static BPatch_image* lm_image;                             // MIAMI::LoadModule

static std::vector<BPatch_function*>* lm_functions;  // MIAMI::Routine

// Convert to per-Routine map: address-range -> basic-block
static std::map<int, std::set<BPatch_basicBlock*>> lm_func2blockMap;

// These will no longer be needed
// FIXME: tallent: static data. Should be part of Routine... or not...
static std::vector<BPatch_basicBlock*> func_blockVec;


// incorporate isaXlate_insn() into InstructionDecoder::decode_dbg()


void dyninst_note_loadModule(uint32_t id, std::string& file_name, addrtype start_addr, addrtype low_offset)
{
  //FIXME:tallent Dyninst::ParseAPI::SymtabCodeSource* lm_codeSource;
  lm_codeSource = new Dyninst::ParseAPI::SymtabCodeSource((char*)file_name.c_str());

  BPatch bpatch;
  BPatch_addressSpace* app = bpatch.openBinary(file_name.c_str(),false);
  lm_image = app->getImage(); // global varible

  std::vector<BPatch_object*> objs;
  lm_image->getObjects(objs);

#if 0  
  unsigned long start_addr = get_start_address(&objs, file_name);
  //unsigned long low_offset = get_low_offset(&objs, file_name);
  unsigned long low_offset = lm_codeSource->loadAddress();
#endif

  lm_functions = lm_image->getProcedures(true);

  for (unsigned int i = 0; i < lm_functions->size(); ++i) {
    BPatch_flowGraph *fg = (*lm_functions)[i]->getCFG();
    std::set<BPatch_basicBlock*> blks;
    fg->getAllBasicBlocks(blks);
    lm_func2blockMap[i] = blks;
  }
}


int get_routine_number()
{
  return (int) lm_functions->size();
}


unsigned long get_start_address(std::vector<BPatch_object*> * objs, std::string file_name)
{
  for (unsigned int i = 0; i < objs->size(); ++i) {
    if (objs->at(i)->pathName().compare(file_name) == 0) 
      return objs->at(i)->fileOffsetToAddr(0);
    
  }
  return 0;
}


unsigned long get_low_offset(std::vector<BPatch_object*> * objs, std::string file_name)
{
  for (unsigned int i = 0; i < objs->size(); ++i) {
    if (objs->at(i)->pathName().compare(file_name) == 0) 
      return objs->at(i)->fileOffsetToAddr(1);
    
  }
  return 0;
}

#endif



//***************************************************************************
// 
//***************************************************************************

// FIXME: tallent: For one instruction, this is O(|functions| *
// |blocks| * |insn-in-block|).  Should grab function and block only
// as necessary.

static int last_used_function = 0; // FIXME

// Get the corresponding function according to pc.
int isaXlate_getFunction(unsigned long pc)
{
  Dyninst::Address start, end;
	
  for (unsigned int i = 0; i < lm_functions->size(); ++i) {
    (*lm_functions)[i]->getAddressRange(start, end);
    if (start <= pc && pc < end) {
      return i;  
    }
  }

  assert(false); // FIXME
  
  // no demangled name match, check again using function's base address
  for (unsigned int i = 0; i < lm_functions->size(); ++i) {
    if ((unsigned long) (*lm_functions)[i]->getBaseAddr() == pc) {
      last_used_function = i;
      return i;
    }
  }

  // what about the last block without a name
  for (unsigned int i = 0; i < lm_functions->size() - 1; ++i) {
    if ((pc > (unsigned long) (*lm_functions)[i]->getBaseAddr()) && (pc < (unsigned long) (*lm_functions)[i+1]->getBaseAddr())) {
      return i;
    }
  }

  return last_used_function;
}


// Get the Block for a given PC with function index f.
Dyninst::ParseAPI::Block* isaXlate_getBlock(int f, addrtype pc)
{
  std::set<BPatch_basicBlock*>& blkSet = lm_func2blockMap[f];
  
  for (auto bit = blkSet.begin(); bit != blkSet.end(); bit++) {
    BPatch_basicBlock* bb = *bit;
    if (bb->getStartAddress() <= pc && pc < bb->getEndAddress()) { // end address pass the last insn
      insn_myBlock = Dyninst::ParseAPI::convert(bb);
      if (NULL == insn_myBlock) {
	assert("Cannot find the function block\n");
      }
      
      return insn_myBlock;
    }
  }
  
  return NULL;
}


// Get all the assignments 'logical instruction' of a given instruction.
void isaXlate_getDyninstInsn(addrtype pc, BPatch_function *f, std::vector<Assignment::Ptr> * assignments, Dyninst::InstructionAPI::Instruction::Ptr* insn)
{
  assert(NULL != insn_myBlock);

  Architecture arch = insn_myBlock->obj()->cs()->getArch();

  ParseAPI::Function* func = ParseAPI::convert(f);

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
  InstructionDecoder dec(buf, InstructionDecoder::maxInstructionLength, arch);
  (*insn) = dec.decode();

  AssignmentConverter ac(true);
  ac.convert((*insn), pc, func, insn_myBlock, *assignments);
  
#endif
}


//***************************************************************************
// 
//***************************************************************************

int dyninst_build_CFG(MIAMI::CFG* cfg, std::string func_name);
void traverse_cfg(MIAMI::CFG* cfg, BPatch_basicBlock* bb, CFG::Node* nn,
		  std::map<int, BPatch_basicBlock*>& blockMap);


void dyninst_note_routine(MIAMI::LoadModule* lm, int i)
{
  BPatch_function* func = lm_functions->at(i);
  std::string func_name = func->getName();

  Dyninst::Address _start, _end;
  if (!(func->getAddressRange(_start, _end))) {
    assert("dyninst_note_routine: Address not available!");
  }

#if 0
  Routine * rout = new Routine(lm, _start, _end - _start, func_name, _start/*offset*/, lm->RelocationOffset());

  MIAMI::CFG* cfg = rout->ControlFlowGraph();
  assert(cfg);
#endif

#if 0 // FIXME:tallent: something appears to be broken
  dyninst_build_CFG(cfg, func_name);
#endif
}


int dyninst_build_CFG(MIAMI::CFG* cfg, std::string func_name)
{
  if (!lm_functions->size()) return 0;

  // FIXME: tallent: this searches all routine
  for (unsigned int i = 0; i < lm_functions->size(); ++i)
  {
    if (lm_functions->at(i)->getName().compare(func_name))
    {
      BPatch_flowGraph *fg = lm_functions->at(i)->getCFG();
      fg->createSourceBlocks();
      std::vector<BPatch_basicBlock*> entries;
      fg->getEntryBasicBlock(entries);

      if (entries.size())
        cfg->setCfgFlags(CFG_HAS_ENTRY_POINTS);

      std::map<int, BPatch_basicBlock*> blockMap;
      func_blockVec.clear(); // FIXME:tallent
      
      for (unsigned int i = 0; i < entries.size(); ++i)
      {
        CFG::Node* nn = new CFG::Node(cfg, entries.at(i)->getStartAddress(), entries.at(i)->getEndAddress(), MIAMI::PrivateCFG::MIAMI_CODE_BLOCK);
        traverse_cfg(cfg, entries.at(i), nn, blockMap);
      }
      cfg->computeTopNodes();
      cfg->removeCfgFlags(CFG_GRAPH_IS_MODIFIED);
      cfg->ComputeNodeRanks();
    }
  }
  return 1;
}


// Should make special cases for repeatition block and call surrogate block like MIAMI
// does, or should I directly translating Dyninst block into MIAMI block.

void traverse_cfg(MIAMI::CFG* cfg, BPatch_basicBlock* bb, CFG::Node* nn,
		  std::map<int, BPatch_basicBlock*>& blockMap)
{
  func_blockVec.push_back(bb);

  if (NULL == blockMap[bb->getBlockNumber()])
  {
    blockMap[bb->getBlockNumber()] = bb;

    if (NULL != bb->getCallTarget()) {
      nn = new CFG::Node(cfg, bb->getStartAddress(), bb->getStartAddress(), MIAMI::PrivateCFG::MIAMI_CALL_SITE);
      nn->setTarget(bb->getEndAddress()); // What does this mean?
    }
    cfg->add(nn);

    if (bb->isEntryBlock())
    {
      nn->setRoutineEntry();
      cfg->topNodes.push_back(nn);
      CFG::Edge* edge = new CFG::Edge(static_cast<CFG::Node*>(cfg->cfg_entry), nn, MIAMI::PrivateCFG::MIAMI_CFG_EDGE);
      cfg->add(edge);
    } 

    std::vector<BPatch_basicBlock*> targets;
    bb->getTargets(targets);

    for (unsigned int i = 0; i < targets.size(); ++i)
    {
      CFG::CFG::Node* newNode = new CFG::Node(cfg, targets.at(i)->getStartAddress(), targets.at(i)->getEndAddress(), MIAMI::PrivateCFG::MIAMI_CODE_BLOCK);
      CFG::Edge* newEdge = new CFG::Edge(nn, newNode, MIAMI::PrivateCFG::MIAMI_CFG_EDGE);
      cfg->add(newEdge);        
      traverse_cfg(cfg, targets.at(i), newNode, blockMap);
    }  
    
    if (!targets.size()) //sink
    {
      CFG::Edge* edge = new CFG::Edge(nn, static_cast<CFG::Node*>(cfg->cfg_exit), MIAMI::PrivateCFG::MIAMI_CFG_EDGE);
      cfg->add(edge);
    }
  }
  return;
}


// This function is not used.
std::vector<unsigned long> get_instructions_address_from_block(MIAMI::CFG::Node *b)
{
  std::vector<unsigned long> addressVec;
  unsigned long start, end, cur;
  start = b->getStartAddress();
  end = b->getEndAddress();
  if (!func_blockVec.size())
  {
    assert("No blocks available.\n");
  }
  unsigned int i;
  for (i = 0; i < func_blockVec.size(); ++i)
  {
    if (func_blockVec.at(i)->getStartAddress() <= start && func_blockVec.at(i)->getEndAddress() > start)
    {
      break;
    }
  }

  BPatch_basicBlock* bb = func_blockVec.at(i);
  std::vector<Dyninst::InstructionAPI::Instruction::Ptr> insns;
  bb->getInstructions(insns);
  cur = start;

  for (unsigned int i = 0; i < insns.size(); ++i)
  {
    if (cur < end)
    {
      addressVec.push_back((unsigned int) cur);
      cur += insns.at(i)->size();
    } else { break; }
  }
  if (addressVec.size())
  {
    return addressVec;
  } else {
    assert("No block found given the address.\n");
    return addressVec;
  }
}


//***************************************************************************

void traverseCFG(BPatch_basicBlock* blk, std::map<BPatch_basicBlock *,bool> seen, std::map<std::string,std::vector<BPatch_basicBlock*> >& paths, std::vector<BPatch_basicBlock*> path, string pathStr, graph& g);


// Not used in the program
void startCFG(BPatch_function* function,std::map<std::string,std::vector<BPatch_basicBlock*> >& paths, graph& g)
{
  if (function!= 0 ) {
    std::map<BPatch_basicBlock *,bool> seen;
    
    BPatch_flowGraph *fg =function->getCFG();		
    
    std::vector<BPatch_basicBlock *> entryBlk;
    std::vector<BPatch_basicBlock *> exitBlk;
    fg->getExitBasicBlock(exitBlk);
    fg->getEntryBasicBlock(entryBlk);
    
    std::string pathStr;
    if (entryBlk.size() > 0 and exitBlk.size()>0){
      g.entry=g.basicBlockNoMap[entryBlk[0]->blockNo()];
      g.exit=g.basicBlockNoMap[exitBlk[0]->blockNo()];
      std::vector<BPatch_basicBlock*> path;
      for(unsigned int b=0;b<entryBlk.size();b++){
        //std::cerr << "startCFG entryblk size is "<<  entryBlk.size() << endl;
	traverseCFG(entryBlk[b],seen,paths,path,pathStr,g);
      }
    }
  }
}


// Called by startCFG. Not used in this program.
void traverseCFG(BPatch_basicBlock* blk, std::map<BPatch_basicBlock *,bool> seen, std::map<std::string,std::vector<BPatch_basicBlock*> >& paths, std::vector<BPatch_basicBlock*> path, string pathStr, graph& g)
{
  seen[blk]=true;
	
  std::string graphBlkNo = std::to_string((long long)g.basicBlockNoMap[blk->blockNo()]);
  pathStr+=graphBlkNo+"->";
	
  path.push_back(blk);
	
  std::vector<BPatch_basicBlock*> targets;
  blk->getTargets(targets);
  for(unsigned int t=0;t<targets.size();t++) {
    if (seen.count(targets[t])==0) {
      traverseCFG(targets[t],seen,paths,path,pathStr,g);
    }
  }
  if (g.exit==g.basicBlockNoMap[blk->blockNo()]) {
    paths[pathStr]=path;
  }
}
