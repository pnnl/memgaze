/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: MiamiDriver.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the main control logic for the MIAMI scheduler.
 */

#ifndef _MIAMI_DRIVER_H
#define _MIAMI_DRIVER_H

#include "miami_types.h"

#include "load_module.h"
#include "prog_scope.h"
#include "Machine.h"
#include "scope_reuse_data.h"
#include "fast_hashmap.h"
#include "slice_references.h"

#include "memory_latency_histograms.h"
#include "memory_reuse_histograms.h"
//#include "stream_reuse_histograms.h"
#include "miami_options.h"

#include <BPatch.h>

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string>
#include <map>

namespace MIAMI
{

extern ScopeImplementation* defaultSIP;
typedef MIAMIU::HashMap<unsigned int, ScopeImplementation*, &defaultSIP> ScopeHashMap;

typedef std::map<uint64_t, double*> UI64DoublePMap;

extern std::string StripPath (const std::string&  path);
extern const char * StripPath (const char * path);

class CompareStrings
{
public:
  bool operator() (const std::string& s1, const std::string& s2) const
  {
     return (s1.compare(s2)<0);
  }
};
/* ===================================================================== */

// define a map for associating file offsets to image names
typedef std::map<std::string, uint64_t, CompareStrings> StringOffsetMap;

// define a map for associating integer indices to file names
typedef std::map<std::string, unsigned int, CompareStrings> StringIndexMap;
typedef std::vector<const std::string*> StringVector;

typedef std::vector<double> DoubleVector;

#define IRREG_LOOP_LEVEL_SHIFT 24
#define IRREG_LOOP_LEVEL_MASK  0xff000000
#define IRREG_SET_INDEX_MASK   0x00ffffff

class Routine;

/* MIAMI_Driver is the top MIAMI object that drives the entire analysis
 * and modeling logic. The schedtool PIN tool should interact with the 
 * MIAMI toolkit only through this object.
 * It defines entry points for the major analysis phases and contains
 * all the global data.
 */
class MIAMI_Driver
{
public:
   friend class MIAMI::Routine;
   
   MIAMI_Driver()
   {
      allImgs = 0;
      maxImgs = 0;
      loadedImgs = 0;
      imgNames = 0;

      perform_memory_analysis = false;
      no_fpga_acc = true;
      compute_reuse = false;
      
      mem_level_has_carried_info = 0;
      mem_level_has_irreg_carried_info = 0;
      mem_level_has_frag_info = 0;
      totalCarried = 0;
      totalIrregCarried = 0;
      totalFragMisses = 0;
      fstream = 0;
      mo = 0;
      
      highFileIndex = 1;
      fileIndices.insert(StringIndexMap::value_type(std::string("unknown-file"), 0));
      fileNames.push_back(&(fileIndices.begin()->first));
   }
   
   ~MIAMI_Driver();

   int Initialize(MiamiOptions *_mo, int _pid);
   
   void Finalize(const std::string& outFile);
   void LoadImage(uint32_t id, std::string& iname, addrtype start_addr, addrtype low_offset);
   int NumberOfImages() const  { return (maxImgs); }
   const std::string* getImageNames() const { return (imgNames); }
   LoadModule* GetModuleWithIndex(int id);
   
//   void addFootPrintDataForScopes (ScopeImplementation *prog, int no_fpga_acc);
//   void addMemoryDataForScope (ScopeImplementation *pscope);

   const MachineList& Targets() const    { return (targets); }
   inline ProgScope* GetProgramScope()   { return (prog); }

   MiamiOptions* getProgramOptions() const  { return mo; }

   unsigned int GetFileIndex(std::string &file);
   inline const MIAMIU::UISet& GetLinesForFile(unsigned int findex) const
   { 
      assert (findex < highFileIndex);
      MIAMIU::Ui2SetMap::const_iterator sit = allFileLines.find(findex);
      return (sit->second);
   }
   void AddSourceFileInfo(unsigned int findex, unsigned int l1, unsigned int l2);
   
   inline PairSRDMap& GetScopePairsReuse()  { return (reusePairs); }
   
   void ParseIncludeExcludeFiles(const std::string& include_file, 
                    const std::string& exclude_file);

private:
   void parse_machine_description (const std::string& machine_file);
   
