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
//extern "C" {
// #include "xed-interface.h"
//}

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
      uint64_t _pathFreq, float _avgNumIters) 
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
//   std::cout<<"blkPath: "<<routine->getBlockNoFromAddr(ba[0]->getStartAddress(),ba[0]->getEndAddress())<<":"<<ba[0]->getType()<<std::endl;
   minPathAddress = ba[0]->getStartAddress();
   maxPathAddress = ba[0]->getEndAddress();
//   std::cout<<"MinAddr: "<<minPathAddress<<" MaxAddr: "<<maxPathAddress<<std::endl;
   // gmarin, 09/11/2013: The first block can be an inner loop, and
   // inner loop nodes do not have an associated CFG, so the reloc_offset
   // is always zero. Can all the blocks of a path be inner loops?
   // Probabaly not, but it is safer to get it from the img object.
   reloc_offset = img->RelocationOffset();  //ba[0]->RelocationOffset();

   for( int i=1 ; i<numBlocks ; ++i )
   {
//      std::cout<<"->"<<routine->getBlockNoFromAddr(ba[i]->getStartAddress(),ba[i]->getEndAddress())<<":"<<ba[i]->getType();
      if (minPathAddress > ba[i]->getStartAddress())
         minPathAddress = ba[i]->getStartAddress();
      if (maxPathAddress < ba[i]->getEndAddress())
         maxPathAddress = ba[i]->getEndAddress();
   }
//   std::cout<<std::endl;
//   std::cout<<"probs: "<<endl;
//   for( int i=1 ; i<numBlocks ; ++i ){
//      std::cout<<fa[i]<<endl;
//   }
//   std::cout<<std::endl;
//   std::cout<<"_pathFreq: "<<_pathFreq<<" avgNumIters: "<<_avgNumIters<<endl;
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
   //   calculateMemoryData(-1);

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

