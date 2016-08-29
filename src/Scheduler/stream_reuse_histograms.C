/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: stream_reuse_histograms.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Parse and process stream simulation results.
 */

#include "MiamiDriver.h"
#include "miami_types.h"
#include "debug_scheduler.h"
#include "slice_references.h"
#include <algorithm>
#include <iostream>
#include <sstream>

using std::cerr;
using std::endl;

namespace StreamRD {
   IMG_Info **allImgs = 0;
   uint32_t maxImgs = 0;
   uint32_t loadedImgs = 0;
   count_type defCountType = 0;
   
   uint32_t history_size = 0, table_size = 0;
   uint32_t line_size = 0, max_stride = 0;
   uint32_t cache_size = 0, cache_assoc = 0;

   StreamCountBin getBinForStreamCount(int cnt)
   {
      if (cnt<0) return (NOT_A_STREAM);
      
      switch(cnt)
      {
         case 0: return (STREAMS_1);
         case 1: return (STREAMS_2);
         case 2: return (STREAMS_3);
         case 3: return (STREAMS_4);
         
         case 4:
         case 5: return (STREAMS_5_6);
         
         case 6:
         case 7: return (STREAMS_7_8);

         case 8:
         case 9:
         case 10:
         case 11: return (STREAMS_9_12);

         case 12:
         case 13:
         case 14:
         case 15: return (STREAMS_13_16);

         case 16:
         case 17:
         case 18:
         case 19: return (STREAMS_17_20);

         case 20:
         case 21:
         case 22:
         case 23: return (STREAMS_21_24);
         
         default:
            return (STREAMS_25_P);
      }
   }

   const char* getNameForStreamBin(StreamCountBin scb)
   {
      switch (scb)
      {
         case NEW_STREAMS:    return ("New");
         case STREAMS_1:      return ("1");
         case STREAMS_2:      return ("2");
         case STREAMS_3:      return ("3");
         case STREAMS_4:      return ("4");
         case STREAMS_5_6:    return ("5-6");
         case STREAMS_7_8:    return ("7-8");
         case STREAMS_9_12:   return ("9-12");
         case STREAMS_13_16:  return ("13-16");
         case STREAMS_17_20:  return ("17-20");
         case STREAMS_21_24:  return ("21-24");
         case STREAMS_25_P:   return ("25+");
         case NOT_A_STREAM:   return ("Not");
         case LARGE_STRIDE:   return ("LargeStride");
         case SYMBOLIC_STRIDE:return ("SymbolicStride");
         case IRREG_STRIDE:   return ("IrregStride");
         case NO_STRIDE:      return ("NoStride");
         case TOO_MANY_STREAMS_4:  return ("5+");
         case INNER_STREAMS_4:     return ("5+ Inner");
         case OUTER_STREAMS_4:     return ("5+ Outer");
         case TOO_MANY_STREAMS_8:  return ("9+");
         case INNER_STREAMS_8:     return ("9+ Inner");
         case OUTER_STREAMS_8:     return ("9+ Outer");
         case TOO_MANY_STREAMS_16: return ("17+");
         case INNER_STREAMS_16:    return ("17+ Inner");
         case OUTER_STREAMS_16:    return ("17+ Outer");

//         case NOT_A_STREAM_STRIDED: return ("Strided_Not_a_Stream");
//         case IS_A_STREAM_IRREG:    return ("Irreg_Is_Stream");
         default:            return ("Invalid");
      }
   }

   ScopeStreamInfo::ScopeStreamInfo (const ScopeStreamInfo& ssi)
   {
      tot_count = ssi.tot_count;
      tot_cache_misses = ssi.tot_cache_misses;
      tot_history_misses = ssi.tot_history_misses;
      tot_history_hits = ssi.tot_history_hits;
      tot_new_streams = ssi.tot_new_streams;
      tot_stream_reuses = ssi.tot_stream_reuses;
      for (int i=0 ; i<NUM_STREAM_BINS ; ++i)
         reuses[i] = ssi.reuses[i];
   }

