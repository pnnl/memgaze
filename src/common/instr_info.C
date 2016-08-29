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
    InstrList::const_iterator lit;
    int j;
    cerr << "Instruction of " << dInst->len << " bytes has " 
         << dInst->micro_ops.size() << " micro-ops:" << endl;
    for (lit=dInst->micro_ops.begin(), j=0 ; lit!=dInst->micro_ops.end() ; ++lit, ++j) {
        cerr << j << ")"
             << " IB:      " << Convert_InstrBin_to_string(lit->type) << endl
             << "   Width:   " << lit->width << endl
             << "   Veclen:  " << (int)lit->vec_len << endl
             << "   ExUnit:  " << ExecUnitToString(lit->exec_unit) << endl
             << "   ExType:  " << ExecUnitTypeToString(lit->exec_unit_type) << endl
             << "   Primary: " << (lit->primary?"yes":"no") << endl;
        
        cerr << "   SrcOps: " << (int)lit->num_src_operands;
        for (uint8_t i=0 ; i<lit->num_src_operands ; ++i) {
            OperandType ot = (OperandType)extract_op_type(lit->src_opd[i]);
            cerr << "  (" << OperandTypeToString(ot)
                 << "/" << extract_op_index(lit->src_opd[i]) << ")";
        }
        
        cerr << endl << "   DstOps: " << (int)lit->num_dest_operands;
        for (uint8_t i=0 ; i<lit->num_dest_operands ; ++i) {
            OperandType ot = (OperandType)extract_op_type(lit->dest_opd[i]);
            cerr << "  (" << OperandTypeToString(ot)
                 << "/" << extract_op_index(lit->dest_opd[i]) << ")";
        }

        cerr << endl << "   ImmValues: " << (int)lit->num_imm_values;
        for (uint8_t i=0 ; i<lit->num_imm_values ; ++i) {
            cerr << "  (" << (lit->imm_values[i].is_signed?"s":"u") << "/";
            if (lit->imm_values[i].is_signed)
               cerr << lit->imm_values[i].value.s << "/" << hex 
                    << lit->imm_values[i].value.s << dec;
            else
               cerr << lit->imm_values[i].value.u << "/" << hex 
                    << lit->imm_values[i].value.u << dec;
            cerr << ")";
        }
        
        cerr << dec << endl;
   }
}

    
}
