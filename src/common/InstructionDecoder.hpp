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

class InstructionDecoder {
public:

  // TODO: subsume instruction-decoding...
  
  static int
  decode(unsigned long pc, int len,
	 MIAMI::DecodedInstruction* dInst);

  static int
  decode_dbg(unsigned long pc, int len,
	     MIAMI::DecodedInstruction* dInst,
	     const char* ctxt = NULL);
  
  static int
  decode_dbg(unsigned long pc, int len,
	     MIAMI::DecodedInstruction* dInst,
	     MIAMI::DecodedInstruction* dInst1 = NULL,
	     const char* ctxt = NULL);

  static int
  decode_dyninst(unsigned long pc, int len,
		 MIAMI::DecodedInstruction* dInst,
		 BPatch_function* dyn_func,
		 BPatch_basicBlock* dyn_blk);
  
};