   ScopeStreamInfo 
   ScopeStreamInfo::operator+ (const ScopeStreamInfo& ssi) const
   {
      ScopeStreamInfo temp(*this);
      temp.tot_count += ssi.tot_count;
      temp.tot_cache_misses += ssi.tot_cache_misses;
      temp.tot_history_misses += ssi.tot_history_misses;
      temp.tot_history_hits += ssi.tot_history_hits;
      temp.tot_new_streams += ssi.tot_new_streams;
      temp.tot_stream_reuses += ssi.tot_stream_reuses;
      for (int i=0 ; i<NUM_STREAM_BINS ; ++i)
         temp.reuses[i] += ssi.reuses[i];
      return (temp);
   }
   
   ScopeStreamInfo& 
   ScopeStreamInfo::operator+= (const ScopeStreamInfo& ssi)
   {
      tot_count += ssi.tot_count;
      tot_cache_misses += ssi.tot_cache_misses;
      tot_history_misses += ssi.tot_history_misses;
      tot_history_hits += ssi.tot_history_hits;
      tot_new_streams += ssi.tot_new_streams;
      tot_stream_reuses += ssi.tot_stream_reuses;
      for (int i=0 ; i<NUM_STREAM_BINS ; ++i)
         reuses[i] += ssi.reuses[i];
      return (*this);
   }

   ScopeStreamInfo& 
   ScopeStreamInfo::operator= (const ScopeStreamInfo& ssi)
   {
      tot_count = ssi.tot_count;
      tot_cache_misses = ssi.tot_cache_misses;
      tot_history_misses = ssi.tot_history_misses;
      tot_history_hits = ssi.tot_history_hits;
      tot_new_streams = ssi.tot_new_streams;
      tot_stream_reuses = ssi.tot_stream_reuses;
      for (int i=0 ; i<NUM_STREAM_BINS ; ++i)
         reuses[i] = ssi.reuses[i];
      return (*this);
   }
}

