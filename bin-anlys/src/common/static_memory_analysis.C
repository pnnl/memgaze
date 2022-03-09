/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: static_memory_analysis.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Computes generic formulas for memory references' addresses
 * and strides, using data flow analysis.
 */

#include <stdio.h>
#include <assert.h>
#include "math_routines.h"

#include "slice_references.h"
#include "static_memory_analysis.h"
#include "InstructionDecoder.hpp"
#include "debug_miami.h"

namespace MIAMI
{

#define DEBUG_REFERENCE_SLICE 0
#define TRACE_REFERENCE_SLICE 0

GFSliceVal* defaultGFSvP = 0;

StrideInst* defaultStrideInstP = 0;
unsigned long StrideSlice::strideDFSCount = 0;
unsigned long StrideSlice::sliceKey = 0;


#ifndef min
#define min(x,y)  ((x)<(y)?(x):(y))
#endif

/* build a GFSliceVal formula for the specified operand of a micro-op.
 */
static GFSliceVal
MakeFormulaForOperand(const DecodedInstruction *dinst, 
             const instruction_info *ii, int uop_idx, int op_num)
{
   OperandType optype = (OperandType)extract_op_type(ii->src_opd[op_num]);
   int opidx = extract_op_index(ii->src_opd[op_num]);
   GFSliceVal form;
   if (optype == OperandType_INTERNAL)
   {
      int rwidth = ii->width;
      if (ii->exec_unit==ExecUnit_VECTOR)
         rwidth *= ii->vec_len;
      register_info reg1(opidx, RegisterClass_TEMP_REG, 0, rwidth-1);
      form = GFSliceVal(sliceVal(1, TY_REGISTER, dinst->pc, uop_idx, reg1));
   } else if (optype == OperandType_REGISTER)
   {
      register_info reg1 = mach_inst_reg_operand(dinst, opidx);
      reg1.name = register_name_actual(reg1.name);
      form = GFSliceVal(sliceVal(1, TY_REGISTER, dinst->pc, uop_idx, reg1));
   } else
   {
      assert (optype == OperandType_IMMED);
      form = GFSliceVal(sliceVal(ii->imm_values[opidx].value.s, TY_CONSTANT));
   }
   return (form);
}


static GFSliceVal 
processReferenceSliceOperation (const register_info& reg, addrtype pc, int uop_idx,
         GFSliceVal &formula1, GFSliceVal &formula2, CanonicalOp op)
{
   GFSliceVal cachedFormula;

   coeff_t valueNum1, valueNum2;
   ucoeff_t valueDen1, valueDen2;
   bool rf1IsConst = IsConstantFormula(formula1, valueNum1, valueDen1);
   bool rf2IsConst = IsConstantFormula(formula2, valueNum2, valueDen2);

   valueNum1 /= valueDen1;
   valueNum2 /= valueDen2;
   
   // if we have a shift operation and the second operand is a negative constant
   // something must went wrong. The slicing likely encountered a conditional move 
   // instruction or such where we considered the condition as false, thus not 
   // handling the exceptional case.
   // We just mark the operand as non-constant here.
   if (rf2IsConst && valueNum2<0 && 
           (op==OP_SLL || op==OP_SRL || op==OP_SRA))
      rf2IsConst = false;

   // work only with integer, so use int arithmetic
   if (rf1IsConst && rf2IsConst)
   {
      if (op == OP_OR)
      {
         cachedFormula = GFSliceVal(sliceVal(valueNum1|valueNum2, 
                 TY_CONSTANT));
      } else
      if (op == OP_ORN)
      {
          // I do not think we have ORN on x86, do we?
         cachedFormula = GFSliceVal(sliceVal(valueNum1|(~valueNum2), 
                  TY_CONSTANT));
      } else
      if (op == OP_ANDN)
      {
         cachedFormula = GFSliceVal(sliceVal(valueNum1&(~valueNum2),
                   TY_CONSTANT));
      } else
      if (op == OP_AND)
      {
         cachedFormula = GFSliceVal(sliceVal(valueNum1&valueNum2, 
                    TY_CONSTANT));
      } else
      if (op == OP_XOR)
      {
         cachedFormula = GFSliceVal(sliceVal(valueNum1^valueNum2, 
                    TY_CONSTANT));
      } else
      if (op == OP_XNOR)
      {
         // haven't seen any XNOR on x86, amirite?
         cachedFormula =  GFSliceVal(sliceVal(~(valueNum1^valueNum2),
                     TY_CONSTANT));
      }
   } else
   if (rf2IsConst)
   {
      /* in the new version of the tool I keep track of bit ranges
       * for registers (more or less).
       * Should I shift bit ranges on SLL and SRL operations or
       * execute the multiplication and division respectively?
       * Probably I have to add shift operators to Formulas.
       */
      if (op == OP_SLL)
      {
         assert(valueNum2>=0);
         cachedFormula = formula1;
         cachedFormula *= (1<<valueNum2);
      } else
      if (op == OP_SRL || op == OP_SRA)
      {
         if (valueNum2<0)
         {
            cerr << "Error in SRL/SRA at pc=0x" << hex << pc << dec 
                 << ", oper1=" << formula1 << ", oper2="
                 << formula2 << endl;
            assert(valueNum2>=0);
         }
         cachedFormula = formula1;
         cachedFormula /= (1<<valueNum2);
         if (FormulaHasComplexDenominator(cachedFormula))
            cachedFormula = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
      } else
      if ((op == OP_OR || op == OP_ANDN) && valueNum2==0)
      {
         cachedFormula = formula1;
      } else
      if (op == OP_MUL)
      {
         cachedFormula = formula1;
         cachedFormula *= (valueNum2);
      } else
      if (op == OP_DIV)
      {
         cachedFormula = formula1;
         if (valueNum2<0)
            cachedFormula *= (-1);
         cachedFormula /= abs(valueNum2);
         if (FormulaHasComplexDenominator (cachedFormula))
            cachedFormula = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
      }
   } else
   if (rf1IsConst)
   {
      if (op == OP_OR && valueNum1==0)
      {
         cachedFormula = formula2;
      } else
      if (op == OP_MUL)
      {
         cachedFormula = formula2;
         cachedFormula *= (valueNum1);
      }
   }


   // in any other case it is an unknown computation
   if (cachedFormula.is_uninitialized())
   {
      cachedFormula = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
   }
   return (cachedFormula);
}


bool 
ReferenceSlice::SliceIn(PrivateCFG::Edge *e, const register_info& reg)
{
   // Follow only forward edges, no loop back edges
   PrivateCFG::Node* bp = e->source();
   return (!e->isLoopBackEdge() && bp->getEndAddress()>to_pc);
}

void 
ReferenceSlice::SliceOut(PrivateCFG::Edge *e, const register_info& reg)
{
}

// called at the beginning of SliceNext, check if we reached a stopping condition
bool 
ReferenceSlice::SliceNextIn(PrivateCFG::Node *b, const register_info& reg)
{
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(3,
      cerr << endl << "  --- RefSlice::SliceNextIn, slice block [0x" << hex << b->getStartAddress()
           << ",0x" << b->getEndAddress() << dec << "], reg=" << reg << endl;
   )
#endif
   if (b->getStartAddress()<=to_pc)
   {
      _values.push_back (new RegFormula (reg, 
           GFSliceVal (sliceVal (1, TY_REGISTER, to_pc, 0, reg)) ));
      return (false);
   }
   if (b->isCfgEntry())   // we are the start of the routine
   {
      _values.push_back (new RegFormula (reg, 
           GFSliceVal (sliceVal (1, TY_REGISTER, b->getStartAddress(), 0, reg)) ));
      return (false);
   }
   return (true);
}

void 
ReferenceSlice::SliceNextOut(PrivateCFG::Node *b, const register_info& reg)
{
   // SliceNextOut is called if I did not find any valid incoming edge
   // setup a register term
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(3,
      cerr << endl << "  === RefSlice::SliceNextOut, slice block [0x" << hex << b->getStartAddress()
           << ",0x" << b->getEndAddress() << dec << "], reg=" << reg << endl;
   )
#endif
   _values.push_back (new RegFormula (reg, 
         GFSliceVal (sliceVal (1, TY_REGISTER, b->getStartAddress(), 0, reg)) ));
}

// save the constant value of a register at a given PC
void 
ReferenceSlice::RecordRegisterValue(const register_info& reg, const uOp_t& uop, coeff_t value)
{
   _values.push_back (new RegFormula (reg, 
         GFSliceVal (sliceVal (value, TY_CONSTANT)) ));
}

/* build a RegFormula for the specified operand of a micro-op.
 */
RegFormula*
ReferenceSlice::RegFormulaForOperand(const DecodedInstruction *dinst, 
             const instruction_info *ii, int op_num)
{
   OperandType optype = (OperandType)extract_op_type(ii->src_opd[op_num]);
   int opidx = extract_op_index(ii->src_opd[op_num]);
   RegFormula *rf = 0;
   if (optype == OperandType_INTERNAL)
   {
      int rwidth = ii->width;
      if (ii->exec_unit==ExecUnit_VECTOR)
         rwidth *= ii->vec_len;
      register_info reg1(opidx, RegisterClass_TEMP_REG, 0, rwidth-1);
      rf = getValueForRegAndDelete(reg1);
   } else if (optype == OperandType_REGISTER)
   {
      register_info reg1 = mach_inst_reg_operand(dinst, opidx);
      reg1.name = register_name_actual(reg1.name);
      rf = getValueForRegAndDelete(reg1);
   } else
   {
      assert (optype == OperandType_IMMED);
      rf = new RegFormula(register_info(), 
               GFSliceVal(sliceVal(ii->imm_values[opidx].value.s, TY_CONSTANT)));
   }
   return (rf);
}


// Slice back until encountering a function call, a load operation, or start
// of function;
// another stopping condition can be a unsupported instruction;
// in general, go back along add, sub, mov, shift, sethi, 
// (or, and, not, xor - for constant values only);
// When slicing back, several paths may exist. Avoid backedges in order to
// avoid an infinite loop. Stop at the first value.
//
bool 
ReferenceSlice::SliceIn(PrivateCFG::Node *b, int uop_idx, const register_info& reg)
{
   UopCodeCache *ucc = b->MicroOpCode();
   int num_uops = ucc->NumberOfUops();
   // I should always receive a valid uop index
   assert(uop_idx>=0 && uop_idx<num_uops);
   const uOp_t& uop = ucc->UopAtIndex(uop_idx);
   addrtype pc = uop.dinst->pc;
   
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(4,
      cerr << "  - - > RefSlice::slice_in, reg=" << reg << ", pc=0x" << hex << pc 
           << ", to_pc=0x" << to_pc << ", start_pc=0x" << start_pc << dec << endl;
   )
#endif
   if (pc<to_pc)
      return (false);   // stop slicing
      
   // check if we have an entry in cache for this micro-op
   if (cacheValues.hasKey(pc, uop.idx))
      return (false);  // stop slicing if we have the value of the register
                      // defined at this address in cache

   switch (SliceEffect(b, uop, reg))
   {
      case SliceEffect_EASY:
      {
         return (false);    // stop slicing
      }

      case SliceEffect_NORMAL:
      {
         // continue back-slicing on each input register only if this is not 
         // an INVALID op. If INVALID op, print a warning and generate a 
         // REGISTER formula.
         CanonicalOp op = uop_canonical_decode(uop.dinst, uop.iinfo);
         if (op == OP_INVALID)
         {
            // an operation that I do not consider yet.
            // Print a BIG warning
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(1,
               addrtype reloc = b->RelocationOffset();
               cerr << "Yo, ReferenceSlice::SliceIn, found INVALID op code at pc 0x"
                    << hex << pc << dec << " idx=" << uop.idx << ". CHECK THIS OUT." << endl;
               debug_decode_instruction_at_pc((void*)(pc+reloc), uop.dinst->len);
            )
#endif
            // I can create a register formula right here
//            _values.push_back(new RegFormula(reg, 
//                      GFSliceVal(sliceVal (1, TY_REGISTER, pc, reg))));
            return (false);
         } else
            return (true);
      }
         
      case SliceEffect_HARD:
         // this must be a Load;
         {
            // create a local block for any temporal variable
            int opidx = uop.iinfo->first_src_operand_of_type(OperandType_MEMORY);
            assert(opidx >= 0);  // opidx should be positive is this is a Load
            RefFormulas *rf = refFormulas(pc, opidx);
            coeff_t loadNum;
            ucoeff_t loadDen;
            bool is_const = false;
         
            // if I have a formula computed for this load, try finding a
            // preceeding store to the same location and continue slicing 
            // from there
            if (rf && !rf->base.is_uninitialized() && 
                  (!(is_const=IsConstantFormula(rf->base, loadNum, loadDen)) || loadNum!=0))
            {
               // Load from some computed formula. Search for a reaching 
               // store at same address (same formula)
               // and continue slicing from there.
               //
               register_info spilled_reg;
               start_pc = pc;
               if (SliceFollowSave(b, uop_idx, rf->base, spilled_reg))
               {
#if DEBUG_STATIC_MEMORY
                  DEBUG_STMEM(5,
                     cerr << "ReferenceSlice::SliceIn, returned from SliceFollowSave, reg=" 
                          << reg << ", pc=0x" << hex << pc << dec << ", SpilledREG=" 
                          << spilled_reg << " :: ";
                  )
#endif
                  if (spilled_reg==MIAMI_NO_REG)
                  {
                     assert(!"we should not be here. FollowSave should return false instead.");
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(5,
                        cerr << "found NO_REG" << endl;
                     )
#endif
                     _values.push_back(new RegFormula(pc, uop.idx, GFSliceVal(
                            sliceVal(1, TY_LOAD, pc, uop_idx, reg))));
                  }
                  else
                  {
                     RegFormula* rftemp = getValueForRegAndDelete(spilled_reg);
                     if (rftemp==NULL || 
                          FormulaDefinedBeforePC(rftemp->formula, to_pc))
                     {
#if DEBUG_STATIC_MEMORY
                        DEBUG_STMEM(5,
                           cerr << "found reg=" << spilled_reg << "; formula NULL or defined before pc" << endl;
                        )
#endif
                        _values.push_back(new RegFormula(pc, uop.idx, GFSliceVal(
                               sliceVal(1, TY_LOAD, pc, uop_idx, reg))));
                        if (rftemp)
                           delete rftemp;
                     }
                     else
                     {
#if DEBUG_STATIC_MEMORY
                        DEBUG_STMEM(5,
                           cerr << "found reg=" << spilled_reg << "; formula for reg " 
                                << rftemp->reg << " is " << rftemp->formula << endl;
                        )
#endif
                        rftemp->reg = register_info();
                        rftemp->pc = pc;
                        rftemp->idx = uop.idx;
                        _values.push_back(rftemp);
                     }
                  }
                  return (false);
               } else
               {
                  // follow_save did not find a corresponding store
                  // if base formula is constant, create a REFERENCE formula instead
                  if (is_const)
                     _values.push_back(new RegFormula(pc, uop.idx, GFSliceVal(
                            sliceVal (1, TY_REFERENCE, pc, uop_idx, loadNum))));
                  else
                     _values.push_back(new RegFormula(pc, uop.idx, GFSliceVal(
                            sliceVal(1, TY_LOAD, pc, uop_idx, reg))));
               }
            } else  // found a load whose address is not computed or is zero??
            {
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(5,
                  cerr << "ReferenceSlice::SliceIn on reg=" << reg << " found load without a valid base " 
                       << "formula. Cannot execute SliceFollowSave for this case." << endl;
               )
#endif
               _values.push_back(new RegFormula(pc, uop.idx, GFSliceVal(
                           sliceVal(1, TY_LOAD, pc, uop_idx, reg))));
            }
            // stop slicing
            return (false);
         }

      case SliceEffect_IMPOSSIBLE:
         // stop slicing at calls. We do not perform inter-procedural slicing
         return (false);

      default:
         assert(!"Bad case");
         return (false);
   }
}

void 
ReferenceSlice::SliceOut(PrivateCFG::Node *b, int uop_idx, const register_info& reg)
{
   UopCodeCache *ucc = b->MicroOpCode();
   int num_uops = ucc->NumberOfUops();
   // I should always receive a valid uop index
   assert(uop_idx>=0 && uop_idx<num_uops);
   const uOp_t& uop = ucc->UopAtIndex(uop_idx);
   uOp_t& uop_ozgur = ucc->UopAtIndex_Ozgur(uop_idx);
   DecodedInstruction *dinst_ozgur = uop_ozgur.dinst;
   addrtype pc = uop.dinst->pc;
   
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(4,
      cerr << "  < - - RefSlice::slice_out, reg=" << reg << ", pc=0x" << hex << pc 
           << ", to_pc=0x" << to_pc << ", start_pc=0x" << start_pc << dec << endl;
   )
#endif
   if (pc<to_pc)
   {
      _values.push_back(new RegFormula(reg, 
           GFSliceVal(sliceVal(1, TY_REGISTER, to_pc, -1, reg)) ));
      return;   // stop slicing
   }
   
   // check if we have a cached value already
   GFSliceVal& cachedFormula = cacheValues(pc, uop.idx);
   if (! cachedFormula.is_uninitialized())
   {
#if DEBUG_STATIC_MEMORY
      DEBUG_STMEM(5,
         cerr << "RefSliceOut: pc=0x" << hex << pc << dec << ", reg=" << reg
              << ", found cached formula=" << cachedFormula << endl;
      )
#endif
      _values.push_back(new RegFormula(reg, cachedFormula));
      return;  // stop slicing if we have the value of the register
               // defined at this address in cache
   }

   // create some shortcut variables
   const instruction_info *ii = uop.iinfo;
   const DecodedInstruction *dinst = uop.dinst;

   switch (SliceEffect(b, uop, reg))
   {
      case SliceEffect_EASY:
          // *** I need to start implementing the instruction semantic
          // *** First get the immediate operands from XED
         // some of the instructions that are possible, are: Move, Jump, Branch, PriviledgedOp, NOP
         // Only Move is a plausible operation while slicing address registers
         if (ii->type==IB_move || ii->type==IB_copy)
         {
            assert (ii->num_imm_values == 1);
            cachedFormula = GFSliceVal(sliceVal(ii->imm_values[0].value.s, TY_CONSTANT));
            _values.push_back(new RegFormula(reg, cachedFormula));
         } else
         {
            fprintf (stderr, "ALERT, Micro-op %d at pc 0x%" PRIxaddr " with slice effect EASY on reg %s, has type %d which is not MOVE. Check it out!\n",
                    uop.idx, pc, reg.ToString().c_str(), ii->type);
         }
         break;

      case SliceEffect_NORMAL:
         {
            int num_src = ii->num_src_operands;
            assert (num_src > 0);
            // do not include flag and status source registers;
            // Iterate over source operands, starting at the last one, and skip over
            // all flag/status register operands
            while (num_src>0 && 
                   (OperandType)extract_op_type(ii->src_opd[num_src-1])==OperandType_REGISTER)
            {
               int opidx = extract_op_index(ii->src_opd[num_src-1]);
               register_info reg1 = mach_inst_reg_operand(dinst, opidx);
               if (!is_flag_or_status_register(reg1))
                  break;
               else
                  num_src -= 1;
            }
            
            CanonicalOp op = uop_canonical_decode(dinst, ii);
//            if ((ii->type!=IB_div && ii->type!=IB_shift && ii->type!=IB_cmp_xchg && 
//                   ii->type!=IB_cmp && ii->type!=IB_insert && num_src>2) || num_src>3)
            // tired of adding op-types for all kinds of obscure operations that I cannot
            // handle anyway. We do not care if an unhandled (Invalid) opcode has more
            // than two source operands.
            if ((ii->type!=IB_div && op!=OP_INVALID && num_src>2) || num_src>3)
            {
               addrtype reloc = b->RelocationOffset();
               fprintf(stderr, "ALERT, Found micro-op %d at pc 0x%" PRIxaddr " of type %s with slice effect NORMAL on reg %s, that has %d source operands. Check it out!\n",
                    uop.idx, pc, Convert_InstrBin_to_string(ii->type), 
                    reg.ToString().c_str(), num_src);
               debug_decode_instruction_at_pc((void*)(pc+reloc), uop.dinst->len);
               DumpInstrList(uop.dinst);
               assert(! "Found micro-op with more than expected source operands.");
            }
            // check the type of the first two operands
            RegFormula *rf1 = 0, *rf2 = 0, *rf1b = 0;
            // I already asserted that num_src > 0; I should have at least one source operand
            // no source operand should be of MEMORY type; MEM goes to the HARD case
            OperandType opt1 = (OperandType)extract_op_type(ii->src_opd[0]);
            int opidx1 = extract_op_index(ii->src_opd[0]);
            assert (opt1!=OperandType_MEMORY || ii->type==IB_lea);
            
            if (ii->type == IB_lea)
            {
               assert (num_src == 1);  // only a memory operand
               // TODO: compute effective address
               GFSliceVal _formula (sliceVal (0, TY_CONSTANT));
               cachedFormula = GFSliceVal(sliceVal(0, TY_CONSTANT));
               //OZGUR Calling my function to update dInst with scale and disp
               calculate_disp_and_scale(uop_ozgur.dinst,opidx1,pc);
               if (!generic_formula_for_memory_operand(dinst, uop_idx, opidx1, pc, _formula))
               {
                  // iterate over the terms of the formula and find formulas for all registers
                  GFSliceVal::iterator fit;
                  for (fit=_formula.begin() ; fit!=_formula.end() ; ++fit)
                  {
                     if (fit->TermType() == TY_REGISTER)
                     {
                        const register_info& areg = fit->Register();
                        // I take care of hardwired registers before SliceIn
                        // I should find a value set even for hardwired regs
                        RegFormula *rf = getValueForRegAndDelete(areg);
                        if (rf == NULL)
                        {
#if DEBUG_STATIC_MEMORY
                           DEBUG_STMEM(4,
                              cerr << "SliceOut for LEA uop at address 0x" << hex 
                                   << pc << dec << ", uop index=" << uop.idx << endl;
                              cerr << "Found no formula for register " << areg << endl;
                           )
#endif
                           cachedFormula += *fit;
                        } else
                        {
                           cachedFormula += rf->formula * fit->ValueNum();
#if DEBUG_STATIC_MEMORY
                           DEBUG_STMEM(4,
                              cerr << "SliceOut for LEA uop at address 0x" << hex 
                                   << pc << dec << ", uop index=" << uop.idx << endl;
                              cerr << "Address Register " << areg << ": "
                                   << rf->formula << ", valueNum=" << fit->ValueNum() 
                                   << ", temp cachedFormula=" << cachedFormula
                                   << endl;
                           )
#endif
                           delete rf;
                        }
                     } else  // if it is not a register formula term
                        cachedFormula += *fit;
                  }  // for each term of the generic formula
               } else  // if could not get generic formula for memory operand of LEA
               {
                  // this should never happen, assert
                  cerr << "Could not get generic formula for memory operand of LEA micro-op at pc 0x"
                       << hex << pc << dec << ", index " << uop.idx << endl;
                  assert (!"We should be able to compute a generic formula for LEA always.");
               }
               
               _values.push_back(new RegFormula(reg, cachedFormula));
               break;  // fully process the LEA case here
            }
            // not a LEA, so no memory operands
            // check if it is an INVALID canonical opcode first
            if (op == OP_INVALID)
            {
               // an operation that I do not consider yet.
               // Print a BIG warning
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(1,
                  cerr << "Yo, ReferenceSlice::SliceOut, found INVALID op code at pc 0x"
                       << hex << pc << dec << " idx=" << uop.idx << ", num src operands="
                       << num_src << ". CHECK THIS OUT." << endl;
               )
#endif
               // Just create a register formula right here
               cachedFormula = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
               _values.push_back(new RegFormula(reg, cachedFormula));
               break;  // cannot refine formula further
            }

            rf1 = RegFormulaForOperand(dinst, ii, 0);
            if (rf1 == NULL)
            {
               fprintf (stderr, "In ReferenceSlice, case NORMAL does not have a valid first source operand; operand type %d, index %d, pc=0x%" PRIxaddr "\n",
                      opt1, opidx1, pc);
               cachedFormula = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
               _values.push_back(new RegFormula(reg, cachedFormula));
               break;  // cannot refine formula further
            }
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(5,
               cerr << "RefSlice: pc=0x" << hex << pc << dec << ", operand 1 type/index=" 
                    << opt1 << "/" << opidx1
                    << ", formula=" << rf1->formula << endl;
            )
#endif

            if (num_src==3)
            {
               rf1b = RegFormulaForOperand(dinst, ii, 1);
               OperandType opt1b = (OperandType)extract_op_type(ii->src_opd[1]);
               int opidx1b = extract_op_index(ii->src_opd[1]);
               rf2 = RegFormulaForOperand(dinst, ii, 2);
               OperandType opt2 = (OperandType)extract_op_type(ii->src_opd[2]);
               int opidx2 = extract_op_index(ii->src_opd[2]);
               if (rf1b==NULL)
                  fprintf (stderr, "In slice_reference, uop with 3 operands does not have a valid second source operand; operand type %d, index %d, pc=0x%" PRIxaddr "\n",
                         opt1b, opidx1b, pc);
               if (rf2==NULL)
                  fprintf (stderr, "In slice_reference, uop with 3 operands does not have a valid third source operand; operand type %d, index %d, pc=0x%" PRIxaddr "\n",
                         opt2, opidx2, pc);
               if (!rf1b || !rf2 || ii->type==IB_div)
               {
                  // we create a register formula for this instruction anyway. It computes
                  // both a quotient and a reminder
                  cachedFormula = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
               } else
               {
                  assert (op==OP_SLL);
                  coeff_t valueNum1, valueNum2;
                  ucoeff_t valueDen1, valueDen2;
                  if (IsConstantFormula(rf1b->formula, valueNum1, valueDen1) &&
                      IsConstantFormula(rf2->formula, valueNum2, valueDen2) &&
                      valueDen1==1 && valueDen2==1 && 
                      valueNum2>=0 && valueNum2<ii->width)
                  {
                     rf1->formula *= valueNum2;
                     int iw = ii->width;
                     uint64_t xvalue = (valueNum1>>(iw-valueNum2)) & ((1<<valueNum2)-1);
                     rf1->formula += sliceVal (xvalue, TY_CONSTANT);
                     cachedFormula = rf1->formula;
                  }
                  else
                  {
                     // operands are not constant, create register value
                     cachedFormula = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                  }
               }
               _values.push_back(new RegFormula(reg, cachedFormula));
               delete (rf1);
               if (rf1b)
                  delete (rf1b);
               if (rf2)
                  delete (rf2);
               break;  // cannot refine formula further
            } else
            if (num_src > 1)
            {
               rf2 = RegFormulaForOperand(dinst, ii, 1);
               OperandType opt2 = (OperandType)extract_op_type(ii->src_opd[1]);
               int opidx2 = extract_op_index(ii->src_opd[1]);
               if (rf2 == NULL)
               {
                  fprintf (stderr, "In slice_reference, case NORMAL does not have a valid second source operand; operand type %d, index %d, pc=0x%" PRIxaddr "\n",
                         opt2, opidx2, pc);
                  cachedFormula = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                  _values.push_back(new RegFormula(reg, cachedFormula));
                  delete (rf1);
                  break;  // cannot refine formula further
               }
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(5,
                  cerr << "RefSlice: pc=0x" << hex << pc << dec << ", operand 2 type/index=" 
                       << opt2 << "/" << opidx2
                       << ", formula=" << rf2->formula << endl;
               )
#endif
            } else
            {
               // only 1 operand; this must be a copy or an invalid operation 
               if (op!=OP_MOV && op!=OP_NOT && op!=OP_INVALID)  // can it be something else? 
               {
                  cerr << "ReferenceSlice::SliceOut, uop at address 0x" << hex << pc
                       << dec << ", index " << uop.idx << " has " << num_src 
                       << " source operands and is of type " 
                       << Convert_InstrBin_to_string(ii->type) << ", OP type: " 
                       << Convert_CanonicalOp_to_string(op) << endl;
                  assert (!"Unexpected OP type for a single source operand.");
               }
            }
            
            /* ***** ***** FIXME ***** ***** */
            if (op == OP_ADD)
            {
               rf1->formula += rf2->formula;
               cachedFormula = rf1->formula;
               _values.push_back(new RegFormula(reg, rf1->formula));
               delete (rf1);
               assert(rf2 != NULL);
               delete (rf2);
               break;
            }
            if (op == OP_SUB)
            {
               rf1->formula -= rf2->formula;
               cachedFormula = rf1->formula;
               _values.push_back(new RegFormula(reg, rf1->formula));
               delete (rf1);
               assert(rf2 != NULL);
               delete (rf2);
               break;
            }
            if (op == OP_MOV)
            {
               assert (rf2==NULL);
               cachedFormula = rf1->formula;
               _values.push_back(new RegFormula(reg, cachedFormula));
               delete (rf1);
               break;
            }
            if (op == OP_NOT)
            {
               assert (rf2==NULL);
               coeff_t valueNum1;
               ucoeff_t valueDen1;
               if (IsConstantFormula(rf1->formula, valueNum1, valueDen1))
                  cachedFormula = GFSliceVal(sliceVal (~valueNum1, TY_CONSTANT));
               else
                  cachedFormula = GFSliceVal(sliceVal (1, TY_REGISTER, pc, uop_idx, reg));
               _values.push_back(new RegFormula(reg, cachedFormula));
               delete (rf1);
               break;
            }
            if (op==OP_XOR && FormulasAreEqual(rf1->formula, rf2->formula))
            {
               // the result of the XOR is 0
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(1,
                 cerr << "SliceOut found XOR on equal formulas " << rf1->formula
                      << " setting output register " << reg << " to 0." << endl;
               )
#endif
               cachedFormula = GFSliceVal(sliceVal (0, TY_CONSTANT));
               _values.push_back(new RegFormula(reg, cachedFormula));
               delete (rf1);
               delete (rf2);
               break;
            } 
            
            // in all other cases, I may produce a non-trivial formula if one
            // or both operands are constant
            cachedFormula = processReferenceSliceOperation (reg, pc, uop_idx,
                            rf1->formula, rf2->formula, op);
            _values.push_back (new RegFormula (reg, cachedFormula));
            delete (rf1);
            delete (rf2);
            break;
         }
         break;

      case SliceEffect_HARD:
         // it is a Load
         {
            // Load instruction
            //
            // search for pc; if it is an unspill, on slice_in I searched
            // for the corresponding spill and inserted the formula
            // using pc as a key. if it is not found, then there is 
            // nothing we can do => define it as a LOAD to register
            RegFormula *rftemp = getValueForRegAndDelete(pc, uop.idx);
            if (rftemp == NULL)
            {
               // Inside SliceIn I create a TY_LOAD formula when I cannot find a 
               // corresponding SAVE. Thus, I should always find a formula
               // when I execute SliceOut.
               assert(! "Should I even be here?");
               cachedFormula = GFSliceVal(sliceVal(1, TY_LOAD, pc, uop_idx, reg));
               _values.push_back (new RegFormula (reg, cachedFormula));
            }
            else
            {
               rftemp->reg = reg;
               rftemp->pc = 0;
               rftemp->idx = -1;
               cachedFormula = rftemp->formula;
               _values.push_back(rftemp);
            }
         }
         break;
         
      case SliceEffect_IMPOSSIBLE:
      {
#if DEBUG_STATIC_MEMORY
         DEBUG_STMEM(5,
            cerr << "ReferenceSlice::SliceOut, handling IMPOSSIBLE case." << endl;
         )
#endif
         
#if 0   // Right now, I do not consider conditional moves to be IMPOSSIBLE
         if (inst_performs_conditional_move (inst->bits()) )
         {
            cachedFormula = GFSliceVal (sliceVal (1, TY_REGISTER, pc, uop_idx, reg));
            _values.push_back(new RegFormula(reg, cachedFormula));
            break;
         } else
#endif
         if (InstrBinIsBranchType(ii->type))
         {
            // only interprocedural calls are classified as IMPOSSIBLE
            // get call target from the surrogate block
            assert (b->num_outgoing()>0);
            PrivateCFG::Node *nb = b->firstOutgoing()->sink();
            // next assert can fail since I set calls based on XED as well.
            addrtype call_target = 0;
            if (nb && nb->isCallSurrogate())
               call_target = nb->getTarget();
            else
               call_target = mach_inst_branch_target(dinst);
            cachedFormula = GFSliceVal (sliceVal (1, TY_CALL, pc, uop_idx, call_target));
            _values.push_back(new RegFormula(reg, cachedFormula));
            break;
         } else
         {
            assert(! "Unhandled IMPOSSIBLE case.");
         }
         break;
      }

      default:
         assert(!"Bad case");
   }
   /* Print the final formula computed at this step. TODO ******/
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(4,
      cerr << "RefSliceOut: pc=0x" << hex << pc << dec << ", reg=" << reg
           << ", computed formula=" << cachedFormula << endl;
      printAllValues();
   )
#endif
}


GFSliceVal
ReferenceSlice::ComputeFormulaForMemoryOperand(PrivateCFG::Node* node, const DecodedInstruction *dInst, 
                  int uop_idx, int op_num, addrtype pc)
{
   // how to compute an effective address is platform dependent
   // RISC ISAs have only base + index registers. The two values
   // are summed. 
   // On x86, you have base, index, scale and displacement, brrr.
   // How to represent the address calculation in a machine 
   // independent way? Is it even possible?
   GFSliceVal opFormula (sliceVal(0, TY_CONSTANT));
   GFSliceVal _formula (sliceVal (0, TY_CONSTANT));
   if (!generic_formula_for_memory_operand(dInst, uop_idx, op_num, pc, _formula))
   {
      // iterate over the terms of the formula and slice all registers
      GFSliceVal::iterator fit;
      for (fit=_formula.begin() ; fit!=_formula.end() ; ++fit)
      {
         if (fit->TermType() == TY_REGISTER)
         {
            const register_info& reg = fit->Register();
            long value = 0;
            if (is_hardwired_register(reg, dInst, value))
            {
               opFormula += sliceVal(value, TY_CONSTANT);
               continue;
            }
            clear();
            Slice(node, uop_idx, reg, pc);
            RegFormula *rf = getValueForRegAndDelete(reg);
            if (rf==NULL)
            {
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(5,
                  cerr << "End slice for memory uop at address 0x" << hex 
                       << pc << dec << ", uop index=" << uop_idx << endl;
                  cerr << "Found no formula for register " << reg << endl;
               )
#endif
               opFormula += *fit;
            } else
            {
               opFormula += rf->formula * fit->ValueNum();
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(5,
                  cerr << "End slice for memory uop at address 0x" << hex 
                       << pc << dec << ", uop index=" << uop_idx << endl;
                  cerr << "Address Register " << reg << ": "
                       << rf->formula << endl;
               )
#endif
               delete rf;
            }
         } else  // if it is not a register formula term
            opFormula += *fit;
      }  // for each term of the generic formula
   } else  // if could not get generic formula for memory operand
   {
      // this should never happen, assert
      cerr << "Could not get generic formula for memory operand of micro-op at pc 0x"
           << hex << pc << dec << ", uop index " << uop_idx << endl;
      assert (!"We should be able to compute a generic formula always.");
   }
   return (opFormula);
}


/***************************************
 * Start of StrideSlice implementation *
 ***************************************/
GFSliceVal
StrideSlice::ComputeFormulaForMemoryOperand(PrivateCFG::Node* node, const DecodedInstruction *dInst, 
         int uop_idx, int op_num, addrtype pc)
{
//std::cout<<"OZGURDBG::Discover the Path func:"<<__func__<<" line"<<__LINE__<<std::endl;
   // how to compute an effective address is platform dependent
   // RISC ISAs have only base + index registers. The two values
   // are summed. 
   // On x86, you have base, index, scale and displacement, brrr.
   // How to represent the address calculation in a machine 
   // independent way? Is it even possible?
   GFSliceVal opFormula (sliceVal(0, TY_CONSTANT));
   GFSliceVal _formula (sliceVal (0, TY_CONSTANT));
   if (!generic_formula_for_memory_operand(dInst, uop_idx, op_num, pc, _formula))
   {
//std::cout<<"OZGURDBG::Discover the Path func:"<<__func__<<" line"<<__LINE__<<std::endl;
      // iterate over the terms of the formula and slice all registers
      GFSliceVal::iterator fit;
      for (fit=_formula.begin() ; fit!=_formula.end() ; ++fit)
      {
         if (fit->TermType() == TY_REGISTER)
         {
            // hardwired registers do not provide a stride. Their value
            // depends only on the instruction's encoding and the 
            // instruction's PC address. I can skip over them.
            const register_info& reg = fit->Register();
            long value = 0;
            if (! is_hardwired_register(reg, dInst, value))
            {
               clear();
               StartSlice(node, uop_idx, reg, pc);
               opFormula += getStrideForReg(reg) * fit->ValueNum();
            }
         } 
         // else, if it is not a register formula term, then it cannot have a stride
         // opFormula += *fit;
      }  // for each term of the generic formula
   } else  // if could not get generic formula for memory operand
   {
//std::cout<<"OZGURDBG::Discover the Path func:"<<__func__<<" line"<<__LINE__<<std::endl;
      // this should never happen, assert
      cerr << "Could not get generic formula for memory operand of micro-op at pc 0x"
           << hex << pc << dec << ", uop index " << uop_idx << endl;
      assert (!"We should be able to compute a generic formula always.");
   }
//std::cout<<"OZGURDBG::Discover the Path func:"<<__func__<<" line"<<__LINE__<<std::endl;
   return (opFormula);
}

void 
StrideSlice::BuildCyclicPath(PrivateCFG::Node *startb)
{
   PrivateCFG::NodeList ba;
   PrivateCFG::EdgeList ea;

   PrivateCFG *g = startb->inCfg();
   unsigned int mm = g->new_marker(3);  // I need 2 marker values
   start_rank = startb->getRank();
   const unsigned int* loopTree = g->LoopTree();
   unsigned int treeSize = g->LoopTreeSize();
   
   if (RecursiveBuildCyclicPath(startb, startb, ba, ea, 0, mm, loopTree, treeSize))
   {
      // now add the blocks and edges to the incomingEdges map
      assert (ba.size() == ea.size());
      assert (ba.size() > 0);
      
      PrivateCFG::NodeList::iterator nit = ba.begin();
      PrivateCFG::EdgeList::iterator eit = ea.begin();
      
      int max_rank = ba.size();
      PrivateCFG::NodeList::iterator head_node;
      bool head_set = false;
      for ( ; nit!=ba.end() && eit!=ea.end() ; ++nit, ++eit)
      {
         incomingEdges.insert(NodeEdgeMap::value_type(*nit, *eit));
         if (head_set)
            (*nit)->setPathRank(max_rank--);
         // find the back edge of the slice, to identify the head node
         if  ( (*eit)->isLoopBackEdge() &&
                  ((*nit)->getMarkedWith()==slice_marker || 
                   (*eit)->source()->getMarkedWith()==slice_marker)
             )
         {
            assert (! head_set);   // I cannot see it more than once
            head_node = nit;
            ++ head_node;
            head_set = true;
         }
      }
      assert(nit==ba.end() && eit==ea.end());
      assert (head_set);  // I should have found a head node
      
      // Now, I have to traverse the nodes again, up to the head_node point;
      for (nit=ba.begin() ; nit!=head_node ; ++nit)
         (*nit)->setPathRank(max_rank--);
      
      // After the loop, max_rank should be 0
      assert (max_rank == 0);

#if DEBUG_STATIC_MEMORY
      DEBUG_STMEM(2,
         fprintf(stderr, "NEW StrideSlice::BuildCyclicPath for scope marker %u: found cyclic path: ", slice_marker);
         for (nit=ba.begin() ; nit!=ba.end() ; ++nit)
            fprintf(stderr, "-> B%d [0x%" PRIxaddr ",0x%" PRIxaddr "] ", 
                (*nit)->getId(), (*nit)->getStartAddress(), (*nit)->getEndAddress());
         fprintf(stderr, "\n");
      )
#endif
   } else  // could not find a 1-iteration cyclic path that includes the
           // start block; do I need more than 1 iteration????
   {
      // for now print an error message and assert
      fprintf(stderr, "ERROR: Could not build a 1-iteration cyclic path that includes block %d [0x%" 
              PRIxaddr ",0x%" PRIxaddr "] for scope with marker %u. I do not know what to do.\n",
           startb->getId(), startb->getStartAddress(), startb->getEndAddress(), slice_marker);
      //assert(!"Chech the CFG of this routine.");//OZGUR FIXME:unknown original
      std::cout<<"OZGURERROR:: Chech the CFG of this routine: "<<startb->inCfg()->name()<<std::endl;
   }
}

// Find the LCA scope of 3 (or 2) loops, represented by marker1, marker2, marker3.
// The last maker can be 0, and then it is ignored. The function then computes 
// the LCA of only two of the loops.
// Children loops have a higher marker than their parents.
// - loopTree stores a representation of the tree of loops, each entry points 
//   to its parent
unsigned int
StrideSlice::loops_LCA(const unsigned int* loopTree, unsigned int treeSize, unsigned int marker1, 
             unsigned int marker2, unsigned int marker3)
{
   if (!loopTree || marker1>=treeSize || marker2>=treeSize || marker3>=treeSize)
      return (0);
   
   // compute the LCA of the first two markers
   while (marker1 != marker2)
   {
      while (marker1>marker2)
         marker1 = loopTree[marker1];
      while (marker2>marker1)
         marker2 = loopTree[marker2];
   }
   
   if (marker1>0 && marker3>0)
      while (marker1 != marker3)
      {
         while (marker1>marker3)
            marker1 = loopTree[marker1];
         while (marker3>marker1)
            marker3 = loopTree[marker3];
      }
   return (marker1);
}

bool
StrideSlice::RecursiveBuildCyclicPath(PrivateCFG::Node *startb, PrivateCFG::Node *b,
       PrivateCFG::NodeList &ba, PrivateCFG::EdgeList &ea, int back_edges, unsigned int mm,
       const unsigned int* loopTree, unsigned int treeSize)
{
   unsigned int marked = b->getMarkedWith();

#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(4,
      cerr << "RecursivePath: slice marker=" << slice_marker 
           << ", path marker=" << mm << ", start rank=" << start_rank
           << ", back edges=" << back_edges
           << ", consider block " << *b << " visited with " 
           << b->getVisitedWith() << " and rank " << b->getRank()
           << " and marked with " << marked
           << endl;
   )
#endif

