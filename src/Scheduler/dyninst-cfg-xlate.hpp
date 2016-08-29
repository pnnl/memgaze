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
using namespace std;

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

void dyninst_CFG(char* module_name, MIAMI::LoadModule **myimg);
unsigned long get_low_offset(std::vector<BPatch_object*> * objs, std::string file_name);
MIAMI::Routine* create_routine(MIAMI::LoadModule* lm, int i);
int dyninst_build_CFG(MIAMI::CFG* cfg, std::string func_name);
int get_routine_number();
MIAMI::LoadModule* create_loadModule(int count, std::string file_name);
std::vector<unsigned long> get_instructions_address_from_block(MIAMI::CFG::Node *b);
