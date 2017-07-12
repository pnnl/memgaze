/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: x86_xed_decoding.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements x86 specific instruction decoding based on XED.
 */

/*
 * Architecture specific instruction decoding
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
 
extern "C" {
#include "xed-interface.h"
}
#include "miami_types.h"
#include "instruction_decoding.h"
#include "register_class.h"


using namespace std;

#define USE_XED_DECODE_CACHE 0
#define DEBUG_INST_DECODE 0
#define DEBUG_REGISTER_EXTRACTION 0

namespace MIAMI
{
static xed_state_t xed_dstate;
#if USE_XED_DECODE_CACHE
static xed_decode_cache_t cache;  // use a cache for decoded instructions
#endif
#if USE_CUSTOM_REGISTER_TRANSLATION
static RegName xed_register_names[XED_REG_LAST];
#endif

int number_of_register_stacks()
{
   // on x86 we have only the x87 FPU register stack
   return (1);
}

int max_stack_top_value(int stack)
{
   if (stack==0)
      return (8);
   else
      return (0);
}

/* print_operands: copied from one of the XED examples; used only for debugging.
 * Not invoked during production runs.
 */
static void 
print_operands(xed_decoded_inst_t* xedd) 
{
    unsigned int i, noperands;
    cerr << "Operands" << endl;
    const xed_inst_t* xi = xed_decoded_inst_inst(xedd);
//    const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xedd);
    noperands = xed_inst_noperands(xi);
    cerr << "#   TYPE               DETAILS        VIS  RW       OC2 BITS BYTES NELEM ELEMSZ   ELEMTYPE"
         << endl;
    cerr << "#   ====               =======        ===  ==       === ==== ===== ===== ======   ========"
         << endl;




    for( i=0; i < noperands ; i++) { 
        const xed_operand_t* op = xed_inst_operand(xi,i);
        xed_operand_enum_t op_name = xed_operand_name(op);
        cerr << i << " " << setw(6) << xed_operand_enum_t2str(op_name) << " ";
        ostringstream os;


        switch(op_name) {
          case XED_OPERAND_AGEN:
          case XED_OPERAND_MEM0:
          case XED_OPERAND_MEM1:
            // we print memops in a different function
            os << "(see below)";
            break;
          case XED_OPERAND_PTR:  // pointer (always in conjunction with a IMM0)
          case XED_OPERAND_RELBR: { // branch displacements
              xed_uint_t disp_bits = xed_decoded_inst_get_branch_displacement_width(xedd);
              if (disp_bits) {
                  os  << "BRANCH_DISPLACEMENT_BYTES= " << disp_bits << " ";
                  xed_int32_t disp = xed_decoded_inst_get_branch_displacement(xedd);
                  os << hex << setfill('0') << setw(8) << disp << setfill(' ') << dec;
              }
            }
            break;

          case XED_OPERAND_IMM0: { // immediates
              xed_uint_t bits = xed_decoded_inst_get_immediate_width_bits(xedd);
              os << hex << "0x" << setfill('0');
              if (xed_decoded_inst_get_immediate_is_signed(xedd)) {
                  xed_uint_t swidth = bits?(bits/4):8;
                  xed_int32_t x =xed_decoded_inst_get_signed_immediate(xedd);
                  os << setw(swidth) << x;
              }
              else {
                  xed_uint64_t x = xed_decoded_inst_get_unsigned_immediate(xedd); 
                  xed_uint_t swidth = bits?(bits/4):16;
                  os << setw(swidth) << x;
              }
              os << setfill(' ') << dec << '(' << bits << "b)";
              break;
          }
          case XED_OPERAND_IMM1: { // 2nd immediate is always 1 byte.
              xed_uint8_t x = xed_decoded_inst_get_second_immediate(xedd);
              os << hex << "0x" << setfill('0') << setw(2) << (int)x << setfill(' ') << dec;
              break;
          }

          case XED_OPERAND_REG0:
          case XED_OPERAND_REG1:
          case XED_OPERAND_REG2:
          case XED_OPERAND_REG3:
          case XED_OPERAND_REG4:
          case XED_OPERAND_REG5:
          case XED_OPERAND_REG6:
          case XED_OPERAND_REG7:
          case XED_OPERAND_REG8:
// PIN kits up to version 2.13-61206 (inclusive) included operands REG9 to REG 15
// These have been removed from version 2.13-62141
// No good way to test if these enum constants are defined or not
#ifdef XED_OPERAND_REG9  // this is an enum constant, not visible to the preprocessor
          case XED_OPERAND_REG9:
          case XED_OPERAND_REG10:
          case XED_OPERAND_REG11:
          case XED_OPERAND_REG12:
          case XED_OPERAND_REG13:
          case XED_OPERAND_REG14:
          case XED_OPERAND_REG15:
#endif
          case XED_OPERAND_BASE0:
          case XED_OPERAND_BASE1:
            {
              xed_reg_enum_t r = xed_decoded_inst_get_reg(xedd, op_name);
              os << xed_operand_enum_t2str(op_name) << "=" << xed_reg_enum_t2str(r);
              break;
          }
          default: //FIXME: check the width and print the correct right value.
            // this is for other miscellaneous immediate fields captured during decode.
            os << " = " << "**" 
               << "need to add support for printing operand: " << xed_operand_enum_t2str(op_name)
//               << xed_operand_values_get_operand_decider(ov, op_name) 
               << "**";
            break;
        }
        cerr << setw(21) << os.str();
        cerr << " " << setw(10) << xed_operand_visibility_enum_t2str(xed_operand_operand_visibility(op))
             << " " << setw(3) << xed_operand_action_enum_t2str(xed_operand_rw(op))
             << " " << setw(9) << xed_operand_width_enum_t2str(xed_operand_width(op));

        xed_uint_t bits =  xed_decoded_inst_operand_length_bits(xedd,i);
        cerr << "  " << setw(3) << bits;

        /* rounding, bits might not be a multiple of 8 */
        cerr << "  " << setw(4) << ((bits +7) >> 3); 

        cerr << "    " << setw(2) << xed_decoded_inst_operand_elements(xedd,i);
        cerr << "    " << setw(3) << xed_decoded_inst_operand_element_size_bits(xedd,i);
        cerr << " " << setw(10) <<  xed_operand_element_type_enum_t2str(xed_decoded_inst_operand_element_type(xedd,i));
        cerr << endl;
    }
}

/* print_memops: copied from one of the XED examples; used only for debugging.
 * Not invoked during production runs.
 */
static void 
print_memops(xed_decoded_inst_t* xedd) 
{
    int i;
    int memops = xed_decoded_inst_number_of_memory_operands(xedd);
    cerr << "Memory Operands = " << memops << endl;
    
    for( i=0;i<memops ; i++)   {
        xed_bool_t r_or_w = false;
        cerr << "  " << i << " ";
        if ( xed_decoded_inst_mem_read(xedd,i)) {
            cerr << "   read ";
            r_or_w = true;
        }
        if (xed_decoded_inst_mem_written(xedd,i)) {
            cerr << "written ";
            r_or_w = true;
        }
        if (!r_or_w) {
            cerr << "   agen "; // LEA instructions
        }
        xed_reg_enum_t seg = xed_decoded_inst_get_seg_reg(xedd,i);
        if (seg != XED_REG_INVALID) {
            cerr << "SEG= " << xed_reg_enum_t2str(seg) << " ";
        }
        xed_reg_enum_t base = xed_decoded_inst_get_base_reg(xedd,i);
        if (base != XED_REG_INVALID) {
            cerr << "BASE= " << setw(3) << xed_reg_enum_t2str(base) << "/" 
                 << setw(3)
                 <<  xed_reg_class_enum_t2str(xed_reg_class(base)) << " "; 
        }
        xed_reg_enum_t indx = xed_decoded_inst_get_index_reg(xedd,i);
        if (i == 0 && indx != XED_REG_INVALID) {
            cerr << "INDEX= " << setw(3) << xed_reg_enum_t2str(indx)
                 << "/" <<  setw(3) 
                 << xed_reg_class_enum_t2str(xed_reg_class(indx)) << " ";
            if (xed_decoded_inst_get_scale(xedd,i) != 0) {
                // only have a scale if the index exists.
                cerr << "SCALE= " <<  xed_decoded_inst_get_scale(xedd,i) << " ";
            }
        }
        xed_uint_t disp_bits = xed_decoded_inst_get_memory_displacement_width(xedd,i);
        if (disp_bits) {
            cerr  << "DISPLACEMENT_BYTES= " << disp_bits << " ";
            xed_int64_t disp = xed_decoded_inst_get_memory_displacement(xedd,i);
            cerr << "0x" << hex << setfill('0') << setw(16) << disp << setfill(' ') << dec << " base10=" << disp;
        }
        
        cerr << " ASZ" << i << "=" << xed_decoded_inst_get_memop_address_width(xedd,i);
        cerr << endl;
    }
    cerr << "  MemopBytes = " << xed_decoded_inst_get_memory_operand_length(xedd,0) << endl;
}

static int
GetNumberOfElements(xed_decoded_inst_t* inst, int operand)
{
   int num_elems = xed_decoded_inst_operand_elements(inst, operand);
   xed_iclass_enum_t xclass = xed_decoded_inst_get_iclass(inst);
   switch (xclass)
   {
      // single-element versions of AVX encoded instructions with 3 operands
      // perform the respective scalar operation on the lower bits of the 
      // source operands, but also copy the bits up to 128 from the first 
      // source operand to the destination operand. It also zeroes out the
      // high 128 bits of an YMM register.
      // XED reports destination operand length as 128 bits, but the operation 
      // itself is performed on a single element. We want to focus on the 
      // primary operation code, ignore the copy.
      case XED_ICLASS_VADDSD:             // AVX
      case XED_ICLASS_VADDSS:             // AVX
      case XED_ICLASS_VCMPSD:             // AVX
      case XED_ICLASS_VCMPSS:             // AVX
      case XED_ICLASS_VDIVSD:             // AVX
      case XED_ICLASS_VDIVSS:             // AVX
      case XED_ICLASS_VMAXSD:             // AVX
      case XED_ICLASS_VMAXSS:             // AVX
      case XED_ICLASS_VMINSD:             // AVX
      case XED_ICLASS_VMINSS:             // AVX
      case XED_ICLASS_VMULSD:             // AVX
      case XED_ICLASS_VMULSS:             // AVX
      case XED_ICLASS_VRCPSS:             // AVX
      case XED_ICLASS_VROUNDSD:           // AVX
      case XED_ICLASS_VROUNDSS:           // AVX
      case XED_ICLASS_VRSQRTSS:           // AVX
      case XED_ICLASS_VSQRTSD:            // AVX
      case XED_ICLASS_VSQRTSS:            // AVX
      case XED_ICLASS_VSUBSD:             // AVX
      case XED_ICLASS_VSUBSS:             // AVX
      case XED_ICLASS_VCVTSD2SS:          // CONVERT
      case XED_ICLASS_VCVTSS2SD:          // CONVERT
      case XED_ICLASS_VCVTSI2SD:          // CONVERT
      case XED_ICLASS_VCVTSI2SS:          // CONVERT

      case XED_ICLASS_VFMADD132SD:        // AVX2
      case XED_ICLASS_VFMADD132SS:        // AVX2
      case XED_ICLASS_VFMADD213SD:        // AVX2
      case XED_ICLASS_VFMADD213SS:        // AVX2
      case XED_ICLASS_VFMADD231SD:        // AVX2
      case XED_ICLASS_VFMADD231SS:        // AVX2
      case XED_ICLASS_VFMADDSD:           // AVX2
      case XED_ICLASS_VFMADDSS:           // AVX2
      case XED_ICLASS_VFNMADD132SD:       // AVX2
      case XED_ICLASS_VFNMADD132SS:       // AVX2
      case XED_ICLASS_VFNMADD213SD:       // AVX2
      case XED_ICLASS_VFNMADD213SS:       // AVX2
      case XED_ICLASS_VFNMADD231SD:       // AVX2
      case XED_ICLASS_VFNMADD231SS:       // AVX2
      case XED_ICLASS_VFNMADDSD:          // AVX2
      case XED_ICLASS_VFNMADDSS:          // AVX2

      case XED_ICLASS_VFMSUB132SD:        // AVX2
      case XED_ICLASS_VFMSUB132SS:        // AVX2
      case XED_ICLASS_VFMSUB213SD:        // AVX2
      case XED_ICLASS_VFMSUB213SS:        // AVX2
      case XED_ICLASS_VFMSUB231SD:        // AVX2
      case XED_ICLASS_VFMSUB231SS:        // AVX2
      case XED_ICLASS_VFMSUBSD:           // AVX2
      case XED_ICLASS_VFMSUBSS:           // AVX2
      case XED_ICLASS_VFNMSUB132SD:       // AVX2
      case XED_ICLASS_VFNMSUB132SS:       // AVX2
      case XED_ICLASS_VFNMSUB213SD:       // AVX2
      case XED_ICLASS_VFNMSUB213SS:       // AVX2
      case XED_ICLASS_VFNMSUB231SD:       // AVX2
      case XED_ICLASS_VFNMSUB231SS:       // AVX2
      case XED_ICLASS_VFNMSUBSD:          // AVX2
      case XED_ICLASS_VFNMSUBSS:          // AVX2
         num_elems = 1;
         break;
      default:
         break;
   }
   return (num_elems);
}

/* helper decoding routines from Collin
*/
static ExecUnit
GetExecUnit(xed_decoded_inst_t* inst)
{
    switch(xed_decoded_inst_get_extension(inst)) 
    {
       case XED_EXTENSION_3DNOW:
       case XED_EXTENSION_AES:
       case XED_EXTENSION_AVX:
       case XED_EXTENSION_AVX2:
       case XED_EXTENSION_AVX2GATHER:
       case XED_EXTENSION_MMX:
       case XED_EXTENSION_PCLMULQDQ:
       case XED_EXTENSION_SSE:
       case XED_EXTENSION_SSE2:
       case XED_EXTENSION_SSE3:
       case XED_EXTENSION_SSE4:
       case XED_EXTENSION_SSE4A:
       case XED_EXTENSION_SSSE3:
           return ExecUnit_VECTOR;
       default:
           return ExecUnit_SCALAR;
    }
}

static ExecUnitType
GetExecUnitType(xed_decoded_inst_t* inst, int operand)
{
    switch(xed_decoded_inst_operand_element_type(inst, operand)) 
    {
       case XED_OPERAND_ELEMENT_TYPE_SINGLE: ///< 32b FP single precision
       case XED_OPERAND_ELEMENT_TYPE_DOUBLE: ///< 64b FP double precision
       case XED_OPERAND_ELEMENT_TYPE_LONGDOUBLE: ///< 80b FP x87
           return ExecUnitType_FP;
       default:
           return ExecUnitType_INT;
    }
}

static InstrBin
GetInstrBin(xed_decoded_inst_t* inst)
{
    switch (xed_decoded_inst_get_iclass(inst)) 
#include "InstructionDecoder-xed-iclass.h"
}


static bool
IsDest(const xed_operand_t* op)
{
    switch (xed_operand_rw(op)) 
    {
       case XED_OPERAND_ACTION_RW: ///< Read and written (must write)
       case XED_OPERAND_ACTION_W: ///< Write-only (must write)
       case XED_OPERAND_ACTION_RCW: ///< Read and conditionlly written (may write)
       case XED_OPERAND_ACTION_CW: ///< Conditionlly written (may write)
       case XED_OPERAND_ACTION_CRW: ///< Conditionlly read, always written (must write)
           return true;
       default:
           return false;
    }
}


