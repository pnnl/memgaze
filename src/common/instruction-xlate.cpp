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
// 
//***************************************************************************

#include "instruction-xlate.hpp"

#include "instruction_decoding.h"

//***************************************************************************
// 
//***************************************************************************

int
InstructionXlate::xlate(unsigned long pc, int len,
			MIAMI::DecodedInstruction* dInst)
{
  void* pc_ptr = (void*)pc;
  return MIAMI::decode_instruction_at_pc(pc_ptr, len, dInst);
}


int
InstructionXlate::xlate_dbg(unsigned long pc, int len,
			    MIAMI::DecodedInstruction* dInst)
{
  using std::cerr;

  void* pc_ptr = (void*)pc;
  
  cerr << "**********************************************************\n"
       << "InstructionXlate::xlate(" << (void*)pc << ")\n\n";

  MIAMI::dump_instruction_at_pc(pc_ptr, len);
  std::cout << "\n";

  int ret = MIAMI::decode_instruction_at_pc(pc_ptr, len, dInst);
  if (ret < 0) { } // error while decoding
  
  DumpInstrList(dInst);
  cerr << "\n";

  return ret;
}


int
InstructionXlate::xlate_dbg(unsigned long pc, int len,
			    MIAMI::DecodedInstruction* dInst,
			    MIAMI::DecodedInstruction* dInst1)
{
  int ret = 0;

#if 0
  if (dInst1) {
    DumpInstrList(dInst1);
  }
#endif

  return ret;
}
