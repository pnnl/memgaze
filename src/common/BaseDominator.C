/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: BaseDominator.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A base dominator interface.
 */

#include <string.h>
#include "BaseDominator.h"

BaseDominator::BaseDominator (DGraph *_g)
{
   g = _g;
   numNodes = g->HighWaterMarker();
   nodeDom = new MIAMI::SNode* [numNodes];
   memset (nodeDom, 0, numNodes*sizeof(MIAMI::SNode*));
}

BaseDominator::~BaseDominator ()
{
   delete[] nodeDom;
}
