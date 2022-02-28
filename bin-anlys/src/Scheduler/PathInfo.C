/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: PathInfo.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data structure that holds performance data associated with a path through
 * a loop.
 */

#include "PathInfo.h"

using namespace MIAMI;

MemLevelData::MemLevelData (int _index)
{
   levelIndex = _index;
}

MemLevelData::~MemLevelData ()
{
   // I do not have to deallocate the reuse groups because they are managed
   // by the container PathInfo class
   set2RGs.clear ();
   set2MissCount.clear ();
}
   
void 
MemLevelData::addRGToSet (ReuseGroup* _rg, unsigned int _setIdx)
{
   Set2RGLMap::iterator sit = set2RGs.find (_setIdx);
   if (sit == set2RGs.end())  // first RG from this set
   {
     sit = set2RGs.insert (set2RGs.begin(), 
           Set2RGLMap::value_type (_setIdx, RGList()) );
     // first time I see a new set, add also an element into the missCount map
     set2MissCount.insert (MIAMIU::UiDoubleMap::value_type (_setIdx, 0));
   }
   sit->second.push_back (_rg);
}


PathInfo::PathInfo (uint64_t i1) : count(i1)
{
   latency = 0;
//ozgurS 
   cpulatency = 0; 
   memlatency = 0; 
//ozgurE
   timeStats.getData().clear();
   unitUsage.getData().clear();
}

#if 0  
PathInfo::PathInfo (const PathInfo& uip)
{
   count = uip.count; latency = uip.latency;
   unusedCycles = uip.unusedCycles;
   timeStats = uip.timeStats;
   memcpy(ineffType, uip.ineffType, (int)(INEF_LAST)*sizeof(int) );
   if (uip.iratio == NULL)
      iratio = NULL;
   else
      iratio = new PairArray(*uip.iratio);
  reuseGroups = uip.reuseGroups;
  cliques = uip.cliques;
   serialMemLat = uip.serialMemLat;
   exposedMemLat = uip.exposedMemLat;
   dynamicInstructions = uip.dynamicInstructions;
   CliqueList::iterator clit = uip.cliques.begin ();
   for ( ; clit!=uip.cliques.end() ; ++clit)
      cliques.push_back (new Clique (*(*clit)) );
   RGList::iterator rgit = uip.reuseGroups.begin ();
   for ( ; rgit!=uip.reuseGroups.end() ; ++rgit)
      reuseGroups.push_back (new ReuseGroup (*(*rgit)) );
}
#endif
  
PathInfo::~PathInfo ()
{
   timeStats.getData().clear ();
   unitUsage.getData().clear ();
   CliqueList::iterator clit = cliques.begin ();
   for ( ; clit!=cliques.end() ; ++clit)
      delete (*clit);
   cliques.clear ();
   RGList::iterator rgit = reuseGroups.begin ();
   for ( ; rgit!=reuseGroups.end() ; ++rgit)
      delete (*rgit);
   reuseGroups.clear ();
}

void
PathInfo::trimFat ()
{
   CliqueList::iterator clit = cliques.begin ();
   for ( ; clit!=cliques.end() ; ++clit)
      delete (*clit);
   cliques.clear ();
   RGList::iterator rgit = reuseGroups.begin ();
   for ( ; rgit!=reuseGroups.end() ; ++rgit)
      delete (*rgit);
   reuseGroups.clear ();
   dist2use.clear ();
}

#if 0
PathInfo& 
PathInfo::operator= (const PathInfo& uip)
{
   count = uip.count; latency = uip.latency;
   unusedCycles = uip.unusedCycles;
   timeStats = uip.timeStats;
   return (*this);
}
#endif
  
