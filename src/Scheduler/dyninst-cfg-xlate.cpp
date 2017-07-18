// -*-Mode: C++;-*-

//*BeginPNNLCopyright********************************************************
//
// $HeadURL$
// $Id$
//
//**********************************************************EndPNNLCopyright*

#include "routine.h"
#include "dyninst-cfg-xlate.hpp"


//***************************************************************************

int dyninst_build_CFG(MIAMI::CFG* cfg, std::string func_name);
void traverse_cfg(MIAMI::CFG* cfg, BPatch_basicBlock* bb, CFG::Node* nn,
		  std::map<int, BPatch_basicBlock*>& blockMap);


//***************************************************************************
// 
//***************************************************************************

int dyninst_build_CFG(MIAMI::CFG* cfg, std::string func_name)
{
  if (!lm_functions->size()) return 0;

  // FIXME: tallent: this searches all routine
  for (unsigned int i = 0; i < lm_functions->size(); ++i)
  {
    if (lm_functions->at(i)->getName().compare(func_name))
    {
      BPatch_flowGraph *fg = lm_functions->at(i)->getCFG();
      fg->createSourceBlocks();
      std::vector<BPatch_basicBlock*> entries;
      fg->getEntryBasicBlock(entries);

      if (entries.size())
        cfg->setCfgFlags(CFG_HAS_ENTRY_POINTS);

      std::map<int, BPatch_basicBlock*> blockMap;
      func_blockVec.clear(); // FIXME:tallent
      
      for (unsigned int i = 0; i < entries.size(); ++i)
      {
        CFG::Node* nn = new CFG::Node(cfg, entries.at(i)->getStartAddress(), entries.at(i)->getEndAddress(), MIAMI::PrivateCFG::MIAMI_CODE_BLOCK);
        traverse_cfg(cfg, entries.at(i), nn, blockMap);
      }
      cfg->computeTopNodes();
      cfg->removeCfgFlags(CFG_GRAPH_IS_MODIFIED);
      cfg->ComputeNodeRanks();
    }
  }
  return 1;
}


// Should make special cases for repeatition block and call surrogate block like MIAMI
// does, or should I directly translating Dyninst block into MIAMI block.

void traverse_cfg(MIAMI::CFG* cfg, BPatch_basicBlock* bb, CFG::Node* nn,
		  std::map<int, BPatch_basicBlock*>& blockMap)
{
  func_blockVec.push_back(bb);

  if (NULL == blockMap[bb->getBlockNumber()])
  {
    blockMap[bb->getBlockNumber()] = bb;

    if (NULL != bb->getCallTarget()) {
      nn = new CFG::Node(cfg, bb->getStartAddress(), bb->getStartAddress(), MIAMI::PrivateCFG::MIAMI_CALL_SITE);
      nn->setTarget(bb->getEndAddress()); // What does this mean?
    }
    cfg->add(nn);

    if (bb->isEntryBlock())
    {
      nn->setRoutineEntry();
      cfg->topNodes.push_back(nn);
      CFG::Edge* edge = new CFG::Edge(static_cast<CFG::Node*>(cfg->cfg_entry), nn, MIAMI::PrivateCFG::MIAMI_CFG_EDGE);
      cfg->add(edge);
    } 

    std::vector<BPatch_basicBlock*> targets;
    bb->getTargets(targets);

    for (unsigned int i = 0; i < targets.size(); ++i)
    {
      CFG::CFG::Node* newNode = new CFG::Node(cfg, targets.at(i)->getStartAddress(), targets.at(i)->getEndAddress(), MIAMI::PrivateCFG::MIAMI_CODE_BLOCK);
      CFG::Edge* newEdge = new CFG::Edge(nn, newNode, MIAMI::PrivateCFG::MIAMI_CFG_EDGE);
      cfg->add(newEdge);        
      traverse_cfg(cfg, targets.at(i), newNode, blockMap);
    }  
    
    if (!targets.size()) //sink
    {
      CFG::Edge* edge = new CFG::Edge(nn, static_cast<CFG::Node*>(cfg->cfg_exit), MIAMI::PrivateCFG::MIAMI_CFG_EDGE);
      cfg->add(edge);
    }
  }
  return;
}


//***************************************************************************
// 
//***************************************************************************

void traverseCFG(BPatch_basicBlock* blk, std::map<BPatch_basicBlock *,bool> seen, std::map<std::string,std::vector<BPatch_basicBlock*> >& paths, std::vector<BPatch_basicBlock*> path, string pathStr, graph& g);