static bool
IsSrc(const xed_operand_t* op)
{
    switch (xed_operand_rw(op)) 
    {
       case XED_OPERAND_ACTION_RW: ///< Read and written (must write)
       case XED_OPERAND_ACTION_R: ///< Read-only
       case XED_OPERAND_ACTION_RCW: ///< Read and conditionlly written (may write)
       case XED_OPERAND_ACTION_CRW: ///< Conditionlly read, always written (must write)
       case XED_OPERAND_ACTION_CR: ///< Conditional read
           return true;
       default:
           return false;
    }
}

/* Specialized routine for decoding a LEAVE instruction into micro-ops. 
 */
static int
XedDecodeLeaveInstruction(xed_decoded_inst_t* inst, InstrList *iList, void *pc)
{
   // A LEAVE instruction performs the following tasks:
   // - copy (E/R)BP to (E/R)SP
   // - load (E)BP from [EBP]   -> the MEM0 operand
   // - increment ESP by operand size (4 or 8 bytes)
   // Thus, we will have 3 micro-ops: COPY (primary), LOAD, ADD
   iList->push_back(instruction_info());
   instruction_info &primary_uop = iList->back();
   primary_uop.type = IB_copy;
   primary_uop.exec_unit = GetExecUnit(inst);
   primary_uop.primary = true;

   iList->push_back(instruction_info());
   instruction_info &load_uop = iList->back();
   load_uop.type = IB_load;
   
   iList->push_back(instruction_info());
   instruction_info &add_uop = iList->back();
   add_uop.type = IB_add;
   add_uop.exec_unit = GetExecUnit(inst);
   
   const xed_inst_t* xi = xed_decoded_inst_inst(inst);
   int noperands = xed_inst_noperands(xi);
   // use first destination operand for overall instruction type
   // info; if no dest operand, defaults to first source
   int i, desti = 0, memops = 0;
   int stack_width = 0;
   int regop_idx = 0;
   
    for (i=0; i < noperands ; i++) 
    { 
        const xed_operand_t* op = xed_inst_operand(xi,i);
        xed_operand_enum_t op_name = xed_operand_name(op);
        bool src = IsSrc(op), dest = IsDest(op);
        // use *first* dest operand...
        if (!desti && dest) desti = i;
        switch(op_name) 
        {
           case XED_OPERAND_MEM0:
               assert (src && !dest);
               load_uop.src_opd[load_uop.num_src_operands++] = 
                             make_operand(OperandType_MEMORY, memops);
               load_uop.vec_len = xed_decoded_inst_operand_elements(inst,i);
               stack_width = xed_decoded_inst_operand_length_bits(inst,i);
               load_uop.width = stack_width/load_uop.vec_len;
               stack_width /= 8;
               load_uop.exec_unit = GetExecUnit(inst);
               load_uop.exec_unit_type = GetExecUnitType(inst,i);
               memops += 1;
               break;

           case XED_OPERAND_REG0:
           case XED_OPERAND_REG1:
           case XED_OPERAND_REG2:
               regop_idx += 1;
               if (regop_idx == 1)
               {
                  // reg1 should be the (E)BP register of the right size; assert it.
                  xed_reg_enum_t r = xed_decoded_inst_get_reg(inst, op_name);
                  assert(xed_get_largest_enclosing_register((xed_reg_enum_t)r)==XED_REG_RBP);
                  // this is source for primary uop and dest for load_uop;
                  assert (dest && src);
                  load_uop.dest_opd[load_uop.num_dest_operands++] = 
                                       make_operand(OperandType_REGISTER, i);
                  primary_uop.src_opd[primary_uop.num_src_operands++] = 
                                       make_operand(OperandType_REGISTER, i);
               } else
               {
                  // LEAVE has only two register operands
                  assert (regop_idx == 2);
                  // reg2 should be the (E)SP register of the right size; assert it.
                  xed_reg_enum_t r = xed_decoded_inst_get_reg(inst, op_name);
                  assert(xed_get_largest_enclosing_register((xed_reg_enum_t)r)==XED_REG_RSP);
                  // this is dest for primary uop and source/dest for add_uop;
                  assert (dest && src);
                  primary_uop.dest_opd[primary_uop.num_dest_operands++] = 
                                       make_operand(OperandType_REGISTER, i);
                  add_uop.dest_opd[add_uop.num_dest_operands++] = 
                                       make_operand(OperandType_REGISTER, i);
                  add_uop.src_opd[add_uop.num_src_operands++] = 
                                       make_operand(OperandType_REGISTER, i);
                  add_uop.vec_len = xed_decoded_inst_operand_elements(inst,i);
                  add_uop.width = 
                           xed_decoded_inst_operand_length_bits(inst,i)/add_uop.vec_len;
                  add_uop.exec_unit = GetExecUnit(inst);
                  add_uop.exec_unit_type = GetExecUnitType(inst,i);
                  
                  primary_uop.vec_len = add_uop.vec_len;
                  primary_uop.width = add_uop.width;
                  primary_uop.exec_unit_type = add_uop.exec_unit_type;
               }
               break;

           case XED_OPERAND_BASE0:
               // nothing to do on BASE0 operand
               break;

           default: // Did we miss something?
               // Unhandled operand type. Print a message. Assert??
               cerr << "Yo, Unknown operand type." << endl;
               assert (!"LEAVE should not have any other operands.");
               break;
        }
    }
    
    assert (stack_width>0);
    add_uop.imm_values[add_uop.num_imm_values].is_signed = true;
    add_uop.imm_values[add_uop.num_imm_values].value.s = stack_width;
    add_uop.src_opd[add_uop.num_src_operands++] = 
                 make_operand(OperandType_IMMED, add_uop.num_imm_values++);
    return (3);
}

/* Specialized routine for decoding an ENTER instruction into micro-ops. 
 */
static int
XedDecodeEnterInstruction(xed_decoded_inst_t* inst, InstrList *iList, void *pc)
{
   // An ENTER instruction performs the following tasks:
   // - decrement ESP by operand size (4 or 8 bytes)
   // - store (E)BP to [ESP]
   // - copy (E/R)SP to (E/R)BP
   // - decrement ESP by imm16 parameter
   // Thus, we will have 4 micro-ops: ADD, STORE, COPY (primary), ADD
   assert (!"Write this routine after I find an ENTER instruction.");
   
   iList->push_back(instruction_info());
   instruction_info &primary_uop = iList->back();
   primary_uop.type = IB_copy;
   primary_uop.exec_unit = GetExecUnit(inst);
   primary_uop.primary = true;

   iList->push_back(instruction_info());
   instruction_info &load_uop = iList->back();
   load_uop.type = IB_load;
   
   iList->push_back(instruction_info());
   instruction_info &add_uop = iList->back();
   add_uop.type = IB_add;
   add_uop.exec_unit = GetExecUnit(inst);
   
   const xed_inst_t* xi = xed_decoded_inst_inst(inst);
   int noperands = xed_inst_noperands(xi);
   // use first destination operand for overall instruction type
   // info; if no dest operand, defaults to first source
   int i, desti = 0, memops = 0;
   int stack_width = 0;
   
    for (i=0; i < noperands ; i++) 
    { 
        const xed_operand_t* op = xed_inst_operand(xi,i);
        xed_operand_enum_t op_name = xed_operand_name(op);
        bool src = IsSrc(op), dest = IsDest(op);
        // use *first* dest operand...
        if (!desti && dest) desti = i;
        switch(op_name) 
        {
           case XED_OPERAND_MEM0:
               assert (src && !dest);
               load_uop.src_opd[load_uop.num_src_operands++] = 
                             make_operand(OperandType_MEMORY, memops);
               load_uop.vec_len = xed_decoded_inst_operand_elements(inst,i);
               stack_width = xed_decoded_inst_operand_length_bits(inst,i);
               load_uop.width = stack_width/load_uop.vec_len;
               stack_width /= 8;
               load_uop.exec_unit = GetExecUnit(inst);
               load_uop.exec_unit_type = GetExecUnitType(inst,i);
               memops += 1;
               break;
#if 0
           case XED_OPERAND_IMM0: // this must be source 
               if ((add_uop.imm_values[add_uop.num_imm_values].is_signed = 
                         xed_decoded_inst_get_immediate_is_signed(inst)) == true)
                  add_uop.imm_values[add_uop.num_imm_values].value.s = 
                          xed_decoded_inst_get_signed_immediate(inst) + stack_width;
               else
                  add_uop.imm_values[add_uop.num_imm_values].value.u = 
                          xed_decoded_inst_get_unsigned_immediate(inst) + stack_width;
               
               add_uop.src_opd[add_uop.num_src_operands++] = 
                       make_operand(OperandType_IMMED, add_uop.num_imm_values++);
               break;
#endif

           case XED_OPERAND_REG1:
           {
               // reg1 should be the (E)BP register of the right size; assert it.
               xed_reg_enum_t r = xed_decoded_inst_get_reg(inst, op_name);
               assert(xed_get_largest_enclosing_register((xed_reg_enum_t)r)==XED_REG_RBP);
               // this is source for primary uop and dest for load_uop;
               assert (dest && src);
               load_uop.dest_opd[load_uop.num_dest_operands++] = 
                                    make_operand(OperandType_REGISTER, i);
               primary_uop.src_opd[primary_uop.num_src_operands++] = 
                                    make_operand(OperandType_REGISTER, i);
           }
               break;
               
           case XED_OPERAND_REG2:
           {
               // reg2 should be the (E)SP register of the right size; assert it.
               xed_reg_enum_t r = xed_decoded_inst_get_reg(inst, op_name);
               assert(xed_get_largest_enclosing_register((xed_reg_enum_t)r)==XED_REG_RSP);
               // this is dest for primary uop and source/dest for add_uop;
               assert (dest && src);
               primary_uop.dest_opd[primary_uop.num_dest_operands++] = 
                                    make_operand(OperandType_REGISTER, i);
               add_uop.dest_opd[add_uop.num_dest_operands++] = 
                                    make_operand(OperandType_REGISTER, i);
               add_uop.src_opd[add_uop.num_src_operands++] = 
                                    make_operand(OperandType_REGISTER, i);
               add_uop.vec_len = xed_decoded_inst_operand_elements(inst,i);
               add_uop.width = 
                        xed_decoded_inst_operand_length_bits(inst,i)/add_uop.vec_len;
               add_uop.exec_unit = GetExecUnit(inst);
               add_uop.exec_unit_type = GetExecUnitType(inst,i);
               
               primary_uop.vec_len = add_uop.vec_len;
               primary_uop.width = add_uop.width;
               primary_uop.exec_unit_type = add_uop.exec_unit_type;
           }
               break;

           case XED_OPERAND_BASE0:
               // nothing to do on BASE0 operand
               break;

           default: // Did we miss something?
               // Unhandled operand type. Print a message. Assert??
               cerr << "Yo, Unknown operand type." << endl;
               assert (!"ENTER should not have any other operands.");
               break;
        }
    }
    
    if (add_uop.num_imm_values == 0)  // add stack increment value
    {
       assert (stack_width>0);
       add_uop.imm_values[add_uop.num_imm_values].is_signed = true;
       add_uop.imm_values[add_uop.num_imm_values].value.s = stack_width;
       add_uop.src_opd[add_uop.num_src_operands++] = 
                   make_operand(OperandType_IMMED, add_uop.num_imm_values++);
    }
    return (4);
}

/* Specialized routine for decoding an XCHG instruction into micro-ops. 
 */
static int
XedDecodeXchgInstruction(xed_decoded_inst_t* inst, InstrList *iList, void *pc)
{
   // An XCHG instruction swaps the values held by the two operands.
   // It can be decoded into a sequence of three Copy operations using
   // a temporary register.
   // If one of the operands is a memory operand, two of the micro-ops
   // will become Load and Store respectively.

//   debug_decode_instruction_at_pc(pc, xed_decoded_inst_get_length(inst));
   
   iList->push_back(instruction_info());
   instruction_info &first_uop = iList->back();
   first_uop.type = IB_copy;
   first_uop.exec_unit = GetExecUnit(inst);
   first_uop.primary = false;
   first_uop.num_src_operands = 1;
   first_uop.num_dest_operands = 1;

   iList->push_back(instruction_info());
   instruction_info &second_uop = iList->back();
   second_uop.type = IB_copy;
   second_uop.exec_unit = first_uop.exec_unit;
   second_uop.primary = false;
   second_uop.num_src_operands = 1;
   second_uop.num_dest_operands = 1;
   
   iList->push_back(instruction_info());
   instruction_info &third_uop = iList->back();
   third_uop.type = IB_copy;
   third_uop.exec_unit = first_uop.exec_unit;
   third_uop.primary = false;
   third_uop.num_src_operands = 1;
   third_uop.num_dest_operands = 1;

   // add internal source operand to third uop, and dest operand to first uop
   third_uop.src_opd[0] = make_operand(OperandType_INTERNAL, 0);
   first_uop.dest_opd[0] = make_operand(OperandType_INTERNAL, 0);

   const xed_inst_t* xi = xed_decoded_inst_inst(inst);
   int noperands = xed_inst_noperands(xi);
   assert (noperands==2 || !"Found XCHG instruction with more than 2 operands");

   int i, memops = 0;
   for (i=0; i < noperands ; i++) 
   {
      const xed_operand_t* op = xed_inst_operand(xi,i);
      xed_operand_enum_t op_name = xed_operand_name(op);
      
      OperandType opt = OperandType_REGISTER;
      int op_idx = i;
      if (op_name==XED_OPERAND_MEM0 || op_name==XED_OPERAND_MEM1)
      {
         opt = OperandType_MEMORY;
         op_idx = memops++;
         if (i==0)
         {
            first_uop.type = IB_load;
            second_uop.type = IB_store;
         } else
         {
            assert (i==1);
            second_uop.type = IB_load;
            third_uop.type = IB_store;
         }
      } else
      {
         assert (op_name==XED_OPERAND_REG0 || op_name==XED_OPERAND_REG1);
         first_uop.vec_len = second_uop.vec_len = third_uop.vec_len = 
                    xed_decoded_inst_operand_elements(inst,i);
         first_uop.width = second_uop.width = third_uop.width = 
                    xed_decoded_inst_operand_length_bits(inst,i)/first_uop.vec_len;
         first_uop.exec_unit_type = second_uop.exec_unit_type = 
                    third_uop.exec_unit_type = GetExecUnitType(inst,i);
         if (i == 0)
            first_uop.primary = true;
         else
            third_uop.primary = true;
      }
      if (i==0)  // first operand
      {
         first_uop.src_opd[0] = second_uop.dest_opd[0] = make_operand(opt, op_idx);
      } else
      {
         assert (i==1);
         second_uop.src_opd[0] = third_uop.dest_opd[0] = make_operand(opt, op_idx);
      }
   }

    return (3);
}

/* Specialized routine for decoding STRINGOP instructions into micro-ops. 
 */
