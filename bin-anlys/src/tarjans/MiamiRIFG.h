/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: MiamiRIFG.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Extends the Representation Independent Flowgraph Interface to access 
 * a MIAMI PrivateCFG graph implementation.
 */

#ifndef MIAMI_RIFG_h
#define MIAMI_RIFG_h

//************************************************************************************************
// MIAMI Flowgraph, extends the Representation Independent Flowgraph Interface
// 
//************************************************************************************************

#include "RIFG.h"
#include "PrivateCFG.h"
#include "OAUtils/BaseGraph.h"
#include "tarjans/TarjanIntervals.h"

namespace MIAMI
{

//===============================================================================================
// Representation Independent Flowgraph Interface
//===============================================================================================
class MiamiRIFG : public RIFG
{
private:
    PrivateCFG::Node** _cfgNodes;
    PrivateCFG::Edge** _cfgEdges;
    int *topMap;
    PrivateCFG *pCfg;

   void Initialize();
    
public:
    MiamiRIFG();
    MiamiRIFG(PrivateCFG *_cfg);
    ~MiamiRIFG();
    
    inline  void* GetBaseCFG() { return (pCfg); }
    
    //-------------------------------------------------------------------------------------------
    // assumption: node identifiers are mostly dense, though some may have been freed
    //-------------------------------------------------------------------------------------------
    RIFGNodeId HighWaterMarkNodeId();  // largest node id in the graph


    bool IsValid(RIFGNodeId n);     // is the node id still valid, or has it been freed
    int GetFanin(TarjanIntervals*, RIFGNodeId);

    RIFGNodeId GetRootNode();
    RIFGNodeId GetFirstNode();
    RIFGNodeId GetLastNode();

    RIFGNodeId GetNextNode(RIFGNodeId n);
    RIFGNodeId GetPrevNode(RIFGNodeId n);

    RIFGNodeId GetEdgeSrc(RIFGEdgeId e);
    RIFGNodeId GetEdgeSink(RIFGEdgeId e);

    RIFGNodeId *GetTopologicalMap(TarjanIntervals *);

    RIFGNode *GetRIFGNode(RIFGNodeId n);
    RIFGEdge *GetRIFGEdge(RIFGEdgeId e);
    
    RIFGEdgeIterator *GetEdgeIterator(RIFGNodeId n,  EdgeDirection ed);
    RIFGEdgeIterator *GetSortedEdgeIterator(RIFGNodeId n,  EdgeDirection ed);
    RIFGNodeIterator *GetNodeIterator(ForwardBackward fb = FORWARD);
};


//===============================================================================================
// MIAMI Edge Iterator - extends the Representation Independent Edge Iterator Interface
//===============================================================================================
class MiamiEdgeIterator : public RIFGEdgeIterator
{
private:
        RIFG *fg;
        RIFGNodeId n;
        EdgeDirection ed;
        RIFGEdgeId currentEdge;
        BaseGraph::BaseEdgesIterator *ei;
public:
        MiamiEdgeIterator(RIFG *_fg, RIFGNodeId _n,  EdgeDirection _ed);
        ~MiamiEdgeIterator();

        RIFGEdgeId Current();
        RIFGEdgeId operator++(int);

        void Reset();
};

//===============================================================================================
// MIAMI Sorted Edge Iterator - extends the Representation Independent Edge Iterator Interface
// Provides a consistent traversal of the edges.
//===============================================================================================
class MiamiSortedEdgeIterator : public RIFGEdgeIterator
{
private:
        RIFG *fg;
        RIFGNodeId n;
        EdgeDirection ed;
        RIFGEdgeId currentEdge;
        PrivateCFG::SortedEdgesIterator *ei;
public:
        MiamiSortedEdgeIterator(RIFG *_fg, RIFGNodeId _n,  EdgeDirection _ed);
        ~MiamiSortedEdgeIterator();

        RIFGEdgeId Current();
        RIFGEdgeId operator++(int);

        void Reset();
};


//===============================================================================================
// MIAMI All Edges Iterator - extends the Representation Independent Edge Iterator Interface
//===============================================================================================
class MiamiAllEdgesIterator : public RIFGEdgeIterator
{
private:
        RIFG *fg;
        RIFGEdgeId currentEdge;
        BaseGraph::BaseEdgesIterator *ei;
public:
        MiamiAllEdgesIterator(RIFG *_fg);
        ~MiamiAllEdgesIterator();

        RIFGEdgeId Current();
        RIFGEdgeId operator++(int);

        void Reset();
};


//===============================================================================================
// MIAMI Node Iterator - extends the Representation Independent Node Iterator Interface
//===============================================================================================
class MiamiNodeIterator : public RIFGNodeIterator
{
private:
        RIFG *fg;
        ForwardBackward fb;
        RIFGNodeId currentNode;
public:
        MiamiNodeIterator(RIFG *_fg, ForwardBackward _fb = FORWARD);
        ~MiamiNodeIterator();

        RIFGNodeId Current();
        RIFGNodeId operator++(int);

        void Reset();
};

}

#endif
