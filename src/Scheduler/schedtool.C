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

#include <pin.H>

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
KNOB<string> KnobOutputFile (KNOB_MODE_WRITEONCE,    "pintool",
    "o", "", "specify output file name prefix, default: execname-machname");
KNOB<BOOL>   KnobNoPid (KNOB_MODE_WRITEONCE,    "pintool",
    "no_pid", "0", "do not add process PID to output file name");
KNOB<string> KnobMachineFile (KNOB_MODE_WRITEONCE,    "pintool",
    "m", "", "specify machine description file (computes instruction schedule if specified)");
KNOB<string> KnobPersistentDB (KNOB_MODE_WRITEONCE,    "pintool",
    "db", "", "name of a directory to use as a persistent storage of static analysis results");
KNOB<string> KnobMemoryLatency (KNOB_MODE_WRITEONCE,    "pintool",
    "L", "", "specify name of file containing memory latency results, default: none");
KNOB<string> KnobStreamReuse (KNOB_MODE_WRITEONCE,    "pintool",
    "S", "", "specify name of file containing stream reuse results, default: none");
KNOB<string> KnobIncludeFile (KNOB_MODE_WRITEONCE,    "pintool",
    "I", "", "only analyze routines whose names are listed in the specified file, default: none");
KNOB<string> KnobExcludeFile (KNOB_MODE_WRITEONCE,    "pintool",
    "X", "", "do not analyze routines whose names are listed in the specified file, default: none");
KNOB<string> KnobRoutineName (KNOB_MODE_WRITEONCE,    "pintool",
    "r", "", "specify routine name for which to dump CFG in dot format, default: none");
KNOB<int> KnobNumberParts (KNOB_MODE_WRITEONCE,    "pintool",
    "p", "1", "number of parts to split a routine's CFG into for drawing in dot format. Useful for very large routines.");
KNOB<string> KnobCfgFile (KNOB_MODE_WRITEONCE,    "pintool",
    "c", "", "read CFG information from specified file (required)");
KNOB<string> KnobMrdFiles (KNOB_MODE_APPEND,    "pintool",
    "mrd", "", "specify a memory reuse distance file for data reuse analysis. This switch can be used more than once to pass multiple MRD files.");

KNOB<double> KnobThreshold (KNOB_MODE_WRITEONCE,    "pintool",
    "f", "0.0", "threshold to use for filtering program scopes. Only output program scopes that contribute more than the specified fraction of at least one performance metric.");

KNOB<BOOL>   KnobDetailedMetrics (KNOB_MODE_WRITEONCE,    "pintool",
    "detailed", "0", "detailed performance database: output additional performance metrics, see manual for the list of affected metrics");
KNOB<BOOL>   KnobNoScheduling (KNOB_MODE_WRITEONCE,    "pintool",
    "no_scheduling", "0", "do not compute instruction schedules even if a machine model is provided");
KNOB<BOOL>   KnobSkipLineMap (KNOB_MODE_WRITEONCE,    "pintool",
    "no_linemap", "0", "do not read line mapping information, do not map scopes to source files");
KNOB<BOOL>   KnobNoScopes (KNOB_MODE_WRITEONCE,    "pintool",
    "no_scopes", "0", "do not present information at scope level; do not compute scope tree");
KNOB<BOOL>   KnobInstructionMix (KNOB_MODE_WRITEONCE,    "pintool",
    "imix", "0", "compute/report instruction mix information");
KNOB<BOOL>   KnobInstructionWidthMix (KNOB_MODE_WRITEONCE,    "pintool",
    "iwmix", "0", "compute/report instruction/width mix information");
KNOB<string> KnobScopeName (KNOB_MODE_WRITEONCE,    "pintool",
    "s", "", "specify scope name for which to dump imix data in CSV format, default: all");
KNOB<BOOL>   KnobGenerateXML (KNOB_MODE_WRITEONCE,    "pintool",
    "xml", "0", "generate XML output in hpcviewer format. Enabled by default only if a machine file is provided and scheduling is performed.");


KNOB<string> KnobBinaryPath (KNOB_MODE_WRITEONCE,    "pintool",
    "bin_path", "", "binary to analyze (required).");