static int
XedDecodeStringInstruction(xed_decoded_inst_t* inst, InstrList *iList, void *pc)
{
   // A STRINGOP instruction streams over memory locations
   // I need to create a Load for a read memory operand, a Store for a
   // write memory operand, and Add instructions to increment the
   // BASE registers (by 4 or 8 bytes - given by operand size).
   // Finally, I will create a branch instruction at the end.
   // I think that I can have up to 6 micro-ops.
   
   iList->push_back(instruction_info());
   instruction_info &primary_uop = iList->back();
   primary_uop.type = GetInstrBin(inst);
   primary_uop.exec_unit = GetExecUnit(inst);
   primary_uop.primary = true;

   // if instruction has REP prefix, add also a branch micro-op
   instruction_info *br_uop = 0;
//   bool has_branch = false;
   if (xed_operand_values_has_real_rep(xed_decoded_inst_operands_const(inst)))
   {
       br_uop = new instruction_info();
       br_uop->type = IB_branch;
       br_uop->vec_len = 0;
       br_uop->width = 0;
       br_uop->exec_unit = GetExecUnit(inst);
       br_uop->exec_unit_type = ExecUnitType_INT;
       br_uop->num_src_operands = 0;
       br_uop->num_dest_operands = 0;
//       has_branch = true;
   }

   const xed_inst_t* xi = xed_decoded_inst_inst(inst);
   int num_internal_operands = 0;
   int noperands = xed_inst_noperands(xi);
   // use first destination operand for overall instruction type
   // info; if no dest operand, defaults to first source
   int i, desti = -1, memops = 0, res = 1;
   int mem_width[2] = {0, 0};
   bool is_load_uop = InstrBinIsLoadType(primary_uop.type);
   bool is_store_uop = InstrBinIsStoreType(primary_uop.type);
   
    for (i=0; i < noperands ; i++) 
    { 
        const xed_operand_t* op = xed_inst_operand(xi,i);
        xed_operand_enum_t op_name = xed_operand_name(op);
        bool src = IsSrc(op), dest = IsDest(op);
        // use *first* dest operand...
        switch(op_name) 
        {
           case XED_OPERAND_MEM0:
           case XED_OPERAND_MEM1:
           {
               if (desti<0) desti = i;
               int midx = (op_name==XED_OPERAND_MEM0?0:1);
               mem_width[midx] = xed_decoded_inst_operand_length_bits(inst,i)/8;
               if (dest) 
               {
                   if (is_store_uop)
                   {
                      primary_uop.dest_opd[primary_uop.num_dest_operands++] = 
                          make_operand(OperandType_MEMORY, memops);
                   } else
                   {
                      // create store uop
                      iList->push_back(instruction_info());
                      instruction_info &store_uop = iList->back();
                      res += 1;
                      store_uop.type = IB_store;
                      store_uop.vec_len = xed_decoded_inst_operand_elements(inst,i);
                      store_uop.width = 
                          xed_decoded_inst_operand_length_bits(inst,i)/store_uop.vec_len;
                      store_uop.exec_unit = GetExecUnit(inst);
                      store_uop.exec_unit_type = GetExecUnitType(inst,i);
                      store_uop.num_src_operands = 1;
                      store_uop.src_opd[0] = make_operand(OperandType_INTERNAL, 
                                                    num_internal_operands);
                      store_uop.num_dest_operands = 1;
                      store_uop.dest_opd[0] = make_operand(OperandType_MEMORY, memops);
                      // add internal dest operand to primary uop
                      primary_uop.dest_opd[primary_uop.num_dest_operands++] = 
                          make_operand(OperandType_INTERNAL, num_internal_operands++);
                   }
               }
               if (src) 
               {
                   if (is_load_uop)
                   {
                      primary_uop.src_opd[primary_uop.num_src_operands++] = 
                          make_operand(OperandType_MEMORY, memops);
                   } else
                   {
                      // create load uop
                      iList->push_front(instruction_info());
                      instruction_info &load_uop = iList->front();
                      res += 1;

                      load_uop.type = IB_load;
                      load_uop.vec_len = xed_decoded_inst_operand_elements(inst,i);
                      load_uop.width = 
                          xed_decoded_inst_operand_length_bits(inst,i)/load_uop.vec_len;
                      load_uop.exec_unit = GetExecUnit(inst);
                      load_uop.exec_unit_type = GetExecUnitType(inst,i);
                      load_uop.num_src_operands = 1;
                      load_uop.src_opd[0] = make_operand(OperandType_MEMORY, memops);
                      load_uop.num_dest_operands = 1;
                      load_uop.dest_opd[0] = make_operand(OperandType_INTERNAL, 
                                                    num_internal_operands);
                      // add internal source operand to primary uop
                      primary_uop.src_opd[primary_uop.num_src_operands++] = 
                          make_operand(OperandType_INTERNAL, num_internal_operands++);
                   }
               }
               // some operands can be R/W. We want to use the same mem operand index 
               // for both uses. Thus, we increment memops count only once at the end.
               memops += 1;
           }
           break;

           case XED_OPERAND_REG0:
           case XED_OPERAND_REG1:
           case XED_OPERAND_REG2:
           {
               // I have to decide where I store the register.
               // Should it be the primary uop, or the loop back?
               // I need to create the loop back as well;
               // If this is register RAX, then it goes to primary_uop.
               xed_reg_enum_t r = xed_decoded_inst_get_reg(inst, op_name);
               xed_reg_enum_t ler = xed_get_largest_enclosing_register(r);
               if (ler==XED_REG_RAX)
               {
                  assert (src && !dest); 
                  // always use the AL/AX/EAX/RAX register to determine
                  // the width; if not present, use the first mem operand
                  desti = i;
                  primary_uop.src_opd[primary_uop.num_src_operands++] = 
                                    make_operand(OperandType_REGISTER, i);
               }
               else if (ler==XED_REG_RCX)
               {  // ECX is used as a loop count, add it to the branch
                  assert (src && dest && br_uop);
                  br_uop->src_opd[br_uop->num_src_operands++] = 
                                    make_operand(OperandType_REGISTER, i);
                  br_uop->dest_opd[br_uop->num_dest_operands++] = 
                                    make_operand(OperandType_REGISTER, i);
               } else
               {  // this must be the flags register
                  // whatever register it is, if it is dest, it is written by primary uop
                  // if source, it is used by the add_uops, but add it to the br_uop instead, meh
                  if (dest)
                     primary_uop.dest_opd[primary_uop.num_dest_operands++] = 
                                    make_operand(OperandType_REGISTER, i);
                  if (src && br_uop)
                     br_uop->src_opd[br_uop->num_src_operands++] = 
                                    make_operand(OperandType_REGISTER, i);
               }
           }
           break;
               
           case XED_OPERAND_BASE0:
           case XED_OPERAND_BASE1:
               // I noticed that BASE operands are listed only when we have some
               // stackpush or stackpop operation
               {
                   int sidx = (op_name==XED_OPERAND_BASE0?0:1);
                   // make an add instruction
                   assert (dest && src);
                   iList->push_back(instruction_info());
                   instruction_info &add_uop = iList->back();
                   res += 1;
                   add_uop.type = IB_add;
                   add_uop.vec_len = 1;
                   add_uop.width = 
                       xed_decoded_inst_operand_length_bits(inst,i);
                   add_uop.exec_unit = GetExecUnit(inst);
                   add_uop.exec_unit_type = GetExecUnitType(inst,i);
                   add_uop.num_src_operands = 2;
                   add_uop.src_opd[0] = make_operand(OperandType_REGISTER, i); 
                   add_uop.num_dest_operands = 1;
                   add_uop.dest_opd[0] = make_operand(OperandType_REGISTER, i);

                   // I have to create also an immediate operand ...
                   add_uop.num_imm_values = 1;
                   add_uop.imm_values[0].is_signed = true;
                   add_uop.imm_values[0].value.s = mem_width[sidx];
                   add_uop.src_opd[1] = make_operand(OperandType_IMMED, 0);
               }
               break;

           default: // Did we miss something?
               // Unhandled operand type. Print a message. Assert??
               cerr << "Yo, Unknown operand type." << endl;
               assert (!"STRINGOP should not have any other operands.");
               break;
        }
    }
    
    if (desti>=0) {
        primary_uop.vec_len = xed_decoded_inst_operand_elements(inst,desti);
        primary_uop.width = 
            xed_decoded_inst_operand_length_bits(inst,desti)/primary_uop.vec_len;
        primary_uop.exec_unit_type = GetExecUnitType(inst,desti);
    } 
    else {
        primary_uop.vec_len = 0;
        primary_uop.width = 0;
        primary_uop.exec_unit_type = ExecUnitType_INT;
    }
    
    if (br_uop)
    {
       iList->push_back(*br_uop);
       res += 1;
       delete (br_uop);
    }
    
    return (res);
}