   if ((b->visited(mm-2) && back_edges==0) || (b->visited(mm-1) && back_edges>0))
      // we reached a node that was explored before, but
      // it did not lead to a good cyclic path
      return (false);     // return right away; no point in wasting time with it
      
   if (b->visited(mm))  // found node already in the path, we close a cycle
   {
      // if we started from a node inside an inner loop of the current scope, 
      // we can stop at a node different than the start block. 
      // Because this block is part of the currently discovered path, we know
      // that we have an acyclic path from this block to the start block;
      // However, if the starting block has the same marker as the slice marker,
      // then we can stop only at the starting block
//      if ((startb->getMarkedWith()>slice_marker || b==startb) && back_edges==1)
      if (b==startb && back_edges==1)  // this is good
         return (true);
      if (back_edges==0)
         return (false);
      // Otherwise I must test if we are closing the loop after taking the back edge
      // Iterate back over blocks and edges until we either find the current block
      // or we find the loop back edge. If we find the loop back edge first, then we
      // are good. Otherwise, this not the cyclic path we want
      PrivateCFG::NodeList::iterator nit = ba.end();
      PrivateCFG::EdgeList::iterator eit = ea.end();
      PrivateCFG::Node *src_b = b;
      while (nit!=ba.begin() && eit!=ea.begin())
      {
         --nit;
         --eit;
         // first test if this is the back edge
         assert((*eit)->sink() == (*nit));
         if ((*eit)->isLoopBackEdge() &&
               (src_b->getMarkedWith()==slice_marker || (*nit)->getMarkedWith()==slice_marker)
            )
            // this is the loop back edge, so we close the cycle after traversing it.
            return (true);
         if ((*nit) == b)
            return (false);
         src_b = (*nit);
      }
      // I should return from the loop, not exit normally
      assert(!"I should return, not exit from the loop");
      
      // otherwise, we are crossing the path in a bad place
      return (false);
   }
   
