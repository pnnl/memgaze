/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: XML_output.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the logic for outputting performance data to
 * hpcviewer XML format.
 */

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "prog_scope.h"
#include "xml.h"
#include "uipair.h"
#include "Machine.h"
#include "TimeAccount.h"
#include "scope_reuse_data.h"
#include "MiamiDriver.h"
#include "debug_scheduler.h"

/* Implements the XML output methods of the MIAMI_Driver object.
 */
namespace MIAMI
{

#if VERBOSE_DEBUG_OUTPUT
static int64_t *trigger = 0;
#endif
/*
 * threshold - minimum contribution to any of the metrics in order to
 *   present information about a scope. Represents a fraction out of the 
 *   total value of each metric.
 */
void
MIAMI_Driver::dump_xml_output (FILE* _out_fd, ScopeImplementation *pscope, 
          const std::string& title, double threshold, bool flat_data)
{
   int indent = 0;

   int numLevels = 0;
   Machine *tmach = 0;
   if (!targets.empty())
   {
      tmach = targets.front();
      numLevels = tmach->getMemoryLevels()->NumberOfElements();
   }
         
   MIAMIU::UiUiMap TAshortNameMap;
   int memParMapping = 0;
   int carriedMissesBase = 0;
   DoubleVector threshValues;
   
//   fprintf (_out_fd, "%s", XML_header);
   fprintf (_out_fd, "<HPCVIEWER>\n<CONFIG>\n");
   fprintf (_out_fd, "  <TITLE name=\"%s (%s)\"/>\n", title.c_str(),
//               ((ProgScope*)pscope)->Name(), 
               (flat_data?"FLAT":"TREE"));
   fprintf (_out_fd, "  <PATH name=\".\"/>\n");
   fprintf (_out_fd, "  <METRICS>\n");
   TimeAccount* incStats;
   UiToDoubleMap const * incStatsMap;

   // always use the inclusive stats of the top scope to generate the metrics
   incStats = &(pscope->getInclusiveTimeStats());
   incStatsMap = &(incStats->getDataConst());
   UiToDoubleMap::const_iterator inctait = incStatsMap->begin ();
   int i = 0;
   for ( ; inctait!=incStatsMap->end(); ++inctait)
   {
      if (TAdisplayValueForKey(inctait->first, mo->DetailedMetrics()))
      {
         fprintf (_out_fd, "    <METRIC shortName=\"%d\" nativeName=\"%s\" displayName=\"%s\" display=\"true\" percent=\"%s\"/>\n",
               i, getTANameForKey(inctait->first, tmach).c_str(),
               getTAShortNameForKey(inctait->first, tmach).c_str(),
               TAcomputePercentForKey(inctait->first) );
         assert (TAshortNameMap.find (inctait->first) == TAshortNameMap.end());
         TAshortNameMap.insert (MIAMIU::UiUiMap::value_type (inctait->first, i));
         threshValues.push_back(threshold * incStats->getDisplayValue(inctait->first, tmach));
         ++ i;
      }
   }
   // I added all items into TimeAccount. But I need to add a few more which 
   // are seprate. Memory parallelism (no percent for it), 
   // number of misses carried by a scope (no percent for them either).
   if (perform_memory_analysis)
   {
      int j;
      carriedMissesBase = i; i += numLevels;
      for (j=0 ; j<numLevels ; ++j)
         if (mem_level_has_carried_info && mem_level_has_carried_info[j])
         {
            MemoryHierarchyLevel *mhl = tmach->getMemoryLevels()->
                       getElementAtIndex(j);
            std::string longname = std::string("Misses carried for level ") + 
                       mhl->getLongName();
            std::string shortname = std::string("Carried_") + mhl->getLongName();
            fprintf (_out_fd, "    <METRIC shortName=\"%d\" nativeName=\"%s\" displayName=\"%s\" display=\"true\" percent=\"true\"/>\n",
                 carriedMissesBase+j, longname.c_str(), shortname.c_str());
            threshValues.push_back(threshold * totalCarried[j]);
         } else
            threshValues.push_back(0.1);
      memParMapping = i++;
      // do not print memory parallelism yet
      /*
      fprintf (_out_fd, "    <METRIC shortName=\"%d\" nativeName=\"%s\" displayName=\"%s\" display=\"true\" percent=\"false\"/>\n",
            memParMapping, "Memory parallelism", "MEM_PAR");
      */
   }
#if VERBOSE_DEBUG_OUTPUT
   trigger = new int64_t[i];
   for (int j=0 ; j<i ; ++j)
      trigger[j] = 0;
#endif
   fprintf (_out_fd, "  </METRICS>\n");
   fprintf (_out_fd, "</CONFIG>\n<SCOPETREE>\n");
   
#if VERBOSE_DEBUG_OUTPUT
   DEBUG_OUT (2,
      DoubleVector::iterator dvit = threshValues.begin();
      fprintf(stderr, "\n --- Threshold values for each metric, flat_data=%d ---\n", 
              (int)flat_data);
      for (int j=0 ; dvit!=threshValues.end() ; ++dvit, ++j)
      {
         fprintf(stderr, " - M=%d --> %lg\n", j, *dvit);
      }
   )
#endif
   xml_dump_for_scope (_out_fd, pscope, TAshortNameMap, memParMapping, 
         carriedMissesBase, indent, numLevels, threshValues,
         flat_data, true);

#if VERBOSE_DEBUG_OUTPUT
   if (trigger)
   {
      DEBUG_OUT (1,
         fprintf(stderr, "\n --- Distribution of metric triggers, flat_data=%d ---\n", 
                 (int)flat_data);
         for (int j=0 ; j<i ; ++j)
         {
            fprintf(stderr, " - M=%d --> %" PRId64 "\n", j, trigger[j]);
         }
      )
      delete[] trigger;
      trigger = 0;
   }
#endif

   fprintf (_out_fd, "</SCOPETREE>\n</HPCVIEWER>\n");
}

#define WITH_FLAT_SCOPE_HEADERS 1
#define USE_ALIEN_SCOPES 1

void
MIAMI_Driver::dump_scope_pairs_xml_output (FILE* _out_fd, 
           ScopeImplementation *pscope, const std::string& title, double threshold)
{
   int indent = 0;

   int numLevels = 0;
   Machine *tmach = 0;
   if (!targets.empty())
   {
      tmach = targets.front();
      numLevels = tmach->getMemoryLevels()->NumberOfElements();
   }
         
   MIAMIU::UiUiMap TAshortNameMap;
   int carriedMissesBase = 0;
   int64_t filteredOutPatterns = 0;
   DoubleVector threshValues;
   
//   fprintf (_out_fd, "%s", XML_header);
   fprintf (_out_fd, "<HPCVIEWER>\n<CONFIG>\n");
   fprintf (_out_fd, "  <TITLE name=\"%s\"/>\n", title.c_str());
   fprintf (_out_fd, "  <PATH name=\".\"/>\n");
   fprintf (_out_fd, "  <METRICS>\n");

   int i = 0, j;
   if (perform_memory_analysis)
   {
      carriedMissesBase = i; i += numLevels;
      int j3;
      for (j=0, j3=0 ; j<numLevels ; j+=1, j3+=3)
      {
         if (mem_level_has_carried_info && mem_level_has_carried_info[j])
         {
            MemoryHierarchyLevel *mhl = tmach->getMemoryLevels()->
                       getElementAtIndex(j);
            std::string longname = std::string("Long reused locations for level ") + 
                       mhl->getLongName();
            std::string shortname = std::string("Reuse_") + mhl->getLongName();
            fprintf (_out_fd, "    <METRIC shortName=\"%d\" nativeName=\"%s\" displayName=\"%s\" display=\"true\" percent=\"true\"/>\n",
                 carriedMissesBase+j3, longname.c_str(), shortname.c_str());

            if (mem_level_has_irreg_carried_info && 
                    mem_level_has_irreg_carried_info[j])
            {
               longname = std::string("Irregular long reused locations for level ") + 
                          mhl->getLongName();
               shortname = std::string("IrregReuse_") + mhl->getLongName();
               fprintf (_out_fd, "    <METRIC shortName=\"%d\" nativeName=\"%s\" displayName=\"%s\" display=\"true\" percent=\"true\"/>\n",
                    carriedMissesBase+j3+1, longname.c_str(), shortname.c_str());
            }

            if (mem_level_has_frag_info && 
                    mem_level_has_frag_info[j])
            {
               longname = std::string("Non stride one reused locations for level ") + 
                          mhl->getLongName();
               shortname = std::string("FragReuse_") + mhl->getLongName();
               fprintf (_out_fd, "    <METRIC shortName=\"%d\" nativeName=\"%s\" displayName=\"%s\" display=\"true\" percent=\"true\"/>\n",
                    carriedMissesBase+j3+2, longname.c_str(), shortname.c_str());
            }
            threshValues.push_back(threshold * totalCarried[j]);
         } else
            threshValues.push_back(0.1);
      }
   } else
      std::cerr << "Requested to output memory reuse between scopes, but memory analysis was not performed."
           << std::endl;

   fprintf (_out_fd, "  </METRICS>\n");
   fprintf (_out_fd, "</CONFIG>\n<SCOPETREE>\n");

#if VERBOSE_DEBUG_OUTPUT
   DEBUG_OUT (2,
      DoubleVector::iterator dvit = threshValues.begin();
      fprintf(stderr, "\n --- Threshold values for data reuse patterns ---\n");
      for (int j=0 ; dvit!=threshValues.end() ; ++dvit, ++j)
      {
         fprintf(stderr, " - M=%d --> %lg\n", j, *dvit);
      }
   )
   trigger = new int64_t[numLevels];
   for (int j=0 ; j<numLevels ; ++j)
      trigger[j] = 0;
#endif
   
   if (perform_memory_analysis)
   {
      // I can actually output everything in this routine, because this file
      // will have a fixed depth and I do not have to traverse the scopes
      // recursively, but only traverse the reusePairs map.
      // First, write performance data at top level
      fprintf (_out_fd, "%*s%s\n", indent, "", 
                 pscope->XMLScopeHeader().c_str());

      double *carriedMisses = totalCarried;
      double *irregMisses = totalIrregCarried;
      double *fragMisses = totalFragMisses;
      int i3;
      if (carriedMisses)
         for (i=0, i3=0 ; i<numLevels ; ++i, i3+=3)
         {
            if (mem_level_has_carried_info && mem_level_has_carried_info[i] &&
                     carriedMisses[i]>0.0)
               fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                     carriedMissesBase+i3, carriedMisses[i]);
            if (mem_level_has_irreg_carried_info && mem_level_has_irreg_carried_info[i] &&
                     irregMisses && irregMisses[i]>0.0)
               fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                     carriedMissesBase+i3+1, irregMisses[i]);
            if (mem_level_has_frag_info && mem_level_has_frag_info[i] &&
                     fragMisses && fragMisses[i]>0.0)
               fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                     carriedMissesBase+i3+2, fragMisses[i]);
         }

      // Now I have to travers the PairSRDMap
      PairSRDMap::iterator srit = reusePairs.begin ();
      indent += 2;
      for ( ; srit!= reusePairs.end() ; ++srit)
      {
         // check if we have to dump this reuse pattern
         bool dump_scope = false;
         carriedMisses = srit->second->misses;
         if (carriedMisses)
            for (i=0 ; i<numLevels && !dump_scope ; ++i)
            {
               if (mem_level_has_carried_info && mem_level_has_carried_info[i] &&
                    carriedMisses[i]>threshValues[i])
               {
                  dump_scope = true;
#if VERBOSE_DEBUG_OUTPUT
                  trigger[i] += 1;
#endif
               }
            }
         if (!dump_scope)
         {
            filteredOutPatterns += 1;
            continue;
         }
            
         uint64_t sinkId = srit->first.first;
         uint64_t srcId = srit->first.second;
         int32_t srcImgId = GET_PATTERN_IMG(srcId);
         int32_t srcScopeId = GET_PATTERN_INDEX(srcId);
         int32_t sinkImgId = GET_PATTERN_IMG(sinkId);
         int32_t sinkScopeId = GET_PATTERN_INDEX(sinkId);
         
         LoadModule *srcImg = GetModuleWithIndex(srcImgId);
         assert (srcImg || !"We do not have a source module??");
         LoadModule *sinkImg = GetModuleWithIndex(sinkImgId);
         assert (sinkImg || !"We do not have a sink module??");
         
         ScopeImplementation *srcScope = srcImg->GetSIForScope(srcScopeId);
         if (!srcScope)
         {
            if (srcScopeId)  // print error message only for valid scope IDs. scope 0 is special
               fprintf(stderr, "ERROR: MIAMI_Driver::dump_scope_pairs_xml_output: Couldn't find a source ScopeImplementation for scopeId %d, in image %d (%s)\n",
                    srcScopeId, srcImgId, srcImg->Name().c_str());
            // create a ScopeNotFound scope to use when we cannot find one
            srcScope = srcImg->GetDefaultScope();
         }
         assert (srcScope || !"I still do not have a valid source scope object??");

         ScopeImplementation *sinkScope = sinkImg->GetSIForScope(sinkScopeId);
         if (!sinkScope)
         {
            if (sinkScopeId)  // print error message only for valid scope IDs. scope 0 is special
               fprintf(stderr, "ERROR: MIAMI_Driver::dump_scope_pairs_xml_output: Couldn't find a sink ScopeImplementation for scopeId %d, in image %d (%s)\n",
                    sinkScopeId, sinkImgId, sinkImg->Name().c_str());
            // create a ScopeNotFound scope to use when we cannot find one
            sinkScope = sinkImg->GetDefaultScope();
         }
         assert (sinkScope || !"I still do not have a valid source scope object??");
         
         // I should always find something
         // maybe I am not going to find some of them because I did not
         // analyze all routines. Remember to change back when we fix the
         // other problems. REMOVE THIS!!!
         assert (srcScope && sinkScope);
         if (!srcScope || !sinkScope)
         {
            fprintf (stderr, "ERROR: in XML_output did not find one of the scopes for scopeIds src %d, dest %d\n",
                   srcScopeId, sinkScopeId);
            continue;
         }
         
#if USE_ALIEN_SCOPES
         fprintf (_out_fd, "%*s<UG n=\"(expand)\">\n", indent, "");
#else
         fprintf (_out_fd, "%*s<UG n=\"%s .TO. %s\">\n", indent, "", 
                   xml::EscapeStr(srcScope->ToString().c_str()),
                   xml::EscapeStr(sinkScope->ToString().c_str()) );
#endif
         carriedMisses = srit->second->misses;
         irregMisses = srit->second->misses + numLevels;
         fragMisses = srit->second->misses + numLevels + numLevels;
         if (carriedMisses)
            for (i=0, i3=0 ; i<numLevels ; ++i, i3+=3)
            {
               if (mem_level_has_carried_info && mem_level_has_carried_info[i] && \
                        carriedMisses[i]>0)
                  fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                        carriedMissesBase+i3, carriedMisses[i]);
               if (mem_level_has_irreg_carried_info && mem_level_has_irreg_carried_info[i] && \
                        irregMisses && irregMisses[i]>0)
                  fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                        carriedMissesBase+i3+1, irregMisses[i]);
               if (mem_level_has_frag_info && mem_level_has_frag_info[i] && \
                        fragMisses && fragMisses[i]>0)
                  fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                        carriedMissesBase+i3+2, fragMisses[i]);
            }

         indent += 2;
