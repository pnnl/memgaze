/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: DGDominator.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Extends the base Dominator class to implement a dominator algorithm for the
 * the Scheduler dependence graph implementation.
 */

#ifndef _MIAMI_DG_DOMINATOR_H
#define _MIAMI_DG_DOMINATOR_H

#include "Dominator.h"
#include "SchedDG.h"

namespace MIAMI_DG
{

template <class forward_iterator, class backward_iterator>
class DGDominator : public MIAMI::Dominator<forward_iterator, backward_iterator>
{
public:
   DGDominator (SchedDG *_g, MIAMI::SNodeList &root_nodes, bool _incl_struct=false, 
             const char *prefix = "") 
        : Dominator<forward_iterator, backward_iterator>(_g, root_nodes, _incl_struct, prefix)
   { }
   
   DGDominator (SchedDG *_g, MIAMI::SNode* rootN, bool _incl_struct=false, 
             const char *prefix = "") 
        : Dominator<forward_iterator, backward_iterator>(_g, rootN, _incl_struct, prefix)
   { }

   virtual SchedDG::Node* iDominator (MIAMI::SNode *node) {
      return (dynamic_cast<SchedDG::Node*>((this->nodeDom)[node->getId()]));
   }
   
   virtual ~DGDominator() { }
   
private:
   
};

}  /* namespace MIAMI_DG */

#endif
