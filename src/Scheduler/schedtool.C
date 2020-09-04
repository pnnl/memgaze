/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: schedtool.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the control management logic for the MIAMI post-
 * processing tool. The tool uses PIN to load binaries to memory. It then 
 * uses XED to decode the instructions from the native binary to convert 
 * them to their MIAMI intermediate representations. MIAMI combines
 * information from static binary analysis with dynamic analysis results 
 * from the MIAMI profilers to estimate execution cost, understand 
 * performance bottlenecks and identify opportunities for tuning.
 */

#include <argp.h>

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <assert.h>

#include "miami_options.h"
#include "MiamiDriver.h"
#include "debug_scheduler.h"
#include "miami_utils.h"

#define KILO 1024
#define MEGA (KILO*KILO)
#define GIGA (KILO*MEGA)

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */
/*
*/

// crating constant variables for argument parser 
const int outputFile = 'o';
const int noPid = 900;
const int machineFile = 'm';// "m", "", "specify machine description file (computes instruction schedule if specified)");
const int persistentDB = 901; // "db", "", "name of a directory to use as a persistent storage of static analysis results");
const int memoryLatency = 'L'; //"L", "", "specify name of file containing memory latency results, default: none");
const int streamReuse = 'S'; //"S", "", "specify name of file containing stream reuse results, default: none");
const int includeFile = 'I'; //"I", "", "only analyze routines whose names are listed in the specified file, default: none");
const int excludeFile = 'X'; //"X", "", "do not analyze routines whose names are listed in the specified file, default: none");
const int routineName = 'r'; //"r", "", "specify routine name for which to dump CFG in dot format, default: none");
const int numberParts = 'p'; //"p", "1", "number of parts to split a routine's CFG into for drawing in dot format. Useful for very large routines.");
const int cfgFile = 'c'; //"c", "", "read CFG information from specified file (required)"); 
const int mrdFiles = 902; //"mrd", "", "specify a memory reuse distance file for data reuse analysis. This switch can be used more than once to pass multiple MRD files.");
const int threshold = 'f'; //"f", "0.0", "threshold to use for filtering program scopes. Only output program scopes that contribute more than the specified fraction of at least one performance metric.");
const int detailedMetrics = 903; //"detailed", "0", "detailed performance database: output additional performance metrics, see manual for the list of affected metrics");
const int noScheduling = 904; //"no_scheduling", "0", "do not compute instruction schedules even if a machine model is provided");
const int skipLineMap = 905; //"no_linemap", "0", "do not read line mapping information, do not map scopes to source files");
const int noScopes = 906; //"no_scopes", "0", "do not present information at scope level; do not compute scope tree");
const int instructionMix = 907; //"imix", "0", "compute/report instruction mix information");
const int instructionWidthMix = 908; //"iwmix", "0", "compute/report instruction/width mix information");
const int scopeName = 's'; //"s", "", "specify scope name for which to dump imix data in CSV format, default: all");
const int generateXML = 909; //"xml", "0", "generate XML output in hpcviewer format. Enabled by default only if a machine file is provided and scheduling is performed.");
const int binaryPath = 910; //"bin_path", "", "binary to analyze (required).");
const int funcName = 911; //"func", "", "function to analyze (required).");
const int blkPath = 912; //"blk_path", "", "specify basic blocks to analyze.");
const int latPath = 913; //"lat_path", "", "path containing instruction level load latency");
const int dumpFile = 914; //"dump_file", ""
const int fpPath = 915; //"lat_path", "", "path containing instruction level data access levels");
const int palm_cfgFile = 916; //"palm_cfg", "", "read palm CFG information from specified file (required)");
const int load_classes = 917; //"load_class", "", "does load classification for every function if --func not specified"
const int inst_loads = 918; //"inst_loads", "", "instrument only load instructions Default = 1"
const int inst_stores = 919; //"inst_stores", "", "instrument only store instructions Default = 1"
const int inst_strided = 920; //"inst_strided", "", "instrument only strided instructions Default = 1"
const int inst_indirect = 921; //"inst_indirect", "", "instrument only indirect instructions Default = 1"
const int inst_frame = 922; //"inst_frame", "", "instrument only frame/constant instructions Default = 0"
const int linemap = 923; //"linemap", "", "specify instumented binary to print linemap.");


 
std::string KnobOutputFile = ""; //  "o", "", "specify output file name prefix, default: execname-machname");
bool KnobNoPid = 0; // "no_pid", "0", "do not add process PID to output file name");
std::string KnobMachineFile = "";// "m", "", "specify machine description file (computes instruction schedule if specified)");
std::string KnobPersistentDB = ""; // "db", "", "name of a directory to use as a persistent storage of static analysis results");
std::string KnobMemoryLatency = ""; //"L", "", "specify name of file containing memory latency results, default: none");
std::string KnobStreamReuse = ""; //"S", "", "specify name of file containing stream reuse results, default: none");
std::string KnobIncludeFile = ""; //"I", "", "only analyze routines whose names are listed in the specified file, default: none");
std::string KnobExcludeFile = ""; //"X", "", "do not analyze routines whose names are listed in the specified file, default: none");
std::string KnobRoutineName = ""; //"r", "", "specify routine name for which to dump CFG in dot format, default: none");
int KnobNumberParts = 1; //"p", "1", "number of parts to split a routine's CFG into for drawing in dot format. Useful for very large routines.");
std::string KnobCfgFile = ""; //"c", "", "read CFG information from specified file (required)");
std::string KnobPalmCfgFile = ""; //"palm_cfg", "", "read palm CFG information from specified file (required)");
std::string KnobMrdFiles = ""; //"mrd", "", "specify a memory reuse distance file for data reuse analysis. This switch can be used more than once to pass multiple MRD files.");

