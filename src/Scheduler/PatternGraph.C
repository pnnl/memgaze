/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: PatternGraph.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Graph class to hold the left and the right sides of a replacement rule.
 */

#include "GenericInstruction.h"
#include "PatternGraph.h"
#include "debug_scheduler.h"
#include <assert.h>

using namespace MIAMI;
namespace MIAMI {
extern int haveErrors;
}

ObjId PatternGraph::Node::nextId = 1;
ObjId PatternGraph::Edge::nextId = 1;

PatternGraph::PatternGraph(GenInstList* pattern, const Position& patternPos, PRegMap& regMap)
{
#if DEBUG_MACHINE_CONSTRUCTION
   DEBUG_MACHINE (2,
      fprintf(stderr, "** Inside PatternGraph constructor **\n");
   )
#endif
   
   // create a CFG top node; dependencies from this node to another node, 
   // will represent dependencies from the instructions before the pattern.
//   cfg_top = new Node(-1);  // I think we do not really need it

   int destRegs = 0;
   GenInstList::iterator git = pattern->begin();
   // I have a sequence of instructions; I create only forward dependencies.
   // The graph will be acyclic. I also have a series of restrictions:
   // - Instructions in the source pattern should form a list-graph, not a generalized
   //   acyclic graph.
   // - The pattern may have only one destination register in the end
   // - Each source register name may appear only once in the entire pattern
   for ( ; git!=pattern->end() ; ++git)
   {
      const InstructionClass& ic = (*git)->getIClass();
      Node *node = new Node(ic);
      add(node);
      int gpSrcIdx = 0, memSrcIdx = 0, gpDestIdx = 0, memDestIdx = 0;
      GenRegList* regList = (*git)->SourceRegisters();
      GenRegList::iterator rit = regList->begin();
      for ( ; rit!=regList->end() ; ++rit)
      {
         unsigned int rtype  = (*rit)->Type();
         PRegMap::iterator pit = regMap.find((*rit)->RegName());
         if (pit == regMap.end())  // have not seen it before, good
         {
            // the key of the map is a char pointer. I can use the memory
            // allocated for the GenericRegister's name, as it will not be
            // deallocated before I end using this map.
            int idx;
            if (rtype == GP_REGISTER_TYPE) idx = gpSrcIdx ++;
            else
            {
               assert (rtype==MEMORY_TYPE || !"Invalid register type");
               idx = memSrcIdx ++;
            }
            regMap.insert(PRegMap::value_type((*rit)->RegName(), 
                PRegInfo(node, rtype | SOURCE_REG, idx, (*rit)->FilePosition() )) );
         } else   // I saw this register name before -> must be a temporary or an error
         {
            const Position& crtPos = (*rit)->FilePosition();
            if ( (pit->second.regType) & SOURCE_REG )   // it was source and it is source
            {
               // In the past, I was requiring source register names to be unique in a source pattern.
               // Now, I want to allow a regiter to appear twice in same instruction, but not across
               // different instructions of the source pattern. This would still limit the source pattern
               // to a degenerated list, but enables us to recognize some very specific idioms that use
               // the same register for both source operands. Do this.
               if (rtype==GP_REGISTER_TYPE && pit->second.node==node && 
                     ((pit->second.regType)&REGTYPE_MASK)==GP_REGISTER_TYPE)
               {
                  // TODO *** HERE ***
                  pit->second.regType |= MULTI_SOURCE_REG;
                  pit->second.count += 1;
                  if (pit->second.count > node->requiredRegisterCount())
                     node->requireRegisterCount(pit->second.count);
               } else
               {
                  // register names must be unique; print an error message
                  haveErrors += 1;
                  fprintf(stderr, "Error %d: File %s (%d, %d): Register $%s in replacement template has been defined before at (%d, %d). ",
                       haveErrors, crtPos.FileName(), crtPos.Line(), crtPos.Column(), 
                       (*rit)->RegName(), pit->second.pos.Line(), pit->second.pos.Column() );
                  fprintf(stderr, "A register cannot be source register more than once in one template.\n");
               }
            } else
            if ( (pit->second.regType) & TEMPORARY_REG )   // it was dest/source and it is source
            {
               // a register defined by a previous instruction in a template which was
               // already used (therefore it is marked as temporary) is used again.
               // This is forbidden because it creates pattern graphs which are not lists
               haveErrors += 1;
               fprintf(stderr, "Error %d: File %s (%d, %d): Register $%s in replacement template which was defined at (%d, %d) ",
                    haveErrors, crtPos.FileName(), crtPos.Line(), crtPos.Column(), 
                    (*rit)->RegName(), pit->second.pos.Line(), pit->second.pos.Column() );
               fprintf(stderr, "cannot be used again because it creates two outgoing dependencies from the same node.\n");
            } else
            {
               assert ((pit->second.regType) & DESTINATION_REG || !"Invalid register type");
               // if this was defined as a destination register, then this node just
               // uses it and I have to create a dependency.
               // Compute dependency type: if this reg use is memory type, and the definition is
               // register type, then this is an ADDR_REGISTER_TYPE dependency. Otherwise, the two
               // types should match
               unsigned int usetype  = (*rit)->Type();
               unsigned int deftype = (pit->second.regType) & REGTYPE_MASK;
               DependencyType dtype;
               if (deftype==GP_REGISTER_TYPE && usetype==MEMORY_TYPE)
                  dtype = ADDR_REGISTER_TYPE;
               else
               {
                  assert (deftype==usetype || !"Register use does not match definition type.");
                  dtype = (DependencyType)usetype;
               }
               Edge* edge = new Edge(pit->second.node, node, TRUE_DEPENDENCY, dtype);
               add(edge);
               // mark the register as temporary
               destRegs -= 1;
               pit->second.regType = deftype | TEMPORARY_REG;
            }
         }
      }

      regList = (*git)->DestRegisters();
      rit = regList->begin();
      for ( ; rit!=regList->end() ; ++rit)
      {
         unsigned int rtype  = (*rit)->Type();
         PRegMap::iterator pit = regMap.find((*rit)->RegName());
         if (pit == regMap.end())  // have not seen it before, good
         {
            // the key of the map is a char pointer. I can use the memory
            // allocated for the GenericRegister's name, as it will not be
            // deallocated before I end using this map.
            int idx;
            if (rtype == GP_REGISTER_TYPE) idx = gpDestIdx ++;
            else
            {
               assert (rtype==MEMORY_TYPE || !"Invalid register type");
               idx = memDestIdx ++;
            }
            destRegs += 1;
            regMap.insert(PRegMap::value_type((*rit)->RegName(), 
                PRegInfo(node, rtype | DESTINATION_REG, idx, 
                         (*rit)->FilePosition() )) );
         } else   // I saw this register name before -> must be a temporary or an error
         {
            const Position& crtPos = (*rit)->FilePosition();
            if ( (pit->second.regType) & SOURCE_REG )   // it was source and it is dest
            {
               // cannot write over a previous source reg; print an error message
               haveErrors += 1;
               fprintf(stderr, "Error %d: File %s (%d, %d): Register $%s in replacement template has been used before at (%d, %d) ",
                    haveErrors, crtPos.FileName(), crtPos.Line(), crtPos.Column(), 
                    (*rit)->RegName(), pit->second.pos.Line(), pit->second.pos.Column() );
               fprintf(stderr, "as a source register. Cannot write over a defined source register.\n");
            } else
            if ( (pit->second.regType) & DESTINATION_REG )  // it was dest and it is dest
            {
               // a register defined by a previous instruction is defined again
               // before being used by any instruction
               // If this is not a memory register (write to memory) then the previous
               // instruction that was defining this register does not define a 
               // dependency to other instructions. The pattern graph should be a list
               // of connected nodes.
               DGraph::Node* oldNode = pit->second.node;
               if ( ((pit->second.regType) & REGTYPE_MASK) != MEMORY_TYPE  &&
                         oldNode->num_outgoing() == 0 )
               {
                  haveErrors += 1;
                  fprintf(stderr, "Error %d: File %s (%d, %d): Register $%s in replacement template was defined before at (%d, %d) ",
                       haveErrors, crtPos.FileName(), crtPos.Line(), crtPos.Column(), 
                       (*rit)->RegName(), pit->second.pos.Line(), pit->second.pos.Column() );
                  fprintf(stderr, "and it is redifined here without being used. This causes its original defining instruction to have no outgoing dependency.\n");
               } else
               if ( ((pit->second.regType) & REGTYPE_MASK) == MEMORY_TYPE)
               {
                  if (oldNode->num_outgoing() == 0)
                  {
                     // add an output dependency
                     Edge* edge = new Edge(oldNode, node, OUTPUT_DEPENDENCY, 
                                MEMORY_TYPE );
                     add(edge);
                     // mark the register as temporary
                     pit->second.node = node;
                  } else
                  {
                     // oldNode has at least one other outgoing edge; report an error
                     // Should we allow multiple dependencies if they all go to the
                     // same node and are of different type??? not for now
                     haveErrors += 1;
                     fprintf(stderr, "Error %d: File %s (%d, %d): Register $%s in replacement template cause an OUTPUT memory dependency from the memory write at (%d, %d) ",
                          haveErrors, crtPos.FileName(), crtPos.Line(), crtPos.Column(), 
                          (*rit)->RegName(), pit->second.pos.Line(), pit->second.pos.Column() );
                     fprintf(stderr, " which is part of an instruction that already has an outgoing dependency.\n");
                  }
               }
            } else
            {
               assert ((pit->second.regType) & TEMPORARY_REG || !"Invalid register type");
               // if this was a temporary register, I can just reuse the same name
               if ( ((pit->second.regType) & REGTYPE_MASK) != MEMORY_TYPE)
               {
                  pit->second.regType = ((pit->second.regType) & REGTYPE_MASK) 
                                           | DESTINATION_REG;
                  pit->second.node = node;
                  destRegs += 1;
               } else  // it is MEMORY type
               {
                  // I do not have info about the node that reads the memory location
                  // If I did, I could create an ANTI-DEPENDENCY; As of now just do not
                  // allow a write into a temporary memory location
                  haveErrors += 1;
                  fprintf(stderr, "Error %d: File %s (%d, %d): Memory location $%s in replacement template is written after it was first written at (%d, %d) ",
                       haveErrors, crtPos.FileName(), crtPos.Line(), crtPos.Column(), 
                       (*rit)->RegName(), pit->second.pos.Line(), pit->second.pos.Column() );
                  fprintf(stderr, " and then used. This is not allowed in a source pattern.\n");
               }
               
            }
         } // if / else I saw this register before
      }  // for each dest register
      
      // If I've seen any memory operand, mark the node as having memory operands.
      // This means that I will try to match all the ADDR_REGISTER_TYPE dependencies.
      // Mark also if we required exclusive use of the output dependency for this node 
      // (not counting control dependencies).
      if ((*git)->ExclusiveOutputMatch())
         node->setExclusiveOutputMatch();
      if (memSrcIdx>0 || memDestIdx>0)  // instruction has memory operand
         node->setHasMemoryOperand();
   }
   
   // We must have only one destination register in the end
   if (destRegs != 1)
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: File %s (%d, %d): Source pattern has %d destination registers, instead of one as it is required.\n",
           haveErrors, patternPos.FileName(), patternPos.Line(), patternPos.Column(), destRegs);
   }
   // we should also check that each node except one (the last one) has one outgoing
   // dependency
   int zero_outs = 0;
   int more_outs = 0;
   int zero_ins = 0;
   int more_ins = 0;
   int numNodes = 0;
   DGraph::NodesIterator nit(*this);
   while((bool)nit)
   {
      numNodes += 1;
      if (nit->num_outgoing() == 0)
         ++ zero_outs;
      else if (nit->num_outgoing() > 1)
         ++ more_outs;

      if (nit->num_incoming() == 0)
      {
         ++ zero_ins;
         cfg_top = dynamic_cast<Node*>((DGraph::Node*)nit);
      }
      else if (nit->num_incoming() > 1)
         ++ more_ins;
         
      ++ nit;
   }