#if USE_ALIEN_SCOPES
         // Now write an alien scope for each of the scopes
         fprintf (_out_fd, "%*s%s\n", indent, "", 
                  srcScope->AlienXMLScopeHeader("SRC: ").c_str());
         // write the total values for each of them as well
         // do not write the totals for both SRC and DEST, as they are shown for the
         // parent "(expand)" entry and the table becomes too crowded. Revert if 
         // it does not look good.
         fprintf (_out_fd, "%*s</A>\n", indent, "");

         // Same thing for the second scope
         fprintf (_out_fd, "%*s%s\n", indent, "", 
                   sinkScope->AlienXMLScopeHeader("DEST: ").c_str());
         // write the total values for each of them as well
         // see how it looks without writing totals again
#endif

         // if we have values per name, write the totals for each name
         CNameDoublePMap *allVals = &(srit->second->values);
         CNameDoublePMap::iterator dpit = allVals->begin();
         if (dpit!=allVals->end() && dpit->first.carryId==0 &&
                  dpit->first.name)
         {
#if !USE_ALIEN_SCOPES
            fprintf (_out_fd, "%*s<UG n=\"Names\">\n", indent, "");
#endif
            indent += 2;
            for ( ; dpit!=allVals->end() && dpit->first.carryId==0 ; 
                       ++dpit)
            {
               assert (dpit->first.name);
               fprintf (_out_fd, "%*s<UG n=\"%s\">\n", indent, "",
                       xml::EscapeStr(dpit->first.name).c_str());
               carriedMisses = dpit->second;
               irregMisses = dpit->second + numLevels;
               fragMisses = dpit->second + numLevels + numLevels;
               if (carriedMisses)
                  for (i=0, i3=0 ; i<numLevels ; ++i, i3+=3)
                  {
                     if (mem_level_has_carried_info && mem_level_has_carried_info[i] && \
                              carriedMisses[i]>0)
                        fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                              carriedMissesBase+i3, carriedMisses[i]);
                     if (mem_level_has_irreg_carried_info && mem_level_has_irreg_carried_info[i] && \
                              irregMisses && irregMisses[i]>0)
                        fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                              carriedMissesBase+i3+1, irregMisses[i]);
                     if (mem_level_has_frag_info && mem_level_has_frag_info[i] && \
                              fragMisses && fragMisses[i]>0)
                        fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                              carriedMissesBase+i3+2, fragMisses[i]);
                  }
               fprintf (_out_fd, "%*s</UG>\n", indent, "");
            }
            indent -= 2;