namespace MIAMI {

#define BUFFER_SIZE  512
int
MIAMI_Driver::parse_stream_reuse_data(const std::string& stream_file)
{
   FILE *fd;
   
   fd = fopen (stream_file.c_str(), "r");
   if (fd == NULL)
   {
      fprintf (stderr, "WARNING: Cannot open the file %s containing stream reuse information for references.\n",
             stream_file.c_str());
      return (-1);
   }

   // This file contains histograms of stream reuse distances, stream strides, 
   // and non-streaming access counts in the following format:
   // - First, 3 lines with comments that must be ignored. Then:
   // - one line for each image, including, imgId, img_base_addr, # distinct instructions, lib name
   // - several lines for each instruction, one line per distinct stride observed, and one generic line
   // - stride new_streams tot_reuse_streams num_bins dist1 cnt1 ...
   // - 0 PC_offset memop_idx tot_inst_count new_history history_reuse
   unsigned int img_id;
   int res, num_insts;
   addrtype img_base;
   char img_name[BUFFER_SIZE];
   bool got_errors = false;
   
   // first we have a format line for the simulation parameters
   char* crez = fgets(img_name, BUFFER_SIZE, fd);
   if (crez == NULL)
   {
      // file did not have at least 1 line of text, invalid file
      return (-2);
   }
   
   // next, we have the actual simulation parameters
   res = fscanf(fd, "%" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32,
            &StreamRD::history_size, &StreamRD::table_size,
            &StreamRD::line_size, &StreamRD::max_stride,
            &StreamRD::cache_size, &StreamRD::cache_assoc);
   if (res < 6)  // invalid file format
   {
      fprintf(stderr, "MIAMI_Driver::parse_stream_reuse_data: cannot read simulation parameters. Read %d values instead of 6.\n", res);
      return (-2);
   }

   crez = fgets(img_name, BUFFER_SIZE, fd);
   if (crez && strlen(crez)<5)  // if I read the reminder of the previous line, then read another line
      crez = fgets(img_name, BUFFER_SIZE, fd);
   crez = fgets(img_name, BUFFER_SIZE, fd);
   crez = fgets(img_name, BUFFER_SIZE, fd);
   
   if (crez == NULL)
   {
      // file did not have at least 3 lines of text, invalid file
      return (-2);
   }

   StreamRD::maxImgs = 16;
   StreamRD::allImgs = (StreamRD::IMG_Info**) malloc (StreamRD::maxImgs * sizeof (StreamRD::IMG_Info*));
   for (uint32_t i=0 ; i<StreamRD::maxImgs ; ++i)
      StreamRD::allImgs[i] = 0;
   
   int crtRow = 5;
   // next we have the actual data in the format described above
   while (!got_errors && !feof(fd))
   {
      ++ crtRow;
      res = fscanf(fd, "%d 0x%" PRIxaddr " %d", &img_id, &img_base, &num_insts);
      if (res<=0)  // it is a valid format
         break;
      if (res < 3)  // invalid file format
      {
         fprintf(stderr, "MIAMI_Driver::parse_stream_reuse_data: cannot read data for image on row %d, Read %d values instead of 3.\n", crtRow, res);
         got_errors = true; break;
      }
      // read image name
      crez = fgets(img_name, BUFFER_SIZE, fd);
      if (crez == NULL)  // invalid file format
      {
         fprintf(stderr, "MIAMI_Driver::parse_stream_reuse_data: cannot read image name for image %d on row %d\n", 
                img_id, crtRow);
         got_errors = true; break;
      }
      // remove any newline from the end of the image name
      int namelen = strlen(img_name);
      if (namelen>0 && img_name[namelen-1]=='\n')
         img_name[namelen-1] = 0;
      
      // create the IMG_Info structure for this image
      if (img_id>=StreamRD::maxImgs)  // I need to extend my array
      {
         uint32_t oldSize = StreamRD::maxImgs;
         while (StreamRD::maxImgs <= img_id)
            StreamRD::maxImgs <<= 1;
         StreamRD::allImgs = (StreamRD::IMG_Info**) realloc(StreamRD::allImgs, 
                    StreamRD::maxImgs*sizeof(StreamRD::IMG_Info*));
         for (uint32_t i=oldSize ; i<StreamRD::maxImgs ; ++i)
            StreamRD::allImgs[i] = 0;
      }
      StreamRD::IMG_Info* & newimg = StreamRD::allImgs[img_id];
      if (newimg)   // should be NULL I think
         assert(!"Seeing data for image with id seen before");

      newimg = new StreamRD::IMG_Info(img_id, img_base, num_insts); 
      newimg->img_name = img_name;
      ++ StreamRD::loadedImgs;
      
      bool new_inst = true;
      StreamRD::InstInfo *newInst = 0;
      // now parse data for num_insts instructions
      for (int inst=0 ; inst<num_insts ; )
      {
         int stride, memop, nbins;
         StreamRD::count_type tot_count, cache_misses, history_miss, history_hit, new_stream, stream_reuses;
         addrtype pc;
         
         ++ crtRow;
         res = fscanf(fd, "%d", &stride);
         if (res < 1)  // could not read stride
         {
            fprintf(stderr, "MIAMI_Driver::parse_stream_reuse_data: cannot read stride info on row %d for instr %d of image %d\n", 
                     crtRow, inst, img_id);
            got_errors = true; break;
         }
         if (new_inst)  // create an InstInfo object
         {
            newInst = new StreamRD::InstInfo();
            new_inst = false;
         }
         
         // now, we parse different things depending if this is a stride 0 (last line for
         // current instruction), or stride!=0 (actual stream data)
         if (stride)   
         {
            res = fscanf(fd, "%" PRIcnt " %" PRIcnt " %d", &new_stream, &stream_reuses, &nbins);
            if (res < 3)   // bad file format
            {
               fprintf(stderr, "MIAMI_Driver::parse_stream_reuse_data: cannot read stream data for stride %d, on row %d, for instr %d of image %d\n", 
                        stride, crtRow, inst, img_id);
               got_errors = true; break;
            }
            if (!newInst->siList)
               newInst->siList = new StreamRD::InstInfo::StrideInfoList();
            // create a StrideInfo object for this stride
            StreamRD::InstInfo::StrideInfo *si_obj = 0;
            newInst->siList->push_back(StreamRD::InstInfo::StrideInfo(stride));
            si_obj = &(newInst->siList->back());
            si_obj->newStream = new_stream;
            si_obj->streamReuses = stream_reuses;
            
            // now parse the histogram
            for (int j=0 ; j<nbins ; ++j)
            {
               int dist;
               StreamRD::count_type count;
               res = fscanf(fd, "%d %" PRIcnt, &dist, &count);
               if (res < 2)   // bad file format
               {
                  fprintf(stderr, "MIAMI_Driver::parse_stream_reuse_data: cannot read reuse histogram bin %d, for stride %d, on row %d, for instr %d of image %d\n", 
                           j, stride, crtRow, inst, img_id);
                  got_errors = true; break;
               }
               si_obj->reuseHist.push_back(StreamRD::CTPair(dist, count));
            }
            
         } else   // stride == 0, read the generic stuff for the instruction
         {
            // now we get the instruction offset, memop, and non-streaming info
            res = fscanf(fd, "%" PRIxaddr " %d %" PRIcnt " %" PRIcnt " %" PRIcnt " %" PRIcnt,
                  &pc, &memop, &tot_count, &cache_misses, &history_miss, &history_hit);
            if (res < 6)
            {
               fprintf(stderr, "MIAMI_Driver::parse_stream_reuse_data: cannot read non-streaming data on row %d, for instr %d of image %d\n", 
                        crtRow, inst, img_id);
               got_errors = true; break;
            }
            newInst->totCount = tot_count;
            newInst->totMisses = cache_misses;
            newInst->noStream = history_miss;
            newInst->repeatAccess = history_hit;
            
            // add the instruction index to the instrMapper, 
            // and add it to the image data structure
            newimg->setInstMapping(pc, memop, inst);
            assert (newimg->_instData[inst] == 0);
            newimg->_instData[inst] = newInst;
            
            ++ inst;
            new_inst = true;
            newInst = 0;
         }

      }  // for each instruction
   }  // while not end-of-file

   fclose(fd);
   if (got_errors)
   {
      return (-2);
   } else
      return (0);
}

static int 
processBinToVector(void* arg0, unsigned int key, void* value)
{
   StreamRD::CTPairArray* histogram = (StreamRD::CTPairArray*)arg0;
   StreamRD::count_type* pValue = (StreamRD::count_type*)value;
   histogram->push_back(StreamRD::CTPair(key, *pValue) );
   return 0;
}

void
MIAMI_Driver::compute_stream_reuse_for_scope(FILE *fout, ScopeImplementation *pscope, uint32_t img_id,
          AddrIntSet& scopeMemRefs, RFormulasMap& refFormulas, addrtype img_base)
{
   int i;
#if DEBUG_STREAM_REUSE
   DEBUG_STREAMS(1,
      fprintf(stderr, "Computing stream reuse for scope %s, in img %d, num mem refs %ld\n",
           pscope->ToString().c_str(), img_id, scopeMemRefs.size());
   )
#endif
   if (! scopeMemRefs.empty() && img_id>=0 && img_id < StreamRD::maxImgs && 
         StreamRD::allImgs && StreamRD::allImgs[img_id])
   {
      TimeAccount& stats = pscope->getExclusiveTimeStats();
      
      // I need to build the aggregate histogram for this scope
      // For this, I will have to use a histogram data structures because 
      // I don't know before hand how many distinct distances I can see.
      StreamRD::IMG_Info *img = StreamRD::allImgs[img_id];
      StreamRD::ScopeStreamInfo& scopeData = pscope->getScopeStreamInfo();
      
      AddrIntSet::iterator ait = scopeMemRefs.begin();
      for ( ; ait!=scopeMemRefs.end() ; ++ait)
      {
         AddrIntPair refinfo(ait->first-img_base, ait->second);
#if DEBUG_STREAM_REUSE
         DEBUG_STREAMS(4,
            fprintf(stderr, "Searching for mem ref at 0x%" PRIxaddr " / %d\n",
                   refinfo.first, refinfo.second);
         )
#endif
         // find if we have any stream information associated with this ref
         StreamRD::RefAddrMap::iterator rit = img->instMapper.find(refinfo);
         if (rit != img->instMapper.end())
         {
#if DEBUG_STREAM_REUSE
            DEBUG_STREAMS(4,
               fprintf(stderr, "Found ref at 0x%" PRIxaddr " / %d\n",
                      refinfo.first, refinfo.second);
            )
#endif

            // find if this reference has a small constant stride, and at what loop level
            // Small constant is defined as <= max SRD stride
            // This is not entirely accurate because in the presence of unrolling the
            // detected stride is larger (by the unroll factor). I have to understand which
            // references form a group, and do this analysis at group level.
            // **** TODO ****
            //
            // find the RefFormulas for this memory operand
            RefFormulas* rf = refFormulas.hasFormulasFor(ait->first, ait->second);
            int numStrides = 0;
            if (rf)
               numStrides = rf->NumberOfStrides();
            
            int irreg_level = -1, symbolic_level = -1, zero_levels = 0;
            // find the loop level with the smaller non-zero constant stride
            int minLevel = -1;
            int64_t minStride = 0;
            for (i=0 ; i<numStrides ; ++i)
            {
               GFSliceVal& stride = rf->strides[i];
               if (stride.is_uninitialized())
               {
                  std::cerr << "Booo, I found uninitialized stride formula at level " 
                       << i << " for uop at pc 0x" << std::hex << refinfo.first << std::dec
                       << ", idx=" << refinfo.second << std::endl;
                  irreg_level = i;
                  break;
               }
               if (stride.has_irregular_access() || stride.has_indirect_access())
               {
                  irreg_level = i;
                  break;
               }
               coeff_t valueNum;
               ucoeff_t valueDen;
               if (IsConstantFormula(stride, valueNum, valueDen))
               {
                  if (valueNum==0) 
                     zero_levels += 1;
                  else  // valueNum != 0
                     if (valueDen==1)
                     {
                        int64_t absval = llabs(valueNum);
                        if (!minStride || minStride > absval)
                        {
                           minStride = absval;
                           minLevel = i;
                        }
                     }
               } else
                  if (symbolic_level<0)
                     symbolic_level = i;
            }
            
            bool is_inner = false, is_outer = false, is_large = false, is_irreg = false, is_symbolic = false;
            if (minStride)  // seen a non-zero constant stride before an irregular stride
            {
               if (minStride<=(StreamRD::line_size * StreamRD::max_stride))
               {
                  if (minLevel==0)
                     is_inner = true;
                  else
                     is_outer = true;
               } else
                  is_large = true;
            }    // no non-zero constant stride ...
            else if (symbolic_level>=0)
               is_symbolic = true;
            else if (irreg_level>=0)
               is_irreg = true;
#if DEBUG_STREAM_REUSE
            else  // no non-zero strides found
            {
               // print the reference's symbolic formulas for debugging
               DEBUG_STREAMS(2,
                  cerr << "Reference at pc 0x" << std::hex << ait->first 
                       << " (offset 0x" << refinfo.first << ")" << std::dec
                       << ", idx=" << ait->second << " has no non-zero stride out of "
                       << numStrides << " stride formulas. Formulas below:" << endl;
                  if (rf)
                  {
                     // print base formula and all strides
                     cerr << " - BaseF: " << rf->base << endl;
                     for (i=0 ; i<numStrides ; ++i)
                        cerr << " - StrideF " << i << "/" << numStrides
                             << ": " << rf->strides[i] << endl;
                  } else
                     cerr << " - NO FORMLAS found!" << endl;
               )
            }
#endif
            
            StreamRD::InstInfo *iinfo = img->_instData[rit->second];
            assert(iinfo);
            scopeData.tot_count += iinfo->totCount;
            scopeData.tot_cache_misses += iinfo->totMisses;
            scopeData.tot_history_misses += iinfo->noStream;
            scopeData.tot_history_hits += iinfo->repeatAccess;
            StreamRD::count_type notAStream = iinfo->noStream + iinfo->repeatAccess;
            
            if (is_inner)
               scopeData.reuses[StreamRD::INNER_STREAMS_16] += notAStream;
            else if (is_outer)
               scopeData.reuses[StreamRD::OUTER_STREAMS_16] += notAStream;
            else if (is_large)
               scopeData.reuses[StreamRD::LARGE_STRIDE] += notAStream;
            else if (is_symbolic)
               scopeData.reuses[StreamRD::SYMBOLIC_STRIDE] += notAStream;
            else if (is_irreg)
               scopeData.reuses[StreamRD::IRREG_STRIDE] += notAStream;
            else if (rf && zero_levels==numStrides)  // no strides, it may be a routine body
               scopeData.reuses[StreamRD::NO_STRIDE] += notAStream;
            
            int delta = 0;
            if (is_inner) delta = 1;
            else if (is_outer) delta = 2;
            
            if (iinfo->siList)
            {
               StreamRD::InstInfo::StrideInfoList::iterator lit = iinfo->siList->begin();
               for ( ; lit!=iinfo->siList->end() ; ++lit)
               {
                  scopeData.tot_new_streams += lit->newStream;
                  scopeData.tot_stream_reuses += lit->streamReuses;
                  // new streams should not be classified as not a stream
                  // The prefetcher must initiate a data fetch request for the
                  // next line when the stream is created.
                  //notAStream += lit->newStream;
                  
                  StreamRD::CTPairArray::iterator ctit = lit->reuseHist.begin();
                  for ( ; ctit!=lit->reuseHist.end() ; ++ctit)
                  {
                     if (ctit->first >= 16)
                        scopeData.reuses[StreamRD::TOO_MANY_STREAMS_16+delta] += ctit->second;
                     else if (ctit->first >= 8)
                        scopeData.reuses[StreamRD::TOO_MANY_STREAMS_8+delta] += ctit->second;
                     else if (ctit->first >= 4)
                        scopeData.reuses[StreamRD::TOO_MANY_STREAMS_4+delta] += ctit->second;
                     
                     scopeData.reuses[StreamRD::getBinForStreamCount(ctit->first)] += ctit->second;
//                     scopeData.reuses[ctit->first] += ctit->second;
                  }
               }  // iterate over list of strides
            }  // if we have a lis of strides
            scopeData.reuses[StreamRD::NOT_A_STREAM] += notAStream;
/*
            if (is_irreg)
               scopeData.reuses[StreamRD::IS_A_STREAM_IRREG] += (iinfo->totMisses - notAStream);
            else
               scopeData.reuses[StreamRD::NOT_A_STREAM_STRIDED] += notAStream;
*/
         }  // iterate over all entries corresponding to this PC
      }  // iterate over the refs in one scope
      
      // finalize the aggregated metrics for this scope
      scopeData.reuses[StreamRD::NEW_STREAMS] = scopeData.tot_new_streams;

      scopeData.reuses[StreamRD::TOO_MANY_STREAMS_16] += 
               scopeData.reuses[StreamRD::INNER_STREAMS_16] + 
               scopeData.reuses[StreamRD::OUTER_STREAMS_16];
      scopeData.reuses[StreamRD::TOO_MANY_STREAMS_8] += 
               scopeData.reuses[StreamRD::INNER_STREAMS_8] + 
               scopeData.reuses[StreamRD::OUTER_STREAMS_8];
      scopeData.reuses[StreamRD::TOO_MANY_STREAMS_4] += 
               scopeData.reuses[StreamRD::INNER_STREAMS_4] + 
               scopeData.reuses[StreamRD::OUTER_STREAMS_4];

      scopeData.reuses[StreamRD::TOO_MANY_STREAMS_8] += scopeData.reuses[StreamRD::TOO_MANY_STREAMS_16];
      scopeData.reuses[StreamRD::INNER_STREAMS_8] += scopeData.reuses[StreamRD::INNER_STREAMS_16];
      scopeData.reuses[StreamRD::OUTER_STREAMS_8] += scopeData.reuses[StreamRD::OUTER_STREAMS_16];
      
      scopeData.reuses[StreamRD::TOO_MANY_STREAMS_4] += scopeData.reuses[StreamRD::TOO_MANY_STREAMS_8];
      scopeData.reuses[StreamRD::INNER_STREAMS_4] += scopeData.reuses[StreamRD::INNER_STREAMS_8];
      scopeData.reuses[StreamRD::OUTER_STREAMS_4] += scopeData.reuses[StreamRD::OUTER_STREAMS_8];

      
      stats.addMemReferenceCount(scopeData.tot_count);
      stats.addCacheMissesCount(scopeData.tot_cache_misses);
//      stats.addNoStreamCount(scopeData.reuses[StreamRD::NOT_A_STREAM]);
      for (int ii=0 ; ii<StreamRD::NUM_STREAM_BINS ; ++ii)
         stats.addStreamBinCount(ii, scopeData.reuses[ii]);
      
      // we have the full counts for this scope now; 
      // print its totals, then print data for its individual instructions
      // finally, add the data to the parent scope
#if USE_HISTOGRAM_DS
      StreamRD::CTPairArray *histogram = new StreamRD::CTPairArray();
      scopeData.reuses.map(processBinToVector, histogram);
      // The complete histogram of the instruction must be in the histogram array
      // Sort it and process it
      std::sort(histogram->begin(), histogram->end(), StreamRD::CTPair::OrderAscByKey1());
#endif

      // we are going to use the external aggregate_for_scope method
#if AGGREGATE_TO_PARENT
      ScopeImplementation *parent = pscope->Parent();
      if (parent)
      {
         StreamRD::ScopeStreamInfo& parentData = parent->getScopeStreamInfo();
         parentData.tot_count += scopeData.tot_count;
         parentData.tot_cache_misses += scopeData.tot_cache_misses;
         parentData.tot_history_misses += scopeData.tot_history_misses;
         parentData.tot_history_hits += scopeData.tot_history_hits;
         parentData.tot_new_streams += scopeData.tot_new_streams;
         parentData.tot_stream_reuses += scopeData.tot_stream_reuses;
         
         // add counts to the parent scope
         StreamRD::CTPairArray::iterator ctit = histogram->begin();
         for ( ; ctit!=histogram->end() ; ++ctit)
         {
            parentData.reuses[ctit->first] += ctit->second;
         }
      }
#endif
      
      // finally, print the data, only if we have scopes and linemap info
      if (mo->do_scopetree && mo->do_linemap)
      {
         fprintf(fout, "Scope, %s\n", pscope->ToString().c_str());
         fprintf(fout, "Total, %" PRIcnt ", %" PRIcnt ", %" PRIcnt ", %" PRIcnt ", %" PRIcnt ", %" PRIcnt ", %ld\n",
             scopeData.tot_count, scopeData.tot_cache_misses, scopeData.tot_history_misses, 
             scopeData.tot_history_hits, scopeData.tot_new_streams, 
             scopeData.tot_stream_reuses, (long)StreamRD::NUM_STREAM_BINS);
#if USE_HISTOGRAM_DS
             histogram->size());
         if (histogram->size() > 0)
         {
            fprintf(fout, "Total-streamReuse-dist");
            StreamRD::CTPairArray::iterator it = histogram->begin();
            for ( ; it!=histogram->end() ; ++it)
            {
               fprintf(fout, ", %d", it->first);
            }
            fprintf(fout, "\n");
         
            fprintf(fout, "Total-streamReuse-count");
            for (it = histogram->begin() ; it!=histogram->end() ; ++it)
            {
               fprintf(fout, ", %" PRIcnt, it->second);
            }
            fprintf(fout, "\n");
         }
#else
            fprintf(fout, "Total-streamReuse-dist");
            for (int ii=0 ; ii<StreamRD::NUM_STREAM_BINS ; ++ii)
               fprintf(fout, ", %s", StreamRD::getNameForStreamBin((StreamRD::StreamCountBin)ii));
            fprintf(fout, "\n");
            fprintf(fout, "Total-streamReuse-count");
            for (int ii=0 ; ii<StreamRD::NUM_STREAM_BINS ; ++ii)
               fprintf(fout, ", %" PRIcnt, scopeData.reuses[ii]);
            fprintf(fout, "\n");
#endif
         
         // print also the info for the individual instructions in this scope
         ait = scopeMemRefs.begin();
         for ( ; ait!=scopeMemRefs.end() ; ++ait)
         {
            AddrIntPair refinfo(ait->first-img_base, ait->second);
            // find if we have any memory latency information associated with this ref
//            std::pair<StreamRD::RefAddrMap::iterator, StreamRD::RefAddrMap::iterator> res = 
//                  img->instMapper.equal_range(*ait);
            StreamRD::RefAddrMap::iterator rit = img->instMapper.find(refinfo);
            if (rit != img->instMapper.end())
            {
               // find the RefFormulas for this memory operand
               RefFormulas* rf = refFormulas.hasFormulasFor(ait->first, ait->second);
            
               StreamRD::InstInfo *iinfo = img->_instData[rit->second];
               assert(iinfo);
               fprintf(fout, "%" PRIxaddr "-%d-0, %" PRIcnt ", %" PRIcnt ", %" PRIcnt ", %" PRIcnt ,
                     ait->first, ait->second, iinfo->totCount, iinfo->totMisses,
                     iinfo->noStream, iinfo->repeatAccess);

               if (rf)
               {
                  std::ostringstream oss;
                  oss << "base: " << rf->base;
                  for (unsigned int s=0 ; s<rf->NumberOfStrides() ; ++s)
                     oss << ", S" << s << ": " << rf->strides[s];
                  fprintf(fout, ", %s\n", oss.str().c_str());
               } else
                  fprintf(fout, ", no-formulas\n");
                     
               if (iinfo->siList)
               {
                  StreamRD::InstInfo::StrideInfoList::iterator lit = iinfo->siList->begin();
                  for ( ; lit!=iinfo->siList->end() ; ++lit)
                  {
                     fprintf(fout, "%" PRIxaddr "-%d-%d, %" PRIcnt ", %" PRIcnt ", %ld",
                         ait->first, ait->second, lit->stride,
                         lit->newStream, lit->streamReuses, lit->reuseHist.size());
                     StreamRD::CTPairArray::iterator ctit = lit->reuseHist.begin();
                     for ( ; ctit!=lit->reuseHist.end() ; ++ctit)
                     {
                        fprintf(fout, ", %d, %" PRIcnt, ctit->first, ctit->second);
                     }
                     fprintf(fout, "\n");
                  }  // iterate over list of strides
               }  // if we have a lis of strides
            }  // iterate over all entries corresponding to this PC
         }  // iterate over the refs in one scope
      }  // if do_streams && do_linemap
      
   }  // should we aggregate stream reuse histograms?

}

