/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: DGBuilder.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the logic for building the dependence graph of
 * a path. It creates register, memory and control dependencies within the
 * same iteration. If the path takes the back edge of a loop, it also 
 * computes loop carried dependencies between micro-operations.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <list>

#include "DGBuilder.h"
#include "debug_scheduler.h"

#include "instr_info.H"
#include "InstructionDecoder.hpp"

//#include "dyninst-insn-xlate.hpp"


using namespace MIAMI;

namespace MIAMI_DG
{
DecodedInstruction* defDInstP = 0;
typedef std::list<DecodedInstruction*> DIList;

static int deleteInstsInMap(void *arg0, addrtype pc, void *value)
{
   DecodedInstruction *di = static_cast<DecodedInstruction*>((void*)(*(addrtype*)value));
   delete (di);
   return (0);
}

#define DECODE_INSTRUCTIONS_IN_PATH 0
static PathID targetPath(0xe7367c, 37, 2);

DGBuilder::DGBuilder(const char* func_name, PathID _pathId,
		     int _opt_mem_dep, RFormulasMap& _refAddr,
		     LoadModule *_img,
		     int numBlocks, CFG::Node** ba, float* fa,
		     RSIList* innerRegs,
		     uint64_t _pathFreq, float _avgNumIters)
  : SchedDG(func_name, _pathId, _pathFreq, _avgNumIters, _refAddr, _img),
    optimistic_memory_dep(_opt_mem_dep)
{
   assert( (numBlocks>=1) || 
           !"Number of blocks to schedule is < 1. What's up?");
#if DEBUG_GRAPH_CONSTRUCTION
   DEBUG_GRAPH (1,
      fprintf(stderr, "DGBuilder create path with %f average iterations\n", 
          avgNumIters);
   )
#endif
   
   // we do not read this flag right now.
   // Assume we do not have pessimistic memory dependencies.
   pessimistic_memory_dep = 0;
   num_instructions = 0;
   
   nextGpReg = 1;
   nextInReg = 1;
   nextUopIndex = 1;
   minPathAddress = ba[0]->getStartAddress();
   maxPathAddress = ba[0]->getEndAddress();
   // gmarin, 09/11/2013: The first block can be an inner loop, and
   // inner loop nodes do not have an associated CFG, so the reloc_offset
   // is always zero. Can all the blocks of a path be inner loops?
   // Probabaly not, but it is safer to get it from the img object.
   reloc_offset = img->RelocationOffset();  //ba[0]->RelocationOffset();
   
   for( int i=1 ; i<numBlocks ; ++i )
   {
      if (minPathAddress > ba[i]->getStartAddress())
         minPathAddress = ba[i]->getStartAddress();
      if (maxPathAddress < ba[i]->getEndAddress())
         maxPathAddress = ba[i]->getEndAddress();
   }
   prev_inst_may_have_delay_slot = false;
   instruction_has_stack_operation = false;
   gpRegs = &gpMapper;
//   fpRegs = &fpMapper;
   prevBranch = NULL;
   prevBarrier = lastBarrier = NULL;
   lastBranchBB = NULL;
   lastBranchProb = 0;
   lastBranchId = 0;
   // find how many register stacks are on this architecture.
   // I have to maintain top markers for each one.
   numRegStacks = number_of_register_stacks();
   if (numRegStacks>0)
   {
      crt_stack_tops = new int[numRegStacks];
      prev_stack_tops = new int[numRegStacks];
      for (int i=0 ; i<numRegStacks ; ++i) {
         crt_stack_tops[i] = prev_stack_tops[i] = max_stack_top_value(i);
      }
      stack_tops = crt_stack_tops;
   } else {
      stack_tops = crt_stack_tops = prev_stack_tops = 0;
   }
   
   build_graph(numBlocks, ba, fa, innerRegs);
   
   // in loops with many indirect accesses we see an explosion of unnecessary 
   // memory dependencies. Write a method that traverses only memory 
   // instructions, along memory dependencies, and removes any trivial deps.
//   if (avgNumIters > ONE_ITERATION_EPSILON) 
   pruneTrivialMemoryDependencies();
   
   finalizeGraphConstruction();
   
   setCfgFlags(CFG_CONSTRUCTED);
   
   // delete the decoded instructions stored in builtNodes
   builtNodes.map(deleteInstsInMap, NULL);
}

// TODO: Palm version!
DGBuilder::DGBuilder(Routine* _routine, PathID _pathId,
		     int _opt_mem_dep, RFormulasMap& _refAddr,
		     LoadModule *_img,
		     int numBlocks, CFG::Node** ba, float* fa,
		     RSIList* innerRegs,
		     uint64_t _pathFreq, float _avgNumIters, 
                     MIAMI::MemListPerInst * memData, MemDataPerLvlList *mdplList)
  : SchedDG(_routine->Name().c_str(), _pathId, _pathFreq, _avgNumIters,
	    _refAddr, _img),
    optimistic_memory_dep(_opt_mem_dep)
{
   assert( (numBlocks>=1) || 
           !"Number of blocks to schedule is < 1. What's up?");
#if DEBUG_GRAPH_CONSTRUCTION
   DEBUG_GRAPH (1,
      fprintf(stderr, "DGBuilder create path with %f average iterations\n", 
          avgNumIters);
   )
#endif

   routine = _routine;
   
   // we do not read this flag right now.
   // Assume we do not have pessimistic memory dependencies.
   pessimistic_memory_dep = 0;
   num_instructions = 0;
   
   nextGpReg = 1;
   nextInReg = 1;
   nextUopIndex = 1;
   std::cout<<"blkPath: "<<routine->getBlockNoFromAddr(ba[0]->getStartAddress(),ba[0]->getEndAddress())<<":"<<ba[0]->getType();
   minPathAddress = ba[0]->getStartAddress();
   maxPathAddress = ba[0]->getEndAddress();
   // gmarin, 09/11/2013: The first block can be an inner loop, and
   // inner loop nodes do not have an associated CFG, so the reloc_offset
   // is always zero. Can all the blocks of a path be inner loops?
   // Probabaly not, but it is safer to get it from the img object.
   reloc_offset = img->RelocationOffset();  //ba[0]->RelocationOffset();
   
   
   for( int i=1 ; i<numBlocks ; ++i )
   {
      std::cout<<"->"<<routine->getBlockNoFromAddr(ba[i]->getStartAddress(),ba[i]->getEndAddress())<<":"<<ba[i]->getType();
      if (minPathAddress > ba[i]->getStartAddress())
         minPathAddress = ba[i]->getStartAddress();
      if (maxPathAddress < ba[i]->getEndAddress())
         maxPathAddress = ba[i]->getEndAddress();
   }
   std::cout<<std::endl;
   std::cout<<"probs: "<<endl;
   for( int i=1 ; i<numBlocks ; ++i ){
      std::cout<<fa[i]<<endl;
   }
   std::cout<<std::endl;
   std::cout<<"_pathFreq: "<<_pathFreq<<" avgNumIters: "<<_avgNumIters<<endl;
   prev_inst_may_have_delay_slot = false;
   instruction_has_stack_operation = false;
   gpRegs = &gpMapper;
//   fpRegs = &fpMapper;
   prevBranch = NULL;
   prevBarrier = lastBarrier = NULL;
   lastBranchBB = NULL;
   lastBranchProb = 0;
   lastBranchId = 0;
   // find how many register stacks are on this architecture.
   // I have to maintain top markers for each one.
   numRegStacks = number_of_register_stacks();
   if (numRegStacks>0)
   {
      crt_stack_tops = new int[numRegStacks];
      prev_stack_tops = new int[numRegStacks];
      for (int i=0 ; i<numRegStacks ; ++i) {
         crt_stack_tops[i] = prev_stack_tops[i] = max_stack_top_value(i);
      }
      stack_tops = crt_stack_tops;
   } else {
      stack_tops = crt_stack_tops = prev_stack_tops = 0;
   }
   
   build_graph(numBlocks, ba, fa, innerRegs);
   calculateMemoryData(memData, mdplList);
   // in loops with many indirect accesses we see an explosion of unnecessary 
   // memory dependencies. Write a method that traverses only memory 
   // instructions, along memory dependencies, and removes any trivial deps.
//   if (avgNumIters > ONE_ITERATION_EPSILON) 
   pruneTrivialMemoryDependencies();
   
   finalizeGraphConstruction();
   
   setCfgFlags(CFG_CONSTRUCTED);
   
   // delete the decoded instructions stored in builtNodes
   builtNodes.map(deleteInstsInMap, NULL);
}

DGBuilder::~DGBuilder()
{
  if (crt_stack_tops) delete[] crt_stack_tops;
  if (prev_stack_tops) delete[] prev_stack_tops;
  // I need to delete all the decoded instructions stored in builtNodes
  // Perhaps I can delete them even earlier, after the graph is build. 
  // I am not going to use the buildNodes hashMap after that.
  // builtNodes is emptied at the end of the constructor now.
}

//ozgurS
void DGBuilder::calculateMemoryData(MIAMI::MemListPerInst * memData , MIAMI::MemDataPerLvlList * mdplList){
   NodesIterator nit(*this);
   std::vector<MIAMI::InstlvlMap *>::iterator mit=memData->begin();
   while ((bool)nit) {
      Node *nn = nit;
      std::cerr<<"in while inst address: "<<std::hex<<nn->getAddress()<<std::endl;
      if (nn->isInstructionNode()){
         std::cerr<<"1 if is inst node inst address: "<<std::hex<<nn->getAddress()<<std::endl;
         if(nn->is_load_instruction()){
         std::cerr<<"2 if is inst node inst address: "<<std::hex<<nn->getAddress()<<std::endl;
            memData->push_back(nn->getLvlMap()); 
         std::cerr<<"3 if is inst node inst address: "<<std::hex<<nn->getAddress()<<std::endl;
            mit++;  
            std::cerr<<__func__<<std::dec<<" ozgurlets test addr: "<<std::hex<<nn->getAddress()<<" hit at lvl 0 is "<<std::dec<<nn->getLvlMap()->find(0)->second.hitCount<<std::endl; 
         }
      }
      ++nit;
   }
   struct Mem{
      int level;
      int hits;
      int miss;
      float windowSize;
   }; 
   std::vector<Mem> memVector;            //TODO find a better  way to loop for each level
   for (int lvl =0 ; lvl < 10 ; lvl++){   //or just try to ask MAX_LEVEL from user
      Mem mem;
      mem.level = lvl;
      mem.hits = 0;
      mem.miss = 0;
      mem.windowSize = 0.0;
      for (std::vector<MIAMI::InstlvlMap *>::iterator it=memData->begin() ; it!=memData->end() ; it++){  
         mem.hits += (*it)->find(lvl)->second.hitCount;
         for (int nextlvl =lvl+1 ; nextlvl < 10 ; nextlvl++){
            mem.miss += (*it)->find(nextlvl)->second.hitCount;
         }
         std::cerr<<" OZGUR DATA CONTROL : "<<std::dec<< (*it)->find(lvl)->second.hitCount << std::endl;
      }
      if (mem.miss){
         mem.windowSize = (float)mem.hits/ (float)mem.miss;
      } else {
         mem.windowSize = -1.0;
      }
      memVector.push_back(mem);
   }
   for( std::vector<Mem>::iterator dit=memVector.begin() ; dit!=memVector.end() ; dit++){
      MIAMI::MemoryDataPerLevel md = MemoryDataPerLevel((*dit).level, (*dit).hits, (*dit).miss, (*dit).windowSize);
      mdplList->push_back(md);
      std::cerr<<" OZGUR mem data at lvl: "<<std::dec<< (*dit).level<<" hits: "<<(*dit).hits<<" miss: "<<(*dit).miss<<"windowSize: "<<(*dit).windowSize<< std::endl;
   }
}
//OzgurE

void
DGBuilder::build_graph(int numBlocks, CFG::Node** ba, float* fa,
		       RSIList* innerRegs)
{
  std::cerr << "[INFO]DGBuilder::build_graph: '" << name() << "'\n";
  
  int i, numLoops = 0;
  bool lastInsnNOP = false;

  // Find a block with a non NULL CFG pointer. Inner loop nodes are
  // special nodes that are not part of the CFG and they have an
  // uninitialized pointer.
  CFG *g = ba[0]->inCfg();   
  for (i = 1; i < numBlocks && g == NULL; ++i) {
    g = ba[i]->inCfg();
  }

  std::cout<<"hmm 0"<<std::endl;
   
  for(i = 0; i < numBlocks; ++i) {

#if DEBUG_GRAPH_CONSTRUCTION
    DEBUG_GRAPH (3,
		 fprintf(stderr, "Processing block %d with prob %f, from address [%" PRIxaddr ",%" PRIxaddr "]\n",
			 i, fa[i], ba[i]->getStartAddress(), ba[i]->getEndAddress());
		 )
#endif

    addrtype pc;
    if (! ba[i]->is_inner_loop()) {
      if (! ba[i]->isCfgEntry()) {
	CFG::ForwardInstructionIterator iit(ba[i]);
	while ((bool)iit)
	  {
	    pc = iit.Address();
	    inMapper.clear();  // internal registers are valid only across 
	    // micro-ops part of one native instruction
	    build_node_for_instruction(pc, ba[i], fabs(fa[i]));
	    MIAMI::DecodedInstruction* &dInst = builtNodes[pc];
	    MIAMI::instruction_info &primary_uop = dInst->micro_ops.back();
	    if (primary_uop.type == MIAMI::InstrBin::IB_nop){
	      lastInsnNOP = true;
	    }
	    else{
	      lastInsnNOP = false;
	    }
	    std::cout <<"here "<< primary_uop.type <<" " << MIAMI::InstrBin::IB_nop<<std::endl;
	    num_instructions += 1;
	    if (ba[i]->is_delay_block())
	      {
		prev_inst_may_have_delay_slot = false;
		gpRegs = &gpMapper;
		stack_tops = crt_stack_tops;
		//                  fpRegs = &fpMapper;
		break;
	      }
	    ++ iit;
	  }
      }
    }
    else {
      // An inner loop entry. Create a special 'barrier' node.
      int type = IB_inner_loop;  // CFG_LOOP_ENTRY_TYPE;
      pc = numLoops + 1;

#if DEBUG_GRAPH_CONSTRUCTION
      DEBUG_GRAPH (1,
		   fprintf(stderr, " => Found entry into inner loop. Create a loop entry node at %" PRIaddr ".\n",
			   pc);
		   )
#endif

      MIAMI::DecodedInstruction* &dInst = builtNodes[pc];
      assert(dInst == NULL); 
      dInst = new MIAMI::DecodedInstruction();
      dInst->micro_ops.push_back(MIAMI::instruction_info());
      MIAMI::instruction_info &primary_uop = dInst->micro_ops.back();

      primary_uop.type = (MIAMI::InstrBin)type;         
      primary_uop.width = 0;
      primary_uop.vec_len = 1;
      primary_uop.exec_unit = ExecUnit_SCALAR;
      primary_uop.exec_unit_type = ExecUnitType_INT;
      primary_uop.num_src_operands = 0;
      primary_uop.num_dest_operands = 0;

      SchedDG::Node* node = new Node(this, pc, 0, primary_uop);
      node->setInOrderIndex(nextUopIndex++);
//ozgurS
      if (node->is_load_instruction()){
         node->setLvlMap(img->getMemLoadData(node->getAddress()));
         std::cerr<<__func__<<" ozgur test total hit at lvl 0 is "<<node->getLvlMap()->find(0)->second.hitCount<<std::endl; 
         assert(false && "Impossible!!");
      }
//ozgurE
      add(node);
      markBarrierNode (node);
         
      primary_uop.data = node;
         
      // I could also clear the register renaming maps. This will ensure
      // there are no dependencies between instructions before and after
      // the inner loop (dependencies would be covered by the execution 
      // time of the loop anyway). But I choose to keep them for now, to 
      // account for the missing dependencies between instructions in
      // loops at different levels.
      //
      // gmarin: Unfortunately, these dependecies across 
      // inner loops and routine calls, create a lot of problems for the
      // scheduler. I will clear the renaming maps now inside the method
      // "handle_control_dependencies" :(
      // gmarin: Now we set register dependencies into and from
      // an inner loop. We still do not allow register dependencies between
      // instructions before and after an inner loop.
      handle_inner_loop_register_dependencies(node, innerRegs[numLoops], 1);
      handle_control_dependencies(node, type, ba[i], -fa[i]);
      numLoops += 1;
    }
  }

  std::cout<<"hmm 1"<<std::endl;

  // compute the top nodes here. Register carried dependencies are not 
  // taken into account anyway
  // add also control dependencies from the lastBranch to the top nodes
  if (lastBranch!=NULL)
    {
      NodesIterator nit(*this);
      while ((bool)nit) 
	{
	  Node *nn = nit;
	  if (nn->isInstructionNode())
	    {
	      bool is_top = true;
	      IncomingEdgesIterator ieit(nn);
	      while ((bool)ieit)
		{
		  // check if this is a loop independent scheduling edge
		  // for control edges, verify that this is not one that can
		  // be removed by branch predictor.
		  if ( ieit->getLevel()==0 && !ieit->IsRemoved() && 
		       ieit->isSchedulingEdge() && 
                       (ieit->getType()!=CONTROL_TYPE || 
                        ieit->getProbability()<BR_HIGH_PROBABILITY ||
                        nn->isBarrierNode() || 
                        ieit->source()->isBarrierNode()) )
		    {
		      is_top = false;
		      break;
		    }
		  ++ ieit;
		}
	      if (is_top)
		{
		  addUniqueDependency(lastBranch, nn, TRUE_DEPENDENCY,
				      CONTROL_TYPE, 1, 1, 0, lastBranchProb);
		}
	    }
	  ++ nit;
	}
    }

  // If there are any inner loops or function calls in this path, then
  // I should disable SWP. This is what most compilers do.
  // if (hasBarrierNodes())
  //    avgNumIters = 1.0;
  std::cout <<"lastInsnNOP "<<lastInsnNOP<<std::endl;
  if (avgNumIters <= ONE_ITERATION_EPSILON)  // no SWP for this path
    {
      if (lastBranch && !lastBranch->isBarrierNode()) // it was not added before
	{
	  markBarrierNode (lastBranch);
	  // add also control dependencies from all recent branches to this 
	  // barrier
	  NodeSet::iterator nsit = recentBranches.begin();
	  for ( ; nsit!=recentBranches.end() ; ++nsit )
	    {
	      // lastBranch should be in recentBranches, but do not process
	      // it like a regular branch since it is a barrier node now.
	      if (*nsit == lastBranch)
		continue;
	      // if the node does not have any outgoing dependency, create one;
	      // create one even if it has an outgoing control dependency with 
	      // high probability
	      int has_dep = 0;
	      OutgoingEdgesIterator oeit(*nsit);
	      while ((bool)oeit)
		{       
		  if ( oeit->getLevel()==0 && 
		       (oeit->getType()!=CONTROL_TYPE || 
			oeit->getProbability()<BR_HIGH_PROBABILITY ||
			oeit->sink()->isBarrierNode()) )
		    {    
		      has_dep = 1;
		      break;
		    }
		  ++ oeit;
		}
	      if (!has_dep)
		addUniqueDependency(*nsit, lastBranch, TRUE_DEPENDENCY,
				    CONTROL_TYPE, 0, 0, 0, 0.0);
	    }
	  recentBranches.clear();
	}
      else if (!lastBranch && !lastInsnNOP)
	assert (!"Can we have a path without a lastBranch? Look at this path pal.");
      // if the assert above ever fails, understand when it can happen and
      // place a inner_loop_entry node there, just to restrict the 
      // scheduling from wrapping around.
    }
   
  // add the loop carried register dependencies; go through all the blocks
  // again. Only if this path was executed multiple times.
  if (avgNumIters > ONE_ITERATION_EPSILON)
    {
      numLoops = 0;
      for( i=0 ; i<numBlocks ; ++i )
	{
	  addrtype pc;
	  if (ba[i]->is_inner_loop ())
	    {
	      MIAMI::DecodedInstruction* &dInst = builtNodes[numLoops + 1];
	      assert(dInst != NULL && dInst->micro_ops.size()==1);

	      SchedDG::Node* node = static_cast<SchedDG::Node*>(dInst->micro_ops.front().data);
	      assert(node != NULL);
              handle_inner_loop_register_dependencies (node, innerRegs[numLoops], 0);
	      ++ numLoops;
	    }
	  else
            if (!ba[i]->isCfgEntry())
	      {
		CFG::ForwardInstructionIterator iit(ba[i]);
		while ((bool)iit)
		  {
		    pc = iit.Address();
                  
		    MIAMI::DecodedInstruction* &dInst = builtNodes[pc];
		    assert(dInst != NULL && !dInst->micro_ops.empty());
                  
		    // One native instruction can be decoded into multiple micro-ops
		    // Iterate over all micro-ops
		    MIAMI::InstrList::iterator lit = dInst->micro_ops.begin();
		    for ( ; lit!=dInst->micro_ops.end() ; ++lit)
		      {
			SchedDG::Node* node = static_cast<SchedDG::Node*>(lit->data);
			assert(node != NULL);
			if (node->is_intrinsic_type ())
			  handle_intrinsic_register_dependencies (node, 0);
			else
			  handle_register_dependencies (dInst, *lit, node, ba[i], 0);
		      }

		    if (ba[i]->is_delay_block())
		      {
			prev_inst_may_have_delay_slot = false;
			gpRegs = &gpMapper;
			stack_tops = crt_stack_tops;
			//                     fpRegs = &fpMapper;
			break;
		      }
		    ++ iit;
		  }
	      }
	}
    }
}


int
DGBuilder::build_node_for_instruction(addrtype pc, MIAMI::CFG::Node* b, float freq)
{
   MIAMI::DecodedInstruction* &dInst = builtNodes[pc];
   //assert(dInst == NULL); // rfriese: probably need to make this an if to use with dyninst
   dInst = new MIAMI::DecodedInstruction();
   
   int res = InstructionDecoder::decode_dbg(pc+reloc_offset, b->getEndAddress()-pc, dInst, "DGBuilder::build_node_for_instruction");

   // DynInst-based decoding
#if 0
   BPatch_function* dyn_func = routine->getDynFunction();
   BPatch_basicBlock* dyn_blk = routine->getBlockFromAddr(b->getStartAddress(), b->getEndAddress());
   if (!dyn_blk) {
     fprintf(stderr,"Dyninst: no block found at addr: 0x%lx\n",b->getStartAddress());
   }
   
   MIAMI::DecodedInstruction dInst2;
   res = InstructionDecoder::decode_dyninst(pc+reloc_offset, b->getEndAddress()-pc, &dInst2, dyn_func, dyn_blk);

   //res = isaXlate_insn_old(pc+reloc_offset, dInst); //FIXME: deprecated
#endif
   
   if (res < 0) { // error while decoding
      return (res);
   }

   
#if DECODE_INSTRUCTIONS_IN_PATH
   if (!targetPath || targetPath==pathId)
   {
      std::cerr << std::endl;
      debug_decode_instruction_at_pc((void*)(pc+reloc_offset), b->getEndAddress()-pc);
      DumpInstrList(dInst);
   }
#endif
      
   // one native instruction can be decoded into multiple micro-ops
   // iterate over them
   MIAMI::InstrList::iterator lit = dInst->micro_ops.begin();
   int num_uops = dInst->micro_ops.size();  // number of micro-ops in this instruction
   for (int i=0 ; lit!=dInst->micro_ops.end() ; ++lit, ++i)
   {
      MIAMI::instruction_info& iiobj = (*lit);
      int type = iiobj.type;

      // entry value holds information about input and output registers
      // of fortran intrinsics and other short inlinable functions
      EntryValue *entryVal = 0;
#if 0  // not sure if we still need to deal with instrinsics on x86 where
       // we see the code in shared libraries. Decide later.
      if (type==IB_jump)  // compute function call target
      {
//      fprintf (stderr, "Found function call at pc=0x%08x\n", pc);
         routine *r = b->in_routine ();
         addr temp = mach_inst_target (r->get_mach_inst(pc), pc, 0);
         executable *exec = r->in_executable ();
         if (temp!=NO_TARGET && !exec->in_text_segment(temp))
         {
            if ((entryVal=is_intrinsic_routine_call (exec, temp)) != 0)
               type = IB_intrinsic;
         }
      }
#endif
#if DEBUG_GRAPH_CONSTRUCTION
      DEBUG_GRAPH (7,
         fprintf(stderr, " -> process micro-op %d of instruction at 0x%lx of type %s\n",
               i, pc, Convert_InstrBin_to_string((InstrBin)type) );
      )
#endif
      SchedDG::Node* node = new Node(this, pc, i, iiobj);
      if (entryVal)
         node->setEntryValue (entryVal);
      add(node);
      // save how many micro-ops were in the original instruction
      // we may use this info later to reason about the number of instructions
      // and retirement rate
      node->setNumUopsInInstruction(num_uops);
      node->setInOrderIndex(nextUopIndex++);
      if (isUnreordableNode(node, b))
         markBarrierNode (node);

      iiobj.data = node;
//ozgurS
      if (node->is_load_instruction()){
         //TODO findMe and FIXME
         node->setLvlMap(img->getMemLoadData(node->getAddress()));
         std::cerr<<__func__<<" ozgur testing img total hit at lvl 0 is "<<node->getLvlMap()->find(0)->second.hitCount<<std::endl; 
      }
//ozgurE
      if (node->is_memory_reference())
      {
         int stackAccess = 0;
         // find index of memory operand corresponding to this memory micro-op
         int opidx = -1;
         if (node->is_store_instruction())
            opidx = iiobj.first_dest_operand_of_type(OperandType_MEMORY);
         else
            opidx = iiobj.first_src_operand_of_type(OperandType_MEMORY);
         if (opidx < 0)   // did not find one?, error
         {
            fprintf(stderr, "Error: DGBuilder::build_node_for_instruction: cannot find memory operand index for instruction at 0x%" PRIxaddr ", micro-op %d of type %s\n",
               pc, i, Convert_InstrBin_to_string((InstrBin)type));
            assert(!"I cannot find memory operand for micro-op!!!");
         }
         node->setMemoryOpIndex(opidx);
         handle_memory_dependencies (node, pc, opidx, type, stackAccess);
         
         // store all the PCs corresponding to memory references
         // if needed, differentiate between loads and stores
         memRefs.insert(pc);
      }
      if (node->is_intrinsic_type ())
         handle_intrinsic_register_dependencies (node, 1);
      else
         handle_register_dependencies (dInst, iiobj, node, b, 1);
   
      handle_control_dependencies (node, type, b, freq);

#if DECODE_INSTRUCTIONS_IN_PATH
      if (!targetPath || targetPath==pathId)
      {
         node->write_label(std::cerr);
         std::cerr << std::endl;
      }
#endif
   }
   return (res);
}

void
DGBuilder::handle_control_dependencies (SchedDG::Node *node, int type, 
                MIAMI::CFG::Node* b, float freq)
{
   // if this instruction has no LOOP_INDEPENDENT dependencies from a node
   // after the lastBranch, but has register or memory dependencies from a 
   // node before the last branch, add a control dependency from the last 
   // branch.
   // This does not apply to instructions in the delay slot whose execution 
   // is not conditioned by the branch (unless it is an anuled delay slot)
   
   // if this is a delay slot and I do not have a register dependency from 
   // last branch to this instruction, then this instruction is logically
   // before the branch. Check if this is such a delay slot.
   bool delay_process_later = false;
   if (b->is_delay_block() && lastBranch && 
              b->firstIncoming()->source()==lastBranchBB &&
              !node->isBarrierNode() )
   {
      int has_dep = 0;
      IncomingEdgesIterator ieit(node);
      while ((bool)ieit)
      {       
         if ( ieit->getLevel()==0 &&
              ieit->source()==lastBranch )
         {    
            has_dep = 1;
            break;
         }
         ++ ieit;
      }
      if (!has_dep)
         delay_process_later = true;
   }

   // If this node is a branch/barrier type and recentNodes is not empty
   // then this node will have control edges from the recent nodes to it,
   // and in addition the recent nodes should have control edges from the
   // last branch already; therefore I should not add an extra edge from
   // the last branch to this node in this case
   if (lastBranch && !delay_process_later)
   {
      int has_dep = 0;
      int has_strong_dep = 0;
      int has_memreg_dep_from_before = 0;
      IncomingEdgesIterator ieit(node);
      while ((bool)ieit)
      {       
         int elevel = ieit->getLevel();
         MIAMI::ObjId srcId = ieit->source()->getId();
         if (elevel==0 && srcId>=lastBranchId)
         {
            has_dep = 1;
            if (ieit->getType()!=CONTROL_TYPE ||
                ieit->getProbability()<BR_HIGH_PROBABILITY ||
                node->isBarrierNode() || 
                ieit->source()->isBarrierNode())
            {
               has_strong_dep = 1;
               break;
            }
         } else
         if (elevel==0 && ieit->getType()!=CONTROL_TYPE)
         {
            assert (srcId < lastBranchId);
            has_memreg_dep_from_before = 1;
         }
         ++ ieit;
      }
      
      if (!has_dep && (has_memreg_dep_from_before || lastBranch==lastBarrier))
         addUniqueDependency(lastBranch, node, TRUE_DEPENDENCY,
             CONTROL_TYPE, 0, 0, 0, lastBranchProb);

      if (!has_strong_dep && lastBarrier && lastBarrier!=lastBranch)
         addUniqueDependency(lastBarrier, node, TRUE_DEPENDENCY,
             CONTROL_TYPE, 0, 0, 0, 0.0);
   }
   
   // barrier nodes should have been identified as such by now
   if (node->isBarrierNode())
   {
      prevBarrier = lastBarrier;
      lastBarrier = node;
   }
   
//   if (type==CFG_LOOP_ENTRY_TYPE || InstrBinIsBranchType ((InstrBin)type))
   if ( InstrBinIsBranchType ((InstrBin)type) || 
        node->isBarrierNode())
   {
      // add a control dependency from each of the recent nodes to this one
      NodeSet::iterator nsit = recentNodes.begin();
      for ( ; nsit!=recentNodes.end() ; ++nsit )
      {
         // if the node does not have any outgoing dependency, create one;
         // create one even if it has an outgoing control dependency with 
         // high probability
         int has_dep = 0;
         OutgoingEdgesIterator oeit(*nsit);
         while ((bool)oeit)
         {       
            if ( oeit->getLevel()==0 && 
                 (oeit->getType()!=CONTROL_TYPE || 
                  oeit->getProbability()<BR_HIGH_PROBABILITY ||
                  oeit->source()->isBarrierNode() ||
                  oeit->sink()->isBarrierNode()) )
            {    
               has_dep = 1;
               break;
            }
            ++ oeit;
         }
         if (!has_dep)
            // I mark it with probability 1.0, but I do not prune control 
            // dependencies that go into a branch. I should set it to 0.
            addUniqueDependency(*nsit, node, TRUE_DEPENDENCY,
                CONTROL_TYPE, 0, 0, 0, 0.0);  //1.0);
      }
      recentNodes.clear();

      if (node->isBarrierNode())
      {
         // add a control dependency from each of the recent branches to this
         // barrier
         nsit = recentBranches.begin();
         for ( ; nsit!=recentBranches.end() ; ++nsit )
         {
            // if the node does not have any outgoing dependency, create one;
            // create one even if it has an outgoing control dependency with 
            // high probability
            int has_dep = 0;
            OutgoingEdgesIterator oeit(*nsit);
            while ((bool)oeit)
            {       
               if ( oeit->getLevel()==0 && 
                    (oeit->getType()!=CONTROL_TYPE || 
                     oeit->getProbability()<BR_HIGH_PROBABILITY ||
                     oeit->sink()->isBarrierNode()) )
               {    
                  has_dep = 1;
                  break;
               }
               ++ oeit;
            }
            if (!has_dep)
               // I mark it with probability 1.0, but I do not prune control 
               // dependencies that go into a branch. I should set it to 0.
               addUniqueDependency(*nsit, node, TRUE_DEPENDENCY,
                   CONTROL_TYPE, 0, 0, 0, 0.0);  //1.0);
         }
         recentBranches.clear();
      }  // else, this is a branch; add it to recentBranches
      else
         recentBranches.insert (node);
     
      prevBranch = lastBranch;
      prevBranchProb = lastBranchProb;
      prevBranchId = lastBranchId;
      
      lastBranch = node;
      lastBranchBB = b;
      lastBranchId = node->getId();
      // the fraction of times the next edge was taken
      lastBranchProb = freq;
   }
   
   // When I process the basic blocks, I see the instruction in the delay 
   // slot after I see the branch whose delay slot it fills. But the 
   // instruction in the delay slot is logically before the branch (if there
   // is no register dependency from the branch to the delay slot instruction),
   // and I will create the same control dependency from it to the branch that 
   // I've seen before. Do not add the instruction in the delay slot into the
   // recentNodes set.
   if (delay_process_later)
   {
      if (lastBranch)
      {
         // I mark it with probability 1.0, but I do not prune control 
         // dependencies that go into a branch. I should set it to 0.
         addUniqueDependency(node, lastBranch, TRUE_DEPENDENCY,
                CONTROL_TYPE, 0, 0, 0, 0.0);  //1.0);
                
         // update the lastBranchId to point to the instruction in the 
         // delay slot because this instruction has an id greater than the
         // branch itself (it is the order in which we process the blocks)
         // but it is logically before the branch.
         lastBranchId = node->getId();
         switchNodeIds (lastBranch, node);
      }
      
      // however, I need to add a control dependency from the branch before 
      // the last to the delay slot instruction if it has incoming 
      // dependencies from before the lastBranch, but not from a node after 
      // the last branch. Proceed as with the other nodes.
      if (prevBranch)
      {
         int has_dep = 0;
         int has_strong_dep = 0;
         int has_memreg_dep_from_before = 0;
         IncomingEdgesIterator ieit(node);
         while ((bool)ieit)
         {       
            int elevel = ieit->getLevel();
            MIAMI::ObjId srcId = ieit->source()->getId();
            if (elevel==0 && srcId>=prevBranchId)
            {    
               has_dep = 1;
               if (ieit->getType()!=CONTROL_TYPE ||
                   ieit->getProbability()<BR_HIGH_PROBABILITY ||
                   node->isBarrierNode() || 
                   ieit->source()->isBarrierNode())
               {
                  has_strong_dep = 1;
                  break;
               }
            } else
            if (elevel==0 && ieit->getType()!=CONTROL_TYPE)
            {
               assert (srcId < prevBranchId);
               has_memreg_dep_from_before = 1;
            }
            ++ ieit;
         }
         if (!has_dep && (has_memreg_dep_from_before || 
                 prevBranch==lastBarrier || prevBranch==prevBarrier))
            addUniqueDependency(prevBranch, node, TRUE_DEPENDENCY,
                CONTROL_TYPE, 0, 0, 0, prevBranchProb);
         if (!has_strong_dep)
         {
            if (lastBarrier && lastBarrier!=lastBranch && prevBranch!=lastBarrier)
               addUniqueDependency(lastBarrier, node, TRUE_DEPENDENCY,
                        CONTROL_TYPE, 0, 0, 0, 0.0);
            else 
               if (prevBarrier && lastBarrier==lastBranch && 
                         prevBarrier!=prevBranch)
                  addUniqueDependency(prevBarrier, node, TRUE_DEPENDENCY,
                           CONTROL_TYPE, 0, 0, 0, 0.0);
         }
      }
   } 
   else if (!lastBranch || node->getId()>lastBranchId)  
      // do not add the most recent branch into recentNodes
      recentNodes.insert(node);
}

// this method is called only for memory references
// no additional check is performed inside this routine
void
DGBuilder::handle_memory_dependencies(SchedDG::Node* node, addrtype pc, int opidx,
            int type, int stackAccess)
{
   int is_store =  node->is_store_instruction();

   // we do not differentiate between stack, fp and int memory ops anymore
   memory_dependencies_for_node(node, pc, opidx, is_store, 
                 gpStores, gpLoads);
   if (is_store)
      gpStores.push_back(node);
   else
      gpLoads.push_back(node);
}


void
DGBuilder::memory_dependencies_for_node(SchedDG::Node* node, addrtype pc, int opidx,
         int is_store, UNPArray& stores, UNPArray& loads)
{
   RefFormulas *refF = refFormulas.hasFormulasFor(pc,opidx);
   GFSliceVal _formula;
   if (refF == NULL)
   {
#if DEBUG_GRAPH_CONSTRUCTION
      DEBUG_GRAPH (1,
         fprintf(stderr, "WARNING: Did not find any formulas for memory instruction at address 0x%" PRIxaddr ", opidx %d\n",
                    pc, opidx);
      )
#endif
      if (!pessimistic_memory_dep)
         return;
      
      // TODO: refF is NULL; implement alternate solution
      // Until then, I have to return
//      return;
   } else
   {
      _formula = refF->base;
      if (_formula.is_uninitialized())
      {
#if DEBUG_GRAPH_CONSTRUCTION
         DEBUG_GRAPH (2,
            fprintf(stderr, "WARNING: Did not find base formula for memory instruction at address 0x%" PRIxaddr ", opidx %d\n",
                       pc, opidx);
         )
#endif
         if (optimistic_memory_dep)
            return;
      }
   }
   // Stack accesses sould only have the stack pointer register and a constant offset
   coeff_t offset;
   bool is_stack = FormulaIsStackReference(_formula, offset);
   
   UNPArray::iterator npit;
   UNPArray::iterator auxit;
   bool res;
   // if we deal with a stack load/store, compute only dependencies for one
   // iteration. This is equivalent to register renaming. The dependency cycle
   // due to loading/saving to the same stack location is a problem created
   // by the current compiler. If we applied software pipelining, a different
   // stack location could be used.
   if (avgNumIters<=ONE_ITERATION_EPSILON || is_stack)
   {
      // only one iteration, do not compute loop-carried dependencies
      //
      int depDir = TRUE_DEPENDENCY;
      if (is_store)
         depDir = OUTPUT_DEPENDENCY;
         
      npit=stores.end();
      if (!is_store || !is_stack)
         while(npit!=stores.begin())
         {
            --npit;
            res = computeMemoryDependenciesForOneIter(node, _formula, 
                     *npit, depDir);
            // stop when first dependency is found. It does not make sense to
            // go further back because the most recent store we find will be
            // dependence chained to another store and so on.
            // If current access is a load, we depend only on the last value
            // written to the same location, not the ones before
            if (res)
               break;
         }

      if (is_store && !is_stack)
      {
         depDir = ANTI_DEPENDENCY;
         
         npit=loads.end();
         res = false;
         while(npit!=loads.begin())
         {
            if (res)
            {
               auxit = npit;
               -- npit;
               loads.erase (auxit);
            } else
               --npit;
            res = computeMemoryDependenciesForOneIter(node, _formula, *npit, 
                 depDir);
            // if we created a dependency we should remove current load
            // from loads, such that it is not tested against newer stores
            // to the same location. Next store will be daisy-chained to
            // current store. Question: Does it affect the iterator if I
            // remove current element?
         }
         if (res)
            loads.erase (npit);
      }
   } else  // multiple iterations; compute loop-carried depend also
   {
      GFSliceVal _iterFormula;
      if (refF && refF->strides.size()>0)
         _iterFormula = refF->strides[0];

      if (_iterFormula.is_uninitialized())
      {
#if DEBUG_GRAPH_CONSTRUCTION
         DEBUG_GRAPH (2,
            fprintf(stderr, "WARNING: Did not find stride formula for memory instruction at address 0x%" PRIxaddr "\n",
                      pc);
         )
#endif
         if (optimistic_memory_dep)
            return;
      }
      coeff_t valueNum;
      ucoeff_t valueDen;
      
      int refIsScalar = 0;
      int refIsIndirect = 0;
      if (_iterFormula.has_indirect_access())
         refIsIndirect = 1;
      if (IsConstantFormula(_iterFormula, valueNum, valueDen) &&
            valueNum==0)
         refIsScalar = 1;

      int negDir = TRUE_DEPENDENCY;
      int posDir = ANTI_DEPENDENCY;
      int refTypes = 0;
      if (is_store)
      {
         refTypes = 2;
         negDir = posDir = OUTPUT_DEPENDENCY;
      }
      
      npit=stores.end();
      if (!is_store || !is_stack || !refIsScalar || refIsIndirect)
         while(npit!=stores.begin())
         {
            -- npit;
            res = computeMemoryDependenciesForManyIter(node, _formula, 
                _iterFormula, refIsScalar, refIsIndirect, *npit, negDir, 
                posDir, refTypes);
            if (res && is_stack && refIsScalar && !refIsIndirect)
               break;
         }  // while npit!=stores.begin()

      if (is_store && (!is_stack || !refIsScalar || refIsIndirect))
      {
         posDir = TRUE_DEPENDENCY;
         negDir = ANTI_DEPENDENCY;
         refTypes = 1;
         
         npit=loads.end();
         while(npit!=loads.begin())
         {
            --npit;
            res = computeMemoryDependenciesForManyIter(node, _formula, 
                _iterFormula, refIsScalar, refIsIndirect, *npit, negDir, 
                posDir, refTypes);
         }
      }  // while npit!=loads.begin()
   }  // multiple iterations executed

}


bool
DGBuilder::computeMemoryDependenciesForOneIter(SchedDG::Node *node, 
        GFSliceVal& _formula, SchedDG::Node *nodeB, int depDir)
{
   coeff_t valueNum;
   ucoeff_t valueDen;
   addrtype opc = nodeB->getAddress();  // pc of the Other instruction
   int oopidx = nodeB->memoryOpIndex();
   if (oopidx < 0) {
      fprintf(stderr, "Error: DGBuilder::computeMemoryDependenciesForOneIter: Instruction at 0x%" PRIxaddr ", uop of type %s, hash memory opidx %d\n",
            opc, Convert_InstrBin_to_string((MIAMI::InstrBin)nodeB->getType()), oopidx);
      assert(!"Negative memopidx. Why??");
   }
   GFSliceVal oform;
   RefFormulas *refF = refFormulas.hasFormulasFor(opc,oopidx);
   if (refF == NULL)
   {
      if (!pessimistic_memory_dep)
         return (false);
   }
   
   oform = refF->base;
   if (oform.is_uninitialized() || _formula.is_uninitialized() ||
       !IsConstantFormula(_formula-oform, valueNum, valueDen))
   {
      if (!optimistic_memory_dep)
         addUniqueDependency(nodeB, node, 
            depDir, MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
   } else
      if (valueNum==0)
      {
         addUniqueDependency(nodeB, node, 
            depDir, MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
         return (true);
      }
   return (false);
}


bool
DGBuilder::computeMemoryDependenciesForManyIter(SchedDG::Node *node, 
        GFSliceVal& _formula, GFSliceVal& _iterFormula, int refIsScalar, 
        int refIsIndirect, SchedDG::Node *nodeB, int negDir, int posDir,
        int refTypes)
{
   coeff_t valueNum;
   ucoeff_t valueDen;
   bool res = false;
   addrtype reloc = img->RelocationOffset();
   
   addrtype pc = node->getAddress();  // pc of the this instruction
   addrtype opc = nodeB->getAddress();  // pc of the Other instruction
   int oopidx = nodeB->memoryOpIndex();
   if (oopidx < 0) {
      fprintf(stderr, "Error: DGBuilder::computeMemoryDependenciesForManyIter: Instruction at 0x%" PRIxaddr ", uop of type %s, hash memory opidx %d\n",
            opc, Convert_InstrBin_to_string((MIAMI::InstrBin)nodeB->getType()), oopidx);
      assert(!"Negative memopidx. Why??");
   }
#if DEBUG_GRAPH_CONSTRUCTION
   DEBUG_GRAPH (4,
      fprintf(stderr, "Computing multi-iter mem dependency between inst at 0x%" PRIxaddr " and inst at pc 0x%" PRIxaddr "\n",
               pc, opc);
   )
#endif

   GFSliceVal oform;
   GFSliceVal oiter;
   RefFormulas *refF = refFormulas.hasFormulasFor(opc,oopidx);
   if (refF == NULL)
   {
      if (pessimistic_memory_dep)
      {
         addUniqueDependency(nodeB, node, 
              negDir, MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
         addUniqueDependency(node, nodeB, 
              posDir, MEMORY_TYPE, 1, 1, 0);
      }
      return (false);
   }
   
   oform = refF->base;
   if (refF->strides.size()>0)
      oiter = refF->strides[0];
      
   int ref2IsScalar = 0;
   if  (!oiter.is_uninitialized() && 
        IsConstantFormula(oiter, valueNum, valueDen) &&
        valueNum==0)
      ref2IsScalar = 1;
         
   coeff_t factor1 = 0, factor2 = 0;
#if DEBUG_GRAPH_CONSTRUCTION
   DEBUG_GRAPH (4,
      std::cerr << "Testing dependencies between B" << node->getId() << " with baseF=" << _formula
           << " and iterF=" << _iterFormula << ", and B" << nodeB->getId()
           << " with baseF=" << oform << " and iterF=" << oiter 
           << ". DIFF BaseF=" << _formula-oform << std::endl;
   )
#endif
   
   if (oform.is_uninitialized() || oiter.is_uninitialized() || 
       _formula.is_uninitialized() || _iterFormula.is_uninitialized())
   {
      if (!optimistic_memory_dep)
      {
         addUniqueDependency(nodeB, node, 
              negDir, MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
         addUniqueDependency(node, nodeB, 
              posDir, MEMORY_TYPE, 1, 1, 0);
      }
   } else
   // if one of the accesses is indirect, be conservative and assume
   // dependency such that reordering is disabled.
   // the compiler is conservative also in such cases and I think
   // the static analysis will produce few if any false positives.
   // Be a little more optimistic. If the two base formulas differ through 
   // some constant term (fixed register, not a load to reg), then its likely
   // that the corresponding references access different arrays, so I should
   // not have memory dependencies. With statically allocated arrays, I will
   // not find any registers. Add another case: if the formulas differ by a
   // constant term that is larger than the stride of the non-indirect one
   // times the number of iterations, then we do not have a dependency either.
   // This is not very clean, but I found a case where thousands of 
   // dependencies were created while not needed.
   // Since I recover names for all references, I should test also the names.
   // Create a dependence only if the two references access same array.
   // Another instance for avoiding dependencies: if base formulas differ by a 
   // constant term and the strides are equal and non-zero (even if marked as indirect), 
   // but the difference of the base formulas is not a multiple of the stride,
   // then it is very likely that the two references access different locations.
   // 11/13/2013, gmarin: do not mark as a dependence if the strides are equal and zero 
   // (even if indirect), and if the base formulas differe by a non-zero term. 
   // I think HasIntegerRatio between a non-zero contant term and zero returns true, 
   // so check explicitly if the iter formula is in fact zero.
   if ( IsConstantFormula(_iterFormula-oiter, valueNum, valueDen) && // the same stride (even if marked indirect)
        valueNum==0 &&
        IsConstantFormula(_formula-oform, valueNum, valueDen) && // but the two refs access
        valueNum!=0 &&    // locations a constant offset apart
        ((IsConstantFormula(_iterFormula, valueNum, valueDen) && valueNum==0) ||
          !HasIntegerRatio(_formula-oform, _iterFormula, factor1, factor2))
      )  // and base's difference is not a multiple of the stride
      return (false);   // then, we do not have a dependency
   else
   if (refIsIndirect || oiter.has_indirect_access())
   {
      // check array names
      int32_t iidx = img->GetIndexForInstPC(pc+reloc, node->memoryOpIndex());
      assert (iidx > 0);
      int32_t oidx = img->GetIndexForInstPC(opc+reloc, oopidx);
      assert (oidx > 0);
      const char* name_pc = img->GetNameForReference(iidx);
      const char* name_opc = img->GetNameForReference(oidx);
      
      /** gmarin 03/28/2013: I should also exclude cases when the base 
       * formulas differ by Loads from constant addresses. I do not know if 
       * I can make a good case for it yet, but it would fix a lot false 
       * memory dependencies between indirect loads and stores.
       */
      // do not create dependencies between indirect accesses if
      // - they access different array names (I do not have names yet)
      // - optimistic_memory_dep is set AND
      //    - formulas differ by a register, or
      //    - formulas differ by a TY_REFERENCE term (load from constant address)
      GFSliceVal diffFormula = _formula - oform;

      // if the two refs access same data array
      if (!strcmp (name_pc, name_opc) && 
           (!optimistic_memory_dep || 
               (!FormulaContainsRegister (diffFormula) &&
                !FormulaContainsReferenceTerm (diffFormula))
           ) )
      {
         // My only hope is to have a large constant term difference
         int constStride = 0;
         if (!refIsScalar && !refIsIndirect && 
                IsConstantFormula(_iterFormula, valueNum, valueDen))
            constStride = -valueNum;
         else
            if (!ref2IsScalar && !oiter.has_indirect_access() && 
                 IsConstantFormula (oiter, valueNum, valueDen))
               constStride = valueNum;
      
         // if no access is direct with a constant stride, or we cannot tell 
         // much from the constant terms, then create the dependencies
         // This solution is not that good. It does not guarantee neither few
         // positiver, nor few negatives. It just fixes a particular case I
         // found in nogaps. FIXME???
         float rez;
         if (!constStride ||
             ((rez=(((float)ConstantTermOfFormula(_formula-oform))/
               ((float)constStride)))<=avgNumIters && rez>=0) )
         {
            Edge *ee = addUniqueDependency(nodeB, node, 
                 negDir, MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
            ee->markIndirect();  // mark dependencies that are created due to
                                 // indirect accesses
            ee = addUniqueDependency(node, nodeB, 
                 posDir, MEMORY_TYPE, 1, 1, 0);
            ee->markIndirect();
         }
      }
   } else  // strides are computed and accesses are not indirect
   if (refIsScalar || ref2IsScalar)
   {
      if (refIsScalar && ref2IsScalar)
      {
         if (!IsConstantFormula(_formula-oform, valueNum, valueDen))
         {
            if (!optimistic_memory_dep)
               addUniqueDependency(nodeB, node, 
                    negDir, MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
         } else
            if (valueNum==0)
            {
               // if both accesses are scalars, create only true dependencies.
               // Anti- and Output- ones can be eliminated by scalar 
               // expansion.
               if (refTypes==0)
                  addUniqueDependency (nodeB, node, 
                       negDir, MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
               else if (refTypes==1)
                  addUniqueDependency (node, nodeB, posDir, MEMORY_TYPE, 
                       1, 1, 0);
               res = true;
            }
      } else  // one access is scalar and one is array
      {
         GFSliceVal stride;
         if (refIsScalar)
            stride = oiter;
         else
            stride = _iterFormula;
         GFSliceVal diffFormula = _formula - oform;
         coeff_t factor1 = 0, factor2 = 0;
         if ( IsConstantFormula(diffFormula, valueNum, valueDen) &&
              valueNum==0)
         {
#if DEBUG_GRAPH_CONSTRUCTION
            DEBUG_GRAPH (2,
               std::cerr << "Mem reference at " << std::hex << pc
                    << " and reference at " << opc << " have memory "
                    << "dependency caused by first iteration " 
                    << "(FIXED to most restrictive)." << std::dec << std::endl;
            )
#endif
            // I think that even though the dependency is only on the
            // first iteration, or it has varying distance, I should not
            // mark them as special dependencies, but use the distance
            // that is the strictest (smallest) from all the values
            // taken.
            addUniqueDependency(nodeB, node, 
                    negDir, MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
//            addUniqueDependency(nodeB, node, 
//                    negDir, MEMORY_TYPE, VARYING_DISTANCE, 1, 0);
         } else
            if (HasIntegerRatio(diffFormula, stride, factor1, factor2)
                && (factor2==1 || factor2==(-1)) )
            {
               factor1 = factor1 / factor2;
               int minIter = abs(factor1);
               // the existence of this dependency depends on the
               // number of iterations. But the number of iterations
               // depends on the problem size. I have to parameterize
               // this dependency. FIXME
               // 10/23/06 mgabi: Well, if the distance is larger than the 
               // average number of iterations, then most likely there isn't
               // a dependency for larger problem sizes either. This happens
               // frequently for statically allocated data (like for a maximum
               // problem size set at compile time). If we ever reach a 
               // problem size that causes a dependency, then most likely we
               // have exceeded the allocated memory and need to recompile 
               // with a larger maximum data set.
               if ( ((ref2IsScalar && factor1<0) || 
                    (refIsScalar && factor1>0)) 
                    && minIter<avgNumIters )
               {
#if DEBUG_GRAPH_CONSTRUCTION
                  DEBUG_GRAPH (2,
                     std::cerr << "Mem reference at " << std::hex << pc
                          << " and reference at " << opc << " have memory "
                          << "dependency caused by iteration " << std::dec
                          << minIter << " (FIXED to most restrictive)."
                          << std::endl;
                  )
#endif
                  // I think that even though the dependency has varying 
                  // distance, I should not mark them as special 
                  // dependencies, but use the distance that is the strictest 
                  // (smallest) from all the values taken.
                  addUniqueDependency(nodeB, node, 
                          negDir, MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
                  if ((minIter+1) < avgNumIters)
                     addUniqueDependency (node, nodeB, 
                             posDir, MEMORY_TYPE, 1, 1, 0);
//                  addUniqueDependency(nodeB, node, CHANGING_DEPENDENCY, 
//                            MEMORY_TYPE, VARYING_DISTANCE, abs(factor1), 0);
               }
               
            }
      }
   } else  // strides are computed and both accesses are of array type
   {
      // instead of treating the equal strides case separately,
      // can I generalize for all cases?
      // should I check for irregular strides? 
      // (these are not accurately computed)
      factor1 = factor2 = 0;
      if (HasIntegerRatio(_iterFormula, oiter, factor1, factor2))
      {
         // strides have the same terms; good I guess
         GFSliceVal diffFormula = _formula - oform;
         if ( IsConstantFormula(diffFormula, valueNum, valueDen) &&
              valueNum==0)
         {
            // base formulas are equal; if strides are equal than we 
            // have loop independent dependency, otherwise we have
            // dependency only in the first iteration
            if (factor1 == factor2)
            {
               addUniqueDependency(nodeB, node, 
                       negDir, MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
            } else
            // else, if factors have different signs, they go away
            // from each other => depend on first iteration only
            if ((factor1>0 && factor2<0) || (factor1<0 && factor2>0))
            {
#if DEBUG_GRAPH_CONSTRUCTION
               DEBUG_GRAPH (2,
                  std::cerr << "Mem reference at " << std::hex << pc 
                       << " and reference at " << opc << " have memory "
                       << "dependency only on the first iteration." 
                       << std::dec << std::endl;
               )
#endif
               // I think that even though the dependency is only on the
               // first iteration, or it has varying distance, I should not
               // mark them as special dependencies, but use the distance
               // that is the strictest (smallest) from all the values
               // taken.
               addUniqueDependency(nodeB, node, 
                       negDir, MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
//               addUniqueDependency(nodeB, node, 
//                       negDir, MEMORY_TYPE, ONE_ITERATION, 1, 0);
            } else  // factors have same sign => varying distance
            {
               // factors can be only positive
               assert(factor1>0 && factor2>0);
               
#if DEBUG_GRAPH_CONSTRUCTION
               DEBUG_GRAPH (2,
                  std::cerr << "Mem reference at " << std::hex << pc
                       << " and reference at " << opc << " have memory "
                       << "dependencies with varying distance starting "
                       << "with the first iteration." << std::dec << std::endl;
               )
#endif
               addUniqueDependency(nodeB, node, 
                       negDir, MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
               if (factor1 > factor2)
               {
                  addUniqueDependency (node, nodeB, 
                         posDir, MEMORY_TYPE, 1, 1, 0);
               }
#if 0
               if (factor1 > factor2)
               {
                  addUniqueDependency (nodeB, node, 
                         negDir, MEMORY_TYPE, ONE_ITERATION, 0, 0);
                  addUniqueDependency (node, nodeB, 
                         posDir, MEMORY_TYPE, VARYING_DISTANCE, 1, 0);
               } else
                  addUniqueDependency(nodeB, node,
                         negDir, MEMORY_TYPE, VARYING_DISTANCE, 0, 0);
#endif
            }
         } else   // base formulas are not equal
                  // check if difference and strides have integer ratio
         {
            coeff_t diffFact1 = 0, diffFact2 = 0;
            // strides have integer factor, but compute their GCD
            // this is equal to stride1/factor1 or stride2/factor2.
            // Division should be exact.
            
            // Both factors cannot be negative at the same time b/c
            // the -1 term is reduced in HasIntegerRatio. At most one
            // factor is negative
            GFSliceVal gcdStride;
            if (factor1 < 0)
            {
               assert(factor2>0);
               gcdStride = oiter / factor2;
            } else
               gcdStride = _iterFormula / factor1;
               
#if DEBUG_GRAPH_CONSTRUCTION
            DEBUG_GRAPH (2,
               std::cerr << "Comparing unequal formulas with integer ratio strides: "
                    << " _formula=" << _formula << "; _iterFormula=" << _iterFormula
                    << "; oform=" << oform << "; oiter=" << oiter 
                    << "; diffFormula=" << diffFormula << "; gcdStride=" << gcdStride
                    << std::endl;
            )
#endif
            
            if (HasIntegerRatio(diffFormula, gcdStride, diffFact1,
                  diffFact2) && (diffFact2==1 || diffFact2==(-1)) )
            {
#if DEBUG_GRAPH_CONSTRUCTION
              DEBUG_GRAPH (3,
                 std::cerr << "-> diffFact1=" << diffFact1 << ", diffFact2=" << diffFact2 
                      << ", factor1=" << factor1 << ", factor2=" << factor2 << std::endl;
              )
#endif
               if (diffFact2==(-1))
                  diffFact1 = -(diffFact1);
               // simple case if strides are equal, then distance must
               // be an integer
               if (factor1==factor2)
               { 
                  if ((abs(diffFact1)%abs(factor1))==0)
                  {
                     // division should be exact
                     coeff_t dist = diffFact1 / factor1;
                     if (dist>0)
                     {
                        if (dist<avgNumIters)
                           addUniqueDependency(node, nodeB, 
                                 posDir, MEMORY_TYPE, dist, 1, 0);
                     }
                     else
                     {
                        assert(dist<0);  // it cannot be zero
                        if ((-dist)<avgNumIters)
                           addUniqueDependency(nodeB, node,
                                 negDir, MEMORY_TYPE, -dist, 1, 0);
                     }
                  }
               } else   // strides are not equal;
                        // test for cases when we cannot have dependencies
               {
                  if (!( (diffFact1>0 && factor1>=0 && factor2<=0) ||
                         (diffFact1<0 && factor1<=0 && factor2>=0) ))
                  {
                     float distance = 0;
                     int dist = 0, ldist = 0;
                     // if the strides have different signs, then the
                     // two curves come towards each other (the case 
                     // when they go away from each other was eliminated
                     // by the condition of the "if" above) and the
                     // intersection point represents the minimum
                     // number of iterations necessary to have a depend
                     if ( (factor1>0 && factor2<0) || 
                          (factor1<0 && factor2>0) )
                     {
                        distance = ((float)diffFact1)/(factor2-factor1);
                        assert (distance > 0);
                        dist  = ceil  (distance - 0.00001f);
                        ldist = floor (distance + 0.00001f);
#if DEBUG_GRAPH_CONSTRUCTION
                        DEBUG_GRAPH (3,
                           std::cerr << "--> distance=" << distance << ", dist=" << dist << ", ldist=" << ldist << std::endl;
                        )
#endif
                        if (dist < avgNumIters)
                        {
                           // compute the minimum distance achievable.
                           // For this, check what locations are touched by 
                           // node in iteration numbers ldist and dist 
                           // respectively. Then compute the iteration
                           // numbers when nodeB touches the same locations.
                           coeff_t locF1=0, locF2=0;
                           GFSliceVal lDifference = diffFormula + 
                                          _iterFormula * ldist;
                           if (HasIntegerRatio (lDifference, oiter, locF1,
                                 locF2) && (locF2==1 || locF2==(-1)) )
                           {
                              // I think both locF2 and locF1 should be
                              // positive for this case
                              assert (locF2==1 && locF1>0);
                              // if (locF2<0)
                              //   // this is actually the iteration number for nodeB
                              //   locF1 = -locF1;
                              
                              if (locF1 < avgNumIters)
                              {
                                 if (locF1 == ldist) // same iteration
                                    addUniqueDependency(nodeB, node, negDir, 
                                          MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
                                 else if (locF1 < ldist)
                                    addUniqueDependency(nodeB, node, negDir, 
                                          MEMORY_TYPE, ldist-locF1, 1, 0);
                                 else  // locF1 > ldist
                                    addUniqueDependency(node, nodeB, posDir, 
                                          MEMORY_TYPE, locF1-ldist, 1, 0);
                              }
                           }
                           
                           if (ldist != dist)
                           {
                              lDifference = diffFormula + 
                                              _iterFormula * dist;
                              if (HasIntegerRatio (lDifference, oiter, locF1,
                                    locF2) && (locF2==1 || locF2==(-1)) && locF1>0)
                              {
#if DEBUG_GRAPH_CONSTRUCTION
                                 DEBUG_GRAPH (3,
                                    std::cerr << "DEBUG: pc=" << std::hex << pc << ", opc=" << opc << std::dec
                                         << ", lDifference=" << lDifference << ", oiter=" << oiter
                                         << ", locF1=" << locF1 << ", locF2=" << locF2 << std::endl;
                                 )
#endif
                                 // I think both locF2 and locF1 should be
                                 // positive for this case
                                 assert (locF2==1 && locF1>0);
                                 // if (locF2<0)
                                 //   // this is actually the iteration number for nodeB
                                 //   locF1 = -locF1;
                              
                                 if (locF1 < avgNumIters)
                                 {
                                    if (locF1 == dist) // same iteration
                                       addUniqueDependency(nodeB, node, negDir, 
                                             MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
                                    else if (locF1 < dist)
                                       addUniqueDependency(nodeB, node, negDir, 
                                             MEMORY_TYPE, dist-locF1, 1, 0);
                                    else  // locF1 > dist
                                       addUniqueDependency(node, nodeB, posDir, 
                                             MEMORY_TYPE, locF1-dist, 1, 0);
                                 }
                              }
                              
                           }
                           
#if 0
                           std::cerr << "Add varying dist depend (1) from " 
                                << *nodeB << " to " << *node << " with "
                                << "f1=" << factor1 << ", f2=" << factor2
                                << ", diffF1=" << diffFact1 << ", dist="
                                << dist << std::endl;
                           addUniqueDependency(nodeB, node, negDir, 
                                 MEMORY_TYPE, VARYING_DISTANCE, dist, 0);
#endif
                        }
                     }
                     else  // factors have same sign (positive only)
                     {
                        assert(factor1>0 && factor2>0);
                        if (diffFact1>0)
                           distance = ((float)diffFact1)/factor2;
                        else
                           distance = ((float)diffFact1)/factor1;
                        dist = ceil (fabs (distance) - 0.00001f);
                        if (dist >= avgNumIters)
                           // the two references cannot access the same location
                           return (res);
                        
                        coeff_t locF1=0, locF2=0;

                        // if diffFact1>0 && factor1>factor2 
                        // or diffFact1<0 && factor1<factor2
                        // then distance is growing. Smaller distance is
                        // in the first iteration. 
                        // Dependencies have same direction in this case
                        // compute the minimum number of iterations required
                        // to have a dependency

                        if (diffFact1>0 && factor1>factor2)
                        {
                           // distance is growing; compute location accessed
                           // by node in iteration 0, and compute the 
                           // iteration number when nodeB accesses that location
                           
                           if (HasIntegerRatio (diffFormula, oiter, locF1,
                                 locF2) && (locF2==1 || locF2==(-1)) )
                           {
                              // I think both locF2 and locF1 should be
                              // positive for this case
                              assert (locF2==1 && locF1>0);
                              // if (locF2<0)
                              //   // this is actually the iteration number for nodeB
                              //   locF1 = -locF1;
                              
                              // since I return is dist>=avgNumIters (above)
                              // locF1 should always be < avgNumIters
                              assert (locF1 < avgNumIters);
                              addUniqueDependency(node, nodeB, posDir, 
                                       MEMORY_TYPE, locF1, 1, 0);
                           }
                        } else if (diffFact1<0 && factor1<factor2)
                        {
                           // distance is growing; compute location accessed
                           // by nodeB in iteration 0, and compute the 
                           // iteration number when node accesses that location

                           if (HasIntegerRatio (diffFormula*(-1), _iterFormula, 
                                locF1, locF2) && (locF2==1 || locF2==(-1)) )
                           {
                              // I think both locF2 and locF1 should be
                              // positive for this case
                              assert (locF2==1 && locF1>0);
                              // if (locF2<0)
                              //   // this is actually the iteration number for nodeB
                              //   locF1 = -locF1;
                              
                              assert (locF1 < avgNumIters);
                              addUniqueDependency(nodeB, node, negDir, 
                                       MEMORY_TYPE, locF1, 1, 0);
                           }
                        } else if (diffFact1>0 && factor1<factor2)
                        {
                           // distance is shrinking; compute iteration number
                           // when dependency direction is changing.
                           // If this number is less than avgNumIters, then
                           // compute the distances around this point.
                           // Otherwise, compute the iteration number when
                           // nodeB touches the location accessed by node in
                           // first iteration. If this number is < avgNumIters
                           // compute the location accessed by nodeB at
                           // iteration avgNumIters, and then compute the 
                           // iteration number when node accesses the same
                           // location (should be earlier), and that gives the
                           // shortest distance.
                           distance = ((float)diffFact1)/(factor2-factor1);
                           assert (distance > 0);
                           dist  = ceil  (distance - 0.00001f);
                           ldist = floor (distance + 0.00001f);
                           
                           if (ldist < avgNumIters)
                           {
                              // compute the minimum distance achievable.
                              // For this, check what locations are touched by 
                              // nodeB in iteration numbers ldist and dist 
                              // respectively. Then compute the iteration
                              // numbers when node touches the same locations.
                              coeff_t locF1=0, locF2=0;
                              GFSliceVal lDifference = oiter * ldist - 
                                               diffFormula;
                              if (HasIntegerRatio (lDifference, _iterFormula, 
                                   locF1, locF2) && (locF2==1 || locF2==(-1)))
                              {
                                 // I think both locF2 and locF1 should be
                                 // positive for this case
                                 assert (locF2==1 && locF1>0);
                                 // if (locF2<0)
                                 //   // this is actually the iteration number for nodeB
                                 //   locF1 = -locF1;
                                 
                                 if (locF1 < avgNumIters)
                                 {
                                    if (locF1 == ldist) // same iteration
                                       addUniqueDependency(nodeB, node, negDir, 
                                             MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
                                    else if (locF1 > ldist)
                                       addUniqueDependency(nodeB, node, negDir, 
                                             MEMORY_TYPE, locF1-ldist, 1, 0);
                                    else  // locF1 < ldist
                                       addUniqueDependency(node, nodeB, posDir, 
                                             MEMORY_TYPE, ldist-locF1, 1, 0);
                                 }
                              }
                              
                              if (ldist!=dist && dist<avgNumIters)
                              {
                                 lDifference = oiter * dist - 
                                                 diffFormula;
                                 if (HasIntegerRatio (lDifference, _iterFormula, 
                                     locF1, locF2) && (locF2==1 || locF2==(-1)))
                                 {
                                    // I think both locF2 and locF1 should be
                                    // positive for this case
                                    assert (locF2==1 && locF1>0);
                                    // if (locF2<0)
                                    //   // this is actually the iteration number for nodeB
                                    //   locF1 = -locF1;
                                 
                                    if (locF1 < avgNumIters)
                                    {
                                       if (locF1 == dist) // same iteration
                                          addUniqueDependency(nodeB, node, negDir, 
                                                MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
                                       else if (locF1 > dist)
                                          addUniqueDependency(nodeB, node, negDir, 
                                                MEMORY_TYPE, locF1-dist, 1, 0);
                                       else  // locF1 < dist
                                          addUniqueDependency(node, nodeB, posDir, 
                                                MEMORY_TYPE, dist-locF1, 1, 0);
                                    }
                                 }
                              }
                           } else 
                           {
                              // we know there is at least one iteration 
                              // with dependencies before we reach avgNumIters
                              // Compute location accessed by nodeB in the
                              // last iteration
                              coeff_t locF1=0, locF2=0;
                              GFSliceVal lDifference = oiter * (avgNumIters-1) - 
                                               diffFormula;
                              if (HasIntegerRatio (lDifference, _iterFormula, 
                                   locF1, locF2) && (locF2==1 || locF2==(-1)))
                              {
                                 // I think both locF2 and locF1 should be
                                 // positive for this case
                                 assert (locF2==1 && locF1>0);
                                 // if (locF2<0)
                                 //   // this is actually the iteration number for nodeB
                                 //   locF1 = -locF1;
                                 
                                 assert (locF1 < avgNumIters);
                                 addUniqueDependency(node, nodeB, posDir, 
                                        MEMORY_TYPE, avgNumIters-locF1-1, 1, 0);
                              }
                           }
                        } else // if (diffFact1<0 && factor1>factor2)
                        {
                           assert (diffFact1<0 && factor1>factor2);
                           // distance is shrinking and there is at least one
                           // iteration with dependencies; 
                           // compute iteration number when dependency 
                           // direction is changing.
                           // If this number is less than avgNumIters, then
                           // compute the distances around this point.
                           // Otherwise, compute the location accessed by node 
                           // at iteration avgNumIters, and then compute the 
                           // iteration number when nodeB accesses the same
                           // location (should be earlier), and that gives the
                           // shortest distance.
                        
                           distance = ((float)diffFact1)/(factor2-factor1);
                           assert (distance > 0);
                           dist  = ceil  (distance - 0.00001f);
                           ldist = floor (distance + 0.00001f);
                           
                           if (ldist < avgNumIters)
                           {
                              // compute the minimum distance achievable.
                              // For this, check what locations are touched by 
                              // node in iteration numbers ldist and dist 
                              // respectively. Then compute the iteration
                              // numbers when nodeB touches the same locations.
                              coeff_t locF1=0, locF2=0;
                              GFSliceVal lDifference = _iterFormula * ldist +
                                               diffFormula;
                              if (HasIntegerRatio (lDifference, oiter, 
                                   locF1, locF2) && (locF2==1 || locF2==(-1)))
                              {
                                 // I think both locF2 and locF1 should be
                                 // positive for this case
                                 assert (locF2==1 && locF1>0);
                                 // if (locF2<0)
                                 //   // this is actually the iteration number for nodeB
                                 //   locF1 = -locF1;
                                 
                                 if (locF1 < avgNumIters)
                                 {
                                    if (locF1 == ldist) // same iteration
                                       addUniqueDependency(nodeB, node, negDir, 
                                             MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
                                    else if (locF1 < ldist)
                                       addUniqueDependency(nodeB, node, negDir, 
                                             MEMORY_TYPE, ldist-locF1, 1, 0);
                                    else  // locF1 > ldist
                                       addUniqueDependency(node, nodeB, posDir, 
                                             MEMORY_TYPE, locF1-ldist, 1, 0);
                                 }
                              }
                              
                              if (ldist!=dist && dist<avgNumIters)
                              {
                                 lDifference = _iterFormula * dist + 
                                                 diffFormula;
                                 if (HasIntegerRatio (lDifference, oiter, 
                                     locF1, locF2) && (locF2==1 || locF2==(-1)))
                                 {
                                    // I think both locF2 and locF1 should be
                                    // positive for this case
                                    assert (locF2==1 && locF1>0);
                                    // if (locF2<0)
                                    //   // this is actually the iteration number for nodeB
                                    //   locF1 = -locF1;
                                 
                                    if (locF1 < avgNumIters)
                                    {
                                       if (locF1 == dist) // same iteration
                                          addUniqueDependency(nodeB, node, negDir, 
                                                MEMORY_TYPE, LOOP_INDEPENDENT, 0, 0);
                                       else if (locF1 < dist)
                                          addUniqueDependency(nodeB, node, negDir, 
                                                MEMORY_TYPE, dist-locF1, 1, 0);
                                       else  // locF1 > dist
                                          addUniqueDependency(node, nodeB, posDir, 
                                                MEMORY_TYPE, locF1-dist, 1, 0);
                                    }
                                 }
                              }
                           } else 
                           {
                              // we know there is at least one iteration 
                              // with dependencies before we reach avgNumIters
                              // Compute location accessed by node in the
                              // last iteration
                              coeff_t locF1=0, locF2=0;
                              GFSliceVal lDifference = _iterFormula * (avgNumIters-1) +
                                               diffFormula;
                              if (HasIntegerRatio (lDifference, oiter, 
                                   locF1, locF2) && (locF2==1 || locF2==(-1)))
                              {
                                 // I think both locF2 and locF1 should be
                                 // positive for this case
                                 assert (locF2==1 && locF1>0);
                                 // if (locF2<0)
                                 //   // this is actually the iteration number for nodeB
                                 //   locF1 = -locF1;
                                 
                                 assert (locF1 < avgNumIters);
                                 addUniqueDependency(nodeB, node, negDir, 
                                        MEMORY_TYPE, avgNumIters-1-locF1, 1, 0);
                              }
                           }
                        }

#if 0
                        if ( (diffFact1>0 && factor1>factor2) ||
                             (diffFact1<0 && factor1<factor2) )
                        {  // mode 1
                           if (distance>0)  // diffFormula>0 && factor1>factor2
                           {
                              if (distance < avgNumIters)
                              {
                           std::cerr << "Add varying dist depend (2) from " 
                                << *node << " to " << *nodeB << " with "
                                << "f1=" << factor1 << ", f2=" << factor2
                                << ", diffF1=" << diffFact1 << ", dist="
                                << dist << std::endl;
                                 addUniqueDependency(node, nodeB, posDir, 
                                     MEMORY_TYPE, VARYING_DISTANCE, dist, 0);
                              }
                           }
                           else
                           {
                              if ((-distance) < avgNumIters)
                              {
                           std::cerr << "Add varying dist depend (3) from " 
                                << *nodeB << " to " << *node << " with "
                                << "f1=" << factor1 << ", f2=" << factor2
                                << ", diffF1=" << diffFact1 << ", dist="
                                << dist << std::endl;
                                 addUniqueDependency(nodeB, node, negDir, 
                                     MEMORY_TYPE, VARYING_DISTANCE, dist, 0);
                              }
                           }
                        }
                        // if diffFact1>0 && factor1<factor2 
                        // or diffFact1<0 && factor1>factor2
                        // then distance is shrinking. 
                        // Dependencies may change direction in this case
                        // if enough iterations are executed.
                        // Try to compute the minimum number of iterations 
                        // required to have a dependency (not to change dir)
                        else
                        {  // mode = 2;
                           if (distance>0)
                           {
                              if (distance < avgNumIters)
                              {
                           cerr << "Add varying dist depend (4) from " 
                                << *node << " to " << *nodeB << " with "
                                << "f1=" << factor1 << ", f2=" << factor2
                                << ", diffF1=" << diffFact1 << ", dist="
                                << dist << endl;
                                 addUniqueDependency(node, nodeB, CHANGING_DEPENDENCY, 
                                     MEMORY_TYPE, VARYING_DISTANCE, dist, 0);
                              }
                           }
                           else
                           {
                              if ((-distance) < avgNumIters)
                              {
                           std::cerr << "Add varying dist depend (5) from " 
                                << *nodeB << " to " << *node << " with "
                                << "f1=" << factor1 << ", f2=" << factor2
                                << ", diffF1=" << diffFact1 << ", dist="
                                << dist << std::endl;
                                 addUniqueDependency(nodeB, node, CHANGING_DEPENDENCY, 
                                     MEMORY_TYPE, VARYING_DISTANCE, dist, 0);
                              }
                           }
                        }
#endif
                     }  // factors have same sign (positive)
                  }
               }
            } 
         }  // I think I considered all cases
      }
   }  // from else - both accesses are of array type
   return (res);
}

void 
DGBuilder::handle_inner_loop_register_dependencies (SchedDG::Node *node, 
            RSIList &loopRegs, int firstIteration)
{
   RSIList::iterator rlit = loopRegs.begin ();
   for ( ; rlit!=loopRegs.end() ; ++rlit)
   {
      if (rlit->dest==0)
      {
         readsGpRegister (rlit->reg, node, firstIteration, rlit->cycles);
         if (firstIteration)
            node->addRegReads (rlit->reg);
      }
   }
   
   // clear the register maps right here
   gpMapper.clear ();
//   fpMapper.clear ();
   if (crt_stack_tops)
   {
      for (int i=0 ; i<numRegStacks ; ++i)
         crt_stack_tops[i] = max_stack_top_value(i);
   }
   
   // now process the destination registers
   for (rlit=loopRegs.begin() ; rlit!=loopRegs.end() ; ++rlit)
   {
      if (rlit->dest)
      {
         writesGpRegister (rlit->reg, node, firstIteration, rlit->cycles);
         if (firstIteration)
            node->addRegWrites (rlit->reg);
      }
   }
}

void
DGBuilder::handle_intrinsic_register_dependencies (SchedDG::Node* node, 
               int firstIteration)
{
#if 0  // do not handle intrinsic Fortran functions yet
   int regNum;

   unsigned int i = 0;
   EntryValue *entryVal = node->getEntryValue ();
   if (! entryVal)
      return;

   if (firstIteration)
   {
      addr pc = node->getAddress ();
      i = 0;
      regNum = entryVal->spec_inst_ref_src (i);
      while (regNum != NO_REG)
      {
         addr newPc = pc | (regNum << INTRINSIC_REG_SHIFT);
         // do not actually create a new node for input registers.
         // I am not going to add this node to the list of stackLoads.
         // I do not need additional dependencies from later stores to
         // these loads.
         // However, I will create a newNode for output ref registers.
#if 0
         SchedDG::Node* &newNode = builtNodes[newPc];
         if (newNode == NULL)  // newNode could exist if we have the same
                               // ref register on both input and output
         {
            newNode = new Node (newPc, IB_load_fp, this);
            add (newNode);
            node->setInOrderIndex(nextUopIndex++);
            // connect also the newNode to node using a control dependence
            addUniqueDependency (newNode, node, TRUE_DEPENDENCY,
                      CONTROL_TYPE, 0, 0, 0, 0.0);
         }
#endif
         memory_dependencies_for_node (node, newPc, 0, 
                          stackStores, stackLoads, true);
         ++ i;
         regNum = entryVal->spec_inst_ref_src (i);
      }

      i = 0;
      regNum = entryVal->spec_inst_ref_dest (i);
      while (regNum != NO_REG)
      {
         addr newPc = pc | (regNum << INTRINSIC_REG_SHIFT);
         // do not actually create a new node for input registers.
         // I am not going to add this node to the list of stackLoads.
         // I do not need additional dependencies from later stores to
         // these loads.
         // However, I will create a newNode for output ref registers.
         SchedDG::Node* &newNode = builtNodes[newPc];
         if (newNode == NULL)  // newNode could exist if we have the same
                               // ref register on both input and output
         {
            newNode = new Node (newPc, IB_store_fp, this);
            add (newNode);
            node->setInOrderIndex(nextUopIndex++);
            // connect also the newNode to node using a control dependence
            addUniqueDependency (node, newNode, TRUE_DEPENDENCY,
                      CONTROL_TYPE, 0, 0, 0, 0.0);
         }

         memory_dependencies_for_node (newNode, newPc, 1, 
                          stackStores, stackLoads, true);
         stackStores.push_back (newNode);
         ++ i;
         regNum = entryVal->spec_inst_ref_dest (i);
      }
   }
   
   i = 0;
   regNum = entryVal->spec_inst_src (i);
   while(regNum != NO_REG)
   {
      // reg 0 is a special register which is hardwired to value 0
      // It should not cause any dependencies
      if (regNum!=0)
      {
         readsGpRegister (regNum, node, firstIteration);
         if (firstIteration)
            node->addGpReads (regNum);
      }
      ++ i;
      regNum = entryVal->spec_inst_src (i);
   }

   i = 0;
   regNum = entryVal->spec_inst_fp_src (i);
   // f0 and f1 are not special registers on Sparc
   while (regNum != NO_REG)
   {
      readsFpRegister (regNum, node, firstIteration);
      if (firstIteration)
         node->addFpReads (regNum);
      ++ i;
      regNum = entryVal->spec_inst_fp_src (i);
   }

   i = 0;
   regNum = entryVal->spec_inst_dest (i);
   while (regNum != NO_REG)
   {
      // reg 0 is a special register which is hardwired to value 0
      // It should not cause any dependencies
      if (regNum!=0)
      {
         writesGpRegister (regNum, node, firstIteration);
         if (firstIteration)
            node->addGpWrites (regNum);
      }
      ++ i;
      regNum = entryVal->spec_inst_dest (i);
   }
   
   i = 0;
   regNum = entryVal->spec_inst_fp_dest (i);
   while (regNum != NO_REG)
   {
      writesFpRegister (regNum, node, firstIteration);
      if (firstIteration)
         node->addFpWrites (regNum);
      ++ i;
      regNum = entryVal->spec_inst_fp_dest (i);
   }
#endif   // if 0
}

void
DGBuilder::handle_register_dependencies(MIAMI::DecodedInstruction *dInst,
     MIAMI::instruction_info& iiobj, SchedDG::Node* node, 
     MIAMI::CFG::Node* b, int firstIteration)
{
   
   if (prev_inst_may_have_delay_slot)
   {
      if (b->is_delay_block())
      {
         gpRegs = &prevGpMapper;
//         fpRegs = &prevFpMapper;
         stack_tops = prev_stack_tops;
      } else
         prev_inst_may_have_delay_slot = false;
   }
   
   // iterate over all source registers first
   MIAMI::RInfoList regs;
   // fetch all source registers in the regs list
   if (MIAMI::mach_inst_src_registers(dInst, &iiobj, regs))
   {
      MIAMI::RInfoList::iterator rlit = regs.begin();
      for ( ; rlit!=regs.end() ; ++rlit)
      {
         // use the register_actual_name to map AL, AH, AX, EAX, RAX 
         // to the same register. Eventually, I will use the actual
         // bit range information from the registers as well.
         // TODO
         // Do not create dependencies on the instruction pointer register.
         rlit->name = register_name_actual(rlit->name);
         if (*rlit!=MIAMI::MIAMI_NO_REG && rlit->type!=RegisterClass_PSEUDO &&
                !is_instruction_pointer_register(*rlit))
         {
            if (rlit->type == RegisterClass_STACK_OPERATION)
            {
               instruction_has_stack_operation = true;
               stack_index = rlit->stack;
               top_increment = stack_operation_top_increment_value(rlit->name);
            } else 
            {
               if (rlit->type == RegisterClass_STACK_REG)
                  readsStackRegister(*rlit, node, firstIteration);
               else
                  readsGpRegister(*rlit, node, firstIteration);
               
               if (firstIteration)
                  node->addRegReads(*rlit);
            }
         }
      }
   }
   
   // separately, I need to iterate over internal operands to create
   // the needed dependencies between micro-ops
   // I only need to do this for the first iteration pass; 
   // dependencies between micro-ops of the same native instruction
   // cannot have carried dependencies
   uint8_t i = 0;
   if (firstIteration)
   {
      for (i=0 ; i<iiobj.num_src_operands ; ++i)
      {
         int op_num = extract_op_index(iiobj.src_opd[i]);
         switch (extract_op_type(iiobj.src_opd[i]))
         {
            case OperandType_INTERNAL:
            {
               // for internal operands, the operand number 
               // defines the register name
               int rwidth = iiobj.width;
               if (iiobj.exec_unit==ExecUnit_VECTOR)
                     rwidth *= iiobj.vec_len;
               register_info ri(op_num, RegisterClass_TEMP_REG, 0, rwidth-1);
               readsTemporaryRegister(ri, node);
               // internal registers are not actual machine registers on x86,
               // but they could be actual register on RISC architectures.
               // They live only inside one x86 instruction to represent 
               // data movement between micro-ops, thus, they cannot create
               // carried dependencies.
               node->addRegReads(ri);
               break;
            }
            
            // add also immediate values, for pretty drawing dependency graphs
            case OperandType_IMMED:
            {
               node->addImmValue(iiobj.imm_values[op_num]);
               break;
            }
         }
      }
   }
   
   // I need to somehow consider dependencies created by stack
   // push and pop operations. How do they work for x87 operations?
   // TODO, FIXME???


   // fetch all destination registers in the regs list
   if (MIAMI::mach_inst_dest_registers(dInst, &iiobj, regs))
   {
      MIAMI::RInfoList::iterator rlit = regs.begin();
      for ( ; rlit!=regs.end() ; ++rlit)
      {
         rlit->name = register_name_actual(rlit->name);
         if (*rlit!=MIAMI::MIAMI_NO_REG && rlit->type!=RegisterClass_PSEUDO &&
                !is_instruction_pointer_register(*rlit))
         {
            if (rlit->type == RegisterClass_STACK_OPERATION)
            {
               assert(!"I should not have a destination STACK_OPERATION register");
               instruction_has_stack_operation = true;
               stack_index = rlit->stack;
               top_increment = stack_operation_top_increment_value(rlit->name);
            } else 
            {
               if (rlit->type == RegisterClass_STACK_REG)
                  writesStackRegister(*rlit, node, firstIteration);
               else
                  writesGpRegister(*rlit, node, firstIteration);

               if (firstIteration)
                  node->addRegWrites(*rlit);
            }
         }
      }
   }
   
   // separately, I need to iterate over internal operands to create
   // the needed dependencies between micro-ops
   // I only need to do this for the first iteration pass; 
   // dependencies between micro-ops of the same native instruction
   // cannot be carried dependencies
   if (firstIteration)
   {
      for (i=0 ; i<iiobj.num_dest_operands ; ++i)
      {
         int op_num = extract_op_index(iiobj.dest_opd[i]);
         switch (extract_op_type(iiobj.dest_opd[i]))
         {
            case OperandType_INTERNAL:
            {
               // for internal operands, the operand number 
               // defines the register name
               int rwidth = iiobj.width;
               if (iiobj.exec_unit==ExecUnit_VECTOR)
                     rwidth *= iiobj.vec_len;
               register_info ri(op_num, RegisterClass_TEMP_REG, 0, rwidth-1);
               writesTemporaryRegister(ri, node);
               // internal registers are not actual machine registers
               // They live only inside one x86 instruction to represent 
               // data movement between micro-ops.
               node->addRegWrites(ri);
               break;
            }
         }
      }
   }
   
   // I need to somehow consider dependencies created by FPU stack
   // push and pop operations. How do they work for x87 operations?
   // Will this work?
   if (instruction_has_stack_operation)
   {
      assert(stack_tops && stack_index>0 && stack_index-1<numRegStacks);
      stack_tops[stack_index-1] += top_increment;
      instruction_has_stack_operation = false;
   }

   // if the node has a delay instruction, then I have to clear
   // the maps after I process the delay instruction
   if (node->isBarrierNode())
   {
      // clear the register maps right here
      // test first if this instruction can have a delay slot
      if (mach_inst_delay_slot_length (dInst, &iiobj) > 0)  // yes, save the maps
      {
         // I expect not to have a call in the delay slot of another call
         assert (!prev_inst_may_have_delay_slot);
         prevGpMapper = gpMapper;
//         prevFpMapper = fpMapper;
         prev_inst_may_have_delay_slot = true;
         if (crt_stack_tops)
            memcpy(prev_stack_tops, crt_stack_tops, sizeof(int)*numRegStacks);
      }
      gpMapper.clear ();
      if (crt_stack_tops)
      {
         for (i=0 ; i<numRegStacks ; ++i)
            crt_stack_tops[i] = max_stack_top_value(i);
      }
//      fpMapper.clear ();
   }
}

unsigned int
DGBuilder::readsGpRegister(register_info& ri, SchedDG::Node* node,
       int firstIteration, int cycle)
{
   RI2RDMultiMap::iterator it;
   std::pair<RI2RDMultiMap::iterator,RI2RDMultiMap::iterator> match = gpRegs->equal_range(ri);
   // traverse the matches and create dependencies based on the bit range of each match
   int cnt = 0;  // count how many are really matched
   DependencyType dtype = GP_REGISTER_TYPE;
   if (ri.type == RegisterClass_MEM_OP)
      dtype = ADDR_REGISTER_TYPE;
   for (it=match.first ; it!=match.second ; ++it)
   {
      if (ri.OverlapsRange(it->first))  // it is a match made in heaven
      {
         cnt += 1;
         it->second.usedBy.push_back (node);
         // Create a dependency of proper type
         if (firstIteration)
         {
            addUniqueDependency (it->second.definedAt, node, 
                  TRUE_DEPENDENCY, dtype, LOOP_INDEPENDENT, 0,
                  it->second.overlapped+cycle);
         } 
         else if (node->getId() <= it->second.definedAt->getId())
         {
            addUniqueDependency (it->second.definedAt, node, 
                  TRUE_DEPENDENCY, dtype, 1, 1,
                  it->second.overlapped+cycle);
         }
      
      }
   }
   
   if (cnt==0 && ri.stack==0)  // no match found; register was defined before this scope 
   // it means no dependency
   {
      // collect all uses of registers that were defined before this loop
      // into a separate map; once scheduling is over, compute the minimum
      // distance from the start of the schedule to one of its users.
      // Do this only for general purpose registers, not for stack registers since we
      // cannot correctly match the absolute names of the stack registers.
      match = externalGpReg.equal_range (ri);
      bool found = false;
      for (it=match.first ; it!=match.second ; ++it)
      {
         if (ri.isIncludedInRange(it->first))  // full match, I do not need to add it again
         {
            found = true;
            it->second.usedBy.push_back (node);
            break;
         }
      }
      if (! found)   // first use of this register
      {
         RI2RDMultiMap::iterator res = externalGpReg.insert (
                RI2RDMultiMap::value_type (ri, RegData (ri.name, NULL, 0)) );
         res->second.usedBy.push_back (node);
      }
   }
   return (cnt);
}

unsigned int
DGBuilder::readsStackRegister(register_info& ri, SchedDG::Node* node,
       int firstIteration, int cycle)
{
   // for stack registers I have to translate the relative register name
   // to an absolute register name using the stack_tops value;
   // absolute_name = stack_top - relative_name;
   // I start with a top of 0 at the start of a path.
   // NOTE: if absolute_name < 0, then register is defined before loop
   // This assumption is not fully accurate. I think we can write a
   // result to another register than st0 in a few cases.
   // Hmm, start with a stack top of 8 and keep incrementing on push.
   // This provides some buffer for looking for registers back in the stack
   assert(stack_tops and ri.stack>0 && ri.stack-1<numRegStacks);
   ri.name = stack_tops[ri.stack-1] - ri.name;
   if (ri.name>0)
      return (readsGpRegister(ri, node, firstIteration, cycle));
   else
      return (0);
}


unsigned int
DGBuilder::readsTemporaryRegister(register_info& ri, SchedDG::Node* node)
{
   RegMap::iterator it = inMapper.find (ri.name);
   if (it != inMapper.end())   // it is found; it was renamed earlier
   {
      it->second.usedBy.push_back (node);
      addUniqueDependency (it->second.definedAt, node, 
             TRUE_DEPENDENCY, GP_REGISTER_TYPE, LOOP_INDEPENDENT, 0,
             it->second.overlapped);
      
      return (it->second.newName);
   }
   else  // not found; register was not defined before
   {
      // this should not happen for internal registers; they must be
      // defined by a preceeding micro-op. Assert for now.
      assert(!"Internal register use. Cannot find definition in inMapper.");
      return (ri.name);
   }
}


unsigned int
DGBuilder::writesGpRegister(register_info& ri, SchedDG::Node* node,
       int firstIteration, int cycle)
{
   RI2RDMultiMap::iterator it;
   std::pair<RI2RDMultiMap::iterator,RI2RDMultiMap::iterator> match = gpRegs->equal_range(ri);
   // traverse the matches and create dependencies based on the bit range of each match
   int cntov = 0;  // count how many are at least partially overlapped
   int cntin = 0;  // and how many are fully overlapped
   for (it=match.first ; it!=match.second ; )
   {
      bool overlaps = false;
      if (ri.OverlapsRange(it->first))  // it is a match made in heaven
      {
         overlaps = true;
         cntov += 1;
#if GENERATE_ALL_REGISTER_DEP
         Edge* depE;
         if (firstIteration)
         {
            // ADDRESS registers can only be read. So any register
            // output dependencies are marked as GP_REGISTER_TYPE
            depE = addUniqueDependency(rit->second.definedAt, node, 
                     OUTPUT_DEPENDENCY, GP_REGISTER_TYPE, LOOP_INDEPENDENT, 0,
                     rit->second.overlapped);
            depE->MarkRemoved();
            NodeList::iterator nit = rit->second.usedBy.begin();
            for( ; nit!=rit->second.usedBy.end() ; ++nit )
            {
               depE = addUniqueDependency(*nit, node, 
                      ANTI_DEPENDENCY, GP_REGISTER_TYPE, LOOP_INDEPENDENT, 0, 0);
               depE->MarkRemoved();
            }
         } else
            if (node->getId() <= rit->second.definedAt->getId())
            {
               depE = addUniqueDependency(rit->second.definedAt, node, 
                        OUTPUT_DEPENDENCY, GP_REGISTER_TYPE, 1, 1,
                        rit->second.overlapped);
               depE->MarkRemoved();
               NodeList::iterator nit = rit->second.usedBy.begin();
               for( ; nit!=rit->second.usedBy.end() ; ++nit )
                  if (node->getId() != (*nit)->getId())
                  {
                     depE = addUniqueDependency(*nit, node, 
                            ANTI_DEPENDENCY, GP_REGISTER_TYPE, 1, 1, 0);
                     depE->MarkRemoved();
                  }
            }
#endif
      }
      if (ri.IncludesRange(it->first) ||  // I fully overlap the bit range of the old definition, replace it
          ! can_have_unmodified_register_range(ri, it->first))
      {
         cntin += 1;
         // we will delete all matches where current ri more the overlaps 
         // existing entries
         // Then I will add a mnew entry always.
         RI2RDMultiMap::iterator tmp = it++;
         gpRegs->erase(tmp);
      } else
      if (overlaps)
      {
         // subtract the bit range of our new definition from the range of this map entry
         register_info treg(it->first);
         treg.SubtractRange(ri.lsb, ri.msb);
         gpRegs->insert(RI2RDMultiMap::value_type(treg, it->second));
         RI2RDMultiMap::iterator tmp = it++;
         gpRegs->erase(tmp);
      } else
         ++it;
   }
   
   // now add a new entry
   gpRegs->insert(RI2RDMultiMap::value_type(ri, 
                RegData(nextGpReg++, node, cycle)) );
   return (nextGpReg-1);
}

unsigned int
DGBuilder::writesStackRegister(register_info& ri, SchedDG::Node* node,
       int firstIteration, int cycle)
{
   // for stack registers I have to translate the relative register name
   // to an absolute register name using the stack_tops value;
   // absolute_name = stack_top - relative_name;
   // I start with a top of 0 at the start of a path.
   // NOTE: if absolute_name < 0, then register is defined before loop
   // This assumption is not fully accurate. I think we can write a
   // result to another register than st0 in a few cases.
   // Hmm, start with a stack top of 8 and keep incrementing on push.
   // This provides some buffer for looking for registers back in the stack
   assert(stack_tops and ri.stack>0 && ri.stack-1<numRegStacks);
   ri.name = stack_tops[ri.stack-1] - ri.name;
   if (ri.name>0)
      return (writesGpRegister(ri, node, firstIteration, cycle));
   else
      return (0);
}

unsigned int
DGBuilder::writesTemporaryRegister(register_info& ri, SchedDG::Node* node)
{
   RegMap::iterator rit = inMapper.find(ri.name);
   if (rit == inMapper.end())
   {
      inMapper.insert(RegMap::value_type(ri.name, 
                RegData(nextInReg++, node, 0)) );
   } else
   {
      // I should not encounter such a case with internal
      // registers. They simulate the flow of data between 
      // micro-ops. Just assert.
      assert(!"Internal register define. Another definition found in inMapper.");
   }
   return (nextInReg-1);
}

void 
DGBuilder::computeRegisterScheduleInfo (RSIList &reglist)
{
   RI2RDMultiMap::iterator rit;
   reg_sched_info elem;
   
   for (rit=externalGpReg.begin() ; rit!=externalGpReg.end() ; ++rit)
   {
      elem.reg = rit->first;
      elem.dest = false;
      // compute number of cycles to first use
      int cyc = schedule_length;
      NodeList::iterator nlit = rit->second.usedBy.begin ();
      for ( ; nlit!=rit->second.usedBy.end() ; ++nlit)
         if (cyc > (*nlit)->getScheduleTime().ClockCycle())
            cyc = (*nlit)->getScheduleTime().ClockCycle();
      assert (cyc < schedule_length);
      elem.cycles = cyc;
      reglist.push_back (elem);
   }
   for (rit=gpMapper.begin() ; rit!=gpMapper.end() ; ++rit)
   {
      elem.reg = rit->first;
      elem.dest = true;
      SchedDG::Node *tNode = rit->second.definedAt;
      if (tNode && tNode->IsRemoved())  // node was removed, most likely as part of 
                               // a replacement pattern
         tNode = tNode->getReplacementNode ();

      if (tNode != NULL)
      {
         // compute number of cycles from definition to end of schedule
         int cyc = tNode->getScheduleTime().ClockCycle ();
         assert (cyc < schedule_length);
         elem.cycles = schedule_length - cyc - 1 + rit->second.overlapped;
         reglist.push_back (elem);
      }
   }
#if DEBUG_GRAPH_CONSTRUCTION
   DEBUG_GRAPH (3,
      fprintf (stderr, "DGBuilder::computeRegisterScheduleInfo: register map has %ld registers overall.\n",
                reglist.size ());
   )
#endif
}


void
DGBuilder::computeMemoryInformationForPath (/*HashMapUI &refsTable, */
         RGList &globalRGs, CliqueList &allCliques, HashMapUI &refsDist2Use)
{
   UiNLMap liveRefs;
   addrtype reloc = img->RelocationOffset();
   // traverse all references and compute a data structure liveRefs with
   // all references from this path organized by sets
   NodesIterator nit (*this);
   while ((bool) nit)
   {
      Node *nn = nit;
      if (nn->isInstructionNode() && nn->is_memory_reference())
      {
         addrtype npc = nn->getAddress ();
         int32_t iidx = img->GetIndexForInstPC(npc+reloc, nn->memoryOpIndex());
         assert (iidx > 0);  // we should always find a ref index??
         int32_t setIdx = img->GetSetIndexForReference(iidx);
         // check if we have info about this reference. We do not
         // have info about spill/unspill code, so ignore refs w/o info.
         if (setIdx>0)  // found
         {
            UiNLMap::iterator usit = liveRefs.find(setIdx);
            if (usit == liveRefs.end ())  // first ref from this set
            {
               usit = liveRefs.insert (liveRefs.begin(),
                    UiNLMap::value_type (setIdx, NodeList()) );
            }
            usit->second.push_back(nn);
         } else   // did not find it
         {
            // Did not find info about this reference in the recovery file.
            // It must be a spill/unspill instruction. I could place all
            // spill refs in a separate set and treat them the same as the 
            // regular refs. This should be easy to change/add. For now I
            // just ignore them.
#if DEBUG_GRAPH_CONSTRUCTION
            DEBUG_GRAPH (7,
               std::cerr << "WARNING: We do not hava set index for " 
                    << *nn << " No idea why."
                    << std::endl;
            )
#endif
         }
      }
      ++ nit;
   }
   
//   RGList globalRGs;
   
   // At this point we have all the live refs in this path organized on sets
   UiNLMap::iterator nlit = liveRefs.begin ();
   for ( ; nlit!=liveRefs.end() ; ++nlit )
   {
      NodeList &nlist = nlit->second;
      computeRefsReuseGroups (nlit->first, nlist, globalRGs);
   }
   
   // compute also all cliques
   find_memory_parallelism (allCliques, refsDist2Use);
}

// compute reuse groups for refs from the same set. 
// We know that all refs considered at once have the same stride at all
// loop levels.
void
DGBuilder::computeRefsReuseGroups (unsigned int setIndex, NodeList &nlist,
       RGList &globalRGs)
{
   // I need a local list of reuse groups, for refs from this set.
   // Once I process the entire set, I can merge it into a global list
   // of reuse groups.
   // For each ref, check the representant of each group and if it falls
   // into one of them, then check if it becomes the next group leader.
   // All other refs in a group will have associated a temporal distance
   // from the group leader (in clock cycles, according to the computed
   // schedule) and a spatial distance (which should be a constant, smaller
   // than the cache line size or so).
   // Actually, I need to keep a minimum and maximum spatial distance for a 
   // group. It seems very unclear how it should be.
   
   RGList localRGs;    // groups for references from this set
   RGList undefRGs;    // groups for references with uninitialized formulas

#if DEBUG_GRAPH_CONSTRUCTION
   DEBUG_GRAPH (6,
      std::cerr << "Compute reuse groups for setId " << setIndex << std::endl;
   )
#endif

   NodeList::iterator nit = nlist.begin ();
   for ( ; nit!=nlist.end() ; ++nit )
   {
      Node *node = *nit;
      findReuseGroupForNode (node, setIndex, localRGs, undefRGs);
   }
   
   // at the end, add all local reuse groups to the global list
   globalRGs.splice (globalRGs.end(), localRGs);
   globalRGs.splice (globalRGs.end(), undefRGs);
}

void
DGBuilder::findReuseGroupForNode (Node *node, unsigned int setIndex, 
             RGList &listRGs, RGList &undefRGs)
{
   bool found = false;
   RGList::iterator rit = listRGs.begin ();
   addrtype pc = node->getAddress ();
   int opidx = node->memoryOpIndex();
   if (opidx < 0) {
      fprintf(stderr, "Error: DGBuilder::findReuseGroupForNode: Instruction at 0x%" PRIxaddr ", uop of type %s, has memory opidx %d\n",
            pc, Convert_InstrBin_to_string((MIAMI::InstrBin)node->getType()), opidx);
      assert(!"Negative memopidx. Why??");
   }
   RefFormulas *refF = refFormulas.hasFormulasFor(pc,opidx);
   if (refF == NULL)
   {
#if DEBUG_GRAPH_CONSTRUCTION
      DEBUG_GRAPH (5,
         fprintf(stderr, "WARNING: findReuseGroupForNode: Did not find any formulas for memory instruction at address %" PRIxaddr ", opidx %d\n",
                    pc, opidx);
      )
#endif
      // if formula is uninitialized, the reference makes a group by itself
      undefRGs.push_back (new ReuseGroup (pc, opidx,
                             node->getScheduleTime(), setIndex) );
      return;
   }
   
   GFSliceVal& _formula = refF->base;

#if DEBUG_GRAPH_CONSTRUCTION
   DEBUG_GRAPH (6,
      std::cerr << "Find reuse group for node " << *node << " with base formula "
           << _formula << std::endl;
   )
#endif

   if (_formula.is_uninitialized())
   {
#if DEBUG_GRAPH_CONSTRUCTION
      DEBUG_GRAPH (5,
         fprintf(stderr, "WARNING: findReuseGroupForNode: Did not find base formula for memory instruction at address %" PRIxaddr "\n",
                    pc);
      )
#endif
      // if formula is uninitialized, the reference makes a group by itself
      undefRGs.push_back (new ReuseGroup (pc, opidx, 
                             node->getScheduleTime(), setIndex) );
      return;
   }
   int refIsScalar = 0;
   bool constantStride = false;
   coeff_t valueStride = 0;
   GFSliceVal _iterFormula;
   if (avgNumIters <= ONE_ITERATION_EPSILON)
   {
      // only one iteration, consider stride formula is 0
      refIsScalar = 1;
      constantStride = true;
   }
   else   // multiple iterations; check stride formula
   {
      _iterFormula = refF->strides[0];

      if (_iterFormula.is_uninitialized())
      {
#if DEBUG_GRAPH_CONSTRUCTION
         DEBUG_GRAPH (5,
            fprintf(stderr, "WARNING: findReuseGroupForNode: Did not find stride formula for memory instruction at address %" PRIxaddr "\n",
                      pc);
         )
#endif
         undefRGs.push_back (new ReuseGroup (pc, opidx, 
                                node->getScheduleTime(), setIndex) );
         return;
      }
      coeff_t valueNum;
      ucoeff_t valueDen;
      
      if (IsConstantFormula(_iterFormula, valueNum, valueDen))
      {
         constantStride = true;
         if (valueNum==0)
            refIsScalar = 1;
         else
            valueStride = (valueDen==1)?valueNum:(valueNum / valueDen);
      }
   }  // multiple iterations executed

   
   for ( ; rit!=listRGs.end() ; ++rit)
   {
      // get the pc of the ledear and its base formula
      addrtype opc = (*rit)->leaderAddress ();
      int oopidx = (*rit)->leaderMemOpIndex();
      if (oopidx < 0) {
         fprintf(stderr, "Error: DGBuilder::findReuseGroupForNode: Reuse group leader at 0x%" PRIxaddr " has memory opidx %d\n",
               opc, oopidx);
         assert(!"Negative memopidx. Why??");
      }
      RefFormulas *orefF = refFormulas.hasFormulasFor(opc,oopidx);
      assert (orefF);
      GFSliceVal &_Oformula = orefF->base;
      
      // _Oformula must be initialized or it would have been placed in the 
      // undefRGs list
      coeff_t valueNum;
      ucoeff_t valueDen;
      coeff_t ratio, remainder;
      GFSliceVal diffFormula = _Oformula - _formula;

      if (IsConstantFormula (diffFormula, valueNum, valueDen) )
      {
         ratio = 0;
         assert (valueDen != 0);
         remainder = valueNum / valueDen;
         found = true;
      }
      else
         if (!refIsScalar && HasIntegerRatioAndRemainder (diffFormula, 
                   _iterFormula, ratio, remainder) )
         {
            found = true;
         }
         
      if (found)
      {
         // the spatial offset is the inverse of the one computed
         remainder = - remainder;
#if DEBUG_GRAPH_CONSTRUCTION
         DEBUG_GRAPH (6,
            std::cerr << "Reference " << *node << " is " << ratio << " iterations and "
                 << remainder << " bytes after current leader, reference at pc "
                 << std::hex << opc << std::dec << " scheduled at " 
                 << (*rit)->leaderSchedTime() << std::endl;
         )
#endif
         // I do not include the ratio (which is the difference in iteration
         // numbers, or distance) in the delay info. Think if I should include
         // it separately, or merge it with the scheduling delay info?
         // I merge it now.
         (*rit)->addMember (pc, opidx,
                 node->getScheduleTime() % ratio - (*rit)->leaderSchedTime(),
                 remainder);
         break; 
      }
   }
   
   if (! found)   // I could not find a match with any of the existing groups
   {
      // create a new reuse group
      listRGs.push_back (new ReuseGroup (pc, opidx, 
                             node->getScheduleTime(), setIndex, 
                             constantStride, valueStride) );
   }
}

void
DGBuilder::pruneTrivialMemoryDependencies ()
{
   int res = 0;
   if (! gpStores.empty())
      res += pruneMemoryDependencies (&gpStores);
   if (! gpLoads.empty())
      res += pruneMemoryDependencies (&gpLoads);
   if (! fpStores.empty())
      res += pruneMemoryDependencies (&fpStores);
   if (! fpLoads.empty())
      res += pruneMemoryDependencies (&fpLoads);

#if DEBUG_GRAPH_CONSTRUCTION
   DEBUG_GRAPH (1, 
      std::cerr << "PruneTrivialMemoryDep: removed " << res << " dependencies." << std::endl;
      fflush (stderr);
   )
#endif
//   setCfgFlags (CFG_GRAPH_PRUNED);
}


bool 
DGBuilder::isUnreordableNode(SchedDG::Node *node, MIAMI::CFG::Node *b) const
{
   return (node->getType()==IB_inner_loop || isInterproceduralJump(node, b));
}

bool 
DGBuilder::isInterproceduralJump(SchedDG::Node *node, MIAMI::CFG::Node *b) const
{
   if (InstrBinIsBranchType((MIAMI::InstrBin)node->getType()) && b->num_outgoing()==1 &&
         b->firstOutgoing()->sink()->isCallSurrogate())
      return (true);
   else
      return (false);
}

}  /* namespace MIAMI_DG */