#if DEBUG_MACHINE_CONSTRUCTION
   DEBUG_MACHINE (2,
      fprintf(stderr, "Pattern source has %d nodes: zeroIn=%d, moreIn=%d, zeroOut=%d, moreOut=%d\n",
              numNodes, zero_ins, more_ins, zero_outs, more_outs);
   )
#endif
   if (zero_outs != 1)
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: File %s (%d, %d): Source pattern has %d nodes with no outgoing dependencies instead of one (the last node).\n",
            haveErrors, patternPos.FileName(), patternPos.Line(), patternPos.Column(), zero_outs);
   }
   if (more_outs != 0)
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: File %s (%d, %d): Source pattern has %d nodes with more than one outgoing dependencies. A node in the source pattern must have 0 or 1 dependencies.\n",
            haveErrors, patternPos.FileName(), patternPos.Line(), patternPos.Column(), more_outs);
   }
   if (zero_ins != 1)
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: Found %d nodes with no incoming dependencies instead of one (the first node).\n",
            haveErrors, zero_ins);
   }
   if (more_ins != 0)
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: Found %d nodes with more than one incoming dependencies. A node in the source pattern must have 0 or 1 incoming edges.\n",
            haveErrors, more_ins);
   }
   // if there are no errors, we also identified the entry node
}