#if !USE_ALIEN_SCOPES
            fprintf (_out_fd, "%*s</UG>\n", indent, "");
#endif
         }
#if USE_ALIEN_SCOPES
         fprintf (_out_fd, "%*s</A>\n", indent, "");
#endif
         
         // Now write the reuse separrately by each carrier
         fprintf (_out_fd, "%*s<UG n=\"Carried By\">\n", indent, "");
         
         indent += 2;
         for ( ; dpit!=allVals->end() ; )
         {
            uint64_t fullCarryId = dpit->first.carryId;
            int32_t carryImgId = GET_PATTERN_IMG(fullCarryId);
            int32_t carryScopeId = GET_PATTERN_INDEX(fullCarryId);
            int loopCarried = carryScopeId & 0x1;
            int32_t targetId = carryScopeId >> 1;
            
            LoadModule *carryImg = GetModuleWithIndex(carryImgId);
            assert (carryImg || !"We do not have a carry module??");
            
            ScopeImplementation *carryScope = carryImg->GetSIForScope(targetId);
            if (!carryScope)
            {
               if (targetId)  // print error message only for valid scope IDs. scope 0 is special
                  fprintf(stderr, "ERROR: MIAMI_Driver::dump_scope_pairs_xml_output: Couldn't find a carry ScopeImplementation for scopeId %d, in image %d (%s)\n",
                       targetId, carryImgId, carryImg->Name().c_str());
               // create a ScopeNotFound scope to use when we cannot find one
               carryScope = carryImg->GetDefaultScope();
            }
            assert (carryScope || !"I still do not have a valid carry scope object??");

            // I should always find something
            assert (carryScope);
            std::string namePrefix = "";
            if (loopCarried)
               namePrefix = "LC_";
#if 0
            if (carryScope->getScopeType()==LOOP_SCOPE)
            {
               if (loopCarried)
                  namePrefix = "LC_";
               else
                  namePrefix = "LI_";
            }
#endif

#if USE_ALIEN_SCOPES
            fprintf (_out_fd, "%*s%s\n", indent, "", 
                    carryScope->AlienXMLScopeHeader(namePrefix.c_str()).c_str());
#else
            fprintf (_out_fd, "%*s<UG n=\"%s%s\">\n", indent, "", 
                    namePrefix.c_str(),
                    xml::EscapeStr(carryScope->ToString()).c_str() );
#endif
            if (dpit->first.name == 0)  // print totals for carrier
            {
               carriedMisses = dpit->second;
               irregMisses = dpit->second + numLevels;
               fragMisses = dpit->second + numLevels + numLevels;
               if (carriedMisses)
                  for (i=0, i3=0 ; i<numLevels ; ++i, i3+=3)
                  {
                     if (mem_level_has_carried_info && mem_level_has_carried_info[i] && \
                              carriedMisses[i]>0)
                        fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                              carriedMissesBase+i3, carriedMisses[i]);
                     if (mem_level_has_irreg_carried_info && mem_level_has_irreg_carried_info[i] && \
                              irregMisses && irregMisses[i]>0)
                        fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                              carriedMissesBase+i3+1, irregMisses[i]);
                     if (mem_level_has_frag_info && mem_level_has_frag_info[i] && \
                              fragMisses && fragMisses[i]>0)
                        fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                              carriedMissesBase+i3+2, fragMisses[i]);
                  }
            }
            ++ dpit;

            indent += 2;
            for ( ; dpit!=allVals->end() && dpit->first.carryId==fullCarryId ; 
                       ++dpit)
            {
               assert (dpit->first.name);
               fprintf (_out_fd, "%*s<UG n=\"%s\">\n", indent, "",
                       xml::EscapeStr(dpit->first.name).c_str());
               carriedMisses = dpit->second;
               irregMisses = dpit->second + numLevels;
               fragMisses = dpit->second + numLevels + numLevels;
               if (carriedMisses)
                  for (i=0, i3=0 ; i<numLevels ; ++i, i3+=3)
                  {
                     if (mem_level_has_carried_info && mem_level_has_carried_info[i] && \
                              carriedMisses[i]>0)
                        fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                              carriedMissesBase+i3, carriedMisses[i]);
                     if (mem_level_has_irreg_carried_info && mem_level_has_irreg_carried_info[i] && \
                              irregMisses && irregMisses[i]>0)
                        fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                              carriedMissesBase+i3+1, irregMisses[i]);
                     if (mem_level_has_frag_info && mem_level_has_frag_info[i] && \
                              fragMisses && fragMisses[i]>0)
                        fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                              carriedMissesBase+i3+2, fragMisses[i]);
                  }
               fprintf (_out_fd, "%*s</UG>\n", indent, "");
            }
            indent -= 2;
            