static int
XedInstToUopList(xed_decoded_inst_t* inst, InstrList *iList, void *pc)
{
    iList->clear();
    bool special_handling = true;
    int res = 1;

    xed_category_enum_t cat = xed_decoded_inst_get_category(inst);
    xed_iclass_enum_t xclass = xed_decoded_inst_get_iclass(inst);
    if (cat==XED_CATEGORY_STRINGOP || cat==XED_CATEGORY_IOSTRINGOP)
       res = XedDecodeStringInstruction(inst, iList, pc);
    else  // not a string op, check class for special cases
    switch(xclass)
    {
#if 0   // CALL and RET are handled by the regular case for now.
        // If you notice any strange behavior due to CALL/RET, create
        // specialized decoding for them
       case XED_ICLASS_CALL_FAR:           // CALL
       case XED_ICLASS_CALL_NEAR:          // CALL
          res = XedDecodeCallInstruction(inst, iList, pc);
          break;

       case XED_ICLASS_RET_FAR:            // RET
       case XED_ICLASS_RET_NEAR:           // RET
          res = XedDecodeRetInstruction(inst, iList, pc);
          break;
#endif

    case XED_ICLASS_ENTER:              // MISC
          res = XedDecodeEnterInstruction(inst, iList, pc);
          break;

    case XED_ICLASS_LEAVE:              // MISC
          res = XedDecodeLeaveInstruction(inst, iList, pc);
          break;
          
    case XED_ICLASS_XCHG:               // DATAXFER
          res = XedDecodeXchgInstruction(inst, iList, pc);
          break;

       default:
          special_handling = false;
          break;
    }
    if (special_handling)
    {
       // before returning, check if we have any unknown operation ...
       if (iList)
       {
          InstrList::iterator iit = iList->begin();
          for ( ; iit!=iList->end() ; ++iit)
          {
             if (iit->type == IB_unknown)
             {
                // list the xed decoded instructions
                fprintf(stderr, "XedInstToUopList (special case): HELLO, MIAMI does not handle the following opcode. Please report this error to mgabi99@gmail.com\n");
                debug_decode_instruction_at_pc(pc, xed_decoded_inst_get_length(inst));
                break;
             }
          }
       }
       return (res);
    }

    // continue with regular handling
    iList->push_back(instruction_info());
    instruction_info &primary_uop = iList->back();
    instruction_info *stack_uop = 0, *mem_uop = 0;
    InstrList::iterator primary_it = iList->begin();
    InstrList::iterator mem_it = iList->end();
    
    primary_uop.type = GetInstrBin(inst);
    if (primary_uop.type == IB_unknown)  // an unknown opcode
    {
       // list the xed decoded instructions
       fprintf(stderr, "XedInstToUopList: HELLO, MIAMI does not handle the following opcode. Please report this error to mgabi99@gmail.com\n");
       debug_decode_instruction_at_pc(pc, xed_decoded_inst_get_length(inst));
    }
    primary_uop.exec_unit = GetExecUnit(inst);
    primary_uop.primary = true;
    
    bool is_load_uop = InstrBinIsLoadType(primary_uop.type);
    bool is_store_uop = InstrBinIsStoreType(primary_uop.type);
    bool is_copy = (primary_uop.type == IB_copy);

    const xed_inst_t* xi = xed_decoded_inst_inst(inst);
//    const xed_operand_values_t* ov = xed_decoded_inst_operands_const(inst);
    int noperands = xed_inst_noperands(xi);
    // use first destination operand for overall instruction type
    // info; if no dest operand, defaults to first source
    int i, desti = -1, srci = -1, memi = -1, memops = 0;
    int num_internal_operands = 0;
    bool is_stackpush = false, is_stackpop = false;
    int stack_width = 0;
    
    // do not bother with decoding operands for NOPs, LOL
    if (primary_uop.type == IB_nop)
       noperands = 0;
    
    for (i=0; i < noperands ; i++) 
    {
        const xed_operand_t* op = xed_inst_operand(xi,i);
        xed_operand_enum_t op_name = xed_operand_name(op);
        bool src = IsSrc(op), dest = IsDest(op);
        // use *first* dest operand...
        if (desti<0 && dest && op_name!=XED_OPERAND_MEM0 && op_name!=XED_OPERAND_MEM1) desti = i;
        switch(op_name) 
        {
           case XED_OPERAND_MEM0:
           case XED_OPERAND_MEM1:
               if (memi < 0) memi = i;
               // for each mem operand create an additional load or store
               // uop, only if the primary_uop is not a memory type already
               if (dest) {
                   if (is_store_uop)
                   {
                      primary_uop.dest_opd[primary_uop.num_dest_operands++] = 
                          make_operand(OperandType_MEMORY, memops);
                   } else if (is_stackpush)
                   {
                      assert (mem_uop);
                      mem_uop->dest_opd[mem_uop->num_dest_operands++] = 
                          make_operand(OperandType_MEMORY, memops);
                   } else if (primary_uop.type == IB_copy)
                   {
                      // copy operation to memory; make it a store
                      primary_uop.dest_opd[primary_uop.num_dest_operands++] = 
                          make_operand(OperandType_MEMORY, memops);
                      primary_uop.type = IB_store;
                      is_store_uop = true;
                   } else
                   {
                      // should I assert that the main uop is not a load_uop? hmm ...
                      // create store uop
                      iList->push_back(instruction_info());
                      instruction_info &store_uop = iList->back();
                      res += 1;
                      store_uop.type = IB_store;
                      store_uop.vec_len = xed_decoded_inst_operand_elements(inst,i);
                      store_uop.width = 
                          xed_decoded_inst_operand_length_bits(inst,i)/store_uop.vec_len;
                      store_uop.exec_unit = GetExecUnit(inst);
                      store_uop.exec_unit_type = GetExecUnitType(inst,i);
                      store_uop.num_src_operands = 1;
                      store_uop.src_opd[0] = make_operand(OperandType_INTERNAL, 
                                                    num_internal_operands);
                      store_uop.num_dest_operands = 1;
                      store_uop.dest_opd[0] = make_operand(OperandType_MEMORY, memops);
                      // add internal dest operand to primary uop
                      primary_uop.dest_opd[primary_uop.num_dest_operands++] = 
                          make_operand(OperandType_INTERNAL, num_internal_operands++);
                   }
               }
               if (src) {
                   if (is_load_uop)
                   {
                      primary_uop.src_opd[primary_uop.num_src_operands++] = 
                          make_operand(OperandType_MEMORY, memops);
                   } else if (is_stackpop)
                   {
                      assert (mem_uop);
                      mem_uop->src_opd[mem_uop->num_src_operands++] = 
                          make_operand(OperandType_MEMORY, memops);
                   } else if (primary_uop.type == IB_copy)
                   {
                      // copy operation to memory; make it a load
                      primary_uop.src_opd[primary_uop.num_src_operands++] = 
                          make_operand(OperandType_MEMORY, memops);
                      primary_uop.type = IB_load;
                      is_load_uop = true;
                   } else
                   {
                      // should I assert that the main uop is not a load_uop? hmm ...
                      // create load uop
                      iList->push_front(instruction_info());
                      instruction_info &load_uop = iList->front();
                      res += 1;

                      load_uop.type = IB_load;
                      load_uop.vec_len = xed_decoded_inst_operand_elements(inst,i);
                      load_uop.width = 
                          xed_decoded_inst_operand_length_bits(inst,i)/load_uop.vec_len;
                      load_uop.exec_unit = GetExecUnit(inst);
                      load_uop.exec_unit_type = GetExecUnitType(inst,i);
                      load_uop.num_src_operands = 1;
                      load_uop.src_opd[0] = make_operand(OperandType_MEMORY, memops);
                      load_uop.num_dest_operands = 1;
                      load_uop.dest_opd[0] = make_operand(OperandType_INTERNAL, 
                                                    num_internal_operands);
                      // add internal source operand to primary uop
                      primary_uop.src_opd[primary_uop.num_src_operands++] = 
                          make_operand(OperandType_INTERNAL, num_internal_operands++);
                   }
               }
               // some operands can be R/W. We want to use the same mem operand index 
               // for both uses. Thus, we increment memops count only once at the end.
               memops += 1;
               break;

           case XED_OPERAND_IMM0: // this must be source 
               if ((primary_uop.imm_values[primary_uop.num_imm_values].is_signed = 
                         xed_decoded_inst_get_immediate_is_signed(inst)) == true)
                  primary_uop.imm_values[primary_uop.num_imm_values].value.s = 
                          xed_decoded_inst_get_signed_immediate(inst);
               else
                  primary_uop.imm_values[primary_uop.num_imm_values].value.u = 
                          xed_decoded_inst_get_unsigned_immediate(inst);
               
               primary_uop.src_opd[primary_uop.num_src_operands++] = 
                       make_operand(OperandType_IMMED, primary_uop.num_imm_values++);
               break;

           case XED_OPERAND_IMM1: // this must be source 
               primary_uop.imm_values[primary_uop.num_imm_values].is_signed = false;
               primary_uop.imm_values[primary_uop.num_imm_values].value.u = 
                           xed_decoded_inst_get_second_immediate(inst);
               primary_uop.src_opd[primary_uop.num_src_operands++] = 
                           make_operand(OperandType_IMMED, primary_uop.num_imm_values++);
               break;

           case XED_OPERAND_PTR:    // pointer (always in conjunction with a IMM0)
           case XED_OPERAND_RELBR:  // branch displacements
           {
               xed_uint_t disp_bits = xed_decoded_inst_get_branch_displacement_width(inst);
               if (disp_bits) {
                  primary_uop.imm_values[primary_uop.num_imm_values].is_signed = true;
                  primary_uop.imm_values[primary_uop.num_imm_values].value.s = 
                             xed_decoded_inst_get_branch_displacement(inst);
                  primary_uop.src_opd[primary_uop.num_src_operands++] = 
                           make_operand(OperandType_IMMED, primary_uop.num_imm_values++);
               }
            }
            break;

           case XED_OPERAND_REG0:
           case XED_OPERAND_REG1:
           case XED_OPERAND_REG2:
           case XED_OPERAND_REG3:
           case XED_OPERAND_REG4:
           case XED_OPERAND_REG5:
           case XED_OPERAND_REG6:
           case XED_OPERAND_REG7:
           case XED_OPERAND_REG8:
// PIN kits up to version 2.13-61206 (inclusive) included operands REG9 to REG 15
// These have been removed from version 2.13-62141
// No good way to test if these enum constants are defined or not
#ifdef XED_OPERAND_REG9  // this is an enum constant, not visible to the preprocessor
           case XED_OPERAND_REG9:
           case XED_OPERAND_REG10:
           case XED_OPERAND_REG11:
           case XED_OPERAND_REG12:
           case XED_OPERAND_REG13:
           case XED_OPERAND_REG14:
           case XED_OPERAND_REG15:
#endif
               {
                  xed_reg_enum_t r = xed_decoded_inst_get_reg(inst, op_name);
                  if (r==XED_REG_STACKPUSH || r==XED_REG_STACKPOP)  // add a stack pointer operation
                  {
                     int64_t stack_inc;
                     stack_width = xed_decoded_inst_operand_length_bits(inst,i)/8;  // width in bytes
                     if (r==XED_REG_STACKPUSH)
                     {
                        is_stackpush = true;
                        stack_inc = -stack_width;
                        if (!is_store_uop)  // must create a separate store uop before primary_uop
                        {
                           mem_it = iList->insert(primary_it, instruction_info());
                           mem_uop = &(*mem_it);
                           mem_uop->type = IB_store;
                        } else
                           mem_it = primary_it;
                           
                        // add a stack uop before the store
                        InstrList::iterator lit = iList->insert(mem_it, instruction_info());
                        stack_uop = &(*lit);
                     } else   // POP
                     {
                        is_stackpop = true;
                        stack_inc = stack_width;
                        if (!is_load_uop)  // must create a separate load uop before primary_uop
                        {
                           mem_it = iList->insert(primary_it, instruction_info());
                           mem_uop = &(*mem_it);
                           mem_uop->type = IB_load;
                        } else
                           mem_it = primary_it;
                           
                        // add a stack uop after the load
                        InstrList::iterator lit = mem_it;
                        ++ lit;
                        lit = iList->insert(lit, instruction_info());
                        stack_uop = &(*lit);
                     }
                     res += 1;  // for the stack_uop
                     if (mem_it != primary_it)  // newly added mem uop
                     {
                        res += 1;
                        mem_uop->vec_len = xed_decoded_inst_operand_elements(inst,i);
                        mem_uop->width = 
                               xed_decoded_inst_operand_length_bits(inst,i)/mem_uop->vec_len;
                        mem_uop->exec_unit = GetExecUnit(inst);
                        mem_uop->exec_unit_type = GetExecUnitType(inst,i);
                     } else    // primary uop is a load/store, set the pointers only
                     {
                        mem_uop = &(*mem_it);
                     } 
                     
                     // also add the stack uop that increments the stack pointer
                     stack_uop->type = IB_add;
                     stack_uop->imm_values[stack_uop->num_imm_values].is_signed = true;
                     stack_uop->imm_values[stack_uop->num_imm_values].value.s = stack_inc;
                     stack_uop->src_opd[stack_uop->num_src_operands++] = 
                              make_operand(OperandType_IMMED, stack_uop->num_imm_values++);
                  } else  // not a stackpush / stackpop pseudo operand
                  {
                     if (dest) {
                         primary_uop.dest_opd[primary_uop.num_dest_operands++] = 
                             make_operand(OperandType_REGISTER, i);
                         if (is_stackpop && mem_uop && mem_uop!=&primary_uop &&
                               mem_uop->num_dest_operands==0)
                         {
                            mem_uop->dest_opd[mem_uop->num_dest_operands++] = 
                                 make_operand(OperandType_REGISTER, i);
                         }
                     }
                     if (src) {
                         if (srci < 0) srci = i;
                  
                         primary_uop.src_opd[primary_uop.num_src_operands++] = 
                             make_operand(OperandType_REGISTER, i);
                         if (is_stackpush && mem_uop && mem_uop!=&primary_uop &&
                               mem_uop->num_src_operands==0)
                         {
                            mem_uop->src_opd[mem_uop->num_src_operands++] = 
                                 make_operand(OperandType_REGISTER, i);
                         }
                     }
                  }
               }
               break;

           case XED_OPERAND_BASE0:
           case XED_OPERAND_BASE1:
               // I noticed that BASE operands are listed only when we have some
               // stackpush or stackpop operation
               {
                  if (!is_stackpush && !is_stackpop)
                  {
                     fprintf(stderr, "HELLO, Special case of STACKPUSH / STACKPOP\n");
                     debug_decode_instruction_at_pc(pc, xed_decoded_inst_get_length(inst));
                  } else
                  {
//                  assert (is_stackpush || is_stackpop);
                     assert (dest && src);
                     assert (stack_uop);
                     stack_uop->dest_opd[stack_uop->num_dest_operands++] = 
                              make_operand(OperandType_REGISTER, i);
                     stack_uop->src_opd[stack_uop->num_src_operands++] = 
                              make_operand(OperandType_REGISTER, i);
                  
                     stack_uop->vec_len = xed_decoded_inst_operand_elements(inst,i);
                     stack_uop->width = 
                               xed_decoded_inst_operand_length_bits(inst,i)/stack_uop->vec_len;
                     stack_uop->exec_unit = GetExecUnit(inst);
                     stack_uop->exec_unit_type = GetExecUnitType(inst,i);
                  }
               }
               break;

           case XED_OPERAND_AGEN:
               if (dest) {
                   primary_uop.dest_opd[primary_uop.num_dest_operands++] = 
                       make_operand(OperandType_MEMORY, memops);
               }
               if (src) {
                   primary_uop.src_opd[primary_uop.num_src_operands++] = 
                       make_operand(OperandType_MEMORY, memops);
               }
               memops += 1;
               break;

           default: // Did we miss something?
               // Unhandled operand type. Print a message. Assert??
               cerr << "Yo, Unknown operand type." << endl;
               assert (!"What operand is this?");
               break;
        }
    }
    
    switch(xclass)
    {
#if 0   // CALL and RET are handled by the regular case for now.
        // If you notice any strange behavior due to CALL/RET, create
        // specialized decoding for them
       case XED_ICLASS_CALL_FAR:           // CALL
       case XED_ICLASS_CALL_NEAR:          // CALL
          res = XedDecodeCallInstruction(inst, iList, pc);
          break;

       case XED_ICLASS_RET_FAR:            // RET
       case XED_ICLASS_RET_NEAR:           // RET
          res = XedDecodeRetInstruction(inst, iList, pc);
          break;
#endif

       case XED_ICLASS_CALL_FAR:           // CALL
       case XED_ICLASS_CALL_NEAR:          // CALL
       {
          // only relative calls should add the RIP register to the
          // source operands for the Jump. Absolute calls should not.
          // Relative calls have a constant displacement. Check for
          // this condition. If I find another register as source
          // operand, then remove the RIP register.
          assert(primary_uop.type==IB_jump);
          int found_reg = 0, found_imm = 0, found_ip = -1;
          for (i=0 ; i<primary_uop.num_src_operands ; ++i)
          {
             int otype = extract_op_type(primary_uop.src_opd[i]);
             if (otype == OperandType_IMMED)
                found_imm += 1;
             else if (otype==OperandType_INTERNAL)
                found_reg += 1;
             else if (otype==OperandType_REGISTER)
             {
                found_reg += 1;
                int oindex = extract_op_index(primary_uop.src_opd[i]);
                
                const xed_operand_t* op = xed_inst_operand(xi, oindex);
                xed_operand_enum_t op_name = xed_operand_name(op);
                xed_reg_enum_t r = xed_decoded_inst_get_reg(inst, op_name);

                if (XED_REG_IP_FIRST<=r && r<=XED_REG_IP_LAST)
                   found_ip = i;
             }
          }
          if (!found_imm && found_reg>1 && found_ip>=0)
          {
             // delete the ip src operand
             for (i=found_ip+1 ; i<primary_uop.num_src_operands ; ++i)
                primary_uop.src_opd[i-1] = primary_uop.src_opd[i];
             primary_uop.num_src_operands -= 1;
          }
          
          // calls push the return address onto the stack
          // However, the callee pops the return address before
          // returing, so I should add a pop uop to maintain the
          // correct stack pointer value
          // add an IB_add instruction which reverse the operation
          // of the existing add instruction
          bool found_add = false;
          InstrList::iterator ilit = iList->begin();
          for ( ; ilit!=iList->end() && !found_add ; ++ilit)
             if (ilit->type == IB_add)  // found the existing add
             {
                iList->push_back(*ilit);
                instruction_info &add2_uop = iList->back();
                res += 1;

                // Change the immediate operand ...
                assert (add2_uop.num_imm_values==1 && 
                        add2_uop.imm_values[0].is_signed);
                add2_uop.imm_values[0].value.s = 
                        -add2_uop.imm_values[0].value.s;
                found_add = true;
             }
          assert (found_add);
          break;
       }

       case XED_ICLASS_INC:
       case XED_ICLASS_DEC:
          // add an immediate src operand to the primary uop
          primary_uop.imm_values[primary_uop.num_imm_values].is_signed = true;
          primary_uop.imm_values[primary_uop.num_imm_values].value.s = 
                      (xclass==XED_ICLASS_INC?1:-1);
          primary_uop.src_opd[primary_uop.num_src_operands++] = 
                           make_operand(OperandType_IMMED, primary_uop.num_imm_values++);
          break;

       case XED_ICLASS_NEG:
          // add an immediate src operand on the first position of a primary uop
          primary_uop.imm_values[primary_uop.num_imm_values].is_signed = true;
          primary_uop.imm_values[primary_uop.num_imm_values].value.s = 0;
          assert (primary_uop.num_src_operands == 1);
          // move the register (or mem operand) to the second position
          primary_uop.src_opd[primary_uop.num_src_operands++] = primary_uop.src_opd[0];
          primary_uop.src_opd[0] = make_operand(OperandType_IMMED, primary_uop.num_imm_values++);
          break;

       case XED_ICLASS_SETB:               // BITBYTE
       case XED_ICLASS_SETBE:              // BITBYTE
       case XED_ICLASS_SETL:               // BITBYTE
       case XED_ICLASS_SETLE:              // BITBYTE
       case XED_ICLASS_SETNB:              // BITBYTE
       case XED_ICLASS_SETNBE:             // BITBYTE
       case XED_ICLASS_SETNL:              // BITBYTE
       case XED_ICLASS_SETNLE:             // BITBYTE
       case XED_ICLASS_SETNO:              // BITBYTE
       case XED_ICLASS_SETNP:              // BITBYTE
       case XED_ICLASS_SETNS:              // BITBYTE
       case XED_ICLASS_SETNZ:              // BITBYTE
       case XED_ICLASS_SETO:               // BITBYTE
       case XED_ICLASS_SETP:               // BITBYTE
       case XED_ICLASS_SETS:               // BITBYTE
       case XED_ICLASS_SETZ:               // BITBYTE
          // these instructions perform conditional move
          // they can set the destination operand to 0 or 1
          // assume condition is false; add a 0 imm value.
          primary_uop.imm_values[primary_uop.num_imm_values].is_signed = true;
          primary_uop.imm_values[primary_uop.num_imm_values].value.s = 0;
          primary_uop.src_opd[primary_uop.num_src_operands++] = 
                           make_operand(OperandType_IMMED, primary_uop.num_imm_values++);
          break;
          
       default:
          break;
    }

    assert(primary_uop.num_src_operands <= max_num_src_operands);
    assert(primary_uop.num_dest_operands <= max_num_dest_operands);
    assert(primary_uop.num_imm_values <= max_num_imm_values);
    
    // DIVs with 3 source operands have only 1 explicit operand, the divisor.
    // The dividend is implicit and is composed of two operands EDX:EAX
    // However, Xed gives us the explicit operand first, that is the divisor
    // I want to have the two dividend operands listed first, and the divisor be last
    if (primary_uop.num_src_operands==3 && primary_uop.type==IB_div)
    {
       uint8_t top = primary_uop.src_opd[0];
       primary_uop.src_opd[0] = primary_uop.src_opd[1];
       primary_uop.src_opd[1] = primary_uop.src_opd[2];
       primary_uop.src_opd[2] = top;
    }

    int opi = -1;
    if (is_copy && memi>=0) opi = memi;
    else if (desti >= 0) opi = desti;
    else if (srci >= 0) opi = srci;
    else if (memi >= 0) opi = memi;
    if (opi>=0) {
        primary_uop.vec_len = GetNumberOfElements(inst, opi);
        // To find the bit width of an element, I must use the number of
        // elements returned by XED, not the fixed value from MIAMI.
        primary_uop.width = xed_decoded_inst_operand_length_bits(inst, opi) /
                  xed_decoded_inst_operand_elements(inst, opi);
        primary_uop.exec_unit_type = GetExecUnitType(inst, opi);
    } 
    else {
        primary_uop.vec_len = 0;
        primary_uop.width = 0;
        primary_uop.exec_unit_type = ExecUnitType_INT;
    }

    // if instruction has REP prefix, add also a branch micro-op
    if (xed_operand_values_has_real_rep(xed_decoded_inst_operands_const(inst)))
    {
       assert(!"All stringops are processed separately. Why am I here?");
       iList->push_back(instruction_info());
       instruction_info &br_uop = iList->back();
       res += 1;
       
       br_uop.type = IB_branch;
       br_uop.vec_len = 0;
       br_uop.width = 0;
       br_uop.exec_unit = GetExecUnit(inst);
       br_uop.exec_unit_type = ExecUnitType_INT;
       br_uop.num_src_operands = 0;
       br_uop.num_dest_operands = 0;
    }
    
    return (res);
}