// Not used in the program
void startCFG(BPatch_function* function,std::map<std::string,std::vector<BPatch_basicBlock*> >& paths, graph& g)
{
  if (function!= 0 ) {
    std::map<BPatch_basicBlock *,bool> seen;
    
    BPatch_flowGraph *fg =function->getCFG();		
    
    std::vector<BPatch_basicBlock *> entryBlk;
    std::vector<BPatch_basicBlock *> exitBlk;
    fg->getExitBasicBlock(exitBlk);
    fg->getEntryBasicBlock(entryBlk);
    
    std::string pathStr;
    if (entryBlk.size() > 0 and exitBlk.size()>0){
      g.entry=g.basicBlockNoMap[entryBlk[0]->blockNo()];
      g.exit=g.basicBlockNoMap[exitBlk[0]->blockNo()];
      std::vector<BPatch_basicBlock*> path;
      for(unsigned int b=0;b<entryBlk.size();b++){
        //std::cerr << "startCFG entryblk size is "<<  entryBlk.size() << endl;
	traverseCFG(entryBlk[b],seen,paths,path,pathStr,g);
      }
    }
  }
}


// Called by startCFG. Not used in this program.
void traverseCFG(BPatch_basicBlock* blk, std::map<BPatch_basicBlock *,bool> seen, std::map<std::string,std::vector<BPatch_basicBlock*> >& paths, std::vector<BPatch_basicBlock*> path, string pathStr, graph& g)
{
  seen[blk]=true;
	
  std::string graphBlkNo = std::to_string((long long)g.basicBlockNoMap[blk->blockNo()]);
  pathStr+=graphBlkNo+"->";
	
  path.push_back(blk);
	
  std::vector<BPatch_basicBlock*> targets;
  blk->getTargets(targets);
  for(unsigned int t=0;t<targets.size();t++) {
    if (seen.count(targets[t])==0) {
      traverseCFG(targets[t],seen,paths,path,pathStr,g);
    }
  }
  if (g.exit==g.basicBlockNoMap[blk->blockNo()]) {
    paths[pathStr]=path;
  }
}


//***************************************************************************
// 
//***************************************************************************

// cf. Routine::compute_lineinfo_for_block()
void compute_lineinfo_for_block_dyninst(LoadModule *lm, ScopeImplementation *pscope, CFG::Node *b)
{
   std::vector<unsigned long> addressVec = get_instructions_address_from_block(b);
   //std::cout << "routine: compute_lineinfo_for_block_dyninst: 8\n";
   if (!addressVec.size())
   {
      assert("No instructions available");
      return;
   }
   std::string func, file;
   int32_t lineNumber1 = 0, lineNumber2 = 0;
   
#if DEBUG_COMPUTE_LINEINFO
   std::cerr << "LineInfo for block [" << std::hex << b->getStartAddress() 
             << "," << b->getEndAddress() << "]" << std::dec << " exec-count " 
             << b->ExecCount() << std::endl;
#endif
   // iterate over variable length instructions. Define an architecture independent API
   // for common instruction decoding tasks. That API can be implemented differently for
   // each architecture type
   //CFG::ForwardInstructionIterator iit(b);
   for (unsigned int i = 0; i < addressVec.size(); ++i)
   {
      addrtype pc = addressVec.at(i);
      lm->GetSourceFileInfo(pc, 0, file, func, lineNumber1, lineNumber2);
      unsigned int findex = mdriver.GetFileIndex(file);
      mdriver.AddSourceFileInfo(findex, lineNumber1, lineNumber2);
      pscope->addSourceFileInfo(findex, func, lineNumber1, lineNumber2);
     // std::cout << "routine: lineMappings size is " << pscope->GetLineMappings().size() << endl;
   }
}


// This function is not used.
std::vector<unsigned long> get_instructions_address_from_block(MIAMI::CFG::Node *b)
{
  std::vector<unsigned long> addressVec;
  unsigned long start, end, cur;
  start = b->getStartAddress();
  end = b->getEndAddress();
  if (!func_blockVec.size())
  {
    assert("No blocks available.\n");
  }
  unsigned int i;
  for (i = 0; i < func_blockVec.size(); ++i)
  {
    if (func_blockVec.at(i)->getStartAddress() <= start && func_blockVec.at(i)->getEndAddress() > start)
    {
      break;
    }
  }

  BPatch_basicBlock* bb = func_blockVec.at(i);
  std::vector<Dyninst::InstructionAPI::Instruction::Ptr> insns;
  bb->getInstructions(insns);
  cur = start;

  for (unsigned int i = 0; i < insns.size(); ++i)
  {
    if (cur < end)
    {
      addressVec.push_back((unsigned int) cur);
      cur += insns.at(i)->size();
    } else { break; }
  }
  if (addressVec.size())
  {
    return addressVec;
  } else {
    assert("No block found given the address.\n");
    return addressVec;
  }
}


