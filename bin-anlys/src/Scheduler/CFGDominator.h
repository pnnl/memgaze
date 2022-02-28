/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: CFGDominator.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Extends the base Dominator class to implement a dominator algorithm for the
 * the Scheduler CFG implementation.
 */

#ifndef _MIAMI_CFG_DOMINATOR_H
#define _MIAMI_CFG_DOMINATOR_H

#include "Dominator.h"
#include "CFG.h"

namespace MIAMI
{

template <class forward_iterator, class backward_iterator>
class CFGDominator : public Dominator<forward_iterator, backward_iterator>
{
public:
   CFGDominator (CFG *_g, SNodeList &root_nodes, bool _incl_struct=false, 
             const char *prefix = "") 
        : Dominator<forward_iterator, backward_iterator>(_g, root_nodes, _incl_struct, prefix)
   { }
   
   CFGDominator (CFG *_g, SNode* rootN, bool _incl_struct=false, 
             const char *prefix = "") 
        : Dominator<forward_iterator, backward_iterator>(_g, rootN, _incl_struct, prefix)
   { }

   virtual CFG::Node* iDominator (SNode *node) {
      return (dynamic_cast<CFG::Node*>((this->nodeDom)[node->getId()]));
   }

/*  cannot dynamic cast to **. Use the implementation from the base class which 
 *  returns an array of base pointers. Dynamic cast each individual pointer 
 *  when needed. 
 */
/*
   CFG::Node** getDomTree () const  { 
      return (dynamic_cast<CFG::Node**>(this->nodeDom)); 
   }
*/
private:
   
};

}  // namespace MIAMI

#endif
