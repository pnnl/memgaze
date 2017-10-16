/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: private_routine.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Base class for storing routine specific data.
 */

#include <assert.h>
#include <string.h>
#include "private_routine.h"
#include "static_branch_analysis.h"
#include "InstructionDecoder.hpp"
#include "debug_miami.h"

using namespace std;
using namespace MIAMI;

#define DEBUG_BBLs 1

#if DEBUG_BBLs
static const char *debugName = "strerror_r";
#endif

PrivateRoutine::PrivateRoutine(PrivateLoadModule *_img, addrtype _start, usize_t _size, 
          const std::string& _name, addrtype _offset)
{
   start_addr = _start;
   start_offset = _offset;
   size = _size;
   name = _name;
   g = NULL;
   img = _img;
}

PrivateRoutine::~PrivateRoutine()
{
}

void
PrivateRoutine::CfgToDot(const char* suffix, int parts) const
{
   if (g)
   {
      MiamiRIFG mCfg(g);
      TarjanIntervals tarj(mCfg);
      RIFGNodeId root = mCfg.GetRootNode();
      fprintf(stderr, "PrivateRoutine::CfgToDot: tarjan root node for routine %s = %d\n", 
            name.c_str(), root);
      
      g->set_node_levels(root, &tarj, &mCfg, 0);
      tarj.Dump(cerr);
      std::string fname = name;
      if (suffix)
         fname += suffix;
      addrtype offset = 0;
      if (InLoadModule()->RelocationOffset() == 0)
         offset = InLoadModule()->BaseAddr();
      g->draw_CFG(fname.c_str(), offset,
             parts);
   }
}

void
PrivateRoutine::CfgToDot(MiamiRIFG *mCfg, TarjanIntervals *tarj, 
                  const char* suffix, int parts) const
{
   if (g && tarj && mCfg)
   {
      RIFGNodeId root = mCfg->GetRootNode();
      
      g->set_node_levels(root, tarj, mCfg, 0);
      tarj->Dump(cerr);
      std::string fname = name;
      if (suffix)
         fname += suffix;
      addrtype offset = 0;
      if (InLoadModule()->RelocationOffset() == 0)
         offset = InLoadModule()->BaseAddr();
      g->draw_CFG(fname.c_str(), offset, 
             parts);
   }
}

/* Create a call surrogate block for this call
 *  - etype: specifies the edge type
 *  - control_returns: create a fall-thru edge if control returns
 *  - pe: output parameter - store the pointer to the edge towards the CALL_SITE block
 */
int 
PrivateRoutine::AddCallSurrogateBlock(addrtype call_addr, addrtype _end, addrtype target, 
           PrivateCFG::EdgeType etype, bool control_returns, PrivateCFG::Edge** pe)
{
   if (!g)
   {
      fprintf(stderr, "PrivateRoutine::AddCallSurrogateBlock: The control flow graph is not initialized.\n");
      exit(-1);
   }
   // because the block has size zero, I cannot use the usual strategy of checking if
   // the block already exists. As a result, I will keep a separate map for all
   // call sites, from call address to end of call address
   AddrEdgeMap::iterator ait = _calls.find(call_addr);
   if (ait==_calls.end())  // first time I see it
   {
      PrivateCFG::Node *n = g->MakeNode(_end, _end, PrivateCFG::MIAMI_CALL_SITE);
      g->addNode(n);
      n->setTarget(target);
      
      // now search for the block containing the function call and add an edge
      // to this call site block
      BBMap::iterator bit = _blocks.upper_bound(_end-1);
      assert (bit!=_blocks.end() && bit->second->getStartAddress()<_end &&
              bit->second->getEndAddress()==_end);
      PrivateCFG::Edge *ee = g->addEdge (bit->second, n, etype);
      _calls.insert(AddrEdgeMap::value_type(call_addr, AddrEdgePair(_end, ee)));
      
      if (pe)
         *pe = ee;

#if DEBUG_BBLs
      if (name.compare(debugName)==0)
      {  
         fprintf(stderr, " - routine %s, add CallSurrogate block at addr 0x%" PRIxaddr ", _end=0x%" PRIxaddr
                ", target=0x%" PRIxaddr ", etype=%d, control_returns=%d\n", name.c_str(),
                call_addr, _end, target, etype, control_returns);
      }
#endif
      
      // add also a FALLTHRU edge from the call site to the instruction after the call
      // if the control transfer returns (a typical function call)
      if (control_returns)
      {
         bit = _blocks.upper_bound(_end);
         if (bit!=_blocks.end() && bit->second->getStartAddress()==_end)
         {
            g->addEdge (n, bit->second, PrivateCFG::MIAMI_FALLTHRU_EDGE);
         } else   // sink block does not exist yet
         {
            // add the edge request to a separate multimap
            g->addProspectiveEdge (n, _end, PrivateCFG::MIAMI_FALLTHRU_EDGE);
         }
      } else
      {
         // add an edge to the exit node
         g->addUniqueEdge (n, g->CfgExit(), PrivateCFG::MIAMI_CFG_EDGE);
      }
      // I am also adding a return edge from each call site. Some calls may never return
      // 05/17/2012, gmarin: I am adding these edges in the scheduler only where there are
      // unreturned calls, while computing BB and Edge counts
//      g->addUniqueEdge(n, g->CfgExit(), PrivateCFG::MIAMI_RETURN_EDGE);
      
      return (1);
   } else
   {
      if (pe)  // I must find the existing edge
      {
         *pe = ait->second.second;
      }
      return (0);
   }
}

