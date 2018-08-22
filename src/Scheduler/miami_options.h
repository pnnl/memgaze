/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: miami_options.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Stores scheduler configuration options.
 */

#ifndef _MIAMI_OPTIONS_H
#define _MIAMI_OPTIONS_H

#include "miami_types.h"
#include "mrd_file_info.h"
#include <string>
#include <map>

using std::string;

namespace MIAMI
{
   typedef std::map<uint32_t, MrdInfo> MrdFileMap;

   class MiamiOptions
   {
   public:
      bool do_scheduling; // compute full instruction schedule
      bool do_fp; // compute full footprint
      bool has_mdl;       // has machine description file
      bool do_cfgcounts;  // compute BB and Edge counts
      bool do_scopetree;  // construct the scope tree
      bool do_cfgpaths;   // reconstruct paths

      bool do_build_dg;   // build dependence graph
      bool do_ideal_sched;// compute ideal schedule (no machine info)
      bool do_streams;    // aggregate stream reuse
      bool do_imix;       // compute instruction mix
      bool do_iwmix;      // compute instruction/width mix
      bool do_ibins;      // compute fine grain instruction mix
      bool do_memlat;     // use memory latency file to compute mem overlap
      bool do_staticmem;  // perform static memory analysis to compute base and stride formulas
      bool do_idecode;    // do instruction decoding
      bool do_linemap;    // map metrics to source code
      bool no_pid;        // do not include pid in output file name
      bool dump_xml;      // generate XML in hpcviewer format
      bool units_usage;   // compute machine units usage, output in XML format
      bool has_mrd;       // we have at least one MRD file
      bool no_scheduling; // do not compute scheduling even if we have a machine file
      bool do_ref_scope_index; // assign unique index to scopes and mem references inside each image
      bool detailed_metrics; // output additional performance metrics with dubious value

      bool do_mycfgpaths;
      
    
      double threshold;   // a scope must contribute more than this fraction of any performance metric 
                          // to be included in the output database.
    //ozgurS
      bool palm_dump_flag_set;
      std::string palm_dump_file;
    //ozgurE

      
      string machine_file;
      string output_file_prefix;
      string memory_latency_file;
      string stream_file;
      string debug_routine;
      int    debug_parts;   // split routine CFG into these many parts based on rank when drawing
      string cfg_file;
      string palm_cfg_file;
      string persistent_cache;
      string exec_path;
      string exec_name;
      string scope_name;
      MrdFileMap mrdFiles;

      string binary_path;
      string block_path;
      string func_name;
      string lat_path;
      string fp_path; //footprint profile
      
      MiamiOptions() {
         do_scheduling = false;
         do_fp = false;
         has_mdl = false;
         do_cfgcounts = false;
         do_scopetree = true;
         do_cfgpaths = false;
         do_build_dg = false;
         do_ideal_sched = false;
         do_streams = false;
         do_imix = false;
         do_iwmix = false;
         do_ibins = false;
         do_memlat = false;
         do_staticmem = false;
         do_idecode = false;
         do_linemap = true;
         no_pid = false;
         dump_xml = false;
         units_usage = false;
         has_mrd = false;
         no_scheduling = false;
         do_ref_scope_index = false;
         detailed_metrics = false;
         debug_parts = 1;
         palm_dump_flag_set = false;
         threshold = 0.0;
         
         // make default exec name and path in case we cannot open the first image??
         exec_path = ".";
         exec_name = "unknown-binary";

         do_mycfgpaths = false;
      }
      
