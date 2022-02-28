/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: math_routines.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Support for Fortran intrinsics.
 */

#ifndef _MATH_ROUTINES_H
#define _MATH_ROUTINES_H

namespace MIAMI_DG
{

/* these values may need to be adjusted for the move to 64-bit OSes.
 * I cannot remember what they represent. EntryValue will not be used
 * initially. We can see all the executed user code, including dynamic
 * libraries, so it may not be needed to treat Fortran intrinsics differently.
 */
#define INTRINSIC_REG_SHIFT  27
#define INTRINSIC_REG_MASK   0x1f
#define INTRINSIC_PC_MASK    0x07ffffff

// Machine independent header file for inclusion in SchedDG
class EntryValue
{
public:
   EntryValue (const char* in_regs, const char* out_regs, const char* _name);
   
   ~EntryValue ();
   
   const char* intrinsic_name () const { return (rname); }
   
   int spec_inst_src (int idx)
   {
      if (!gpSrcRegs || idx>=gpSrcRegs[0])
         return (-1);
      else
         return (gpSrcRegs[idx+1]);
   }
   
   int spec_inst_fp_src (int idx)
   {
      if (!fpSrcRegs || idx>=fpSrcRegs[0])
         return (-1);
      else
         return (fpSrcRegs[idx+1]);
   }
   
   int spec_inst_dest (int idx)
   {
      if (!gpDestRegs || idx>=gpDestRegs[0])
         return (-1);
      else
         return (gpDestRegs[idx+1]);
   }
   
   int spec_inst_fp_dest (int idx)
   {
      if (!fpDestRegs || idx>=fpDestRegs[0])
         return (-1);
      else
         return (fpDestRegs[idx+1]);
   }
   
   int spec_inst_ref_src (int idx)
   {
      if (!refSrcRegs || idx>=refSrcRegs[0])
         return (-1);
      else
         return (refSrcRegs[idx+1]);
   }
   
   int spec_inst_ref_dest (int idx)
   {
      if (!refDestRegs || idx>=refDestRegs[0])
         return (-1);
      else
         return (refDestRegs[idx+1]);
   }
   
private:
   void parse_registers (const char *in_regs, int*& gpRegsArray, 
             int*&fpRegsArray, int*&refRegsArray);
   
   int* gpSrcRegs;
   int* gpDestRegs;
   int* fpSrcRegs;
   int* fpDestRegs;
   int* refSrcRegs;
   int* refDestRegs;
   const char* rname;
};

} /* namespace MIAMI_DG */

#endif