#if USE_ALIEN_SCOPES
            fprintf (_out_fd, "%*s</A>\n", indent, "");
#else
            fprintf (_out_fd, "%*s</UG>\n", indent, "");
#endif
         }

         indent -= 2;
         fprintf (_out_fd, "%*s</UG>\n", indent, "");

         indent -= 2;
         fprintf (_out_fd, "%*s</UG>\n", indent, "");
      }
      indent -= 2;
      fprintf (_out_fd, "%*s%s\n", indent, "", 
                pscope->XMLScopeFooter().c_str());
   }
#if VERBOSE_DEBUG_OUTPUT
   if (trigger)
   {
      DEBUG_OUT (1,
         fprintf(stderr, "\n --- Distribution of metric triggers for data reuse patterns ---\n");
         for (int j=0 ; j<numLevels ; ++j)
         {
            fprintf(stderr, " - M=%d --> %" PRId64 "\n", j, trigger[j]);
         }
         fprintf(stderr, " --- Reuse patterns not displayed = %" PRId64 " ---\n", 
                filteredOutPatterns);
      )
      delete[] trigger;
      trigger = 0;
   }
#endif

   fprintf (_out_fd, "</SCOPETREE>\n</HPCVIEWER>\n");
}


void
MIAMI_Driver::xml_dump_for_scope (FILE *_out_fd, ScopeImplementation *pscope, 
         MIAMIU::UiUiMap &TAshortNameMap, int memParMapping, int carriedMissesBase, 
         int indent, int numLevels, DoubleVector& threshValues,
         bool flat_data, bool top_scope)
{
   int i;
   Machine *tmach = 0;
   if (!targets.empty())
      tmach = targets.front();
   
   bool is_file = (pscope->getScopeType() == FILE_SCOPE);
   bool is_loop = (pscope->getScopeType() == LOOP_SCOPE);
   TimeAccount* incStats;
   CharDoublePMap* incNames;
   if (!flat_data || top_scope)
   {
      incStats = &(pscope->getInclusiveTimeStats ());
      incNames = &(pscope->getInclusiveStatsPerName ());
   }
   else
   {
      incStats = &(pscope->getExclusiveTimeStats ());
      incNames = &(pscope->getExclusiveStatsPerName ());
   }
   
   UiToDoubleMap const *incStatsMap = &(incStats->getDataConst());
   // Check if any of the metrics exceed the threshold
   bool dump_scope = false;
   // if this is a top-down (not flat) output, we should not exclude any
   // of the loops of a routine. We can exclude a file or routine in 
   // its entirety.
   if (top_scope || (is_file && flat_data))
      dump_scope = true;
   else
   {
      UiToDoubleMap::const_iterator inctait = incStatsMap->begin();
      const char* percT;
      for ( ; !dump_scope && inctait!=incStatsMap->end(); ++inctait)
      {
         if (TAkeyActsAsThreshold(inctait->first) &&
             (percT=TAcomputePercentForKey(inctait->first))!=0 && percT[0]=='t')
         {
            MIAMIU::UiUiMap::iterator uit = TAshortNameMap.find(inctait->first);
            assert (uit != TAshortNameMap.end());
            assert(uit->second>=0 && uit->second<threshValues.size());
            double dval = incStats->getDisplayValue(inctait->first, tmach);
            if ((dval>threshValues[uit->second]) ||  // greater than threshold, we are good
                (is_loop && !flat_data && dval>0.))
            {
               dump_scope = true;
#if VERBOSE_DEBUG_OUTPUT
               trigger[uit->second] += 1;
#endif
            }
         }
      }
      
      // if we did not pass the mustard, we have one more chance to be a 
      // scope that carries a large number of data reuses
      if (!dump_scope && flat_data && perform_memory_analysis)
      {
         double *carriedMisses = NULL, *LIcarriedMisses = NULL;
         carriedMisses = pscope->CarriedMisses();
         LIcarriedMisses = pscope->LICarriedMisses();
         if (carriedMisses || LIcarriedMisses) 
         {
            for (i=0 ; i<numLevels && !dump_scope ; ++i) {
               if (mem_level_has_carried_info && mem_level_has_carried_info[i])
               {
                  double allCarriedMisses = 0.0;
                  if (carriedMisses)
                     allCarriedMisses += carriedMisses[i];
                  if (LIcarriedMisses)
                     allCarriedMisses += LIcarriedMisses[i];
                  if (allCarriedMisses > threshValues[carriedMissesBase+i])
                  {
                     dump_scope = true;
#if VERBOSE_DEBUG_OUTPUT
                     trigger[carriedMissesBase+i] += 1;
#endif
                  }
               }
            }
         }
      }
   }

   if (dump_scope)
   {
      // print the XML header for every scope if we have a top-down profile
      // for flat profiles, print the XML header only for the root node
      // try the XML header for all scopes since we have a file scope above.
      // I may need to add a dummy group scope as well if loops are not allowed
      // directly under files
   #if WITH_FLAT_SCOPE_HEADERS
      if (!flat_data || top_scope || is_file || !incStats->getDataConst().empty())
      {
         if (flat_data && !top_scope && pscope->getScopeType()==LOOP_SCOPE)
            fprintf (_out_fd, "%*s%s\n", indent, "", 
                       pscope->AlienXMLScopeHeader().c_str());
         else
            fprintf (_out_fd, "%*s%s\n", indent, "", 
                       pscope->XMLScopeHeader().c_str());
      }
   #else
      if (!flat_data || top_scope || is_file)
         fprintf (_out_fd, "%*s%s\n", indent, "", 
                    pscope->XMLScopeHeader().c_str());
      else if (!incStats->empty())  // do not print anything for an empty scope
         fprintf (_out_fd, "%*s<G n=\"%s\">\n", indent, "", 
                    xml::EscapeStr(pscope->ToString()).c_str() );
   #endif

      dump_line_mapping_for_scope(_out_fd, pscope, indent);
      
      UiToDoubleMap::const_iterator inctait = incStatsMap->begin();
      for ( ; inctait!=incStatsMap->end(); ++inctait)
      {
         if (TAdisplayValueForKey(inctait->first, mo->DetailedMetrics()))
         {
            MIAMIU::UiUiMap::iterator uit = TAshortNameMap.find(inctait->first);
            assert (uit != TAshortNameMap.end());
            fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                        uit->second, 
                        incStats->getDisplayValue(inctait->first, tmach));
         }
      }
      if (perform_memory_analysis)
      {
         double *carriedMisses = NULL, *LIcarriedMisses = NULL, *allCarriedMisses = NULL;
         if (top_scope)
            allCarriedMisses = totalCarried;
         else
         {
            carriedMisses = pscope->CarriedMisses();
            LIcarriedMisses = pscope->LICarriedMisses();
            if (carriedMisses || LIcarriedMisses) {
               allCarriedMisses = new double[numLevels];
               for (i=0 ; i<numLevels ; ++i) {
                  allCarriedMisses[i] = 0.0;
                  if (carriedMisses)
                     allCarriedMisses[i] += carriedMisses[i];
                  if (LIcarriedMisses)
                     allCarriedMisses[i] += LIcarriedMisses[i];
               }
            }
         }
         if (allCarriedMisses)
         {
            for (i=0 ; i<numLevels ; ++i)
               if (mem_level_has_carried_info && mem_level_has_carried_info[i] &&
                         allCarriedMisses[i]>0)
                  fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                        carriedMissesBase+i, allCarriedMisses[i]);
            if (! top_scope)
               delete[] allCarriedMisses;
         }
         
         // do not print memory parallelism at this time. We do not 
         // compute serial and exposed memory latency
         /*
         float serialMemLat = 0;
         float exposedMemLat = 0;
         if (flat_data)
         {
            serialMemLat = pscope->getExclusiveSerialMemLat();
            exposedMemLat = pscope->getExclusiveExposedMemLat();
         } else
         {
            serialMemLat = pscope->getSerialMemLat();
            exposedMemLat = pscope->getExposedMemLat();
         }
         
         if (exposedMemLat > 0.1f)
            fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%g\"/>\n", indent, "", 
                  memParMapping, serialMemLat/exposedMemLat);
         */
      }
      
      if (!incStatsMap->empty() || is_file)
         indent += 2;
      
      if (perform_memory_analysis)
      {
         // print reuse data for this scope
         Trio64DoublePMap& reuseMap = pscope->getReuseFromScopeMap ();
         if (!reuseMap.empty ())
         {
            // if we have any miss information, I will create /*a group for
            // cold misses and*/ two groups representing reuse info organised in two
            // different ways. 
            // Once with root "Reuse source" under which we have scopes and under 
            // scopes we have "Carried by" and another level of scopes.
            // Second group organized by "Carried by" first and then by 
            // "Reuse source".

            // I need to process the map and get the number of distinct reuseOn 
            // sources and the number of distinct carriedBy info.
            int numLevels2 = numLevels + numLevels;
            int numLevels3 = numLevels + numLevels2;
            
            UI64DoublePMap dReuse, dCarrier;  // distinct reuse and carrier values
            UI64DoublePMap::iterator uit;
            Trio64DoublePMap::iterator dpit = reuseMap.begin ();
            for ( ; dpit!=reuseMap.end() ; ++dpit)
            {
               double fragFactor = dpit->second[numLevels+1];
               bool is_irreg = (dpit->second[numLevels] > 0.0);
               bool is_frag = (fragFactor > 0.0);
               uit = dReuse.find (dpit->first.first);
               if (uit==dReuse.end())
               {
                  uit = dReuse.insert (dReuse.begin(), 
                        UI64DoublePMap::value_type (dpit->first.first,
                        new double[numLevels3]));
                  for (i=0 ; i<numLevels ; ++i)
                  {
                     uit->second[i] = dpit->second[i];
                     if (is_irreg)
                        uit->second[i+numLevels] = dpit->second[i];
                     else
                        uit->second[i+numLevels] = 0.0;
                     if (is_frag && dpit->second[i]>0.0)
                        uit->second[i+numLevels2] = dpit->second[i]*fragFactor;
                     else
                        uit->second[i+numLevels2] = 0.0;
                  }
               } else
                  for (i=0 ; i<numLevels ; ++i)
                  {
                     uit->second[i] += dpit->second[i];
                     if (is_irreg)
                        uit->second[i+numLevels] += dpit->second[i];
                     if (is_frag && dpit->second[i]>0.0)
                        uit->second[i+numLevels2] += dpit->second[i]*fragFactor;
                  }
               
               uit = dCarrier.find (dpit->first.second);
               if (uit==dCarrier.end())
               {
                  uit = dCarrier.insert (dCarrier.begin(), 
                        UI64DoublePMap::value_type (dpit->first.second,
                        new double[numLevels3]));
                  for (i=0 ; i<numLevels ; ++i)
                  {
                     uit->second[i] = dpit->second[i];
                     if (is_irreg)
                        uit->second[i+numLevels] = dpit->second[i];
                     else
                        uit->second[i+numLevels] = 0.0;
                     if (is_frag && dpit->second[i]>0.0)
                        uit->second[i+numLevels2] = dpit->second[i]*fragFactor;
                     else
                        uit->second[i+numLevels2] = 0.0;
                  }
               } else
                  for (i=0 ; i<numLevels ; ++i)
                  {
                     uit->second[i] += dpit->second[i];
                     if (is_irreg)
                        uit->second[i+numLevels] += dpit->second[i];
                     if (is_frag && dpit->second[i]>0.0)
                        uit->second[i+numLevels2] += dpit->second[i]*fragFactor;
                  }
            }
            
            double incCpuTime = incStats->getTotalCpuTime ();
            fprintf (_out_fd, "%*s<UG n=\"@Reuse Source\">\n", indent, "");
            dumpOneReuseMap (_out_fd, TAshortNameMap, dReuse, &reuseMap, 
                  indent+2, numLevels, incCpuTime, true, false);
            fprintf (_out_fd, "%*s</UG>\n", indent, "");

            fprintf (_out_fd, "%*s<UG n=\"@Reuse Carrier\">\n", indent, "");
            dumpOneReuseMap (_out_fd, TAshortNameMap, dCarrier, &reuseMap, 
                  indent+2, numLevels, incCpuTime, false, true);
            fprintf (_out_fd, "%*s</UG>\n", indent, "");
            
            // deallocate dReuse and dCarrier
            for (uit=dReuse.begin() ; uit!=dReuse.end() ; ++uit)
               delete[] (uit->second);
            dReuse.clear ();
            for (uit=dCarrier.begin() ; uit!=dCarrier.end() ; ++uit)
               delete[] (uit->second);
            dCarrier.clear ();
         }

         // print carried misses data for this scope
         Pair64DoublePMap *carryMap = &(pscope->getCarriedForScopeMap());
         if (!carryMap->empty ())
         {
            // if we have any carried miss information, I will create two groups 
            // representing carried info organised in two different ways. 
            // Once with root "Carried From" under which we have scopes and under 
            // scopes we have "Carried To" and another level of scopes.
            // Second group organized by "Carried To" first and then by 
            // "Carried From".
      
            // I need to process the map and get the number of distinct reuseOn 
            // sources and the number of distinct carriedBy info.
            
            UI64DoublePMap dTo, dFrom;  // distinct To and From values
            UI64DoublePMap::iterator uit;
            Pair64DoublePMap::iterator dpit = carryMap->begin ();
            for ( ; dpit!=carryMap->end() ; ++dpit)
            {
               uit = dTo.find (dpit->first.first);
               if (uit==dTo.end())
               {
                  uit = dTo.insert (dTo.begin(), 
                        UI64DoublePMap::value_type (dpit->first.first,
                        new double[numLevels]));
                  for (i=0 ; i<numLevels ; ++i)
                     uit->second[i] = dpit->second[i];
               } else
                  for (i=0 ; i<numLevels ; ++i)
                     uit->second[i] += dpit->second[i];
               
               uit = dFrom.find (dpit->first.second);
               if (uit==dFrom.end())
               {
                  uit = dFrom.insert (dFrom.begin(), 
                        UI64DoublePMap::value_type (dpit->first.second,
                        new double[numLevels]));
                  for (i=0 ; i<numLevels ; ++i)
                     uit->second[i] = dpit->second[i];
               } else
                  for (i=0 ; i<numLevels ; ++i)
                     uit->second[i] += dpit->second[i];
            }
            
            double *cMisses = pscope->CarriedMisses();
            fprintf (_out_fd, "%*s<UG n=\"@Loop Carried To\">\n", indent, "");
            if (cMisses)
               for (i=0 ; i<numLevels ; ++i)
                  if (mem_level_has_carried_info && mem_level_has_carried_info[i] &&
                           cMisses[i]>0)
                     fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                          carriedMissesBase+i, cMisses[i]);
            dumpOneCarryMap (_out_fd, carriedMissesBase, dTo, carryMap, 
                  indent+2, numLevels, true);
            fprintf (_out_fd, "%*s</UG>\n", indent, "");
      
            fprintf (_out_fd, "%*s<UG n=\"@Loop Carried From\">\n", indent, "");
            if (cMisses)
               for (i=0 ; i<numLevels ; ++i)
                  if (mem_level_has_carried_info && mem_level_has_carried_info[i] &&
                            cMisses[i]>0)
                     fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                          carriedMissesBase+i, cMisses[i]);
            dumpOneCarryMap (_out_fd, carriedMissesBase, dFrom, carryMap, 
                  indent+2, numLevels, false);
            fprintf (_out_fd, "%*s</UG>\n", indent, "");
            
            // deallocate dTo and dFrom
            for (uit=dTo.begin() ; uit!=dTo.end() ; ++uit)
               delete[] (uit->second);
            dTo.clear ();
            for (uit=dFrom.begin() ; uit!=dFrom.end() ; ++uit)
               delete[] (uit->second);
            dFrom.clear ();
         }
         
         carryMap = &(pscope->getLICarriedForScopeMap());
         if (!carryMap->empty()) // && pscope->getScopeType()==LOOP_SCOPE)
         {
            // if we have any carried miss information, I will create two groups 
            // representing carried info organised in two different ways. 
            // Once with root "Carried From" under which we have scopes and under 
            // scopes we have "Carried To" and another level of scopes.
            // Second group organized by "Carried To" first and then by 
            // "Carried From".
      
            // I need to process the map and get the number of distinct reuseOn 
            // sources and the number of distinct carriedBy info.
            
            UI64DoublePMap dTo, dFrom;  // distinct To and From values
            UI64DoublePMap::iterator uit;
            Pair64DoublePMap::iterator dpit = carryMap->begin ();
            for ( ; dpit!=carryMap->end() ; ++dpit)
            {
               uit = dTo.find (dpit->first.first);
               if (uit==dTo.end())
               {
                  uit = dTo.insert (dTo.begin(), 
                        UI64DoublePMap::value_type (dpit->first.first,
                        new double[numLevels]));
                  for (i=0 ; i<numLevels ; ++i)
                     uit->second[i] = dpit->second[i];
               } else
                  for (i=0 ; i<numLevels ; ++i)
                     uit->second[i] += dpit->second[i];
               
               uit = dFrom.find (dpit->first.second);
               if (uit==dFrom.end())
               {
                  uit = dFrom.insert (dFrom.begin(), 
                        UI64DoublePMap::value_type (dpit->first.second,
                        new double[numLevels]));
                  for (i=0 ; i<numLevels ; ++i)
                     uit->second[i] = dpit->second[i];
               } else
                  for (i=0 ; i<numLevels ; ++i)
                     uit->second[i] += dpit->second[i];
            }
            
            double *cMisses = pscope->LICarriedMisses();
            fprintf (_out_fd, "%*s<UG n=\"@LI Carried To\">\n", indent, "");
            if (cMisses)
               for (i=0 ; i<numLevels ; ++i)
                  if (mem_level_has_carried_info && mem_level_has_carried_info[i] &&
                            cMisses[i]>0)
                     fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                             carriedMissesBase+i, cMisses[i]);
            dumpOneCarryMap (_out_fd, carriedMissesBase, dTo, carryMap, 
                  indent+2, numLevels, true);
            fprintf (_out_fd, "%*s</UG>\n", indent, "");
      
            fprintf (_out_fd, "%*s<UG n=\"@LI Carried From\">\n", indent, "");
            if (cMisses)
               for (i=0 ; i<numLevels ; ++i)
                  if (mem_level_has_carried_info && mem_level_has_carried_info[i] &&
                            cMisses[i]>0)
                     fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                             carriedMissesBase+i, cMisses[i]);
            dumpOneCarryMap (_out_fd, carriedMissesBase, dFrom, carryMap, 
                  indent+2, numLevels, false);
            fprintf (_out_fd, "%*s</UG>\n", indent, "");
            
            // deallocate dTo and dFrom
            for (uit=dTo.begin() ; uit!=dTo.end() ; ++uit)
               delete[] (uit->second);
            dTo.clear ();
            for (uit=dFrom.begin() ; uit!=dFrom.end() ; ++uit)
               delete[] (uit->second);
            dFrom.clear ();
         }

         if (!incNames->empty ())
         {
            // if we have miss information per name, I will create a groups Names
            // with the data for each name underneath.

            int numLevels2 = numLevels + numLevels;
            fprintf (_out_fd, "%*s<UG n=\"@Data Names\">\n", indent, "");
            CharDoublePMap::iterator cit = incNames->begin();
            indent += 2;
            for ( ; cit!=incNames->end() ; ++cit)
            {
               TimeAccount tempTA;
               bool found = false;
               for (int lev=0 ; lev<numLevels ; ++lev)
                  if (cit->second[lev] > 0.01)
                  {
                     tempTA.addTotalMissCountLevel (lev, cit->second[lev]);
                     found = true;
                     
                     if (cit->second[lev+numLevels] > 0.01)
                        tempTA.addIrregMissCountLevel (lev, cit->second[lev+numLevels]);
                     
                     if (cit->second[lev+numLevels2] > 0.01)
                        tempTA.addFragMissCountLevel (lev, cit->second[lev+numLevels2]);
                  }
               if (found)
               {
                  fprintf (_out_fd, "%*s<UG n=\"%s\">\n", indent, "", 
                          xml::EscapeStr(cit->first).c_str() );
                  UiToDoubleMap::const_iterator inctait;
                  const UiToDoubleMap& tempData = tempTA.getDataConst();
                  inctait = tempData.begin ();
                  for ( ; inctait!=tempData.end(); ++inctait)
                  {
                     if (TAdisplayValueForKey(inctait->first, mo->DetailedMetrics()))
                     {
                        MIAMIU::UiUiMap::iterator uit = TAshortNameMap.find (inctait->first);
                        assert (uit != TAshortNameMap.end());
                        fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                              uit->second, 
                              tempTA.getDisplayValue (inctait->first, tmach));
                     }
                  }
                  
                  fprintf (_out_fd, "%*s</UG>\n", indent, "");
               }
            }
            indent -= 2;
            fprintf (_out_fd, "%*s</UG>\n", indent, "");
         }
      }
      
      // now print profile data for any paths in this scope
      BPMap* bpm = pscope->Paths();
      if (bpm != NULL)
      {
         BPMap::iterator bit = bpm->begin();
         for ( ; bit!=bpm->end() ; ++bit)
         {
            int pathIdx = bit->second->pathId.PathIndex();
            fprintf (_out_fd, "%*s<PH n=\"Path %d%s\" v=\"%" PRIu64 "\">\n", indent, "", 
                         pathIdx, (bit->first->isExitPath()?" (x)":""),
                         bit->second->count);
            

            const UiToDoubleMap& timeStatsMap = bit->second->timeStats.getDataConst();
            inctait = timeStatsMap.begin ();
            for ( ; inctait!=timeStatsMap.end(); ++inctait)
            {
               if (TAdisplayValueForKey(inctait->first, mo->DetailedMetrics()))
               {
                  MIAMIU::UiUiMap::iterator uit = TAshortNameMap.find (inctait->first);
                  assert (uit != TAshortNameMap.end());
                  fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                        uit->second, 
                        bit->second->timeStats.getDisplayValue (inctait->first, tmach));
               }
            }
            
            if (perform_memory_analysis)
            {
               // do not print memory parallelism yet
               /*
               float serialMemLat = bit->second->serialMemLat;
               float exposedMemLat = bit->second->exposedMemLat;
               if (exposedMemLat > 0.1f)
                  fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%g\"/>\n", indent, "", 
                          memParMapping, serialMemLat/exposedMemLat);
               */
            }
            
            fprintf (_out_fd, "%*s</PH>\n", indent, "");
         }
      }
      
      // for a top-down profile, call recursive first and then print the
      // scope XML footer
      // for a flat profile, if this is not the top scope, print the footer
      // first and then call recursive
      if (flat_data && !top_scope && !is_file && !incStatsMap->empty()) // print the footer (group type)
      {
         indent -= 2;
   #if WITH_FLAT_SCOPE_HEADERS
         if (pscope->getScopeType()==LOOP_SCOPE)
            fprintf (_out_fd, "%*s</A>\n", indent, "");
         else
            fprintf (_out_fd, "%*s%s\n", indent, "", 
                     pscope->XMLScopeFooter().c_str());
   #else
         fprintf (_out_fd, "%*s</G>\n", indent, "");
   #endif
      }
   } else  /* if dump_scope */
   {
#if VERBOSE_DEBUG_OUTPUT
      DEBUG_OUT (3,
         fprintf(stderr, "Not outputting data for scope %s in flat_mode=%d\n",
               pscope->ToString().c_str(), (flat_data?1:0));
      )
#endif
   }
   
   ScopeImplementation::iterator it;
   if (dump_scope || flat_data)
   {
      for( it=pscope->begin() ; it!=pscope->end() ; it++ )
      {
         ScopeImplementation *sci = 
                   dynamic_cast<ScopeImplementation*>(it->second);
         xml_dump_for_scope (_out_fd, sci, TAshortNameMap, memParMapping, 
                     carriedMissesBase, indent, numLevels, threshValues,
                     flat_data, false);
      }
   }
   
   if (dump_scope && (!flat_data || top_scope || is_file))
   {
      indent -= 2;
      fprintf (_out_fd, "%*s%s\n", indent, "", 
                pscope->XMLScopeFooter().c_str());
   }
}

