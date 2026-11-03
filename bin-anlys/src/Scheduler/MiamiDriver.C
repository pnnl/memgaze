/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: MiamiDriver.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the main control logic for the MIAMI scheduler.
 */

#include <fstream>
#include <string>

#include "MiamiDriver.h"
#include "scope_implementation.h"
#include "InstructionDecoder.hpp"
#include "source_file_mapping.h"
#include "routine.h"
#include "debug_scheduler.h"
#include "report_time.h"
#include "CRCHash.h"
#include "miami_utils.h"

//***************************************************************************
// DynInst includes
//***************************************************************************

#include <CodeObject.h>
#include <CodeSource.h>

#include <BPatch.h>
#include <BPatch_object.h>
#include <BPatch_addressSpace.h>
#include <BPatch_function.h>
#include <BPatch_flowGraph.h>
#include "LineInformation.h"

#include <Function.h>
#include <Instruction.h>

#include <Symtab.h>

//#include "dyninst-insn-xlate.hpp"
//***************************************************************************
//
namespace MIAMI
{

ScopeImplementation* defaultSIP = 0;
extern const char* defaultVarName;

/* ===================================================================== */

static int deleteHistInMap(void *arg0, addrtype pc, void *value)
{
//   fprintf(stderr, "Deleting atg0=%p, pc=0x%" PRIxaddr ", value=%p\n", arg0, pc, (void*)(*(addrtype*)value)
   MemoryLatencyHistogram *mlh = static_cast<MemoryLatencyHistogram*>((void*)(*(addrtype*)value));
   delete (mlh);
   return (0);
}

MIAMI_Driver::~MIAMI_Driver()
{
   uint32_t i;
   if (fstream)
      fclose(fstream);
      
   // delete the Machine list
   MachineList::iterator mlit = targets.begin();
   for ( ; mlit!=targets.end() ; ++mlit)
      delete (*mlit);
   targets.clear();
   
   // deallocate the info stored for each image
   if (allImgs)
   {
      for (i=0 ; i<=maxImgs ; ++i)
         if (allImgs[i])
            delete (allImgs[i]);
      free (allImgs);
   }
   
   if (imgNames)
      delete[] imgNames;
      
   // I must deallocate the histograms stored in refLatencies
   refLatencies.map(deleteHistInMap, NULL);
}

void 
MIAMI_Driver::ParseIncludeExcludeFiles(const std::string& include_file, 
                    const std::string& exclude_file)
{
   Routine::parse_include_exclude_files(include_file, exclude_file);
}

LoadModule* 
MIAMI_Driver::GetModuleWithIndex(int id)
{
   if (allImgs && id>0 && id<=(int)maxImgs)  // Image index in range
      return (allImgs[id]);
   else
      return (0);
}

void
MIAMI_Driver::parse_machine_description (const std::string& machine_file)
{
   Machine* tmach = parseMachineDescription(machine_file.c_str());

   if (mo->has_mrd)
   {
      // if we have MRD information, check if we have data for block sizes
      // equal to the line sizes in the cache hierarchy
      MemLevelAssocTable *memLevels = tmach->getMemoryLevels();
      int numLevels = memLevels->NumberOfElements();
      
      int idx;
      for (idx=0 ; idx<numLevels ; ++idx)
      {
         // just check if we have a MRD file for this level
         MemoryHierarchyLevel *mLevel = memLevels->getElementAtIndex(idx);
         int bsize = mLevel->getBlockSize();
         if (bsize > 0)
         {
            // Check that we do not have a record of it already
            MIAMI_MEM_REUSE::MRDDataVec::iterator vit = mrdData.begin();
            int i = 0, fidx = -1;
            for ( ; vit!=mrdData.end() ; ++vit, ++i)
               if ((*vit)->GetBlockSize() == bsize)  // we already have a record
               {
                  fidx = i;
                  break;
               }
            // if we do not have a record of it, see if it was passed on the command line
            if (fidx < 0)
            {
               MrdFileMap::iterator mit = mo->mrdFiles.find(bsize);
               if (mit != mo->mrdFiles.end())
               {
                  // Found a file, add a record of it to mrdData
                  fidx = mrdData.size();
                  mrdData.push_back(new MIAMI_MEM_REUSE::BlockMRDData(mit->second));
               }
            }
            if (fidx >= 0)
            {
               fprintf(stderr, "Using MRD file %s for memory level %s.\n",
                        mrdData[fidx]->GetFileName().c_str(), mLevel->getLongName());
               mLevel->setMrdFileIndex(fidx);
            } else
            {
               fprintf(stderr, "No data reuse information found for memory level %s with block size %d.\n",
                        mLevel->getLongName(), bsize);
            }
         }
      }
      perform_memory_analysis = true;
   }
   
   targets.push_back(tmach);
}

int
MIAMI_Driver::Initialize(MiamiOptions *_mo, int _pid)
{
    mo = _mo;
    
    pid = _pid;
    if (mo->cfg_file.length()<1 && mo->binary_path.length() < 1 && mo->palm_cfg_file.length()<1 && !mo->printLinemap)  // required argument is missing
    {
       fprintf(stderr, "Required CFG_file_name is missing.\n");
       return (-1);
    }


    if (mo->cfg_file.length()>1){
      
      fd = fopen(mo->cfg_file.c_str(), "rb");
      if (fd == NULL)
      {
         fprintf (stderr, "Cannot access cfg file %s\n", mo->cfg_file.c_str());
         return (-2);
      }
      //  the last 8 bytes of the file contain the offset of the image table
      int res = fseek(fd, -8L, SEEK_END);
      if (res < 0)  // error
      {
         perror ("Changing offset in the CFG file failed");
         return (-3);
      }
      
      // read the offset of the image table
      uint64_t tableOffset;
      res = fread(&tableOffset, 8, 1, fd);
      if (res != 1)
      {
         fprintf (stderr, "Cannot read the table offset from the cfg file %s\n", mo->cfg_file.c_str());
         return (-4);
      }

      // go to start of the table
      res = fseek(fd, tableOffset, SEEK_SET);
      if (res < 0)  // error
      {
         perror ("Changing offset to start of table failed");
         return (-5);
      }
      
      // Now parse the table and populate a map data structure
      res = fread(&maxImgs, 4, 1, fd);
      if (res != 1)
      {
         fprintf (stderr, "Reading the number of images failed\n");
         return (-6);
      }
      
      allImgs = (LoadModule**) malloc ((maxImgs+1) * sizeof (LoadModule*));
      for (uint32_t i=0 ; i<=maxImgs ; ++i)
         allImgs[i] = 0;
      imgNames = new std::string[maxImgs+1];

      for (uint32_t i=0 ; i<maxImgs ; ++i)
      {
         int32_t nlen = 0;
         uint64_t foffset = 0;
         res = fread(&nlen, 4, 1, fd);
         if (res != 1 || nlen<1 || nlen>2048)
         {
            fprintf (stderr, "Reading image %u name length failed\n", i);
            return (-7);
         }
         char *buf = new char[nlen+1];
         res = fread(buf, 1, nlen, fd);
         if (res != nlen)
         {
            fprintf (stderr, "Reading image %u name failed\n", i);
            return (-8);
         }
         buf[nlen] = 0;
         res = fread(&foffset, 8, 1, fd);
         if (res != 1)
         {
            fprintf (stderr, "Reading image %u file offset failed\n", i);
            return (-9);
         }
         imgNames[i] = std::string(buf);
         std::cout <<imgNames[i]<<" test1 "<<(imgNames[i].find("a.out") != std::string::npos)<<std::endl;
         imgFileOffsets.insert(StringOffsetMap::value_type(
             imgNames[i], foffset));
         delete[] buf;
      }
    } else if (mo->palm_cfg_file.length()>1){ //OzgurS adding palm cfg
   std::cout<<__func__<<" line:"<<__LINE__<<std::endl;
      //fd = fopen(mo->palm_cfg_file.c_str(), "rb");
      std::ifstream palm_fd;
      palm_fd.open(mo->palm_cfg_file.c_str());

      if (!palm_fd.is_open())
      {
         fprintf (stderr, "Cannot access palm cfg file %s\n", mo->cfg_file.c_str());
         return (-2);
      }
      maxImgs = 1;
      std::string line;
      std::string bin_path;
      while (! palm_fd.eof()){
   std::cout<<__func__<<" line:"<<__LINE__<<std::endl;
         std::getline(palm_fd , line);
         if (line.find("binary-path") != std::string::npos){
            bin_path =line.substr(line.find(": ")+2, -1);
            break;
         }
      }
      allImgs = (LoadModule**) malloc ((maxImgs+1) * sizeof (LoadModule*));
      for (uint32_t i=0 ; i<=maxImgs ; ++i)
         allImgs[i] = 0;
      imgNames = new std::string[maxImgs+1];
      imgNames[0] = bin_path;
      palm_fd.close();
      std::cout <<imgNames[0]<<" debuug "<<(imgNames[0].find("a.out") != std::string::npos)<<std::endl;
//OzguE
    } else if (mo->printLinemap){
      std::cout << "Nothing needs to initilize for Readin Linemap\n"; 
    }else { // a binary file was specified instead of a cfg
      maxImgs = 1;
      allImgs = (LoadModule**) malloc ((maxImgs+1) * sizeof (LoadModule*));
      for (uint32_t i=0 ; i<=maxImgs ; ++i)
         allImgs[i] = 0;
      imgNames = new std::string[maxImgs+1];
      imgNames[0] = mo->binary_path;
      std::cout <<imgNames[0]<<" test2 "<<(imgNames[0].find("a.out") != std::string::npos)<<std::endl;
    }
    
    /* create the root of the program scope tree */
    prog = new ProgScope("Whole Program", 0);

    /* parse machine file */
    if (mo->has_mdl)  // machine file is specified
       parse_machine_description(mo->machine_file);

    /* parse memory file */
    if (mo->do_memlat)  // machine file is specified
       parse_memory_latencies(mo->memory_latency_file);
    
    /* initialize the architecture dependent instruction decoding */
    if (mo->do_idecode)
       instruction_decoding_init(NULL);
    
    /* initialize the architecture dependent source code mapping */
    if (mo->do_linemap)
       MIAMIP::source_file_info_init();
    
    /* check if we must aggregate stream reuse */
    if (mo->do_streams)
    {
       parse_stream_reuse_data(mo->stream_file);
       // should create the detailed, instruction level file only if 
       // we can map instruction to source file
       if (mo->do_linemap && mo->do_scopetree)
       {
          assert (mo->stream_file.length()>0);
          std::string fname = mo->stream_file + "out";
       
          fstream = fopen(fname.c_str(), "wt");
          if (fstream == NULL)
          {
             fprintf (stderr, "Cannot open output stream file %s, writing to stdout.\n", fname.c_str());
             fstream = stdout;
          }
       }
    }

    return (0);  // everything is OK with the world
}

void
MIAMI_Driver::Finalize(const std::string& outFile)
{
   // now write all the performance data into fout
  std::cerr << "[INFO]MIAMI_Driver::Finalize\n";
   Machine *tmach = 0;
   if (mo->has_mdl)
      tmach =  targets.front();
   
   // I will need to compute the memory statistics before doing the aggregation
   // For now, I am reading latency histograms and I apply the mechanistic model
   // after processing each scope
   // gmarin, 08/06/2013: I am now able to parse MRD files for the most part.
   // If we perform memory analysis, parse the MRD files here
   if (perform_memory_analysis) 
   {
      // Iterate over all MRDData elements, and parse the corresponding file.
      fprintf(stderr, "Parsing the MRD files ...\n");
      MIAMI_MEM_REUSE::MRDDataVec::iterator mit = mrdData.begin();
      for ( ; mit!=mrdData.end() ; ++mit)
      {
         fprintf(stderr, "Parsing MRD file %s\n", (*mit)->GetFileName().c_str());
         (*mit)->ParseMrdFile();
      }
#if PROFILE_SCHEDULER
      MIAMIP::report_time (stderr, "Parse MRD files");
#endif
      // We filled up some detastructures inside each BlockMRDData 
      // instance.
      // Now, we need to traverse all scopes and add relevant memory 
      // information
      if (tmach)
         compute_memory_information(tmach);
   }
   
   // aggregate all metrics bottom-up
   aggregate_counts_for_scope (prog, tmach, no_fpga_acc);

   // print results into the specified output file
   std::string fname = mo->output_file_prefix;
   char tmp[32] = {""};
   if (mo->printLinemap)
      return ;
   if (fname.length()<1)  // output file name not specified, use default
   {
      LoadModule *img1 = allImgs [1];
      assert (img1 || ! "Fini: We have no data about the image with id 1");
      sprintf(tmp, "-%05d", pid);
      fname = MIAMIU::StripPath(img1->Name());
      if (tmach)
         fname = fname + "-" + targets.front()->MachineName();
   } else  // we have an output file name specified
   {
      // check if it has the .xml extension already. We want the name without the extension
      if (fname.length()>=4 && !fname.compare (fname.length()-4, 4, ".xml"))
         // shorten the string
         fname.erase(fname.length()-4);
   }
   if (! mo->no_pid)
      fname = fname + tmp;

   double threshold = mo->getOutputThreshold();
   if (mo->dump_xml)
   {
      FILE *fout = fopen((fname+".xml").c_str(), "wt");
      if (fout == NULL)
      {
         fprintf (stderr, "Cannot open output file %s.xml, writing to stdout.\n", fname.c_str());
         fout = stdout;
      }
      dump_xml_output(fout, prog, fname, threshold);
      if (fout != stdout)
         fclose(fout);
      
      if (compute_reuse)   // print the reuse pairs and the flat view
      {
         fout = fopen((fname+"-flat.xml").c_str(), "wt");
         if (fout == NULL)
         {
            fprintf (stderr, "Cannot open output file %s-flat.xml, writing to stdout.\n", fname.c_str());
            fout = stdout;
         }
         dump_xml_output(fout, prog, fname, threshold, true);
         if (fout != stdout)
            fclose(fout);
         
         fout = fopen((fname+"-reuse.xml").c_str(), "wt");
         if (fout == NULL)
         {
            fprintf (stderr, "Cannot open output file %s-reuse.xml, writing to stdout.\n", fname.c_str());
            fout = stdout;
         }
         dump_scope_pairs_xml_output (fout, prog, fname+" (REUSE)", threshold);
         // deallocate the scopePairs map right here   
         PairSRDMap::iterator psit = reusePairs.begin();
         for ( ; psit!=reusePairs.end() ; ++psit)
            delete (psit->second);
         reusePairs.clear();
         if (fout != stdout)
            fclose(fout);
      }
   }
   
   if (mo->units_usage)
   {
      // aggregate unit usage information bottom-up
      perform_memory_analysis = false;
      aggregate_resources_used_for_scope (prog, tmach);
      std::string funits = fname + "-units";

      FILE *fout = fopen((fname+"-units.xml").c_str(), "wt");
      if (fout == NULL)
      {
         fprintf (stderr, "Cannot open output file %s-units.xml, writing to stdout.\n", fname.c_str());
         fout = stdout;
      }
      dump_xml_output(fout, prog, funits, threshold);
      if (fout != stdout)
         fclose(fout);
   }
   
   if (mo->do_streams && !mo->dump_xml)
   {
      FILE *fout = fopen((fname+"-srd.csv").c_str(), "wt");
      if (fout == NULL)
      {
         fprintf (stderr, "Cannot open output file %s-srd.csv, writing to stdout.\n", fname.c_str());
         fout = stdout;
      }
      dump_stream_count_histograms(fout, prog);
      if (fout != stdout)
         fclose(fout);
   }

   if (mo->do_ibins)
   {
      ScopeImplementation *scope = 0;
      // Check if we dump only the counts for a particular scope
      if (mo->scope_name.length()>0)
      {
         // recursively traverse the tree of scopes and compare scope XML 
         // names against the "scope_name."
         scope = prog->FindScopeByName(mo->scope_name);
      }
      if (!scope)
         scope = prog;
      
      // first, dump the fine grain instruction classes histograms
      FILE *fout = fopen((fname+"-ibins.csv").c_str(), "wt");
      if (fout == NULL)
      {
         fprintf (stderr, "Cannot open output file %s-ibins.csv, writing to stdout.\n", fname.c_str());
         fout = stdout;
      }
      dump_ibins_histograms(fout, scope);
      if (fout != stdout)
         fclose(fout);

      // dump the coarser instructions mix histograms if requested
      if (mo->do_imix)
      {
         fout = fopen((fname+"-imix.csv").c_str(), "wt");
         if (fout == NULL)
         {
            fprintf (stderr, "Cannot open output file %s-imix.csv, writing to stdout.\n", fname.c_str());
            fout = stdout;
         }
         dump_imix_histograms(fout, scope, false);
         if (fout != stdout)
            fclose(fout);
      }

      // finally, dump the instruction / width mix histograms if requested
      if (mo->do_iwmix)
      {
         fout = fopen((fname+"-iwmix.csv").c_str(), "wt");
         if (fout == NULL)
         {
            fprintf (stderr, "Cannot open output file %s-iwmix.csv, writing to stdout.\n", fname.c_str());
            fout = stdout;
         }
         dump_imix_histograms(fout, scope, true);
         if (fout != stdout)
            fclose(fout);
      }
   }
}

void MIAMI_Driver::printLinemapInfo(){
        std::cout<<"PrintLinemap= "<<mo->printLinemap<<std::endl;
     	std::string file = mo->linemapFile;
       	std::cout<<"Starting to Pring Line map for "<<file<<std::endl;
       	Dyninst::SymtabAPI::Symtab *obj = NULL;
      	bool err = Dyninst::SymtabAPI::Symtab::openFile(obj, file);
        if (err){
          std::cout<<"Error opening object file with SymtabAPI"<<std::endl;
        }
       	Dyninst::SymtabAPI::Module mod;
       	vector<Dyninst::SymtabAPI::Module *> ret;
       	//obj->findModuleByName(mod, "");
       	obj->getAllModules(ret);
       	Dyninst::SymtabAPI::LineInformation * lineInformation = NULL;
       	//Dyninst::SymtabAPI::StringTablePtr * strptr;
       	for (std::vector<Dyninst::SymtabAPI::Module *>::iterator it = ret.begin() ; it != ret.end(); ++it){
		std::cout<<"LINEINFO: "<<(*it)->parseLineInformation()<<std::endl;//OZGURDBG LINEMAP TEST
          	lineInformation = (*it)->parseLineInformation();
          	lineInformation->dump();
 //         lineInformation->getSize();
 //         Dyninst::SymtabAPI::LineInformation::dump();
 //         strptr = lineInformation->getStrings();
       	}
       	std::cout<<"Exiting the Program!!! "<<std::endl; 
  //    }
      return;
}

void MIAMI_Driver::LoadImage(uint32_t id, std::string &iname, addrtype start_addr, addrtype low_offset) {
  std::cerr << "[INFO] MIAMI_Driver::LoadImage " << iname << std::hex << " " << start_addr << " " << low_offset
            << std::dec << endl;
#if DEBUG_CFG_COUNTS
  DEBUG_CFG(1, fprintf(stderr,
                       "MIAMI_Driver::LoadImage called for id %u, name %s, start_addr 0x%" PRIxaddr
                       ", low_offset 0x%" PRIxaddr "\n",
                       id, iname.c_str(), start_addr, low_offset);
            fflush(stderr);)
#endif

  if (id > maxImgs) { // I need to extend my array
    uint32_t oldSize = maxImgs;
    while (maxImgs < id)
      maxImgs <<= 1;
    allImgs = (LoadModule **)realloc(allImgs, (maxImgs + 1) * sizeof(LoadModule *));
    for (uint32_t i = oldSize + 1; i <= maxImgs; ++i)
      allImgs[i] = 0;
  }
  LoadModule *&newimg = allImgs[id];
  if (newimg) {
    assert(!"Loading image with id seen before");
  }
  // check if we have any data about this image in the CFG file
  StringOffsetMap::iterator sit = imgFileOffsets.find(iname);
  if (sit != imgFileOffsets.end()) {
    //  only process images for which we've collected data before
    //  now go and read the info for this image
    int res = fseek(fd, sit->second, SEEK_SET);
    if (res < 0) { // error
      perror("Changing offset to start of image failed");
      return;
    }
    // compute a CRC32 checksum for this image file if we need to perform static
    // memory analysis. The CRC is used to check the integrity of the cached values
    uint32_t hashKey = 0;
    if (mo->do_staticmem) {
      FILE *img_fd = fopen(iname.c_str(), "rb");
      if (img_fd) {
        MIAMIU::CRC32Hash crc32;
        crc32.UpdateForFile(img_fd);
        hashKey = crc32.Evaluate();
        fclose(img_fd);
      } else {
        cerr << "[LOG] WARNING: Cannot open image file " << iname << " for reading. Cannot compute its checksum."
             << endl;
      }
    }
    newimg = new LoadModule(id, start_addr, low_offset, iname, hashKey);
    ++loadedImgs;
    // TODO FIXME:dyninst I commented out newimg->loadFromFile(fd, false) to force
    // control flow to fetch cfg from dynisnt
    //  read only data for this image.
    if (mo->do_fp && mo->palm_cfg_file.length() > 1) {
      newimg->palmLoadFromFile(mo);
      newimg->createDyninstImage(bpatch);
      newimg->loadFPfile(mo->func_name, prog, mo);
      newimg->palmAnalyzeRoutines(prog, mo);
    } else if (mo->load_classes) {
      newimg->loadFromFile(fd, false); // do not parse routines now
      newimg->createDyninstImage(bpatch);
      
      // From here I planing to follow v1 path to create routines vy using dyninst.
      newimg->dyninstAnalyzeRoutines(fd, prog, mo);
      
      // write this image back to a file
      BPatch_binaryEdit *BPapp = static_cast<BPatch_binaryEdit *>(newimg->getDyninstApp());
      std::string newName;
      
      if (mo->outBinName.size() > 0) {
        newName = mo->outBinName;
      } else {
        newName = iname + "-memgaze";
      }
            
      Dyninst::PatchAPI::Patcher *patcher = newimg->getPatcher();
      // Dyninst::PatchAPI::PatchMgrPtr patchMgr = Dyninst::PatchAPI::convert(BPapp);
      // Dyninst::PatchAPI::Patcher patcher(patchMgr);
      if (!patcher->commit()) {
        std::cout << "[LOG] ERROR PATCHER IS NOT WORKING" << std::endl;
      }
      BPapp->writeFile(newName.c_str());
      std::cout << "[LOG] wrote to a file " << newName << " image: " << iname << std::endl;
    } else {
      std::cout << "[LOG] old cfg do fp:" << mo->do_fp << " fp file size:" << mo->fp_path.length() << std::endl;
      newimg->loadFromFile(fd, false); // do not parse routines now
      newimg->createDyninstImage(bpatch);
      // FIXME:dyninst TODO
      // newimg->dyninstAnalyzeRoutine(mo->func_name, prog, mo);
      newimg->loadFPfile(mo->func_name, prog, mo);
      // Now go and analyze each routine; compute counts for all blocks and edges,
      // recover executed paths, attempt to decode and schedule the instructions
      // if string not empty, dump CFG of this routine
      newimg->analyzeRoutines(fd, prog, mo);
      // TODO lets try this here too.
      // newimg->dyninstAnalyzeRoutine(mo->func_name, prog, mo);
    }
  } else {
    uint32_t hashKey = 0;
    if (mo->do_staticmem) {
      FILE *img_fd = fopen(iname.c_str(), "rb");
      if (img_fd) { // I was able to open it, compute its CRC32 hash
        MIAMIU::CRC32Hash crc32;
        crc32.UpdateForFile(img_fd);
        hashKey = crc32.Evaluate();
        fclose(img_fd);
      } else {
        cerr << "[LOG] ERROR Cannot open image file " << iname << " for reading. Cannot compute its checksum." << endl;
      }
    }

    newimg = new LoadModule(id, start_addr, low_offset, iname, hashKey);
    ++loadedImgs;
    if (mo->do_fp && mo->palm_cfg_file.length() > 1) {
      std::cout << "[LOG] in do fp:" << mo->do_fp << " file size:" << mo->palm_cfg_file.length() << std::endl;
      newimg->palmLoadFromFile(mo);
      newimg->createDyninstImage(bpatch);
      newimg->loadFPfile(mo->func_name, prog, mo);
      newimg->palmAnalyzeRoutines(prog, mo);
    } else if (mo->load_classes) {
      // newimg->loadFromFile(fd, false);  // do not parse routines now
      newimg->createDyninstImage(bpatch);
      BPatch_binaryEdit *BPapp = static_cast<BPatch_binaryEdit *>(newimg->getDyninstApp());
      Dyninst::PatchAPI::PatchMgrPtr patchMgr = Dyninst::PatchAPI::convert(BPapp);
      Dyninst::PatchAPI::Patcher patcher(patchMgr);
      newimg->dyninstAnalyzeRoutines(prog, mo, &patcher);
      // newimg->dyninstAnalyzeRoutines(prog, mo);
      std::string newName;
      if (mo->outBinName.size() > 0) {
        newName = mo->outBinName;
      } else {
        newName = iname + "-memgaze";
      }
      if (!patcher.commit()) {
        std::cout << "[LOG] ERROR PATCHER IS NOT WORKING" << std::endl;
      } else {
        std::cout << "[LOG] DID I COMMIT???" << std::endl;
      }
      BPapp->writeFile(newName.c_str());
      std::cout << "[LOG] Wrote to a file named " << newName << " image: " << iname << std::endl;
    } else {
      newimg->createDyninstImage(bpatch);
      newimg->dyninstAnalyzeRoutine(mo->func_name, prog, mo);
    }
  }
}

void
MIAMI_Driver::aggregate_counts_for_scope (ScopeImplementation *pscope,
       Machine *tmach, int no_fpga_acc)
{
   int i;
   // first compute exclusive time stats as the sum over all local paths
   BPMap* bpm = pscope->Paths();
   TimeAccount& stats = pscope->getExclusiveTimeStats();
   if (bpm != NULL)
   {
      BPMap::iterator bit = bpm->begin();
      for( ; bit!=bpm->end() ; ++bit )
      {
         PathInfo *_path = bit->second;
//         _path->timeStats.computeBandwidthTime (tmach);
         stats += (_path->timeStats * _path->count);
         pscope->addExclusiveSerialMemLat (_path->serialMemLat);
         pscope->addExclusiveExposedMemLat (_path->exposedMemLat);
      }
   }
   // we have memory hierarchy metrics at scope level only.
   // when/if we add them to the path level, compute bandwidth 
   // requirements and delays per path (above), and comment this call
   if (perform_memory_analysis && tmach) 
      stats.computeBandwidthTime (tmach);

   ScopeImplementation::iterator it;
   pscope->getInclusiveTimeStats() = stats;
   
   // copy also the miss counts per name; I need to handle this explicitly
   // because the values are pointers to arrays of doubles
   CharDoublePMap::iterator cit;
   CharDoublePMap &exclNameVals = pscope->getExclusiveStatsPerName();
   CharDoublePMap &inclNameVals = pscope->getInclusiveStatsPerName();
   int numLevels = 0, numLevels3 = 0;
   if (tmach)
   {
      MemLevelAssocTable *memLevels = tmach->getMemoryLevels ();
      numLevels = memLevels->NumberOfElements ();
      numLevels3 = numLevels + numLevels + numLevels;
      for (cit=exclNameVals.begin() ; cit!=exclNameVals.end() ; ++cit)
      {
         // I just start copying stuff in this map. I know it is empty when
         // I start, and I cannot find duplicate keys in the input map
         double *vals = new double[numLevels3];
         memcpy (vals, cit->second, numLevels3*sizeof(double));
         inclNameVals.insert (CharDoublePMap::value_type (cit->first,
                   vals));
      }
   }
   
   for( it=pscope->begin() ; it!=pscope->end() ; it++ )
   {
      ScopeImplementation *sci = 
               dynamic_cast<ScopeImplementation*>(it->second);
      aggregate_counts_for_scope(sci, tmach, no_fpga_acc);
      
      // the latency and inefficiency must be aggregated
      pscope->Latency() += sci->Latency();
      pscope->NumUops() += sci->NumUops();
//      pscope->Inefficiency() += sci->Inefficiency();
      pscope->NumLoads() += sci->NumLoads();
      pscope->NumStores() += sci->NumStores();
      pscope->getInclusiveTimeStats() += sci->getInclusiveTimeStats ();
      pscope->addInnerSerialMemLat (sci->getSerialMemLat ());
      pscope->addInnerExposedMemLat (sci->getExposedMemLat ());
      
      // add the values per name from the inner scopes
      if (tmach)
      {
         CharDoublePMap &subInclVals = sci->getInclusiveStatsPerName();
         for (cit=subInclVals.begin() ; cit!=subInclVals.end() ; ++cit)
         {
            CharDoublePMap::iterator cit2 = inclNameVals.find (cit->first);
            if (cit2 != inclNameVals.end())
               for (i=0 ; i<numLevels3 ; ++i)
                  cit2->second[i] += cit->second[i];
            else
            {
               double *vals = new double[numLevels3];
               memcpy (vals, cit->second, numLevels3*sizeof(double));
               inclNameVals.insert (CharDoublePMap::value_type (cit->first,
                      vals));
            }
         }
      }
      
      // add stream reuse information
      pscope->getScopeStreamInfo() += sci->getScopeStreamInfo();
      
      // aggregate instruction mixes as well
      pscope->getInstructionMixInfo() += sci->getInstructionMixInfo();
   }
   
   // I need to use the inclusive NoDependency time when computing potential
   // for improvement from FPGA. For this reason, I have to do it here when
   // I compute the inclusive metrics. 
   if (!no_fpga_acc)
   {
      double inclCost = pscope->getInclusiveTimeStats().getNoDependencyTime ();
      pscope->getInclusiveTimeStats().addNonFpgaComputeTime (inclCost);
      stats.addNonFpgaComputeTime (inclCost);
   }
}

void
MIAMI_Driver::aggregate_resources_used_for_scope (ScopeImplementation *pscope, Machine *tmach)
{
   // first compute exclusive time stats as the sum over all local paths
   BPMap* bpm = pscope->Paths();
   TimeAccount& stats = pscope->getExclusiveTimeStats();
   stats.getData().clear();
   if (bpm != NULL)
   {
      BPMap::iterator bit = bpm->begin();
      for( ; bit!=bpm->end() ; ++bit )
      {
         PathInfo *_path = bit->second;
         stats += (_path->unitUsage * _path->count);
         _path->timeStats = _path->unitUsage;
      }
   }

   ScopeImplementation::iterator it;
   pscope->getInclusiveTimeStats() = stats;
   
   for( it=pscope->begin() ; it!=pscope->end() ; it++ )
   {
      ScopeImplementation *sci = 
               dynamic_cast<ScopeImplementation*>(it->second);
      aggregate_resources_used_for_scope(sci, tmach);
      
      pscope->getInclusiveTimeStats() += sci->getInclusiveTimeStats ();
   }
}

void
MIAMI_Driver::compute_memory_information(Machine *tmach)
{
   int i;
   MemLevelAssocTable *memLevels = tmach->getMemoryLevels();
   int numLevels = memLevels->NumberOfElements();
   // after all paths are processed I need to traverse the entire scope tree
   // again to add the memory info. I could not do this when analyzing each 
   // scope because it requires some data from other loops.
   mem_level_has_carried_info = new int[numLevels];
   totalCarried = new double[numLevels];
   mem_level_has_irreg_carried_info = new int[numLevels];
   totalIrregCarried = new double[numLevels];
   mem_level_has_frag_info = new int[numLevels];
   totalFragMisses = new double[numLevels];
   for (i=0 ; i<numLevels ; ++i)
   {
      mem_level_has_carried_info[i] = 0;
      mem_level_has_irreg_carried_info[i] = 0;
      mem_level_has_frag_info[i] = 0;
      totalCarried[i] = 0.0;
      totalIrregCarried[i] = 0.0;
      totalFragMisses[i] = 0.0;
   }
   
   // For each memory level, check if we have an associated mrd file
   // and compute the memory information for that level if we do
   for (i=0 ; i<numLevels ; ++i)
   {
      // just check if we have a MRD file for this level
      MemoryHierarchyLevel *mLevel = memLevels->getElementAtIndex(i);
      int fidx = mLevel->getMrdFileIndex();
      if (fidx >= 0)  // we have an associated file
      {
         mrdData[fidx]->ComputeMemoryEventsForLevel(i, numLevels, mLevel,
              totalCarried[i], totalIrregCarried[i], 
              totalFragMisses[i]);

         if (totalCarried[i] > 1.0)
         {
            mem_level_has_carried_info[i] = 1;
            compute_reuse = true;
         }
         if (totalIrregCarried[i] > 1.0)
            mem_level_has_irreg_carried_info[i] = 1;
         if (totalFragMisses[i] > 1.0)
            mem_level_has_frag_info[i] = 1;
      }
   }
   
//   addMemoryDataForScope (prog);
   
   // I need to add footprint info and number of carried misses for each scope
//   addFootPrintDataForScopes (prog, no_fpga_acc);
   
   
#if PROFILE_SCHEDULER
   MIAMIP::report_time (stderr, "Compute memory and foot-print information");
#endif
}

#if 0
void
MIAMI_Driver::addMemoryDataForScope (ScopeImplementation *pscope)
{
   Machine *tmach = targets.front ();
   MemLevelAssocTable *memLevels = tmach->getMemoryLevels ();
   int numLevels = memLevels->NumberOfElements ();
   
#if VERBOSE_DEBUG_MEMORY
   DEBUG_MEM (2,
      cerr << "In addMemoryDataForScope, Scope " << pscope->ToString()
           << ", num memory levels=" << numLevels << endl;
   )
#endif
   // process my local paths first
   MIAMIU::TrioDoublePMap &reuseMap = pscope->getReuseFromScopeMap();
   BPMap* bpm = pscope->Paths();
   if (bpm != NULL)
   {
      BPMap::iterator bit = bpm->begin();
      for( ; bit!=bpm->end() ; ++bit )
      {
         PathInfo *_path = bit->second;
#if VERBOSE_DEBUG_MEMORY
         DEBUG_MEM (6,
            cerr << "Processing reuse, found path " << hex << _path->pathId
                 << dec << " with count " << _path->count << endl;
         )
#endif
         MLDList::iterator mit = _path->memData.begin ();
         for ( ; mit!=_path->memData.end() ; ++mit )
         {
            MemoryHierarchyLevel *mhl = memLevels->getElementAtIndex 
                      (mit->levelIndex);
            mhl->fillMissCounts (_path->pathId, mit->set2MissCount,
                     mit->totalMissCount, numLevels, 
                     reuseMap, mit->levelIndex);
            // for now I will use only the totalMissCount value for a path
            // add the info to the TimeAccount?
            // add the miss count per iteration
            _path->timeStats.addMissCountLevel (mit->levelIndex, 
                   mit->totalMissCount/_path->count, tmach);
         }
      }
   }
   
   // add carry information here by processing the reuseMap
   // Also, I would like to compute how many misses are the result of 
   // irregular accesses.
   unsigned int destId = pscope->getScopeId ();
#if VERBOSE_DEBUG_MEMORY
   DEBUG_MEM (4,
      cerr << "  destId = " << destId << endl;
   )
#endif
   // However, when I use pmemmix and I additionally aggregate sets, 
   // some set that has references in this scope may be associated with a
   // different scope (the main scope of this super-aggregated set). I should
   // read the scope id from setsTable instead of using destId. I also have to
   // find the ScopeImplementation* for that scope instead of using pscope
   // automatically.
   MIAMIU::TrioDoublePMap::iterator rit = reuseMap.begin ();
   int numLevels2 = numLevels + numLevels;
   int numLevels3 = numLevels2 + numLevels;
   MIAMIU::UiDoubleMap &fragFactors = pscope->getFragFactorsForScope ();
   MIAMIU::UiDoubleMap::iterator dit;
   TimeAccount &exclStats = pscope->getExclusiveTimeStats ();
   CharDoublePMap &exclByName = pscope->getExclusiveStatsPerName ();
   CharDoublePMap::iterator cit;
   
   for ( ; rit!=reuseMap.end() ; ++rit)
   {
      int j;
      unsigned int fullCarrierId = rit->first.second;
      // carrierId encodes both the id of the scope (31 bits) and if the reuse
      // is loop independent or loop carried (Least significant bit)
      unsigned int loopIndep = rit->first.second & 0x1;
      unsigned int carrierId = rit->first.second >> 1;
      unsigned int sourceId = rit->first.first;
      unsigned int setIdx = rit->first.third;

      unsigned int realDestId = destId;
      ScopeImplementation *destScope = pscope;
      int idxVal = setsTable.indexOf (setIdx);
      assert (idxVal >= 0);
      unsigned int set_scope = setsTable.atIndex (idxVal);
      if (destId != set_scope)
      {
         realDestId = set_scope;
         int idxScope = scopesMap.indexOf (set_scope);
         assert (idxScope >= 0);
         destScope = scopesMap.atIndex (idxScope);
      }
      
#if VERBOSE_DEBUG_MEMORY
      DEBUG_MEM (5,
         cerr << "    fullCarrierId=" << fullCarrierId << ", sourceId=" << sourceId 
              << " and setIdx=" << setIdx << " part of destScope="
              << realDestId << endl;
      )
#endif
      // check if we have fragmentation factor computed for this set
      // I need to check the map from pcost, and not destScope
      dit = fragFactors.find (setIdx);
      if (dit!=fragFactors.end() && dit->second>0.0)
      {
         rit->second[numLevels+1] = dit->second;
         for (j=0 ; j<numLevels ; ++j)
            if (rit->second[j]>0.0)
            {
               double fragMisses = rit->second[j]*dit->second;
               totalFragMisses[j] += fragMisses;
               exclStats.addFragMissCountLevel (j, fragMisses);
            }
      }
      const char* set_name = defaultVarName;
      int idxName = refNames.indexOf (setIdx);
      if (idxName >= 0)
         set_name = refNames.atIndex (idxName);
      
      if (carrierId && scopesMap.is_member (carrierId))
      {
         assert (sourceId);
         ScopeImplementation *cscope = scopesMap[carrierId];
         
         if (compute_reuse)
         {
            MIAMIU::UIPair tempP (sourceId, destId);
            PairSRDMap::iterator srit = scopePairs.find (tempP);
            ScopeReuseData *srd = 0;
            if (srit == scopePairs.end())
            {
               srd = new ScopeReuseData (numLevels);
               scopePairs.insert (PairSRDMap::value_type (tempP, srd));
            } else
               srd = srit->second;
            
            // now determine if this set has irregular access with respect
            // to the carrying scope. First I have to determine the distance
            // between the set's scope and the carrying scope. Possible values
            // are -1 if they are not in an Ancestor relationship in the same
            // routine. Otherwise it is a value >=0 representing how many
            // scopes away is the carrying scope. Zero means the carrying 
            // scope is the same as the set's scope.
            bool is_irreg = false;
            
            // should not set_scope be the current scope? I am afraid that
            // when I do additional aggregation of sets in pmemmix this might 
            // not be true always. But I think I made this assumption with
            // respect to the other data anyway. I will place an assert and
            // we will see the result when we actually use the pmemmix.
            // If the next assert fails, you must check the correctness of
            // assuming that the set's scope is destId, not only for the
            // scopePairs, but also when computing carryied misses.
            // I am using realDestId now.
            
            // determine if the sets are in an ancestor relationship
            int scopeDist = destScope->getAncestorDistance (cscope);
            if (scopeDist>=0 && scopeDist<4)
            {
               // we should have a stride formula computed for this loop
               // However, I cannot assert using the stride formulas because
               // on one hand I do not have a reference address but a set 
               // index, and second, formulas are not kept in memory at this 
               // point. I have the irregular information in irregSetInfo, but
               // I do not know if there was a formula for this loop or it just
               // is not irregular. I could store zeros as well in that table
               // but right now I do not; again, to save memory and time.
               unsigned keyval = (scopeDist<<IRREG_LOOP_LEVEL_SHIFT) + setIdx;
               int idx_irreg = irregSetInfo.indexOf (keyval);
               if (idx_irreg>=0 && irregSetInfo.atIndex (idx_irreg)==1)
                  is_irreg = true;

               if (is_irreg)
               {
                  rit->second[numLevels] = 1.0;  // mark this set as irregular
                  for (j=0 ; j<numLevels ; ++j)
                     if (rit->second[j]>0.0)
                     {
                        totalIrregCarried[j] += rit->second[j];
                        exclStats.addIrregMissCountLevel (j, rit->second[j]);
                     }
               }
            }
            
            srd->addDataForCarrier (fullCarrierId, set_name, rit->second);
         }
         
         double **carriedM;
         // add the detailed info to the carriedMap of cscope
         MIAMIU::PairDoublePMap *carryMap;
         if (cscope->getScopeType()==LOOP_SCOPE && loopIndep)
         {
            carriedM = &(cscope->LICarriedMisses());
            carryMap = &(cscope->getLICarriedForScopeMap());
         } else
         {
            carriedM = &(cscope->CarriedMisses());
            carryMap = &(cscope->getCarriedForScopeMap());
         }
         
//         double* &carriedM = cscope->CarriedMisses ();
         if (! (*carriedM))
         {
            *carriedM = new double[numLevels];
            for (j=0 ; j<numLevels ; ++j)
            {
               (*carriedM)[j] = rit->second[j];
            }
         } else
            for (j=0 ; j<numLevels ; ++j)
            {
               (*carriedM)[j] += rit->second[j];
            }
         
         // add the detailed info to the carriedMap of cscope
//         PairDoublePMap& carryMap = cscope->getCarriedForScopeMap ();

         // create a pair (destination,source) of reuse
         MIAMIU::UIPair tempPair (realDestId, sourceId);
         // I should see all misses with this destination at once
         // not true anymore. I have a separate entry for each setIdx
         MIAMIU::PairDoublePMap::iterator pdit = carryMap->find (tempPair);
         if (pdit == carryMap->end ())
         {
            double *newVals = new double [numLevels];
            for (j=0 ; j<numLevels ; ++j)
            {
               newVals[j] = rit->second[j];
               totalCarried[j] += rit->second[j];
            }
            carryMap->insert (MIAMIU::PairDoublePMap::value_type (tempPair, newVals));
         } else
            for (j=0 ; j<numLevels ; ++j)
            {
               pdit->second[j] += rit->second[j];
               totalCarried[j] += rit->second[j];
            }
      }
      
      // add cache miss info to the stats by name
      cit = exclByName.find (set_name);
      double fragFactor = rit->second[numLevels+1];
      bool is_irreg = (rit->second[numLevels] > 0.0);
      bool is_frag = (fragFactor > 0.0);
      if (cit == exclByName.end())
      {
         double *vals = new double[numLevels3];
         exclByName.insert (CharDoublePMap::value_type (set_name, vals));
         for (j=0 ; j<numLevels ; ++j)
         {
            vals[j] = rit->second[j];
            if (is_irreg)
               vals[j+numLevels] = vals[j];
            else
               vals[j+numLevels] = 0.0;
            if (is_frag && vals[j]>0.0)
               vals[j+numLevels2] = vals[j]*fragFactor;
            else
               vals[j+numLevels2] = 0.0;
         }
      } else
         for (j=0 ; j<numLevels ; ++j)
         {
            cit->second[j] += rit->second[j];
            if (is_irreg)
               cit->second[j+numLevels] += rit->second[j];
            if (is_frag && rit->second[j]>0.0)
               cit->second[j+numLevels2] += rit->second[j]*fragFactor;
         }
   }
   
   // next process children
   ScopeImplementation::iterator it;
   for( it=pscope->begin() ; it!=pscope->end() ; ++it )
   {
      ScopeImplementation *sci = 
            dynamic_cast<ScopeImplementation*> (it->second);
      addMemoryDataForScope (sci);
   }
}

// this routine adds footprint information to all scopes
void
MIAMI_Driver::addFootPrintDataForScopes (ScopeImplementation *prog, 
          int no_fpga_acc)
{
   Machine *tmach = targets.front ();
   MemLevelAssocTable *memLevels = tmach->getMemoryLevels ();
   int numLevels = memLevels->NumberOfElements ();
   int i;
   bool first_level_with_info = true;
   
   for (i=0 ; i<numLevels ; ++i)
   {
      float max_fp_value = 0.0f;
      
      MemoryHierarchyLevel *mhl = memLevels->getElementAtIndex (i);
      // add footprint data
      float line_size = mhl->getBlockSize ();
      
      const ScopesFootPrint &scopesFp = mhl->getScopeFootPrintData ();
      ScopesFootPrint::const_iterator sfit = scopesFp.begin ();
      for ( ; sfit!=scopesFp.end() ; ++sfit)
      {
#if VERBOSE_DEBUG_MEMORY
         DEBUG_MEM (3,
            cerr << "Computing footprint for level " << i << " scopeId "
                 << sfit->first << endl;
         )
#endif
         if (scopesMap.is_member (sfit->first))
         {
            ScopeImplementation *pscope = scopesMap[sfit->first];
            TimeAccount& stats = pscope->getExclusiveTimeStats();
            stats.addMemFootprintLevel (i, sfit->second->avgFp);
            if (first_level_with_info && !no_fpga_acc)
            {
               double fpval = sfit->second->avgFp;
               fpval *= line_size;
#if NOT_NOW
               fpval = computeFPGATransferCost (fpval);
#endif
               fpval *= pscope->getExitCount ();
               stats.addFpgaTransferTime (fpval);
            }
            if (max_fp_value < sfit->second->maxFp)
               max_fp_value = sfit->second->maxFp;
         }
      }
      if (max_fp_value > 0.01)
      {
         prog->getExclusiveTimeStats().addMemFootprintLevel (i, max_fp_value);
         first_level_with_info = false;
      }
   }
}
#endif

unsigned int 
MIAMI_Driver::GetFileIndex(std::string &file)
{
   if (file.length()==0)
      return (0);
   else
   {
      StringIndexMap::iterator sit = fileIndices.find(file);
      if (sit == fileIndices.end())  // I must insert it
      {
         sit = fileIndices.insert(fileIndices.begin(), 
                       StringIndexMap::value_type(file, highFileIndex++));
         fileNames.push_back(&(sit->first));
         assert(fileNames.size() == highFileIndex);
      }
      return (sit->second);
   }
}

void 
MIAMI_Driver::AddSourceFileInfo(unsigned int findex, unsigned int l1, unsigned int l2)
{
   if (l1==NO_LINE && l2==NO_LINE)
      return;
   
   // add these numbers to the global line mappings
   MIAMIU::Ui2SetMap::iterator uit = allFileLines.find(findex);
   if (uit == allFileLines.end())
      uit = allFileLines.insert(allFileLines.begin(), 
             MIAMIU::Ui2SetMap::value_type(findex, MIAMIU::UISet()));
   if (l1 != NO_LINE)
      uit->second.insert(l1);
   if (l2!=NO_LINE && l2!=l1)
      uit->second.insert(l2);
}


}  /* namespace MIAMI */