   // if we followed a back edge and we reached a block with a less or equal
   // rank than the starting rank, then back track; not a good path
   // I can test for equal rank, because the block has not been visited before,
   // so it cannot be a block on current path
   if (back_edges>0 && b->getRank()<=start_rank)
      return (false);
   
   typedef std::multimap<int, PrivateCFG::Edge*, std::greater<int> > RankOrderedEdgesMap;
   RankOrderedEdgesMap orderedEdges;
   PrivateCFG::IncomingEdgesIterator ieit(b);
   while ((bool)ieit)
   {
      PrivateCFG::Edge *e = ieit;
      orderedEdges.insert(RankOrderedEdgesMap::value_type(e->source()->getRank(), e));
      ++ ieit;
   }
   b->visit(mm);
   ba.push_back(b);
   int old_start_rank = start_rank;
   
   // if b is the entry into an inner loop, set start_rank to its rank
   // as long as b is part of an inner loop and we did not take any back edge
   // yet, set b's rank as the starting rank
   // gmarin, 09/04/2013: I should only change the start_rank if the new value
   // is smaller. I can take a back edge of an inner loop, which I don't count 
   // as an inner loop, going to a higher rank node. This prevent the 
   // algorithm from closing the loop.
   if (marked>slice_marker && back_edges==0 && start_rank>b->getRank())
      start_rank = b->getRank();
   