typedef enum {
  XED_REG_INVALID,
  XED_REG_BNDCFGU,
  XED_REG_BNDSTATUS,
  XED_REG_BND0,
  XED_REG_BND1,
  XED_REG_BND2,
  XED_REG_BND3,
  XED_REG_CR0,
  XED_REG_CR1,
  XED_REG_CR2,
  XED_REG_CR3,
  XED_REG_CR4,
  XED_REG_CR5,
  XED_REG_CR6,
  XED_REG_CR7,
  XED_REG_CR8,
  XED_REG_CR9,
  XED_REG_CR10,
  XED_REG_CR11,
  XED_REG_CR12,
  XED_REG_CR13,
  XED_REG_CR14,
  XED_REG_CR15,
  XED_REG_DR0,
  XED_REG_DR1,
  XED_REG_DR2,
  XED_REG_DR3,
  XED_REG_DR4,
  XED_REG_DR5,
  XED_REG_DR6,
  XED_REG_DR7,
  XED_REG_DR8,
  XED_REG_DR9,
  XED_REG_DR10,
  XED_REG_DR11,
  XED_REG_DR12,
  XED_REG_DR13,
  XED_REG_DR14,
  XED_REG_DR15,
  XED_REG_FLAGS,
  XED_REG_EFLAGS,
  XED_REG_RFLAGS,
  XED_REG_AX,
  XED_REG_CX,
  XED_REG_DX,
  XED_REG_BX,
  XED_REG_SP,
  XED_REG_BP,
  XED_REG_SI,
  XED_REG_DI,
  XED_REG_R8W,
  XED_REG_R9W,
  XED_REG_R10W,
  XED_REG_R11W,
  XED_REG_R12W,
  XED_REG_R13W,
  XED_REG_R14W,
  XED_REG_R15W,
  XED_REG_EAX,
  XED_REG_ECX,
  XED_REG_EDX,
  XED_REG_EBX,
  XED_REG_ESP,
  XED_REG_EBP,
  XED_REG_ESI,
  XED_REG_EDI,
  XED_REG_R8D,
  XED_REG_R9D,
  XED_REG_R10D,
  XED_REG_R11D,
  XED_REG_R12D,
  XED_REG_R13D,
  XED_REG_R14D,
  XED_REG_R15D,
  XED_REG_RAX,
  XED_REG_RCX,
  XED_REG_RDX,
  XED_REG_RBX,
  XED_REG_RSP,
  XED_REG_RBP,
  XED_REG_RSI,
  XED_REG_RDI,
  XED_REG_R8,
  XED_REG_R9,
  XED_REG_R10,
  XED_REG_R11,
  XED_REG_R12,
  XED_REG_R13,
  XED_REG_R14,
  XED_REG_R15,
  XED_REG_AL,
  XED_REG_CL,
  XED_REG_DL,
  XED_REG_BL,
  XED_REG_SPL,
  XED_REG_BPL,
  XED_REG_SIL,
  XED_REG_DIL,
  XED_REG_R8B,
  XED_REG_R9B,
  XED_REG_R10B,
  XED_REG_R11B,
  XED_REG_R12B,
  XED_REG_R13B,
  XED_REG_R14B,
  XED_REG_R15B,
  XED_REG_AH,
  XED_REG_CH,
  XED_REG_DH,
  XED_REG_BH,
  XED_REG_ERROR,
  XED_REG_RIP,
  XED_REG_EIP,
  XED_REG_IP,
  XED_REG_K0,
  XED_REG_K1,
  XED_REG_K2,
  XED_REG_K3,
  XED_REG_K4,
  XED_REG_K5,
  XED_REG_K6,
  XED_REG_K7,
  XED_REG_MMX0,
  XED_REG_MMX1,
  XED_REG_MMX2,
  XED_REG_MMX3,
  XED_REG_MMX4,
  XED_REG_MMX5,
  XED_REG_MMX6,
  XED_REG_MMX7,
  XED_REG_MXCSR,
  XED_REG_STACKPUSH,
  XED_REG_STACKPOP,
  XED_REG_GDTR,
  XED_REG_LDTR,
  XED_REG_IDTR,
  XED_REG_TR,
  XED_REG_TSC,
  XED_REG_TSCAUX,
  XED_REG_MSRS,
  XED_REG_FSBASE,
  XED_REG_GSBASE,
  XED_REG_X87CONTROL,
  XED_REG_X87STATUS,
  XED_REG_X87TAG,
  XED_REG_X87PUSH,
  XED_REG_X87POP,
  XED_REG_X87POP2,
  XED_REG_X87OPCODE,
  XED_REG_X87LASTCS,
  XED_REG_X87LASTIP,
  XED_REG_X87LASTDS,
  XED_REG_X87LASTDP,
  XED_REG_CS,
  XED_REG_DS,
  XED_REG_ES,
  XED_REG_SS,
  XED_REG_FS,
  XED_REG_GS,
  XED_REG_TMP0,
  XED_REG_TMP1,
  XED_REG_TMP2,
  XED_REG_TMP3,
  XED_REG_TMP4,
  XED_REG_TMP5,
  XED_REG_TMP6,
  XED_REG_TMP7,
  XED_REG_TMP8,
  XED_REG_TMP9,
  XED_REG_TMP10,
  XED_REG_TMP11,
  XED_REG_TMP12,
  XED_REG_TMP13,
  XED_REG_TMP14,
  XED_REG_TMP15,
  XED_REG_ST0,
  XED_REG_ST1,
  XED_REG_ST2,
  XED_REG_ST3,
  XED_REG_ST4,
  XED_REG_ST5,
  XED_REG_ST6,
  XED_REG_ST7,
  XED_REG_XCR0,
  XED_REG_XMM0,
  XED_REG_XMM1,
  XED_REG_XMM2,
  XED_REG_XMM3,
  XED_REG_XMM4,
  XED_REG_XMM5,
  XED_REG_XMM6,
  XED_REG_XMM7,
  XED_REG_XMM8,
  XED_REG_XMM9,
  XED_REG_XMM10,
  XED_REG_XMM11,
  XED_REG_XMM12,
  XED_REG_XMM13,
  XED_REG_XMM14,
  XED_REG_XMM15,
  XED_REG_XMM16,
  XED_REG_XMM17,
  XED_REG_XMM18,
  XED_REG_XMM19,
  XED_REG_XMM20,
  XED_REG_XMM21,
  XED_REG_XMM22,
  XED_REG_XMM23,
  XED_REG_XMM24,
  XED_REG_XMM25,
  XED_REG_XMM26,
  XED_REG_XMM27,
  XED_REG_XMM28,
  XED_REG_XMM29,
  XED_REG_XMM30,
  XED_REG_XMM31,
  XED_REG_YMM0,
  XED_REG_YMM1,
  XED_REG_YMM2,
  XED_REG_YMM3,
  XED_REG_YMM4,
  XED_REG_YMM5,
  XED_REG_YMM6,
  XED_REG_YMM7,
  XED_REG_YMM8,
  XED_REG_YMM9,
  XED_REG_YMM10,
  XED_REG_YMM11,
  XED_REG_YMM12,
  XED_REG_YMM13,
  XED_REG_YMM14,
  XED_REG_YMM15,
  XED_REG_YMM16,
  XED_REG_YMM17,
  XED_REG_YMM18,
  XED_REG_YMM19,
  XED_REG_YMM20,
  XED_REG_YMM21,
  XED_REG_YMM22,
  XED_REG_YMM23,
  XED_REG_YMM24,
  XED_REG_YMM25,
  XED_REG_YMM26,
  XED_REG_YMM27,
  XED_REG_YMM28,
  XED_REG_YMM29,
  XED_REG_YMM30,
  XED_REG_YMM31,
  XED_REG_ZMM0,
  XED_REG_ZMM1,
  XED_REG_ZMM2,
  XED_REG_ZMM3,
  XED_REG_ZMM4,
  XED_REG_ZMM5,
  XED_REG_ZMM6,
  XED_REG_ZMM7,
  XED_REG_ZMM8,
  XED_REG_ZMM9,
  XED_REG_ZMM10,
  XED_REG_ZMM11,
  XED_REG_ZMM12,
  XED_REG_ZMM13,
  XED_REG_ZMM14,
  XED_REG_ZMM15,
  XED_REG_ZMM16,
  XED_REG_ZMM17,
  XED_REG_ZMM18,
  XED_REG_ZMM19,
  XED_REG_ZMM20,
  XED_REG_ZMM21,
  XED_REG_ZMM22,
  XED_REG_ZMM23,
  XED_REG_ZMM24,
  XED_REG_ZMM25,
  XED_REG_ZMM26,
  XED_REG_ZMM27,
  XED_REG_ZMM28,
  XED_REG_ZMM29,
  XED_REG_ZMM30,
  XED_REG_ZMM31,
  XED_REG_LAST,
  XED_REG_BNDCFG_FIRST=XED_REG_BNDCFGU, //< PSEUDO
  XED_REG_BNDCFG_LAST=XED_REG_BNDCFGU, //<PSEUDO
  XED_REG_BNDSTAT_FIRST=XED_REG_BNDSTATUS, //< PSEUDO
  XED_REG_BNDSTAT_LAST=XED_REG_BNDSTATUS, //<PSEUDO
  XED_REG_BOUND_FIRST=XED_REG_BND0, //< PSEUDO
  XED_REG_BOUND_LAST=XED_REG_BND3, //<PSEUDO
  XED_REG_CR_FIRST=XED_REG_CR0, //< PSEUDO
  XED_REG_CR_LAST=XED_REG_CR15, //<PSEUDO
  XED_REG_DR_FIRST=XED_REG_DR0, //< PSEUDO
  XED_REG_DR_LAST=XED_REG_DR15, //<PSEUDO
  XED_REG_FLAGS_FIRST=XED_REG_FLAGS, //< PSEUDO
  XED_REG_FLAGS_LAST=XED_REG_RFLAGS, //<PSEUDO
  XED_REG_GPR16_FIRST=XED_REG_AX, //< PSEUDO
  XED_REG_GPR16_LAST=XED_REG_R15W, //<PSEUDO
  XED_REG_GPR32_FIRST=XED_REG_EAX, //< PSEUDO
  XED_REG_GPR32_LAST=XED_REG_R15D, //<PSEUDO
  XED_REG_GPR64_FIRST=XED_REG_RAX, //< PSEUDO
  XED_REG_GPR64_LAST=XED_REG_R15, //<PSEUDO
  XED_REG_GPR8_FIRST=XED_REG_AL, //< PSEUDO
  XED_REG_GPR8_LAST=XED_REG_R15B, //<PSEUDO
  XED_REG_GPR8H_FIRST=XED_REG_AH, //< PSEUDO
  XED_REG_GPR8H_LAST=XED_REG_BH, //<PSEUDO
  XED_REG_INVALID_FIRST=XED_REG_INVALID, //< PSEUDO
  XED_REG_INVALID_LAST=XED_REG_ERROR, //<PSEUDO
  XED_REG_IP_FIRST=XED_REG_RIP, //< PSEUDO
  XED_REG_IP_LAST=XED_REG_IP, //<PSEUDO
  XED_REG_MASK_FIRST=XED_REG_K0, //< PSEUDO
  XED_REG_MASK_LAST=XED_REG_K7, //<PSEUDO
  XED_REG_MMX_FIRST=XED_REG_MMX0, //< PSEUDO
  XED_REG_MMX_LAST=XED_REG_MMX7, //<PSEUDO
  XED_REG_MXCSR_FIRST=XED_REG_MXCSR, //< PSEUDO
  XED_REG_MXCSR_LAST=XED_REG_MXCSR, //<PSEUDO
  XED_REG_PSEUDO_FIRST=XED_REG_STACKPUSH, //< PSEUDO
  XED_REG_PSEUDO_LAST=XED_REG_GSBASE, //<PSEUDO
  XED_REG_PSEUDOX87_FIRST=XED_REG_X87CONTROL, //< PSEUDO
  XED_REG_PSEUDOX87_LAST=XED_REG_X87LASTDP, //<PSEUDO
  XED_REG_SR_FIRST=XED_REG_CS, //< PSEUDO
  XED_REG_SR_LAST=XED_REG_GS, //<PSEUDO
  XED_REG_TMP_FIRST=XED_REG_TMP0, //< PSEUDO
  XED_REG_TMP_LAST=XED_REG_TMP15, //<PSEUDO
  XED_REG_X87_FIRST=XED_REG_ST0, //< PSEUDO
  XED_REG_X87_LAST=XED_REG_ST7, //<PSEUDO
  XED_REG_XCR_FIRST=XED_REG_XCR0, //< PSEUDO
  XED_REG_XCR_LAST=XED_REG_XCR0, //<PSEUDO
  XED_REG_XMM_FIRST=XED_REG_XMM0, //< PSEUDO
  XED_REG_XMM_LAST=XED_REG_XMM31, //<PSEUDO
  XED_REG_YMM_FIRST=XED_REG_YMM0, //< PSEUDO
  XED_REG_YMM_LAST=XED_REG_YMM31, //<PSEUDO
  XED_REG_ZMM_FIRST=XED_REG_ZMM0, //< PSEUDO
  XED_REG_ZMM_LAST=XED_REG_ZMM31 //<PSEUDO
} xed_reg_enum_t;