      bool CheckDependencies() {
         bool is_good = true;
         if (do_cfgcounts && !((palm_cfg_file.length() || cfg_file.length()) || (binary_path.length() && func_name.length()))) {
            // we do not have a cfg file
            fprintf(stderr, "Some of the specified options require a CFG file. Repeat run with a valid CFG file.\n");
            is_good = false;
         }
         if (do_fp){
            is_good = true;
         } else if ((do_scheduling || has_mrd) && !has_mdl) {
            // we must compute instruction schedule but we lack a machine description file
            fprintf(stderr, "Some of the specified options require a machine description file. Repeat run with a valid MDF file.\n");
            is_good = false;
         }
         if ((do_scheduling || do_ideal_sched || do_streams || has_mrd || do_ref_scope_index) && !do_scopetree) {
            // we must compute the instruction schedule but we were asked not to build the scope tree
            fprintf(stderr, "We were asked not to build the scope tree, however, that option is incompatible with the request to build the instruction schedule, the ideal ILP, the stream concurrency, or the data reuse analysis.\n");
            is_good = false;
         }
         if ((units_usage) && !do_scheduling) {
            // we must compute the instruction schedule but we were asked not to build the scope tree
            fprintf(stderr, "We were asked to compute machine resources usage, but instruction scheduling is required for this.\n");
            is_good = false;
         }
         if ((do_ref_scope_index) && !do_idecode) {
            // we must compute instruction / scope indecies but we are not doing instruction decoding?? 
            fprintf(stderr, "We were asked to compute reference and scope indecies, but instruction decoding is required for this.\n");
            is_good = false;
         }
         
         // Assert that we set correctly other dependencies
         if (is_good)
         {
            assert((!do_scheduling && !do_ideal_sched) || do_build_dg);
            assert(!do_build_dg || (do_cfgpaths || do_mycfgpaths));
            assert(!do_cfgpaths || do_scopetree);
            assert(!do_staticmem || do_scopetree);
         }
         
         return (is_good);
      }
      
      void addMachineFile(const string& mfile) {
         if (mfile.length()) {
            machine_file = mfile;
            has_mdl = true;
            if (! no_scheduling)
            {
               do_scheduling = true;
               do_cfgpaths = true;
               do_build_dg = true;
               units_usage = true;
               do_staticmem = true;
               do_ref_scope_index = true;
               
               do_cfgcounts = true;
               do_idecode = true;
               dump_xml = true;
            }
         }
      }
      
      void setOutputThreshold(double fraction) {
//         std::cerr << "Set output threshold to " << fraction << std::endl;
         if (fraction < 0.0) fraction = 0.0;
         if (fraction > 1.0) fraction = 1.0;
         threshold = fraction;
      }
      
      double getOutputThreshold() const  { return (threshold); }
      
      // I must set this flag first
      void setNoScheduling(bool no_sched) {
         no_scheduling = no_sched;
      }

      void addScopeName(const string& sname) {
         scope_name = sname;
      }
      
      const string& getScopeName() const {
         return (scope_name);
      }
      
      void setDetailedMetrics(bool _dm) {
         detailed_metrics = _dm;
      }

      bool DetailedMetrics() const {
         return (detailed_metrics);
      }

      void addExecutablePath(const string& epath) {
         exec_path = epath;
      }
      
      const string& getExecutablePath() const {
         return (exec_path);
      }
      
      void addExecutableName(const string& ename) {
         exec_name = ename;
      }
      
      const string& getExecutableName() const {
         return (exec_name);
      }
      
      void addPersistentCache(const string& db) {
         if (db.length())
            persistent_cache = db;
         else
         {
            char* db_dir = getenv("MIAMI_PERSISTENT_DB");
            if (db_dir)
               persistent_cache = db_dir;
         }
      }
      
      void addCfgFile(const string& cfile) {
         if (cfile.length())
            cfg_file = cfile;
      }
       
      void addPalmCfgFile(const string& cfile) {
         if (cfile.length())
            palm_cfg_file = cfile;
      }
      
      void setNoPid(bool np) {
         no_pid = np;
      }

      void setDumpXML(bool _xml) {
         dump_xml = _xml;
      }

      void setNoLinemap(bool nlm) {
         do_linemap = !nlm;
      }
      
      void setNoScopeTree(bool nst) {
         do_scopetree = !nst;
      }
      
      void addDebugRoutine(const string& dname) {
         if (dname.length())
            debug_routine = dname;
      }
      
      void addDebugParts(int _parts) {
         debug_parts = _parts;
      }
      
      void addOutputFilePrefix(const string& oname) {
         if (oname.length())
            output_file_prefix = oname;
      }
      
