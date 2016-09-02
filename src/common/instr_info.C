/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: instr_info.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Support for generic MIAMI micro-operations.
 */

#include <assert.h>
#include <iostream>
#include "instr_info.H"

namespace MIAMI
{

using namespace std;

const char*
OperandTypeToString(OperandType o)
{
    switch (o) {
    case OperandType_MEMORY:
        return "MEMORY";
    case OperandType_REGISTER:
        return "REGISTER";
    case OperandType_INTERNAL:
        return "INTERNAL";
    case OperandType_IMMED:
        return "IMMED";
    default:
        return ("unknown OperandType");
    }
}

void
DumpInstrList(const DecodedInstruction *dInst)
{
  using std::cerr;

  cerr << "========== MIAMI Instruction "
       << "(" << std::hex << (void*)dInst->pc << std::dec
       << "; uops=" << dInst->micro_ops.size()
       <<  "; bytes=" << dInst->len << ")" << " ==========\n";
  

  int i = 0;
  for (auto it = dInst->micro_ops.begin(); it != dInst->micro_ops.end(); ++it, ++i) {
    cerr << i << ")"
	 << " Bin, Width, VecLen: "
	 << Convert_InstrBin_to_string(it->type) << ", "
	 << it->width << ", "
	 << (int)it->vec_len
      	 << (it->primary? " (primary)" : "") << endl;
    cerr << "   ExUnit, ExType:  " << ExecUnitToString(it->exec_unit) << ", "
	 << ExecUnitTypeToString(it->exec_unit_type) << endl;
        
    cerr << "   SrcOps(" << (int)it->num_src_operands << ")/Reg: ";
    for (uint8_t i = 0; i < it->num_src_operands; ++i) {
      OperandType ot = (OperandType)extract_op_type(it->src_opd[i]);
      cerr << " (" << OperandTypeToString(ot)
	   << "/" << extract_op_index(it->src_opd[i]) << ")";
    }

    for (auto it2 = it->src_reg_list.begin(); it2 != it->src_reg_list.end(); ++it2) {
      cerr << " [nm,ty: " << it2->name << ", " << RegisterClassToString(it2->type) << "]";
    }
    cerr << endl;
    
    cerr << "   DstOps(" << (int)it->num_dest_operands << ")/Reg:";
    for (uint8_t i = 0; i < it->num_dest_operands ; ++i) {
      OperandType ot = (OperandType)extract_op_type(it->dest_opd[i]);
      cerr << " (" << OperandTypeToString(ot)
	   << "/" << extract_op_index(it->dest_opd[i]) << ") ";
    }

    for (auto it2 = it->dest_reg_list.begin(); it2 != it->dest_reg_list.end(); ++it2) {
      cerr << " [nm,ty: " << it2->name << ", " << RegisterClassToString(it2->type) << "]";
    }
    cerr << endl;
    
    cerr << "   Imm(" << (int)it->num_imm_values << ")";
    for (uint8_t i = 0; i < it->num_imm_values; ++i) {
      cerr << " (" << (it->imm_values[i].is_signed?"s":"u") << "/";
      if (it->imm_values[i].is_signed)
	cerr << it->imm_values[i].value.s << "/"
	     << hex << it->imm_values[i].value.s << dec;
      else
	cerr << it->imm_values[i].value.u << "/"
	     << hex << it->imm_values[i].value.u << dec;
      cerr << ")";
    }
    cerr << endl;    
  }
}

    
}
