/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: scope_implementation.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Data structure to hold scheduler specific information about
 * a program scope, such as a loop, routine, file, or image.
 */

#ifndef _SCOPE_IMPLEMENTATION_H_
#define _SCOPE_IMPLEMENTATION_H_

#include "miami_types.h"
#include "code_scope.h"
#include "block_path.h"
#include "CFG.h"
#include "TimeAccount.h"

#include "uipair.h"
#include "reg_sched_info.h"
#include "charless.h"
#include "stream_reuse_histograms.h"
#include "generic_pair.h"
#include "generic_trio.h"
//#include "routine.h"



namespace MIAMI
{

typedef MIAMIU::GenericPair<uint64_t, uint64_t> Pair64;
typedef MIAMIU::GenericTrio<uint64_t> Trio64;
typedef std::map <Pair64, double*, Pair64::OrderPairs> Pair64DoublePMap;
typedef std::map <Trio64, double*, Trio64::OrderTrio> Trio64DoublePMap;

typedef std::map <MIAMIU::UIPair, RSIList, MIAMIU::UIPair::OrderPairs> PairRSIMap;
typedef std::map <const char*, double*, CharLess> CharDoublePMap;

class ScopeImplementation : public CodeScope
{
public:
   IS_CONCRETE_CLASS;
   
   ScopeImplementation(ScopeImplementation *_parent, ScopeType _stype, 
              CodeScopeKey _key) : 
         CodeScope(_parent, _stype, _key)
   {
      latency = 0;
      num_uops = 0;
//      unusedCycles = 0;
      paths = NULL;
      num_loads = 0; num_stores = 0;
      exit_count = 0;
      total_count = 0;
      serialMemLat = 0;
      exposedMemLat = 0;
      exclusiveSerialMemLat = 0;
      exclusiveExposedMemLat = 0;
      carriedMisses = 0;
      LIcarriedMisses = 0;
      scopeId = 0;
      instCounts = 0;
#if 0
      if (_stype==LOOP_SCOPE || _stype==ROUTINE_SCOPE)
         loop_node = new CFG::Node(NULL, _key, 0, CFG::MIAMI_INNER_LOOP);
      else
         loop_node = 0;
#endif
   }

   virtual ~ScopeImplementation()
   {
      if (paths)
      {
         for(BPMap::iterator bit=paths->begin() ; bit!=paths->end() ; bit++)
         {
            delete (bit->first);
            delete (bit->second);
         }
         delete paths;
      }
//      if (loop_node)
//         delete (loop_node);
      CFG::NodeNodeMap::iterator nit = loopNodes.begin();
      for ( ; nit!=loopNodes.end() ; ++nit)
         delete (nit->second);
      
      Trio64DoublePMap::iterator tit = reuseFromScope.begin ();
      for ( ; tit!=reuseFromScope.end() ; ++tit)
         delete[] (tit->second);
      reuseFromScope.clear ();
      Pair64DoublePMap::iterator dit = carriedForScope.begin ();
      for ( ; dit!=carriedForScope.end() ; ++dit)
         delete[] (dit->second);
      carriedForScope.clear ();
      
      entries.clear ();
      exits.clear ();
      pathRegs.clear ();
      
      if (instCounts)
         delete (instCounts);
      
      if (carriedMisses)
         delete[] (carriedMisses);
         
      CharDoublePMap::iterator cit = inclusiveNameVals.begin();
      for ( ; cit!=inclusiveNameVals.end() ; ++cit)
         delete[] (cit->second);
      inclusiveNameVals.clear();
      for (cit=exclusiveNameVals.begin() ; cit!=exclusiveNameVals.end() ; ++cit)
         delete[] (cit->second);
      exclusiveNameVals.clear();
   }
   
   inline LatencyType& Latency() { return (latency); }
   inline uint64_t& NumUops() { return (num_uops); }
//   inline LatencyType& Inefficiency() { return (unusedCycles); }

   inline BPMap*& Paths()       { return (paths); }
   inline BMSet& InnerEntries() { return (entries); }
   inline BMSet& InnerExits()   { return (exits); }
   inline PairRSIMap& PathRegisters() { return (pathRegs); }

   inline float& NumLoads()     { return (num_loads); }
   inline float& NumStores()    { return (num_stores); }