KNOB<string> KnobFuncName (KNOB_MODE_WRITEONCE,    "pintool",
    "func", "", "function to analyze (required).");
KNOB<string> KnobBlkPath (KNOB_MODE_WRITEONCE,    "pintool",
    "blk_path", "", "specify basic blocks to analyze.");
KNOB<string> KnobLatPath (KNOB_MODE_WRITEONCE,    "pintool",
    "lat_path", "", "path containing instruction level load latency");

/* ===================================================================== */

INT32 Usage()
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

VOID ImageLoad (IMG img, VOID *v)
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


VOID Fini (int code, VOID * v)
{
   MIAMI::mdriver.Finalize(KnobOutputFile.Value());
}


/* ===================================================================== */

int 
main (int argc, char *argv[])
{
    const char* MIAMI_DEBUG_WAIT = getenv("MIAMI_DEBUG_WAIT");
    if (MIAMI_DEBUG_WAIT) {
      volatile int DEBUGGER_WAIT = 1;
      while (DEBUGGER_WAIT);

      unsetenv("MIAMI_DEBUG_WAIT");
    }
  
//    PIN_InitSymbolsAlt(IFUNC_SYMBOLS);
    PIN_InitSymbols();

    if (PIN_Init (argc,argv))
    {
        return Usage();
    }

    MIAMI::instruction_decoding_init(NULL);
    
    MIAMI::MiamiOptions *mo = new MIAMI::MiamiOptions();
    // set the noScheduling option first, before adding the machine file,
    // so we do not enable an entire set of analyses when adding the machine
    // file
    mo->setNoScheduling(KnobNoScheduling.Value());
    // check and set XML option before adding the machine file because
    // the machine file will enable XML output automatically, and I do not want
    // to override it by not having this option specified.
    mo->setDumpXML(KnobGenerateXML.Value());
    mo->addMachineFile(KnobMachineFile.Value());
    mo->addPersistentCache(KnobPersistentDB.Value());
    mo->addOutputFilePrefix(KnobOutputFile.Value());
    mo->setNoPid(KnobNoPid.Value());
    mo->addMemoryLatencyFile(KnobMemoryLatency.Value());
    mo->addStreamReuseFile(KnobStreamReuse.Value());
    mo->addDebugRoutine(KnobRoutineName.Value());
    mo->addDebugParts(KnobNumberParts.Value());
    mo->addCfgFile(KnobCfgFile.Value());
    mo->setNoLinemap(KnobSkipLineMap.Value());
    mo->setNoScopeTree(KnobNoScopes.Value());
    mo->setDoInstructionMix(KnobInstructionMix.Value());
    mo->setDoInstructionWidthMix(KnobInstructionWidthMix.Value());
    mo->addScopeName(KnobScopeName.Value());
    mo->setOutputThreshold(KnobThreshold.Value());
    mo->setDetailedMetrics(KnobDetailedMetrics.Value());
    mo->addBinaryPath(KnobBinaryPath.Value());
    mo->addFuncName(KnobFuncName.Value());
    mo->addBlockPath(KnobBlkPath.Value());
    mo->addLatPath(KnobLatPath.Value());
    
    int numMrdFiles = KnobMrdFiles.NumberOfValues();
    if (numMrdFiles > 0)
    {
       // pass all the mrd files to the MiamiOptions object
       int i;
       for (i=0 ; i<numMrdFiles ; ++i)
          mo->addMrdFile(KnobMrdFiles.Value(i));
    }
    
    if (! mo->CheckDependencies())
       return (Usage());

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
    if (MIAMI::mdriver.Initialize(mo, PIN_GetPid()) < 0)
       return (Usage());
    
    MIAMI::mdriver.ParseIncludeExcludeFiles(KnobIncludeFile.Value(), KnobExcludeFile.Value());
    
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
       MIAMI::mdriver.LoadImage(i+1, iname, IMG_StartAddress(img)-low_offset, low_offset);

        
       IMG_Close(img);
      //}
    }
    MIAMI::mdriver.Finalize(KnobOutputFile.Value());
#else

    IMG_AddInstrumentFunction (ImageLoad, 0);
    PIN_AddFiniFunction (Fini, 0);

    // Never returns
    PIN_StartProgram();

#endif
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