   void dump_xml_output (FILE* _out_fd, ScopeImplementation *pscope, const std::string& title,
             double threshold = 0.0, bool flat_data = false);
   void dump_stream_count_histograms(FILE *fout, ScopeImplementation *pscope);
   void dump_imix_histograms(FILE *fout, ScopeImplementation *pscope, bool use_width=false);
   void dump_ibins_histograms(FILE *fout, ScopeImplementation *pscope);
   
   void aggregate_counts_for_scope (ScopeImplementation *pscope,
             Machine *tmach, int no_fpga_acc);
   void aggregate_resources_used_for_scope (ScopeImplementation *pscope, Machine *tmach);
   
   void compute_memory_information(Machine *tmach);

   int parse_stream_reuse_data(const std::string& stream_file);
   void compute_stream_reuse_for_scope(FILE *fout, ScopeImplementation *pscope, 
             uint32_t img_id, AddrIntSet& scopeMemRefs, RFormulasMap& refFormulas,
             addrtype img_base);
             
   int parse_memory_latencies(const std::string& memory_file);
   void compute_memory_stalls_for_scope(ScopeImplementation *pscope, 
             AddrSet& scopeMemRefs);
   
   void dump_streams_for_scope(FILE *fout, ScopeImplementation *pscope);
   
   void dump_scope_pairs_xml_output (FILE* _out_fd, ScopeImplementation *pscope, 
            const std::string& title, double threshold);

   void xml_dump_for_scope (FILE *_out_fd, ScopeImplementation *pscope,
            MIAMIU::UiUiMap &TAshortNameMap, int memParMapping, int carriedMissesBase,
            int indent, int numLevels, DoubleVector &threshValues,
            bool flat_data, bool top_scope);

   void dumpOneReuseMap (FILE *_out_fd, MIAMIU::UiUiMap &TAshortNameMap,
            UI64DoublePMap &singleMap, Trio64DoublePMap *pairMap, int indent,
            int numLevels, double incCpuTime, bool firstIdx, bool isEncodedId);

   void dumpOneCarryMap (FILE *_out_fd, int carryBaseIndex,
            UI64DoublePMap &singleMap, Pair64DoublePMap *pairMap, int indent,
            int numLevels, bool firstIdx);

   void dump_line_mapping_for_scope(FILE *_out_fd, ScopeImplementation *pscope, int indent);

   
   MiamiOptions *mo;
   
   LoadModule **allImgs;
   uint32_t maxImgs;
   uint32_t loadedImgs;
   FILE *fd;
   ProgScope *prog;
   StringOffsetMap imgFileOffsets;
   std::string *imgNames;
   MachineList targets;
   int pid;
   FILE *fstream;
   
   /* next fields are used by the memory analysis */
   bool perform_memory_analysis;
   bool no_fpga_acc;
   bool compute_reuse;
   
   int *mem_level_has_carried_info;
   int *mem_level_has_irreg_carried_info;
   int *mem_level_has_frag_info;
   double *totalCarried;
   double *totalIrregCarried;
   double *totalFragMisses;

   // PairSRDMap is a global data-structure. It aggregates all data reuse patterns.
   // First, patterns are grouped by source and sink scopes. For each such pair, 
   // we have a map organized by carrier scope and array name, with the value
   // being a vector of miss values for each memory level.
   // This data-structure is used to display the reuse XML file.
   //
   // I need to eventually replace the above data-structure with a
   // reusePatterns one, indexed by carrier scope. I want for each carrier
   // scope to show the dynamic path to the source and the destination scopes.
   PairSRDMap reusePairs;

   // histogram that maps references (represented by their PCs) to a histogram of 
   // memory latencies observed for that instruction
   RefLatencyHistMap refLatencies;

   // maintain a unique mapping from file names to integer indices
   unsigned int highFileIndex;
   StringIndexMap fileIndices;
   StringVector   fileNames;
   // maintain a global set of source lines for each file. I need to find 
   // continuous line ranges
   MIAMIU::Ui2SetMap allFileLines;
   
   MIAMI_MEM_REUSE::MRDDataVec mrdData;


   BPatch bpatch;
};

} /* namespace MIAMI */

#endif