int
PrivateRoutine::AddBlock(addrtype _start, addrtype _end,  PrivateCFG::NodeType nodeType, int* inEdges)
{
   if (!g)
   {
      fprintf(stderr, "PrivateRoutine::AddBlock: The control flow graph is not initialized.\n");
      exit(-1);
   }
   
   int retval = -1;
   assert (_end > _start);  // I should not include empty or illformed blocks
   BBMap::iterator mits = _blocks.upper_bound(_start);
   BBMap::iterator mite;  // = _blocks.upper_bound(_end-1);
   int incEdges = -1;
#if DEBUG_BBLs
   if (name.compare(debugName)==0)
   {
      fprintf(stderr, "==> Routine %s, add block [%p,%p]\n", name.c_str(),
             (void*)_start, (void*)_end);
   }
#endif
   if (mits == _blocks.end() || _end<=mits->second->getStartAddress())  // must create a new block
   {
      PrivateCFG::Node *n = g->MakeNode(_start, _end, nodeType);
      incEdges = g->addNode(n);
      assert(n);
      _blocks.insert(BBMap::value_type(_end, n));
      retval = CREATE_NEW_BBL;
   } else if (mits->second->getStartAddress()==_start && mits->second->getEndAddress()==_end)   // exact match, aleluia
   {
      retval = IDENTICAL_BBL;
   } else
   {
      addrtype bStart = mits->second->getStartAddress();
      // we dealt with the common (hopefully), clean cases up to now. That is, 
      // create a completely new block for this BBL, or it perfectly overlaps an existing BBL.
      // Next situation would require splitting either existing BBLs, or the current 
      // (_start,_end) range into multiple BBLs.
      if (_start<bStart)
      {
         // _start is not part of the upper_bound block, and it is not part
         // of any known block.
         // we know that old BBL's start address is < current _end based on a previous test, 
         // but its start address is > than current start addr.
         // Split current BBL in two. 
         PrivateCFG::Node *n = g->MakeNode(_start, bStart, nodeType);
         incEdges = g->addNode(n);
         assert(n);
         _blocks.insert(BBMap::value_type(bStart, n));
         // We must create a fall-through edge between the new and the existing blocks
         g->addUniqueEdge (n, mits->second, PrivateCFG::MIAMI_FALLTHRU_EDGE);
         retval = EXTENDS_BEFORE_OLD_BBL;
      } else if (_start==bStart)
         retval = EXTENDS_BEFORE_OLD_BBL | ENTERS_OLD_BBL;
      else   // _start > mits->second.start
      {
         // The SplitAt method creates a new block from the split point to the old end,
         // and adjusts the preexisting block's end to be the splitting point. 
         // It also creates a fallthru edge between the two blocks and returns the new block
         PrivateCFG::Node *newn = mits->second->SplitAt(_start, &incEdges, nodeType);
         assert(mits->second);
         assert(newn);
         _blocks.insert(BBMap::value_type(_start, mits->second));
         mits->second = newn;
         retval = ENTERS_OLD_BBL;
      }
         
      // Check if we need to split the old BBL as well.
      {
         mite = mits;
         PrivateCFG::Node *prevBB = 0;
         addrtype bStart = mite->second->getStartAddress(),
                  bEnd   = mite->second->getEndAddress();
         addrtype prevEnd = bStart;
#if DEBUG_BBLs
         if (name.compare(debugName)==0)
         {
            fprintf(stderr, "Iterating over bStart=0x%" PRIxaddr ", bEnd=0x%" PRIxaddr ", prevEnd=0x%" PRIxaddr ".\n",
                     bStart, bEnd, prevEnd);
         }
#endif
         while (mite!=_blocks.end() && bEnd<_end)
         {
            retval |= COVER_MULTIPLE_BBLS;
            prevEnd = bEnd;
            prevBB = mite->second;
            ++mite;
            if (mite!=_blocks.end()) 
            {
               bStart = mite->second->getStartAddress();
               bEnd   = mite->second->getEndAddress();
#if DEBUG_BBLs
               if (name.compare(debugName)==0)
               {
                  fprintf(stderr, "Iterating over bStart=0x%" PRIxaddr ", bEnd=0x%" PRIxaddr ", prevEnd=0x%" PRIxaddr ".\n",
                      bStart, bEnd, prevEnd);
               }
#endif
                  
               if (prevEnd<bStart)  // there is a gap between nodes
               {
                  addrtype endval = bStart;
                  if (_end<=bStart) 
                  {
                     assert(prevEnd<_end);
                     endval = _end;
                  }
                  PrivateCFG::Node *n = g->MakeNode(prevEnd, endval, nodeType);
                  g->addNode(n);
                  assert(n);
#if DEBUG_BBLs
                  if (name.compare(debugName)==0)
                  {
                     fprintf(stderr, "Creating node from 0x%" PRIxaddr " to 0x%" PRIxaddr ", bStart = 0x%" PRIxaddr ".\n",
                           prevEnd, endval, bStart);
                  }
#endif
                  std::pair<BBMap::iterator, bool> res = 
                        _blocks.insert(BBMap::value_type(endval, n));
                  // We must create a fall-through edge between the new and the existing blocks
                  if (prevBB)
                     g->addUniqueEdge (prevBB, n, PrivateCFG::MIAMI_FALLTHRU_EDGE);
                  if (_end>bStart)
                     g->addEdge (n, mite->second, PrivateCFG::MIAMI_FALLTHRU_EDGE);
                  else
                  {
                     mite = res.first;
                     bEnd = endval;
                     break;
                  }
               }
               else if (prevEnd>bStart)
                  assert ("Existing blocks overlap. It should not happen, right?");
            }
         }

         // now, we either reached the end of the map or we found a block which extends
         // past the current _end (could be equal - it better be)
         if (mite==_blocks.end())  // new block extends farther out
         {
            PrivateCFG::Node *n = g->MakeNode(prevEnd, _end, nodeType);
            g->addNode(n);
            assert(n);
            _blocks.insert(BBMap::value_type(_end, n));
            // We must create a fall-through edge between the new and the existing blocks
            if (prevBB)
               g->addUniqueEdge (prevBB, n, PrivateCFG::MIAMI_FALLTHRU_EDGE);
            retval |= EXPANDS_OVER_OLD_BBL;
         } else if (_end<bEnd)  // dang it, another split
         {
            int dummy = 0;
#if DEBUG_BBLs
            if (name.compare(debugName)==0)
            {
               fprintf(stderr, "Splitting node [0x%" PRIxaddr ",0x%" PRIxaddr "] at 0x%" PRIxaddr "\n", 
                       mite->second->getStartAddress(), mite->second->getEndAddress(), _end);
            }
#endif
            PrivateCFG::Node *newn = mite->second->SplitAt(_end, &dummy);
            assert(mite->second);
            assert(newn);
            _blocks.insert(BBMap::value_type(_end, mite->second));
            mite->second = newn;
            retval |= EXITS_OLD_BBL;
         } else   // _end == mite->second.end
         {
            retval |= (EXPANDS_OVER_OLD_BBL | EXITS_OLD_BBL);
         }
      }
   }   // not NEW and not IDENTICAL BBL
   
   if (inEdges)
      *inEdges = incEdges;
   
#if DEBUG_BBLs
   if (name.compare(debugName)==0)
   {
      fprintf(stderr, "<== Routine %s, add block [%p,%p], result=%d, incEdges=%d\n",
             name.c_str(), (void*)_start, (void*)_end,
             retval, incEdges);
   }
#endif
   return (retval);
}

