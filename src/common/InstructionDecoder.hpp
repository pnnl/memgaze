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

#ifndef _MIAMI_InstructionDecoder_hpp_
#define _MIAMI_InstructionDecoder_hpp_

//***************************************************************************
// system includes
//***************************************************************************

//***************************************************************************
// MIAMI includes
//***************************************************************************

#include "instr_info.H"

//***************************************************************************
// DynInst includes
//***************************************************************************

#if 0
#include <BPatch.h>
#endif

//***************************************************************************
// 
//***************************************************************************

namespace MIAMI
{

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

#if 0
  static int
  decode_dyninst(unsigned long pc, int len,
		 MIAMI::DecodedInstruction* dInst,
		 BPatch_function* dyn_func,
		 BPatch_basicBlock* dyn_blk);
#endif

};

} /* namespace MIAMI */


//***************************************************************************
// 
//***************************************************************************

#include "instruction_decoding.h"

//***************************************************************************

#endif // _MIAMI_InstructionDecoder_hpp_
