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
#include "instruction_decoding_dyninst.h"

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
			    MIAMI::DecodedInstruction* dInst,
			    const char* ctxt)
{
  using std::cerr;

  std::string ctxt_str = (ctxt) ? ((std::string)ctxt + ":") : "";

  void* pc_ptr = (void*)pc;

  cerr << "**********************************************************\n"
       << ctxt_str << "InstructionXlate::xlate(" << pc_ptr << ")\n\n";

  MIAMI::dump_instruction_at_pc(pc_ptr, len);
  std::cout << "\n";

  int ret = MIAMI::decode_instruction_at_pc(pc_ptr, len, dInst);
  if (ret < 0) { } // error while decoding
  
  DumpInstrList(dInst);
  cerr << "\n";

  return ret;
}

int 
InstructionXlate::xlate_dyninst(unsigned long pc, int len, 
          MIAMI::DecodedInstruction* dInst, 
          BPatch_function* f,
          BPatch_basicBlock* blk){

  void* pc_ptr = (void*)pc;

  int ret = dyninst_decode_instruction_at_pc(pc_ptr,len,dInst,f,blk);
  
}


int
InstructionXlate::xlate_dbg(unsigned long pc, int len,
			    MIAMI::DecodedInstruction* dInst,
			    MIAMI::DecodedInstruction* dInst1,
			    const char* ctxt)
{
  int ret = 0;

#if 0
  if (dInst1) {
    DumpInstrList(dInst1);
  }
#endif

  return ret;
}