PrivateCFG::Edge* 
PrivateRoutine::AddPotentialEdge(addrtype end_addr, addrtype target, PrivateCFG::EdgeType edgeType)
{
   // end_addr is the end address of the source block
   // target is the start address of the sink block
   // source block should already exist. If sink block exists as well, add the edge right away.
   // If sink block does not exist, add the edge as pending in a separate data structure.
   // When we create a new node, we must check if there is any incoming edge for it. 
   // We do not create new blocks that often, thus this is not a big overhead. Trying to recreate 
   // an existing edge adds a larger overhead I think.
   BBMap::iterator srcit = _blocks.upper_bound(end_addr-1);
   BBMap::iterator sinkit = _blocks.upper_bound(target);
   PrivateCFG::Edge *ee = NULL;
   if (srcit==_blocks.end() || srcit->second->getEndAddress()!=end_addr)
   {
      // I should always find the source block
      fprintf (stderr, "In PrivateRoutine::AddPotentialEdge, source block does not exist. end_addr=%p, target=%p, edgeType=%d\n",
           (void*)end_addr, (void*)target, edgeType);
      assert(!"Source block does not exist");
   }
#if DEBUG_BBLs
   if (name.compare(debugName)==0)
   {
      fprintf(stderr, "AddPotentialEdge: routine %s, from end PC 0x%" PRIxaddr " to target 0x%" PRIxaddr
             ", of type=%d\n",
             Name().c_str(), end_addr, target, edgeType);
   }
#endif
   if (sinkit!=_blocks.end() && sinkit->second->getStartAddress()==target)
   {
      // we have both the source and the sink nodes
      // just add the edge
      ee = g->addUniqueEdge (srcit->second, sinkit->second, edgeType);
   } else   // sink block does not exist yet
   {
      // add the edge request to a separate multimap
      ee = g->addProspectiveEdge (srcit->second, target, edgeType);
   }
   
   return (ee);
}