//***************************************************************************
// 
//***************************************************************************

#if 0

static std::vector<int> blockNums;
//static BPatch_flowGraph *fg;


/* Not used.
bool visited(BPatch_basicBlock* bb){
  for (unsigned int i = 0; i < blockNums.size(); ++i)
  {
    if (bb->getBlockNumber() == blockNums.at(i))
    {
      return true;
    } else {
      blockNums.push_back(bb->getBlockNumber());
      return false;
    }
  }
}
*/

/* Not used.
bool has_REP(BPatch_basicBlock* bb, std::vector<Dyninst::InstructionAPI::Instruction::Ptr> insns, \
                                    std::vector<std::pair<unsigned int, unsigned long>>* REPInsns) {
  unsigned long pc = bb->getStartAddress();
  unsigned long cur = pc;

  for (unsigned int i = 0; i < insns.size(); ++i)
  {
    const Operation* op = &insns.at(i)->getOperation();
    if (op->getPrefixID() == prefix_rep || op->getPrefixID() == prefix_repnz) {
      //std::cout << "has_REP: op is " << op->format() << endl;
      std::pair<unsigned int, unsigned long> idxAddr;
      idxAddr.first = i;
      idxAddr.second = cur;
      REPInsns->push_back(idxAddr);
     } 
    cur += insns.at(i)->size();
  }
  if (REPInsns->size()) {
    //std::cout << "has_REP: size is " << REPInsns->size() << endl;
    return true;
  } else {
    return false;
  }
}
*/

/* Not used.
bool has_branch(BPatch_basicBlock* bb, std::vector<Dyninst::InstructionAPI::Instruction::Ptr> insns, \
                                  std::vector<std::pair<unsigned int, unsigned long>> branchInsns, std::string func_name){
  unsigned long pc = bb->getStartAddress();
  for (unsigned int i = 0; i < insns.size(); ++i) 
  {
    if (insns.at(i)->getCategory() == c_BranchInsn || insns.at(i)->getCategory() == c_CallInsn || \
                                                      insns.at(i)->getCategory() == c_ReturnInsn)
    {
      std::pair <unsigned int, unsigned long> idxAddr;
      idxAddr.first = i;
      idxAddr.second = pc;
      branchInsns.push_back(idxAddr);
    }
    pc += insns.at(i)->size();
  }
  //std::cout << "has_branch: size is " << branchInsns.size() << "\n";
  if (branchInsns.size())
    return true;
  else 
    return false;
}*/


