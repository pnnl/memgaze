/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: private_routine.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Base class for storing routine specific data.
 */

#ifndef MIAMI_PRIVATE_ROUTINE_H
#define MIAMI_PRIVATE_ROUTINE_H

#include <set>
#include <map>
#include <string>

#include <stdio.h>

#include "miami_types.h"
#include "PrivateCFG.h"
#include "private_load_module.h"
#include "tarjans/MiamiRIFG.h"

namespace MIAMI
{

// store the BBL's end address in a map, so that a search
// using any address will return the block containing the address
// if such a block exists, or the first block after that address 
// or even the map->end() iterator if there is no BBL containing 
// the address

typedef std::map<addrtype, PrivateCFG::Node*> BBMap;
typedef std::pair<addrtype, PrivateCFG::Edge*> AddrEdgePair;
typedef std::map<addrtype, AddrEdgePair> AddrEdgeMap;

typedef std::map<addrtype, uint64_t> EntryMap;

class PrivateRoutine
{
#define CREATE_NEW_BBL 0
#define IDENTICAL_BBL 15
#define ENTERS_OLD_BBL  1
#define EXTENDS_BEFORE_OLD_BBL  2
#define EXITS_OLD_BBL  4
#define EXPANDS_OVER_OLD_BBL  8
#define COVER_MULTIPLE_BBLS  16
#define NUM_BBL_TYPES 32

#define BBL_ENTER_MASK 3
#define BBL_EXIT_MASK  12

public:
   PrivateRoutine(PrivateLoadModule *_img, addrtype _start, usize_t _size, 
               const std::string& _name, addrtype _offset);
   virtual ~PrivateRoutine();
   // optionally, pass a string that is appended to a routine name to create the 
   // name of the output file. The idea is to pass process and thread IDs from the 
   // profiler so that we get separate CFG dumps for each thread.
   void CfgToDot(const char* suffix=0, int parts=1) const;
   void CfgToDot(MiamiRIFG *mCfg, TarjanIntervals *tarj, const char* suffix=0, int parts=1) const;

   const std::string& Name() const { return (name); }

   virtual PrivateCFG* ControlFlowGraph() const   { return (g); };
   void DeleteControlFlowGraph()  {
      if (g) delete (g);
      g = NULL;
   }
   virtual PrivateLoadModule* InLoadModule() const
   {
      return (img);
   }
   
   bool HasEntryAt(addrtype pc) {
      return (entries.find(pc)!=entries.end());
   }
   bool HasEntryInRange(addrtype start, addrtype end, addrtype& entry) {
      EntryMap::iterator eit = entries.lower_bound(start);
      if (eit!=entries.end() && eit->first<end)
      {
         entry = eit->first;
         return (true);
      } else
         return (false);
   }
#if 0
   bool HasCallReturnAt(addrtype pc) {
      return (returns.find(pc)!=returns.end());
   }
#endif
   bool HasTraceStartAt(addrtype pc) {
      return (traces.find(pc)!=traces.end());
   }
   bool HasTraceStartInRange(addrtype start, addrtype end, addrtype& entry) {
      EntryMap::iterator eit = traces.lower_bound(start);
      if (eit!=traces.end() && eit->first<end)
      {
         entry = eit->first;
         return (true);
      } else
         return (false);
   }
   int DeleteAnyTraceStartsInRange(addrtype start, addrtype end, addrtype& del_addr)
   {
      int i=0;
      EntryMap::iterator eit = traces.lower_bound(start);
      while (eit!=traces.end() && eit->first<end)
      {
         if (i==0)
            del_addr = eit->first;
         EntryMap::iterator tmp = eit;
         ++ eit;
         traces.erase(tmp);
         ++ i;
      }
      return (i);
   }
   
   inline uint64_t* EntryCounter(addrtype pc)
   {
      return (&entries[pc]);
   }
#if 0
   inline uint64_t* ReturnCounter(addrtype pc)
   {
      return (&returns[pc]);
   }
#endif
   inline uint64_t* TraceCounter(addrtype pc)
   {
      return (&traces[pc]);
   }

   inline void addEntryCounter(addrtype pc)
   {
      if (entries.find(pc)==entries.end())
         entries.insert(EntryMap::value_type(pc, 0));
   }
#if 0
   inline void addReturnCounter(addrtype pc)
   {
      if (returns.find(pc)==returns.end())
         returns.insert(EntryMap::value_type(pc, 0));
   }
#endif
   inline void addTraceCounter(addrtype pc)
   {
      if (traces.find(pc)==traces.end())
         traces.insert(EntryMap::value_type(pc, 0));
   }
   
   inline addrtype Start() const  { return (start_addr); }
   inline addrtype End() const  { return (start_addr+size); }
   inline addrtype StartOffset() const  { return (start_offset); }
   inline usize_t Size() const  { return (size); }

   /* routines used during CFG discovery */
   virtual int AddBlock(addrtype _start, addrtype _end, 
               PrivateCFG::NodeType type = PrivateCFG::MIAMI_CODE_BLOCK,
               int *inEdges = 0);
   virtual int AddCallSurrogateBlock(addrtype call_addr, addrtype _end, addrtype target,
               PrivateCFG::EdgeType etype, bool control_returns, PrivateCFG::Edge** pe = 0);
   
   PrivateCFG::Edge* AddPotentialEdge(addrtype end_addr, addrtype target, 
               PrivateCFG::EdgeType type = PrivateCFG::MIAMI_FALLTHRU_EDGE);
   PrivateCFG::Edge* AddReturnEdge(addrtype end_addr, 
               PrivateCFG::EdgeType etype = PrivateCFG::MIAMI_RETURN_EDGE);
   // statically discover the control flow graph, starting from the instruction
   // at address entry
   int discover_CFG(addrtype entry);

   // return true if we may fall-through from a call-site to the PC
   // This includes cases when we can certainly identify such cases, and
   // cases when we do not have the CFG for the previous address built.
   // @strict - if unset, return true also if the block at PC does not have
   //           any incoming control flow edges (MIAMI_CFG_EDGEs excluded)
   bool MayFollowCallSite(addrtype pc, bool strict = true);

   PrivateCFG::Node* FindCFGNodeAtAddress(addrtype pc);

   addrtype start_addr;
protected:
   addrtype start_offset;
   std::string name;
   usize_t size;
   EntryMap entries;   // store the entry points for the routine
//   EntryMap returns;   // store the points after call-sites for the routine
   EntryMap traces;    // store trace starts

   BBMap _blocks;
   AddrEdgeMap _calls;
   
   // CFG for this routine
   PrivateCFG *g;
   PrivateLoadModule *img;
};

}

#endif