void
MIAMI_Driver::dump_line_mapping_for_scope(FILE *_out_fd, ScopeImplementation *pscope, int indent)
{
   const MIAMIU::Ui2SetMap& scopeLines = pscope->GetLineMappings();
   if (scopeLines.empty())
      return;
   
   indent += 2;
   fprintf (_out_fd, "%*s<UG n=\"@FullLineMapping\">\n", indent, "");
   indent += 2;
   MIAMIU::Ui2SetMap::const_iterator uit = scopeLines.begin();
   for ( ; uit!=scopeLines.end() ; ++uit)
   {
      const MIAMIU::UISet& localLines = uit->second;
      const MIAMIU::UISet& allLines = GetLinesForFile(uit->first);
      MIAMIU::UISet::const_iterator lit = localLines.begin();

      while (lit != localLines.end())
      {
         unsigned int sLine = *lit;
         MIAMIU::UISet::const_iterator ait = allLines.find(sLine);
         if (ait == allLines.end())
         {
            fprintf(stderr, "ERROR: File index %d, found line %d in local map, but I cannot find it in the global map for this file. How can this be??\n",
                 uit->first, sLine);
            assert (ait != allLines.end());
         }
         unsigned int eLine = sLine;
         ++ lit;
         ++ ait;
         
         for ( ; lit!=localLines.end() && ait!=allLines.end() && *lit==*ait ; 
                 ++lit, ++ait )
            eLine = *lit;
         // I cannot only exit if lines are different, or if I reach end of local map
         // I cannot reach end of global map first
         assert (lit==localLines.end() || ait!=allLines.end());
         
         // I have a range of lines, sLine - eLine; Add an Alien scope
         const std::string *fName = fileNames[uit->first];
         std::string result ("<A");
         result = result + " f=\"" + xml::EscapeStr (fName->c_str()) + "\"";

         const char* fname = fName->c_str();
         size_t lastSlash = fName->rfind("/");
         if (lastSlash != std::string::npos)  // we found a '/'
            fname = fname + lastSlash + 1;
         
         std::ostringstream oss;
         oss << result << " n=\"" << xml::EscapeStr (fname) 
             << " [" << sLine << "-" << eLine << "]\" b=\"" 
             << sLine << "\" e=\"" << eLine << "\">";

         fprintf (_out_fd, "%*s%s\n", indent, "", oss.str().c_str());
         fprintf (_out_fd, "%*s</A>\n", indent, "");
      }
   }
   indent -= 2;
   fprintf (_out_fd, "%*s</UG>\n", indent, "");
}

