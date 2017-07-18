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

#include <stdint.h>
#include <fnmatch.h>

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iterator>
#include <stack>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>


//***************************************************************************
// 
//***************************************************************************

#include "BPatch.h"
#include "BPatch_object.h"
#include "BPatch_addressSpace.h"
#include "BPatch_process.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_point.h"
#include "BPatch_function.h"
#include "BPatch_flowGraph.h"
#include "BPatch_statement.h"

#include "Function.h"
#include "Instruction.h"
#include "InstructionDecoder.h"
#include "InstructionSource.h"
#include "slicing.h"

using namespace Dyninst;
using namespace InstructionAPI;
using namespace DataflowAPI;
using namespace ParseAPI;
using namespace PatchAPI;


//***************************************************************************

#include "load_module.h"
#include "routine.h"
#include "CodeObject.h"
#include "CFG.h"
#include "Graph.h"
#include "Symtab.h"
#include "SymEval.h"
#include "DynAST.h"

using namespace MIAMI;

//***************************************************************************
// 
//***************************************************************************


int isaXlate_insn(void* pc, MIAMI::DecodedInstruction* dInst);


//***************************************************************************
// 
//***************************************************************************

