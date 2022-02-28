/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: MiamiRIFG.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Extends the Representation Independent Flowgraph Interface to access 
 * a MIAMI PrivateCFG graph implementation.
 */

//************************************************************************************************
// MIAMI Flowgraph, extends the Representation Independent Flowgraph Interface
// 
//************************************************************************************************

#include <tarjans/MiamiRIFG.h>
#include "Assertion.h"

using namespace MIAMI;

MiamiRIFG::MiamiRIFG() : pCfg(NULL)
{
   Initialize();
}

MiamiRIFG::MiamiRIFG(PrivateCFG *_cfg) : pCfg(_cfg)
{
   Initialize();
}

MiamiRIFG::~MiamiRIFG()
{
  if (_cfgNodes != NULL)
    delete[] _cfgNodes;
  if (_cfgEdges != NULL)
    delete[] _cfgEdges;
  if (topMap != NULL)
    delete[] topMap;
}


void MiamiRIFG::Initialize()
{
   _cfgNodes = NULL;
   _cfgEdges = NULL;
   topMap = NULL;
   if (pCfg)
      pCfg->FinalizeGraph();
}


RIFGNodeId MiamiRIFG::HighWaterMarkNodeId()  // largest node id in the graph
{
   return pCfg->HighWaterMarker();
}

// tell if a node (basic_block) is valid or not
// for now it will return always 1 (all nodes are valid)
bool MiamiRIFG::IsValid(RIFGNodeId n)     // is the node id still valid, or has it been freed
{
   return (GetRIFGNode(n)!=NULL);
}

RIFGNodeId MiamiRIFG::GetRootNode()
{
   return pCfg->CfgEntry()->getId();
}

RIFGNodeId MiamiRIFG::GetFirstNode()
{
   return GetNextNode(-1);
}

RIFGNodeId MiamiRIFG::GetLastNode()
{
   return GetPrevNode(pCfg->NumNodes());
}

RIFGNodeId MiamiRIFG::GetNextNode(RIFGNodeId i)
{
   RIFGNodeId n = i;
   ObjId size = pCfg->NumNodes();
  
   for (n=i+1 ; n<size && !GetRIFGNode(n) ; ++n);

   BriefAssertion(n>=0 && n<=size);
   if ( n == size )
      return RIFG_NIL;
   else
      return n;
}

RIFGNodeId MiamiRIFG::GetPrevNode(RIFGNodeId i)
{
   RIFGNodeId n = i;
  
   for (n=i-1 ; n>=0 && !GetRIFGNode(n) ; --n);

   BriefAssertion(n>=-1);
   if ( n == (-1) )
      return RIFG_NIL;
   else
      return n;
}

RIFGNodeId MiamiRIFG::GetEdgeSrc(RIFGEdgeId e)
{
   return (static_cast<PrivateCFG::Edge*>(GetRIFGEdge(e))->source()->getId());
}

RIFGNodeId MiamiRIFG::GetEdgeSink(RIFGEdgeId e)
{
   return (static_cast<PrivateCFG::Edge*>(GetRIFGEdge(e))->sink()->getId());
}

// WARNING: Tarjan tree must have been built before
int MiamiRIFG::GetFanin(TarjanIntervals *tj, RIFGNodeId n)
{
   int fFanIn = 0;  /* fanin for forward edges */

   PrivateCFG::IncomingEdgesIterator ieit(static_cast<PrivateCFG::Node*>(GetRIFGNode(n)));
   while ((bool)ieit)
   {
      if (!tj->IsBackEdge(ieit->getId()))
          fFanIn++;
      ++ieit;
   }
   return (fFanIn);
}