void
MIAMI_Driver::dumpOneReuseMap (FILE *_out_fd, MIAMIU::UiUiMap &TAshortNameMap, 
         UI64DoublePMap &singleMap, Trio64DoublePMap *pairMap, int indent, 
         int numLevels, double incCpuTime, bool firstIdx, bool isEncodedId)
{
   int i;
   Machine *tmach = 0;
   if (!targets.empty())
      tmach = targets.front();
   int numLevels2 = numLevels + numLevels;
   
   UiToDoubleMap::const_iterator inctait;
   UI64DoublePMap::iterator dpit = singleMap.begin ();
   for ( ; dpit!=singleMap.end() ; ++dpit)
   {
      uint64_t fullScopeId = dpit->first;
      int32_t imgId = GET_PATTERN_IMG(fullScopeId);
      int32_t scopeId = GET_PATTERN_INDEX(fullScopeId);
      int loopCarried = 0;
      if (isEncodedId) {
         loopCarried = scopeId & 0x1;
         scopeId = scopeId >> 1;
      }
      
      TimeAccount tempTA;
      bool found = false;
      for (int lev=0 ; lev<numLevels ; ++lev)
         if (dpit->second[lev] > 0.01)
         {
            // do not compute the bandwidth and all the other derived metrics
            // tempTA.addReuseMissCountLevel (lev, dpit->second[lev], tmach);
            tempTA.addTotalMissCountLevel (lev, dpit->second[lev]);
            found = true;
            
            if (dpit->second[lev+numLevels] > 0.01)
               tempTA.addIrregMissCountLevel (lev, dpit->second[lev+numLevels]);
            
            if (dpit->second[lev+numLevels2] > 0.01)
               tempTA.addFragMissCountLevel (lev, dpit->second[lev+numLevels2]);
         }
      // I will use the scope XML to present the reuse info
      if (found)   // print the headers for this group
      {
         ScopeImplementation *pscope = NULL;
         // tempTA.addReuseSchedulingTime (incCpuTime);
         if (scopeId == 0)
         {
            fprintf (_out_fd, "%*s<UG n=\"Cold misses\">\n", indent, "");
         } else
         {
            // Find which is the scope on which we have reuse
            LoadModule *scopeImg = GetModuleWithIndex(imgId);
            assert (scopeImg || !"We do not have a scope image??");
            
            pscope = scopeImg->GetSIForScope(scopeId);
            if (! pscope)
            {
               // print the error message only for valid Scope IDs. ID 0 is special.
               if (scopeId)
                  fprintf(stderr, "ERROR: MIAMI_Driver::dumpOneReuseMap: Couldn't find a ScopeImplementation for scopeId %d, in image %d (%s)\n",
                       scopeId, imgId, scopeImg->Name().c_str());
               // create a ScopeNotFound scope to use when we cannot find one
               pscope = scopeImg->GetDefaultScope();
            }
            // I should always find something
            assert (pscope || !"I still do not have a valid scope object??");

            std::string namePrefix = "";
            if (loopCarried)
               namePrefix = "LC_";

#if USE_ALIEN_SCOPES
            fprintf (_out_fd, "%*s%s\n", indent, "", 
                    pscope->AlienXMLScopeHeader(namePrefix.c_str()).c_str());
#else
            fprintf (_out_fd, "%*s<UG n=\"%s%s\">\n", indent, "", 
                    namePrefix.c_str(),
                    xml::EscapeStr(pscope->ToString()).c_str());
#endif
         }
   
         const UiToDoubleMap& tempData = tempTA.getDataConst();
         inctait = tempData.begin();
         for ( ; inctait!=tempData.end(); ++inctait)
         {
            if (TAdisplayValueForKey(inctait->first, mo->DetailedMetrics()))
            {
               MIAMIU::UiUiMap::iterator uit = TAshortNameMap.find(inctait->first);
               assert (uit != TAshortNameMap.end());
               fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                     uit->second, 
                     tempTA.getDisplayValue(inctait->first, tmach));
            }
         }
         
         if (scopeId == 0)
            // we do not use alien header tag for cold misses
            fprintf (_out_fd, "%*s</UG>\n", indent, "");
         else
         {
            if (pairMap)
            {
               int numLevels3 = numLevels + numLevels2;
               // Now I have to call it recursively for the second level
               UI64DoublePMap localMap;
               UI64DoublePMap::iterator uit;
               Trio64DoublePMap::iterator pait = pairMap->begin ();
               for ( ; pait!=pairMap->end() ; ++pait)
               {
                  uint64_t newId = 0;
                  if (firstIdx && pait->first.first==fullScopeId)
                     newId = pait->first.second;
                  else
                     if (!firstIdx && pait->first.second==fullScopeId)
                        newId = pait->first.first;
                  if (newId)  // not for cold misses
                  {
                     // I need to actually allocate memory for this one
                     // because I aggregate over multiple sets
                     uit = localMap.find (newId);
                     double fragFactor = pait->second[numLevels+1];
                     bool is_irreg = (pait->second[numLevels] > 0.0);
                     bool is_frag = (fragFactor > 0.0);
                     if (uit==localMap.end())
                     {
                        uit = localMap.insert (localMap.begin(), 
                             UI64DoublePMap::value_type (newId,
                                     new double[numLevels3]));
                        for (i=0 ; i<numLevels ; ++i)
                        {
                           uit->second[i] = pait->second[i];
                           if (is_irreg)
                              uit->second[i+numLevels] = pait->second[i];
                           else
                              uit->second[i+numLevels] = 0.0;
                           if (is_frag && pait->second[i]>0.0)
                              uit->second[i+numLevels2] = pait->second[i]*fragFactor;
                           else
                              uit->second[i+numLevels2] = 0.0;
                        }
                     } else
                        for (i=0 ; i<numLevels ; ++i)
                        {
                           uit->second[i] += pait->second[i];
                           if (is_irreg)
                              uit->second[i+numLevels] += pait->second[i];
                           if (is_frag && pait->second[i]>0.0)
                              uit->second[i+numLevels2] += pait->second[i]*fragFactor;
                        }
                  }
               }
               if (firstIdx)
                  fprintf (_out_fd, "%*s  <UG n=\"@Reuse carrier\">\n", indent, "");
               else
                  fprintf (_out_fd, "%*s  <UG n=\"@Reuse source\">\n", indent, "");

               dumpOneReuseMap (_out_fd, TAshortNameMap, localMap, NULL, 
                    indent+4, numLevels, incCpuTime, false, !isEncodedId);
               fprintf (_out_fd, "%*s  </UG>\n", indent, "");
               
               // deallocate localMap 
               for (uit=localMap.begin() ; uit!=localMap.end() ; ++uit)
                  delete[] (uit->second);
               localMap.clear ();
            }
#if USE_ALIEN_SCOPES
            fprintf (_out_fd, "%*s</A>\n", indent, "");
#else
            fprintf (_out_fd, "%*s</UG>\n", indent, "");
#endif
#if 0
            fprintf (_out_fd, "%*s%s\n", indent, "", 
                 pscope->XMLScopeFooter().c_str());
#endif
         }
      }
      tempTA.getData().clear ();
   }
}