// This function is not used for translating the Dyninst CFG to MIAMI CFG. It is
// used in CFGTool folder in external3/MIAMI
/*
void process_block(BPatch_basicBlock* bb, MIAMI::Routine * robj, std::string func_name){
  if (!visited(bb))
    {
      std::vector<std::pair<unsigned int, unsigned long>> REPInsns;
      std::vector<std::pair<unsigned int, unsigned long>> branchInsns;
      std::vector<Dyninst::InstructionAPI::Instruction::Ptr> insns;
      bb->getInstructions(insns);

      unsigned long start = bb->getStartAddress();
      unsigned long end = bb->getEndAddress();
      unsigned long lpc, pc;
      // has REP
      if (has_REP(bb, insns, &REPInsns))
      {
       // std::cout << "process_block: size is " << REPInsns.size() << endl;
        unsigned int j = 0;
        while (j < REPInsns.size() && start < end){
          lpc = REPInsns.at(j).second;
         // std::cout << "process_block: lpc is " << lpc << endl;
          pc = lpc + insns.at(REPInsns.at(j).first)->size();
         // std::cout << "process_block: pc is " << pc << endl;
         // std::cout << "process_block: start is " << start << " end is " << end << endl;
          
          if(lpc > start) {
            robj->AddBlock(start, lpc, MIAMI::PrivateCFG::MIAMI_CODE_BLOCK, 0);
            robj->AddPotentialEdge(lpc, lpc, MIAMI::PrivateCFG::MIAMI_FALLTHRU_EDGE);
          }
          robj->AddBlock(lpc, pc, MIAMI::PrivateCFG::MIAMI_REP_BLOCK, 0);
          // add a loop edge for the REP instruction
          robj->AddPotentialEdge(pc, lpc, MIAMI::PrivateCFG::MIAMI_DIRECT_BRANCH_EDGE);
          // I also need to create a bypass edge to go around the REP instruction
          // in case that its count is set to zero.
          if (pc < end) robj->AddPotentialEdge(lpc, pc, MIAMI::PrivateCFG::MIAMI_BYPASS_EDGE); //FIXME
          start = pc;
          j++;
        }

        if (start < end) {
          robj->AddBlock(pc, end, MIAMI::PrivateCFG::MIAMI_CODE_BLOCK, 0);
          robj->AddPotentialEdge(pc, pc, MIAMI::PrivateCFG::MIAMI_FALLTHRU_EDGE);
        }

      }
      // has branch
      else if (has_branch(bb, insns, branchInsns, func_name))
      {
        unsigned int j = 0;
        while (j < branchInsns.size() && start < end) {
          lpc = branchInsns.at(j).second;
          pc = lpc + insns.at(branchInsns.at(j).first)->size();
          InstructionAPI::Instruction::cftConstIter cftBegin = insns.at(branchInsns.at(j).first)->cft_begin();
          InstructionAPI::Instruction::cftConstIter cftEnd = insns.at(branchInsns.at(j).first)->cft_end();

          for (auto cit = cftBegin; cit != cftEnd; cit++)
          {
            if (cit->isCall || cit->isConditional) // conditional (IB_br_CC) and unconditional (IB_branch, CALL) branching statements
            {
              unsigned long targetAddr;
              if (cit->target->eval().defined) {
                targetAddr = cit->target->eval().convert<unsigned long>(); // CHECK unsigned??
              //  std::cout << "process_block: unconditional: targetAddr is " << hex << targetAddr << dec << endl;
              //  std::cout << "process_block 6\n";
                robj->AddPotentialEdge(pc, targetAddr, MIAMI::PrivateCFG::MIAMI_DIRECT_BRANCH_EDGE);
                if (cit->isConditional)
                {
                  BPatch_basicBlock* b = fg->findBlockByAddr((Dyninst::Address) targetAddr);
                  if (!visited(b)) process_block(b, robj, func_name);  
                }
              } else {
                // cannot find target
              }
          
              
            } else if (cit->isIndirect) // IB_jump
            {
              if (insns.at(branchInsns.at(j).first)->getCategory() == c_ReturnInsn) // return instruction
              {
                robj->AddReturnEdge(pc);

              } else if (cit->target->eval().defined) { // indirect jump with defined target

                unsigned long targetAddr;
                targetAddr = cit->target->eval().convert<unsigned long>(); // CHECK unsigned??
                robj->AddCallSurrogateBlock(lpc, pc, targetAddr, MIAMI::PrivateCFG::MIAMI_FALLTHRU_EDGE, true); // fall through edge is created here

                BPatch_basicBlock* b = fg->findBlockByAddr((Dyninst::Address) targetAddr);
                if (!visited(b)) process_block(b, robj, func_name);
                 
              }
              
            } else if (cit->isFallthrough) // what is this??
            {
              if (pc < end)
                robj->AddPotentialEdge(pc, pc, MIAMI::PrivateCFG::MIAMI_FALLTHRU_EDGE);
              else 
                robj->AddReturnEdge(pc, MIAMI::PrivateCFG::MIAMI_FALLTHRU_EDGE);
              

              if (cit->target->eval().defined) { 

                unsigned long targetAddr;
                targetAddr = cit->target->eval().convert<unsigned long>(); // CHECK unsigned??
                BPatch_basicBlock* b = fg->findBlockByAddr((Dyninst::Address) targetAddr);
                if (!visited(b)) process_block(b, robj, func_name);
              }
              
            } // categories
          } // for cfts per insn
          j++;
        } // while each branch insn
      } // if has branch
      // normal blocks
      else {
        //std::cout << "process_block: normal add block\n";
        robj->AddBlock(start, end, MIAMI::PrivateCFG::MIAMI_CODE_BLOCK, 0);
      }
    } else {
      return;
    }
}
*/

#endif