   for (RankOrderedEdgesMap::iterator reit=orderedEdges.begin() ;
        reit!=orderedEdges.end() ;
        ++ reit)
   {
      PrivateCFG::Edge *e = reit->second;
      PrivateCFG::Node* bp = e->source();
      unsigned int bmarker = bp->getMarkedWith();

//      if (bmarker==slice_marker || (bmarker>slice_marker && !e->isLoopBackEdge()))
      // I may follow back edges of inner loops as well as long as I do not close
      // a cyclic path on an inner loop block. However, I only count back edges of the
      // slice_marker loop as traversed back edges. Some CFG are complex where
      // loops have strange overlaps and thus I need to allow for this behavior.
      // Example: routine isortidc_ part of benchmark 454.calculix (SPECCPU 2006)
      // 
      // Hmm, I found a failing case in a benchmark based on Cactus, 
      // routine parse_expression. This routine is actually in a shared library linked 
      // with Cactus. The problem is that as I traverse inner-loops' back edges,
      // I end up at higher ranks than the rank of the node which is the source node of
      // the back edge of this slice.
      // The solution is not to use the global CFG's rank values as stop conditions,
      // but to compute an additional rank value of the blocks on a cyclic path.
      // Computing ranks this way will prevent the rank inversion problem.
      //
      // Damn, found another bad case in routine cvtas_t_to_a, where I need to take
      // the back-edge of an inner loop, even though I start from another inner loop.
      // I must allow back-edges of inner loops that are disjoint. That is, they are
      // not on the path from the starting inner loop to the slicing loop, in a
      // hypotetical loop nesting tree. How do I represent the tree without the
      // Tarjans informations?? What info do I need to determine disjointness??
      if (bmarker>=slice_marker && 
              (loops_LCA(loopTree, treeSize, startb->getMarkedWith(), bmarker, marked)
                     ==slice_marker || !e->isLoopBackEdge()
              )
         )
//              (startb->getMarkedWith()==slice_marker || !e->isLoopBackEdge() 
//                || bmarker==slice_marker || marked==slice_marker
//              )
//         )
      {
         int be_delta = 0;
         if (e->isLoopBackEdge() && (bmarker==slice_marker || marked==slice_marker))
            be_delta = 1;
         
         ea.push_back(e);
         if (RecursiveBuildCyclicPath(startb, bp, ba, ea, back_edges+be_delta, mm,
                      loopTree, treeSize))  // found path
            return (true);
         // otherwise, we did not find a path
         // remove current edge and try a new one
         ea.pop_back();
      }
   }
   ba.pop_back();
   // I found that by blocking fully explore nodes, I can lock myself out of
   // discovering the path. Should I limit the blocking to only blocks that are 
   // explored after I traverse the loop's back edge? EXPERIMENTAL, it may fail again!
   if (back_edges)
      b->visit(mm-1);
   else
      b->visit(mm-2);
   start_rank = old_start_rank;
   
   // did not find an edge, return 0
   return (false);
}

bool 
StrideSlice::SliceIn(PrivateCFG::Edge *e, const register_info& reg)
{
   // Follow forward edges of current or inner loops, 
   // and the back edge of current loop only.
   //
   // New version. I precomputed cyclic paths that include the starting block.
   // I only follow edges that are part of this cyclic path.
   // Just check that the sink of this edge is in the map and that
   // this edge is its selected incoming Edge.

#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(4,
      PrivateCFG::Node* bp = e->source();
      unsigned int bmarker = bp->getMarkedWith();
      cerr << endl << "  --- StrideSlice::SliceIn(Edge), slice_marker=" << slice_marker
           << ", bmarker=" << bmarker << " testing Edge " << *e 
           << ", is backEdge? " << e->isLoopBackEdge()
           << ", with source node " << *bp << ", start_rank=" << start_rank
           << ", source rank=" << bp->getPathRank() << ", start_pc=0x" << hex
           << start_pc << dec << endl;
   )
#endif

   PrivateCFG::Node* bn = e->sink();
   NodeEdgeMap::iterator neit = incomingEdges.find(bn);
   if (neit!=incomingEdges.end() && neit->second == e)
   {
      if (e->isLoopBackEdge() && 
          (bn->getMarkedWith()==slice_marker || e->source()->getMarkedWith()==slice_marker))
         back_edged += 1;
#if DEBUG_STATIC_MEMORY
      DEBUG_STMEM(4,
         PrivateCFG::Node* bs = e->source();
         fprintf (stderr, "SliceIn: taking edge E%d from B%d [0x%" PRIxaddr ",0x%" PRIxaddr "] to B%d [0x%"
                  PRIxaddr ",0x%" PRIxaddr "]\n", e->getId(), bs->getId(), bs->getStartAddress(),
                  bs->getEndAddress(), bn->getId(), bn->getStartAddress(), bn->getEndAddress());
      )
#endif
      return (true);
   }
   else
      return (false);
}

#if 0
bool 
StrideSlice::SliceIn(PrivateCFG::Edge *e, const register_info& reg)
{
   // Follow forward edges of current or inner loops, 
   // and the back edge of current loop only.
   //
   // Instead of using addresses, which can be misleading in some
   // cases when the loop entry block is not at the lowest address, check
   // the markers of the head and tail nodes. We take advantage of the fact
   // that inner loops have greater markers than their parent loops. 
   // For StrideSlice, we follow any forward edge as long as both its ends
   // have markers >= this loop's marker. But we follow only back-edges
   // for which the head node's marker is equal to the loop's marker, while
   // the tail node's marker doesn't matter. I am thinking here that in  
   // theory, a node can act as loop entry node for 2 or more loops if more
   // than two backedges go into it. But the source node of the back-edge
   // must be a clear internal node of the loop (cannot have two back-edges
   // starting in that node).
   // Moreover, we do not have to test the sink node of an edge (the current node),
   // because it should satisfy the condition if we are even trying to continue
   // slicing from it. Test only the source node.
   PrivateCFG::Node* bp = e->source();
   unsigned int bmarker = bp->getMarkedWith();
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(4,
      cerr << endl << "  --- StrideSlice::SliceIn(Edge), slice_marker=" << slice_marker
           << ", bmarker=" << bmarker << " testing Edge " << *e 
           << ", is backEdge? " << e->isLoopBackEdge()
           << ", with source node " << *bp << ", start_rank=" << start_rank
           << ", source rank=" << bp->getPathRank() << ", start_pc=0x" << hex
           << start_pc << dec << endl;
   )
#endif
   if (bmarker==slice_marker || (bmarker>slice_marker && !e->isLoopBackEdge()))
   {
      if (e->isLoopBackEdge())
      {
         if ( bp->getPathRank()<start_rank ||
             (bp->getPathRank()==start_rank && bp->getEndAddress()<start_pc) )
            return (false);
         back_edged += 1;
      }
      return (true);
   } else
      return (false);
}
#endif

void 
StrideSlice::SliceOut(PrivateCFG::Edge *e, const register_info& reg)
{
   if (e->isLoopBackEdge() && 
       (e->sink()->getMarkedWith()==slice_marker || e->source()->getMarkedWith()==slice_marker))
      back_edged -= 1;
}

// called at the beginning of SliceNext, check if we reached a stopping condition
bool 
StrideSlice::SliceNextIn(PrivateCFG::Node *b, const register_info& reg)
{
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(3,
      cerr << endl << "  --- StrideSlice::SliceNextIn, slice block [0x" << hex 
           << b->getStartAddress() << ",0x" << b->getEndAddress() << dec 
           << "], reg=" << reg << endl;
   )
#endif
   int rank = b->getPathRank();
//   if (back_edged==0 && b->getMarkedWith()>slice_marker)
//      start_rank = rank;
   if ((back_edged>start_be && (rank<start_rank ||
         (rank==start_rank && b->getStartAddress()<=start_pc)))
       || (back_edged-start_be)>1)
   {
      _values.push_back(new RegCache(reg, MIAMI_NO_ADDRESS, 0));
      return (false);
   }
   return (true);
}

void 
StrideSlice::SliceNextOut(PrivateCFG::Node *b, const register_info& reg)
{
   // SliceNextOut is called if I did not find any valid incoming edge
   // setup a register term
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(3,
      cerr << endl << "  === StrideSlice::SliceNextOut, slice block [0x" << hex << b->getStartAddress()
           << ",0x" << b->getEndAddress() << dec << "], reg=" << reg << endl;
   )
#endif
   _values.push_back (new RegCache (reg, MIAMI_NO_ADDRESS, 0));
}

// save the constant value of a register at a given PC
void 
StrideSlice::RecordRegisterValue(const register_info& reg, const uOp_t& uop, coeff_t value)
{
   // for now, save only constant value from Stores.
   // Stores do not create any value, so I can use their coordinates (pc, uop idx)
   // to cache the value that is stored.
   // For other uop types it is best if I just ignore this value. Test for it in 
   // SliceOut when I need it.
   if (InstrBinIsStoreType(uop.iinfo->type))
   {
      addrtype pc = uop.dinst->pc;
      int idx = uop.idx;
      _values.push_back (new RegCache (reg, pc, idx));
      // where should I save the hardwired register value? What PC/idx combo defines it?
      StrideInst*& si = iMarkersLow(pc, idx);
      if (!si)
         si = new StrideInst();
      else if (si->formula)
         delete si->formula;
      si->formula = new GFSliceVal (sliceVal (value, TY_CONSTANT));
   }
}

/* extract formula and stride info for the specified operand of a micro-op.
 */
int 
StrideSlice::GetFormulaInfoForOperand(const DecodedInstruction *dinst, 
        const instruction_info *ii, int op_num, GFSliceVal& formula, 
        int& strideF, addrtype& defPc, unsigned long& dfsTarget)
{
   OperandType optype = (OperandType)extract_op_type(ii->src_opd[op_num]);
   int opidx = extract_op_index(ii->src_opd[op_num]);
   
   if (optype == OperandType_IMMED)
   {
      formula = GFSliceVal(sliceVal(ii->imm_values[opidx].value.s, TY_CONSTANT));
      strideF = 0;
      defPc = dinst->pc;
      dfsTarget = 0;
   } else
   {
      RegCache *rc = 0;
      register_info reg1;
      bool initialized = false;
      if (optype == OperandType_INTERNAL)
      {
         int rwidth = ii->width;
         if (ii->exec_unit==ExecUnit_VECTOR)
            rwidth *= ii->vec_len;
         reg1 = register_info(opidx, RegisterClass_TEMP_REG, 0, rwidth-1);
         rc = getValueForRegAndDelete(reg1);
      } else
      {
         assert (optype == OperandType_REGISTER);
         reg1 = mach_inst_reg_operand(dinst, opidx);
         reg1.name = register_name_actual(reg1.name);
         // check if it is a hardwired register
         long value = 0;
         if (is_hardwired_register(reg1, dinst, value))
         {
            formula = GFSliceVal(sliceVal(value, TY_CONSTANT));
            strideF = 0;
            defPc = dinst->pc;
            dfsTarget = 0;
            initialized = true;
         } else
            rc = getValueForRegAndDelete(reg1);
      }
      if (!initialized)
      {
         if (rc == NULL)
         {
            fprintf (stderr, "In StrideSlice::GetFormulaInfoForOperand, cannot find a valid entry for operand number %d. "
                     "Operand type %d, index %d, pc=0x%" PRIxaddr "\n",
                         op_num, optype, opidx, dinst->pc);
            return (-1);
         }

          // assert(rc != NULL);
         dfsTarget = rc->targetDFSIndex;
         defPc = rc->defPc;
         if (defPc == MIAMI_NO_ADDRESS)
         {
            formula = GFSliceVal (sliceVal (1, TY_REGISTER, 
#if USE_ADDR
                          start_addr, 
#else
                          dinst->pc,
#endif
                          -1,
                          reg1));
            strideF = 0;
         } else
         {
            assert (defPc != 0);
            SIPCache *iMarkers = &iMarkersLow;
            StrideInst *si = (*iMarkers)(defPc, rc->idx);
            formula = *(si->formula);
            strideF = si->is_strideF;
         }
         delete (rc);
      }
   }
   return (0);
}