double KnobThreshold = 0.0; //"f", "0.0", "threshold to use for filtering program scopes. Only output program scopes that contribute more than the specified fraction of at least one performance metric.");

bool   KnobDetailedMetrics = 0; //"detailed", "0", "detailed performance database: output additional performance metrics, see manual for the list of affected metrics");
bool   KnobNoScheduling = 0; //"no_scheduling", "0", "do not compute instruction schedules even if a machine model is provided");
bool   KnobSkipLineMap = 0; //"no_linemap", "0", "do not read line mapping information, do not map scopes to source files");
bool   KnobNoScopes = 0; //"no_scopes", "0", "do not present information at scope level; do not compute scope tree");
bool   KnobInstructionMix = 0; //"imix", "0", "compute/report instruction mix information");
bool   KnobInstructionWidthMix = 0; //"iwmix", "0", "compute/report instruction/width mix information");
std::string KnobScopeName = ""; //"s", "", "specify scope name for which to dump imix data in CSV format, default: all");
bool   KnobGenerateXML = 0; //"xml", "0", "generate XML output in hpcviewer format. Enabled by default only if a machine file is provided and scheduling is performed.");
bool KnobLoad_classes = 0; //"load_class", "", "does load classification for every function if --func not specified"
bool KnobInst_loads = 1; //"inst_loads", "", "instrument only load instructions Default = 1"
bool KnobInst_stores = 1; //"inst_stores", "", "instrument only store instructions Default = 1"
bool KnobInst_strided = 1; //"inst_strided", "", "instrument only strided instructions Default = 1"
bool KnobInst_indirect = 1; //"inst_indirect", "", "instrument only indirect instructions Default = 1"
bool KnobInst_frame = 0; //"inst_frame", "", "instrument only frame/constant instructions Default = 0"

std::string KnobBinaryPath = ""; //"bin_path", "", "binary to analyze (required).");
std::string KnobFuncName = ""; //"func", "", "function to analyze (required).");
std::string KnobBlkPath = ""; //"blk_path", "", "specify basic blocks to analyze.");
std::string KnobLatPath = ""; //"lat_path", "", "path containing instruction level load latency");
std::string KnobFpPath = ""; //"fp_path", "", "path containing instruction footprint profile");
std::string KnobDumpFile = ""; //dump_file, "", "file name to dwar scheduling dump""
std::string KnobLinemap = ""; //"linemap", "", "specify instrumented binary to print linemap.");

/*
 * Compute an application's instruction execution cost by re-scheduling
 * its native instructions on a model of the target architecture.
 * The tool receives as input an application's control flow (.cfg) file, 
 * and a machine description file (.mdf).
 */