// WARNING: Tarjan tree must have been built before
RIFGNodeId* MiamiRIFG::GetTopologicalMap(TarjanIntervals *tj)
{
   int *map;
   int *fanInLeft;
   int *readyToNumber;   /* stack of CFG id's ready to get a number */
   int stackTop = 0;
   int i, cfgId;        
   int dest;             /* successor CFG node of cfgId */
   int cfgSize;           /* size of CFG graph */
   int topNum;            /* next topological number */

   if (topMap)
      return(topMap);

   cfgSize = pCfg->NumNodes();
   map = new int[cfgSize];

   for (int j=0 ; j<cfgSize ; ++j)
      map[j] = RIFG_NIL;

   fanInLeft = new int[cfgSize];

   readyToNumber = new int[cfgSize];

   /*
    * Initialize fanInLeft[] to contain number of forward CFG in edges
    * Initialize readyToNumber to contain nodes with no forward in edges
    */
   for (i=GetFirstNode() ; i!=RIFG_NIL ; i=GetNextNode(i))
   {
      fanInLeft[i] = GetFanin(tj, i);

      if (fanInLeft[i] == 0)
         readyToNumber[stackTop++] = i;  /* ready to give a Top number */
   } /* end of for each CFG node initialize */


   /*
    * Now start issuing topological numbers
    */
   topNum = GetFirstNode();
   while (stackTop > 0)        /* while stack not empty */
   {
      cfgId = readyToNumber[--stackTop];
      map[topNum] = cfgId;

      topNum = GetNextNode(topNum);

      /* Decrement fanin count of cfgId's forward successors */
      PrivateCFG::OutgoingEdgesIterator oeit(static_cast<PrivateCFG::Node*>(GetRIFGNode(cfgId)));
      while ((bool)oeit)
      {
         dest = oeit->sink()->getId();
         if (!tj->IsBackEdge(oeit->getId()))
         {
            fanInLeft[dest]--;
            if (fanInLeft[dest] == 0)
            {
               readyToNumber[stackTop++] = dest;
            }
         }
         ++oeit;
      }
   } /* end of while stack not empty */

   BriefAssertion (stackTop == 0);

   delete[] readyToNumber;
   delete[] fanInLeft;

   topMap = map;
   return map;
}

RIFGNode *MiamiRIFG::GetRIFGNode(RIFGNodeId n)
{
   if (_cfgNodes == NULL)
   {
      BriefAssertion (_cfgEdges == NULL);

      // allocate the array and assign the blocks in it
      _cfgNodes = new PrivateCFG::Node*[pCfg->NumNodes()];
      _cfgEdges = new PrivateCFG::Edge*[pCfg->NumEdges()];

      PrivateCFG::NodesIterator nit(*pCfg);
      while ((bool)nit)
      {
         PrivateCFG::Node *nn = nit;
         _cfgNodes[nn->getId()] = nn;
         ++nit;
      }
      PrivateCFG::EdgesIterator eit(*pCfg);
      while ((bool)eit)
      {
         PrivateCFG::Edge *ee = eit;
         _cfgEdges[ee->getId()] = ee;
         ++eit;
      }
   }
  
   return (RIFGNode*)_cfgNodes[n];
}

RIFGEdge *MiamiRIFG::GetRIFGEdge(RIFGEdgeId e)
{
   if (_cfgEdges == NULL)
   {
      BriefAssertion (_cfgNodes == NULL);
      //RIFGNode *b = GetRIFGNode(0);
      BriefAssertion (_cfgEdges != NULL);
   }
  
   return (RIFGEdge*)_cfgEdges[e];
}


RIFGEdgeIterator *MiamiRIFG::GetEdgeIterator(RIFGNodeId n,  EdgeDirection ed)
{
   return (new MiamiEdgeIterator(this, n, ed));
}

RIFGEdgeIterator *MiamiRIFG::GetSortedEdgeIterator(RIFGNodeId n,  EdgeDirection ed)
{
   return (new MiamiSortedEdgeIterator(this, n, ed));
}

RIFGNodeIterator *MiamiRIFG::GetNodeIterator(ForwardBackward _fb)
{
   return (new MiamiNodeIterator(this, _fb));
}


MiamiEdgeIterator::MiamiEdgeIterator(RIFG *_fg, RIFGNodeId _n,
			EdgeDirection _ed) : fg(_fg)
{
   n = _n;
   ed = _ed;
   ei = NULL;
   Reset();
}


MiamiEdgeIterator::~MiamiEdgeIterator()
{
   if (ei)
      delete ei;
   ei = NULL;
}


RIFGEdgeId 
MiamiEdgeIterator::Current()
{
   return currentEdge;
}


RIFGEdgeId 
MiamiEdgeIterator::operator++(int)
{
   if (currentEdge!=RIFG_NIL)
   {
      ++ (*ei);
      if ((bool)(*ei))
         currentEdge = (*ei)->getId();
      else
         currentEdge = RIFG_NIL;
   }
   return currentEdge;
}


