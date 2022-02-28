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

//ozgurS
//class to keep memory data per level

class MemoryDataPerLevel{
public:
//   MemoryDataPerLevel(int i);
   MemoryDataPerLevel(int _level = -1 , int _totalHits = -1, int _totalMiss = -1  ,  float _avgWindow = -1.0){
      level = _level;
      totalHits = _totalHits;
      totalMiss = _totalMiss;
      avgWindow = _avgWindow;      
   }

   int level;
   int totalHits;
   int totalMiss;
   float avgWindow;
};

typedef std::vector <MIAMI::InstlvlMap *> MemListPerInst; 

//this vector will keep all the data in it per level
//I need to find a wat to build it 
typedef std::vector <MemoryDataPerLevel> MemDataPerLvlList;
//ozgurE

class PathInfo
{
public:
  PathInfo (uint64_t i1 = 0);
//  PathInfo (const PathInfo& uip);
  ~PathInfo();
  
  void trimFat ();
  
//  PathInfo& operator= (const PathInfo& uip);
/*
 * ozgurS adding memlatency and cpulatency
 */
  LatencyType memlatency;
  LatencyType cpulatency;
 //ozgurE 
  uint64_t count;
  LatencyType latency;  //ozgur TODO fallow this to get infor of Hits
  PathID pathId;
  TimeAccount timeStats;
  TimeAccount unitUsage;
  RGList reuseGroups;
  CliqueList cliques;
  HashMapUI dist2use;
  float serialMemLat;
  float exposedMemLat;
  
//ozgurS total hits and miss per level in an vector`
  MemDataPerLvlList memDataPerLevel;
  MemListPerInst memDataPerInst;
//ozgurE

  int32_t num_uops;
  // miss info for each memory level
  MLDList memData;
};

}  /* namespace MIAMI */

#endif
