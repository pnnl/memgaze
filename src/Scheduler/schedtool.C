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

//ozgurS commenting out include pin adding include xed
//#include <pin.H>
// including argparse 
#include <argp.h>
//ozgurE

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
KNOB<string> KnobFamilyRequired (KNOB_MODE_COMMENT,    "required",
    "", "", "MIAMI required switches");
KNOB<string> KnobFamilyOptional (KNOB_MODE_COMMENT,    "optional",
    "", "", "MIAMI optional switches");
*/
/* ozgurS removing all KNOB
 */
 
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


std::string KnobBinaryPath = ""; //"bin_path", "", "binary to analyze (required).");
std::string KnobFuncName = ""; //"func", "", "function to analyze (required).");
std::string KnobBlkPath = ""; //"blk_path", "", "specify basic blocks to analyze.");
std::string KnobLatPath = ""; //"lat_path", "", "path containing instruction level load latency");

/* ===================================================================== */

/*ozgurS
 *closing this
 *
 * INT32 Usage()
{
    cerr << endl << "Usage:" << endl <<
        "  This tool reads the previously discovered CFG of an application,\n"
        "  recovers the executed paths through the CFG and their execution \n"
        "  frequencies, and computes a baseline execution time in absence\n"
        "  of dynamic misses.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}
/* ===================================================================== */

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
 * ozgurS
 * Closing this
 */

/*VOID ImageLoad (IMG img, VOID *v)
{
   uint32_t id = IMG_Id (img);
   std::string iname = IMG_Name(img);

   if (id==1)  // this is the first image, extract the path and the name of the executable
   {
      string ename, epath;
      MIAMIU::ExtractNameAndPath(iname, epath, ename);
      MIAMI::MiamiOptions *mo = MIAMI::mdriver.getProgramOptions();
      mo->addExecutableName(ename);
      mo->addExecutablePath(epath);
   }
   
   // print info about the sections in this image, for debugging
   // comment out in production runs
#if DEBUG_CFG_COUNTS
   DEBUG_CFG(4,
      cerr << "Image: " << iname << ", id " << id << hex
           << " load offser=0x" << IMG_LoadOffset(img)
           << ", low addr=0x" << IMG_LowAddress(img)
           << ", high addr=0x" << IMG_HighAddress(img)
           << ", start addr=0x" << IMG_StartAddress(img)
           << ", mapped size=0x" << IMG_SizeMapped(img) << dec
           << ", has the following sections:" << endl;
      for (SEC sec= IMG_SecHead(img) ; SEC_Valid(sec) ; sec = SEC_Next(sec))
      {
         cerr << "Section " << SEC_Name(sec) << " of type " << SEC_Type(sec)
              << " at address 0x" << hex << SEC_Address(sec) << " of size 0x" 
              << SEC_Size(sec) << dec << "/" << SEC_Size(sec) << " bytes:"
              << " valid? " << SEC_Valid(sec) << ", mapped? " << SEC_Mapped(sec)
              << ", executable? " << SEC_IsExecutable(sec) 
              << ", readable? " << SEC_IsReadable(sec)
              << ", writable? " << SEC_IsWriteable(sec) << endl;
      }
   )
#endif
   MIAMI::addrtype load_offset = IMG_LoadOffset(img);
   MIAMI::mdriver.LoadImage(id, iname, load_offset, IMG_LowAddress(img)-load_offset);
}
/* ===================================================================== */


/*VOID Fini (int code, VOID * v)
{
   MIAMI::mdriver.Finalize(KnobOutputFile.Value());
}


/* ===================================================================== */
/*
 * ozgurS parse_arg which will replace KNOB
 * argc = number of argumenets
 * argv argument list 
 * return 0 if works errno if there is an error
 */