void 
MiamiEdgeIterator::Reset()
{
   if (ei != NULL)
   {
      delete ei;
      ei = NULL;
   }
   if (ed == ED_INCOMING)
      ei = new PrivateCFG::IncomingEdgesIterator( ((PrivateCFG::Node*)fg->GetRIFGNode(n)) );
   else
      ei = new PrivateCFG::OutgoingEdgesIterator( ((PrivateCFG::Node*)fg->GetRIFGNode(n)) );
   if ((bool)(*ei))
      currentEdge = (*ei)->getId();
   else
      currentEdge = RIFG_NIL;
}

MiamiSortedEdgeIterator::MiamiSortedEdgeIterator(RIFG *_fg, RIFGNodeId _n,
			EdgeDirection _ed) : fg(_fg)
{
   n = _n;
   ed = _ed;
   ei = NULL;
   Reset();
}


MiamiSortedEdgeIterator::~MiamiSortedEdgeIterator()
{
   if (ei)
      delete ei;
   ei = NULL;
}


RIFGEdgeId 
MiamiSortedEdgeIterator::Current()
{
   return currentEdge;
}


RIFGEdgeId 
MiamiSortedEdgeIterator::operator++(int)
{
   if (currentEdge!=RIFG_NIL)
   {
      ++ (*ei);
      if ((bool)(*ei))
         currentEdge = (*ei)->getId();
      else
         currentEdge = RIFG_NIL;
   }
   return currentEdge;
}


void 
MiamiSortedEdgeIterator::Reset()
{
   if (ei != NULL)
   {
      delete ei;
      ei = NULL;
   }
   BaseGraph::BaseEdgesIterator *temp = 0;
   if (ed == ED_INCOMING)
      temp = new PrivateCFG::IncomingEdgesIterator( ((PrivateCFG::Node*)fg->GetRIFGNode(n)) );
   else
      temp = new PrivateCFG::OutgoingEdgesIterator( ((PrivateCFG::Node*)fg->GetRIFGNode(n)) );
   if (temp)
   {
      ei = new PrivateCFG::SortedEdgesIterator(temp);
      delete (temp);
   }
   if ((bool)(*ei))
      currentEdge = (*ei)->getId();
   else
      currentEdge = RIFG_NIL;
}

MiamiAllEdgesIterator::MiamiAllEdgesIterator(RIFG *_fg) : fg(_fg)
{
   ei = NULL;
   Reset();
}


MiamiAllEdgesIterator::~MiamiAllEdgesIterator()
{
   if (ei)
      delete ei;
   ei = NULL;
}


RIFGEdgeId 
MiamiAllEdgesIterator::Current()
{
   return currentEdge;
}


RIFGEdgeId 
MiamiAllEdgesIterator::operator++(int)
{
   if (currentEdge!=RIFG_NIL)
   {
      ++ (*ei);
      if ((bool)(*ei))
         currentEdge = (*ei)->getId();
      else
         currentEdge = RIFG_NIL;
   }
   return currentEdge;
}


void 
MiamiAllEdgesIterator::Reset()
{
   if (ei != NULL)
   {
      delete ei;
      ei = NULL;
   }
   ei = new PrivateCFG::EdgesIterator(*(static_cast<PrivateCFG*>(fg->GetBaseCFG())));
   if ((bool)(*ei))
      currentEdge = (*ei)->getId();
   else
      currentEdge = RIFG_NIL;
}


MiamiNodeIterator::MiamiNodeIterator(RIFG *_fg, ForwardBackward _fb) : fg(_fg)
{
   fb = _fb;
   Reset();
}


MiamiNodeIterator::~MiamiNodeIterator()
{
}


RIFGNodeId 
MiamiNodeIterator::Current()
{
   return currentNode;
}


RIFGNodeId 
MiamiNodeIterator::operator++(int)
{
   if (fb == FORWARD)
      currentNode = fg->GetNextNode(currentNode);
   else
      currentNode = fg->GetPrevNode(currentNode);
   return currentNode;
}


void 
MiamiNodeIterator::Reset()
{
   if (fb == FORWARD)
      currentNode = fg->GetFirstNode();
   else
      currentNode = fg->GetLastNode();
}