namespace MIAMI
{
   MIAMI_Driver mdriver;  // global object holding most of the global info
};

/* ===================================================================== */
/*
 * parse_arg which will replace KNOB
 * argc = number of argumenets
 * argv argument list 
 * return 0 if works errno if there is an error
 */
static int parse_opt (int key, char *arg, struct argp_state *state)
{    
    switch (key)
    {
         case outputFile:
        {
            KnobOutputFile.assign(arg);
            break;
        }

        case noPid: 
        {
            KnobNoPid = atoi(arg);
            break;
        }

        case machineFile:
        {
            KnobMachineFile.assign(arg);
            break;
        }

        case persistentDB:
        {
            KnobPersistentDB.assign(arg);   
            break;
        }

        case memoryLatency:
        {
            KnobMemoryLatency.assign(arg);
            break;
        }

        case streamReuse:
        {
            KnobStreamReuse.assign(arg);
            break;
        }

        case includeFile:
        {
            KnobIncludeFile.assign(arg);
            break;
        }

        case excludeFile:
        {
            KnobExcludeFile.assign(arg);
            break;
        }

        case routineName:
        {
            KnobRoutineName.assign(arg);
            break;
        }
 
        case numberParts:
        {
            KnobNumberParts =  atoi(arg);
            break;
        }
 
        case cfgFile:
        {
            KnobCfgFile.assign(arg);
            break;
        }

        case palm_cfgFile:
        {
            KnobPalmCfgFile.assign(arg);
            break;
        }


        case mrdFiles:
        {
            KnobMrdFiles.append(arg);
            break;
        }

        case threshold:
        {
            KnobThreshold = atof(arg);
            break;
        }

        case detailedMetrics:
        {
            KnobDetailedMetrics =  atoi(arg);
            break;
        }

        case noScheduling:
        {
            KnobNoScheduling = atoi(arg);
            break;
        }

        case skipLineMap:
        {
            KnobSkipLineMap = atoi(arg);
            break;
        }

        case noScopes:
        {
            KnobNoScopes = atoi(arg);
            break;
        }

        case instructionMix:
        {
            KnobInstructionMix = atoi(arg);
            break;
        }
 
        case instructionWidthMix:
        {
            KnobInstructionWidthMix = atoi(arg);
            break;
        }
         
        case scopeName:
        {
            KnobScopeName.assign(arg);
            break;
        }

        case generateXML:
        {
            KnobGenerateXML = atoi(arg);
            break;
        }

        case binaryPath:
        {
            KnobBinaryPath.assign(arg);
            break;
        }

        case funcName:
        {
            KnobFuncName.assign(arg);
            break;
        }
 
        case blkPath:
        {
            KnobBlkPath.assign(arg);
            break;
        }

        case latPath:
        {
            KnobLatPath.assign(arg);
            break;
        }
        
        case fpPath:
        {
            KnobFpPath.assign(arg);
            break;
        }
        
        case dumpFile:
        {
            KnobDumpFile.assign(arg);
            break;
        }
        
        case load_classes:
        {
            KnobLoad_classes = true;
        }
        case inst_loads:
        {
            KnobInst_loads = atoi(arg);
        }
        case inst_stores:
        {
            KnobInst_stores = atoi(arg);
        }
        case inst_strided:
        {
            KnobInst_strided = atoi(arg);
        }
        case inst_indirect:
        {
            KnobInst_indirect = atoi(arg);
        }
        case inst_frame:
        {
            KnobInst_frame = atoi(arg);
        }
        case linemap:
        {
            KnobLinemap.assign(arg);
            break;
        }


        default :
        {
            break; 
        }
    }
    return 0;
}