   inline void addExitCount (unsigned long long _cnt) { exit_count += _cnt; }
   inline float getExitCount ()                       { return (exit_count); }
   inline void addExecCount (unsigned long long _cnt) { total_count += _cnt; }
   inline unsigned long long getTotalCount ()         { return (total_count); }
   
   inline TimeAccount& getInclusiveTimeStats ()  { return (statsInc); }
   inline TimeAccount& getExclusiveTimeStats ()  { return (statsExc); }
   inline TimeAccount* getExclusiveTimeStatsAddress ()  { return (&statsExc); }
   inline Trio64DoublePMap& getReuseFromScopeMap ()  { return (reuseFromScope); }
   inline Pair64DoublePMap& getCarriedForScopeMap () { return (carriedForScope); }
   inline Pair64DoublePMap& getLICarriedForScopeMap () { return (LIcarriedForScope); }
   
   inline MIAMIU::UiDoubleMap& getFragFactorsForScope ()   { return (fragFactors); }

   inline CharDoublePMap& getInclusiveStatsPerName (){ return (inclusiveNameVals); }
   inline CharDoublePMap& getExclusiveStatsPerName (){ return (exclusiveNameVals); }

   inline void addInnerSerialMemLat(float _cnt)  { serialMemLat += _cnt; }
   inline float getSerialMemLat() const          { return (serialMemLat); }
   inline void addInnerExposedMemLat(float _cnt) { exposedMemLat += _cnt; }
   inline float getExposedMemLat() const         { return (exposedMemLat); }

   inline void addExclusiveSerialMemLat (float _cnt) 
   { 
      exclusiveSerialMemLat += _cnt;
      serialMemLat += _cnt; 
   }
   inline void addExclusiveExposedMemLat (float _cnt) 
   { 
      exclusiveExposedMemLat += _cnt; 
      exposedMemLat += _cnt; 
   }
   inline float getExclusiveSerialMemLat ()  { return (exclusiveSerialMemLat); }
   inline float getExclusiveExposedMemLat () { return (exclusiveExposedMemLat); }
   
   inline double* &CarriedMisses ()      { return (carriedMisses); }
   inline double* &LICarriedMisses ()    { return (LIcarriedMisses); }
   
   inline void setScopeId (unsigned int _scopeId) { scopeId = _scopeId; }
   inline unsigned int getScopeId () const        { return (scopeId); }
   
   inline MIAMIU::PairDoubleMap* &getInstCountsMap ()     { return (instCounts); }
   void trimFat ()  
   { 
      if (instCounts) 
         delete (instCounts); 
      instCounts = NULL;
   }
   
   inline void setMarkedWith(unsigned int _marker) { marker = _marker; }
   inline unsigned int getMarkedWith() { return (marker); }
   
   inline StreamRD::ScopeStreamInfo& getScopeStreamInfo() { return (streamInfo); }
//   inline CFG::Node* LoopNode() const  { return (loop_node); }
   inline ICIMap& getInstructionMixInfo()  { return (imixInfo); }
   
   void AddCarriedMisses(int iter, Pair64& tempP64, int level, int numLevels,
                        double missCount)  //, double fragF, bool is_irreg)
   {
      double **pCarried;
      Pair64DoublePMap *pCarriedMap;
      int j;
      
      if (iter)  // it is loop carried
      {
         pCarried = &carriedMisses;
         pCarriedMap = &carriedForScope;
      } else   // it is loop independent
      {
         pCarried = &LIcarriedMisses;
         pCarriedMap = &LIcarriedForScope;
      }

      if (! (*pCarried))
      {
         *pCarried = new double[numLevels];
         for (j=0 ; j<numLevels ; ++j)
            (*pCarried)[j] = 0.0;
      }
      (*pCarried)[level] += missCount;

      Pair64DoublePMap::iterator pdit = pCarriedMap->find (tempP64);
      double *vals = 0;
      if (pdit == pCarriedMap->end ())
      {
         vals = new double [numLevels];
         for (j=0 ; j<numLevels ; ++j)
            vals[j] = 0.0;
         pCarriedMap->insert (Pair64DoublePMap::value_type (tempP64, vals));
      } else
         vals = pdit->second;
      vals[level] += missCount;
   }