PatternGraph::~PatternGraph()
{
   // I have to delete all nodes and all edges
   DGraph::NodesIterator nit(*this);
   while((bool)nit)
   {
      DGraph::OutgoingEdgesIterator oeit(nit);
      while((bool)oeit)
      {
         DGraph::Edge* tempE = oeit;
         ++ oeit;
         delete (tempE);
      }
      DGraph::Node* tempN = nit;
      ++ nit;
      delete (tempN);
   }
   node_set.clear();
   edge_set.clear();
}


//-----------------------------------------------------------------------------
PatternGraph::Edge::Edge (DGraph::Node* n1, DGraph::Node* n2, int _ddirection, 
     int _dtype) : DGraph::Edge(n1, n2), ddirection(_ddirection), dtype(_dtype)
{
   id = nextId++;
   actualE = NULL;
}
//-----------------------------------------------------------------------------
PatternGraph::Edge*
PatternGraph::Node::FirstSuccessor ()
{
   if (num_outgoing()==0) return NULL;
   DGraph::OutgoingEdgesIterator oeit(this);
   return (dynamic_cast<Edge*>((DGraph::Edge*)oeit));
}
//-----------------------------------------------------------------------------

PatternGraph::Edge*
PatternGraph::Node::FirstPredecessor ()
{
   if (num_incoming()==0) return NULL;
   DGraph::IncomingEdgesIterator ieit(this);
   return (dynamic_cast<Edge*>((DGraph::Edge*)ieit));
}
//-----------------------------------------------------------------------------