static int parse_opt (int key, char *arg, struct argp_state *state)
{    
    switch (key)
    {
         case 'o':
        {
            KnobOutputFile.assign(arg);
            break;
        }

        case 900: //no_pid
        {
            KnobNoPid = atoi(arg);
            break;
        }

        case 'm':
        {
            KnobMachineFile.assign(arg);
            break;
        }

        case 901://db
        {
            KnobPersistentDB.assign(arg);   
            break;
        }

        case 'L':
        {
            KnobMemoryLatency.assign(arg);
            break;
        }

        case 'S':
        {
            KnobStreamReuse.assign(arg);
            break;
        }

        case 'I':
        {
            KnobIncludeFile.assign(arg);
            break;
        }

        case 'X':
        {
            KnobExcludeFile.assign(arg);
            break;
        }

        case 'r':
        {
            KnobRoutineName.assign(arg);
            break;
        }
 
        case 'p':
        {
            KnobNumberParts =  atoi(arg);
            break;
        }
 
        case 'c':
        {
            KnobCfgFile.assign(arg);
            break;
        }

        case 902: //mrd
        {
            KnobMrdFiles.append(arg);
            break;
        }

        case 'f':
        {
            KnobThreshold = atof(arg);
            break;
        }

        case 903: //detailed
        {
            KnobDetailedMetrics =  atoi(arg);
            break;
        }

        case 904: //no_scheduling
        {
            KnobNoScheduling = atoi(arg);
            break;
        }

        case 905://no_linemap
        {
            KnobSkipLineMap = atoi(arg);
            break;
        }

        case 906://no_scopes
        {
            KnobNoScopes = atoi(arg);
            break;
        }

        case 907://imix
        {
            KnobInstructionMix = atoi(arg);
            break;
        }
 
        case 908://iwmix
        {
            KnobInstructionWidthMix = atoi(arg);
            break;
        }
         
        case 's':
        {
            KnobScopeName.assign(arg);
            break;
        }

        case 909://xml
        {
            KnobGenerateXML = atoi(arg);
            break;
        }

        case 910://bin_path
        {
            KnobBinaryPath.assign(arg);
            break;
        }

        case 911://func
        {
            KnobFuncName.assign(arg);
            break;
        }
 
        case 912://blk_path
        {
            KnobBlkPath.assign(arg);
            break;
        }

        case 913://lat_path
        {
            KnobLatPath.assign(arg);
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
        { 0, 'o', "STRING", 0, "specify output file name prefix, default: execname-machname"},
        { "no_pid", 900, "BOOL", 0, "do not add process PID to output file name"},
        { "mfile", 'm', "STRING", 0, "specify machine description file (computes instruction schedule if specified)"},
        { "db", 901, "STRING", 0, "name of a directory to use as a persistent storage of static analysis results"},
        { 0, 'L', "STRING", 0, "specify name of file containing memory latency results, default: none"},
        { 0, 'S', "STRING", 0, "specify name of file containing stream reuse results, default: none"},
        { 0, 'I', "STRING", 0, "only analyze routines whose names are listed in the specified file, default: none"},
        { 0, 'X', "STRING", 0, "do not analyze routines whose names are listed in the specified file, default: none"},
        { 0, 'r', "STRING", 0, "specify routine name for which to dump CFG in dot format, default: none"},
        { 0, 'p', "BOOL", 0, "number of parts to split a routine's CFG into for drawing in dot format. Useful for very large routines."},
        { 0, 'c', "STRING", 0, "read CFG information from specified file (required)"},
        { "mrd", 902, "STRING", 0, "specify a memory reuse distance file for data reuse analysis. This switch can be used more than once to pass multiple MRD files."},
        { 0, 'f', "FLOAT", 0, "threshold to use for filtering program scopes. Only output program scopes that contribute more than the specified fraction of at least one performance metric."},
        { "detailed", 903, "BOOL", 0, "detailed performance database: output additional performance metrics, see manual for the list of affected metrics"},
        { "no_scheduling", 904, "BOOL", 0,  "do not compute instruction schedules even if a machine model is provided"},
        { "no_linemap", 905, "BOOL", 0, "do not read line mapping information, do not map scopes to source files"},
        { "no_scopes", 906, "BOOL", 0, "do not present information at scope level; do not compute scope tree"},
        { "imix", 907, "BOOL", 0, "compute/report instruction mix information"},
        { "iwmix", 908, "BOOL", 0, "compute/report instruction/width mix information"},
        { 0, 's', "STRING", 0, "specify scope name for which to dump imix data in CSV format, default: all"},
        { "xml", 909, "BOOL", 0, "generate XML output in hpcviewer format. Enabled by default only if a machine file is provided and scheduling is performed."},
        { "bin_path", 910, "STRING", 0, "binary to analyze (required)."},
        { "func", 911, "STRING", 0, "function to analyze (required)."},
        { "blk_path", 912, "STRING", 0, "specify basic blocks to analyze."},
        { "lat_path", 913, "STRING", 0, "path containing instruction level load latency"},
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
/*
 * ozgurS adding a function to parce arguments
 */
    parse_args(argc , argv);  


    MIAMI::instruction_decoding_init(NULL);
    
    MIAMI::MiamiOptions *mo = new MIAMI::MiamiOptions();
    // set the noScheduling option first, before adding the machine file,
    // so we do not enable an entire set of analyses when adding the machine
    // file
    //
    // ozgurS removing .Value  since we get rid of pin KNOB
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
   // TO DO ozgur chech this is it a array ? and fix it  
/*    int numMrdFiles = KnobMrdFiles.NumberOfValues();
    if (numMrdFiles > 0)
    {
       // pass all the mrd files to the MiamiOptions object
       int i;
       for (i=0 ; i<numMrdFiles ; ++i)
          mo->addMrdFile(KnobMrdFiles.Value(i));
    }
  */

  
    if (! mo->CheckDependencies())
       return 0; // (Usage()); ozgurS get rid of usage for now

    // string pathStr = "";
    // for(int i;i<argc;i++){
    //   if(strcmp("-path",argv[i])==0){
    //     pathStr = std::string(argv[++i]);
    //     mo->addBlockPath(pathStr);
    //     break;
    //   }
    // }
    
    // This tool is compiled both as a standalone tool
    // and as a dynamic object pintool. getpid is safe in the standalone
    // case, but it may return a wrong pid when running under PIN.
/*
 * ozgurS commenting out PIN_GetPid
 */
   if (MIAMI::mdriver.Initialize(mo, 0) < 0)
       return 0 ; //(Usage());
 
   MIAMI::mdriver.ParseIncludeExcludeFiles(KnobIncludeFile, KnobExcludeFile);
    
#ifdef STATIC_COMPILATION
    int nImgs = MIAMI::mdriver.NumberOfImages();
    const std::string* iNames = MIAMI::mdriver.getImageNames();
    for (int i=0 ; i<nImgs ; ++i)
    {
      //if (iNames[i].find("a.out") != std::string::npos){
       if (i==0)  // this is the first image, extract the path and the name of the executable
       {
          string ename, epath;
          MIAMIU::ExtractNameAndPath(iNames[i], epath, ename);
          mo->addExecutableName(ename);
          mo->addExecutablePath(epath);
       }

#if 0
       // This test was used to post-process data collected on a different machine
       // Libraries in local system folders were different, so they were generating 
       // errors at decode time. This test just skips over them.
       if (iNames[i].compare(0, 4, "/lib") == 0)
       {
          fprintf(stderr, "Found local library path, %s. Skip it. Remember to remove this check.\n",
               iNames[i].c_str());
          continue;
       }
#endif

#if DEBUG_CFG_COUNTS
       DEBUG_CFG(1,
          fprintf(stderr, "Attempting to open file %d: %s\n",  i, iNames[i].c_str());
       )
#endif
/*
 * ozgurS creating iname
 */
std::string iname = iNames[i];

      
/*
 * ozgurS commenting out IMG part 
 *
 *
        IMG img = IMG_Open(iNames[i]);
       
       if (!IMG_Valid(img))
       {
          fprintf(stderr, "ERROR: Could not open file %s with idx %d\n", iNames[i].c_str(), i);
          fflush(stderr);
          continue;
       }
       
       std::string iname = IMG_Name(img);
   
       // print info about the sections in this image, for debugging
       // comment out in production runs
#if DEBUG_CFG_COUNTS
       DEBUG_CFG(4,
          cerr << "Image: " << iname << ", id " << IMG_Id(img) << hex
               << " load offset=0x" << IMG_LoadOffset(img)
               << ", low addr=0x" << IMG_LowAddress(img)
               << ", high addr=0x" << IMG_HighAddress(img)
               << ", start addr=0x" << IMG_StartAddress(img)
               << ", mapped size=0x" << IMG_SizeMapped(img) << dec
               << ", has the following sections:" << endl;
#if 0
          for (SEC sec= IMG_SecHead(img) ; SEC_Valid(sec) ; sec = SEC_Next(sec))
          {
             cerr << "Section " << SEC_Name(sec) << " of type " << SEC_Type(sec)
                  << " at address 0x" << hex << SEC_Address(sec) << " of size 0x" 
                  << SEC_Size(sec) << dec << "/" << SEC_Size(sec) << " bytes:"
                  << " valid? " << SEC_Valid(sec) << ", mapped? " << SEC_Mapped(sec)
                  << ", executable? " << SEC_IsExecutable(sec) 
                  << ", readable? " << SEC_IsReadable(sec)
                  << ", writable? " << SEC_IsWriteable(sec) << endl;
          }
#endif
       )
#endif
   
       if (mo->func_name.length() > 0){
        RTN rtn = RTN_FindByName ( img, mo->func_name.c_str()); 
        if (!RTN_Valid(rtn)){
          cerr<<"Error: routine "<<mo->func_name<<" not found in: "<<iname<<endl;
        }
        else{
          ADDRINT rStart = RTN_Address(rtn);
          cout<<"routine: "<<mo->func_name<<" addr: "<<(unsigned int*)rStart<<endl;
        }

       }

       MIAMI::addrtype low_offset = IMG_LowAddress(img) - IMG_LoadOffset(img);
       cerr << "Image: " << iname << ", id " << IMG_Id(img) << hex
               << " load offset=0x" << IMG_LoadOffset(img)
               << ", low addr=0x" << IMG_LowAddress(img)
               << ", high addr=0x" << IMG_HighAddress(img)
               << ", start addr=0x" << IMG_StartAddress(img)
               << ", mapped size=0x" << IMG_SizeMapped(img) << dec
               << ", has the following sections:" << endl;
      switch (IMG_Type(img)){
        case IMG_TYPE_STATIC:
            cout<<"static"<<endl;
            break;
        case IMG_TYPE_SHARED:
            cout<<"shared"<<endl;
            break;
        case IMG_TYPE_SHAREDLIB:
            cout<<"shared library"<<endl;
            break;
        case IMG_TYPE_RELOCATABLE:
            cout<<"relocatable"<<endl;
            break;
        default:
            cout<<"unknown"<<endl;
    }
      for (SEC sec= IMG_SecHead(img) ; SEC_Valid(sec) ; sec = SEC_Next(sec))
          {
             cerr << "Section " << SEC_Name(sec) << " of type " << SEC_Type(sec)
                  << " at address 0x" << hex << SEC_Address(sec) << " of size 0x" 
                  << SEC_Size(sec) << dec << "/" << SEC_Size(sec) << " bytes:"
                  << " valid? " << SEC_Valid(sec) << ", mapped? " << SEC_Mapped(sec)
                  << ", executable? " << SEC_IsExecutable(sec) 
                  << ", readable? " << SEC_IsReadable(sec)
                  << ", writable? " << SEC_IsWriteable(sec) << endl;
          }
*/
//TO DO Ryan
       MIAMI::mdriver.LoadImage(i+1, iname, NULL, NULL); //IMG_StartAddress(img)-low_offset, low_offset);

        
//       IMG_Close(img);
      //}

    }

    MIAMI::mdriver.Finalize(KnobOutputFile);
#else
/*
 * ozgruS commenting out else part since it is related to pin
 *
    IMG_AddInstrumentFunction (ImageLoad, 0);
    PIN_AddFiniFunction (Fini, 0);

    // Never returns
    PIN_StartProgram();
*/
#endif
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
