/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: routine.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data bookkeeping and analysis at the routine level for a data reuse 
 * analysis tool. Extends the PrivateRoutine implementation with 
 * functionality specific to the data reuse tool.
 */

#include <assert.h>
#include <string.h>
#include "routine.h"

using namespace std;
using namespace MIAMI;

extern int MIAMI_MEM_REUSE::verbose_level;

#define DEBUG_BBLs 0

#if DEBUG_BBLs
const char *debugName = "__mempcpy";
#endif

Routine::Routine(LoadModule *_img, addrtype _start, usize_t _size, 
           const std::string& _name, addrtype _offset)
     : PrivateRoutine(_img, _start, _size, _name, _offset)
{
   g = new CFG(this, _name);
}

Routine::~Routine()
{
   if (g)
      delete g;
}

void
Routine::DetermineInstrumentationPoints()
{
   if (! g)
   {
      fprintf(stderr, "ERROR: Routine::DetermineInstrumentationPoints: routine %s does not have a control flow graph yet?? Unacceptable.\n", 
              name.c_str());
      assert(!"No CFG for routine.");
      return;
   }
   
   MiamiRIFG mCfg(g);
   TarjanIntervals tarj(mCfg);
   RIFGNodeId root = mCfg.GetRootNode();
   if (MIAMI_MEM_REUSE::verbose_level > 1)
   {
      fprintf(stderr, ">> Routine::DetermineInstrumentationPoints: tarjan root node for routine %s, [0x%" 
          PRIxaddr ",0x%" PRIxaddr "], root=%d\n", 
               name.c_str(), start_addr, start_addr+size, root);
   }
   // I need to do a DFS traversal of the graph, and determine the places 
   // where I need to insert scope ENTRY/EXIT/ITER CHANGE/etc. events
   static_cast<CFG*>(g)->FindScopesRecursively(root, &tarj, &mCfg, InLoadModule(), 0);
   static_cast<CFG*>(g)->FindLoopEntryExitEdges(root, &tarj, &mCfg, InLoadModule());
}