int parse_args(int argc , char * argv[]){
    int err_no = 0;
    struct argp_option options[] = {
        { 0, outputFile, "STRING", 0, "specify output file name prefix, default: execname-machname"},
        { "no_pid", noPid, "BOOL", 0, "do not add process PID to output file name"},
        { "mfile", machineFile, "STRING", 0, "specify machine description file (computes instruction schedule if specified)"},
        { "db", persistentDB, "STRING", 0, "name of a directory to use as a persistent storage of static analysis results"},
        { 0, memoryLatency, "STRING", 0, "specify name of file containing memory latency results, default: none"},
        { 0, streamReuse, "STRING", 0, "specify name of file containing stream reuse results, default: none"},
        { 0, includeFile, "STRING", 0, "only analyze routines whose names are listed in the specified file, default: none"},
        { 0, excludeFile, "STRING", 0, "do not analyze routines whose names are listed in the specified file, default: none"},
        { 0, routineName, "STRING", 0, "specify routine name for which to dump CFG in dot format, default: none"},
        { 0, numberParts, "BOOL", 0, "number of parts to split a routine's CFG into for drawing in dot format. Useful for very large routines."},
        { 0, cfgFile, "STRING", 0, "read CFG information from specified file (required)"},
        { "palm_cfg", palm_cfgFile, "STRING", 0, "read palm CFG information from specified file (required)"},
        { "mrd", mrdFiles, "STRING", 0, "specify a memory reuse distance file for data reuse analysis. This switch can be used more than once to pass multiple MRD files."},
        { 0, threshold, "FLOAT", 0, "threshold to use for filtering program scopes. Only output program scopes that contribute more than the specified fraction of at least one performance metric."},
        { "detailed", detailedMetrics, "BOOL", 0, "detailed performance database: output additional performance metrics, see manual for the list of affected metrics"},
        { "no_scheduling", noScheduling, "BOOL", 0,  "do not compute instruction schedules even if a machine model is provided"},
        { "no_linemap", skipLineMap, "BOOL", 0, "do not read line mapping information, do not map scopes to source files"},
        { "no_scopes", noScopes, "BOOL", 0, "do not present information at scope level; do not compute scope tree"},
        { "imix", instructionMix, "BOOL", 0, "compute/report instruction mix information"},
        { "iwmix", instructionWidthMix, "BOOL", 0, "compute/report instruction/width mix information"},
        { 0, scopeName, "STRING", 0, "specify scope name for which to dump imix data in CSV format, default: all"},
        { "xml", generateXML, "BOOL", 0, "generate XML output in hpcviewer format. Enabled by default only if a machine file is provided and scheduling is performed."},
        { "bin_path", binaryPath, "STRING", 0, "binary to analyze (required)."},
        { "func", funcName, "STRING", 0, "function to analyze (required)."},
        { "blk_path", blkPath, "STRING", 0, "specify basic blocks to analyze."},
        { "lat_path", latPath, "STRING", 0, "path containing instruction level load latency"},
        { "fp_path", fpPath, "STRING", 0, "path containing instruction footprint profile"},
        { "dump_file", dumpFile, "STRING", 0, "file name to draw scheduling dump"},
        { "load_classes", load_classes, "STRING", 0, "Does load classification for every function if --func is not defined"},
        { "inst_loads", inst_loads, "BOOL", 1, "Instrument Loads (Default=1)"},
        { "inst_stores", inst_stores, "BOOL", 1, "Instrument stores (Default=1)"},
        { "inst_strided", inst_strided, "BOOL", 1, "Instrument Strided Load/Stores (Default=1)"},
        { "inst_indirect", inst_indirect, "BOOL", 1, "Instrument Indirect Load/Stores (Default=1)"},
        { "inst_frame", inst_frame, "BOOL", 1, "Instrument Frame/Constant Load/Stores (Default=0)"},
        { "linemap", linemap, "STRING", 0, "specify an instrumented binary to print Linemap."},
        {0}
     };

    struct argp argp = { options, parse_opt };
    err_no = argp_parse (&argp, argc, argv, 0, 0, 0);

    return err_no;
} 


/* ====================================================================== */