static int
decode_instruction(void *pc, int len, xed_decoded_inst_t *xedd)
{
   xed_decoded_inst_zero_set_mode(xedd, &xed_dstate);
   if (len > XED_MAX_INSTRUCTION_BYTES)
      len = XED_MAX_INSTRUCTION_BYTES;
      
#if DEBUG_INST_DECODE
   cerr << "Attempting to decode instruction at " << hex << pc << dec 
        << ", len=" << len << endl;
#endif

#if USE_XED_DECODE_CACHE
   xed_error_enum_t xed_error = xed_decode_cache(xedd,
                                    XED_REINTERPRET_CAST(const xed_uint8_t*,pc),
                                    len, &cache);
#else
   xed_error_enum_t xed_error = xed_decode(xedd,
                                    XED_REINTERPRET_CAST(const xed_uint8_t*,pc),
                                    len);
#endif

   // The len parameter specifies how many bytes are left till the end of the current block.
   // When I construct blocks, I split them either based on the PIN BBL boundaries
   // or I split existing blocks if a branch jumps in the middle of one. I hope PIN terminates
   // BBLs at instruction boundaries (or it makes things complicated). However, in optimzed codes
   // it is possible to have a branch jump in the middle of an instruction with both encodings 
   // being legal. That is, an instruction may have a 1+ byte prefix which still makes it legal, 
   // but the prefix by itself is not forming a legal instruction.
   // I found such a case in routine __cxa_finalize part of the C++ standard library.
   // For this reason, if I get the XED_ERROR_BUFFER_TOO_SHORT error, I will attempt invoking the 
   // decoding routine again with a longer length.
   if (xed_error == XED_ERROR_BUFFER_TOO_SHORT)  // try a longer buffer
   {
#if DEBUG_INST_DECODE
      cerr << "Warning: Decoding of " << len << " bytes at address 0x" << hex << pc 
           << " has failed due to insuficient bytes provided. Repeating with more bytes."
           << dec << endl;
#endif
      xed_decoded_inst_zero_keep_mode(xedd);
      
#if USE_XED_DECODE_CACHE
      xed_error = xed_decode_cache(xedd,
                                    XED_REINTERPRET_CAST(const xed_uint8_t*,pc),
                                    XED_MAX_INSTRUCTION_BYTES, &cache);
#else
      xed_error = xed_decode(xedd,
                                    XED_REINTERPRET_CAST(const xed_uint8_t*,pc),
                                    XED_MAX_INSTRUCTION_BYTES);
#endif

#if DEBUG_INST_DECODE
      fprintf(stderr, "Decoding with %d bytes returned code %d\n", 
            XED_MAX_INSTRUCTION_BYTES, xed_error);
#endif
   }
   
   if (xed_error != XED_ERROR_NONE)
   {
      cerr << "decode_instruction: Error " << xed_error << " while decoding instruction at " 
           << hex << pc << " with max length of " << dec << len << " bytes." << endl;
   }

   switch(xed_error)
   {
      case XED_ERROR_NONE:
         break;
      case XED_ERROR_BUFFER_TOO_SHORT:
         cerr << "Not enough bytes provided" << endl;
         break;
      case XED_ERROR_INVALID_FOR_CHIP:
         cerr << "The instruction was not valid for the specified chip." << endl;
         break;
      case XED_ERROR_GENERAL_ERROR:
         cerr << "Could not decode given input." << endl;
         break;
      default:
         cerr << "Unhandled error code " << xed_error_enum_t2str(xed_error) << endl;
         break;
   }
   
   if (xed_error != XED_ERROR_NONE)
      return (-xed_error);

#if DEBUG_INST_DECODE
   cerr << "iclass "
        << xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(xedd))  << "\t";
   cerr << "category "
        << xed_category_enum_t2str(xed_decoded_inst_get_category(xedd))  << "\t";
   cerr << "ISA-extension "
        << xed_extension_enum_t2str(xed_decoded_inst_get_extension(xedd)) << "\t";
   cerr << "ISA-set "  
        << xed_isa_set_enum_t2str(xed_decoded_inst_get_isa_set(xedd))  << endl;
   cerr << "instruction-length " 
        << xed_decoded_inst_get_length(xedd) << "   ";

   cerr << "operand-width "
        << xed_decoded_inst_get_operand_width(xedd)  << "   ";
   cerr << "effective-operand-width "
        << xed_operand_values_get_effective_operand_width(xed_decoded_inst_operands_const(xedd))
        << "   ";
   cerr << "effective-address-width "
        << xed_operand_values_get_effective_address_width(xed_decoded_inst_operands_const(xedd))
        << endl;
   cerr << "stack-address-width "
        << xed_operand_values_get_stack_address_width(xed_decoded_inst_operands_const(xedd))
        << "   ";
   cerr << "iform-enum-name "
        << xed_iform_enum_t2str(xed_decoded_inst_get_iform_enum(xedd)) << "   ";
   cerr << "iform-enum-name-dispatch (zero based) "
        << xed_decoded_inst_get_iform_enum_dispatch(xedd) << "   ";
   cerr << "iclass-max-iform-dispatch "
        << xed_iform_max_per_iclass(xed_decoded_inst_get_iclass(xedd))  << endl;
    // operands
    // operands
    print_operands(xedd);
    // memops
    print_memops(xedd);
#endif

   return (0);
}

/* Decode the instruction at given pc and build a list of micro-ops.
 * @parameter len specifies how many bytes are available for decoding.
 * @parameter dInst is an output parameter receiving the micro-ops list.
 */
int 
decode_instruction_at_pc(void *pc, int len, DecodedInstruction *dInst)
{
   xed_decoded_inst_t xedd;
   
   int res = decode_instruction(pc, len, &xedd);
   if (res < 0)
      return (res);
   
   dInst->len = xed_decoded_inst_get_length(&xedd);
   dInst->pc = (addrtype)pc;
   dInst->is_call = (xed_decoded_inst_get_category(&xedd)==XED_CATEGORY_CALL);
/*
   if (dInst->is_call)  // print the call target
   {
      xed_uint_t disp_bits = xed_decoded_inst_get_branch_displacement_width(&xedd);
      xed_int32_t disp = 0;
      if (disp_bits) {
         disp = xed_decoded_inst_get_branch_displacement(&xedd);
      }
      fprintf(stderr, "Found CALL instruction, pc %p, displacement bits %d, displacement 0x%" PRIx32 
            ", target=0x%" PRIxaddr "\n",
            pc, disp_bits, disp, (addrtype)pc+dInst->len+disp);
   }
*/
   dInst->mach_data = malloc(sizeof(xed_decoded_inst_t));
   if (! dInst->mach_data)  // cannot allocate memory ??
   {
      fprintf(stderr, "ERROR: Instruction decode: cannot allocate memory for machine dependent data\n");
      return (-1);
   }
   memcpy(dInst->mach_data, &xedd, sizeof(xed_decoded_inst_t));
   // decide if we need to store anything in mach_data2
   
   res = XedInstToUopList(&xedd, &(dInst->micro_ops), pc);
   // traverse all the micro-ops and change ExecUnit from VECTOR to SCALAR if vec_len==1
   InstrList::iterator ilit = dInst->micro_ops.begin();
   for ( ; ilit!=dInst->micro_ops.end() ; ++ilit)
      if (ilit->exec_unit==ExecUnit_VECTOR && ilit->vec_len==1)
         ilit->exec_unit = ExecUnit_SCALAR;
   return (res);
}

/* return length of instruction at given pc (in bytes).
 * @parameter len specifies how many bytes are available for decoding.
 */
int 
length_instruction_at_pc(void *pc, int len)
{
   xed_decoded_inst_t xedd;
   
   int res = decode_instruction(pc, len, &xedd);
   if (res < 0)
      return (res);
   
   return (xed_decoded_inst_get_length(&xedd));
}

/* debug instruction at given pc and print debug information about it.
 * @parameter len specifies how many bytes are available for decoding.
 */
int 
debug_decode_instruction_at_pc(void *pc, int len)
{
   xed_decoded_inst_t xedd;

   int res = decode_instruction(pc, len, &xedd);
   if (res < 0)
      return (res);
   
   // if DEBUG_INST_DECODE is enabled, then the print-out happens inside
   // decode_instruction. Otherwise, print that stuff here.
#if !DEBUG_INST_DECODE
   cerr << "iclass "
        << xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&xedd))  << "\t";
   cerr << "category "
        << xed_category_enum_t2str(xed_decoded_inst_get_category(&xedd))  << "\t";
   cerr << "ISA-extension "
        << xed_extension_enum_t2str(xed_decoded_inst_get_extension(&xedd)) << "\t";
   cerr << "ISA-set "  
        << xed_isa_set_enum_t2str(xed_decoded_inst_get_isa_set(&xedd))  << endl;
   cerr << "instruction-length " 
        << xed_decoded_inst_get_length(&xedd) << "   ";

   cerr << "operand-width "
        << xed_decoded_inst_get_operand_width(&xedd)  << "   ";
   cerr << "effective-operand-width "
        << xed_operand_values_get_effective_operand_width(xed_decoded_inst_operands_const(&xedd))
        << "   ";
   cerr << "effective-address-width "
        << xed_operand_values_get_effective_address_width(xed_decoded_inst_operands_const(&xedd))
        << endl;
   cerr << "stack-address-width "
        << xed_operand_values_get_stack_address_width(xed_decoded_inst_operands_const(&xedd))
        << "   ";
   cerr << "iform-enum-name "
        << xed_iform_enum_t2str(xed_decoded_inst_get_iform_enum(&xedd)) << "   ";
   cerr << "iform-enum-name-dispatch (zero based) "
        << xed_decoded_inst_get_iform_enum_dispatch(&xedd) << "   ";
   cerr << "iclass-max-iform-dispatch "
        << xed_iform_max_per_iclass(xed_decoded_inst_get_iclass(&xedd))  << endl;
    // operands
    // operands
    print_operands(&xedd);
    // memops
    print_memops(&xedd);
#endif

   return (0);
}


int
dump_instruction_at_pc(void *pc, int len)
{
  std::cout << "========== XED Instruction "
	    << "(" << std::hex << pc << std::dec << ", upto " << len << " bytes)"
	    << " ==========\n";

  xed_decoded_inst_t xedd;
  int res = decode_instruction(pc, len, &xedd);

  if (res < 0) {
    return -1;
  }

  int real_len = xed_decoded_inst_get_length(&xedd);

  const int bufSz = 1024;
  char buf[bufSz] = {};
  xed_decoded_inst_dump(&xedd, buf, bufSz);
  std::cout << buf << " <len=" << real_len << " bytes>\n";

  return real_len;
}
  

/* Check if the given instruction can execute multiple times.
 * This is the case with REP prefixed instructions on x86.
 */
int 
mach_inst_stutters(const DecodedInstruction *dInst)
{
    assert (dInst->mach_data);
    const xed_decoded_inst_t* xedd = static_cast<xed_decoded_inst_t*>(dInst->mach_data);
    
    // if instruction has REP prefix, add also a branch micro-op
    if (xed_operand_values_has_real_rep(xed_decoded_inst_operands_const(xedd)))
       return (1);
    else
       return (0);
}

/* Check if the instruction at the given pc can execute multiple times.
 * This is the case with REP prefixed instructions on x86.
 */
int 
instruction_at_pc_stutters(void *pc, int len, int& ilen)
{
   xed_decoded_inst_t xedd;

   int res = decode_instruction(pc, len, &xedd);
   if (res < 0)
      return (res);

   ilen = xed_decoded_inst_get_length(&xedd);
   // check if instruction has REP prefix
   if (xed_operand_values_has_real_rep(xed_decoded_inst_operands_const(&xedd)))
      return (1);
   else
      return (0);
}

/* Check if the instruction at the given memory location is a control
 * transfer instruction (terminating a basic block), and return also
 * the length in bytes of the encoded machine instruction.
 */
int instruction_at_pc_transfers_control(void *pc, int len, int& ilen)
{
   xed_decoded_inst_t xedd;
   
   int res = decode_instruction(pc, len, &xedd);
   if (res < 0)
      return (res);

   ilen = xed_decoded_inst_get_length(&xedd);
   InstrBin ibin = GetInstrBin(&xedd);
    if (ibin == IB_unknown)  // an unknown opcode
    {
       // list the xed decoded instructions
       fprintf(stderr, "instruction_at_pc_transfers_control: HELLO, MIAMI does not handle the following opcode. Please report this error to mgabi99@gmail.com\n");
       debug_decode_instruction_at_pc(pc, xed_decoded_inst_get_length(&xedd));
    }
   return (InstrBinIsBranchType(ibin));
}


RegName register_name_actual(RegName reg)
{
#if USE_CUSTOM_REGISTER_TRANSLATION
   return (xed_register_names[reg]);
#else
   return (xed_get_largest_enclosing_register((xed_reg_enum_t)reg));
#endif
}

/* return the possible return registers for the given mico-op.
 * The micro-op information can be ignored if it does not encode the
 * return register infor. For Linux x86-64, return values can be stored in
 * RAX, RDX, XMM0-1, YMM0-1, depending on return type. I can include all of
 * them because these are scratch registers. They are not supposed to be
 * live across function calls (unless very specialized code).
 * @append - specified that registers should be pushed onto the list without
 * clearing previous content.
 */
int
mach_inst_return_registers(const DecodedInstruction *dInst, const instruction_info *ii, 
                RInfoList &regs, bool append)
{
   if (!append)
      regs.clear();
   regs.push_back(register_info(XED_REG_RAX, RegisterClass_REG_OP, 0, 63, 0));
   regs.push_back(register_info(XED_REG_RDX, RegisterClass_REG_OP, 0, 63, 0));
   regs.push_back(register_info(XED_REG_XMM0, RegisterClass_REG_OP, 0, 127, 0));
   regs.push_back(register_info(XED_REG_XMM1, RegisterClass_REG_OP, 0, 127, 0));
   regs.push_back(register_info(XED_REG_YMM0, RegisterClass_REG_OP, 0, 255, 0));
   regs.push_back(register_info(XED_REG_YMM1, RegisterClass_REG_OP, 0, 255, 0));
   return (0);
}

