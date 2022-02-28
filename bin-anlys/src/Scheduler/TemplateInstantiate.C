/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: TemplateInstantiate.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Declares certain template instantiations to force the compiler to define
 * some otherwise missing symbols.
 */

#include "StringAssocTable.h"
#include "StringAssocTable.C"
#include "MachineExecutionUnit.h"
#include "MemoryHierarchyLevel.h"

template class MIAMI::StringAssocTable<MIAMI::MachineExecutionUnit>;
template class MIAMI::StringAssocTable<MIAMI::MemoryHierarchyLevel>;

#include "SchedDG.h"
#include "Dominator.h"
#include "Dominator.C"

template class MIAMI::Dominator<MIAMI_DG::SchedDG::PredecessorNodesIterator, MIAMI_DG::SchedDG::SuccessorNodesIterator>;
template class MIAMI::Dominator<MIAMI_DG::SchedDG::SuccessorNodesIterator, MIAMI_DG::SchedDG::PredecessorNodesIterator>;
/*
template int MIAMI::Dominator<MIAMI_DG::SchedDG::SuccessorNodesIterator, MIAMI_DG::SchedDG::PredecessorNodesIterator>::findLCA(SNodeList &nodeset, int nid, DisjointSet *dsuf, DisjointSet *ds,
               SNodeList *otherset, SNode **minLCA);
*/