void
MIAMI_Driver::dumpOneCarryMap (FILE *_out_fd, int carryBaseIndex, 
         UI64DoublePMap &singleMap, Pair64DoublePMap *pairMap, int indent, 
         int numLevels, bool firstIdx)
{
   int i;
   
   UI64DoublePMap::iterator dpit = singleMap.begin();
   for ( ; dpit!=singleMap.end() ; ++dpit)
   {
      uint64_t fullScopeId = dpit->first;
      assert (fullScopeId);

      int32_t imgId = GET_PATTERN_IMG(fullScopeId);
      int32_t scopeId = GET_PATTERN_INDEX(fullScopeId);
      
      LoadModule *img = GetModuleWithIndex(imgId);
      assert (img || !"We do not have a scope image??");
      
      // Find which is the scope at first end of the carried reuse
      // It is possible that I do not find a scope always (not sure why, but 
      // it happens with some obscure system routines, etc). Print a warning
      ScopeImplementation *pscope = img->GetSIForScope(scopeId);
      if (! pscope)
      {
         // print the error message only for valid Scope IDs. ID 0 is special.
         if (scopeId)
            fprintf(stderr, "ERROR: MIAMI_Driver::dumpOneCarryMap: Couldn't find a ScopeImplementation for scopeId %d, in image %d (%s)\n",
                 scopeId, imgId, img->Name().c_str());
         // create a ScopeNotFound scope to use when we cannot find one
         pscope = img->GetDefaultScope();
      }
      assert (pscope || !"I still do not have a valid scope object??");

#if USE_ALIEN_SCOPES
      fprintf (_out_fd, "%*s%s\n", indent, "", 
                  pscope->AlienXMLScopeHeader().c_str());
#else
      fprintf (_out_fd, "%*s<UG n=\"%s\">\n", indent, "", 
                  xml::EscapeStr(pscope->ToString()).c_str() );
//                  (const char*)(pscope->XMLScopeHeader ()));
#endif

      for (i=0 ; i<numLevels ; ++i)
      {
         if (mem_level_has_carried_info && mem_level_has_carried_info[i] &&
                   dpit->second[i]>0.0001)
            fprintf (_out_fd, "%*s  <M n=\"%d\" v=\"%lg\"/>\n", indent, "", 
                  carryBaseIndex+i, dpit->second[i]);
      }
         
      if (pairMap)
      {
         // Now I have to call it recursively for the second level
         UI64DoublePMap localMap;
         Pair64DoublePMap::iterator pait = pairMap->begin ();
         for ( ; pait!=pairMap->end() ; ++pait)
         {
            uint64_t newId = 0;
            if (firstIdx && pait->first.first==fullScopeId)
               newId = pait->first.second;
            else
               if (!firstIdx && pait->first.second==fullScopeId)
                  newId = pait->first.first;
            if (newId)  // not for cold misses
               localMap.insert (UI64DoublePMap::value_type (newId,
                          pait->second));
         }
         if (firstIdx)
            fprintf (_out_fd, "%*s  <UG n=\"@Carried from\">\n", indent, "");
         else
            fprintf (_out_fd, "%*s  <UG n=\"@Carried to\">\n", indent, "");

         dumpOneCarryMap (_out_fd, carryBaseIndex, localMap, NULL, 
                 indent+4, numLevels, false);
         fprintf (_out_fd, "%*s  </UG>\n", indent, "");
               
         // no need to deallocate localMap since it uses pointers to
         // data in pairMap
         localMap.clear ();
      }
#if USE_ALIEN_SCOPES
      fprintf (_out_fd, "%*s</A>\n", indent, "");
#else
      fprintf (_out_fd, "%*s</UG>\n", indent, "");
#endif
#if 0
      fprintf (_out_fd, "%*s%s\n", indent, "", 
           pscope->XMLScopeFooter().c_str());
#endif
   }
}

}  /* namespace MIAMI */