/* return the register used for the param-th parameter of type int or fp.
 * Some ABIs have separate counts for interger and fp parameters, so one
 * can have 6 int and 8 fp register parameters for a total of 14. These are
 * the values for Linux x86-64. On the other hand, some ABIs define a total
 * limit and a parameter can be in one register or another depending on its
 * type. This is the case for Win-64.
 * Counting starts from 1.
 */
register_info 
register_for_int_func_parameter(int param)
{
   switch (param)
   {
      case 1: return (register_info(XED_REG_RDI, RegisterClass_REG_OP, 0, 63, 0));
      case 2: return (register_info(XED_REG_RSI, RegisterClass_REG_OP, 0, 63, 0));
      case 3: return (register_info(XED_REG_RDX, RegisterClass_REG_OP, 0, 63, 0));
      case 4: return (register_info(XED_REG_RCX, RegisterClass_REG_OP, 0, 63, 0));
      case 5: return (register_info(XED_REG_R8, RegisterClass_REG_OP, 0, 63, 0));
      case 6: return (register_info(XED_REG_R9, RegisterClass_REG_OP, 0, 63, 0));
      default: return (MIAMI_NO_REG);
   }
   // should not get here
   return (MIAMI_NO_REG);
}

register_info 
register_for_fp_func_parameter(int param)
{
   switch (param)
   {
      case 1: return (register_info(XED_REG_XMM0, RegisterClass_REG_OP, 0, 127, 0));
      case 2: return (register_info(XED_REG_XMM1, RegisterClass_REG_OP, 0, 127, 0));
      case 3: return (register_info(XED_REG_XMM2, RegisterClass_REG_OP, 0, 127, 0));
      case 4: return (register_info(XED_REG_XMM3, RegisterClass_REG_OP, 0, 127, 0));
      case 5: return (register_info(XED_REG_XMM4, RegisterClass_REG_OP, 0, 127, 0));
      case 6: return (register_info(XED_REG_XMM5, RegisterClass_REG_OP, 0, 127, 0));
      case 7: return (register_info(XED_REG_XMM6, RegisterClass_REG_OP, 0, 127, 0));
      case 8: return (register_info(XED_REG_XMM7, RegisterClass_REG_OP, 0, 127, 0));
      default: return (MIAMI_NO_REG);
   }
   // should not get here
   return (MIAMI_NO_REG);
}

/* return true if the ABI defines that params of both fp and int types
 * are counted together to determine the parameter register.
 * return false if the ABI defines that int and fp parameters must be
 * separately counted.
 */
bool combined_int_fp_param_register_count()
{
   /* on Linux x86-64 int and fp parameters are separately counted */
   return (false);
}

/* return the machine register name for the given register name.
 */
const char* 
register_name_to_string(RegName rn)
{
   return (xed_reg_enum_t2str((xed_reg_enum_t)rn));
}

// initialization function. 
// Call this function once before any other function in this file.
// Includes machine specific initialization code
void
instruction_decoding_init(void *arg0)
{
   xed_tables_init();
   xed_state_zero(&xed_dstate);
   if (sizeof(addrtype) == 8)
      xed_dstate.mmode=XED_MACHINE_MODE_LONG_64;
   else
   {
      xed_dstate.mmode=XED_MACHINE_MODE_LEGACY_32;
      xed_dstate.stack_addr_width=XED_ADDRESS_WIDTH_32b;
   }
   
#if USE_XED_DECODE_CACHE
   // initialize a decode cache of 4096 entries
   // once per thread, we have only one thread for now
   xed_uint32_t n_cache_entries = 4*1024;
   xed_decode_cache_entry_t* cache_entries = 
       (xed_decode_cache_entry_t*) malloc(n_cache_entries * 
                                     sizeof(xed_decode_cache_entry_t));
   xed_decode_cache_initialize(&cache, cache_entries, n_cache_entries);
#endif

#if USE_CUSTOM_REGISTER_TRANSLATION
   // initialize here the register name map; try to make it as independent 
   // of the XED version as possible
   for (int i=0 ; i<XED_REG_LAST ; ++i)
   {
      if (i>=XED_REG_GPR16_FIRST && i<=XED_REG_GPR16_LAST)
         xed_register_names[i] = i - XED_REG_GPR16_FIRST + XED_REG_GPR64_FIRST;
      else if (i>=XED_REG_GPR32_FIRST && i<=XED_REG_GPR32_LAST)
         xed_register_names[i] = i - XED_REG_GPR32_FIRST + XED_REG_GPR64_FIRST;
      else if (i>=XED_REG_GPR8_FIRST && i<=XED_REG_GPR8_LAST)
         xed_register_names[i] = i - XED_REG_GPR8_FIRST + XED_REG_GPR64_FIRST;
      else if (i>=XED_REG_GPR8H_FIRST && i<=XED_REG_GPR8H_LAST)
         xed_register_names[i] = i - XED_REG_GPR8H_FIRST + XED_REG_GPR64_FIRST;
      else
         xed_register_names[i] = i;
   }
#endif
}

// finalization function. 
// Call this function once at the end to clean-up memory and what not.
// Includes machine specific clean-up and finalization code
void
instruction_decoding_fini(void *arg0)
{
#if USE_XED_DECODE_CACHE
   free(cache.entries);
#endif
}

/* translate the xed register name into the proper
 * stack operation. The stack index has been passed
 * as part of the register decode step.
 */
int 
stack_operation_top_increment_value(int reg)
{
   switch (reg)
   {
      case XED_REG_X87PUSH: return (1);
      case XED_REG_X87POP:  return (-1);
      case XED_REG_X87POP2: return (-2);
      default:
         assert(!"Invalid stack operation.");
   }
   return (0);
}

static int
dest_register_lsb_bit(const xed_decoded_inst_t* xedd, int op_num)
{
    switch (xed_decoded_inst_get_iclass(xedd)) 
    {
       case XED_ICLASS_MOVHLPS:            // DATAXFER
          return (0);
          
       case XED_ICLASS_MOVHPD:             // DATAXFER
       case XED_ICLASS_MOVHPS:             // DATAXFER
          return (64);

       case XED_ICLASS_MOVLHPS:            // DATAXFER
          return (64);
          
       case XED_ICLASS_MOVLPD:             // DATAXFER
       case XED_ICLASS_MOVLPS:             // DATAXFER
          return (0);

       case XED_ICLASS_VMOVHLPS:           // AVX
          return (0);
          
       case XED_ICLASS_VMOVLHPS:           // AVX
          return (0);
          
       case XED_ICLASS_VMOVHPD:            // DATAXFER
       case XED_ICLASS_VMOVHPS:            // DATAXFER
          return (0);
          
       case XED_ICLASS_VMOVLPD:            // DATAXFER
       case XED_ICLASS_VMOVLPS:            // DATAXFER
          return (0);
       
       default:
          return (0);
    }
}

static int
src_register_lsb_bit(const xed_decoded_inst_t* xedd, int op_num)
{
    switch (xed_decoded_inst_get_iclass(xedd)) 
    {
       case XED_ICLASS_MOVHLPS:            // DATAXFER
          return (64);
          
       case XED_ICLASS_MOVHPD:             // DATAXFER
       case XED_ICLASS_MOVHPS:             // DATAXFER
          return (64);
          
       case XED_ICLASS_MOVLHPS:            // DATAXFER
          return (0);
          
       case XED_ICLASS_MOVLPD:             // DATAXFER
       case XED_ICLASS_MOVLPS:             // DATAXFER
          return (0);

       case XED_ICLASS_VMOVHLPS:           // AVX
          return (64);

       case XED_ICLASS_VMOVLHPS:           // AVX
          return (0);
       
       case XED_ICLASS_VMOVHPD:            // DATAXFER
       case XED_ICLASS_VMOVHPS:            // DATAXFER
          return (0);
          
       case XED_ICLASS_VMOVLPD:            // DATAXFER
       case XED_ICLASS_VMOVLPS:            // DATAXFER
          return (0);
       
       default:
          return (0);
    }
}

/* VEX.128 encoded versions of AVX instructions reset to 0 bits VLMAX-1:128.
 * However, XED reports operand length as 128. To compute dependencies 
 * correctly, I need to understand that the higher 128 bits are killed by 
 * such an instruction.
 */
static int
dest_register_killed_bits(const xed_decoded_inst_t* xedd, int op_num)
{
    xed_extension_enum_t iext = xed_decoded_inst_get_extension(xedd);
    int op_len = 0;
    // I think XED does a decent job of getting operand length
    // The only inaccuracies are with AVX instructions because the
    // semantic has changed to reset to 0 any unmodified bits.
    // Right now, I cannot think of any AVX instruction that preserves
    // any bits of a register. Therefore, I just test for the extension
    // and return 256 for all AVX instructions. Use XED for everything else.
    // If I find any exceptions from this rule, I will have to handle
    // individual opcodes separately.
    if (iext==XED_EXTENSION_AVX || iext==XED_EXTENSION_AVX2)
       op_len = 256;
    else
       op_len = xed_decoded_inst_operand_length_bits(xedd, op_num);
    return (op_len);
}

/* return the register associated with the register 
 * operand op_num
 */
register_info
mach_inst_reg_operand(const DecodedInstruction *dInst, int op_num)
{
    assert (dInst->mach_data);
    const xed_decoded_inst_t* xedd = static_cast<xed_decoded_inst_t*>(dInst->mach_data);
    const xed_inst_t* xi = xed_decoded_inst_inst(xedd);
//    const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xedd);

    const xed_operand_t* op = xed_inst_operand(xi,op_num);
    xed_operand_enum_t op_name = xed_operand_name(op);
    xed_reg_enum_t r = xed_decoded_inst_get_reg(xedd, op_name);
    RegisterClass rclass = RegisterClass_REG_OP;
    xed_reg_class_enum_t xed_class = xed_reg_class(r);
    stack_index_t stack = 0;
    int rname = register_name_actual(r);
    if (xed_class == XED_REG_CLASS_PSEUDO) {
       rclass = RegisterClass_PSEUDO;
       if (r==XED_REG_X87PUSH || r==XED_REG_X87POP || r==XED_REG_X87POP2) {
          rclass = RegisterClass_STACK_OPERATION;
          stack = 1;
       }
    }
    else if (xed_class == XED_REG_CLASS_X87)
    {
       rclass = RegisterClass_STACK_REG;
       rname = r - XED_REG_ST0;  // store the offset of the register relative to top of stack
       stack = 1;
    }
    
    int lsb = 0;
    int killed_bits = 0;
    bool dest = IsDest(op);
    
    if (dest) {
       lsb = dest_register_lsb_bit(xedd, op_num);
       killed_bits = dest_register_killed_bits(xedd, op_num);
    }
    else {
       lsb = src_register_lsb_bit(xedd, op_num);
       killed_bits = xed_decoded_inst_operand_length_bits(xedd,op_num);
    }
    return (register_info(rname, rclass, lsb, 
          lsb+killed_bits-1, stack));
}


/* return the i-th register of memory operand op_num
 */
register_info
mach_inst_mem_operand_reg(const DecodedInstruction *dInst, int op_num, int i)
{
    assert (dInst->mach_data);
    const xed_decoded_inst_t* xedd = static_cast<xed_decoded_inst_t*>(dInst->mach_data);

    int cnt = 0;
    RegisterClass rclass = RegisterClass_MEM_OP;
    if ( !xed_decoded_inst_mem_read(xedd,op_num) &&
         !xed_decoded_inst_mem_written(xedd,op_num))
       rclass = RegisterClass_LEA_OP;
    int addr_width = xed_decoded_inst_get_memop_address_width(xedd,op_num);
    
    xed_reg_enum_t r = xed_decoded_inst_get_seg_reg(xedd, op_num);
    if (r != XED_REG_INVALID) 
    {
#if DEBUG_REGISTER_EXTRACTION
       cerr << "SEG= " << xed_reg_enum_t2str(r) << " ";
#endif
       if (i == cnt) 
          return (register_info(r, rclass, 0, addr_width-1));
       cnt += 1;
    }
    r = xed_decoded_inst_get_base_reg(xedd, op_num);
    if (r != XED_REG_INVALID) 
    {
#if DEBUG_REGISTER_EXTRACTION
       cerr << "BASE= " << setw(3) << xed_reg_enum_t2str(r) << "/" 
            << setw(3)
            <<  xed_reg_class_enum_t2str(xed_reg_class(r)) << " "; 
#endif
       if (i == cnt) 
          return (register_info(r, rclass, 0, addr_width-1));
       cnt += 1;
    }
    r = xed_decoded_inst_get_index_reg(xedd, op_num);
    if (op_num==0 && r!=XED_REG_INVALID)
    // taken from xed-ex1.cpp; only the first mem operand can have an index register??
    {
#if DEBUG_REGISTER_EXTRACTION
       cerr << "INDEX= " << setw(3) << xed_reg_enum_t2str(r)
            << "/" <<  setw(3) 
            << xed_reg_class_enum_t2str(xed_reg_class(r)) << " ";
#endif
       if (i == cnt) 
          return (register_info(r, rclass, 0, addr_width-1));
       cnt += 1;
    }
#if DEBUG_REGISTER_EXTRACTION
    if (cnt > 0)
       cerr << endl;
#endif
    return (MIAMI_NO_REG);
}

/* build a list with all registers read by micro-op ii part of decoded instruction dInst
 */