void 
MIAMI_Driver::dump_stream_count_histograms(FILE *fout, ScopeImplementation *pscope)
{
   // print a header row first
   // next, print one row per scope
   fprintf(fout, "Scope,Refs,Misses");
   for (int ii=0 ; ii<StreamRD::NUM_STREAM_BINS ; ++ii)
      fprintf(fout, ",%s", StreamRD::getNameForStreamBin((StreamRD::StreamCountBin)ii));
   fprintf(fout, "\n");
   
   // next, I recursively have to traverse the scope tree and print the information
   // one scope per line
   dump_streams_for_scope(fout, pscope);
}

void 
MIAMI_Driver::dump_streams_for_scope(FILE *fout, ScopeImplementation *pscope)
{
   StreamRD::ScopeStreamInfo& scopeData = pscope->getScopeStreamInfo();
   // print the scope name and the global info for this scope
   std::string sname = pscope->ToString();
   std::replace(sname.begin(), sname.end(), ',', ';');
   fprintf(fout, "%s,%" PRIcnt ",%" PRIcnt, 
      sname.c_str(),
      scopeData.tot_count, 
      scopeData.tot_cache_misses);
   for (int ii=0 ; ii<StreamRD::NUM_STREAM_BINS ; ++ii)
      fprintf(fout, ",%" PRIcnt, scopeData.reuses[ii]);
   fprintf(fout, "\n");
      
   ScopeImplementation::iterator it;
   for( it=pscope->begin() ; it!=pscope->end() ; it++ )
   {
      ScopeImplementation *sci = 
                dynamic_cast<ScopeImplementation*>(it->second);
      dump_streams_for_scope(fout, sci);
   }
}

}  /* namespace MIAMI */