void DGBuilder::addPTWSnippet(Dyninst::PatchAPI::Patcher *patcher, Dyninst::PatchAPI::Point* new_point, Node *nn){

   // Construct our snippet that will emit PTWRITE instruction
   Dyninst::PatchAPI::Snippet::Ptr ptrEAX  = PTWriteSnippetEAX::create(new PTWriteSnippetEAX());
   Dyninst::PatchAPI::Snippet::Ptr ptrEBX = PTWriteSnippetEBX::create(new PTWriteSnippetEBX());
   Dyninst::PatchAPI::Snippet::Ptr ptrECX = PTWriteSnippetECX::create(new PTWriteSnippetECX());
   Dyninst::PatchAPI::Snippet::Ptr ptrEDX = PTWriteSnippetEDX::create(new PTWriteSnippetEDX());
   Dyninst::PatchAPI::Snippet::Ptr ptrRAX = PTWriteSnippetRAX::create(new PTWriteSnippetRAX());
   Dyninst::PatchAPI::Snippet::Ptr ptrRBX = PTWriteSnippetRBX::create(new PTWriteSnippetRBX());
   Dyninst::PatchAPI::Snippet::Ptr ptrRCX = PTWriteSnippetRCX::create(new PTWriteSnippetRCX());
   Dyninst::PatchAPI::Snippet::Ptr ptrRDX = PTWriteSnippetRDX::create(new PTWriteSnippetRDX());
   Dyninst::PatchAPI::Snippet::Ptr ptrRBP = PTWriteSnippetRBP::create(new PTWriteSnippetRBP());
   Dyninst::PatchAPI::Snippet::Ptr ptrRSP = PTWriteSnippetRSP::create(new PTWriteSnippetRSP());
   Dyninst::PatchAPI::Snippet::Ptr ptrRSI = PTWriteSnippetRSI::create(new PTWriteSnippetRSI());
   Dyninst::PatchAPI::Snippet::Ptr ptrRDI = PTWriteSnippetRDI::create(new PTWriteSnippetRDI());
   Dyninst::PatchAPI::Snippet::Ptr ptrR8 = PTWriteSnippetR8::create(new PTWriteSnippetR8());
   Dyninst::PatchAPI::Snippet::Ptr ptrR9 = PTWriteSnippetR9::create(new PTWriteSnippetR9());
   Dyninst::PatchAPI::Snippet::Ptr ptrR10 = PTWriteSnippetR10::create(new PTWriteSnippetR10());
   Dyninst::PatchAPI::Snippet::Ptr ptrR11 = PTWriteSnippetR11::create(new PTWriteSnippetR11());
   Dyninst::PatchAPI::Snippet::Ptr ptrR12 = PTWriteSnippetR12::create(new PTWriteSnippetR12());
   Dyninst::PatchAPI::Snippet::Ptr ptrR13 = PTWriteSnippetR13::create(new PTWriteSnippetR13());
   Dyninst::PatchAPI::Snippet::Ptr ptrR14 = PTWriteSnippetR14::create(new PTWriteSnippetR14());
   Dyninst::PatchAPI::Snippet::Ptr ptrR15 = PTWriteSnippetR15::create(new PTWriteSnippetR15());

   int reg;
   RInfoList srcreg = nn->getSrcReg();
   RInfoList destreg = nn->getDestReg();
   std::cout<<"Node has "<<srcreg.size()<<" src regs\n";
   std::cout<<"Node has "<<destreg.size()<<" dest regs\n";
   RInfoList::iterator nnrit;
   if (nn->is_store_instruction()) {
      nnrit = destreg.begin();
      for( ; nnrit!=destreg.end() ; ++nnrit ) {
         std::cout<<"Print Dest reg info\n"<<nnrit->ToString()<<std::endl;
         std::cout<<"Print Src reg Name: "<<nnrit->name<<std::endl;
         reg = nnrit->name;  
         if(new_point){
            switch (reg){
               case  XED_REG_EAX:
                  cerr << "PTW eax  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrEAX));
                  break;	    		
               case XED_REG_EBX:
                  cerr << "PTW ebx  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrEBX));
                  break;	    		
               case XED_REG_ECX:
                  cerr << "PTW ecx  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrECX));
                  break;	    		
               case XED_REG_EDX:
                  cerr << "PTW edx  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrEDX));
                  break;	    		
               case XED_REG_RAX:
                  cerr << "PTW rax  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRAX));
                  break;	    		
               case XED_REG_RBX:
                  cerr << "PTW rbx  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRBX));
                  break;	    		
               case XED_REG_RCX:
                  cerr << "PTW rcx  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRCX));
                  break;	    		
               case XED_REG_RDX:
                  cerr << "PTW rdx  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRDX));
                  break;	    		
               case XED_REG_RBP:
                  cerr << "PTW rbp  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRBP));
                  break;	    		
               case XED_REG_RSP:
                  cerr << "PTW rsp  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRSP));
                  break;	    		
               case XED_REG_RSI:
                  cerr << "PTW rsi  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRSI));
                  break;	    		
               case XED_REG_RDI:
                  cerr << "PTW rdi  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRDI));
                  break;	    		
               case XED_REG_R8:
                  cerr << "PTW r8  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR8));
                  break;	    		
               case XED_REG_R9:
                  cerr << "PTW r9  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR9));
                  break;	    		
               case XED_REG_R10:
                  cerr << "PTW r10  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR10));
                  break;	    		
               case XED_REG_R11:
                  cerr << "PTW r11  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR11));
                  break;	    		
               case XED_REG_R12:
                  cerr << "PTW r12  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR12));
                  break;	    		
               case XED_REG_R13:
                  cerr << "PTW r13  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR13));
                  break;	    		
               case XED_REG_R14:
                  cerr << "PTW r14  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR14));
                  break;	    		
               case XED_REG_R15:
                  cerr << "PTW r15  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR15)); 
                  break;	    		
               default:
                  cerr << "Register could not found\n"; 
                  break;	    		
            }
         }  
      }
   }
   
   if (nn->is_load_instruction()){
      nnrit = srcreg.begin();
      for( ; nnrit!=srcreg.end() ; ++nnrit ) {
         std::cout<<"Print Src reg info\n"<<nnrit->ToString()<<std::endl;
         std::cout<<"Print Src reg Name: "<<nnrit->name<<std::endl;
         reg = nnrit->name;  
         if(new_point){
            switch (reg){
               case  XED_REG_EAX:
                  cerr << "PTW eax  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrEAX));
                  break;	    		
               case XED_REG_EBX:
                  cerr << "PTW ebx  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrEBX));
                  break;	    		
               case XED_REG_ECX:
                  cerr << "PTW ecx  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrECX));
                  break;	    		
               case XED_REG_EDX:
                  cerr << "PTW edx  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrEDX));
                  break;	    		
               case XED_REG_RAX:
                  cerr << "PTW rax  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRAX));
                  break;	    		
               case XED_REG_RBX:
                  cerr << "PTW rbx  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRBX));
                  break;	    		
               case XED_REG_RCX:
                  cerr << "PTW rcx  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRCX));
                  break;	    		
               case XED_REG_RDX:
                  cerr << "PTW rdx  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRDX));
                  break;	    		
               case XED_REG_RBP:
                  cerr << "PTW rbp  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRBP));
                  break;	    		
               case XED_REG_RSP:
                  cerr << "PTW rsp  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRSP));
                  break;	    		
               case XED_REG_RSI:
                  cerr << "PTW rsi  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRSI));
                  break;	    		
               case XED_REG_RDI:
                  cerr << "PTW rdi  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrRDI));
                  break;	    		
               case XED_REG_R8:
                  cerr << "PTW r8  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR8));
                  break;	    		
               case XED_REG_R9:
                  cerr << "PTW r9  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR9));
                  break;	    		
               case XED_REG_R10:
                  cerr << "PTW r10  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR10));
                  break;	    		
               case XED_REG_R11:
                  cerr << "PTW r11  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR11));
                  break;	    		
               case XED_REG_R12:
                  cerr << "PTW r12  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR12));
                  break;	    		
               case XED_REG_R13:
                  cerr << "PTW r13  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR13));
                  break;	    		
               case XED_REG_R14:
                  cerr << "PTW r14  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR14));
                  break;	    		
               case XED_REG_R15:
                  cerr << "PTW r15  @ " << new_point << endl;
                  patcher->add(Dyninst::PatchAPI::PushBackCommand::create(new_point, ptrR15)); 
                  break;	    		
               default:
                  cerr << "Register could not found\n"; 
                  break;	    		
            }
         }
      }
   }

}