int
mach_inst_src_registers(const DecodedInstruction *dInst, const instruction_info *ii, 
                RInfoList &regs, bool internal)
{
   assert (dInst->mach_data);
   const xed_decoded_inst_t* xedd = static_cast<xed_decoded_inst_t*>(dInst->mach_data);
   const xed_inst_t* xi = xed_decoded_inst_inst(xedd);

   uint8_t i;
   regs.clear();
   for (i=0 ; i<ii->num_src_operands ; ++i)
   {
      int op_num = extract_op_index(ii->src_opd[i]);
      switch (extract_op_type(ii->src_opd[i]))
      {
         case OperandType_MEMORY:
            {
               RegisterClass rclass = RegisterClass_MEM_OP;
               if ( !xed_decoded_inst_mem_read(xedd,op_num) &&
                    !xed_decoded_inst_mem_written(xedd,op_num))
                  rclass = RegisterClass_LEA_OP;
               int addr_width = xed_decoded_inst_get_memop_address_width(xedd,op_num);
               xed_reg_enum_t r = xed_decoded_inst_get_seg_reg(xedd, op_num);
               if (r != XED_REG_INVALID) {
#if DEBUG_REGISTER_EXTRACTION
                  cerr << "MEM Src SEG= " << r << " - " << xed_reg_enum_t2str(r) << " ";
#endif
                  regs.push_back(register_info(r, rclass, 0, addr_width-1));
               }
               r = xed_decoded_inst_get_base_reg(xedd, op_num);
               if (r != XED_REG_INVALID) {
#if DEBUG_REGISTER_EXTRACTION
                  cerr << "MEM Src BASE= " << r << " - " << setw(3) << xed_reg_enum_t2str(r) << "/" 
                       << setw(3)
                       <<  xed_reg_class_enum_t2str(xed_reg_class(r)) << " "; 
#endif
                  regs.push_back(register_info(r, rclass, 0, addr_width-1));
               }
               r = xed_decoded_inst_get_index_reg(xedd, op_num);
               if (op_num==0 && r!=XED_REG_INVALID) {
               // taken from xed-ex1.cpp; only the first mem operand can have an index register??
#if DEBUG_REGISTER_EXTRACTION
                  cerr << "MEM Src INDEX= " << r << " - " << setw(3) << xed_reg_enum_t2str(r)
                       << "/" <<  setw(3) 
                       << xed_reg_class_enum_t2str(xed_reg_class(r)) << " ";
#endif
                  regs.push_back(register_info(r, rclass, 0, addr_width-1));
               }
            }
            break;
            
         case OperandType_REGISTER:
            {
               const xed_operand_t* op = xed_inst_operand(xi, op_num);
               xed_operand_enum_t op_name = xed_operand_name(op);
               xed_reg_enum_t r = xed_decoded_inst_get_reg(xedd, op_name);
               
               if (r != XED_REG_INVALID) 
               {
                  RegisterClass rclass = RegisterClass_REG_OP;
                  xed_reg_class_enum_t xed_class = xed_reg_class(r);
                  stack_index_t stack = 0;
                  int rname = r;
                  if (xed_class == XED_REG_CLASS_PSEUDO) {
                     rclass = RegisterClass_PSEUDO;
                     if (r==XED_REG_X87PUSH || r==XED_REG_X87POP || r==XED_REG_X87POP2) {
                        rclass = RegisterClass_STACK_OPERATION;
                        stack = 1;
                     }
                  } else if (xed_class == XED_REG_CLASS_X87)
                  {
                     rclass = RegisterClass_STACK_REG;
                     rname = r - (int)XED_REG_ST0;  // store the offset of the register relative to top of stack
                     stack = 1;
                  }
#if DEBUG_REGISTER_EXTRACTION
                  cerr << "Src REG= " << rname << " - " << setw(3) << xed_reg_enum_t2str(r)
                       << "/" <<  setw(3) 
                       << xed_reg_class_enum_t2str(xed_reg_class(r)) << " ";
#endif
                  int lsb = src_register_lsb_bit(xedd, op_num);
                  regs.push_back(register_info(rname, rclass, lsb,
                          lsb+xed_decoded_inst_operand_length_bits(xedd,op_num)-1, stack));
               }
            }
            break;

         case OperandType_INTERNAL:
            {
               if (internal) 
               {
                  int rwidth = ii->width;
                  if (ii->exec_unit==ExecUnit_VECTOR)
                     rwidth *= ii->vec_len;
                  regs.push_back(register_info(op_num, RegisterClass_TEMP_REG, 0, rwidth-1));
               }
            }
            break;
      }
   }
   
   // registers used as part of address computation for destination memory operands
   // clasify also as source registers
   for (i=0 ; i<ii->num_dest_operands ; ++i)
   {
      int op_num = extract_op_index(ii->dest_opd[i]);
      switch (extract_op_type(ii->dest_opd[i]))
      {
         case OperandType_MEMORY:
            {
               RegisterClass rclass = RegisterClass_MEM_OP;
               if ( !xed_decoded_inst_mem_read(xedd,op_num) &&
                    !xed_decoded_inst_mem_written(xedd,op_num))
                  rclass = RegisterClass_LEA_OP;
               int addr_width = xed_decoded_inst_get_memop_address_width(xedd,op_num);
               xed_reg_enum_t r = xed_decoded_inst_get_seg_reg(xedd, op_num);
               if (r != XED_REG_INVALID) {
#if DEBUG_REGISTER_EXTRACTION
                  cerr << "MEM Dest SEG= " << r << " - " << xed_reg_enum_t2str(r) << " ";
#endif
                  regs.push_back(register_info(r, rclass, 0, addr_width-1));
               }
               r = xed_decoded_inst_get_base_reg(xedd, op_num);
               if (r != XED_REG_INVALID) {
#if DEBUG_REGISTER_EXTRACTION
                  cerr << "MEM Dest BASE= " << r << " - " << setw(3) << xed_reg_enum_t2str(r) << "/" 
                       << setw(3)
                       <<  xed_reg_class_enum_t2str(xed_reg_class(r)) << " "; 
#endif
                  regs.push_back(register_info(r, rclass, 0, addr_width-1));
               }
               r = xed_decoded_inst_get_index_reg(xedd, op_num);
               if (op_num==0 && r!=XED_REG_INVALID) {
               // taken from xed-ex1.cpp; only the first mem operand can have an index register??
#if DEBUG_REGISTER_EXTRACTION
                  cerr << "MEM Dest INDEX= " << r << " - " << setw(3) << xed_reg_enum_t2str(r)
                       << "/" <<  setw(3) 
                       << xed_reg_class_enum_t2str(xed_reg_class(r)) << " ";
#endif
                  regs.push_back(register_info(r, rclass, 0, addr_width-1));
               }
            }
            break;
      }
   }
#if DEBUG_REGISTER_EXTRACTION
   if (!regs.empty())
      cerr << endl;
#endif
   return (regs.size());
}

/* build a list with all registers written by micro-op ii part of decoded instruction dInst
 */
int
mach_inst_dest_registers(const DecodedInstruction *dInst, const instruction_info *ii, 
             RInfoList &regs, bool internal)
{
   assert (dInst->mach_data);
   const xed_decoded_inst_t* xedd = static_cast<xed_decoded_inst_t*>(dInst->mach_data);
   const xed_inst_t* xi = xed_decoded_inst_inst(xedd);

   uint8_t i;
   regs.clear();
   for (i=0 ; i<ii->num_dest_operands ; ++i)
   {
      int op_num = extract_op_index(ii->dest_opd[i]);
      switch (extract_op_type(ii->dest_opd[i]))
      {
         case OperandType_REGISTER:
            {
               const xed_operand_t* op = xed_inst_operand(xi, op_num);
               xed_operand_enum_t op_name = xed_operand_name(op);
               xed_reg_enum_t r = xed_decoded_inst_get_reg(xedd, op_name);
               
               if (r != XED_REG_INVALID) 
               {
                  RegisterClass rclass = RegisterClass_REG_OP;
                  xed_reg_class_enum_t xed_class = xed_reg_class(r);
                  stack_index_t stack = 0;
                  int rname = r;
                  if (xed_class == XED_REG_CLASS_PSEUDO) {
                     rclass = RegisterClass_PSEUDO;
                     if (r==XED_REG_X87PUSH || r==XED_REG_X87POP || r==XED_REG_X87POP2) {
                        rclass = RegisterClass_STACK_OPERATION;
                        stack = 1;
                     }
                  } else if (xed_class == XED_REG_CLASS_X87)
                  {
                     rclass = RegisterClass_STACK_REG;
                     rname = r - (int)XED_REG_ST0;  // store the offset of the register relative to top of stack
                     stack = 1;
                  }
#if DEBUG_REGISTER_EXTRACTION
                  cerr << "Dest REG= " << r << " - " << setw(3) << xed_reg_enum_t2str(r)
                       << "/" <<  setw(3) 
                       << xed_reg_class_enum_t2str(xed_reg_class(r)) << " ";
#endif
                  int lsb = dest_register_lsb_bit(xedd, op_num);
                  regs.push_back(register_info(rname, rclass, lsb, 
                        lsb+dest_register_killed_bits(xedd, op_num)-1, stack));
               }
            }
            break;

         case OperandType_INTERNAL:
            {
               if (internal)
               {
                  int rwidth = ii->width;
                  if (ii->exec_unit==ExecUnit_VECTOR)
                     rwidth *= ii->vec_len;
                  regs.push_back(register_info(op_num, RegisterClass_TEMP_REG, 0, rwidth-1));
               }
            }
            break;
      }
   }
#if DEBUG_REGISTER_EXTRACTION
   if (!regs.empty())
      cerr << endl;
#endif
   return (regs.size());
}

int 
mach_inst_delay_slot_length(const DecodedInstruction *dInst, const instruction_info *ii)
{
   // no instruction has delay slot on x86
   return (0);
}

/* return true if the register name is an instruction pointer
 * register.
 */
bool 
is_instruction_pointer_register(const register_info& reg)
{
   return (reg.type!=RegisterClass_TEMP_REG && reg.stack==0 && 
            XED_REG_IP_FIRST<=reg.name && reg.name<=XED_REG_IP_LAST);
}

/* return true if the register name is a stack pointer register.
 */
bool is_stack_pointer_register(const register_info& reg)
{
   return (reg.type!=RegisterClass_TEMP_REG && reg.stack==0 && 
          xed_get_largest_enclosing_register((xed_reg_enum_t)reg.name)==XED_REG_RSP);
}

/* return true if the register name is a flags register.
 */
bool 
is_flag_or_status_register(const register_info& reg)
{
   return (reg.type!=RegisterClass_TEMP_REG && reg.stack==0 && 
            ((XED_REG_FLAGS_FIRST<=reg.name && reg.name<=XED_REG_FLAGS_LAST) ||
             (XED_REG_MXCSR_FIRST<=reg.name && reg.name<=XED_REG_MXCSR_LAST))
          );
}

/* return true if the register is hardwired to a specific value.
 */
bool 
is_hardwired_register(const register_info& reg, const DecodedInstruction *dInst, long& value)
{
   // if this is an IP register, return the pc of the next instruction
   if (reg.type!=RegisterClass_TEMP_REG && reg.stack==0 && 
          XED_REG_IP_FIRST<=reg.name && reg.name<=XED_REG_IP_LAST)
   {
      value = dInst->pc + dInst->len;
      return (true);
   }
   // I think there are no other hardwired integer registers on x86.
   return (false);
}

addrtype 
mach_inst_branch_target(const DecodedInstruction *dInst)
{
   assert (dInst->mach_data);
   const xed_decoded_inst_t* xedd = static_cast<xed_decoded_inst_t*>(dInst->mach_data);
   xed_uint_t disp_bits = xed_decoded_inst_get_branch_displacement_width(xedd);
   xed_int32_t disp = 0;
   if (disp_bits) {
      disp = xed_decoded_inst_get_branch_displacement(xedd);
      disp += dInst->pc + dInst->len;
   }
   return (disp);
}

/* return true if this is a conditional move instruction. 
   These instructions have difficult to predict effects on
   the value of the destination register, since they can either
   modify it or not. For this reason, I may classify them as
   having an impossible effect on slicing. On the other hand,
   they are equivalent to conditional branches and in those cases
   I only slice back along one CFG edge.
 */
bool
mach_inst_performs_conditional_move(const DecodedInstruction *dInst, const instruction_info *ii)
{
   // I should check that this is the primary uop. The low level XED decoding makes
   // sense only for the primary uop;
   // I may invoke this function only with a DecodedInstruction;
   // The instruction_info object is there only to check if this is a primary uop;
   // Skip the primary check if the uop object is missing.
   if (ii && !ii->primary)
      return (false);

   assert (dInst->mach_data);
   const xed_decoded_inst_t* xedd = static_cast<xed_decoded_inst_t*>(dInst->mach_data);
   switch (xed_decoded_inst_get_iclass(xedd))
   {
      // conditional set immediate value
      case XED_ICLASS_SETB:               // BITBYTE
      case XED_ICLASS_SETBE:              // BITBYTE
      case XED_ICLASS_SETL:               // BITBYTE
      case XED_ICLASS_SETLE:              // BITBYTE
      case XED_ICLASS_SETNB:              // BITBYTE
      case XED_ICLASS_SETNBE:             // BITBYTE
      case XED_ICLASS_SETNL:              // BITBYTE
      case XED_ICLASS_SETNLE:             // BITBYTE
      case XED_ICLASS_SETNO:              // BITBYTE
      case XED_ICLASS_SETNP:              // BITBYTE
      case XED_ICLASS_SETNS:              // BITBYTE
      case XED_ICLASS_SETNZ:              // BITBYTE
      case XED_ICLASS_SETO:               // BITBYTE
      case XED_ICLASS_SETP:               // BITBYTE
      case XED_ICLASS_SETS:               // BITBYTE
      case XED_ICLASS_SETZ:               // BITBYTE

      // conditional move
      case XED_ICLASS_CMOVB:              // CMOV
      case XED_ICLASS_CMOVBE:             // CMOV
      case XED_ICLASS_CMOVL:              // CMOV
      case XED_ICLASS_CMOVLE:             // CMOV
      case XED_ICLASS_CMOVNB:             // CMOV
      case XED_ICLASS_CMOVNBE:            // CMOV
      case XED_ICLASS_CMOVNL:             // CMOV
      case XED_ICLASS_CMOVNLE:            // CMOV
      case XED_ICLASS_CMOVNO:             // CMOV
      case XED_ICLASS_CMOVNP:             // CMOV
      case XED_ICLASS_CMOVNS:             // CMOV
      case XED_ICLASS_CMOVNZ:             // CMOV
      case XED_ICLASS_CMOVO:              // CMOV
      case XED_ICLASS_CMOVP:              // CMOV
      case XED_ICLASS_CMOVS:              // CMOV
      case XED_ICLASS_CMOVZ:              // CMOV
         return (true);
    
      default:
         return (false);
   }
   return (false);
}

/* return true if this is a compare instruction. 
 */
bool
mach_inst_performs_compare(const instruction_info *ii)
{
   return (ii && ii->type==IB_cmp);
}

/* Return the canonical operation performed by this micro-op.
 * Support only a limited number of micro-ops for starter. Add more as deemed
 * necessary for register slicing.
 */