      void addMemoryLatencyFile(const string& mfile) {
         if (mfile.length()) {
            memory_latency_file = mfile;
            do_memlat = true;
            do_scheduling = true;
            do_ref_scope_index = true;
            units_usage = true;
            do_cfgcounts = true;
            do_cfgpaths = true;
            do_idecode = true;
            do_build_dg = true;
            do_staticmem = true;
         }
      }
      
      void addMrdFile(const string& mfile)
      {
         if (mfile.length()) {
            // Open it and try to read the virst 3 fields
            FILE *fd = fopen(mfile.c_str(), "rt");
            if (fd) {
               int lshift;
               addrtype mask, val;
               // Should we allow block sizes larger than 1 GB ??
               if (fscanf(fd, "%d %" PRIxaddr " %" PRIxaddr, &lshift, &mask, &val)!=3 || lshift<0 || lshift>30)
                  fprintf(stderr, "File %s does not have a valid MRD format. Skipping.\n", mfile.c_str());
               else
               {
                  // we'll add this file to the map
                  uint32_t bsize = 1 << lshift;
                  MrdFileMap::iterator mit = mrdFiles.find(bsize);
                  if (mit != mrdFiles.end())
                     fprintf(stderr, "We are asked to add MRD file %s with a block size of %" PRIu32 
                           ", but we already have file %s with the same block size. Skipping this file.\n", 
                           mfile.c_str(), bsize, mit->second.name.c_str());
                  else
                  {
                     mrdFiles.insert(MrdFileMap::value_type(bsize, MrdInfo(mfile, mask, val, bsize)));
                     fprintf(stderr, "Adding MRD file %s with block size %" PRIu32 
                              ", block mask %" PRIxaddr " and block value %" PRIxaddr "\n",
                           mfile.c_str(), bsize, mask, val);
                     has_mrd = true;
                     do_ref_scope_index = true;
                     
                     do_staticmem = true;
                     do_cfgcounts = true;
                     do_idecode = true;
                     dump_xml = true;
                  }
               }
               fclose(fd);
            } else  // cannot open the file
            {
               fprintf(stderr, "Cannot open MRD file %s for reading.\n", mfile.c_str());
            }
         }
      }
      
      void addStreamReuseFile(const string& sfile) {
         if (sfile.length()) {
            do_streams = true;
            do_cfgcounts = true;
            do_idecode = true;
            do_staticmem = true;
            stream_file = sfile;
         }
      }
      
      void setDoInstructionMix(bool imix) {
         if (imix) {
            do_imix = true;
            do_ibins = true;
            do_cfgcounts = true;
            do_idecode= true;
         }
      }

      void setDoInstructionWidthMix(bool iwmix) {
         if (iwmix) {
            do_iwmix = true;
            do_ibins = true;
            do_cfgcounts = true;
            do_idecode= true;
         }
      }

      void addBinaryPath(const string& bpath) {
         if (bpath.length())
            binary_path = bpath;
      }

      void addFuncName(const string& fname) {
         if (fname.length())
            func_name = fname;
      }

      void addLatPath(const string& lpath) {
         if (lpath.length())
            lat_path = lpath;
      }
      
      void addFpPath(const string& fppath) {
         if (fppath.length())
            fp_path = fppath;

         if (! no_scheduling)
         {
            do_fp = true;
            do_cfgpaths = true;
            do_build_dg = true;
            do_ref_scope_index = true;
            
            do_cfgcounts = true;
            do_idecode = true;
            dump_xml = true;
         }

      }
   
     void addDumpFile(const string& dfile)
     {
        if (dfile.length()){
            palm_dump_file = dfile;
            palm_dump_flag_set = true; 
        } else {
            palm_dump_flag_set = false;
        }
    
     }
    
     std::string getDumpFile() {return palm_dump_file;}
     bool isDumpFlagSet() {return palm_dump_flag_set;}
      void addBlockPath(const string& blkPath){
         if (blkPath.length()){
            block_path = blkPath;
            do_mycfgpaths=true;
            do_cfgpaths=false;
         }
      }
   };

}

#endif