float DGBuilder::printLoadClassifications(const MIAMI::MiamiOptions *mo){
   NodesIterator fnit(*this);
   LoadModule *img =this->img;
   Routine *rout = this->routine;

   BPatch_image* image =  img->getDyninstImage();
   BPatch_Vector<BPatch_function*> funcs;
   BPatch_point* loadPtr;
   Dyninst::Address daddr;
   Dyninst::PatchAPI::Point* lps;
   std::vector<Dyninst::PatchAPI::Point*> points ; 
   bool func_exist = true;   
      std::cout<<"OZGURDBG::func name is : "<<rout->Name()<<std::endl;
   if(rout->Name().find("@") != std::string::npos){
      std::cout<<"OZGURDBG::func name has @ : "<<rout->Name()<<std::endl;
      func_exist  = false;
   } else if (image->findFunction(rout->Name().c_str(), funcs,
                     /* showError */ true,
                     /* regex_case_sensitive */ true,
                     /* incUninstrumentable */ true) == nullptr ||
                     !funcs.size() || funcs[0] == nullptr) {
      func_exist = false;
// TODO add a if switch to enable and disable based on load/store
   }
   BPatch_binaryEdit *DynApp = img->getDyninstBinEdit();
   Dyninst::PatchAPI::PatchMgrPtr patchMgr = Dyninst::PatchAPI::convert(DynApp);
   Dyninst::PatchAPI::Patcher patcher(patchMgr);

   double total_lds = 0;
   double frame_lds = 0;
   double strided_lds = 0;
   double indirect_lds = 0;
   double total_sts = 0;
   double frame_sts = 0;
   double strided_sts = 0;
   double indirect_sts = 0;
   std::cout<<"Address\ttype\tclass\n";
   while ((bool)fnit) {
            Node *nn = fnit;
            int opidx = nn->memoryOpIndex();
            if (opidx >=0){
               RefFormulas *refF = nn->in_cfg()->refFormulas.hasFormulasFor(nn->getAddress(), opidx);
               if(refF != NULL){
                  GFSliceVal oform = refF->base;
                  coeff_t valueNum;
                  ucoeff_t valueDen;
               }
            }

      Node *fnn = fnit;
      if (fnn->isInstructionNode() && fnn->getType() >0){
         if(fnn->is_load_instruction() && mo->inst_loads){
            bool flag_stride = true;
            total_lds+=1;
            if (fnn->is_scalar_stack_reference()){
               frame_lds+=1;
               std::cout<<std::hex<<fnn->getAddress()<<std::dec<<"\tload"<<"\tframe"<<std::endl;
               if(mo->inst_frame){
                  std::cout<<"OZGURDBG Instrumenting Frame Load\n";
                  if (func_exist){
                     daddr= fnn->getAddress();
                     loadPtr = funcs[0]->findPoint(daddr);
                     if (loadPtr){
                        lps = Dyninst::PatchAPI::convert(loadPtr, BPatch_callAfter);
                        addPTWSnippet(&patcher, lps , fnn);
                     }
                  }
               }
            } else if (fnn->is_strided_reference()){
               IncomingEdgesIterator ieit(fnn);
               while ((bool)ieit){
                  Node *nn = ieit->source();
                  if(nn->getType() <=0){
                     ++ieit;
                  } else { 
                  if (nn->is_scalar_stack_reference()){
                     frame_lds+=1;
                     std::cout<<std::hex<<fnn->getAddress()<<std::dec<<"\tload"<<"\tconstant"<<std::endl;
                     flag_stride = false;
                     if(mo->inst_frame){
                  std::cout<<"OZGURDBG Instrumenting Constant Load\n";
                        if (func_exist){
                           daddr= fnn->getAddress();
                           loadPtr = funcs[0]->findPoint(daddr);
                           if (loadPtr){
                              lps = Dyninst::PatchAPI::convert(loadPtr, BPatch_callAfter);
                              addPTWSnippet(&patcher, lps , fnn);
                           }
                        }
                     }
                  }
                  ++ieit;}
               }
               if (flag_stride){
                  strided_lds+=1;
                  std::cout<<std::hex<<fnn->getAddress()<<std::dec<<"\tload"<<"\tstrided"<<std::endl;
                  if (mo->inst_strided){
                  std::cout<<"OZGURDBG Instrumenting Strided Load\n";
                     if (func_exist){
                        daddr= fnn->getAddress();
                        loadPtr = funcs[0]->findPoint(daddr);
                        if (loadPtr){
                           lps = Dyninst::PatchAPI::convert(loadPtr, BPatch_callAfter);
                           addPTWSnippet(&patcher, lps , fnn);
                        }
                     }
                  }
               }
            } else {
               indirect_lds+=1;
               std::cout<<std::hex<<fnn->getAddress()<<std::dec<<"\tload"<<"\tindirect"<<std::endl;
               if (mo->inst_indirect){
                  std::cout<<"OZGURDBG Instrumenting Indirect Load\n";
                  if (func_exist){
                     daddr= fnn->getAddress();
                     loadPtr = funcs[0]->findPoint(daddr);
                     if (loadPtr){
                        lps = Dyninst::PatchAPI::convert(loadPtr, BPatch_callAfter);
                           addPTWSnippet(&patcher, lps , fnn);
                     }
                  }
               }
            }
         }
         if(fnn->is_store_instruction() && mo->inst_stores ){
            bool flag_stride_store = true;
            total_sts+=1;
            if (fnn->is_scalar_stack_reference()){
               frame_sts+=1;
               std::cout<<std::hex<<fnn->getAddress()<<std::dec<<"\tstore"<<"\tframe"<<std::endl;
               if(mo->inst_frame){
                  if (func_exist){
                     daddr= fnn->getAddress();
                     loadPtr = funcs[0]->findPoint(daddr);
                     if (loadPtr){
                        lps = Dyninst::PatchAPI::convert(loadPtr, BPatch_callAfter);
                        addPTWSnippet(&patcher, lps , fnn);
                     }
               }
               }
            } else if (fnn->is_strided_reference()){
               IncomingEdgesIterator ieit(fnn);
               while ((bool)ieit){
                  Node *nn = ieit->source();
                  if(nn->getType() <=0){
                     ++ieit;
                  } else { 
                  if (nn->is_scalar_stack_reference()){
                     frame_sts+=1;
                     std::cout<<std::hex<<fnn->getAddress()<<std::dec<<"\tstore"<<"\tconstant"<<std::endl;
                     flag_stride_store = false;
                     if(mo->inst_frame){
                        if (func_exist){
                           daddr= fnn->getAddress();
                           loadPtr = funcs[0]->findPoint(daddr);
                           if (loadPtr){
                              lps = Dyninst::PatchAPI::convert(loadPtr, BPatch_callAfter);
                              addPTWSnippet(&patcher, lps , fnn);
                           }
                        }
                     }
                  }
                  ++ieit;}
               }
               if (flag_stride_store){
                  strided_sts+=1;
                  std::cout<<std::hex<<fnn->getAddress()<<std::dec<<"\tstore"<<"\tstrided"<<std::endl;
                  if (mo->inst_strided){
                     if (func_exist){
                        daddr= fnn->getAddress();
                        loadPtr = funcs[0]->findPoint(daddr);
                        if (loadPtr){
                           lps = Dyninst::PatchAPI::convert(loadPtr, BPatch_callAfter);
                           addPTWSnippet(&patcher, lps , fnn);
                        }
                     }
                  }
               }
            } else {
               indirect_sts+=1;
               std::cout<<std::hex<<fnn->getAddress()<<std::dec<<"\tstore"<<"\tindirect"<<std::endl;
               if (mo->inst_indirect){
                  if (func_exist){
                     daddr= fnn->getAddress();
                     loadPtr = funcs[0]->findPoint(daddr);
                     if (loadPtr){
                        lps = Dyninst::PatchAPI::convert(loadPtr, BPatch_callAfter);
                           addPTWSnippet(&patcher, lps , fnn);
                     }
                  }
               }
            }
         }
      }
      ++fnit;
   }
   patcher.commit();

   std::cout<<"Testing mem Instructions Final:\nTotal loads:"<<total_lds<<"\tframe:"<<frame_lds<<"\tstrided:"<<strided_lds<<"\tindirect:"<<indirect_lds<<std::endl;
   std::cout<<"Total stores:"<<total_sts<<"\tframe:"<<frame_sts<<"\tstrided:"<<strided_sts<<"\tindirect:"<<indirect_sts<<std::endl;
   return 0;
}