PrivateCFG::Edge* 
PrivateRoutine::AddReturnEdge(addrtype end_addr, PrivateCFG::EdgeType etype)
{
   // end_addr is the end address of the source block
   // target is the start address of the sink block
   // source block should already exist. If sink block exists as well, add the edge right away.
   // If sink block does not exist, add the edge as pending in a separate data structure.
   // When we create a new node, we must check if there is any incoming edge for it. 
   // We do not create new blocks that often, thus this is not a big overhead. Trying to recreate 
   // an existing edge adds a larger overhead I think.
   BBMap::iterator srcit = _blocks.upper_bound(end_addr-1);
   if (srcit == _blocks.end())
   {
      // I should always find the source block
      fprintf (stderr, "In PrivateRoutine::AddReturnEdge, source block does not exist. end_addr=%p\n",
           (void*)end_addr);
      assert(!"Source block does not exist");
   }
   return (g->addUniqueEdge (srcit->second, g->CfgExit(), etype));
}

PrivateCFG::Node*
PrivateRoutine::FindCFGNodeAtAddress(addrtype pc)
{
   if (! _blocks.empty())
   {
      BBMap::iterator bbit = _blocks.upper_bound(pc);
      if  (bbit!=_blocks.end() && pc>=bbit->second->getStartAddress()
                && pc<bbit->second->getEndAddress()
          )
         return (bbit->second);
   } else if (g)
      return (g->findNodeContainingAddress(pc));
   return (0);
}

