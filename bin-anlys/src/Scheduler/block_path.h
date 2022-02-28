/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: block_path.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Data structure associated with a path in the CFG. A path
 * is defined by a vector of CFG basic blocks.
 */

#ifndef MIAMI_BLOCK_PATH_H
#define MIAMI_BLOCK_PATH_H

#include <map>
#include "CFG.h"
#include "uipair.h"
#include "PathInfo.h"
#include "reg_sched_info.h"

#include "fast_hashmap.h"

namespace MIAMI
{

// create a dummy edge class that I create for inner loops connections
class inner_loop_edge : public virtual CFG::Edge
{
public:
  // Constructor:
  //
  inner_loop_edge (CFG::Node* head, CFG::Node* tail /*, card i*/)
    : CFG::Edge (head, tail/*, i*/)
  {
  }
};

class BMarker
{
public:
   int64_t freq;
   CFG::Node* entry;
   CFG::Node* b;
   unsigned int marker;
   CFG::Edge *dummyE;

   BMarker (CFG::Node *_ent, CFG::Node* _b, unsigned int _marker, CFG::Edge *_dummyE, uint64_t _freq=0) :
              freq(_freq), entry(_ent), b(_b), marker(_marker), dummyE(_dummyE)
   { }
   
   BMarker (const BMarker &bm)
   {
      freq = bm.freq;
      entry = bm.entry;
      b = bm.b;
      marker = bm.marker;
//      if (dummyE) delete dummyE;
      if (bm.dummyE)
      {
         dummyE = new inner_loop_edge (bm.dummyE->source(), bm.dummyE->sink());
         dummyE->setExecCount (bm.dummyE->ExecCount ());
         dummyE->setNumLevelsEntered(bm.dummyE->getNumLevelsEntered());
      } else
         dummyE = 0;
   }
   
   BMarker& operator= (const BMarker &bm)
   {
      freq = bm.freq;
      entry = bm.entry;
      b = bm.b;
      marker = bm.marker;
      if (dummyE) delete dummyE;
      if (bm.dummyE)
      {
         dummyE = new inner_loop_edge (bm.dummyE->source(), bm.dummyE->sink());
         dummyE->setExecCount (bm.dummyE->ExecCount ());
         dummyE->setNumLevelsEntered(bm.dummyE->getNumLevelsEntered());
      } else
         dummyE = 0;
      return (*this);
   }
   
   ~BMarker ()
   {
      if (dummyE)
         delete dummyE;
   }
};

class OrderBMs {
public :
   int operator() (const BMarker &x, const BMarker &y) const;
};

typedef std::set <BMarker, OrderBMs> BMSet;


class BlockPath
{
public:
   BlockPath (CFG::NodeList &na, CFG::Node* _next_block, MIAMIU::FloatArray &fa, 
           RSIListList &_regList, bool _exit_path);
   BlockPath (BlockPath& bp);
   ~BlockPath ();
   
   BlockPath& operator= (const BlockPath& bp);
   void include (int pos, CFG::NodeList& na, MIAMIU::FloatArray& fa);
   void trimFat ();
   bool isExitPath () const  { return (is_exit_path); }

   void Dump(FILE *fout);

   class CompareKey
   {
   public:
      bool operator() (const BlockPath* k1, const BlockPath* k2) const;
   };
   
//private:
   uint64_t hashVal;
   uint32_t size;
   uint32_t numLoops;
   bool is_exit_path;
   CFG::Node** blocks;
   float* probabs;
   CFG::Node* next_block;
   RSIList *innerRegs;
};


typedef std::map <BlockPath*, PathInfo*, BlockPath::CompareKey> BPMap;

}  /* namespace MIAMI */

#endif
