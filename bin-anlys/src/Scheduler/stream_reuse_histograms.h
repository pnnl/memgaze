/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: stream_reuse_histograms.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Parse and process stream simulation results.
 */

#ifndef _STREAM_REUSE_HISTOGRAMS_H
#define _STREAM_REUSE_HISTOGRAMS_H

#include <vector>
#include <list>
#include <set>
#include "miami_types.h"
#include "fast_hashmap.h"
#include "generic_pair.h"
#include "miami_containers.h"

namespace MIAMI
{
   extern int32_t defInt32;
}

namespace StreamRD
{
   using MIAMI::addrtype;

   // I need a lot of data structures to hold the stream reuse info.
   // I will copy the data structures used to hold the stream reuse during data collection
   typedef uint64_t count_type;
   #define PRIcnt PRIu64

   typedef MIAMIU::GenericPair<unsigned int, count_type> CTPair;
   typedef std::vector<CTPair> CTPairArray;

   extern count_type defCountType;
   typedef MIAMIU::HashMap<uint32_t, count_type, &defCountType> HashMapCT;
   
   // map from reference ID <pc, memop_idx> to instruction index
   typedef std::map<MIAMI::AddrIntPair, int, MIAMI::AddrIntPair::OrderPairs> RefAddrMap;
   
   enum StreamCountBin { NEW_STREAMS=0, STREAMS_1, STREAMS_2, STREAMS_3, STREAMS_4, STREAMS_5_6,
      STREAMS_7_8, STREAMS_9_12, STREAMS_13_16, STREAMS_17_20, STREAMS_21_24, STREAMS_25_P,
      NOT_A_STREAM, LARGE_STRIDE, SYMBOLIC_STRIDE, IRREG_STRIDE, NO_STRIDE,
      TOO_MANY_STREAMS_4, INNER_STREAMS_4, OUTER_STREAMS_4, 
      TOO_MANY_STREAMS_8, INNER_STREAMS_8, OUTER_STREAMS_8, 
      TOO_MANY_STREAMS_16, INNER_STREAMS_16, OUTER_STREAMS_16,
      NUM_STREAM_BINS };

   StreamCountBin getBinForStreamCount(int cnt);
   const char* getNameForStreamBin(StreamCountBin scb);
   
   class ScopeStreamInfo
   {
   public:
      ScopeStreamInfo() {
         tot_count = 0;
         tot_cache_misses = 0;
         tot_history_misses = 0;
         tot_history_hits = 0;
         tot_new_streams = 0;
         tot_stream_reuses = 0;
         for (int i=0 ; i<NUM_STREAM_BINS ; ++i)
            reuses[i] = 0;
      }
      
      ScopeStreamInfo (const ScopeStreamInfo& ssi);
      ScopeStreamInfo& operator= (const ScopeStreamInfo& ssi);

      ScopeStreamInfo operator+ (const ScopeStreamInfo& ssi) const;
      ScopeStreamInfo& operator+= (const ScopeStreamInfo& ssi);
      
      count_type tot_count; 
      count_type tot_cache_misses;
      count_type tot_history_misses;
      count_type tot_history_hits;
      count_type tot_new_streams;
      count_type tot_stream_reuses;
      count_type reuses[NUM_STREAM_BINS];
   };
   
   class InstInfo
   {
   public:
      class StrideInfo
      {
      public:
         StrideInfo(int _stride)
         {
            newStream = 0;
            streamReuses = 0;
            stride = _stride;
         }
         ~StrideInfo() {
         }
         
         CTPairArray reuseHist;
         count_type newStream;
         count_type streamReuses;
         int stride;
      };
      typedef std::list<StrideInfo> StrideInfoList;
      
      InstInfo () {
         totCount = 0;
         totMisses = 0;
         noStream = 0;
         repeatAccess = 0;
         siList = 0;
      }
      ~InstInfo() {
         delete (siList);
      }
   
      count_type totCount;
      count_type totMisses;
      count_type noStream;
      count_type repeatAccess;
      StrideInfoList *siList;
   };

   typedef MIAMIU::HashMap<addrtype, int32_t, &MIAMI::defInt32> HashMapInt;

   class IMG_Info
   {
   public:
      IMG_Info(int _id, addrtype _base, int num_insts)
      {
         _nextIndex = 0;
         img_id = _id;
         base_addr = _base;
         data_size = num_insts;
         _instData = (InstInfo**)malloc(data_size * sizeof(InstInfo*));
         for (int i=0 ; i<data_size ; ++i)
            _instData[i] = 0;
//            _instData[i].siList = new StreamRD::InstInfo::StrideInfoList();
      }
   
      ~IMG_Info()
      {
         for (int i=0 ; i<data_size ; ++i)
         {
            delete _instData[i];
         }
         free (_instData);
      }
   
      void setInstMapping(addrtype iaddr, int memop, int index)
      {
         instMapper.insert(RefAddrMap::value_type(
                     MIAMI::AddrIntPair(iaddr, memop), index));
      }
      
      count_type write_profile_to_file(FILE *fd);
   
      addrtype base_addr;
      std::string img_name;
      uint32_t img_id;
      RefAddrMap instMapper;
      InstInfo **_instData;
      int32_t _nextIndex;
      int data_size;
   };


}  /* namespace StreamRD */

#endif
