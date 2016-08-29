/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: memory_reuse_histograms.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the logic for parsing memory reuse distance files
 * (.mrdt) and the analysis of the memory reuse information.
 */

#include "miami_types.h"
#include "MiamiDriver.h"
#include "debug_scheduler.h"
#include "slice_references.h"
#include "miami_data_sections.h"
#include "memory_reuse_histograms.h"
#include "miami_utils.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <math.h>

using std::cerr;
using std::endl;

namespace MIAMI {
  extern MIAMI_Driver mdriver;
}
  
namespace MIAMI_MEM_REUSE
{
count_t defCountType = 0;

int 
BlockMRDData::ParseMrdFile()
{
   int res = 0;
   if (!fileParsed)
   {
      if (fileInfo.IsTextFile())
         res = ParseTextMrdFile();
      else
         res = ParseBinMrdFile();
      if (!res)
         fileParsed = true;
   }
   return (res);
}

static int
ReadDistCountHistogramFromFile(FILE *fd, HashMapCT *hist)
{
   count_t  totCount, count;
   int32_t  numBins;
   dist_t   dist;
   int res;
   
   res = fscanf(fd, "%" PRIcount " %" PRId32, &totCount, &numBins);
   if (res!=2) return (-1);
   for (int i=0 ; i<numBins ; ++i)
   {
      res = fscanf(fd, "%" PRIdist " %" PRIcount, &dist, &count);
      if (res!=2) return (-1);
      (*hist)[dist] += count;
   }
   return (0);
}

#define BUFFER_SIZE 256
int 
BlockMRDData::ParseTextMrdFile()
{
#undef CHECK_COND
#undef CHECK_COND0

#define CHECK_COND(cond, err, ...) if (cond) \
   {fprintf(stderr, "ERROR: "err"\n", __VA_ARGS__); goto load_error; }
#define CHECK_COND0(cond, err) if (cond) \
   {fprintf(stderr, "ERROR: "err"\n"); goto load_error; }
      
      
   addrtype tmask, tval;
   uint32_t lshift;
   int res, nimages;
   FILE *fd = 0;
   char buf[BUFFER_SIZE], *crez;
   
   // First open the file
   fd = fopen(fileInfo.name.c_str(), "rt");
   CHECK_COND(!fd, "Cannot open MRD file %s for reading.\n", fileInfo.name.c_str());
   
   res = fscanf(fd, "%" PRIu32 " 0x%" PRIxaddr " 0x%" PRIxaddr "\n",
              &lshift, &tmask, &tval);
   CHECK_COND(res!=3, "Reading first line in MRD file %s", fileInfo.name.c_str());
   CHECK_COND((1u<<lshift)!=fileInfo.bsize, "MRD file %s, block size does not match recorded value %d", 
           fileInfo.name.c_str(), fileInfo.bsize);
   
   // Next, read the number of images for which we have data
   res = fscanf(fd, "%d\n", &nimages);
   CHECK_COND(res!=1, "Reading number of images in MRD file %s", fileInfo.name.c_str());
   
   // Read data for each image in turn
   for (int i=0 ; i<nimages ; ++i)
   {
      int img_id, k;
      addrtype base_addr, low_offset;
      string name;
      
      res = fscanf(fd, "%d 0x%" PRIxaddr " 0x%" PRIxaddr, &img_id, &base_addr, &low_offset);
      CHECK_COND(res!=3, "Reading ID, base and low offset addresses for image %d", i);
      assert(img_id>=0 && img_id < 1024);   // assume we cannot have more than 1024 images
      
      // Now read the image name using a loop
      k = 0;
      while ((crez = fgets(buf, BUFFER_SIZE, fd)))
      {
         while (!k && crez && *crez==' ')
            ++ crez;
         name = name + crez;
         k += 1;
         if (strlen(crez) < BUFFER_SIZE-1)  // buffer not full, stop
            break;
      }
      if (name[name.length()-1] == '\n')
         name[name.length()-1] = 0;
//      fprintf(stderr, "Img %d(%d), read name >>%s<<\n", i, img_id, name.c_str());
      
      // create an image object
      ImageInfo &iinfo = imgData[img_id];
      iinfo.imgName = name;
      iinfo.base_addr = base_addr;
      iinfo.low_offset = low_offset;
      iinfo.id = img_id;
      
      MIAMI::LoadModule *myImg = MIAMI::mdriver.GetModuleWithIndex(img_id);
      assert(myImg || !"We do not have a LoadModule with the requested index???");
      
      // Next we have section type and number of entries in section;
      // Can I have a variable number of sections?
      // I need to have a line with section 0, #entries 0 to detect end of image data
      uint32_t fsection, nentries, j;
      res = fscanf(fd, "%" PRIu32 " %" PRIu32, &fsection, &nentries);
      CHECK_COND(res!=2, "Reading section / # entries for image %d, ID %d, name %s", 
                 i, img_id, name.c_str());
//      fprintf(stderr, "Read section %d with %d entries\n", fsection, nentries);
      while (fsection || nentries)
      {
         addrtype saddr;
         int32_t level;
         // we read different things, depending on the section type
         switch (fsection)
         {
            case MIAMI::MIAMI_DATA_SECTION_SCOPE_FOOTPRINT:
            {
               for (j=0 ; j<nentries ; ++j)
               {
                  res = fscanf(fd, "%" PRIxaddr " %" PRId32 ":", &saddr, &level);
                  CHECK_COND(res!=2, "Reading scope address / level for image %d, section %d, entry %d of %d", 
                        i, fsection, j, nentries);
//                  fprintf(stderr, "Read header of entry %d of %d\n", j, nentries);
                  
                  // Get unique scope index; If we parse the file at the end, after we
                  // processed the entire CFG file, we should have seen this scope before.
                  // But the CFG and the MRD come from different runs, so there may be cases
                  // when we do not have a complet overlap. Moreover, the MRD uses only a
                  // static understanding of the CFG, while the CFG run can resolve the 
                  // targets of indirect jumps as well
                  int32_t sidx = myImg->GetIndexForScopeOffset(saddr, level);
//                  fprintf(stderr, "Scope at offset 0x%" PRIxaddr ", level %d has index %d\n", 
//                          saddr, level, sidx);
                  if (sidx < 0)  // no record of this scope??
                  {
#if VERBOSE_DEBUG_MEMORY
                     DEBUG_MEM (1,
                        fprintf(stderr, "ERROR: We have no record of scope at offset 0x%" PRIxaddr
                             ", level %d, in image %d (%s) with MRD base_addr=0x%" PRIxaddr
                             " and MRD low_offset=0x%" PRIxaddr ", new base_addr=0x%" PRIxaddr
                             " and new low_offset=0x%" PRIxaddr "\n", saddr, level,
                             img_id, name.c_str(), base_addr, low_offset,
                             myImg->BaseAddr(), myImg->LowOffsetAddr());
                     )
#endif
                     sidx = UNKNOWN_SCOPE_ID;
                  }
                  ScopeInfo &sinfo = iinfo.scopeData[sidx];
                  // a ScopeInfo object is in fact an HashMapCT object
                  // I need to parse a histogram and store it in sinfo.
                  res = ReadDistCountHistogramFromFile(fd, &sinfo);
                  CHECK_COND(res!=0, "Reading footprint histogram for scope at offset 0x%" PRIxaddr
                          ", level %d, scopeID=%d, in image %d, entry %d of %d", 
                          saddr, level, sidx, i, j, nentries);
               }
               break;
            }
            
            case MIAMI::MIAMI_DATA_SECTION_MEM_REUSE:
            {
               for (j=0 ; j<nentries ; ++j)
               {
                  res = fscanf(fd, "%" PRIxaddr " %" PRId32, &saddr, &level);
                  CHECK_COND(res!=2, "Reading inst address / memop for image %d, section %d, entry %d of %d", 
                        i, fsection, j, nentries);
//                  fprintf(stderr, "Read header of entry %d of %d\n", j, nentries);
                  
                  // Get unique inst index; If we parse the file at the end, after we
                  // processed the entire CFG file, we should have seen this memory instr before.
                  // But the CFG and the MRD come from different runs, so there may be cases
                  // when we do not have a complet overlap. Moreover, the MRD uses only a
                  // static understanding of the CFG, while the CFG run can resolve the 
                  // targets of indirect jumps as well
                  int32_t iidx = myImg->GetIndexForInstOffset(saddr, level);
                  if (iidx < 0)  // no record of this scope??
                  {
#if VERBOSE_DEBUG_MEMORY
                     DEBUG_MEM (1,
                        fprintf(stderr, "ERROR: We have no record of instruction at offset 0x%" PRIxaddr
                             ", memop %d, in image %d (%s) with MRD base_addr=0x%" PRIxaddr
                             " and MRD low_offset=0x%" PRIxaddr ", new base_addr=0x%" PRIxaddr
                             " and new low_offset=0x%" PRIxaddr "\n", saddr, level,
                             img_id, name.c_str(), base_addr, low_offset,
                             myImg->BaseAddr(), myImg->LowOffsetAddr());
                     )
#endif
                     iidx = UNKNOWN_INSTRUCTION_ID;
                  }
                  InstInfo &inst = iinfo.instData[iidx];
                  // For each instruction I have a count of cold misses (read last),
                  // and a list of reuse patterns terminating at this instruction
                  // Read each pattern on turn. A pattern is characterized by a 
                  // source instruction and a carrier scope.
                  // I need to converst the carrier scope address/level to a 
                  // scope index, and the source instruction adress to its enclosing 
                  // scope index. A reuse pattern will be characterized by destination 
                  // instruction, carrier scope, and source scope.
                  int32_t srcImg, isInst, srcOp, carryImg, carryLevel, iter;
                  addrtype srcAddr, carryAddr;
                  do
                  {
                     res = fscanf(fd, "%" PRId32 "/%" PRId32 "/%" PRIxaddr "/%" PRId32, 
                                 &srcImg, &isInst, &srcAddr, &srcOp);
                     CHECK_COND(res!=4, "Reading source image / isInst / address / memop for dest instruction at 0x%" 
                           PRIxaddr ", memop %d, in image %d, entry %d of %d", 
                           saddr, level, i, j, nentries);
                     CHECK_COND(isInst!=0 && isInst!=1, "Is-Instruction flag has invalid value %d, expecting 0 or 1, for dest instruction at 0x%" 
                           PRIxaddr ", memop %d, in image %d, entry %d of %d", 
                           isInst, saddr, level, i, j, nentries);
                     
                     if (!srcImg && !isInst && !srcAddr && !srcOp) break;
                     
                     // If we are still here, it means this is a reuse pattern
                     // Read carrier information
                     res = fscanf(fd, "%" PRId32 "/%" PRIxaddr "/%" PRId32 "/%" PRId32 ":", 
                                 &carryImg, &carryAddr, &carryLevel, &iter);
                     CHECK_COND(res!=4, "Reading carrier scope image / address / level / iterCrossed for dest instruction at 0x%" 
                           PRIxaddr ", memop %d, in image %d, entry %d of %d, source img/address/op=%" PRId32
                           "/0x%" PRIxaddr "/%" PRId32, 
                           saddr, level, i, j, nentries, srcImg, srcAddr, srcOp);
                     CHECK_COND(iter!=0 && iter!=1, "Loop carried flag iter has invalid value %d, expecting 0 or 1, for dest instruction at 0x%" 
                           PRIxaddr ", memop %d, in image %d, entry %d of %d, source img/address/op=%" PRId32
                           "/0x%" PRIxaddr "/%" PRId32, 
                           iter, saddr, level, i, j, nentries, srcImg, srcAddr, srcOp);
                     
                     MIAMI::LoadModule *srcLM = (srcImg==img_id ? myImg : 
                                    MIAMI::mdriver.GetModuleWithIndex(srcImg));
                     if (!srcLM)
                        continue;
                     assert(srcLM || !"We do not have a source LoadModule with the requested index???");
                     int32_t srcidx = 0;
                     if (isInst)
                     {
                        int32_t iidx = srcLM->GetIndexForInstOffset(srcAddr, srcOp);
                        if (iidx < 0)  // no record of this scope??
                        {
#if VERBOSE_DEBUG_MEMORY
                           DEBUG_MEM (1,
                              fprintf(stderr, "ERROR: We have no record of instruction at offset 0x%" PRIxaddr
                                   ", memop %d, in image %d (%s) with new base_addr=0x%" 
                                   PRIxaddr " and new low_offset=0x%" PRIxaddr "\n", 
                                   srcAddr, srcOp, srcImg, srcLM->Name().c_str(),
                                   srcLM->BaseAddr(), srcLM->LowOffsetAddr());
                           )
#endif
                           iidx = UNKNOWN_INSTRUCTION_ID;
                        }
                        
                        // We have a source instruction index and a carry scope index;
                        // It does not really make sense to have separate reuse patterns
                        // depending on the source instruction. It makes more sense to
                        // aggregate reuse patterns based on the source scope.
                        // I need to translate from a source instruction index to a 
                        // source scope index, brrrr. This means, more conversion tables
                        // stored into the load module object.
                        srcidx = srcLM->GetScopeIndexForReference(iidx);
                     } else  // we have scope information directly
                        srcidx = srcLM->GetIndexForScopeOffset(srcAddr, srcOp);
                     
                     if (srcidx < 0)  // no record of this reference??
                     {
#if VERBOSE_DEBUG_MEMORY
                        DEBUG_MEM (1,
                           fprintf(stderr, "ERROR: We have no scope info for reference with index %d "
                                "at offset 0x%" PRIxaddr ", memop %d, in image %d (%s) with new "
                                "base_addr=0x%" PRIxaddr " and new low_offset=0x%" PRIxaddr "\n", 
                                iidx, srcAddr, srcOp, srcImg, srcLM->Name().c_str(), 
                                srcLM->BaseAddr(), srcLM->LowOffsetAddr());
                        )
#endif
                        srcidx = UNKNOWN_SCOPE_ID;
                     }

                     MIAMI::LoadModule *carryLM = (carryImg==img_id ? myImg :
                                    MIAMI::mdriver.GetModuleWithIndex(carryImg));
                     if (!carryLM)
                        continue;
                     assert(carryLM || !"We do not have a carry LoadModule with the requested index???");
                     int32_t sidx = carryLM->GetIndexForScopeOffset(carryAddr, carryLevel);
                     if (sidx < 0)  // no record of this scope??
                     {
#if VERBOSE_DEBUG_MEMORY
                        DEBUG_MEM (1,
                           fprintf(stderr, "ERROR: We have no record of scope at offset 0x%" PRIxaddr
                                ", level %d, in image %d (%s) with new base_addr=0x%" PRIxaddr
                                " and new low_offset=0x%" PRIxaddr "\n", carryAddr, carryLevel,
                                carryImg, carryLM->Name().c_str(), carryLM->BaseAddr(), 
                                carryLM->LowOffsetAddr());
                        )
#endif
                        sidx = UNKNOWN_SCOPE_ID;
                     }
                     
                     // Find the reuse pattern for this source / carry combination;
                     // While the reuse patterns read from the file are unique, they 
                     // are characterized by a source instruction; we'll aggregate them 
                     // by unique source scope. so we may need to combine some of the
                     // histograms.
                     if (!inst.reuseVec)  // first time we see reuse with this instruction
                        inst.reuseVec = new RI_List();
                        
                     // encode the iter carried flag in the carrier index
                     // iter can be only 0 or 1, I check its value after parsing
                     reuse_info_t ritmp(srcImg, srcidx, carryImg, (sidx << 1) + iter);
                     reuse_info_t &reuseElem = inst.reuseVec->insert(ritmp);
                     if (! reuseElem.reuseHist)
                        reuseElem.reuseHist = new HashMapCT();
                     res = ReadDistCountHistogramFromFile(fd, reuseElem.reuseHist);
                     CHECK_COND(res!=0, "Reading reuse histogram for instruction at offset 0x%" PRIxaddr
                             ", memop %d, in image %d, entry %d of %d"
                             ", with source img/idx=%" PRId32 "/% "PRId32 
                             " and carry img/idx=%" PRId32 "/% "PRId32, 
                             saddr, level, i, j, nentries, srcImg, srcidx, 
                             carryImg, sidx);
                  } while (1);  // we use a break to exit the loop
                  
                  // Once we exit the loop, there is one final line for an instruction
                  // with the total reuse frequency, and the number of cold misses.
                  // Cold misses are not part of any reuse pattern, so that count is
                  // very important;
                  res = fscanf(fd, ": %" PRIcount " %" PRIcount, &
                                 inst.totalReuses, &inst.coldMisses);
                  CHECK_COND(res!=2, "Reading total reuses / cold misses for instruction at offset 0x%"
                        PRIxaddr ", memop %" PRId32 " in image %d, entry %d of %d", 
                        saddr, level, i, j, nentries);
               }
               break;
            }
            
            default:
               assert(!"Bad section type!");
         
         }
      
         res = fscanf(fd, "%" PRIu32 " %" PRIu32, &fsection, &nentries);
         CHECK_COND(res!=2, "Reading section / # entries for image %d, ID %d, name %s", 
                    i, img_id, name.c_str());
//         fprintf(stderr, "Read section %d with %d entries\n", fsection, nentries);
      }
   }
   
   // We should be at the end of the file now. How do I check??
   // Do I need to make another read request?
   while (!feof(fd))
   {
      if ((crez = fgets(buf, BUFFER_SIZE, fd)) && !MIAMIU::is_empty_string(crez))
      {
         fprintf(stderr, "Found residual text >>%s<< at end of MRD file %s\n", 
               crez, fileInfo.name.c_str());
      }
   }
   
   fclose(fd);
   return (0);

load_error:
   if (fd) fclose(fd);
   return (-1);
}

int 
BlockMRDData::ParseBinMrdFile()
{
   assert(!"Parsing of binary MRD files is not implemented yet!");
   return (0);
}

struct statistics_t
{
   count_t count;
   float min, max;
   float mean, stdev;
   float median;
};

struct cache_stats_t
{
   count_t count;   // num accesses
   double fa_misses; // full associative misses
   double sa_misses; // set-associative misses
};


static int 
processBinToVector(void* arg0, dist_t key, void* value)
{
   DCVector* histogram = static_cast<DCVector*>(arg0);
   count_t* pValue = (count_t*)value;
   histogram->push_back(DistCountPair(key, *pValue) );
   return 0;
}

static void
ComputeMissCountsForHistogram(const HashMapCT *hist, MIAMI::MemoryHierarchyLevel *mhl,
          cache_stats_t *stats)
{
   stats->count = 0;
   stats->fa_misses = stats->sa_misses = 0.0;

   DCVector dcArray;
   hist->map(processBinToVector, &dcArray);
   // The complete histogram of the instruction must be in the dcArray array
   // I do not need the histogram in sorted order in this case, I think ...
//   std::sort(dcArray.begin(), dcArray.end(), DistCountPair::OrderAscByKey1());
   
   // traverse the array and compute all the statistics
   assert(! dcArray.empty());
   DCVector::iterator dit = dcArray.begin();
   for ( ; dit!=dcArray.end() ; ++dit)
   {
      double fullMiss, setMiss;
      stats->count += dit->second;
      mhl->ComputeMissesForReuseDistance(dit->first, fullMiss, setMiss);
      stats->fa_misses += (fullMiss * dit->second);
      stats->sa_misses += (setMiss * dit->second);
#if VERBOSE_DEBUG_MEMORY
      DEBUG_MEM (6,
         fprintf(stderr, "MRD_eval: distance=%" PRIdist ", fullMIss=%lg, setMiss=%lg, pattern count=%" PRIcount "\n",
               dit->first, fullMiss, setMiss, dit->second);
      )
#endif
   }
}

static void
ComputeStatisticsForHistogram(const HashMapCT *hist, statistics_t *stats)
{
   DCVector dcArray;
   hist->map(processBinToVector, &dcArray);
   // The complete histogram of the instruction must be in the dcArray array
   // Sort it and process it
   std::sort(dcArray.begin(), dcArray.end(), DistCountPair::OrderAscByKey1());
   
   // traverse the array and compute all the statistics
   assert(! dcArray.empty());
   DCVector::iterator dit = dcArray.begin();
   stats->min = dit->first;
   stats->median = 0.f;
   double sum = 0.0;
   count_t totCount = 0;
   // I will need to do a second pass to computed the median, because 
   // I do not know the total number of elements in the first pass
   for ( ; dit!=dcArray.end() ; ++dit)
   {
      sum += (dit->first * dit->second);
      totCount += dit->second;
      stats->max = dit->first;  // the last assignment will be the maximum
   }
   stats->count = totCount;
   stats->mean = sum / totCount;

   double stdev = 0.0;
   bool is_odd = totCount & 1;
   count_t halfCount = (totCount + 1) >> 1;
   count_t partCount = 0;
   // for odd counts, the median is the middle point
   // for even counts, the median the mean of the two middle values
   for (dit=dcArray.begin() ; dit!=dcArray.end() ; ++dit)
   {
      double rez = dit->first - stats->mean;
      stdev += (rez*rez*dit->second);
      
      count_t oldCount = partCount;
      partCount += dit->second;
      if (oldCount<halfCount && partCount>=halfCount)
         stats->median = dit->first;  // for odd counts this is the median
      if (!is_odd && oldCount<=halfCount && partCount>halfCount)
         stats->median = (stats->median+dit->first) / 2;
   }
   stats->stdev = sqrt(stdev / totCount);
}

int 
BlockMRDData::ComputeMemoryEventsForLevel(int level, int numLevels, 
               MIAMI::MemoryHierarchyLevel *mhl, double& totalCarried,
               double& totalIrregCarried, double& totalFragMisses)
{
   // iterate over all images, all scopes, and all instructions, 
   // and add data to the correct pscopes.
   int numLevels2 = numLevels + numLevels;
   int numLevels3 = numLevels2 + numLevels;
   MIAMI::PairSRDMap& reusePairs = MIAMI::mdriver.GetScopePairsReuse();
   
   ImgContainer::const_iterator cit = imgData.first();
   float max_fp_value = 0.0f;   // determine the maximum program footprint
   mhl->SetSampleMask(fileInfo.bmask);
#if VERBOSE_DEBUG_MEMORY
   DEBUG_MEM (2,
      fprintf(stderr, "Computing memory footprint and miss counts for level %d (%s)\n",
                level, mhl->getLongName());
   )
#endif
   while ((bool)cit)
   {
      int img_id = cit.Index();
      MIAMI::LoadModule *myImg = MIAMI::mdriver.GetModuleWithIndex(img_id);
      assert(myImg || !"BlockMRDData::ComputeMemoryEventsForLevel: We do not have a LoadModule with the requested index???");

      // iterate over the scopes first, and compute the footprint mean or median
      SIContainer::const_iterator sit = cit->scopeData.first();
      while ((bool)sit)
      {
         int scope_id = sit.Index();
         
#if VERBOSE_DEBUG_MEMORY
         DEBUG_MEM (4,
            cerr << "Computing footprint for level " << level << " scopeId "
                 << scope_id << " in image " << img_id << endl;
         )
#endif
         
         statistics_t fpStats;
         ComputeStatisticsForHistogram((const HashMapCT*)sit, &fpStats);
         if (max_fp_value < fpStats.max)
            max_fp_value = fpStats.max;
         
         MIAMI::ScopeImplementation *pscope = myImg->GetSIForScope(scope_id);
         if (pscope)
         {
            MIAMI::TimeAccount& stats = pscope->getExclusiveTimeStats();
            stats.addMemFootprintLevel (level, fpStats.median);
#if COMPUTE_FPGA  // not doing this yet
            if (first_level_with_info && !no_fpga_acc)
            {
               double fpval = sfit->second->avgFp;
               fpval *= mhl->getBlockSize();
               fpval = computeFPGATransferCost (fpval);
               fpval *= pscope->getExitCount ();
               stats.addFpgaTransferTime (fpval);
            }
#endif
         } else  // I could not find a ScopeImplementation associated with this ID
         {
            // print the error message only for valid Scope IDs. ID 0 is special.
            if (scope_id)
               fprintf(stderr, "ERROR: BlockMRDData::ComputeMemoryEventsForLevel: Couldn't find a ScopeImplementation for scopeId %d, in image %d (%s)\n",
                    scope_id, img_id, myImg->Name().c_str());
         }
         
         ++ sit;
      }

      // iterate also over the instructions, and compute the number of cache misses
      IIContainer::const_iterator iit = cit->instData.first();
      while ((bool)iit)
      {
         int inst_id = iit.Index();
         
#if VERBOSE_DEBUG_MEMORY
         DEBUG_MEM (5,
            cerr << "Computing misses for level " << level << " instId "
                 << inst_id << " in image " << img_id << endl;
         )
#endif
         // We can have multiple reuse patterns for each instruction;
         // Right now, we aggregate miss counts at scope level.
         // Therefore, I need to find the scope of this instruction first
         int dest_id = myImg->GetScopeIndexForReference(inst_id);
         MIAMI::ScopeImplementation *pscope = myImg->GetSIForScope(dest_id);
         if (! pscope)
         {
            // print the error message only for valid Scope IDs. ID 0 is special.
            if (dest_id)
               fprintf(stderr, "ERROR: BlockMRDData::ComputeMemoryEventsForLevel 2: Couldn't find a ScopeImplementation for scopeId %d, in image %d (%s)\n",
                    dest_id, img_id, myImg->Name().c_str());
            // create a ScopeNotFound scope to use when we cannot find one
            pscope = myImg->GetDefaultScope();
         }
         assert (pscope || !"I still do not have a valid pscope object??");
         MIAMI::Trio64DoublePMap &reuseMap = pscope->getReuseFromScopeMap();
         MIAMI::TimeAccount& exclStats = pscope->getExclusiveTimeStats();

         MIAMI::CharDoublePMap &exclByName = pscope->getExclusiveStatsPerName ();
         MIAMI::CharDoublePMap::iterator nait; 
         
         // Find also the set of a reference. Var names, irreg info, etc.
         // are stored per set.
         int32_t set_id = 0;
         if (inst_id>0)
         {
            set_id = myImg->GetSetIndexForReference(inst_id);
            assert (set_id > 0);  // we should find a valid set for all references
         }
         
         // Check first the fragmentation factor for this instruction / set
         // check if we have fragmentation factor computed for this set
         // I need to check the map from pcost
         // I will need to form reuse groups and search by group ID;
         // As a placeholder, use the instruction ID of the pattern's sink
         MIAMIU::UiDoubleMap &fragFactors = pscope->getFragFactorsForScope();
         double fragF = 0.0;
         // The frag factor is computed statically, but I am going to do this 
         // search for each memory level. Sadly, I do not see a better 
         // alternative at this point.
         // Do I compute fragmentation factor at instruction or at set level?
         // I think the right answer is reuse group level. Each set may have
         // several reuse groups, and reuse groups should be computed at path
         // level because the reuse group depends on which refs are actually
         // executed in the same iteration. Once I have reuse groups, I may 
         // need to compute an average fragmentation factor at set level.
         // We do not have any fragmentation factors right now, so just use
         // the set ID for now.
         MIAMIU::UiDoubleMap::iterator dit = fragFactors.find(set_id); 
         if (dit!=fragFactors.end() && dit->second>0.0)
            fragF = dit->second;

         const char* set_name = myImg->GetNameForSetIndex(set_id);
         bool is_frag = (fragF > 0.0);
         
         // fill in the reuseMap data-structure and add miss events at scope level
         // We are not adding any memory misses at path level right now
         // I need to compute the total counts (insert into the exclusive TimeAccount)
         // as well as the detailed reuseMap.
         // I also have to compute carried misses for a scope. Compute the totals
         // in the arrays, and add the reuse patterns to the carriedScopes
         
         // I must add cold misses to the reuseMap as well using a sourceId
         // and carrierId equal to 0 (zero)  **** FIXME ****
         double *namevals = 0;
         count_t refMissCount = iit->coldMisses;
         double refFragMisses = 0.0, refIrregMisses = 0.0;

         nait = exclByName.find (set_name);
         if (nait != exclByName.end())
            namevals = nait->second;
         
         if (iit->coldMisses > 0.001)
         {
            // add current number of misses to the reuseMap
            MIAMI::Trio64 tempTrio (0, 0, set_id);
            MIAMI::Trio64DoublePMap::iterator dip = reuseMap.find (tempTrio);
            if (dip == reuseMap.end())
            {
               dip = reuseMap.insert (reuseMap.begin(), 
                    MIAMI::Trio64DoublePMap::value_type (tempTrio,
                                  new double[numLevels+2]));
               for (int i=0 ; i<numLevels ; ++i)
                  dip->second[i] = 0.0;
               dip->second[numLevels] = -1.0;  // marker for irreg pattern
               dip->second[numLevels+1] = fragF;// stores fragmentation factor
            }
            dip->second[level] += iit->coldMisses;
            // I also need to add frag / irreg misses, brr. I must factor this code out.
            // I am not computing irreg misses for cold misses, only for data reuses
            if (is_frag)
               // totalFragMisses is in MiamiDriver; I need to accumulate it locally
               // for each level and then add it to the global thing
               refFragMisses += iit->coldMisses * fragF;

            // add cache miss info to the stats by name
            if (! namevals)
            {
               namevals = new double[numLevels3];
               exclByName.insert (MIAMI::CharDoublePMap::value_type (set_name, namevals));
               for (int j=0 ; j<numLevels3 ; ++j)
                  namevals[j] = 0.0;
            } 
            namevals[level] += iit->coldMisses;
            if (is_frag)
               namevals[level+numLevels2] += iit->coldMisses * fragF;
         }

         if (iit->reuseVec)
         {
            RI_List::iterator lit = iit->reuseVec->first();
            while ((bool)lit)  // iterate over all reuse patterns
            {
               // For each pattern, I must compute the number of misses from
               // the reuse information
               cache_stats_t missStats;
               assert(lit->reuseHist);
               ComputeMissCountsForHistogram(lit->reuseHist, mhl, &missStats);
               // missStats.sa_misses has the desired value
               // Add the miss values to the reuseMap
               if (missStats.sa_misses > 0.001)
               {
                  // add current number of misses to the reuseMap
                  MIAMI::Trio64 tempTrio (lit->sourceId, lit->carryId, set_id);
                  MIAMI::Trio64DoublePMap::iterator dip = reuseMap.find (tempTrio);
                  if (dip == reuseMap.end())
                  {
                     dip = reuseMap.insert (reuseMap.begin(), 
                          MIAMI::Trio64DoublePMap::value_type (tempTrio,
                                        new double[numLevels+2]));
                     for (int i=0 ; i<numLevels ; ++i)
                        dip->second[i] = 0.0;
                     dip->second[numLevels] = -1.0;  // marker for irreg pattern
                     dip->second[numLevels+1] = fragF;// stores fragmentation factor
                  }
                  dip->second[level] += missStats.sa_misses;
                  
                  refMissCount += missStats.sa_misses;

                  if (fragF > 0.0)
                     refFragMisses += missStats.sa_misses * fragF;
               
                  // I should also add the carried misses to the carrier scope
                  int32_t imgCarry = GET_PATTERN_IMG(lit->carryId);
                  int32_t fullIndexCarry = GET_PATTERN_INDEX(lit->carryId);
                  
                  MIAMI::LoadModule *cImg = MIAMI::mdriver.GetModuleWithIndex(imgCarry);
                  assert(cImg || !"BlockMRDData::ComputeMemoryEventsForLevel: We do not have a LoadModule with the requested carry index???");
                  
                  int32_t iter = fullIndexCarry & 1;
                  int32_t indexCarry = fullIndexCarry >> 1;
                  
                  // Now get the actual scope object for the carrier
                  MIAMI::ScopeImplementation *cscope = cImg->GetSIForScope(indexCarry);
                  if (! cscope)
                  {
                     // print the error message only for valid Scope IDs. ID 0 is special.
                     if (indexCarry)
                        fprintf(stderr, "ERROR: BlockMRDData::ComputeMemoryEventsForLevel 3: Couldn't find a ScopeImplementation for carry Id %d, in image %d (%s)\n",
                             indexCarry, imgCarry, cImg->Name().c_str());
                     // create a ScopeNotFound scope to use when we cannot find one
                     cscope = cImg->GetDefaultScope();
                  }
                  assert (cscope || !"I still do not have a valid cscope object??");
                  
                  // Add the count to the PairSRDMap from MiamiDriver.
                  // These global objects are very ugly.
                  uint64_t fullDestId = MAKE_PATTERN_KEY(img_id, dest_id);
                  MIAMI::Pair64 tempP64 (fullDestId, lit->sourceId);
                  MIAMI::PairSRDMap::iterator srit = reusePairs.find (tempP64);
                  MIAMI::ScopeReuseData *srd = 0;
                  if (srit == reusePairs.end())
                  {
                     srd = new MIAMI::ScopeReuseData (numLevels);
                     reusePairs.insert (MIAMI::PairSRDMap::value_type (tempP64, srd));
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
                  if (dip->second[numLevels] < 0.0)
                  {  // we do not know if it is irregular yet
                     // this might be the first level we are analyzing
                     
                     // determine if the sets are in an ancestor relationship
                     int scopeDist = pscope->getAncestorDistance (cscope);
                     // I am not limited to 4 levels in the x86 version of MIAMI
                     if (scopeDist>=0) //  && scopeDist<4)
                     {
                        // we should have a stride formula computed for this loop
                        // However, I cannot assert using the stride formulas because
                        // on one hand I do not have a reference address but a set 
                        // index, and second, formulas are not kept in memory at this 
                        // point. I have the irregular information in irregSetInfo, but
                        // I do not know if there was a formula for this loop or it just
                        // is not irregular. I could store zeros as well in that table
                        // but right now I do not; again, to save memory and time.
                        is_irreg = myImg->SetHasIrregularAccessAtDistance(set_id, scopeDist);
                     }
                     if (is_irreg)
                        dip->second[numLevels] = 1.0;
                     else
                        dip->second[numLevels] = 0.0;
                  } else
                     is_irreg = (dip->second[numLevels] > 0.0);

                  if (is_irreg)
                     refIrregMisses += missStats.sa_misses;
                  
//                  srd->addDataForCarrier (fullCarrierId, set_name, dip->second);
                  srd->addDataForCarrier (lit->carryId, set_name, level, 
                        missStats.sa_misses, fragF, is_irreg);
                  
                  // Check if we have to insert the count into the loop independent
                  // or in the loop carried map
                  // It is easier to just call a method of cscope to add all the counts
                  cscope->AddCarriedMisses(iter, tempP64, level, numLevels,
                        missStats.sa_misses); //, fragF, is_irreg);
                  totalCarried += missStats.sa_misses;

                  // add cache miss info to the stats by name
                  if (! namevals)
                  {
                     namevals = new double[numLevels3];
                     exclByName.insert (MIAMI::CharDoublePMap::value_type (set_name, namevals));
                     for (int j=0 ; j<numLevels3 ; ++j)
                        namevals[j] = 0.0;
                  } 
                  namevals[level] += missStats.sa_misses;
                  if (is_irreg)
                     namevals[level+numLevels] += missStats.sa_misses;
                  if (is_frag)
                     namevals[level+numLevels2] += missStats.sa_misses * fragF;
               }

               ++ lit;
            }  // iterate over reuse patterns
         }  // If any reuse patterns

         // Add the misses to the pscope's account
         exclStats.addMissCountLevel (level, refMissCount, mhl);
         exclStats.addFragMissCountLevel (level, refFragMisses);
         exclStats.addIrregMissCountLevel (level, refIrregMisses);
         totalIrregCarried += refIrregMisses;
         totalFragMisses += refFragMisses;
         
         ++ iit;
      }
      
      ++ cit;
   }
   
   if (max_fp_value > 0.01f)
   {
      MIAMI::mdriver.GetProgramScope()->getExclusiveTimeStats().
                    addMemFootprintLevel (level, max_fp_value);
//      first_level_with_info = false;
   }
   return (0);
}


}  /* namespace MIAMI_MEM_REUSE */

