/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: memory_reuse_histograms.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the logic for parsing memory reuse distance files
 * (.mrdt) and the analysis of the memory reuse information.
 */

#ifndef _MEMORY_REUSE_HISTOGRAMS_H
#define _MEMORY_REUSE_HISTOGRAMS_H

#include <vector>
#include <list>
#include <set>
#include <string>
#include "miami_types.h"
#include "fast_hashmap.h"
#include "self_sorted_list.h"
#include "dense_container.h"
#include "generic_pair.h"
#include "mrd_file_info.h"
#include "MemoryHierarchyLevel.h"

namespace MIAMI_MEM_REUSE
{
   #define UNKNOWN_SCOPE_ID         0
   #define UNKNOWN_INSTRUCTION_ID   0
   
   using MIAMI::addrtype;
   using std::string;

   // I need a lot of data structures to hold the memory reuse info.
   // I will reuse some of the data structures used during the collection
   // of memory reuse information
   typedef uint32_t dist_t;
   typedef uint64_t count_t;
   #define PRIdist  PRIu32
   #define PRIcount PRIu64
//   typedef float  dist_t;
//   #define PRIdist  "f"

   extern count_t defCountType;
   typedef MIAMIU::HashMap<dist_t, count_t, &defCountType, 8> HashMapCT;
   typedef MIAMIU::GenericPair<dist_t, count_t> DistCountPair;
   typedef std::vector<DistCountPair> DCVector;
   typedef std::vector<float> FloatArray;
   typedef std::vector<count_t> CountArray;

   // for each set I will collect separate reuse histograms based on the set 
   // they have reuse on. Collect cold misses separately. 
   // I will use a fast growing array for this. I do not need to delete elements
   // Search is in linear time, but it should be faster than a map for such small
   // arrays; Extend this to differentiate not only on the source set of the 
   // reuse, but also on the loop carrying the reuse.
   // 09/16/2010 gmarin: I need to differentiate also by the iteration distance of
   // the carrying scope
   //
   // 05/29/2013 gmarin: I have four fields to differentiate between different 
   // reuse patterns: source image ID, source instruction (set) index, carry 
   // image ID, and carry scope Idx. Testing equality for all four values is 
   // slow. I will use twp 64-bit ints to store all four fields, 32 bits for 
   // each. Define macros for accessing fields.
   //#define MAKE_PATTERN_KEY(img, inst, carry)  ((((uint64_t)(img))<<48) | (((uint64_t)(inst))<<24) | (uint64_t)(carry))
   #define MASK_32BIT  0xffffffffLL
   #define MAKE_PATTERN_KEY(img, idx)  ((((uint64_t)(img))<<32) | (idx))
   #define GET_PATTERN_IMG(p)     (((p)>>32) & (MASK_32BIT))
   #define GET_PATTERN_INDEX(p)   ((p) & (MASK_32BIT))

   class reuse_info_t
   {
   public:
      reuse_info_t (uint64_t _imgSrc, uint64_t _idxSrc, uint64_t _imgCarry, 
                 uint64_t _idxCarry, HashMapCT *_hist=0) : reuseHist(_hist)
      { 
         sourceId = MAKE_PATTERN_KEY(_imgSrc, _idxSrc);
         carryId  = MAKE_PATTERN_KEY(_imgCarry, _idxCarry);
      }
      
      ~reuse_info_t()
      {
         if (reuseHist)
            delete (reuseHist);
      }

      inline bool operator==(const reuse_info_t& ri) const
      {
         return (sourceId==ri.sourceId && carryId==ri.carryId);
      }
      inline bool operator!=(const reuse_info_t& ri) const
      {
         return (sourceId!= ri.sourceId || carryId!=ri.carryId);
      }
      
      HashMapCT *reuseHist;
      uint64_t sourceId, carryId;
   };
   typedef MIAMIU::SelfSortedList <reuse_info_t, 4> RI_List;

   typedef struct _inst_info
   {
      // I use a self sorted list, so the most recently accessed element is always at 
      // the front of the list, I do not use an explicit cache anymore
      RI_List *reuseVec;  // list of reuse histograms, one entry for each reuse pattern
      // the number of cold misses is equal to the total program footprint. 
      count_t totalReuses;
      count_t coldMisses;
   } InstInfo;

   // for each scope I should keep a histogram of its footprint on each execution
   // anything else?
   typedef HashMapCT ScopeInfo;

   typedef MIAMIU::DenseContainer<InstInfo, 256> IIContainer;
   typedef MIAMIU::DenseContainer<ScopeInfo, 64> SIContainer;

   // I must aggregate results at instruction and scope level. However, those IDs are
   // unique inside each image only, so I will define a structure to hold information
   // about each image.
   // I do not know the number of instruction images a priori, so I need a data structure 
   // with fast random access that also grows.
   typedef struct _image_info
   {
      IIContainer instData;
      SIContainer scopeData;
      string imgName;
      addrtype base_addr;
      addrtype low_offset;
      int32_t id;
   } ImageInfo;
   typedef MIAMIU::DenseContainer<ImageInfo, 4> ImgContainer;


   /* BlockMRDData stores the footprint and memory reuse distance information
    * for a particular block size, the way it is collected right now. 
    * Multiple memory levels may use the same cache line, so they would work
    * with the same BlockMRDData object.
    */
   class BlockMRDData
   {
   // I must store footprint information for scopes, and reuse distance 
   // information at reuse pattern level.
   // I am going to use data structures similar to the ones used during
   // data collection.
   public:
      BlockMRDData(MIAMI::MrdInfo &minfo) : fileInfo(minfo)
      {
         fileParsed = false;
      }
      inline int GetBlockSize() const   { return (fileInfo.bsize); }
      inline string GetFileName() const { return (fileInfo.name);  }
      inline int GetSampleMask() const  { return (fileInfo.bmask); }
      
      int ParseMrdFile();
      int ComputeMemoryEventsForLevel(int level, int numLevels, MIAMI::MemoryHierarchyLevel *mhl,
                 double& totalCarried, double& totalIrregCarried, double& totalFragMisses);
   
   private:
      int ParseTextMrdFile();
      int ParseBinMrdFile();
      
      ImgContainer   imgData;
      MIAMI::MrdInfo fileInfo;  // info about the mrd file
      bool           fileParsed;
   };
   typedef std::vector<BlockMRDData*> MRDDataVec;
   
}  /* namespace MIAMI_MEM_REUSE */

#endif
