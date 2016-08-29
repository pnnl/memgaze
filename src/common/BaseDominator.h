/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: BaseDominator.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A base dominator interface.
 */

#ifndef _MIAMI_BASE_DOMINATOR_H
#define _MIAMI_BASE_DOMINATOR_H

#include <list>
#include "OAUtils/DGraph.h"

namespace MIAMI
{

typedef DGraph::Node SNode;
typedef DGraph::Edge SEdge;
typedef std::list<DGraph::Node*> SNodeList;

}  /* namespace MIAMI */

// define a base dominator class, so I can use it to pass generic dominator 
// objects as arguments.
class BaseDominator
{
public:
   BaseDominator (DGraph *_g);
   virtual ~BaseDominator ();
   
   // need a public method to return the immediate dominator of a node
   virtual MIAMI::SNode *iDominator (MIAMI::SNode *node) {
      return (nodeDom[node->getId()]);
   }
   
   MIAMI::SNode** getDomTree () const      { return (nodeDom); }

protected:
   MIAMI::SNode **nodeDom; // stores the immediate dominator of a nodeId
   int numNodes;    // the number of actual nodes found, duh
   DGraph *g;
};


#endif