   const BMarker* HasEntry(CFG::Node *bb) const
   {
      BMarker temp(bb, bb, marker, 0);
      BMSet::const_iterator eit = entries.find(temp);
      if (eit != entries.end())
         return (&(*eit));
      else
         return (0);
   }
   
   // return the loop node asociated with an entry block
   // create a loop node if we do not find it
   CFG::Node* LoopNode(CFG::Node* key)
   {
      // I create separate loop nodes for each entry
      // I create them on demand; check if we have one
      CFG::NodeNodeMap::iterator nit = loopNodes.find(key);
      if (nit != loopNodes.end())
         return (nit->second);
      // if not found, I must create a loop node for this entry
      assert (getScopeType()==LOOP_SCOPE || getScopeType()==ROUTINE_SCOPE);
      CFG::Node* lnode = new CFG::Node(NULL, getScopeKey(), 0, CFG::MIAMI_INNER_LOOP);
      lnode->markWith(marker);
      loopNodes.insert(CFG::NodeNodeMap::value_type(key, lnode));
//      revLoopNodes.insert(CFG::NodeNodeMap::value_type(lnode, key));
      return (lnode);
   }

   // return the loop node associated with an entry block if one exists
   // return NULL otherwise
   CFG::Node* hasLoopNode(CFG::Node* key) const
   {
      CFG::NodeNodeMap::const_iterator nit = loopNodes.find(key);
      if (nit != loopNodes.end())
         return (nit->second);
      else
         return (0);
   }
   
#if 0
   CFG::Node* EntryBlockForLoopNode(CFG::Node* lnode) const 
   {
      assert(lnode->is_inner_loop());
      CFG::NodeNodeMap::const_iterator nit = revLoopNodes.find(lnode);
      if (nit != revLoopNodes.end())
         return (nit->second);
      else
         return (0);  // can this even happen??
   }
#endif
      
private:
   LatencyType latency;
   uint64_t num_uops;
//   LatencyType unusedCycles;
   BMSet entries, exits;
   PairRSIMap pathRegs;   // keeps input and output registers for inner loops
   BPMap* paths;
   // store a loop node for each scope. I use it to detect cycles through 
   // paths that include inner loops. In a few cases, I start the path on an 
   // inner loop and it would be nice to detect the cycle when I am trying
   // to reenter the loop.
//   CFG::Node *loop_node;
   CFG::NodeNodeMap loopNodes;
//   CFG::NodeNodeMap revLoopNodes; // reverse loop node - map from a loop node to the CFG entry block
   
   float num_loads;
   float num_stores;
   float exit_count;
   unsigned long long total_count;
   unsigned int marker;
   
   TimeAccount statsInc;
   TimeAccount statsExc;
   
   Trio64DoublePMap reuseFromScope;  // collect misses for all levels based on
                                     // the source of the reuse and the carrier
                                     // the third key is the set index
   Pair64DoublePMap carriedForScope; // collect misses carried by this scope
                                     // based on the source and target of the
                                     // reuse; contains only loop carried reuses
                                     // and is valid only for loops
   Pair64DoublePMap LIcarriedForScope; // collect misses carried by this scope
                                     // based on the source and target of the
                                     // reuse; contains only loop independent 
                                     // reuses
   MIAMIU::PairDoubleMap *instCounts;// collect execution frequencies for all 
                                     // combinations of instr type and bit width
   MIAMIU::UiDoubleMap fragFactors;  // stores fragmentation factor for sets
   
   CharDoublePMap inclusiveNameVals;
   CharDoublePMap exclusiveNameVals;
                                   
   float serialMemLat;
   float exposedMemLat;

   float exclusiveSerialMemLat;
   float exclusiveExposedMemLat;
   
   // hold carried misses info for each scope
   double *carriedMisses;    // loop carried misses (only for loops)
   double *LIcarriedMisses;  // loop independent carried misses 
   unsigned int scopeId;
//ozgurS
   double fp_palm; //loop footprint
   MemDataPerLvlList memDataPerLevel;
   MemListPerInst memDataPerInst;
   MLDList memData_palm;
//ozgurE
   
   // data structure to hold the stream reuse histograms
   StreamRD::ScopeStreamInfo streamInfo;
   
   // data structure to hold the instruction mix histogram
   ICIMap imixInfo;
};

} /* namespace MIAMI */

#endif
