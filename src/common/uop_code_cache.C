/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: uop_code_cache.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Decode the native instructions of a basic block into their
 * corresponding micro-operations, and provide direct, linear access to all 
 * the micro-ops in a block.
 */

#include <assert.h>
#include <iostream>
#include "uop_code_cache.h"
#include "debug_miami.h"

using namespace std;

namespace MIAMI
{

uOp_t UopCodeCache::invalid_uOp(0, 0, 0);

/* check if this micro-op writes the given register.
 * @full specifies if the register's entire bit range must be written
 * @ari  return the actual register (bit range) written by the instruction
 */
bool 
uOp_t::Writes(const register_info& reg, bool full, register_info& ari) const
{
   RInfoList regs;
   // fetch all destination registers in the regs list
   if (mach_inst_dest_registers(dinst, iinfo, regs, true))
   {
      // if this is a function call, I must also append any return registers
      // how to check that it is a function call??
      if (dinst->is_call && InstrBinIsBranchType(iinfo->type))
      {
         mach_inst_return_registers(dinst, iinfo, regs, true);  // append them to regs
#if DEBUG_INST_DECODING
         DEBUG_IDECODE(5,
            cerr << "Appended return registers to uop Writes." << endl;
         )
#endif
      }
      
      RInfoList::iterator rlit = regs.begin();
      for ( ; rlit!=regs.end() ; ++rlit)
      {
         // use the register_actual_name to map AL, AH, AX, EAX, RAX 
         // to the same register. Eventually, I will use the actual
         // bit range information from the registers as well.
         // TODO
         // Skip over pseudo registers
         rlit->name = register_name_actual(rlit->name);
         if (*rlit!=MIAMI_NO_REG && rlit->type!=RegisterClass_PSEUDO)
         {
            long value;
            // can we have hardwired (e.g. IP) registers as output registers?
            // of course, control transfer instructions modify the RIP register 
            // which I consider to be hardwired, so exclude RIP from test
            if (is_hardwired_register(*rlit, dinst, value) && 
                    !is_instruction_pointer_register(*rlit))
            {
               cerr << "HEY, Found hardwired register " << *rlit << " with value " << value
                    << " as uOp output at pc 0x" << hex << dinst->pc << dec << endl;
               assert(!"Just check this case and deal with it. Not a show stopper issue.");
            }
            // can we have STACK_OPERATION or STACK_REG inputs??
            if (rlit->type==RegisterClass_STACK_OPERATION)
//                || rlit->type==RegisterClass_STACK_REG)
            {
               fprintf(stderr, "HEY, Found STACK Operation or register as uOp output at pc 0x%" 
                            PRIxaddr ", reg name %d, reg type %d\n",
                            dinst->pc, rlit->name, rlit->type);
               assert(!"Just check this case and deal with it. Not a show stopper issue.");
            }
                     
            // if we are here, check if we are writing the currently sliced register
            if (reg.Overlaps(*rlit))  // it is a match
            {
               // before continuing, check if this is only a partial match
               // and print a warning (possibly assert to catch my attention) if so.
               // Solve any such assert once it fails on a concrete case, or 
               // comment it out at that point.
               if (reg!=*rlit) {
#if DEBUG_INST_DECODING
                  DEBUG_IDECODE(3,
                     std::cerr << "In uOp_t::Writes, uop at pc 0x" << hex << dinst->pc << dec << " index " 
                          << idx << " found overlapping registers, but not a perfect match."
                          << " How should we deal with such cases?" << std::endl
                          << "sliced REG=" << reg << " and found output reg=" << *rlit << std::endl;
                  )
#endif
                  // I found a "xor ebx, ebx" and then a use of %rbx. %rbx is 64 bit, %ebx is 32 bit.
                  // Does the xor changes to 0 all the bits? I do not know.
//                  assert (!"Anything that we should do about this??");
               }
               ari = *rlit;
               if (!full || reg.isIncludedInRange(*rlit))
                  return (true);
            }
         }
      }  // iterate over dest registers
   }   // if any output machine registers 
   return (false);  // not found
}
   
/* check if this micro-op reads the given register.
 * @full specifies if the read bit range is fully included in the register.
 * @ari  return the actual register (bit range) read by the instruction
 */
bool 
uOp_t::Reads(const register_info& reg, bool full, register_info& ari) const
{
   RInfoList regs;
   // fetch all source registers in the regs list
   if (mach_inst_src_registers(dinst, iinfo, regs, true))
   {
      RInfoList::iterator rlit = regs.begin();
      for ( ; rlit!=regs.end() ; ++rlit)
      {
         // use the register_actual_name to map AL, AH, AX, EAX, RAX 
         // to the same register. Eventually, I will use the actual
         // bit range information from the registers as well.
         // TODO
         // Skip over pseudo registers
         rlit->name = register_name_actual(rlit->name);
         if (*rlit!=MIAMI_NO_REG && rlit->type!=RegisterClass_PSEUDO)
         {
            long value;
            // can we have hardwired (e.g. IP) registers as input registers?
            if (is_hardwired_register(*rlit, dinst, value))
            {
               cerr << "HEY, Found hardwired register " << *rlit << " with value " << value
                    << " as uOp input at pc 0x" << hex << dinst->pc << dec << endl;
               // This assert and check are likely unnecessary because hardwired 
               // registers are used all the time to initialize memory or other
               // registers. Remove it after evaluating some concrete cases.
//               assert(!"Just check this case and deal with it. Not a show stopper issue.");
            }
            // can we have STACK_OPERATION or STACK_REG inputs??
            if (rlit->type==RegisterClass_STACK_OPERATION || 
                rlit->type==RegisterClass_STACK_REG)
            {
               fprintf(stderr, "HEY, Found STACK Operation or register as uOp input at pc 0x%" 
                            PRIxaddr ", reg name %d, reg type %d\n",
                            dinst->pc, rlit->name, rlit->type);
               assert(!"Just check this case and deal with it. Not a show stopper issue.");
            }
                     
            // if we are here, check if we are writing the currently sliced register
            if (reg.Overlaps(*rlit))  // it is a match
            {
               ari = *rlit;
               // before continuing, check if this is only a partial match
               // and print a warning (possibly assert to catch my attention) if so.
               // Solve any such assert once it fails on a concrete case, or 
               // comment it out at that point.
               if (reg!=*rlit) {
#if DEBUG_INST_DECODING
                  DEBUG_IDECODE(3,
                     std::cerr << "In uOp_t::Reads, uop at pc 0x" << hex << dinst->pc << dec << " index " 
                          << idx << "found overlapping registers, but not a perfect match."
                          << " How should we deal with such cases?" << std::endl
                          << "sliced REG=" << reg << " and found output reg=" << *rlit << std::endl;
                  )
#endif
//                  assert (!"Anything that we should do about this??");
               }
               if (!full || reg.IncludesRange(*rlit))
                  return (true);
            }
         }
      }  // iterate over src registers
   }   // if any output machine registers 
   return (false);  // not found
}

/* Return the index of the memory operand used by this micro-op.
 * If the uop is not a memory operation, return -1
 */
int 
uOp_t::getMemoryOperandIndex() const
{
   return (iinfo->get_memory_operand_index());
}

/* Implement the Uop code cache class.
 */
UopCodeCache::UopCodeCache(addrtype _start, addrtype _end, addrtype _reloc, bool has_call) :
     start_pc(_start), end_pc(_end), reloc(_reloc)
{
   int i;
   addrtype pc;
   for (pc=start_pc ; pc<end_pc ; )
   {
      DecodedInstruction *di = new DecodedInstruction();
      int res = decode_instruction_at_pc((void*)(pc+reloc), end_pc-pc, di);
      if (res < 0)  // error while decoding
      {
         fprintf (stderr, "Error code %d in UopCodeCache::UopCodeCache while decoding instruction at 0x%" PRIxaddr 
                   " (0x%" PRIxaddr "), bytes to block end %d\n",
               res, pc, pc+reloc, (int)(end_pc-pc));
         break;
      }
      
      di->pc -= reloc;   // store the non-relocated address
      dinsts.push_back(di);
      // one native instruction can be decoded into multiple micro-ops
      // iterate over them
      InstrList::iterator lit = di->micro_ops.begin();
      for (i=0 ; lit!=di->micro_ops.end() ; ++lit, ++i)
      {
         instruction_info* iiobj = static_cast<instruction_info*>(&(*lit));
         if (has_call && di->pc+di->len==end_pc && InstrBinIsBranchType(iiobj->type))
            di->is_call = true;
         uops.push_back(uOp_t(iiobj, di, i));
      }

      pc += di->len;
   }
}

UopCodeCache::UopCodeCache()   // just initialize, no decodig
{
   start_pc = end_pc = 0; reloc = 0;
}

UopCodeCache::~UopCodeCache()  // deallocate the decoded instructions
{
   // deallocate all DecodedInstruction objects
   DInstList::iterator dit = dinsts.begin();
   for ( ; dit!=dinsts.end() ; ++dit)
      delete (*dit);
   dinsts.clear();
   uops.clear();
}

/* decode one instruction starting at address _pc, add the discovered 
 * micro-ops to the array (update start and end pcs if needed),
 * and return the length of the decoded instruction, or -1 in case of error.
 */
int 
UopCodeCache::DecodeAtPC(addrtype pc, int len)
{
   int i;
   DecodedInstruction *di = new DecodedInstruction();
   int res = decode_instruction_at_pc((void*)pc, len, di);
   if (res < 0)  // error while decoding
   {
      fprintf (stderr, "Error code %d in UopCodeCache::DecodeAtPC while decoding instruction at 0x%" PRIxaddr ", len %d\n",
            res, pc, len);
      return (-1);
   }
      
   dinsts.push_back(di);
   // one native instruction can be decoded into multiple micro-ops
   // iterate over them
   InstrList::iterator lit = di->micro_ops.begin();
   for (i=0 ; lit!=di->micro_ops.end() ; ++lit, ++i)
   {
      instruction_info* iiobj = static_cast<instruction_info*>(&(*lit));
      uops.push_back(uOp_t(iiobj, di, i));
   }
   if (start_pc==0 || start_pc>pc)
      start_pc = pc;
   if (end_pc==0 || end_pc<pc+di->len)
      end_pc = pc + di->len;

   return (di->len);
}

}  /* namespace MIAMI */
