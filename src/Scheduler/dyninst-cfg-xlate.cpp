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