int 
main (int argc, char *argv[])
{
    const char* MIAMI_DEBUG_WAIT = getenv("MIAMI_DEBUG_WAIT");
    if (MIAMI_DEBUG_WAIT) {
      volatile int DEBUGGER_WAIT = 1;
      while (DEBUGGER_WAIT);

      unsetenv("MIAMI_DEBUG_WAIT");
    }
    parse_args(argc , argv);  


    MIAMI::instruction_decoding_init(NULL);
    
    MIAMI::MiamiOptions *mo = new MIAMI::MiamiOptions();
    // set the noScheduling option first, before adding the machine file,
    // so we do not enable an entire set of analyses when adding the machine
    // file
    //
    mo->setNoScheduling(KnobNoScheduling);
    // check and set XML option before adding the machine file because
    // the machine file will enable XML output automatically, and I do not want
    // to override it by not having this option specified.
    mo->setDumpXML(KnobGenerateXML);
    mo->addMachineFile(KnobMachineFile);
    mo->addPersistentCache(KnobPersistentDB);
    mo->addOutputFilePrefix(KnobOutputFile);
    mo->setNoPid(KnobNoPid);
    mo->addMemoryLatencyFile(KnobMemoryLatency);
    mo->addStreamReuseFile(KnobStreamReuse);
    mo->addDebugRoutine(KnobRoutineName);
    mo->addDebugParts(KnobNumberParts);
    mo->addCfgFile(KnobCfgFile);
    mo->addPalmCfgFile(KnobPalmCfgFile);
    mo->setNoLinemap(KnobSkipLineMap);
    mo->setNoScopeTree(KnobNoScopes);
    mo->setDoInstructionMix(KnobInstructionMix);
    mo->setDoInstructionWidthMix(KnobInstructionWidthMix);
    mo->addScopeName(KnobScopeName);
    mo->setOutputThreshold(KnobThreshold);
    mo->setDetailedMetrics(KnobDetailedMetrics);
    mo->addBinaryPath(KnobBinaryPath);
    mo->addFuncName(KnobFuncName);
    mo->addBlockPath(KnobBlkPath);
    mo->addLatPath(KnobLatPath);
    mo->addFpPath(KnobFpPath);
    mo->addDumpFile(KnobDumpFile);
    mo->setLoadClasses(KnobLoad_classes);
    mo->setInstLoads(KnobInst_loads);
    mo->setInstStores(KnobInst_stores);
    mo->setInstStrided(KnobInst_strided);
    mo->setInstIndirect(KnobInst_indirect);
    mo->setInstFrame(KnobInst_frame);
    std::cout<<"CALLIng to add line map.\n";
    mo->addLinemap(KnobLinemap);
  
    if (! mo->CheckDependencies())
       return 0;

  
    // This tool is compiled both as a standalone tool
    // and as a dynamic object pintool. getpid is safe in the standalone
    // case, but it may return a wrong pid when running under PIN.
   if (MIAMI::mdriver.Initialize(mo, 0) < 0)
       return 0 ; //(Usage());
    if (mo->printLinemap){
      MIAMI::mdriver.printLinemapInfo();
      return 0;
   }
 
   MIAMI::mdriver.ParseIncludeExcludeFiles(KnobIncludeFile, KnobExcludeFile);
   std::cout<<"Inst loads="<<mo->inst_loads<<" Stores="<<mo->inst_stores<<" Strided="<<mo->inst_strided<<" Indirect="<<mo->inst_indirect<<" Frame/Constant="<<mo->inst_frame<<std::endl;
   if(mo->func_name.length()){
      std::cout<<"Function Name is "<<mo->func_name<<std::endl;
   } else {
      std::cout<<"Function Name is "<<mo->func_name<<std::endl;
   }
    int nImgs = MIAMI::mdriver.NumberOfImages();
    nImgs=1;//TODO FIXME this is a hack delete it for full run
    const std::string* iNames = MIAMI::mdriver.getImageNames();
    for (int i=0 ; i<nImgs ; ++i)
    {
       if (i==0)  // this is the first image, extract the path and the name of the executable
       {
          string ename, epath;
          MIAMIU::ExtractNameAndPath(iNames[i], epath, ename);
          mo->addExecutableName(ename);
          mo->addExecutablePath(epath);
       }

#if DEBUG_CFG_COUNTS
       DEBUG_CFG(1,
          fprintf(stderr, "Attempting to open file %d: %s\n",  i, iNames[i].c_str());
       )
#endif
       std::string iname = iNames[i];
       MIAMI::mdriver.LoadImage(i+1, iname, 0, 0); //IMG_StartAddress(img)-low_offset, low_offset);
    }

    MIAMI::mdriver.Finalize(KnobOutputFile);

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
