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

using namespace std;
#include "BPatch.h"
#include "BPatch_object.h"
#include "BPatch_addressSpace.h"
#include "BPatch_process.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_point.h"
#include "BPatch_function.h"
#include "BPatch_flowGraph.h"
#include "BPatch_statement.h"

using namespace Dyninst;
#include "Function.h"
#include "Instruction.h"
#include "InstructionDecoder.h"
#include "InstructionSource.h"
#include "slicing.h"


using namespace InstructionAPI;
using namespace DataflowAPI;

#include "CodeObject.h"
#include "CFG.h"
#include "Graph.h"
#include "Symtab.h"
#include "SymEval.h"
#include "DynAST.h"


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


using namespace MIAMI;


//***************************************************************************
// 
//***************************************************************************

void isaXlate_init(const char* prog_name);

int isaXlate_insn(std::string func_name, unsigned long  pc, MIAMI::DecodedInstruction* dInst);


//***************************************************************************
// 
//***************************************************************************

void startCFG(BPatch_function* function,std::map<std::string,std::vector<BPatch_basicBlock*> >& paths,graph& g);

void traverseCFG(BPatch_basicBlock* blk,std::map<BPatch_basicBlock *,bool> seen, std::map<std::string,std::vector<BPatch_basicBlock*> >& paths, std::vector<BPatch_basicBlock*> path, string pathStr, graph& g);