CanonicalOp 
uop_canonical_decode(const DecodedInstruction *dInst, const instruction_info *ii)
{
   if (!dInst || !ii)
      return (OP_INVALID);

   assert (dInst->mach_data);
   const xed_decoded_inst_t* xedd = static_cast<xed_decoded_inst_t*>(dInst->mach_data);
   
   // test first by the generic instruction type. Some generic types are a direct
   // correspondent of the native opcode: add, sub, xor, mult, div, ...
   CanonicalOp op = OP_INVALID;
   switch (ii->type)
   {
      case IB_add: 
         op = OP_ADD;
         break;
         
      case IB_sub: 
         op = OP_SUB;
         break;
         
      case IB_mult:
         op = OP_MUL;
         break;
         
      case IB_div:
         op = OP_DIV;
         break;
         
      case IB_xor:
         op = OP_XOR;
         break;
      
      case IB_lea:
         op = OP_LEA;
         break;
      
      // conditional moves may copy the source to the destination or not.
      // Assume the condition is true and the copy is executed. We should 
      // get a valid value (and answer) either way, since both are legal 
      // outcomes at runtime.
      // gmarin, 08/13/2012: This is a bit more tricky; I found cases where
      // a register was initialized with 0 if the sign bit was set (CMOVS),
      // that is, if some parameter value was negative. Assuming that the 
      // conditional move is always executed, made all the following 
      // calculations use the 0 value which was not the right behavior.
      // I better assume CMOVs are never executed. They are more used to
      // set a default value if some condition is true, so are more the 
      // exception than the rule.
      // I think I will have to deal with them by ignoring conditional moves
      // in the "ShouldSliceUop" method.
      case IB_move_cc:
         op = OP_MOV;
         break;

      // assume cary flag is zero, thus we have regular ADD and SUB operations
      case IB_add_cc:
      {
         if (ii->primary)
            switch (xed_decoded_inst_get_iclass(xedd))
            {
               case XED_ICLASS_ADC:           // add with carry
                  op = OP_ADD;
                  break;
                  
               case XED_ICLASS_SBB:           // sub with carry
                  op = OP_SUB;
                  break;

               default: 
                  break;
            }
         break;
      }
      
      case IB_copy:
         op = OP_MOV;
         break;
         
      case IB_move:  // right now, I classify many things as Move; should I be more specific?
      {
         if (ii->primary)
            switch (xed_decoded_inst_get_iclass(xedd))
            {
               case XED_ICLASS_VMOVNTDQ:
               case XED_ICLASS_VMOVNTDQA:
               case XED_ICLASS_VMOVNTPD:
               case XED_ICLASS_VMOVNTPS:
               case XED_ICLASS_MOVNTDQA:
               
               case XED_ICLASS_VPMOVSXBD:
               case XED_ICLASS_VPMOVSXBQ:
               case XED_ICLASS_VPMOVSXBW:
               case XED_ICLASS_VPMOVSXDQ:
               case XED_ICLASS_VPMOVSXWD:
               case XED_ICLASS_VPMOVSXWQ:
               case XED_ICLASS_VPMOVZXBD:
               case XED_ICLASS_VPMOVZXBQ:
               case XED_ICLASS_VPMOVZXBW:
               case XED_ICLASS_VPMOVZXDQ:
               case XED_ICLASS_VPMOVZXWD:
               case XED_ICLASS_VPMOVZXWQ:
               case XED_ICLASS_PMOVSXBD:
               case XED_ICLASS_PMOVSXBQ:
               case XED_ICLASS_PMOVSXBW:
               case XED_ICLASS_PMOVSXDQ:
               case XED_ICLASS_PMOVSXWD:
               case XED_ICLASS_PMOVSXWQ:
               case XED_ICLASS_PMOVZXBD:
               case XED_ICLASS_PMOVZXBQ:
               case XED_ICLASS_PMOVZXBW:
               case XED_ICLASS_PMOVZXDQ:
               case XED_ICLASS_PMOVZXWD:
               case XED_ICLASS_PMOVZXWQ:
               
               case XED_ICLASS_MOV:
               case XED_ICLASS_MOVAPD:
               case XED_ICLASS_MOVAPS:
               case XED_ICLASS_MOVDQ2Q:
               case XED_ICLASS_MOVDQA:
               case XED_ICLASS_MOVDQU:
               case XED_ICLASS_MOVNTDQ:
               case XED_ICLASS_MOVNTI:
               case XED_ICLASS_MOVNTPD:
               case XED_ICLASS_MOVNTPS:
               case XED_ICLASS_MOVNTQ:
               case XED_ICLASS_MOVNTSD:
               case XED_ICLASS_MOVNTSS:
               case XED_ICLASS_MOVQ2DQ:
               case XED_ICLASS_MOVSD_XMM:
               case XED_ICLASS_MOVSS:
               case XED_ICLASS_MOVSX:
               case XED_ICLASS_MOVSXD:
               case XED_ICLASS_MOVUPD:
               case XED_ICLASS_MOVUPS:
               case XED_ICLASS_MOVZX:
               case XED_ICLASS_VMOVAPD:
               case XED_ICLASS_VMOVAPS:
               case XED_ICLASS_VMOVD:
               case XED_ICLASS_VMOVDQA:
               case XED_ICLASS_VMOVDQU:
               case XED_ICLASS_VMOVQ:
               case XED_ICLASS_VMOVSD:
               case XED_ICLASS_VMOVSS:
               case XED_ICLASS_VMOVUPD:
               case XED_ICLASS_VMOVUPS:
               case XED_ICLASS_MOVD:
               case XED_ICLASS_MOVQ:
               
               // ENTER and LEAVE copy the base pointer to the stack pointer and
               // a few other operations as well; the register copy is clasified as
               // the MOV and is the primary uop
               case XED_ICLASS_ENTER:
               case XED_ICLASS_LEAVE:
               
               case XED_ICLASS_MOVSB:
               case XED_ICLASS_MOVSD:
               case XED_ICLASS_MOVSQ:
               case XED_ICLASS_MOVSW:
                  op = OP_MOV;
                  break;
                  
               default: 
                  break;
            }
         break;
      }

      case IB_cmp: 
      {
         if (ii->primary)
            switch (xed_decoded_inst_get_iclass(xedd))
            {
               case XED_ICLASS_CMP:                // BINARY
                  op = OP_CMP;
                  break;

               default: 
                  break;
            }
         break;
      }
         
         
      case IB_logical:
      {
         if (ii->primary)
            switch (xed_decoded_inst_get_iclass(xedd))
            {
               case XED_ICLASS_AND:
               case XED_ICLASS_ANDPD:
               case XED_ICLASS_ANDPS:
               case XED_ICLASS_VANDPD:
               case XED_ICLASS_VANDPS:
               case XED_ICLASS_VPAND:
               case XED_ICLASS_PAND:
                  op = OP_AND;
                  break;
                  
               case XED_ICLASS_ANDNPD:
               case XED_ICLASS_ANDNPS:
               case XED_ICLASS_VANDNPD:
               case XED_ICLASS_VANDNPS:
               case XED_ICLASS_VPANDN:
               case XED_ICLASS_PANDN:
                  op = OP_ANDN;
                  break;
               
               case XED_ICLASS_NOT:
                  op = OP_NOT;
                  break;
               
               case XED_ICLASS_OR:
               case XED_ICLASS_ORPD:
               case XED_ICLASS_ORPS:
               case XED_ICLASS_VORPD:
               case XED_ICLASS_VORPS:
               case XED_ICLASS_VPOR:
               case XED_ICLASS_POR:
                  op = OP_OR;
                  break;
                  
               default: 
                  break;
            }
         break;
      }
      
      case IB_shift:
      {
         if (ii->primary)
            switch (xed_decoded_inst_get_iclass(xedd))
            {
               case XED_ICLASS_VPSLLD:
               case XED_ICLASS_VPSLLDQ:
               case XED_ICLASS_VPSLLQ:
               case XED_ICLASS_VPSLLW:
               case XED_ICLASS_PSLLD:
               case XED_ICLASS_PSLLQ:
               case XED_ICLASS_PSLLW:
               case XED_ICLASS_PSLLDQ:
               case XED_ICLASS_SHL:
               // SHLD has 3 source operands, shifts from one register to another
               // I cannot compute its effects unless its second operand is constant
               case XED_ICLASS_SHLD:
                  op = OP_SLL;
                  break;
                  
               case XED_ICLASS_VPSRAD:
               case XED_ICLASS_VPSRAW:
               case XED_ICLASS_PSRAD:
               case XED_ICLASS_PSRAW:
               case XED_ICLASS_SAR:
                  op = OP_SRA;
                  break;
                  
               case XED_ICLASS_VPSRLD:
               case XED_ICLASS_VPSRLDQ:
               case XED_ICLASS_VPSRLQ:
               case XED_ICLASS_VPSRLW:
               case XED_ICLASS_PSRLD:
               case XED_ICLASS_PSRLQ:
               case XED_ICLASS_PSRLW:
               case XED_ICLASS_PSRLDQ:
               case XED_ICLASS_SHR:
               // SHRD has 3 source operands, shifts from one register to another
               // I cannot compute its effects unless all its operands are constant.
               // Strange instruction for computing memory addresses. Do not consider it.
//               case XED_ICLASS_SHRD:
                  op = OP_SRL;
                  break;
                  
               default: 
                  break;
            }
         break;
      }

      case IB_cvt_prec:
      {
         if (ii->primary)
            switch (xed_decoded_inst_get_iclass(xedd))
            {
               case XED_ICLASS_CBW:                // CONVERT
               case XED_ICLASS_CDQE:               // CONVERT
               case XED_ICLASS_CWDE:               // CONVERT
                  op = OP_MOV;
                  break;

               default: 
                  break;
            }
         break;
      }
      
      case IB_cvt:
      {
         if (ii->primary)
            switch (xed_decoded_inst_get_iclass(xedd))
            {
               case XED_ICLASS_CVTSD2SI:           // CONVERT
               case XED_ICLASS_CVTTSD2SI:          // CONVERT
                  op = OP_MOV;
                  break;

               default: 
                  break;
            }
         break;
      }
      
      default: 
         break;
   }
   
   return (op);
}

#if WITH_REFERENCE_SLICING
int 
generic_formula_for_memory_operand(const DecodedInstruction *dInst, int uop_idx, int op_num, 
             addrtype pc, GFSliceVal& formula)
{
    assert (dInst->mach_data);
    const xed_decoded_inst_t* xedd = static_cast<xed_decoded_inst_t*>(dInst->mach_data);

    formula = GFSliceVal(sliceVal(0, TY_CONSTANT));
    
    RegisterClass rclass = RegisterClass_MEM_OP;
    if ( !xed_decoded_inst_mem_read(xedd,op_num) &&
         !xed_decoded_inst_mem_written(xedd,op_num))
       rclass = RegisterClass_LEA_OP;
    int addr_width = xed_decoded_inst_get_memop_address_width(xedd,op_num);
    
    xed_reg_enum_t r = xed_decoded_inst_get_seg_reg(xedd, op_num);
    if (r != XED_REG_INVALID) 
    {
#if DEBUG_REGISTER_EXTRACTION
       cerr << "SEG= " << xed_reg_enum_t2str(r) << " (ignored) ";
#endif
       // I am not sure what to do about segment registers
       // I think it is safe to ignore them (assume they are 0) in both
       // 32-bit and 64-bit modes.
//          register_info(r, rclass, 0, addr_width-1);
       // Probably, I should not ignore FS and GS segment registers. I hear that they are still used
       if (r==XED_REG_FS || r==XED_REG_GS)
       {
          formula += sliceVal(1, TY_REGISTER, pc, uop_idx, register_info(r, rclass, 0, addr_width-1));
       }
    }
    r = xed_decoded_inst_get_base_reg(xedd, op_num);
    if (r != XED_REG_INVALID) 
    {
#if DEBUG_REGISTER_EXTRACTION
       cerr << "BASE= " << setw(3) << xed_reg_enum_t2str(r) << "/" 
            << setw(3)
            <<  xed_reg_class_enum_t2str(xed_reg_class(r)) << " "; 
#endif
       formula += sliceVal(1, TY_REGISTER, pc, uop_idx, register_info(r, rclass, 0, addr_width-1));
    }
    r = xed_decoded_inst_get_index_reg(xedd, op_num);
    if (op_num==0 && r!=XED_REG_INVALID)
    // taken from xed-ex1.cpp; only the first mem operand can have an index register??
    {
#if DEBUG_REGISTER_EXTRACTION
       cerr << "INDEX= " << setw(3) << xed_reg_enum_t2str(r)
            << "/" <<  setw(3) 
            << xed_reg_class_enum_t2str(xed_reg_class(r)) << " ";
#endif
       uint32_t scale = xed_decoded_inst_get_scale(xedd,op_num);
       if (scale == 0)
          scale = 1;
       formula += sliceVal(scale, TY_REGISTER, pc, uop_idx, register_info(r, rclass, 0, addr_width-1));
    }
    xed_uint_t disp_bits = xed_decoded_inst_get_memory_displacement_width(xedd,op_num);
    if (disp_bits) {
       xed_int64_t disp = xed_decoded_inst_get_memory_displacement(xedd,op_num);
       formula += sliceVal(disp, TY_CONSTANT);
    }
#if DEBUG_REGISTER_EXTRACTION
    cerr << endl;
#endif
    return (0);
}

int 
generic_formula_for_branch_target(const DecodedInstruction *dInst, const instruction_info *ii, 
                int uop_idx, addrtype pc, GFSliceVal& formula)
{
   if (! InstrBinIsBranchType(ii->type))
   {
      return (-1);  // not a control transfer instruction
   }
   assert (dInst->mach_data);
   const xed_decoded_inst_t* xedd = static_cast<xed_decoded_inst_t*>(dInst->mach_data);
   const xed_inst_t* xi = xed_decoded_inst_inst(xedd);

   formula = GFSliceVal(sliceVal(0, TY_CONSTANT));
   
   // Iterate over all the source operands and try to compute the target.
   // I must consider all the possible cases
   int i;
   for (i=0 ; i<ii->num_src_operands ; ++i)
   {
      int op_num = extract_op_index(ii->src_opd[i]);
      int otype = extract_op_type(ii->src_opd[i]);
      // operand cannot be of type memory in my decoding
      // Only memory micro-ops can have memory operands, and this 
      // is a control transfer micro-op
      assert (otype!=OperandType_MEMORY);
      switch (otype)
      {
         case OperandType_REGISTER:
            {
               const xed_operand_t* op = xed_inst_operand(xi, op_num);
               xed_operand_enum_t op_name = xed_operand_name(op);
               xed_reg_enum_t r = xed_decoded_inst_get_reg(xedd, op_name);

               // do not consider invalid register, or flag/status registers
               if (r!=XED_REG_INVALID &&
                     !((XED_REG_FLAGS_FIRST<=r && r<=XED_REG_FLAGS_LAST) ||
                       (XED_REG_MXCSR_FIRST<=r && r<=XED_REG_MXCSR_LAST))
                  )

               {
#if DEBUG_REGISTER_EXTRACTION
                  cerr << "Branch at pc=0x" << hex << dInst->pc << dec << " target reg= " 
                       << setw(3) << xed_reg_enum_t2str(r) << "/" << setw(3) 
                       << xed_reg_class_enum_t2str(xed_reg_class(r)) << endl; 
#endif
                  RegisterClass rclass = RegisterClass_REG_OP;
                  xed_reg_class_enum_t xed_class = xed_reg_class(r);
                  stack_index_t stack = 0;
                  int rname = register_name_actual(r);
                  if (xed_class == XED_REG_CLASS_PSEUDO) {
                     rclass = RegisterClass_PSEUDO;
                     if (r==XED_REG_X87PUSH || r==XED_REG_X87POP || r==XED_REG_X87POP2) {
                        rclass = RegisterClass_STACK_OPERATION;
                        stack = 1;
                     }
                  } else if (xed_class == XED_REG_CLASS_X87)
                  {
                     rclass = RegisterClass_STACK_REG;
                     rname = r - (int)XED_REG_ST0;  // store the offset of the register relative to top of stack
                     stack = 1;
                  }
                  
                  int lsb = src_register_lsb_bit(xedd, op_num);
                  formula += sliceVal(1, TY_REGISTER, pc, uop_idx, 
                                register_info(rname, rclass, lsb,
                                lsb+xed_decoded_inst_operand_length_bits(xedd,op_num)-1, 
                                stack));
               }
            }
            break;

         case OperandType_INTERNAL:
            {
               int rwidth = ii->width;
               if (ii->exec_unit==ExecUnit_VECTOR)
                  rwidth *= ii->vec_len;
               formula += sliceVal(1, TY_REGISTER, pc, uop_idx, 
                          register_info(op_num, RegisterClass_TEMP_REG, 0, rwidth-1));
            }
            break;

         case OperandType_IMMED:
            {
               formula += sliceVal(ii->imm_values[op_num].value.s, TY_CONSTANT);
            }
            break;
      }
   }
   return (0);
}
#endif

/* Temporary hack to determine if a register can have only part of its bits modified.
 * I think that XMM/YMM registers can have lower and upper parts modified independently.
 * However, I've not seen any clear proof that this is the case for other general purpose
 * registers on x86. For example, if you write to EAX, it should reset the high 32-bits of
 * the full 64-bit register (RAX) to 0, not keep them unmodified. 
 * I will assume that this is the case until proven wrong.
 * Parameters: Pass the ranges of both the new and the previous register definitions
 */
bool can_have_unmodified_register_range(const register_info& rnew, const register_info& rold)
{
   if (rnew.stack==0 && rnew.type!=RegisterClass_TEMP_REG)
   {
      xed_reg_class_enum_t rclass = xed_reg_class((xed_reg_enum_t)rnew.name);
      if (rclass==XED_REG_CLASS_XMM || rclass==XED_REG_CLASS_YMM)
         return (true);
   }
   return (false);
}

} /* namespace MIAMI */