// Start stride_slice implementation
//
// Slice back in a loop until encountering a function call, a load 
// operation, or start of function;
// another stopping condition can be a unsupported instruction;
// in general, go back along add, sub, mov, shift, sethi, 
// (or, and, not, xor - for constant values only);
// When slicing back, several paths may exist. Take the backedge of the loop
// to find cycles in values definition
// Stop at the first value found.
//
bool 
StrideSlice::SliceIn(PrivateCFG::Node* b, int uop_idx, const register_info& reg)
//addrtype pc, const instruction* inst, int_reg reg,
//              int high_bits)
{
   UopCodeCache *ucc = b->MicroOpCode();
   int num_uops = ucc->NumberOfUops();
   // I should always receive a valid uop index
   assert(uop_idx>=0 && uop_idx<num_uops);
   const uOp_t& uop = ucc->UopAtIndex(uop_idx);
   addrtype pc = uop.dinst->pc;
   int rank = b->getPathRank();

// print the decoded instruction; 
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(7,
      debug_decode_instruction_at_pc((void*)(pc+b->RelocationOffset()), uop.dinst->len);
      DumpInstrList(uop.dinst);
   )
#endif
   
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(4,
      cerr << "  - - > StrideSlice::slice_in, reg=" << reg
           << ", pc=0x" << hex << pc 
#if USE_ADDR
           << ", to_pc=0x" << start_addr 
#endif
           << dec << ", inst uop=" << uop.idx << ", block uop idx=" << uop_idx
           << ", rank=" << rank << ", _start_rank=" << start_rank
           << ", back_edged=" << back_edged << ", top cycle_addr=" << hex 
           << (sCycles.empty()?0:sCycles.begin()->second.cycle_addr) << dec 
           << ", top cycle_op_idx=" << (sCycles.empty()?0:sCycles.begin()->second.cycle_op_idx)
           << endl;
   )
#endif

   if (
#if USE_ADDR
          pc<min_addr || 
#endif
          (back_edged>start_be && (rank<start_rank || 
             (rank==start_rank && pc<start_pc))) 
         || (back_edged-start_be)>1)
   {
      cerr << "Hello: back_edged=" << back_edged << ", start_be=" << start_be 
           << ", rank=" << rank << ", start_rank=" << start_rank << hex
           << ", pc=0x" << pc << ", start_pc=0x" << start_pc << dec
           << ", back_edged-start_be=" << (back_edged-start_be) << endl;
           
      cerr << "I return false from StrideSlice::slice_in" << endl;
      return (false);   // stop slicing
   }
   SIPCache *iMarkers = &iMarkersLow;
//   if (high_bits)
//      iMarkers = iMarkersHi;
   StrideInst* &si = (*iMarkers)(pc, uop.idx);
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(5,
      cerr << "DEBUG: si=" << si << endl;
      if (si)
      {
         cerr << "DEBUG: si->formula=" << si->formula << endl;
         if (si->formula)
            cerr << "DEBUG: *(si->formula)=" << *(si->formula) << endl;
      }
      fflush(stderr);
   )
#endif
   if (si && si->loopCnt > 0)  // we are in a loop that includes this 
                               // instruction
   {
      return (false);
   }
   if (!si)
      si = new StrideInst();
   si->loopCnt = back_edged+1;
   // TODO set dfsIndex perhaps at this place, only first time I visit the instruction
   si->dfsIndex = ++strideDFSCount;

   // There is a problem if there is not a perfect match between the 
   // instructions where I continue slicing and those where I stop in both
   // slice_in and slice_out. Right now tests are different and everything
   // is a big mess.
   // Possible problems:
   // - if I continued slicing in slice_in and then I take the formula
   // from the cache in slice_out, I will have leftover registers on the stack
   // which can affect future slice_outs (use the wrong reg value).
   // - if I stop slicing in slice_in and then I try to compute a formula in
   // slice_out, I have the opposite problem. I can either fail because I
   // do not find a register (good case), or I can use registers defined at 
   // different PCs, which will cause the computation to fail later, making
   // it harder to debug.
   //
   // This is very fragile. Big rewrite necessary.
   
   // if it is an unfinished cycle, I will try to recompute it again.
   // I also need to have a valid targetDFSIndex for formulas that are 
   // part of unfinished cycles.
   if (si->formula!=NULL && 
        (si->is_strideF!=1 || 
         (si->cacheKey==sliceKey && findTargetInStack(si->dfsTarget))
        ))
   {
      si->useCached = true;
      return (false);  // the formula is already computed, back off
   } 
   else
   {
#if DEBUG_STATIC_MEMORY
      DEBUG_STMEM(5,
         printRegCacheStack();
      )
#endif
      // check if we find a record of this reg / pc in the RegCache.
      // If found, we can take the dfsIndex from there.
      si->useCached = false;
   }
   
   // If we are recomputing the formula, should not we delete any existing formula?
   // Avoid leaks.
   if (si->formula)
   {
      delete (si->formula);
      si->formula = 0;
   }
   
   const instruction_info *ii = uop.iinfo;
   const DecodedInstruction *dinst = uop.dinst;
   
   switch (SliceEffect(b, uop, reg))
   {
      case SliceEffect_EASY:
      {
         return (false);    // stop slicing
      }

      case SliceEffect_NORMAL:
      {
         // continue back-slicing on each input register only if this is not 
         // an INVALID op. If INVALID op, print a warning (and generate a 
         // REGISTER formula?)
         CanonicalOp op = uop_canonical_decode(dinst, ii);
         if (op == OP_INVALID)
         {
            // an operation that I do not consider yet.
            // Print a BIG warning
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(1,
               cerr << "Yo, StrideSlice::SliceIn, found INVALID op code at pc 0x"
                    << hex << pc << dec << " idx=" << uop.idx << ". CHECK THIS OUT." << endl;
            )
#endif
            // should I create a register formula right here?
            return (false);
         }
         // if the operations is a XOR or a SUB and the two source operands are the same,
         // stop slicing and just store a constant 0 formula.
         if (op==OP_XOR || op==OP_SUB)
         // && FormulasAreEqual(rf1->formula, rf2->formula))
         {
            int num_src = ii->num_src_operands;
            assert (num_src >= 2);
            
            OperandType optype1 = (OperandType)extract_op_type(ii->src_opd[0]);
            int opidx1 = extract_op_index(ii->src_opd[0]);

            OperandType optype2 = (OperandType)extract_op_type(ii->src_opd[1]);
            int opidx2 = extract_op_index(ii->src_opd[1]);
            
            if (optype1==OperandType_REGISTER && optype2==OperandType_REGISTER)
            {
               register_info reg1 = mach_inst_reg_operand(dinst, opidx1);
               register_info reg2 = mach_inst_reg_operand(dinst, opidx2);
               if (reg1 == reg2)
               {
                  // the result of the XOR or SUB will be zero in all instances.
                  // No need to slice further.
#if DEBUG_STATIC_MEMORY
                  DEBUG_STMEM(1,
                     cerr << "StrideSlice::SliceIn: found XOR or SUB with identical register operands " << reg1
                          << " Micro-op result will always be 0, so do not slice further." << endl;
                  )
#endif
                  if (si->formula)
                     delete (si->formula);
                  si->formula = new GFSliceVal(sliceVal(0, TY_CONSTANT));
                  si->is_strideF = 0;
                  si->dfsTarget = 0;
                  si->useCached = true;
                  return (false);
               }
            }
         } 
         
         return (true);   // in all other cases continue back-slicing
      }

      case SliceEffect_HARD:
         {
            // Instruction is a load.
            //
            int opidx = uop.iinfo->first_src_operand_of_type(OperandType_MEMORY);
            assert(opidx >= 0);  // opidx should be positive is this is a Load
            RefFormulas *rf = refFormulas(pc, opidx);
            coeff_t loadNum;
            ucoeff_t loadDen;
            bool is_const = false;
         
            // if I have a formula computed for this load, try finding a
            // preceeding store to the same location and continue slicing 
            // from there
            bool foundSpill = false;
            if (rf && !rf->base.is_uninitialized() && 
                  (!(is_const=IsConstantFormula(rf->base, loadNum, loadDen)) || loadNum!=0))
            {
               // Load from some computed formula. Search for a reaching 
               // store at same address (same formula)
               // and continue slicing from there.
               //
               register_info spilled_reg;
               start_pc = pc;
               if (SliceFollowSave(b, uop_idx, rf->base, spilled_reg))
               {
#if DEBUG_STATIC_MEMORY
                  DEBUG_STMEM(5,
                     cerr << "StrideSlice::SliceIn, returned from SliceFollowSave, reg=" 
                          << reg << ", pc=0x" << hex << pc << dec << ", SpilledREG=" 
                          << spilled_reg << " :: ";
                  )
#endif
                  if (spilled_reg==MIAMI_NO_REG)
                  {
                     assert(!"we should not be here. FollowSave should return false instead.");
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(5,
                        cerr << "found NO_REG" << endl;
                     )
#endif
//                     _values.push_back(new RegCache(MIAMI_NO_REG, pc, uop.idx, MIAMI_NO_DFS_INDEX));
                  }
                  else
                  {
                     RegCache* rftemp = getValueForRegAndDelete(spilled_reg);
                     if (rftemp==NULL || rftemp->defPc==MIAMI_NO_ADDRESS)
                     {
#if DEBUG_STATIC_MEMORY
                        DEBUG_STMEM(5,
                           cerr << "found reg=" << spilled_reg << "; formula NULL or defined before pc" << endl;
                        )
#endif
//                        _values.push_back(new RegCache(MIAMI_NO_REG, pc, uop.idx, MIAMI_NO_DFS_INDEX));
                     }
                     else
                     {
                        StrideInst *spillSI = (*iMarkers)(rftemp->defPc, rftemp->idx);
                        assert (spillSI != NULL);
#if DEBUG_STATIC_MEMORY
                        DEBUG_STMEM(5,
                           cerr << "found reg=" << spilled_reg 
                                << ", defPc=0x" << hex << rftemp->defPc << dec 
                                << ", defIdx=" << rftemp->idx
                                << "; formula for reg is " << *(spillSI->formula) << endl;
                        )
#endif
                        foundSpill = true;
                        if (pc!=rftemp->defPc || uop.idx!=rftemp->idx)
                        {
                           assert (si != spillSI);
                           assert (si->formula != spillSI->formula);
                           if (si->formula)
                              delete (si->formula);
                           si->formula = new GFSliceVal(*(spillSI->formula));
                           si->is_strideF = spillSI->is_strideF;
                           si->dfsTarget = spillSI->dfsTarget;
                        }
                        // Should I also mark the formula as maybe indirect on
                        // the unspill reference? I found a case in the memory benchmark
                        // where the index is the result of a function call (drand48/floor),
                        // but we find a SAVE (spill) at the same symbolic address on
                        // a previous iteration, as the code tries to permute indices.
                        // However, that is not really spilling. The address of the unspill
                        // on iteration X is different from the address of spill on iteration Y.
                        si->formula->set_indirect_access(pc, uop_idx);
#if DEBUG_STATIC_MEMORY
                        DEBUG_STMEM(5,
                           cerr << "DEBUG-spill: si->formula=" << si->formula << endl;
                           if (si->formula)
                              cerr << "DEBUG-spill: *(si->formula)=" << *(si->formula) << endl;
                        )
#endif
                     }
                     if (rftemp)
                        delete rftemp;
                  }
               } 
               // else, follow_save did not find a corresponding store
            } else  // found a load whose address is not computed or is zero??
            {
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(5,
                  cerr << "StrideSlice::SliceIn on reg=" << reg << " found load without a valid base " 
                       << "formula. Cannot execute SliceFollowSave for this case." << endl;
               )
#endif
//               _values.push_back(new RegCache(MIAMI_NO_REG, pc, uop.idx, MIAMI_NO_DFS_INDEX));
            }
            // if we did not find a SAVE, create a LOAD or REFERENCE formula
            if (!foundSpill)
            {
               if (is_const)
               {
                  loadNum /= loadDen;
                  si->formula = new GFSliceVal(sliceVal(1, TY_REFERENCE, pc, uop_idx, loadNum));
               } else
                  si->formula = new GFSliceVal(sliceVal(1, TY_LOAD, pc, uop_idx, reg));
               si->formula->set_indirect_access(pc, uop_idx);
               si->is_strideF = 0;
            }
            si->useCached = true;
//            _values.push_back(new RegCache(MIAMI_NO_REG, pc, uop.idx));
            // stop slicing
            return (false);
         }

      case SliceEffect_IMPOSSIBLE:
         // stop slicing at calls. We do not perform inter-procedural slicing
         return (false);

      default:
         assert(!"Bad case");
         return (false);
   }
}


void 
StrideSlice::SliceOut(PrivateCFG::Node* b, int uop_idx, const register_info& reg)
//bb* b, addr pc, const instruction* inst, int_reg reg, int high_bits)
{
   UopCodeCache *ucc = b->MicroOpCode();
   int num_uops = ucc->NumberOfUops();
   // I should always receive a valid uop index
   assert(uop_idx>=0 && uop_idx<num_uops);
   const uOp_t& uop = ucc->UopAtIndex(uop_idx);
   addrtype pc = uop.dinst->pc;
   int rank = b->getPathRank();
   
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(4,
      cerr << "  < - - StrideSlice::slice_out, reg=" << reg 
           << ", pc=0x" << hex << pc 
#if USE_ADDR
           << ", to_pc=0x" << start_addr 
#endif
           << dec << ", inst uop=" << uop.idx << ", block uop idx=" << uop_idx
           << ", rank=" << rank << ", start_rank=" << start_rank
           << ", back_edged=" << back_edged << ", top cycle_addr=" << hex 
           << (sCycles.empty()?0:sCycles.begin()->second.cycle_addr) << dec 
           << ", top cycle_op_idx=" << (sCycles.empty()?0:sCycles.begin()->second.cycle_op_idx)
           << endl;
   )
#endif

   assert(back_edged>=0);
   SIPCache *iMarkers = &iMarkersLow;
//   if (high_bits)
//      iMarkers = iMarkersHi;
   StrideInst* si = (*iMarkers)(pc, uop.idx);
   assert (si != 0);
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(5,
      cerr << "DEBUG-SliceOut: si=" << si << endl;
      if (si)
      {
         cerr << "DEBUG-SliceOut: si->formula=" << si->formula << endl;
         if (si->formula)
            cerr << "DEBUG-SliceOut: *(si->formula)=" << *(si->formula) << endl;
      }
      fflush(stderr);
   )
#endif

//   if (pc<start_pc && back_edged>start_be
//          && (start_pc<end_addr || back_edged>start_be+1) )
   if ((back_edged>start_be && (rank<start_rank || 
             (rank==start_rank && pc<start_pc))) 
        || (back_edged-start_be)>1)
   {
      cerr << "Dump: pc=0x" << hex << pc << ", start_pc=0x" << start_pc
#if USE_ADDR
           << ", end_addr=0x" << end_addr 
#endif
           << ", back_edged=" << dec
           << back_edged << ", start_be=" << start_be 
           << ", rank=" << rank << ", start_rank=" << start_rank << endl;
      // I do not know if I should insert a value here. It may be that 
      // the address is out of range, or we should not get here in fact.
      // Better assert
      assert(!"We should not be here. Run in debuger for more info");
      si->loopCnt = 0;
      return;   // stop slicing
   }

#if USE_ADDR
   if (pc<min_addr)
   {
      // I do not know if I should insert a value here. It may be that 
      // the address is out of range, or we should not get here in fact.
      // Better assert
      fprintf (stderr, "pc=0x%" PRIxaddr ", start_addr=0x%" PRIxaddr ", min_addr=0x%" PRIxaddr "\n", 
              pc, start_addr, min_addr);
      cerr << "StrideSlice::slice_out, reg=" << reg 
           << ", pc=0x" << hex << pc << ", to_pc=0x" << start_addr << dec 
           << ", back_edged=" << back_edged << ", top cycle_addr=" << hex 
           << (sCycles.empty()?0:sCycles.begin()->second.cycle_addr) << dec 
           << ", top cycle_op_idx=" << (sCycles.empty()?0:sCycles.begin()->second.cycle_op_idx)
           << endl;
      assert(!"We should not be here. Run in debuger for more info");
      si->loopCnt = 0;
      return;   // stop slicing
   }
#endif

#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(5,
      printRegCacheStack();
   )
#endif

   addrtype cycle_addr = 0;
   int cycle_op_idx = 0;
   int cycle_cnt = 0;
   register_info cycle_reg(MIAMI_NO_REG);
   int cycle_hb = 0;

   assert(si->loopCnt>0);  // had to be set in slice_in
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(5,
      cerr << "StrideSlice::slice_out, instruction dfsIndex=" << si->dfsIndex
           << endl;
   )
