/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: imix_histograms.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Outputs instruction mixes in CSV format.
 */

#include "MiamiDriver.h"
#include "miami_types.h"
#include "imix_clustering.h"
#include "imix_width_clustering.h"
#include <algorithm>


namespace MIAMI {

typedef std::multimap<int64_t, const InstructionClass*, std::greater<int64_t> > Int2ICMap;

static void 
dump_coarse_imix_for_scope(FILE *fout, ScopeImplementation *pscope)
{
   ICIMap& imix = pscope->getInstructionMixInfo();
   std::string sname = pscope->ToString();
   std::replace(sname.begin(), sname.end(), ',', ';');
   fprintf(fout, "%s", sname.c_str());
   
   int i;
   int64_t icounts[IMIX_LAST];
   for (i=IMIX_INVALID ; i<IMIX_LAST ; ++i)
      icounts[i] = 0;
   
   ICIMap::iterator tit = imix.begin();
   for ( ; tit!=imix.end() ; ++tit)
   {
      int factor = 1;
      int itype = IClassToImixType(tit->first, factor);
      icounts[itype] += (tit->second * factor);
   }
   for (i=IMIX_INVALID+1 ; i<IMIX_LAST ; ++i)
      fprintf(fout, ",%" PRId64, icounts[i]);
   fprintf(fout, "\n");
      
   ScopeImplementation::iterator it;
   for (it=pscope->begin() ; it!=pscope->end() ; ++it)
   {
      ScopeImplementation *sci = 
                dynamic_cast<ScopeImplementation*>(it->second);
      dump_coarse_imix_for_scope(fout, sci);
   }
}

static void 
dump_coarse_iwmix_for_scope(FILE *fout, ScopeImplementation *pscope)
{
   ICIMap& imix = pscope->getInstructionMixInfo();
   std::string sname = pscope->ToString();
   std::replace(sname.begin(), sname.end(), ',', ';');
   fprintf(fout, "%s", sname.c_str());
   
   int i;
   int64_t icounts[NUM_IMIX_WIDTH_BINS];
   for (i=0 ; i<NUM_IMIX_WIDTH_BINS ; ++i)
      icounts[i] = 0;
   
   ICIMap::iterator tit = imix.begin();
   for ( ; tit!=imix.end() ; ++tit)
   {
      int factor = 1;
      int ibin = IClassToImixWidthBin(tit->first, factor);
      icounts[ibin] += (tit->second * factor);
   }
   for (i=IMIXW_INVALID+1 ; i<NUM_IMIX_WIDTH_BINS ; ++i)
      fprintf(fout, ",%" PRId64, icounts[i]);
   fprintf(fout, "\n");
      
   ScopeImplementation::iterator it;
   for (it=pscope->begin() ; it!=pscope->end() ; ++it)
   {
      ScopeImplementation *sci = 
                dynamic_cast<ScopeImplementation*>(it->second);
      dump_coarse_iwmix_for_scope(fout, sci);
   }
}

static void 
dump_fine_imix_for_scope(FILE *fout, ScopeImplementation *pscope, 
         unsigned int numIBins, const InstructionClass **ic_vec)
{
   ICIMap& imix = pscope->getInstructionMixInfo();
   std::string sname = pscope->ToString();
   std::replace(sname.begin(), sname.end(), ',', ';');
   fprintf(fout, "%s", sname.c_str());
   
   for (unsigned int i=0 ; i<numIBins ; ++i)
   {
      ICIMap::iterator pit = imix.find(*(ic_vec[i]));
      if (pit!=imix.end())
         fprintf(fout, ",%" PRId64, pit->second);
      else
         fprintf(fout, ",0");
   }
   fprintf(fout, "\n");
      
   ScopeImplementation::iterator it;
   for (it=pscope->begin() ; it!=pscope->end() ; ++it)
   {
      ScopeImplementation *sci = 
                dynamic_cast<ScopeImplementation*>(it->second);
      dump_fine_imix_for_scope(fout, sci, numIBins, ic_vec);
   }
}

void 
MIAMI_Driver::dump_imix_histograms(FILE *fout, ScopeImplementation *pscope, bool use_width)
{
   // print a header row first
   // next, print one row per scope
   fprintf(fout, "Scope");
   
   if (use_width)
   {
      for (int i=IMIXW_INVALID+1 ; i<IMIXW_LAST ; ++i)
      {
         if (IMIXW_WIDTH_FIRST<=i && i<=IMIXW_WIDTH_LAST)
            for (int w=IWIDTH_INVALID+1 ; w<IWIDTH_LAST ; ++w)
               fprintf(fout, ", %s%s", ImixWTypeToString((ImixWType)i), 
                                      IWidthTypeToString((IWidthType)w));
         else
            fprintf(fout, ", %s", ImixWTypeToString((ImixWType)i));
      }
      fprintf(fout, "\n");
      
      // next, I recursively have to traverse the scope tree and print the information
      // one scope per line
      dump_coarse_iwmix_for_scope(fout, pscope);
   } else
   {
      for (int i=IMIX_INVALID+1 ; i<IMIX_LAST ; ++i)
         fprintf(fout, ", %s", ImixTypeToString((ImixType)i));
      fprintf(fout, "\n");
      
      // next, I recursively have to traverse the scope tree and print the information
      // one scope per line
      dump_coarse_imix_for_scope(fout, pscope);
   }
}

void 
MIAMI_Driver::dump_ibins_histograms(FILE *fout, ScopeImplementation *pscope)
{
   // print a header row first
   // next, print one row per scope
   fprintf(fout, "Scope");
   ICIMap& imix = pscope->getInstructionMixInfo();

   unsigned int numIBins = imix.size();
   Int2ICMap sortedImix;
   ICIMap::iterator it = imix.begin();
   for ( ; it!=imix.end() ; ++it)
      sortedImix.insert(Int2ICMap::value_type(it->second, &(it->first)));
   assert(numIBins == sortedImix.size());
   const InstructionClass **ic_vec = new const InstructionClass*[numIBins];
   
   Int2ICMap::iterator iit = sortedImix.begin();
   for (int i=0 ; iit!=sortedImix.end() ; ++iit, ++i)
   {
      ic_vec[i] = iit->second;
      fprintf(fout, ", %s-%s%02d", Convert_InstrBin_to_string(ic_vec[i]->type),
           ExecUnitTypeToString(ic_vec[i]->eu_type), ic_vec[i]->width);
      if (ic_vec[i]->eu_style == ExecUnit_VECTOR)
         fprintf(fout, ":vec{%d}", ic_vec[i]->vec_width);
   }
   fprintf(fout, "\n");
   
   // next, I recursively have to traverse the scope tree and print the information
   // one scope per line
   dump_fine_imix_for_scope(fout, pscope, numIBins, ic_vec);
   delete[] ic_vec;
}


}  /* namespace MIAMI */