// return true if we may fall-through from a call-site to the PC
// This includes cases when we can certainly identify such cases, and
// cases when we do not have the CFG for the previous address built.
// @strict - if unset, return true also if the block at PC does not have
//           any incoming control flow edges (MIAMI_CFG_EDGEs excluded)
bool 
PrivateRoutine::MayFollowCallSite(addrtype pc, bool strict)
{
   // if this is the routine start, then it cannot follow a function call, duh
   if (pc==start_addr && strict)
      return (false);
   // First check for a fall-thru edge incoming into this block;
   PrivateCFG::Node *crtb = FindCFGNodeAtAddress(pc);
   // if we cannot find even a block for current pc, then return true; anything is possible
   if (!crtb) return (true);
   
   // Find if we have any FALL_THRU edge incomming into this block
   PrivateCFG::Edge *fall_e = crtb->firstIncomingOfType(PrivateCFG::MIAMI_FALLTHRU_EDGE);
   if (fall_e)  // we found one, now we know for sure what we have
   {
      PrivateCFG::Node *pred_b = fall_e->source();
      return (pred_b->isCallSurrogate());
   } else  // we do not have a fall-thru edge yet 
   {
      if (! strict)  // check if it has any incoming edges
      {
         bool no_edges = true;
         PrivateCFG::IncomingEdgesIterator ieit(crtb);
         while ((bool)ieit && no_edges)
         {
            PrivateCFG::Edge *ee = ieit;
            if (ee->getType() != PrivateCFG::MIAMI_CFG_EDGE)
               no_edges = false;
            ++ ieit;
         }
         if (no_edges)
            return (true);
      }
      // maybe we have a block at pc-1, but it ends with an unconditional jump
      PrivateCFG::Node *pred_b = FindCFGNodeAtAddress(pc-1);
      if (! pred_b) return (true); // nothing yet, must be conservative
      
      // otherwise it should mean that we cannot have a call, or we would have found the
      // fall-thru edge and the call surrogate block previously. Sanity check
      PrivateCFG::Edge *fall_out = pred_b->firstOutgoingOfType(PrivateCFG::MIAMI_FALLTHRU_EDGE);
      if (fall_out)  // this is odd, we should not have found one
      {
         // print more information
         PrivateCFG::Node *sinkb = fall_out->sink();
         cerr << "PrivateRoutine::MayFollowCallSite: Strange case: Routine " << name
              << " [0x" << hex << start_addr << ",0x" << start_addr+size << "], pc=0x"
              << pc << ", crtb=[0x" << crtb->getStartAddress() << ",0x"
              << crtb->getEndAddress() << "], has no incoming fall thru edge. But pred_b=[0x"  
              << pred_b->getStartAddress() << ",0x" << pred_b->getEndAddress() 
              << "] has an outgoing fall thru edge {" << dec << *fall_out << hex
              << "} to block at [0x" << sinkb->getStartAddress() << ",0x" 
              << sinkb->getEndAddress() << "]." << endl;
         cerr << "All incoming edges of crtb:" << endl;
         PrivateCFG::IncomingEdgesIterator iit(crtb);
         while ((bool) iit)
         {
            PrivateCFG::Edge *e = iit;
            PrivateCFG::Node *auxb = e->source();
            cerr << "  - edge {" << dec << *e << "} of type " << e->edgeTypeToString(e->getType())
                 << " from [0x" << hex << auxb->getStartAddress() << ",0x" << auxb->getEndAddress()
                 << "]." << endl;
            ++ iit;
         }
         cerr << "All outgoing edges of pred_b:" << endl;
         PrivateCFG::OutgoingEdgesIterator oit(pred_b);
         while ((bool) oit)
         {
            PrivateCFG::Edge *e = oit;
            PrivateCFG::Node *auxb = e->sink();
            cerr << "  - edge {" << dec << *e << "} of type " << e->edgeTypeToString(e->getType())
                 << " to [0x" << hex << auxb->getStartAddress() << ",0x" << auxb->getEndAddress()
                 << "]." << endl;
            ++ oit;
         }
         cerr << dec;
         CfgToDot("_fallout");
         assert(!fall_out);  // we shouldn't find a fall-thru edge here if we did not find one before
      }
      return (fall_out && fall_out->sink()->isCallSurrogate());
   }
}