#endif
   if (si->loopCnt<(back_edged+1))
   // we are in a loop that includes this instruction
   {
      assert (si->dfsIndex > 0);
      assert (si->dfsIndex <= strideDFSCount);
      // found loop; do something here
      
      // I can keep track of multiple loops now. It is possible to
      // find a cycle while backing up from another one

      cycle_addr = pc;
      cycle_op_idx = uop.idx;
      cycle_cnt = back_edged + 1 - si->loopCnt;
      cycle_reg = si->loopReg;
      cycle_hb = si->loopHb;
#if DEBUG_STATIC_MEMORY
      DEBUG_STMEM(5,
         cerr << "StrideSlice::slice_out, top cycle_addr=" << hex 
              << (sCycles.empty()?0:sCycles.begin()->second.cycle_addr) << dec 
              << ", top cycle_op_idx=" << (sCycles.empty()?0:sCycles.begin()->second.cycle_op_idx)
              << ", top cycle_cnt=" << (sCycles.empty()?0:sCycles.begin()->second.cycle_cnt) 
              << ", cycle_reg=" << (sCycles.empty()?MIAMI_NO_REG:sCycles.begin()->second.cycle_reg) 
              << ", cycle_hb=" << (sCycles.empty()?0:sCycles.begin()->second.cycle_hb) << endl;
      )
#endif
      assert (cycle_cnt>0);
      register_info ari;
      assert (InstrBinIsLoadType(uop.iinfo->type) ||
                 (cycle_reg!=MIAMI_NO_REG && uop.Reads(cycle_reg, false, ari))
             );
      // if there is a formula already, I should delete it?
//         assert (si->formula == NULL);
      if (si->formula != NULL) 
      {
         cerr << "Old Formula: " << *(si->formula) << " with strideF="
              << si->is_strideF << " and cacheKey=" 
              << si->cacheKey << " is deleted" << endl;
         delete (si->formula);
      }
      
      si->formula = new GFSliceVal (sliceVal (0, TY_CONSTANT));
      si->is_strideF = 1;  // we begin a stride formula
      si->cacheKey = sliceKey;
      si->dfsTarget = si->dfsIndex;
      _values.push_back (new RegCache (reg, pc, uop.idx, si->dfsIndex));
#if DEBUG_STATIC_MEMORY
      DEBUG_STMEM(5,
         cerr << "StrideSlice::slice_out, add new CycleInfo cycle_addr=" << hex 
              << cycle_addr << dec 
              << ", cycle_op_idx=" << cycle_op_idx
              << ", cycle_cnt=" << cycle_cnt
              << ", cycle_reg=" << cycle_reg 
              << ", cycle_hb=" << cycle_hb << endl;
      )
#endif
      sCycles.insert(CycleMap::value_type(si->dfsIndex,
            CycleInfo(cycle_addr, cycle_op_idx, cycle_cnt, cycle_reg, cycle_hb)));
      return;
   }
   
   // this must be true always
   assert(si->loopCnt == (back_edged+1));
   
   // we can have a cycle only on slice_effect == NORMAL (source registers)
   // We can also have one on HARD (Load operation)
   GFSliceVal rezF;
   int rezStride = 0;
   unsigned long rezDFSTarget = 0;
   bool hard_case = false;

   // create some shortcut variables
   const instruction_info *ii = uop.iinfo;
   const DecodedInstruction *dinst = uop.dinst;
   
   if (!sCycles.empty()) 
   {
      CycleInfo &ci = sCycles.begin()->second;
      cycle_addr = ci.cycle_addr;
      cycle_op_idx = ci.cycle_op_idx;
      cycle_cnt = ci.cycle_cnt;
      cycle_reg = ci.cycle_reg;
      cycle_hb = ci.cycle_hb;
   }
   if (cycle_addr==pc && cycle_op_idx==uop.idx) // this is the start of the cycle
   {
#if DEBUG_STATIC_MEMORY
      DEBUG_STMEM(5,
         cerr << "StrideSlice::slice_out, matching top Cycle: cycle_addr=" << hex 
              << cycle_addr << dec 
              << ", cycle_op_idx=" << cycle_op_idx
              << ", cycle_cnt=" << cycle_cnt
              << ", cycle_reg=" << cycle_reg 
              << ", cycle_hb=" << cycle_hb 
              << ", thisIsLoad?=" << InstrBinIsLoadType(ii->type)
              << endl;
      )
#endif
      // just assert a few things
      register_info ari;
      assert (InstrBinIsLoadType(ii->type) ||
                 (cycle_reg!=MIAMI_NO_REG && uop.Reads(cycle_reg, false, ari))
             );
      // now that we saw it is possible to find a cycle while backing from
      // another one, the formula may be different than zero (I set it to 
      // a register, but I am not sure if it makes a difference)
      assert (si->formula!=NULL 
//           && IsConstantFormula (*(*iMarkers)[pc].formula, valNum, valDen) 
//           && valNum==0 
           && si->is_strideF==1);
      if (InstrBinIsLoadType(ii->type))
      {
         assert (si->formula && si->useCached);
         rezF = *(si->formula);
         rezStride = si->is_strideF;
         rezDFSTarget = si->dfsTarget;
#if DEBUG_STATIC_MEMORY
         DEBUG_STMEM(5,
            cerr << "   - slice_out, LoadType, Formula:" << *(si->formula) 
                 << ", rezF=" << rezF 
                 << ", rezStride=" << rezStride
                 << ", rezDFSTarget=" << rezDFSTarget 
                 << endl;
         )
#endif
      }
   } else
   {
#if DEBUG_STATIC_MEMORY
      DEBUG_STMEM(5,
         cerr << "StrideSlice::slice_out, NOT matching top Cycle: cycle_addr=" << hex 
              << cycle_addr << dec 
              << ", cycle_op_idx=" << cycle_op_idx
              << ", cycle_cnt=" << cycle_cnt
              << ", cycle_reg=" << cycle_reg 
              << ", cycle_hb=" << cycle_hb 
              << ", thisIsLoad?=" << InstrBinIsLoadType(ii->type)
              << endl;
      )
#endif
      // if the formula for this instruction was computed before
      // take it from cache directly
      // Hmm. I will need to remove the formulas for the input registers
      // in this case. Using the value in the cache makes sense only if
      // I find the value in the coresponding slice_in. However, if
      // the formula was computed in the meanwhile, I should ignore it and
      // compute it again. How do I check this???
      if (si->formula != NULL)
      {
//         if ((*iMarkers)[pc].is_strideF!=1 || (*iMarkers)[pc].cacheKey==sliceKey)
         if (si->useCached)
         {
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(5,
               cerr << "   - slice_out, NOT matching top Cycle, USE CACHE, add RegCache for reg=" 
                    << reg << " at pc=" << hex << pc << dec
                    << ", idx=" << uop.idx
                    << ", si->dfsTarget=" << si->dfsTarget 
                    << ". Formula=" << *(si->formula) << endl;
            )
#endif
            _values.push_back(new RegCache(reg, pc, uop.idx, si->dfsTarget));
            si->loopCnt = 0;
            return;
         } else
         {
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(5,
               cerr << "   - slice_out, NOT matching top Cycle, DON'T USE CACHE, delete Formula:" 
                    << *(si->formula) << endl;
            )
#endif
            delete (si->formula);
            si->formula = NULL;
         }
      }
   }


   switch (SliceEffect(b, uop, reg))
   {
      case SliceEffect_EASY:
         assert(cycle_addr != pc || cycle_op_idx!=uop.idx);
         assert(si->formula == NULL);

         // some of the instructions that are possible, are: Move, Jump, Branch, PriviledgedOp, NOP
         // Only Move is a plausible operation while slicing address registers
         if (ii->type==IB_move || ii->type==IB_copy) 
         {
            assert (ii->num_imm_values == 1);
            si->formula = new GFSliceVal(sliceVal(ii->imm_values[0].value.s, TY_CONSTANT));
         } 
         //OZGURFIXME start I am guessing I add this check for lea
         else  if (ii->type==IB_lea){
            si->formula = new GFSliceVal(sliceVal(ii->imm_values[0].value.s, TY_REGISTER));
         }
         //OZGURFIXME END
         else
         {
            fprintf (stderr, "ALERT, Micro-op %d at pc 0x%" PRIxaddr " with slice effect EASY on reg %s, has type %d which is not MOVE. Check it out!\n",
                    uop.idx, pc, reg.ToString().c_str(), ii->type);
            assert (!"StrideSlice::SliceOut: Non MOVE, SliceEffect_EASY micro-op encountered!");
         }
         si->cacheKey = sliceKey;
         si->dfsTarget = 0;
         _values.push_back(new RegCache(reg, pc, uop.idx));
         break;

      case SliceEffect_NORMAL:
         {
            int num_src = ii->num_src_operands;
            assert (num_src > 0);
            // do not include flag and status source registers;
            // Iterate over source operands, starting at the last one, and skip over
            // all flag/status register operands
            while (num_src>0 && 
                   (OperandType)extract_op_type(ii->src_opd[num_src-1])==OperandType_REGISTER)
            {
               int opidx = extract_op_index(ii->src_opd[num_src-1]);
               register_info reg1 = mach_inst_reg_operand(dinst, opidx);
               if (!is_flag_or_status_register(reg1))
                  break;
               else
                  num_src -= 1;
            }
            
            CanonicalOp op = uop_canonical_decode(dinst, ii);
//            if ((ii->type!=IB_div && ii->type!=IB_shift && ii->type!=IB_cmp_xchg && 
//                   ii->type!=IB_cmp && ii->type!=IB_insert && num_src>2) || num_src>3)
            // tired of adding op-types for all kinds of obscure operations that I cannot
            // handle anyway. We do not care if an unhandled (Invalid) opcode has more
            // than two source operands.
            if ((ii->type!=IB_div && op!=OP_INVALID && num_src>2) || num_src>3)
            {
               fprintf(stderr, "ALERT, Found micro-op %d at pc 0x%" PRIxaddr " of type %s with slice effect NORMAL on reg %s, that has %d source operands. Check it out!\n",
                    uop.idx, pc, Convert_InstrBin_to_string(ii->type), 
                    reg.ToString().c_str(), num_src);
               assert(! "Found micro-op with more than expected source operands.");
            }

            // I already asserted that num_src > 0; I should have at least one source operand
            // no source operand should be of MEMORY type; MEM goes to the HARD case
            OperandType opt1 = (OperandType)extract_op_type(ii->src_opd[0]);
            int opidx1 = extract_op_index(ii->src_opd[0]);
            assert (opt1!=OperandType_MEMORY || ii->type==IB_lea);
            
            /** Process LEA here, can it be part of the stride? *
             ****** TODO ********/
            if (ii->type == IB_lea)
            {
               assert (num_src == 1);  // only a memory operand
               // TODO: compute effective address
               GFSliceVal form1 (sliceVal(0, TY_CONSTANT));
               GFSliceVal form2;
               int strideF1 = 0, strideF2;
               unsigned int dfsTarget1 = 0, dfsTarget2;
               int is_irregular = 0;
               int is_indirect = 0;
               AddrIntSet _ind_pcs;
               
               GFSliceVal _formula (sliceVal (0, TY_CONSTANT));
               if (!generic_formula_for_memory_operand(dinst, uop_idx, opidx1, pc, _formula))
               {
                  // iterate over the terms of the formula and slice all registers
                  GFSliceVal::iterator fit;
                  for (fit=_formula.begin() ; fit!=_formula.end() ; ++fit)
                  {
                     if (fit->TermType() == TY_REGISTER)
                     {
                        // hardwired registers do not provide a stride. Their value
                        // depends only on the instruction's encoding and the 
                        // instruction's PC address. I can skip over them.
                        const register_info& reg = fit->Register();
                        coeff_t valNum = fit->ValueNum();
                        long value = 0;
                        if (is_hardwired_register(reg, dinst, value))
                        {
                           form2 = GFSliceVal(sliceVal(value*valNum, TY_CONSTANT));
                           strideF2 = 0;
                           dfsTarget2 = 0;
                        } else
                        // Get the formula for this register ... hmm
                        // I cannot use my getFormulaForOperand method, because the registers
                        // are not actual operands. They are all part of the 1st operand.
                        // I need to do it all manually.
                        {
                           RegCache *rc = getValueForRegAndDelete(reg);
                           if (rc == NULL)
                           {
                              fprintf (stderr, "In StrideSlice::SliceOut, cannot find a valid entry for register %s part of LEA operand at pc 0x%" PRIxaddr "\n",
                                           reg.ToString().c_str(), pc);
                              form2 = GFSliceVal(*fit);
                              strideF2 = 0;
                              dfsTarget2 = 0;
                           } else
                           {
                              // assert(rc != NULL);
                              addrtype defPc2 = rc->defPc;
                              dfsTarget2 = rc->targetDFSIndex;
                              if (defPc2 == MIAMI_NO_ADDRESS)
                              {
                                 form2 = GFSliceVal(*fit);
                                 strideF2 = 0;
                              } else
                              {
                                 assert (defPc2 != 0);
                                 StrideInst *si = (*iMarkers)(defPc2, rc->idx);
                                 form2 = *(si->formula) * valNum;
                                 strideF2 = si->is_strideF;
                              }
                              delete (rc);
                           }
                        }  // get formula info for register
                     } else
                     {
                        form2 = GFSliceVal(*fit);
                        strideF2 = 0;
                        dfsTarget2 = 0;
                     }
                     
                     if (dfsTarget1==0 || (dfsTarget2 && dfsTarget2<dfsTarget1))
                        dfsTarget1 = dfsTarget2;

                     if (form2.has_irregular_access())
                        is_irregular = 1;
                     if (is_indirect==1 || form2.has_indirect_access()==1)
                        is_indirect = 1;
                     else
                     {
                        if (form2.has_indirect_access()==2)
                        {
                           is_indirect = 2;
                           AddrIntSet& tempS = form2.get_indirect_pcs();
                           _ind_pcs.insert(tempS.begin(), tempS.end());
                        }
                     }

                     if (strideF1==1 || strideF2==1)
                     {
                        form1 += form2;
                        strideF1 = 1;
                     } else
                     if (strideF1==strideF2) 
                     {
                        form1 += form2;
                        // strideF1 stays the same, they are equal
                     } else
                     {
                        assert ((strideF2==0 && strideF1==2) ||
                                (strideF1==0 && strideF2==2) );
                        if (strideF1 == 0)
                           form1 = form2;
                        strideF1 = 2;
                     }
                  }  // for each term of the generic formula
                  
                  rezF = form1;
                  rezStride = strideF1;
                  rezDFSTarget = dfsTarget1;
                  
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
               } else  // if could not get generic formula for memory operand
               {
                  // this should never happen, assert
                  cerr << "Could not get generic formula for memory operand of micro-op at pc 0x"
                       << hex << pc << dec << ", index " << uop.idx << endl;
                  assert (!"We should be able to compute a generic formula always.");
               }
               hard_case = true;
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(5,
                  cerr << "StrideSlice::SliceOut - LEA case at pc=0x" << hex << pc << dec 
                       << " found formula " << rezF << " with strideF=" << rezStride << endl;
               )
#endif
               break;
               assert(! "Implement LEA case");
            }
            
            // not a LEA, so no memory operands
            // check if it is an INVALID canonical opcode first
            if (op == OP_INVALID)
            {
               // an operation that I do not consider yet.
               // Print a BIG warning
#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(1,
                  cerr << "Yo, StrideSlice::SliceOut, found INVALID op code at pc 0x"
                       << hex << pc << dec << " idx=" << uop.idx << ", num src operands="
                       << num_src << ". CHECK THIS OUT." << endl;
               )
#endif
               // Just create a register formula right here
               si->formula = new GFSliceVal(sliceVal (1, TY_REGISTER, pc, uop_idx, reg));
               si->cacheKey = sliceKey;
               si->dfsTarget = 0;
               _values.push_back (new RegCache (reg, pc, uop.idx));
               break;  // cannot refine formula further
            }
            
            GFSliceVal form1;
            int strideF1 = 0;
            unsigned long dfsTarget1 = 0;
            addrtype defPc1 = 0;
            if (GetFormulaInfoForOperand(dinst, ii, 0, form1, strideF1, defPc1, dfsTarget1) < 0)
            {
               // could not find a valid operand, create a register formula
               si->formula = new GFSliceVal(sliceVal (1, TY_REGISTER, pc, uop_idx, reg));
               si->cacheKey = sliceKey;
               si->dfsTarget = 0;
               _values.push_back (new RegCache (reg, pc, uop.idx));
               break;  // cannot refine formula further
            }
            
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(5,
               cerr << "StrideSlice: pc=0x" << hex << pc << dec 
                    << ", operand 1 type/index=" << opt1 << "/" << opidx1
                    << ", has formula=" << form1 << " defined at pc=0x" << hex << defPc1 
                    << dec << " with strideF1=" << strideF1 << " and dfsTarget1="
                    << dfsTarget1 << endl;
            )
#endif

            GFSliceVal form1b;
            int strideF1b = 0;
            unsigned long dfsTarget1b = 0;
            addrtype defPc1b = 0;

            GFSliceVal form2;
            int strideF2 = 0;
            unsigned long dfsTarget2 = 0;
            addrtype defPc2 = 0;
            
            int second_op = 1;
            if (num_src==3)  // && ii->type==IB_div)
            {
               second_op = 2;

               if (GetFormulaInfoForOperand(dinst, ii, 1, form1b, strideF1b, defPc1b, dfsTarget1b) < 0)
               {
                  // could not find a valid operand, create a register formula
                  si->formula = new GFSliceVal(sliceVal (1, TY_REGISTER, pc, uop_idx, reg));
                  si->cacheKey = sliceKey;
                  si->dfsTarget = 0;
                  _values.push_back (new RegCache (reg, pc, uop.idx));
                  break;  // cannot refine formula further
               }
            }
            
            if (num_src > 1)
            {
               if (GetFormulaInfoForOperand(dinst, ii, second_op, form2, strideF2, defPc2, dfsTarget2) < 0)
               {
                  // could not find a valid operand, create a register formula
                  si->formula = new GFSliceVal(sliceVal (1, TY_REGISTER, pc, uop_idx, reg));
                  si->cacheKey = sliceKey;
                  si->dfsTarget = 0;
                  _values.push_back (new RegCache (reg, pc, uop.idx));
                  break;  // cannot refine formula further
               }

#if DEBUG_STATIC_MEMORY
               DEBUG_STMEM(5,
                  OperandType opt2 = (OperandType)extract_op_type(ii->src_opd[second_op]);
                  int opidx2 = extract_op_index(ii->src_opd[second_op]);
                  cerr << "StrideSlice: pc=0x" << hex << pc << dec << ", operand 2 type/index=" 
                       << opt2 << "/" << opidx2
                       << ", formula=" << form2 << " defined at pc=0x" << hex << defPc2
                       << dec << " with strideF2=" << strideF2 << " and dfsTarget2="
                       << dfsTarget2 << endl;
               )
#endif
            } else
            {
               // only 1 operand; this must be a copy, unary, or an invalid operation 
               if (op!=OP_MOV && op!=OP_NOT && op!=OP_INVALID)  // can it be something else? 
               {
                  cerr << "StrideSlice::SliceOut, uop at address 0x" << hex << pc
                       << dec << ", index " << uop.idx << " has " << num_src 
                       << " source operands and is of type " 
                       << Convert_InstrBin_to_string(ii->type) << ", OP type: " 
                       << Convert_CanonicalOp_to_string(op) << endl;
                  assert (!"Unexpected OP type for a single source operand.");
               }
               
               // Implement the copy operation
               // **** TODO *****
               // The copy operation is like an add with the second operand a constant of 0
               form2 = GFSliceVal(sliceVal(0, TY_CONSTANT));
               dfsTarget2 = 0;
               strideF2 = 0;
               defPc2 = pc;
            }
            

            coeff_t valueNum1, valueNum2;
            ucoeff_t valueDen1, valueDen2;
            bool rf1IsConst = IsConstantFormula(form1, valueNum1, valueDen1);
            bool rf2IsConst = IsConstantFormula(form2, valueNum2, valueDen2);

            int is_irregular = 0;
            int is_indirect = 0;
            AddrIntSet _ind_pcs;
            // deal with the case of a strideF==1 and the other ==2; I think I've
            // not been doing the right thing until now.
            if (strideF1==1 && strideF2==2)
            {
               if (!rf2IsConst || valueNum2!=0)
               {
                  is_irregular = 1;
               }
               form2 += MakeFormulaForOperand(dinst, ii, uop_idx-1, second_op);
               strideF2 = 0;
            } else
            if (strideF1==2 && strideF2==1)
            {
               if (!rf1IsConst || valueNum1!=0)
               {
                  is_irregular = 1;
               }
               form1 += MakeFormulaForOperand(dinst, ii, uop_idx-1, 0);
               strideF1 = 0;
            }
            // If one of the strides is of type 1 (in process) and the other is 2
            
            if (dfsTarget1==0)
               rezDFSTarget = dfsTarget2;
            else 
               if (dfsTarget2==0)
                  rezDFSTarget = dfsTarget1;
               else   // both are non-zero; select minimum
                  rezDFSTarget = min(dfsTarget1, dfsTarget2);

            if (form1.has_irregular_access() || form2.has_irregular_access())
               is_irregular = 1;
            if (  form1.has_indirect_access()==1 || 
                  form2.has_indirect_access()==1)
               is_indirect = 1;
            else
            {
               if (form1.has_indirect_access()==2)
               {
                  is_indirect = 2;
                  _ind_pcs = form1.get_indirect_pcs();
               }
               if (form2.has_indirect_access()==2)
               {
                  is_indirect = 2;
                  AddrIntSet& tempS = form2.get_indirect_pcs();
                  _ind_pcs.insert(tempS.begin(), tempS.end());
               }
            }
            // I should also test formula 1b in case this is a DIV with 3 operands
            if (num_src==3) // && ii->type==IB_div)
            {
               if (form1b.has_irregular_access())
                  is_irregular = 1;
               if (form1b.has_indirect_access()==1)
                  is_indirect = 1;
               else
                  if (form1b.has_indirect_access()==2 && is_indirect!=1)
                  {
                     is_indirect = 2;
                     AddrIntSet& tempS = form1b.get_indirect_pcs();
                     _ind_pcs.insert(tempS.begin(), tempS.end());
                  }
                  
               if (strideF1==1 || strideF1b==1)
                  strideF1 = 1;
               else
                  strideF1 = std::max(strideF1, strideF1b);
            }

            hard_case = true;
            // Important combinations for strideF1 and strideF2 are:
            // i)   strideF1 == 1 || strideF2 == 1
            // ii)  strideF1 == strideF2 (== 0 or 2)
            // iii) strideF1 != strideF2 (one is 0, one is 2)
            if (op == OP_ADD || op == OP_MOV)
            {
               if (strideF1==1 || strideF2==1)
               {
                  rezF = form1 + form2;
                  rezStride = 1;
               } else
               if (strideF1==strideF2) 
               {
                  rezF = form1 + form2;
                  rezStride = strideF1;  // any of them, they are equal
               } else
               {
                  assert ((strideF2==0 && strideF1==2) ||
                          (strideF1==0 && strideF2==2) );
                  if (strideF1 == 2)
                     rezF = form1;
                  else
                     rezF = form2;
                  rezStride = 2;
               }
               if (is_irregular)
                  rezF.set_irregular_access();
               if (is_indirect==1)
                  rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
               else if (is_indirect==2)
                  rezF.set_indirect_pcs(_ind_pcs);
               break;
            }

            if (op == OP_SUB)
            {
               if ( (strideF1==strideF2) // they can be 0 or 2, not 1 ???
                    || (strideF1==1) )
               {
                  rezF = form1 - form2;
                  rezStride = strideF1;  // any of them, they are equal
               } else
                  if (strideF2==1)
                  {
                     if (rf1IsConst && valueNum1==0)
                     {
                        rezF = form2*(-1);
                        rezStride = 1;
                     }
                     else
                     {
#if DEBUG_STATIC_MEMORY
                        DEBUG_STMEM(0,
                           cerr << "WARNING: stride_slice::slice_out, SUB operation at pc=0x"
                                << hex << pc << dec 
                                << " found index variable that changes sign, resulting in divergent stride" 
                                << endl;
                        )
#endif
                        rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                        rezStride = 1;
                        rezF.set_irregular_access();
                     }
                  } else
                  {
                     assert ((strideF2==0 && strideF1==2) ||
                             (strideF1==0 && strideF2==2) );
                     if (strideF1 == 2)
                        rezF = form1;
                     else
                        rezF = form2*(-1);
                     rezStride = 2;
                  }
               if (is_irregular)
                  rezF.set_irregular_access();
               if (is_indirect==1)
                  rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
               else if (is_indirect==2)
                  rezF.set_indirect_pcs(_ind_pcs);
               break;

            }
            if ((!rf1IsConst && !rf2IsConst) ||
                (num_src==3 && op==OP_SLL) )
            {
               rezF = GFSliceVal (sliceVal (1, TY_REGISTER, pc, uop_idx, reg));
               if (strideF1==1 || strideF2==1)
                  rezStride = 1;
               else
                  rezStride = std::max (strideF1, strideF2);
               if (is_irregular || strideF1==1 || strideF2==1)
                  rezF.set_irregular_access();
               if (is_indirect==1)
                  rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
               else if (is_indirect==2)
                  rezF.set_indirect_pcs(_ind_pcs);
               break;
            }
            
            // work only with integer, so use int arithmetic
            if (valueDen1==0 || valueDen2==0)
            {
               fprintf(stderr, "ERROR: one of the denominators is 0, How did I get here??\n");
               fprintf(stderr, "valueNum1=%" PRIcoeff " / 0x%" PRIxcoeff " ; valueDen1=%" PRIucoeff " / 0x%" PRIxucoeff "\n",
                     valueNum1, valueNum1, valueDen1, valueDen1);
               fprintf(stderr, "valueNum2=%" PRIcoeff " / 0x%" PRIxcoeff " ; valueDen2=%" PRIucoeff " / 0x%" PRIxucoeff "\n",
                     valueNum2, valueNum2, valueDen2, valueDen2);
               assert(!"Bad case of constant arithmetic");
            }
            valueNum1 /= valueDen1;
            valueNum2 /= valueDen2;
            if (rf2IsConst)
            {
               if (op == OP_SLL)
               {
                  if (valueNum2!=0 || strideF2>0)
                  {
                     if (strideF1 == 1)
                     {
#if DEBUG_STATIC_MEMORY
                        DEBUG_STMEM(0,
                           cerr << "WARNING: stride_slice::slice_out, SLL operation at pc=0x"
                                << hex << pc << dec 
                                << " found index variable is shifted left, resulting in geometric stride" 
                                << endl;
                        )
#endif
                        rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                        rezF.set_irregular_access();
                     } else
                     if (strideF2 > 0)
                     {
#if DEBUG_STATIC_MEMORY
                        DEBUG_STMEM(0,
                           cerr << "WARNING: stride_slice::slice_out, SLL operation at pc=0x"
                                << hex << pc << dec 
                                << " found stride formula on right side, resulting in irregular stride" 
                                << endl;
                        )
#endif
                        rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                        rezF.set_irregular_access();
                     } else
                     {
                        assert(valueNum2>=0);
                        rezF = form1;
                        rezF *= (1<<valueNum2);
                     }
                  } else
                     rezF = form1;
                  if (strideF1==1 || strideF2==1)
                     rezStride = 1;
                  else
                     rezStride = std::max(strideF1, strideF2);
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
                  break;
               }
               if (op==OP_SRL || op==OP_SRA)
               {
                  if (valueNum2<0)
                  {
                     cerr << "Error in SRL/SRA at pc=0x" << hex << pc << dec 
                          << ", oper1=" << form1 << ", oper2="
                          << form2 << endl;
                     assert(valueNum2>=0);
                  }
                  
                  if (valueNum2!=0 || strideF2>0)
                  {
                     if (strideF1 == 1)
                     {
#if DEBUG_STATIC_MEMORY
                        DEBUG_STMEM(0,
                           cerr << "WARNING: stride_slice::slice_out, SRL/SRA operation at pc=0x"
                                << hex << pc << dec 
                                << " found index variable is divided, resulting in logarithmic stride" 
                                << endl;
                        )
#endif
                        rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                        rezF.set_irregular_access();
                     } else
                     if (strideF2 > 0)
                     {
#if DEBUG_STATIC_MEMORY
                        DEBUG_STMEM(0,
                           cerr << "WARNING: stride_slice::slice_out, SRL/SRA operation at pc=0x"
                                << hex << pc << dec 
                                << " found stride formula on right side, resulting in irregular stride" 
                                << endl;
                        )
#endif
                        rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                        rezF.set_irregular_access();
                     } else
                     if (strideF1 == 2)
                     {
#if DEBUG_STATIC_MEMORY
                        DEBUG_STMEM(0,
                           cerr << "WARNING: stride_slice::slice_out, SRL/SRA operation at pc=0x"
                                << hex << pc << dec 
                                << " stride term is shifted right, resulting in irregular stride" 
                                << endl;
                        )
#endif
                        rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                        rezF.set_irregular_access();
                        
                     } else
                     {
                        assert(valueNum2>=0);
                        rezF = form1;
                        ucoeff_t divisor = 1;
                        rezF /= (divisor<<valueNum2);
                        if (FormulaHasComplexDenominator (rezF))
                           rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                     }
                  } else
                     rezF = form1;
                  if (strideF1==1 || strideF2==1)
                     rezStride=1;
                  else
                     rezStride = std::max(strideF1, strideF2);
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
                  break;
               }
               if ((op==OP_OR || op==OP_ANDN) && valueNum2==0 && strideF2==0)
               {
                  // well, I cannot be here unless strideF2 == 0, so why test
                  // if it is different than zero? Also, strideF1 can have 
                  // any value as this is actually just a move (register copy)
                  rezF = form1;
                  rezStride = strideF1;
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
                  break;
               }
               if (op == OP_MUL)
               {
                  if (strideF1 == 1 || strideF2==1)
                  {
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(0,
                        cerr << "WARNING: stride_slice::slice_out, MUL operation at pc=0x"
                             << hex << pc << dec 
                             << " found index variable is multiplied, resulting in geometric stride" 
                             << endl;
                     )
#endif
                     rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                     rezF.set_irregular_access();
                     rezStride = 1;
                  } else
                  {
                     rezF = form1;
                     rezF *= (valueNum2);
                     rezStride = std::max(strideF1, strideF2);
                  }
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
                  break;
               }
               if (op == OP_DIV)
               {
                  if (strideF1 == 1)
                  {
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(0,
                        cerr << "WARNING: stride_slice::slice_out, DIV operation at pc=0x"
                             << hex << pc << dec
                             << " found index variable is divided, resulting in logarithmic stride" 
                             << endl;
                     )
#endif
                     rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                     rezF.set_irregular_access();
                  } else
                  if (strideF2 > 0)
                  {
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(0,
                        cerr << "WARNING: stride_slice::slice_out, DIV operation at pc=0x"
                             << hex << pc << dec 
                             << " found stride formula on right side, resulting in undetermined stride" 
                             << endl;
                     )
#endif
                     rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                     rezF.set_irregular_access();
                  } else
                  if (num_src == 3)
                  {
                     rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                  } else
                  {
                     rezF = form1;
                     if (valueNum2<0)
                        rezF *= (-1);
                     rezF /= abs(valueNum2);
                     if (FormulaHasComplexDenominator (rezF))
                        rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                  }
                  if (strideF1==1 || strideF2==1)
                     rezStride = 1;
                  else
                     rezStride = std::max(strideF1, strideF2);
                  if (is_irregular)
                     rezF.set_irregular_access ();
                  if (is_indirect==1)
                     rezF.set_indirect_access (GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs (_ind_pcs);
                  break;
               }
            }
            if (rf1IsConst)
            {
               if (op==OP_OR && valueNum1==0 && strideF1==0)
               {
                  // if it is different than zero? Also, strideF2 can have 
                  // any value as this is actually just a move (register copy)
                  rezF = form2;
                  rezStride = strideF2;
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access (GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs (_ind_pcs);
                  break;
               }
               if (op == OP_MUL)
               {
                  if (strideF1 == 1 || strideF2==1)
                  {
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(0,
                        cerr << "WARNING: stride_slice::slice_out, MUL operation at pc=0x"
                             << hex << pc << dec 
                             << " found index variable is multiplied, resulting in geometric stride" 
                             << endl;
                     )
#endif
                     rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                     rezF.set_irregular_access();
                     rezStride = 1;
                  } else
                  {
                     rezF = form2;
                     rezF *= (valueNum1);
                     rezStride = std::max(strideF1, strideF2);
                  }
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
                  break;
               }
            }

            if (rf1IsConst && rf2IsConst)
            {
               if (op == OP_NOT)
               {
                  // This is actually a one operand computation.
                  // Second operand is a fake constant of 0.
                  if (strideF1!=0)
                  {
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(0,
                        cerr << "WARNING: stride_slice::slice_out, NOT operation at pc=0x"
                             << hex << pc << dec 
                             << " found index variable in bitwise complement operation, resulting in undetermined stride" 
                             << endl;
                     )
#endif
                     rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                     rezF.set_irregular_access();
                  } else
                  {
                     rezF = GFSliceVal(sliceVal(~valueNum1, TY_CONSTANT));
                  }
                  rezStride = strideF1;
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
                  break;
               }
               if (op == OP_OR)
               {
                  if (strideF1!=0 || strideF2!=0)
                  {
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(0,
                        cerr << "WARNING: stride_slice::slice_out, OR operation(3) at pc=0x"
                             << hex << pc << dec 
                             << " found index variable in bitwise operation, resulting in undetermined stride" 
                             << endl;
                     )
#endif
                     rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                     rezF.set_irregular_access();
                  } else
                  {
                     rezF = GFSliceVal(sliceVal(valueNum1|valueNum2, TY_CONSTANT));
                  }
                  if (strideF1==1 || strideF2==1)
                     rezStride = 1;
                  else
                     rezStride = std::max(strideF1, strideF2);
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
                  break;
               }
               if (op == OP_ORN)
               {
                  if (strideF1!=0 || strideF2!=0)
                  {
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(0,
                        cerr << "WARNING: stride_slice::slice_out, ORN operation at pc=0x"
                             << hex << pc << dec 
                             << " found index variable in bitwise operation, resulting in undetermined stride" 
                             << endl;
                     )
#endif
                     rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                     rezF.set_irregular_access();
                  } else
                  {
                     rezF = GFSliceVal(
                          sliceVal(valueNum1|(~valueNum2), TY_CONSTANT));
                  }
                  if (strideF1==1 || strideF2==1)
                     rezStride = 1;
                  else
                     rezStride = std::max(strideF1, strideF2);
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
                  break;
               }
               if (op == OP_ANDN)
               {
                  if (strideF1!=0 || strideF2!=0)
                  {
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(0,
                        cerr << "WARNING: stride_slice::slice_out, ANDN operation at pc=0x"
                             << hex << pc << dec 
                             << " found index variable in bitwise operation, resulting in undetermined stride" 
                             << endl;
                     )
#endif
                     rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                     rezF.set_irregular_access();
                  } else
                  {
                     rezF = GFSliceVal(
                          sliceVal(valueNum1&(~valueNum2), TY_CONSTANT));
                  }
                  if (strideF1==1 || strideF2==1)
                     rezStride = 1;
                  else
                     rezStride = std::max(strideF1, strideF2);
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
                  break;
               }
               if (op == OP_AND)
               {
                  if (strideF1!=0 || strideF2!=0)
                  {
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(0,
                        cerr << "WARNING: stride_slice::slice_out, AND operation at pc=0x"
                             << hex << pc << dec 
                             << " found index variable in bitwise operation, resulting in undetermined stride" 
                             << endl;
                     )
#endif
                     rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                     rezF.set_irregular_access();
                  } else
                  {
                     rezF = GFSliceVal(
                          sliceVal(valueNum1&valueNum2, TY_CONSTANT));
                  }
                  if (strideF1==1 || strideF2==1)
                     rezStride = 1;
                  else
                     rezStride = std::max(strideF1, strideF2);
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
                  break;
               }
               if (op == OP_XOR)
               {
                  if (strideF1!=0 || strideF2!=0)
                  {
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(0,
                        cerr << "WARNING: stride_slice::slice_out, XOR operation at pc=0x"
                             << hex << pc << dec 
                             << " found index variable in bitwise operation, resulting in undetermined stride" 
                             << endl;
                     )
#endif
                     rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                     rezF.set_irregular_access();
                  } else
                  {
                     rezF = GFSliceVal(
                          sliceVal(valueNum1^valueNum2, TY_CONSTANT));
                  }
                  if (strideF1==1 || strideF2==1)
                     rezStride = 1;
                  else
                     rezStride = std::max(strideF1, strideF2);
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
                  break;
               }
               if (op == OP_XNOR)
               {
                  if (strideF1!=0 || strideF2!=0)
                  {
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(0,
                        cerr << "WARNING: stride_slice::slice_out, XNOR operation at pc=0x"
                             << hex << pc << dec 
                             << " found index variable in bitwise operation, resulting in undetermined stride" 
                             << endl;
                     )
#endif
                     rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
                     rezF.set_irregular_access();
                  } else
                  {
                     rezF = GFSliceVal(
                          sliceVal(~(valueNum1^valueNum2), TY_CONSTANT));
                  }
                  if (strideF1==1 || strideF2==1)
                     rezStride = 1;
                  else
                     rezStride = std::max(strideF1, strideF2);
                  if (is_irregular)
                     rezF.set_irregular_access();
                  if (is_indirect==1)
                     rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
                  else if (is_indirect==2)
                     rezF.set_indirect_pcs(_ind_pcs);
                  break;
               }
            }
            // in any other case it is an unknown computation
            rezF = GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
            if (is_irregular || strideF1==1 || strideF2==1)
               rezF.set_irregular_access();
            if (is_indirect==1)
               rezF.set_indirect_access(GUARANTEED_INDIRECT, 0);
            else if (is_indirect==2)
               rezF.set_indirect_pcs(_ind_pcs);
            if (strideF1==1 || strideF2==1)
               rezStride = 1;
            else
               rezStride = std::max(strideF1, strideF2);
            break;
         }
      break;

      case SliceEffect_IMPOSSIBLE:
      // for IMPOSSIBLE we can have a cycle on spill/unspill instructions only
      {
         assert (cycle_addr!=pc || cycle_op_idx!=uop.idx);
         si->formula = new GFSliceVal(sliceVal(1, TY_REGISTER, pc, uop_idx, reg));
         // Formula depends on the return value of a call. I would say that it
         // is GUARANTEED INDIRECT because we cannot slice inside the callee
//         si->formula->set_irregular_access();
         si->formula->set_indirect_access(GUARANTEED_INDIRECT, 0);
         si->cacheKey = sliceKey;
         si->dfsTarget = 0;
         _values.push_back (new RegCache (reg, pc, uop.idx));
         break;
      } // if not a call, then it is a load

      case SliceEffect_HARD:
      {
         if (cycle_addr==pc && cycle_op_idx==uop.idx)
         {
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(5,
               cerr << "   - slice_out: effect HARD, set hard_case to TRUE." << endl;
            )
#endif
            hard_case = true;
         } else
         {
            assert (! "Shouldn't I pick up the formula at the start of SliceOut?");
            RegCache *rftemp = getValueForRegAndDelete(pc, uop.idx);
            if (rftemp == NULL)
            {
               // Inside SliceIn I create a RegCache entry when I cannot find a 
               // corresponding SAVE. Thus, I should always find a non-NULL entry
               // when I execute SliceOut.
               assert(! "Should I even be here?");
            }
            else
            {
               // In SliceIn, I always create a formula, if I find a corresponding SAVE or not.
               // However, I think that SliceOut expects the formula field of the cache to be NULL.
               // I need to make it work with computing the formula inside SliceIn. Perhaps the 
               // formula can be picked up from the cache at the start of the routine, so I do not
               // even come here for the HARD cases.
               //
               // **** See if the top assert fails ****
            }
         }
         break;
      }

      default:
         assert(!"Bad case");
   }
#if USE_OVERRIDE   // try without override
   if (overrideForm2 == pc)
      assert (cycle_addr == pc);
#endif
   if (hard_case)
   {
      assert(! rezF.is_uninitialized());
      if (cycle_addr > 0)
      {
         // it can actually happen to have rezStride==2. It happens in GTC_s and
         // it is legit. If this is not the start of the cycle, then it is fine.
         // Just set the formula for this register that it has strideF=2 and
         // whatever value. When we are going to add this formula with another
         // one that has strideF==1, it will get sorted out at that point. 
//         assert (rezStride==1 || rezStride==0);
         assert (cycle_cnt>0);
         if (cycle_cnt>1)  // index variable has a cycle of more than 
                           // one iteration
         {
#if DEBUG_STATIC_MEMORY
            DEBUG_STMEM(2,
               cerr << "WARNING: stride_slice::slice_out, at pc=0x" << hex << pc
                    << dec << " instruction part of index variable cycle that takes more than one iteration : "
                    << cycle_cnt << ". I ignore this info for now." << endl;
            )
#endif
         }
      }
      if (cycle_addr==pc && cycle_op_idx==uop.idx)
      {
         assert(rezStride == 1);
         assert(si->formula != NULL);
         delete (si->formula);
         // if the formula is 0, it means we do not have a stride formula
         // but current register is constant. So just create a register 
         // formula for it and set is_strideF to 0.
         // NEW 7/21/2006 mgabi: I think this is wrong. We are trying to
         // compute strides here, so if the register is constant with respect
         // to the loop, why set it just as a simple constant formula. 
         // I should set it as a strideF=2 where the stride is zero.
         // Comment this out for now.
#if 0
         if (IsConstantFormula(rezF, valNum, valDen) && valNum==0)
         {
            (*iMarkers)[pc].formula = new GFSliceVal(
                        sliceVal(1, TY_REGISTER, pc, uop_idx, reg, 1));
            (*iMarkers)[pc].is_strideF = 0;
         } else
#endif
         {
            si->formula = new GFSliceVal(rezF);
            si->cacheKey = sliceKey;
            
            // we closed this cycle. Remove the first element from our cycles in progress map
            assert (! sCycles.empty());
            while (!sCycles.empty() && sCycles.begin()->second.cycle_addr==pc &&
                     sCycles.begin()->second.cycle_op_idx==uop.idx)
               sCycles.erase (sCycles.begin());
            
            if (rezDFSTarget==si->dfsIndex)
            {
               si->is_strideF = 2;
               rezDFSTarget = 0;
            }
         }
         
#if USE_OVERRIDE   // try without override
         // final_formula could be different than NULL if this pc was affected
         // by an active overrideForm2 (not for this pc). In such a case, 
         // current pc should be found in overridedPCs. Should I check all these?
         bool cycle_in_cycle = false;
         if (IMARK(pc).final_formula != 0)  // there was one. Check overridePCs
         {
            assert (overrideForm2 && overrideForm2!=pc);
            // our pc should be in overridedPCs. Go and remove it.

            UIList::iterator ulit = overridedPCs.begin ();
            for ( ; ulit!=overridedPCs.end () ; ++ulit)
            {
               addr workpc = *ulit;
               if (workpc == pc)  // found it
               {
                  cycle_in_cycle = true;
                  overridedPCs.erase (ulit);
                  break;
               }
            }
            assert (cycle_in_cycle);  // we should always find it I think
            delete (IMARK(pc).final_formula);
         } 
         // else
         //  the assert is trivial now
         //  assert(IMARK(pc).final_formula == NULL);
         IMARK(pc).final_formula = IMARK(pc).formula;
         IMARK(pc).final_strideF = IMARK(pc).is_strideF;
         cycle_addr = 0;
         cycle_op_idx = 0;

         assert (!overrideForm2 || overrideForm2==pc || cycle_in_cycle);
         if (overrideForm2 == pc)  // non zero and equal to pc (pc cannot be 0)
         {
            overrideForm2 = 0;
            // I should restore formulas for all PCs that have a final formula
            // computed and were affected by overrideForm2. The temporary
            // formulas are needed only up to this point. I need the final
            // formulas as I return back from recursion.
            // There are 2 solutions for this: 
            // 1) everytime I need a register formula, if overrideForm is not
            // set and there is a final formula, use the final formula; 
            // otherwise use the temporary formula. This might be better, but
            // requires checking the entire code to replace the use of formulas
            // with the above test.
            // 2) create a list of instruction PCs that are affected by 
            // overrideForm2 as I go deeper into recurssion. At this place, 
            // when overrideForm is not needed any more, I have to traverse
            // the list and restore the working formulas for those PCs that
            // had a final formula. I guess I add only these ones into the 
            // list, so all instructions found in the list must be restored.
            UIList::iterator ulit = overridedPCs.begin ();
            for ( ; ulit!=overridedPCs.end () ; ++ulit)
            {
               addr workpc = *ulit;
               if (IMARK(workpc).is_strideF==2)  
               // it means we computed a final formula again. Is this even
               // possible? Maybe assert for now and see how it goes.
               // Lol, yes it is possible. I found such a case while another
               // (although related) assert failed. However, I remove the 
               // affected pc from this list when we re-compute a final stride 
               // for it.
               {
                  assert (!"End of Override. Found instruction in the list with formula finalized.");
                  // if this assert fails, check the analyzed code and adjust.
                  // It might be possible that we went on a second cycle
                  // while we were inside one??? If we find a case where
                  // the overriden pc has a strideF==2 formula, maybe we should
                  // make sure that the new formuls is equal with our old
                  // final_formula. Although it can be that the final formula
                  // was overwritten by this new one.
               } else   // I need to restore the final_formula which should
                        // be already computed. Check all facts.
               {
                  if (IMARK(workpc).formula!=NULL)
                     delete (IMARK(workpc).formula);
                  assert (IMARK(workpc).final_strideF==2 && 
                          IMARK(workpc).final_formula);
                  IMARK(workpc).formula = IMARK(workpc).final_formula;
                  IMARK(workpc).is_strideF = 2;
                  IMARK(workpc).cacheKey = sliceKey;
               } 
            }
            // clear the list now
            overridedPCs.clear ();
         }
#endif
      } else
      {
         assert(si->formula == NULL);
         si->formula = new GFSliceVal(rezF);
         si->is_strideF = rezStride;
         si->cacheKey = sliceKey;
      }
      si->dfsTarget = rezDFSTarget;
      _values.push_back(new RegCache(reg, pc, uop.idx, rezDFSTarget));
#if DEBUG_STATIC_MEMORY
      DEBUG_STMEM(5,
         cerr << "StrideSlice::slice_out, reg=" << reg << ", at pc="
              << hex << pc << dec << ", idx=" << uop.idx 
              << " final formula=" << *(si->formula)
              << " , strideF=" << si->is_strideF 
              << ", dfsTarget=" << rezDFSTarget << endl;
      )
#endif
   }
   si->loopCnt = 0;
}

