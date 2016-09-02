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

// Copied directly from DyninstHelper.h
struct graph{
        std::map<std::string, std::string> declarations;
        std::map<std::string, std::string> lineNos;
        std::set<std::string> links;
        std::map<std::string,int> elements;
        std::map<int,BPatch_basicBlock*> blks;
        std::map<std::string,std::string> loops;
        std::map<std::string,std::string> loopEntries;
        std::string libPath;
        std::map<int,int> basicBlockNoMap; // dyninst provide blk numbers arent gauranteed to be the same between runs, so use our own numbers
        std::map<std::string,std::vector<std::pair<std::string, BPatch_variableExpr*> > >varMap;
        std::map<std::string,long long> counts;
        std::map<int,std::string> calledFunctionMap;
        int entry;
        int exit;
        int cnt;
};


//***************************************************************************
// 
//***************************************************************************

//MIAMI::LoadModule* dyninst_note_loadModule(int count, std::string file_name, uint32_t hashKey);

void dyninst_note_loadModule(uint32_t id, std::string& file_name, addrtype start_addr, addrtype low_offset);

//***************************************************************************

void dyninst_note_routine(MIAMI::LoadModule* lm, int i);

//***************************************************************************

//void isaXlate_init(const char* prog_name);

int isaXlate_insn(unsigned long  pc, MIAMI::DecodedInstruction* dInst);


//***************************************************************************
// 
//***************************************************************************

