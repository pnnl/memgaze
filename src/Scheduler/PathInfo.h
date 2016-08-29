/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: PathInfo.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data structure that holds performance data associated with a path through
 * a loop.
 */

#ifndef _PATH_INFO_H
#define _PATH_INFO_H

#include <map>
#include <list>

#include "uipair.h"
#include "TimeAccount.h"
#include "reuse_group.h"
#include "Clique.h"
#include "reg_sched_info.h"
#include "hashmaps.h"
#include "miami_types.h"
#include "path_id.h"

namespace MIAMI
{

typedef std::map <unsigned int, RGList> Set2RGLMap;
typedef float LatencyType;

class MemLevelData
{
public:
   MemLevelData (int _index);
   ~MemLevelData ();
   
   void addRGToSet (ReuseGroup* _rg, unsigned int _setIdx);
   
   unsigned int levelIndex;   // store the index of the level so I can
                              // obtain informations about it from the machine
   Set2RGLMap set2RGs;        // keeps the list of sets that span this path
                              // and the list of RGs that map to each set
   MIAMIU::UiDoubleMap set2MissCount; // keeps the miss counts corresponding to this
                              // path for each set that spans it
   double totalMissCount;     // total miss count for this path
};

typedef std::list <MemLevelData> MLDList;


class PathInfo
{
public:
  PathInfo (uint64_t i1 = 0);
//  PathInfo (const PathInfo& uip);
  ~PathInfo();
  
  void trimFat ();
  
//  PathInfo& operator= (const PathInfo& uip);

  uint64_t count;
  LatencyType latency;
  PathID pathId;
  TimeAccount timeStats;
  TimeAccount unitUsage;
  RGList reuseGroups;
  CliqueList cliques;
  HashMapUI dist2use;
  float serialMemLat;
  float exposedMemLat;
  
  int32_t num_uops;
  // miss info for each memory level
  MLDList memData;
};

}  /* namespace MIAMI */

#endif