void
StrideSlice::Slice(PrivateCFG::Node *b, int from_uop, const register_info& reg, 
                addrtype _start_pc, addrtype _to_pc)
{
//std::cout<<"OZGURDBG::Discover the Path func:"<<__func__<<" line"<<__LINE__<<std::endl;
   start_be = back_edged;
   start_rank = b->getPathRank();
   
//#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(4,
      cerr << endl << "  ** StrideSlice::Slice, block [0x" << hex << b->getStartAddress()
           << ",0x" << b->getEndAddress() << "], from_uop=" << dec << from_uop
           << dec << ", on reg " << reg << ", start_pc=0x" << hex 
           << _start_pc << dec << ", start_be=" << start_be 
           << ", start_rank=" << start_rank << endl;
   )
//#endif
//std::cout<<"OZGURDBG::Discover the Path func:"<<__func__<<" line"<<__LINE__<<std::endl;

   UopCodeCache *ucc = b->MicroOpCode();
   int num_uops = ucc->NumberOfUops();
   // I should always receive a valid uop index
   assert(from_uop>=0 && from_uop<num_uops);
   const uOp_t& uop = ucc->UopAtIndex(from_uop);
   addrtype pc = uop.dinst->pc;
   SIPCache *iMarkers = &iMarkersLow;
   StrideInst *&si = (*iMarkers)(pc, uop.idx);
   if (!si)
      si = new StrideInst();
   
   si->loopReg = reg;
