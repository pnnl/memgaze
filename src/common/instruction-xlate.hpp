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
// system
//***************************************************************************

//***************************************************************************
// DynInst
//***************************************************************************

//***************************************************************************
// 
//***************************************************************************

#include "instr_info.H"

#include <BPatch.h>

//***************************************************************************
// 
//***************************************************************************

class InstructionXlate {
public:

  // TODO: subsume instruction-decoding...
  
  static int xlate(unsigned long pc, int len, MIAMI::DecodedInstruction* dInst);

  static int xlate_dbg(unsigned long pc, int len,
		       MIAMI::DecodedInstruction* dInst,
		       const char* ctxt = NULL);
  static int xlate_dyninst(unsigned long pc, int len, 
               MIAMI::DecodedInstruction* dInst,
               BPatch_function* f,
               BPatch_basicBlock* blk);

  static int xlate_dbg(unsigned long pc, int len,
		       MIAMI::DecodedInstruction* dInst,
		       MIAMI::DecodedInstruction* dInst1 = NULL,
		       const char* ctxt = NULL);
  
};
