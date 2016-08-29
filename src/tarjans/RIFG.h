/* * BeginRiceCopyright *****************************************************
 * 
 * Copyright ((c)) 2002, 2003 Rice University 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 * * Neither the name of Rice University (RICE) nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * This software is provided by RICE and contributors "as is" and any
 * express or implied warranties, including, but not limited to, the
 * implied warranties of merchantability and fitness for a particular
 * purpose are disclaimed. In no event shall RICE or contributors be
 * liable for any direct, indirect, incidental, special, exemplary, or
 * consequential damages (including, but not limited to, procurement of
 * substitute goods or services; loss of use, data, or profits; or
 * business interruption) however caused and on any theory of liability,
 * whether in contract, strict liability, or tort (including negligence
 * or otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage. 
 * 
 * ******************************************************* EndRiceCopyright */


#ifndef RIFG_h
#define RIFG_h

//************************************************************************************************
// Representation Independent Flowgraph interface
// 
//************************************************************************************************

#include <sys/types.h>
#include "EdgeDirection.h"
#include "ForwardBackward.h"

typedef int32_t RIFGNodeId;
typedef int32_t RIFGEdgeId;

typedef void RIFGEdge;
typedef void RIFGNode;
#define RIFG_NIL -1

class TarjanIntervals;
class RIFGEdgeIterator;
class RIFGNodeIterator;

//===============================================================================================
// Representation Independent Flowgraph Interface
//===============================================================================================
class RIFG {
public:
	//-------------------------------------------------------------------------------------------
	// assumption: node identifiers are mostly dense, though some may have been freed
	//-------------------------------------------------------------------------------------------
	virtual int32_t HighWaterMarkNodeId()=0;  // largest node id in the graph


	virtual bool IsValid(RIFGNodeId n)=0;     // is the node id still valid, or has it been freed
	virtual int GetFanin(TarjanIntervals *, RIFGNodeId)=0;

	virtual RIFGNodeId GetRootNode()=0;
	virtual RIFGNodeId GetFirstNode()=0;
	virtual RIFGNodeId GetLastNode()=0;

	virtual RIFGNodeId GetNextNode(RIFGNodeId n)=0;
	virtual RIFGNodeId GetPrevNode(RIFGNodeId n)=0;

	virtual RIFGNodeId GetEdgeSrc(RIFGEdgeId e)=0;
	virtual RIFGNodeId GetEdgeSink(RIFGEdgeId e)=0;

	virtual RIFGNodeId *GetTopologicalMap(TarjanIntervals *)=0;

	virtual RIFGNode *GetRIFGNode(RIFGNodeId n)=0;
	virtual RIFGEdge *GetRIFGEdge(RIFGEdgeId e)=0;
	virtual void* GetBaseCFG() = 0;
	
	virtual RIFGEdgeIterator *GetEdgeIterator(RIFGNodeId n,  EdgeDirection ed)=0;
	virtual RIFGEdgeIterator *GetSortedEdgeIterator(RIFGNodeId n,  EdgeDirection ed)=0;
	virtual RIFGNodeIterator *GetNodeIterator(ForwardBackward fb = FORWARD)=0;
	
	virtual ~RIFG() { }
};

//===============================================================================================
// Representation Independent Flowgraph Edge Iterator
//===============================================================================================
class RIFGEdgeIterator {
public:
	virtual RIFGEdgeId Current()=0;
	virtual RIFGEdgeId operator++(int)=0;

	virtual void Reset()=0;
	virtual ~RIFGEdgeIterator() { }
};

//===============================================================================================
// Representation Independent Flowgraph Node Iterator
//===============================================================================================
class RIFGNodeIterator {
public:
	virtual RIFGNodeId Current() = 0;
	virtual RIFGNodeId operator++(int) = 0;

	virtual void Reset() = 0;
	virtual ~RIFGNodeIterator() { }
};
#endif
