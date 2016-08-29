/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: instr_info.H
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Support for generic MIAMI micro-operations.
 */

//
// Define a structure to store architecture neutral info about an instruction
//

#ifndef INSTRUCTION_INFO_H
#define INSTRUCTION_INFO_H

#include <stdlib.h>
#include <stdio.h>
#include <list>
#include "instr_bins.H"
#include "miami_types.h"
#include "instruction_class.h"
#include "register_class.h"

namespace MIAMI
{

enum OperandType {
    OperandType_MEMORY, 
    OperandType_REGISTER,
    OperandType_INTERNAL,
    OperandType_IMMED,
    OperandType_LAST
};

typedef std::list<register_info> RInfoList;

const int max_num_src_operands = 6;
const int max_num_dest_operands = 4; 
const int max_num_imm_values = 2;   

struct instruction_info
{
#  define make_operand(op_type, op_index)  (((op_type)<<5)|((op_index)&0x1f))
#  define extract_op_type(op)    ((op)>>5)
#  define extract_op_index(op)   ((op)&0x1f)
   
   instruction_info() {
      type = IB_unknown;
      width = 0; vec_len = 0;
      exec_unit = ExecUnit_LAST;
      exec_unit_type = ExecUnitType_LAST;
      primary = false;
      num_src_operands = 0;
      num_dest_operands = 0;
      num_imm_values = 0;
      data = 0;
      flag = false;

   }
   
   int num_src_operands_of_type(OperandType type, int pos = 0) const
   {
      int i, cnt = 0;
      for (i=pos ; i<num_src_operands ; ++i)
         if (extract_op_type(src_opd[i]) == type)  // match
            ++ cnt;
      return (cnt);
   }
   int num_dest_operands_of_type(OperandType type, int pos = 0) const
   {
      int i, cnt = 0;
      for (i=pos ; i<num_dest_operands ; ++i)
         if (extract_op_type(dest_opd[i]) == type)  // match
            ++ cnt;
      return (cnt);
   }
   
   // next two routines extract the actual index field from the first
   // operand of a given type and return it.
   // Return -1 if no operand of that type is found
   int first_src_operand_of_type(OperandType type, int pos = 0) const
   {
      if (pos<0) return (-1);
      for (int i=pos ; i<num_src_operands ; ++i){
        // std::cerr << "extract_op_type is " << extract_op_type(src_opd[i]) << "\n";
         //std::cerr << "type is " << type << "\n";
         if (extract_op_type(src_opd[i]) == type)  {
            // match
            return (extract_op_index(src_opd[i]));
         }
      }
         
      return (-1);
   }
   int first_dest_operand_of_type(OperandType type, int pos = 0) const
   {
      if (pos<0) return (-1);
      for (int i=pos ; i<num_dest_operands ; ++i)
         if (extract_op_type(dest_opd[i]) == type)  // match
            return (extract_op_index(dest_opd[i]));
      return (-1);
   }

   /* Return the index of the memory operand used by this micro-op.
    * If the uop is not a memory operation, return -1
    */
   inline int get_memory_operand_index() const
   {
      int opidx = -1;
      
      // a MIAMI micro-op can have only one memory operand.
      // Thus, we seach for the first operand of type MEMORY
      if (InstrBinIsLoadType(type))    // it is a load operation
         opidx = first_src_operand_of_type(OperandType_MEMORY);
      else if (InstrBinIsStoreType(type))    // it is a store operation
         opidx = first_dest_operand_of_type(OperandType_MEMORY);
      
      return (opidx);
   }
   
   void *data; // auxiliary field that can be used to store a pointer
               // to additional data that is client specific

   InstrBin type;  // op type
   width_t  width; // operand bit width, e.g. 8, 16, 32, 64 ...
   uint8_t  vec_len; // vector length, 1 if scalar
   ExecUnit exec_unit;
   ExecUnitType exec_unit_type;
   bool primary;   // specify if this is the primary uop for its instruction
   uint8_t  num_src_operands;  // number of source operands
   uint8_t  num_dest_operands; // number of destination operands
   uint8_t  num_imm_values;
   bool flag;   

   // source operands. Use make_operand to pack operand info
   uint8_t  src_opd[max_num_src_operands]; 
   
   // destination operands. Use make_operand to pack operand info
   uint8_t  dest_opd[max_num_dest_operands];
   
   // keep the decoded immediate values
   imm_value_t imm_values[max_num_imm_values];
   RInfoList src_reg_list;
   RInfoList dest_reg_list;

};

typedef std::list<instruction_info>  InstrList;


class DecodedInstruction
{
public:
   DecodedInstruction()
   {
      mach_data = 0;
      len = 0;
      no_dyn_translation = false;
   }
   ~DecodedInstruction()
   {
      if (mach_data) free(mach_data);
   }
   
   InstrList micro_ops;
   int len;      // store the length of the instruction in bytes
   addrtype pc;  // store the instruction's address
   bool is_call; // set this field to true if this is a function call
   
   // auxiliary field to store machine dependent info
   // only the machine dependent code knows the type of data
   // pointed to by this pointer
   // destrutor frees memory if pointer is not NULL
   void *mach_data;
   bool no_dyn_translation;
};

const char* OperandTypeToString(OperandType o);
void DumpInstrList(const DecodedInstruction *dInst);

}  /* namespace MIAMI */

#endif
