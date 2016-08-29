/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: MemoryHierarchyLevel.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Stores information about a machine memory hierarchy level.
 */

#ifndef _MEMORY_HIERARCHY_LEVEL_H
#define _MEMORY_HIERARCHY_LEVEL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>

#include "position.h"
#include "uipair.h"
#include "StringAssocTable.h"
#include "hashmaps.h"
#include "bucket_hashmap.h"

namespace MIAMI
{

#define MHL_UNLIMITED          -1
#define MHL_UNDEFINED          -1
#define MHL_FULLY_ASSOCIATIVE  -1

typedef std::map <unsigned long long, double> UllDoubleMap;

#if XXX
class SetInfo
{
public:
   SetInfo (int _setIndex, unsigned int _reuseOn, unsigned int _carrier,
           double _val1, double _val2);
   ~SetInfo ();
   
   double First () const               { return (value1); }
   double Second () const              { return (value2); }
   
   int SetIndex () const               { return (setIndex); }
   unsigned int ReuseOn () const       { return (reuseOn); }
   unsigned int Carrier () const       { return (carriedBy); }
   bool isLoopIndependentReuse() const { return (loopIndep); }
   bool isCarriedReuse() const         { return (!loopIndep); }
//   int NumberOfLabels () const         { return (numLabels); }
//   const unsigned int *Labels () const { return (labels); }
   
   void addFreqForPath (double _freq, unsigned long long _pathId);
   double getFrequencyRatioForPath (unsigned long long _pathId);
   double getMissCountForRatio (double ratio);

private:
   double value1;
   double value2;
   
   int setIndex;
   unsigned int reuseOn;
   unsigned int carriedBy;
   bool loopIndep;
   
   double totalExecCount;
   UllDoubleMap pathsFreq;
};

//typedef std::vector <SetInfo*> SIArray;
// I will need to use a map instead
typedef std::map <MIAMIU::UITrio, SetInfo*, MIAMIU::UITrio::OrderTrio> TrioSIMap;

// I need a map to hold footprint information for each scope
// map from scopeId (unsigned int) to a tuple of one unsigned long long = 
// how many times we entered that scope, and four float values =
// (average footprint, standard deviation, min and max footprints)
class FootPrintInfo
{
public:
   FootPrintInfo (float _count=0, float _first=0, 
          float _second=0, float _third=0, float _fourth=0) : 
             totCount(_count), avgFp(_first), sdFp(_second), 
             minFp(_third), maxFp(_fourth)
   { }
      
   float totCount;
   float avgFp;
   float sdFp;
   float minFp;
   float maxFp;
};
typedef std::map <addrtype, FootPrintInfo*> ScopesFootPrint;
#endif

class MemoryHierarchyLevel
{
   static float defProbValue;
   typedef MIAMIU::BucketHashMap<uint32_t, float, &defProbValue, 64> ProbabilityTable;
   
public:
   MemoryHierarchyLevel();
   ~MemoryHierarchyLevel();

   MemoryHierarchyLevel(int _num_blocks, int _block_size, int _entry_size, 
           int _assoc, float _bdwth, char* _longname, char *_next_level, 
           int _missPenalty);
   MemoryHierarchyLevel(const MemoryHierarchyLevel& mhl);

   MemoryHierarchyLevel& operator= (const MemoryHierarchyLevel& mhl);

   void setNameAndPosition(const char* _longname, const Position& _pos);

   inline int getNumBlocks() const            { return (num_blocks); }
   inline int getBlockSize() const            { return (block_size); }
   inline int getEntrySize() const            { return (entry_size); }
   inline int getAssocLevel() const           { return (assoc); }
   inline float getBandwidth() const          { return (bdwth); }
   inline int getMissPenalty() const          { return (missPenalty); }
   inline const char* getLongName() const     { return (longname); }
   inline const char* getNextLevel() const    { return (next_level); }
   inline const Position& getPosition() const { return (position); }
   
   inline void setMrdFileIndex(int idx)        { mrd_idx = idx; }
   inline int getMrdFileIndex() const          { return (mrd_idx); }
   
   inline bool HasMissCounts () const          { return (hasMissInfo); }
   
   void SetSampleMask(int _mask);
   void ComputeMissesForReuseDistance(float dist, double& fullMiss, double& setMiss);
   void DeleteProbabilityCache() { 
      if (probCache)
         delete (probCache);
      probCache = 0;
   }
   
#if XXX
   // record execution of ReuseGroup, and return the set index of that RG
   unsigned int addReuseGroupData (unsigned int _pc, unsigned long long _pathId,
         unsigned int _numRefs, unsigned long long _execFreq);
   int parseCacheMissFile (const char* fileName, MIAMIU::PairUiMap &unkValues);
   void fillMissCounts (unsigned long long pathId, MIAMIU::UiDoubleMap &missTable,
         double &totalMissCount, int numLevels, MIAMIU::TrioDoublePMap& reuseMap,
         int memLevel);
   
   // return these maps by reference, but do not allow any modifications
   // from outside
   const ScopesFootPrint& getScopeFootPrintData () const { return (scopesFP); }
   const TrioSIMap& getSetsCacheMissInfo () const        { return (setsInfo); }
#endif
   
private:   
   float computeProbability(unsigned int n /* distance */, 
          unsigned int k /* assoc */, int s /* numSets */, 
          double rs /* 1/numSets */);
   double compute_pow (double base, unsigned int expon);
          

   int num_blocks;
   int block_size;
   int entry_size;
   int assoc;
   int num_sets;
   int sample_mask;  // the sample mask used during MRD collection
   double rSets;
   float bdwth;
   char *longname;
   char *next_level;
   int missPenalty;
   Position position;
   
   // use a table to save computed probabilities for different distances, hmmm ...
   ProbabilityTable *probCache;
   
   // also use a cache of powers of (s-1)/s, where s is the # of sets
   double pow_vals[32];
   // I will keep a static array with all powers of 2 of the base
   int maxStoredExp;
   
   int mrd_idx;
   bool hasMissInfo;
/////   unsigned int numSets;
#if XXX
   HashMapUI ref2Set;

   TrioSIMap setsInfo;
   ScopesFootPrint scopesFP;
#endif
};

typedef StringAssocTable <MemoryHierarchyLevel> MemLevelAssocTable;

} /* namespace MIAMI */

#endif