//   si->loopHb = high_bits;
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(5,
      cerr << "StrideSlice::Slice, set start_pc=0x" << hex << _start_pc << dec 
           << ", start_be=" << start_be 
           << ", start_rank=" << start_rank
           << "; iMark.loopCnt=" << si->loopCnt 
           << ", iMark.loopReg=" << si->loopReg
           << endl;
   )
#endif
   
   BaseSlice::Slice(b, from_uop, reg, _start_pc, _to_pc);
//std::cout<<"OZGURDBG::Discover the Path func:"<<__func__<<" line"<<__LINE__<<std::endl;
}

void
StrideSlice::StartSlice(PrivateCFG::Node *b, int from_uop, const register_info& reg, 
                addrtype _start_pc, addrtype _to_pc)
{
//std::cout<<"OZGURDBG::Discover the Path func:"<<__func__<<" line"<<__LINE__<<std::endl;
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(3,
      cerr << "StrideSlice::StartSlice, block [0x" << hex << b->getStartAddress()
           << ",0x" << b->getEndAddress() << "], from_uop=" << dec << from_uop
           << dec << ", on reg " << reg << ", start_pc=0x" << hex 
           << _start_pc << dec << endl;
   )
#endif
   
   // I should not have any leftover cycles. Test?
   if (!sCycles.empty())
   {
//std::cout<<"OZGURDBG::Discover the Path func:"<<__func__<<" line"<<__LINE__<<std::endl;
       cerr << "WARNING: StrideSlice::start_slice, cycle map is not empty. Previous slice may be incorrect!" << endl;
       sCycles.clear();
   }
   
   // adjust the global dfsCount to 0
   strideDFSCount = 0;
   // get a unique sliceKy
   ++ sliceKey;
   
   // test if we have a precomputed cyclic path that includes the starting block
   // if we do not, then we must clear the current one and compute a new path
   NodeEdgeMap::iterator nemit = incomingEdges.find(b);
   if (nemit == incomingEdges.end())  // did not find it
   {
//std::cout<<"OZGURDBG::Discover the Path func:"<<__func__<<" line"<<__LINE__<<std::endl;
      incomingEdges.clear();
      BuildCyclicPath(b);
   }
//std::cout<<"OZGURDBG::Discover the Path func:"<<__func__<<" line"<<__LINE__<<std::endl;
   
   Slice(b, from_uop, reg, _start_pc, _to_pc);
}


}  /* namespace MIAMI */
