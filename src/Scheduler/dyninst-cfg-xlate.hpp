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

#include "BPatch.h"
#include "BPatch_flowGraph.h"
#include "BPatch_object.h"

using namespace Dyninst;
#include "Function.h"
#include "Instruction.h"
#include "InstructionDecoder.h"
#include "InstructionSource.h"

using namespace InstructionAPI;
using namespace DataflowAPI;
using namespace ParseAPI;
using namespace PatchAPI;

#include "CodeObject.h"
#include "CFG.h"
#include "load_module.h"
#include "Graph.h"
#include "Symtab.h"
#include "SymEval.h"
#include "DynAST.h"
using namespace MIAMI;

#include "instr_info.H"

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
