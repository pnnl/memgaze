/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: base_slice.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements support for backwards slicing on a register name 
 * through a CFG. Enables various data flow analyses to be built on top of 
 * it.
 */

#include <limits.h>
#include "base_slice.h"
#include "debug_miami.h"

using namespace std;

namespace MIAMI
{

void 
BaseSlice::SliceNext(PrivateCFG::Node *b, const register_info& reg)
{
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(4,
      cerr << "BaseSlice::SliceNext, block [0x" << hex << b->getStartAddress()
           << ",0x" << b->getEndAddress() << dec << "], on reg " << reg << endl;
   )
#endif
   if (SliceNextIn(b, reg))
   {
      // There are loops with multiple back-edges. 
      // Sometimes it matters which back-edge I need to take.
      // Should I prefer the one that covers more of the loop?
      // That is, the one that starts from the largest rank??
      // 
      // Solution (for now): Place all edges into a map sorted
      // by the sink node's rank. Try to traverse edges
      // starting from the largest rank.
      typedef std::multimap<int, PrivateCFG::Edge*, std::greater<int> > RankOrderedEdgesMap;
      RankOrderedEdgesMap orderedEdges;
      PrivateCFG::IncomingEdgesIterator ieit(b);
      while ((bool)ieit)
      {
         PrivateCFG::Edge *e = ieit;
         orderedEdges.insert(RankOrderedEdgesMap::value_type(e->source()->getRank(), e));
         ++ ieit;
      }
      for (RankOrderedEdgesMap::iterator reit=orderedEdges.begin() ;
           reit!=orderedEdges.end() ;
           ++ reit)
      {
         PrivateCFG::Edge *e = reit->second;
         if (SliceIn(e, reg))
         {
            PrivateCFG::Node* bp = e->source();
            BaseSlice::Slice(bp, INT_MAX, reg, start_pc, to_pc);
            // I need a mechanism to specify that I do not want to try
            // multiple edges back
            SliceOut(e, reg);
            if (!SliceMultiplePaths(e, reg))
               return;
         }
      }
      // if I did not find any valid incoming edge, setup a register term here
      SliceNextOut(b, reg);
   }
}

void
BaseSlice::Slice(PrivateCFG::Node *b, int from_uop, const register_info& reg, 
                addrtype _start_pc, addrtype _to_pc)
{
   start_pc = _start_pc;
   to_pc = _to_pc;
   
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(4,
      cerr << "BaseSlice::Slice, block [0x" << hex << b->getStartAddress()
           << ",0x" << b->getEndAddress() << "], from_uop=" << dec << from_uop
           << dec << ", on reg " << reg << ", from start_pc=0x" << hex 
           << start_pc << dec << endl;
   )
#endif
   
   UopCodeCache *ucc = b->MicroOpCode();
   int num_uops = ucc->NumberOfUops();
   if (from_uop<0 || from_uop>num_uops) from_uop = num_uops;
//   addrtype from_pc = (from_uop==num_uops ? b->getEndAddress() : 
//                       ucc->UopAtIndex(from_uop).pc);

   for (int iu=from_uop-1 ; iu>=0 ; --iu)
   {
      const uOp_t& uop = ucc->UopAtIndex(iu);
      addrtype pc = uop.dinst->pc;
      
      // check if the uop writes the slice register
      register_info areg;  // actual register (name and bit range)
      if (ShouldSliceUop(uop, reg, areg))
      {
         // I should use the bit range written by the uop, not the
         // bit range of the slicing register (reg name is the same)
         //
         // check if we must continue slicing over the inputs
         if (SliceIn(b, iu, areg))
         {
            // must continue slicing over all the inputs
            RInfoList regs;
            // fetch all source registers in the regs list
            if (mach_inst_src_registers(uop.dinst, uop.iinfo, regs, true))
            {
               RInfoList::iterator rlit = regs.begin();
               for ( ; rlit!=regs.end() ; ++rlit)
               {
                  // use the register_actual_name to map AL, AH, AX, EAX, RAX 
                  // to the same register. Eventually, I will use the actual
                  // bit range information from the registers as well.
                  // TODO
                  // Skip over pseudo registers and flag/status registers
                  //  (how about hardwired?)
                  rlit->name = register_name_actual(rlit->name);
                  if (*rlit!=MIAMI_NO_REG && rlit->type!=RegisterClass_PSEUDO &&
                        ShouldSliceRegister(*rlit, pc))
                  {
                     // if this is a hardwired register, call the function to record
                     // its vale
                     long value = 0;
                     // is_hardwired_register sets value if result is true
                     if (is_hardwired_register(*rlit, uop.dinst, value))
                     {
                        RecordRegisterValue(*rlit, uop, value);
                        continue;
                     }
                     
                     // can we have STACK_OPERATION or STACK_REG inputs??
                     if (rlit->type==RegisterClass_STACK_OPERATION || 
                         rlit->type==RegisterClass_STACK_REG)
                     {
                        fprintf(stderr, "HEY, Found STACK Operation or register as input to sliced uop at pc 0x%" 
                                 PRIxaddr ", reg name %d, reg type %d\n",
                                 pc, rlit->name, rlit->type);
                        assert(!"Just check this case and deal with it. Not a show stopper issue.");
                     }
                     
                     // slice on each input register, except RIP/flags
#if DEBUG_STATIC_MEMORY
                     DEBUG_STMEM(4,
                        cerr << "Continue slicing from pc 0x" << hex << pc << dec
                             << " uop index " << iu << " on register " << *rlit << endl;
                     )
#endif
                     Slice(b, iu, *rlit, pc);
                  }
               }
            }   // if any machine source registers
            
         }
         // then call the corresponding SliceOut
         SliceOut(b, iu, areg);
      }
      if (UopTerminatesSlice(uop, iu, reg, areg))
         return;
   }  // iterate over uops ...
   SliceNext(b, reg);
}

#if HAVE_MEMORY_FORMULAS
bool
BaseSlice::SliceFollowSave(PrivateCFG::Node *b, int from_uop, const GFSliceVal& _formula,
                register_info& spilled_reg)
{
#if DEBUG_STATIC_MEMORY
   DEBUG_STMEM(4,
      cerr << "BaseSlice::SliceFollowSave, block [0x" << hex << b->getStartAddress()
           << ",0x" << b->getEndAddress() << "], from_uop=" << dec << from_uop
           << ", loadFormula=" << _formula << ", old start_pc=0x" 
           << hex << start_pc << dec << endl;
   )
#endif
      
   UopCodeCache *ucc = b->MicroOpCode();
#if DEBUG_STATIC_MEMORY
   addrtype reloc = b->RelocationOffset();
#endif
   
   int num_uops = ucc->NumberOfUops();
   if (from_uop<0 || from_uop>num_uops) from_uop = num_uops;

   spilled_reg = register_info();
   for (int iu=from_uop-1 ; iu>=0 ; --iu)
   {
      const uOp_t& uop = ucc->UopAtIndex(iu);
      addrtype pc = uop.dinst->pc;
     
      const instruction_info *iiobj = uop.iinfo;
      int type = iiobj->type;
      int opidx;
      coeff_t valueNum;
      ucoeff_t valueDen;
      RefFormulas *rf = 0;
      
      // look for a store operation, then find the corresponding dest memory operand
      if (InstrBinIsStoreType((InstrBin)type)    // it is a store operation
          && (opidx=iiobj->first_dest_operand_of_type(OperandType_MEMORY))>=0
          && (rf = refFormulas(pc,opidx))!=0
          && !rf->base.is_uninitialized() // has a computed formula
          && IsConstantFormula(_formula-rf->base, valueNum, valueDen)
          && valueNum == 0   // it stores at the same location
         )
      {
         // Found a memory store at the correct memory location.
         // If it has an input register, the reference is a store
         // (spill), continue slicing from it.
         //
#if DEBUG_STATIC_MEMORY
         DEBUG_STMEM(4,
            cerr << "BaseSlice::SliceFollowSave, found SAVE at uop " << iu 
                 << ", address 0x" << hex << pc << dec << ", store formula: "
                 << rf->base << endl;
         )
#endif
#if DEBUG_STATIC_MEMORY
         DEBUG_STMEM(7,
            debug_decode_instruction_at_pc((void*)(pc+reloc), uop.dinst->len);
            DumpInstrList(uop.dinst);
         )
#endif

         // a store should have only 1 source operand
         assert (iiobj->num_src_operands == 1);
         OperandType srcoptype = (OperandType)extract_op_type(iiobj->src_opd[0]);
         int srcopidx = extract_op_index(iiobj->src_opd[0]);
         bool is_reg = true;
         register_info srcreg;
         int64_t imm_value = 0;
         if (srcoptype == OperandType_INTERNAL)
         {
            int rwidth = iiobj->width;
            if (iiobj->exec_unit==ExecUnit_VECTOR)
               rwidth *= iiobj->vec_len;
            srcreg = register_info(srcopidx, RegisterClass_TEMP_REG, 0, rwidth-1);
         } else if (srcoptype == OperandType_REGISTER)
         {
            srcreg = mach_inst_reg_operand(uop.dinst, srcopidx);
            srcreg.name = register_name_actual(srcreg.name);
            
            long value;
            // can we have hardwired (e.g. IP) registers as input to save??
            // Answer: we can have imm values, so it is not a stretch to deal
            // with hardwired registers as well.
            if (is_hardwired_register(srcreg, uop.dinst, value))
            {
               imm_value = value;
               is_reg = false;
            }
            // can we have PSEUDO, STACK_OPERATION or STACK_REG inputs??
            if (srcreg.type==RegisterClass_STACK_OPERATION || 
                srcreg.type==RegisterClass_STACK_REG ||
                srcreg.type==RegisterClass_PSEUDO)
            {
               cerr << "HEY, Found PSEUDO or STACK Operation or register " << srcreg 
                    << " as input to SAVE at pc 0x" << hex << pc << dec << endl;
               assert(!"Just check this case and deal with it. Probably not a show stopper issue.");
            }
         } else
         {
            assert (srcoptype == OperandType_IMMED);
            imm_value = iiobj->imm_values[srcopidx].value.s;
            is_reg = false;
         }
         
         if (is_reg)
         {
            Slice(b, iu, srcreg, pc);
         } else
         {
            // it is an immediate value; how do we pass the value back to the caller??
            // make a fake temporary register
            int rwidth = iiobj->width;
            if (iiobj->exec_unit==ExecUnit_VECTOR)
               rwidth *= iiobj->vec_len;
            srcreg = register_info(srcopidx+100, RegisterClass_TEMP_REG, 0, rwidth-1);
            RecordRegisterValue(srcreg, uop, imm_value);
         }
         spilled_reg = srcreg;
         return (true);
         
      }  // if this is a store at the same address ...
   }  // iterate over uops ...

   if (SliceNextIn(b, spilled_reg))
   {
      // Did not find a save in this block, try following one of the edges
      PrivateCFG::IncomingEdgesIterator ieit(b);
      while ((bool)ieit)
      {
         PrivateCFG::Edge *e = ieit;
         register_info ri;
         if (SliceIn(e, ri))
         {
            PrivateCFG::Node* bp = e->source();
            bool res = SliceFollowSave(bp, INT_MAX, _formula, spilled_reg);
            // I need a mechanism to specify that I do not want to try
            // multiple edges back
            SliceOut(e, ri);
            if (res) 
               return (true);
            // if we did not find a spill on this path, check if we
            // should try multiple paths
            if (!SliceMultiplePaths(e, ri))
               return (false);  // res == false
         }
         ++ ieit;
      }
      // if I did not find any valid incoming edge, setup a register term here
      SliceNextOut(b, spilled_reg);
   }  // if (SliceNextIn)
   return (false);
}
#endif


SliceEffect_t 
BaseSlice::SliceEffect(const PrivateCFG::Node *b, const uOp_t& uop, const register_info& reg)
{
   // check if this is an interprocedural call (I must have a call surrogate block after)
   // which makes it an IMPOSSIBLE case (no inter-procedural analysis)
   if ( (InstrBinIsBranchType(uop.iinfo->type) && uop.dinst->is_call)
         // should I classify conditional moves as having IMPOSSIBLE slice effect?
         // They could be considered to having the effect of branches, creating
         // distinct paths that lead to the creation of a value.
         // We take only one CFG path back to reconstruct the value.
//        || mach_inst_performs_conditional_move(uop.dinst, uop.iinfo)
      )
      return (SliceEffect_IMPOSSIBLE);
   else if (InstrBinIsLoadType(uop.iinfo->type))
      return (SliceEffect_HARD);
   
   // Next, count the number of input operands registers that would have to be sliced.
   // We exclude flag and status registers.
   // If count == 0    => EASY
   //    count > 0     => NORMAL
   //    load uop      => HARD
   //    function call => IMPOSSIBLE
   RInfoList regs;
   int count = 0;
   // fetch all source registers in the regs list
   // I must include INTERNAL registers as well here ...
   if (mach_inst_src_registers(uop.dinst, uop.iinfo, regs, true))
   {
      RInfoList::iterator rlit = regs.begin();
      for ( ; rlit!=regs.end() ; ++rlit)
      {
#if TRACE_REFERENCE_SLICE
         cerr << "Found source reg " << *rlit;
#endif
         // use the register_actual_name to map AL, AH, AX, EAX, RAX 
         // to the same register. Eventually, I will use the actual
         // bit range information from the registers as well.
         // TODO
         // Skip over pseudo and hardwired registers
         rlit->name = register_name_actual(rlit->name);
         if (*rlit!=MIAMI_NO_REG && rlit->type!=RegisterClass_PSEUDO &&
             !is_flag_or_status_register(*rlit))
//             ShouldSliceRegister(*rlit, uop.pc))
         {
            // can we have STACK_OPERATION or STACK_REG inputs??
            if (rlit->type==RegisterClass_STACK_OPERATION || 
                rlit->type==RegisterClass_STACK_REG)
            {
               fprintf(stderr, "HEY, Found STACK Operation or register as input in SliceEffect at pc 0x%" 
                         PRIxaddr ", reg name %d, reg type %d\n",
                         uop.dinst->pc, rlit->name, rlit->type);
//               assert(!"Just check this case and deal with it.");
            }
            
            // count the other registers
            count += 1;
#if TRACE_REFERENCE_SLICE
            cerr << " which is sliceable " << count;
#endif
         }
#if TRACE_REFERENCE_SLICE
         cerr << endl;
#endif
      }
   }   // if any machine source registers
   if (count>0)
      return (SliceEffect_NORMAL);
   else
   { 
      //OZGUR FIXME:instruction not sure what it was new code{
    //  if(InstrBinIsRegisterMove(uop.iinfo->type))
    //     return (SliceEffect_EASY);
    //  else
    //     return (SliceEffect_NORMAL);
    //}
      return (SliceEffect_EASY);
   }
}


}  /* namespace MIAMI */