int 
PrivateRoutine::discover_CFG(addrtype pc)
{
   cout<<"discover_CFG: "<<(unsigned int*)pc<<" "<<(unsigned int*)Start()<<" "<<(unsigned int*)End()<<endl;
   if (pc<Start() || pc>=End())
   {
#if DEBUG_BBLs
      if (name.compare(debugName)==0)
      {
         fprintf(stderr, "==> Routine %s, asked to static check for block at address 0x%" PRIxaddr " which is outside the routine bounds.\n", 
                name.c_str(), pc);
      }
#endif
      return (-1);
   }
   // check if we have a block created for pc already
   BBMap::iterator mits = _blocks.upper_bound(pc);
   BBMap::iterator mite;  // = _blocks.upper_bound(_end-1);
#if DEBUG_BBLs
   if (name.compare(debugName)==0)
   {
      fprintf(stderr, "==> Routine %s, static check for block at address 0x%" PRIxaddr "\n", 
             name.c_str(), pc);
   }
#endif
   if (mits == _blocks.end() || pc<mits->second->getStartAddress())  // must create a new block
   // did not find block, must decode instructions
   {
      addrtype saddr = pc, eaddr;  // how far I have to decode
      if (mits == _blocks.end())
         eaddr = End();  // up to the end of the routine
      else
         eaddr = mits->second->getStartAddress();  // up to the following known block
      
      int res = 0, res2 = 0;
      int len = 0;    // get the instruction length in bytes
      addrtype reloc = InLoadModule()->RelocationOffset();
      while (pc<eaddr && !res && !res2)
      {
         cout<<name<<" pc: "<<(unsigned int*)pc<<" "<<(unsigned int*)reloc<<" "<<(unsigned int*)(pc+reloc)<<" "<<(unsigned int*)saddr<<" "<<(unsigned int*)eaddr<<endl;
         // check if instruction is a branch
         res = instruction_at_pc_transfers_control((void*)(pc+reloc), eaddr-pc, len);
         //dump_instruction_at_pc((void*)(pc+reloc),15);
         
         if (res<0)  // error while decoding
         {
            fprintf(stderr, " - ERROR: Bad decoding (%d) while discovering CFG for routine %s, at address 0x%" PRIxaddr 
                    " (+reloc=0x%" PRIxaddr "), eaddr-pc=%" PRIaddr ", len=%d\n", 
                    res, name.c_str(), pc, pc+reloc, eaddr-pc, len);
            // Should I do anything on a bad instruction? I must wait to see 
            // some concrete cases before making a decision.
            return (res);
         }
         
         // check if instruction has REP prefix
         if (!res)
            res2 = instruction_at_pc_stutters((void*)(pc+reloc), len, len);
         pc += len;
      }
      cout<<name<<" pc: "<<(unsigned int*)pc<<" "<<(unsigned int*)saddr<<" "<<(unsigned int*)eaddr<<" "<<res<<" "<<res2<<endl;
      
#if DEBUG_BBLs
      if (name.compare(debugName)==0)
      {
         fprintf(stderr, "--> Routine %s, static check from address 0x%" PRIxaddr " ended at address 0x%"
              PRIxaddr ", res=%d, res2=%d\n", name.c_str(), saddr, pc, res, res2);
      }
#endif
      // we exit the loop when either we reached eaddr, or we found a transfer control instruction
      // which we consider to terminate the basic block
      if (0 && pc>eaddr)
      {
         // This case is actually possible when an instruction has a prefix, but there is
         // a path that jumps to the instruction itself, while another path falls through
         // the prefix.
         // Set an assert to catch if it happens in practice, but once verified, we can
         // remove the assert.
         // Found this case with a lock prefix. 
         fprintf(stderr, "ERROR: Instruction decoding of bounded binary code stopped at address 0x%" PRIxaddr
                 " which is beyond the earlier detected end address 0x%" PRIxaddr ".\n", pc, eaddr);
         assert (pc <= eaddr);
      }
      assert (pc > saddr);  // we must have advanced at least one instruction
      addrtype lpc = pc - len;  // PC address of last instruction
      bool create_fallthru_edge = true, discover_fallthru_code = false;
      assert (res || res2 || pc>=eaddr);
      
      // I must create a block from saddr to pc
      // If last instruction is a REP prefixed instruction, then create a separate
      // block for it. I will need to create another block for instructions up to
      // the REP instruction in this case.
      if (res2)  // it was a rep instruction
      {
         if (lpc > saddr)  // non-empty block before the REP instruction
         {
            AddBlock(saddr, lpc, PrivateCFG::MIAMI_CODE_BLOCK, 0);
            // connect the two blocks with a fall-through edge
            AddPotentialEdge(lpc, lpc, PrivateCFG::MIAMI_FALLTHRU_EDGE);
         }
         
         // A REP prefixed instruction should take more than 1 byte
         // The REP prefix is 1 byte, + the length of the actual instruction
         AddBlock(lpc, pc, PrivateCFG::MIAMI_REP_BLOCK, 0);
//         AddPotentialEdge(lpc+1, lpc+1, PrivateCFG::MIAMI_FALLTHRU_EDGE);
//         AddBlock(lpc+1, pc, PrivateCFG::MIAMI_CODE_BLOCK, 0);
         
         // add a loop edge for the REP instruction
         AddPotentialEdge(pc, lpc, PrivateCFG::MIAMI_DIRECT_BRANCH_EDGE);
         // I also need to create a bypass edge to go around the REP instruction
         // in case that its count is set to zero.
         AddPotentialEdge(lpc, pc, PrivateCFG::MIAMI_BYPASS_EDGE);
      } else  // no REP instruction found, add a single block
         AddBlock(saddr, pc, PrivateCFG::MIAMI_CODE_BLOCK, 0);
      
      if (res)  // last decoded instruction was a transfer control instruction
      {
         assert (!res2);  // should not find a REP prefixed branch
         
         MIAMI::DecodedInstruction dInst;
         addrtype target = 0;
         res = decode_instruction_at_pc((void*)(lpc+reloc), len, &dInst);
         assert (res>=0); // I just decoded this instruction earlier
         
         // I need to determine what type of branch instruction it is, direct or indirect
         // and if we can compute the branch target
         GFSliceVal targetF(sliceVal (0, TY_CONSTANT));
         
         InstrList::iterator lit = dInst.micro_ops.begin();
         InstrBin branch_type = IB_INVALID;
         for ( ; lit!=dInst.micro_ops.end() ; ++lit)
         {
            instruction_info *ii = &(*lit);
            if (ii->type==IB_br_CC || ii->type==IB_branch || ii->type==IB_jump || ii->type==IB_ret)
            {
               branch_type = ii->type;
               
               res = ComputeBranchTargetFormula(&dInst, ii, -1, dInst.pc, targetF); 
               coeff_t valueNum;
               ucoeff_t valueDen;
               if (!res && IsConstantFormula(targetF, valueNum, valueDen) && valueDen==1)
                  target = valueNum;
               break;
            }
         }
         if (branch_type==IB_branch || branch_type==IB_ret)
            // unconditonal branches and returns have no fall-through
            create_fallthru_edge = false;
         
         // If I have the target address, discover the CFG starting from the target
         PrivateCFG::EdgeType edge_type = PrivateCFG::MIAMI_DIRECT_BRANCH_EDGE;
         if (branch_type==IB_jump)
         {
            edge_type = PrivateCFG::MIAMI_FALLTHRU_EDGE;
            create_fallthru_edge = false;  // do not create fall-thru here, 
            // it will be created by AddCallSurrogate from the call-site block
            discover_fallthru_code = true;  // discover fall-thru code still
         }
         else if (branch_type==IB_ret)
            edge_type = PrivateCFG::MIAMI_RETURN_EDGE;
         
         if (branch_type == IB_jump)  // indirect function call
         {
            // add an unresolved call surrogate
            AddCallSurrogateBlock(lpc, pc, target, edge_type, true);
            // AddCallSurrogateBlock function creates the fall-through edge
            create_fallthru_edge = false;  // do not create it again, or it will be missplaced
            // but I still need to attempt code discovery from the instruction 
            // following the call
            discover_fallthru_code = true;
         }
         else if (branch_type == IB_ret)
            AddReturnEdge(pc);

         cout <<"0 -- t: "<<(unsigned int*)(target-reloc)<<" cfte: "<<create_fallthru_edge<<" dftc: "<<discover_fallthru_code<<endl;
         if (! target)  // target could not be resolved
         {
            // write something about it
#if DEBUG_INDIRECT_TRANSFER
            DEBUG_INDIRECT(3,
               fprintf(stderr, "STATIC CFG Discovery for routine %s, PC 0x%" PRIxaddr " (0x%" PRIxaddr ") is branch of type %d (%s) with unresolved target 0x%" PRIxaddr 
                         ", some sort of INDIRECT control transfer. ",
                        Name().c_str(), lpc-InLoadModule()->BaseAddr(), lpc, 
                        branch_type, Convert_InstrBin_to_string(branch_type), 
                        target);
               cerr << "DecodedFormula: " << targetF << endl;
            )
#endif
            
            // do not attempt to resolve Returns
            if (branch_type!=IB_ret && branch_type!=IB_jump)
            {
               // Find the block that contains the indirect jump
               // if _blocks is not empty, then we must be building the CFG 
               // as we speak; use the _blocks map to find the block.
               // Otherwise, it may mean the CFG is completed (why are we trying 
               // to resolve indirect jumps??), so use the provided findBlock method
               PrivateCFG::Node *brB = FindCFGNodeAtAddress(lpc);
               if (! brB)
               {
                  // we do not have a block for lpc, cry me a river
                  cerr << "ERROR: Cannot find a block that includes control transfer instruction at 0x"
                       << hex << lpc << dec << ". Cannot resolve indirect target." << endl;
               } else
               {
#if DEBUG_INDIRECT_TRANSFER
                  DEBUG_INDIRECT(1,
                     cerr << endl << "Indirect " << Convert_InstrBin_to_string(branch_type)
                          << " found at pc 0x" << hex << lpc << dec
                          << " during static CFG discovery for routine "
                          << Name() << ". Attempting to resolve it." << endl;
                  )
#endif
                  AddrSet targets;
                  ResolveBranchTargetFormula(targetF, g, brB, lpc, (InstrBin)branch_type, targets);
                  if (! targets.empty())  // uhu, we found some targets
                  {
                     AddrSet::iterator ait;
#if DEBUG_INDIRECT_TRANSFER
                     DEBUG_INDIRECT(1,
                        cerr << "INDIRECT " << Convert_InstrBin_to_string(branch_type)
                             << " at 0x" << hex << lpc 
                             << " resolves to the following possible targets:";
                        for (ait=targets.begin() ; ait!=targets.end() ; ++ait)
                           cerr << " 0x" << *ait;
                        cerr << dec << endl;
                     )
#endif
                  
                     for (ait=targets.begin() ; ait!=targets.end() ; ++ait)
                     {
                        AddPotentialEdge(pc, *ait, PrivateCFG::MIAMI_INDIRECT_BRANCH_EDGE);
                        discover_CFG(*ait);
                     }
                  }
               }
            }
            // this must be an indirect jump
            // branch must be instrumented to find address at run-time;
            // hmm, it can leave holes in my precious CFG
         } else
//         if (target && branch_type!=IB_jump)
         {
            
            // check if it is an intra-procedural branch
            // gmarin, 2013/10/16: Apparently, pin provides bad routine bounds,
            // I don't know why. Create call surrogate only on true calls.
            // Otherwise, create a branch, and keep decoding.

            // Add counter edge from this block to the newly discovered block
            if (branch_type!=IB_jump && branch_type!=IB_ret)
            {
               AddPotentialEdge(pc, target, PrivateCFG::MIAMI_DIRECT_BRANCH_EDGE);
            }
               
            discover_CFG(target);
            if (branch_type!=IB_jump && branch_type!=IB_ret)
            {
               AddPotentialEdge(pc, target-reloc, PrivateCFG::MIAMI_DIRECT_BRANCH_EDGE);
            }
               
            discover_CFG(target-reloc);
         } 

         // if instruction has fall-through execution, create also a fal-through edge
         // only unconditional branches do not have fall-through
         // In PIN, calls are considered not to have fall-through, however, I create a
         // fall-through edge, but these edges are not profilable, bummer
#if DEBUG_BBLs
         if (name.compare(debugName)==0)
         {
            fprintf(stderr, "STATIC Routine %s, PC 0x%" PRIxaddr " is branch of type %d (%s) with target 0x%" PRIxaddr 
                      ", and fallthrough=%d\n",
                     Name().c_str(), lpc, branch_type, Convert_InstrBin_to_string((InstrBin)branch_type), 
                     target, create_fallthru_edge);
         }
#endif
      }
      
      if (create_fallthru_edge)
      {
         // create a fall-through edge to the next block.
         // What if the end address was the end of the routine? Should I create
         // a fall-through edge to the exit node, even if no return?
         if (pc < End())
         {
            AddPotentialEdge(pc, pc, PrivateCFG::MIAMI_FALLTHRU_EDGE);
         } else  // we are at the end of the routine and we have a fall-through
            // create a fall-through edge to the Exit node
            AddReturnEdge(pc, PrivateCFG::MIAMI_FALLTHRU_EDGE);
      }
      cout <<"1 -- pc:"<<(unsigned int*)pc <<" cfte: "<<create_fallthru_edge<<" dftc: "<<discover_fallthru_code<<endl;

      if ((create_fallthru_edge || discover_fallthru_code) && pc<End())
      {
         discover_CFG(pc);
      }
   } else if (pc>mits->second->getStartAddress())
   {
      cout <<" here?"<<endl;
      // check if we enter the middle of a block; I must split the block
      AddBlock(pc, mits->second->getEndAddress(), PrivateCFG::MIAMI_CODE_BLOCK, 0);
      
      // If I created a code cache for this block, I need to separate it for the
      // two new blocks
   }
   return (0);         
}