//ozgurS
float DGBuilder::calculateMemoryData(int level, int index , std::map<int, double> levelExecCounts){
   std::map<int, double> loopsExecs = levelExecCounts;

   loopsExecs.find(1)->second = loopsExecs.find(1)->second - 1; 
   for (int i=2; i<=level; i++){
      loopsExecs.find(i)->second = loopsExecs.find(i)->second - loopsExecs.find(i-1)->second; 
   }

   NodesIterator fnit(*this);
   NodesIterator nit(*this);
   typedef std::vector <MIAMI::InstlvlMap *> MEEEM; 
   MEEEM * imemData = new MEEEM;
   std::map<unsigned long ,  int> seen;
   std::list<coeff_t> stackOffsetsOffLoops;
   std::list<int> dependOnList;
   double total_lds = 0;
   double frame_lds = 0;
   double strided_lds = 0;
   double indirect_lds = 0;
   double execRatio= 0.0;
   std::map<Node* ,std::list<depOffset>> dependencyMap;

   while ((bool)fnit) {
      Node *fnn = fnit;
      if(fnn->getLevel()==level && fnn->getLoopIndex()==index){
         if (fnn->isInstructionNode()){
            seen[(unsigned long)fnn->getAddress()] = 5;
            std::cout<<std::endl<<std::endl; 
            std::cout<<"inst address:"<<std::hex<<fnn->getAddress()<<std::dec<<std::endl;
            std::cout<<"lvl is:"<<fnn->getLevel()<<std::endl;
            std::cout<<"is_scalar_stack_reference :"<<fnn->is_scalar_stack_reference()<<std::endl;
            std::cout<<"is_strided_reference :"<<fnn->is_strided_reference()<<std::endl;
            std::cout<<"type:"<<fnn->getType()<<" IB_inner_loop :"<<IB_inner_loop <<std::endl;
            std::cout<<std::endl;
            std::cout<<"Node Dump :\n";
            fnn->longdump(this,std::cout);  
            int opidx = fnn->memoryOpIndex();
            if (opidx >=0){
               RefFormulas *refF = fnn->in_cfg()->refFormulas.hasFormulasFor(fnn->getAddress(), opidx);
               if(refF != NULL){
                  GFSliceVal oform = refF->base;
                  std::cout<<"oform is: "<<oform<<std::endl;
               }
            }

            if(fnn->is_load_instruction()){
               execRatio = (double)fnn->getExecCount() / (double)loopsExecs.find(fnn->getLevel())->second;
               bool flag_stride = true;
               total_lds+=1*execRatio;
               if (fnn->is_scalar_stack_reference()){
                  frame_lds+=1*execRatio;
               } else if (fnn->is_strided_reference()){
                  IncomingEdgesIterator ieit(fnn);
                  while ((bool)ieit){
                     Node *nn = ieit->source();
                     if (nn->is_scalar_stack_reference()){
                        frame_lds+=1*execRatio;
                        flag_stride = false;
                     }
                     ++ieit;
                  }
                  if (flag_stride){
                     strided_lds+=1*execRatio;
                     fnn->checkDependencies(&dependencyMap , level , index);
                  }
               } else {
                  indirect_lds+=1*execRatio;
                  fnn->checkDependencies(&dependencyMap , level , index);
               }
//               std::cout<<"Testing mem Instructions in loop:\nTotal loads:"<<total_lds<<"\tframe:"<<frame_lds<<"\tstrided:"<<strided_lds<<"\tindirect:"<<indirect_lds<<std::endl;
            }
         }
      }
      ++fnit;
   }

   std::cout<<std::endl; 
   std::cout<<"Testing mem Instructions Final:\nTotal loads:"<<total_lds<<"\tframe:"<<frame_lds<<"\tstrided:"<<strided_lds<<"\tindirect:"<<indirect_lds<<std::endl;
   std::cout<<std::endl<<std::endl; 

   std::cout<<"\t\t___XXXXXSTART HEREXXXXX___\n\n";

   while ((bool)nit) {
      Node *nn = nit;
   //Start debug
//   RInfoList _srcRegs = nn->srcRegs;
//   RInfoList::iterator rit = _srcRegs.begin();
//      std::cout<<"Print registers of node: "<<nn->getId()<<std::endl;
//   for( ; rit!=_srcRegs.end() ; ++rit ) {
//      std::cout<<rit->ToString()<<std::endl;
//   }
            nn->longdump(this,std::cout);  
            int opidx = nn->memoryOpIndex();
            if (opidx >=0){
               RefFormulas *refF = nn->in_cfg()->refFormulas.hasFormulasFor(nn->getAddress(), opidx);
               //std::cout<<"Size of Formula: "<<refF->size()<<std::endl;
               if(refF != NULL){
                  GFSliceVal oform = refF->base;
                  coeff_t valueNum;
                  ucoeff_t valueDen;
                  std::cout<<"oform is: "<<oform<<std::endl;
                  std::cout<<"Loop Level: "<<nn->getLevel()<<std::endl;
                  if(IsConstantFormula(oform, valueNum, valueDen))
                     std::cout<<"This is constant formula\n";
                  else
                     std::cout<<"This is not constant formula\n";
                  if (FormulaContainsRegister(oform))
                     std::cout<<"This formula contains register\n";
                  else
                     std::cout<<"This formola does not contains registers\n";
                   if (nn->is_memory_reference())
                     std::cout<<"Node is memory reference\n";
                  else
                     std::cout<<"Node is not memory reference\n";
                   if (nn->is_data_movement())
                     std::cout<<"Node has data movement\n";
                  else
                     std::cout<<"Node has not a data movement\n";
                  RInfoList srcreg = nn->getSrcReg();
                  RInfoList destreg = nn->getDestReg();
                  std::cout<<"Node has "<<srcreg.size()<<" src regs\n";
                  std::cout<<"Node has "<<destreg.size()<<" dest regs\n";
                  RInfoList::iterator nnrit = srcreg.begin();
                  for( ; nnrit!=srcreg.end() ; ++nnrit ) {
                     std::cout<<"Print Src reg info\n"<<nnrit->ToString()<<std::endl;
                  }
                  nnrit = destreg.begin();
                  for( ; nnrit!=destreg.end() ; ++nnrit ) {
                     std::cout<<"Print Dest reg info\n"<<nnrit->ToString()<<std::endl;
                  }
               }
            }

   std::cout<<std::endl<<std::endl; 
//End


      if (nn->isInstructionNode()){
         if(seen.find((unsigned long)nn->getAddress())->second==5){
            if (nn->getLvlMap()){
               imemData->push_back(nn->getLvlMap());
            }
            seen[(unsigned long)nn->getAddress()] = 0;        
         }
      }
      ++nit;
   }

   //   std::cerr<<"DEBUG CAN I CALL"<<__func__<<std::endl;
   struct Mem{
      int level;
      long long int hits;
      long long int miss;
      float windowSize;
   };

   std::vector<Mem> memVector;            

   // initializing collected data variables
   double all_loads = 0;   //lvl 0
   double l1_hits = 0 ;    //lvl 1
   double l2_hits = 0 ;    //lvl 2
   double fb_hits = 0 ;    //lvl 3
   double l3_hits = 0 ;    //lvl 4
   double llc_miss = 0 ;   //lvl 5
   double l2_pf_miss = 0 ; //lvl 6

   // initializing other vaiables
   double l1 = 0;
   double l2 = 0;
   double l3 = 0;
   double mem = 0;
   double pf = 0;
   double mem_hits = 0;
   double pf_wpl = 0;
   double mem_wpl = 0;
   double l1_strided_lds = 0;
   double fp = 0;
///_________________________-
//this is testting before assigning
   std::cout<<std::endl<<std::endl;
   std::cout<<"DEBUG- this is testting before assigning"<<std::endl<<std::endl; 
   std::cout<<"PALM FOOTPRINT OUTPUT FOR ALL MEMORY NODES"<<std::endl;
   std::cout<<"ALL_loads:"<<all_loads<<std::endl;
   std::cout<<"l1_hits:"<<l1_hits<<std::endl;
   std::cout<<"l2_hits:"<<l2_hits<<std::endl;
   std::cout<<"fb_hits:"<<fb_hits<<std::endl;
   std::cout<<"l3_hits:"<<l3_hits<<std::endl;
   std::cout<<"llc_miss:"<<llc_miss<<std::endl;
   std::cout<<"l2_pf_miss:"<<l2_pf_miss<<std::endl;
   std::cout<<"l1_strided_lds:"<<l1_strided_lds<<std::endl;
   std::cout<<"mem_wpl:"<<mem_wpl<<std::endl;
   std::cout<<"pf_wpl:"<<pf_wpl<<std::endl;
   std::cout<<"mem_hits:"<<mem_hits<<std::endl;
   std::cout<<"pf:"<<pf<<std::endl;
   std::cout<<"mem:"<<mem<<std::endl;
   std::cout<<"l3:"<<l3<<std::endl;
   std::cout<<"l2:"<<l2<<std::endl;
   std::cout<<"l1:"<<l1<<std::endl;
   std::cout<<"FP:"<<fp<<std::endl<<std::endl;
   std::cout<<"End OF DEBUG- this is testting before assigning"<<std::endl<<std::endl; 
//End of debug
//___________________-
int demi =0 ;
   for (std::vector<MIAMI::InstlvlMap *>::iterator it=imemData->begin() ; it!=imemData->end() ; it++){      
      all_loads += (*it)->find(0)->second.hitCount;
      l1_hits += (*it)->find(1)->second.hitCount;
      demi++;
      l2_hits += (*it)->find(2)->second.hitCount;
      std::cout<<"in iterarion "<<demi<<" l2_hits:"<<l2_hits<<std::endl;
      std::cout<<" l2_hits:"<<(*it)->find(2)->second.hitCount<<std::endl;
      fb_hits += (*it)->find(3)->second.hitCount;
      l3_hits += (*it)->find(4)->second.hitCount;
      llc_miss += (*it)->find(5)->second.hitCount;
      l2_pf_miss += (*it)->find(6)->second.hitCount;
   }
   //Check if it is smaller then 1 but bigger then 0 then set them 0
      if (all_loads<1 && all_loads >0) all_loads=0;
      if (l1_hits <1 && l1_hits >0 ) l1_hits = 0;
      if (l2_hits<1 && l2_hits>0) l2_hits = 0;
      if (fb_hits <1 && fb_hits>0) fb_hits = 0;
      if (l3_hits<1 && l3_hits>0) l3_hits=0 ;
      if (llc_miss <1  && llc_miss>0) llc_miss=0 ;
      if (l2_pf_miss <1 && l2_pf_miss >0) l2_pf_miss = 0;

///_________________________-
//this is testting after assigning
   std::cout<<std::endl<<std::endl;
   std::cout<<"DEBUG- this is testting after assigning"<<std::endl<<std::endl; 
   std::cout<<"PALM FOOTPRINT OUTPUT FOR ALL MEMORY NODES"<<std::endl;
   std::cout<<"ALL_loads:"<<all_loads<<std::endl;
   std::cout<<"l1_hits:"<<l1_hits<<std::endl;
   std::cout<<"l2_hits:"<<l2_hits<<std::endl;
   std::cout<<"fb_hits:"<<fb_hits<<std::endl;
   std::cout<<"l3_hits:"<<l3_hits<<std::endl;
   std::cout<<"llc_miss:"<<llc_miss<<std::endl;
   std::cout<<"l2_pf_miss:"<<l2_pf_miss<<std::endl;
   std::cout<<"l1_strided_lds:"<<l1_strided_lds<<std::endl;
   std::cout<<"mem_wpl:"<<mem_wpl<<std::endl;
   std::cout<<"pf_wpl:"<<pf_wpl<<std::endl;
   std::cout<<"mem_hits:"<<mem_hits<<std::endl;
   std::cout<<"pf:"<<pf<<std::endl;
   std::cout<<"mem:"<<mem<<std::endl;
   std::cout<<"l3:"<<l3<<std::endl;
   std::cout<<"l2:"<<l2<<std::endl;
   std::cout<<"l1:"<<l1<<std::endl;
   std::cout<<"FP:"<<fp<<std::endl<<std::endl;
   std::cout<<"End OF DEBUG- this is testting after assigning"<<std::endl<<std::endl; 
//End of debug
//___________________-


   if (total_lds){
      l1_strided_lds = ((l1_hits) / total_lds) * strided_lds; 
   } else {
      l1_strided_lds = -1;
   }

   if (total_lds && llc_miss){
      mem_wpl = (double)((((l1_hits+l2_hits+l3_hits))/total_lds) *
            indirect_lds) /(llc_miss); 
   } else {
      mem_wpl = -1; 
   }

   if (l2_pf_miss){
      pf_wpl = l1_strided_lds / (l2_pf_miss) ;
      if(pf_wpl > 32){
         std::cout<<"Pf_wpl is higher then 32 you cannot trust this FP\n";
      }
   } else {
      pf_wpl = -1;
   }

   mem_hits = llc_miss * mem_wpl;
   pf = l2_pf_miss * pf_wpl;
   mem = mem_hits + pf;
   l3 = l3_hits;
   l2 = l2_hits + fb_hits;
   l1 = all_loads -(pf+l2+l3+mem_hits);
   fp = mem ; 

   std::map<int , int> levels;

   for (int i=level; i>1; i--){
      levelExecCounts.find(i)->second = levelExecCounts.find(i)->second / levelExecCounts.find(i-1)->second; 
   }
   levelExecCounts.find(1)->second = levelExecCounts.find(1)->second - 1;
   double totLoads = strided_lds + indirect_lds;
   double adjusted_fp=0;
   std::list<std::list<depOffset>> baseOffsets;
   std::list<depOffset> indirectBaseOffsets;
   std::map<Node* ,std::list<depOffset>>::iterator it =  dependencyMap.begin();
   bool skip = false;
   for (;it!=dependencyMap.end();++it){
      std::list<depOffset>::iterator dit;
      std::list<depOffset>::iterator lit;
      std::list<depOffset>::iterator bit;
      std::list<depOffset>::iterator tit;

      std::cout<<"Looking Node:"<<it->first->getId()<<" at address:"<<std::hex<<it->first->getAddress()<<std::dec<<" size:"<<it->second.size()<<std::endl;
      for (tit=it->second.begin(); tit!=it->second.end(); ++tit){
         std::cout<<"This node dep level:"<<tit->level<<" offset:"<<tit->offset<<std::endl;
      }
      if(!it->first->is_strided_reference()){
         //This block helps to eliminate mulple access but I am not sure
         //if it is correct thing to do.
         if (it->second.size()==1){
            bit = indirectBaseOffsets.begin();
            dit = it->second.begin();
            if(bit != indirectBaseOffsets.end()){
               if(bit->level == dit->offset){
                  int opidx = it->first->memoryOpIndex();
                  if (opidx >=0){
                     RefFormulas *refF = it->first->in_cfg()->refFormulas.hasFormulasFor(it->first->getAddress(), opidx);
                     if(refF != NULL){
                        GFSliceVal oform = refF->base;
                        coeff_t offset1=0;
                        GFSliceVal::iterator sliceit;
                        for (sliceit=oform.begin() ; sliceit!=oform.end() ; ++sliceit){
                           if (sliceit->TermType() == TY_CONSTANT){
                              offset1 = sliceit->ValueNum();
                              std::cout<<"Slice valnum:"<<offset1<<std::endl;
                           }
                        }
                        if(bit->offset == offset1){
                           std::cout<<"Skip  linked ds node:"<<it->first->getId()<<std::endl;
                           std::cout<<"baseoffset:"<<bit->level<<" offset:"<<bit->offset<<std::endl;
                           std::cout<<" others baseoffset:"<<dit->offset<<" offset:"<<offset1<<std::endl;
                           std::cout<<"Formula: "<<oform<<std::endl;
                           continue;
                        }
                        depOffset depLinked;
                        depLinked.level = dit->offset;
                        depLinked.offset = offset1;
                        indirectBaseOffsets.push_back(depLinked);
                        std::cout<<"adding  linked ds node:"<<it->first->getId()<<std::endl;
                        std::cout<<"baseoffset:"<<depLinked.level<<" offset:"<<depLinked.offset<<std::endl;
                     }
                  }
               }
            } else {
               int opidx = it->first->memoryOpIndex();
               if (opidx >=0){
                  RefFormulas *refF = it->first->in_cfg()->refFormulas.hasFormulasFor(it->first->getAddress(), opidx);
                  if(refF != NULL){
                     GFSliceVal oform = refF->base;
                     coeff_t offset2= 0;
                     GFSliceVal::iterator sliceit;
                     for (sliceit=oform.begin() ; sliceit!=oform.end() ; ++sliceit){
                        if (sliceit->TermType() == TY_CONSTANT){
                           offset2 = sliceit->ValueNum();
                        }
                     }
                     depOffset depLinked;
                     depLinked.level = dit->offset;
                     depLinked.offset = offset2;
                     indirectBaseOffsets.push_back(depLinked);
                     std::cout<<"adding  linked ds node:"<<it->first->getId()<<std::endl;
                     std::cout<<"baseoffset:"<<depLinked.level<<" offset:"<<depLinked.offset<<std::endl;
                     std::cout<<"Formula: "<<oform<<std::endl;
                  }
               }


            }
            int size1 = it->first->getBitWidth()/8;
            execRatio = (double)it->first->getExecCount() / (double)loopsExecs.find(it->first->getLevel())->second;
            double fp_of_load = (((fp *size1) / totLoads)*execRatio);///2; TODO why this works??

            adjusted_fp+=fp_of_load;
            std::cout<<"Added linked ds node:"<<it->first->getId()<<std::endl;
            std::cout<<"this loads fp="<<fp_of_load<<" Size="<<size1<<" Ratio="<<execRatio<<std::endl;
            std::cout<<std::endl;
            levels.clear();
            continue;
         }
      }
      //Check End
      for (int i= 0 ; i <= level ; i++){
         levels.insert(std::map<int , int>::value_type(i , 0));
      }
      long int divider =1; 
      std::list<std::list<depOffset>>::iterator baseit = baseOffsets.begin();
      for ( ; baseit!=baseOffsets.end(); ++baseit){
         skip = false;
         for (bit = baseit->begin(), dit = it->second.begin() ; bit!=baseit->end(); ++bit, ++dit){ 
            std::cout<<"base level:"<<bit->level<<" offset:"<<bit->offset<<std::endl;
            std::cout<<"depMap level:"<<dit->level<<" offset:"<<dit->offset<<std::endl;
            if (bit->offset == dit->offset ){
               std::cout<<"offsets are same\n";
               if ( dit->level == 0 && bit->level == 0){
                  std::cout<<"skip is setted true\n";
                  skip = true;
               }
            }
         }
         if(dit != it->second.end()){
            std::cout<<"not same lenght\n";
            skip =false;
         }
         if(skip){
            std::cout<<"I am skipping\n";
            break;
         }

         std::cout<<"go next iter\n";
      }
      if(skip){
         std::cout<<"I found same array\n";
         for (lit = it->second.begin() ; lit!=it->second.end(); ++lit){
            std::cout<<"this is in level:"<<lit->level<<" offset:"<<lit->offset<<std::endl;
         }
         continue;
      } else {
         std::cout<<"This is not same array\n";
         for (lit = it->second.begin() ; lit!=it->second.end(); ++lit){
            std::cout<<"this is in level:"<<lit->level<<" offset:"<<lit->offset<<std::endl;
         }
         baseOffsets.push_back(it->second);
      }
      for (lit = it->second.begin() ; lit!=it->second.end(); ++lit){
         int lvl = lit->level;
         //sliceVal sval = lit->offset;
         levels.find(lvl)->second = 1;
         std::cout<<" Level: "<<lvl;
      }
      for(int i =0 ; i <=level; i++){
         std::cout<<" level:"<<i<<" count:"<<levelExecCounts.find(i)->second<<std::endl;
         if (levels.find(i)->second ==0){
            divider *=levelExecCounts.find(i)->second;
            std::cout<<"divider: "<<divider<<" level:"<<i<<" count:"<<levelExecCounts.find(i)->second<<std::endl;
         }
      }
      int size = it->first->getBitWidth()/8;
      execRatio = (double)it->first->getExecCount() / (double)loopsExecs.find(it->first->getLevel())->second;
      double fp_of_load = (((fp *size) / totLoads)/divider)*execRatio;

      adjusted_fp+=fp_of_load;
      std::cout<<"this loads fp="<<fp_of_load<<" Divider="<<divider<<" Size="<<size<<" Ratio="<<execRatio<<std::endl;
      levels.clear();
   }
   //fp = fp *4;//this is not correct is just there to check results with excel 
   std::cout<<"PALM FOOTPRINT OUTPUT FOR ALL MEMORY NODES"<<std::endl;
   std::cout<<"_ALL_loads:"<<all_loads<<std::endl;
   std::cout<<"l1_hits:"<<l1_hits<<std::endl;
   std::cout<<"l2_hits:"<<l2_hits<<std::endl;
   std::cout<<"fb_hits:"<<fb_hits<<std::endl;
   std::cout<<"l3_hits:"<<l3_hits<<std::endl;
   std::cout<<"llc_miss:"<<llc_miss<<std::endl;
   std::cout<<"l2_pf_miss:"<<l2_pf_miss<<std::endl;
   std::cout<<"l1_strided_lds:"<<l1_strided_lds<<std::endl;
   std::cout<<"mem_wpl:"<<mem_wpl<<std::endl;
   std::cout<<"pf_wpl:"<<pf_wpl<<std::endl;
   std::cout<<"mem_hits:"<<mem_hits<<std::endl;
   std::cout<<"pf:"<<pf<<std::endl;
   std::cout<<"mem:"<<mem<<std::endl;
   std::cout<<"l3:"<<l3<<std::endl;
   std::cout<<"l2:"<<l2<<std::endl;
   std::cout<<"l1:"<<l1<<std::endl;
   std::cout<<"Raw_FP:"<<fp<<std::endl<<std::endl;
   std::cout<<"Adjusted FP:"<<adjusted_fp<<std::endl<<std::endl;
   //return fp; 
   return adjusted_fp;
}


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
//               std::cout <<"here "<< primary_uop.type <<" " << MIAMI::InstrBin::IB_nop<<std::endl;
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
         //ozgurS TODO are we sure that only load inst has FP data ???
         if (node->is_load_instruction()){
 //           std::cout<<" OzgurDebuging lvlMap hit count = "<<img->getMemLoadData(node->getAddress())->find(0)->second.hitCount<<std::endl;
            node->setLvlMap(img->getMemLoadData(node->getAddress()));
//            std::cout<<__func__<<" ozgur test total hit at lvl 0 is "<<node->getLvlMap()->find(0)->second.hitCount<<std::endl; 
            assert(false && "Impossible!!");// why this is impossible ??
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

//   std::cout<<"hmm 1"<<std::endl;

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
//   std::cout <<"lastInsnNOP "<<lastInsnNOP<<std::endl;
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
      //OZGURS
      node->setLevel(b->getLevel());
      node->setLoopIndex(b->getLoopIndex());
      node->setExecCount(b->ExecCount());
      //OZGURE
      // save how many micro-ops were in the original instruction
      // we may use this info later to reason about the number of instructions
      // and retirement rate
      node->setNumUopsInInstruction(num_uops);
      node->setInOrderIndex(nextUopIndex++);
      if (isUnreordableNode(node, b))
         markBarrierNode (node);

      iiobj.data = node;
      //ozgurS
      //      if (node->is_load_instruction()){
      //TODO findMe and FIXME
      node->setLvlMap(img->getMemLoadData(node->getAddress()));
//      std::cout<<"DEBUG OZGUR instadd: "<< std::hex <<node->getAddress()<< std::dec<<" lvl0:"<<node->getLvlMap()->find(0)->second.hitCount<<" lvl-1:"<<node->getLvlMap()->find(-1)->second.hitCount<<std::endl; //segfault FIXME
      //std::cout<<"DEBUG OZGUR instadd: "<< std::hex <<node->getAddress()<< std::dec<<" lvl5:"<<node->getLvlMap()->find(5)->second.hitCount<<std::endl; //segfault FIXME
      //std::cerr<<__func__<<" ozgur testing img total hit at lvl 0 is "<<node->getLvlMap()->find(0)->second.hitCount<<std::endl; 
      //      }
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

