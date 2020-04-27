/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: DGBuilder.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the logic for building the dependence graph of
 * a path. It creates register, memory and control dependencies within the
 * same iteration. If the path takes the back edge of a loop, it also 
 * computes loop carried dependencies between micro-operations.
 */

/** Extends the scheduling class. Builds the dependence graph of 
 *  an executed control-flow path.
 */

#ifndef MIAMI_DG_BUILDER_H
#define MIAMI_DG_BUILDER_H

#include <list>
#include <vector>
#include <map>
#include <set>

#include "CFG.h"
#include "dependency_type.h"
#include "SchedDG.h"
#include "uipair.h"
#include "slice_references.h"
#include "fast_hashmap.h"
#include "reuse_group.h"
#include "reg_sched_info.h"
#include "instr_info.H"
#include "register_class.h"
#include "hashmaps.h"
#include "miami_containers.h"
#include "routine.h"

#include "BPatch.h"
#include "BPatch_function.h"
#include "BPatch_object.h"
#include "BPatch_image.h"
#include "BPatch_point.h"

#include "PatchMgr.h"
#include "PatchModifier.h"
#include "Point.h"
#include "Snippet.h"

namespace MIAMI_DG
{
class PTWriteSnippetEAX : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetEAX() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[4] = {0xf3,0x0f,0xae,0xe0};
        buf.copy(ptwrite_example, 4);
    }
};
class PTWriteSnippetEBX : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetEBX() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[4] = {0xf3, 0x0f, 0xae, 0xe3}; 
        buf.copy(ptwrite_example, 4);
    }
};
class PTWriteSnippetECX : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetECX() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[4] = {0xf3,0x0f,0xae,0xe1};
        buf.copy(ptwrite_example, 4);
    }
};
class PTWriteSnippetEDX : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetEDX() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[4] = {0xf3,0x0f,0xae,0xe2};
        buf.copy(ptwrite_example, 4);
    }
};
class PTWriteSnippetRAX : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetRAX() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x48, 0x0f, 0xae, 0xe0};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetRBX : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetRBX() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x48, 0x0f, 0xae, 0xe3};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetRCX : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetRCX() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x48, 0x0f, 0xae, 0xe1};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetRDX : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetRDX() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x48, 0x0f, 0xae, 0xe2};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetRBP : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetRBP() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x48, 0x0f, 0xae, 0xe5};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetRSP : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetRSP() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x48, 0x0f, 0xae, 0xe4};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetRSI : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetRSI() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x48, 0x0f, 0xae, 0xe6} ;
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetRDI : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetRDI() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x48, 0x0f, 0xae, 0xe7};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetR8 : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetR8() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x49, 0x0f, 0xae, 0xe0};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetR9 : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetR9() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x49, 0x0f, 0xae, 0xe1};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetR10 : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetR10() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x49, 0x0f, 0xae, 0xe2}; 
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetR11 : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetR11() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x49, 0x0f, 0xae, 0xe3};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetR12 : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetR12() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x49, 0x0f, 0xae, 0xe4};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetR13 : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetR13() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x49, 0x0f, 0xae, 0xe5};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetR14 : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetR14() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x49, 0x0f, 0xae, 0xe6};
        buf.copy(ptwrite_example, 5);
    }
};
class PTWriteSnippetR15 : public Dyninst::PatchAPI::Snippet {
public:
    PTWriteSnippetR15() {}
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {
        static unsigned char ptwrite_example[5] = {0xf3, 0x49, 0x0f, 0xae, 0xe7};
        buf.copy(ptwrite_example, 5);
    }
};
typedef std::list<SchedDG::Node*> NodeList;
typedef std::vector<SchedDG::Node*> UNPArray;
typedef std::map<unsigned int, NodeList> UiNLMap;

class RegData
{
public:
   unsigned int newName;
   SchedDG::Node* definedAt;
   int overlapped;
   NodeList usedBy;
//   int loopVar;    // flag indicating if a register is constant with respect
                     // to a loop (scope)
   
   RegData(unsigned int _name, SchedDG::Node* _defined, int _overlapped) :
       newName(_name), definedAt(_defined), overlapped(_overlapped)
   {
   }
   
   RegData(const RegData& rd)
   {
      newName = rd.newName;
      definedAt = rd.definedAt;
      overlapped = rd.overlapped;
      usedBy = rd.usedBy;
   }

   RegData& operator= (const RegData& rd)
   {
      newName = rd.newName;
      definedAt = rd.definedAt;
      overlapped = rd.overlapped;
      usedBy = rd.usedBy;
      return (*this);
   }
};

typedef std::multimap<register_info, RegData, RegisterInfoCompare> RI2RDMultiMap;
typedef std::map<unsigned int, RegData> RegMap;

// map each instruction from the native binary to a list of decoded micro-ops
// Store a SchedDG::Node* in each micro-op's data field.
extern DecodedInstruction* defDInstP;
typedef MIAMIU::HashMap<addrtype, MIAMI::DecodedInstruction*, &defDInstP> HashMapDInstP;


/*
 * Wrapper or front-end class around the instruction scheduler to deal with
 * any architecture/implementation dependent aspects. Originally it was motivated
 * by the dependence on the EEL control flow graph implementation. However, we
 * now have a MIAMI CFG implementation and the instruction decoding is moved
 * to a machine dependent file with a machine independent interface. In theory, 
 * this class should become portable as well. It depends however on the CFG
 * implementation, and if we have to change the CFG implementation, then this class
 * needs to change as well.
 */
class DGBuilder : public SchedDG
{
public:
   DGBuilder(const char* func_name, MIAMI::PathID _pathId, int _opt_mem_dep,
           RFormulasMap& _refAddr, 
           LoadModule *_img,
           int numBlocks, 
           MIAMI::CFG::Node** ba, float* fa, RSIList* innerRegs, 
           uint64_t _pathFreq = 1, float _avgNumIters = 1.0);
   

   DGBuilder(Routine* _routine, MIAMI::PathID _pathId, int _opt_mem_dep,
           RFormulasMap& _refAddr, 
           LoadModule *_img,
           int numBlocks, 
           MIAMI::CFG::Node** ba, float* fa, RSIList* innerRegs, 
           uint64_t _pathFreq = 1, float _avgNumIters = 1.0 ); 
           
   virtual ~DGBuilder();

   void computeMemoryInformationForPath (
            RGList &globalRGs, CliqueList &allCliques, 
            HashMapUI &refsDist2Use);
   
   void computeRegisterScheduleInfo (RSIList &reglist);
   
   const AddrSet& getMemoryReferences() const { return (memRefs); }

//ozgurS
   float calculateMemoryData(int level, int index , std::map<int,double> levelExecCounts);
   float printLoadClassifications(const MIAMI::MiamiOptions *mo);
   void addPTWSnippet(Dyninst::PatchAPI::Patcher *patcher, Dyninst::PatchAPI::Point* new_point, Node *nn);
//ozgurE

private:
   void pruneTrivialMemoryDependencies ();


   void build_graph(int numBlocks, MIAMI::CFG::Node** ba, float* fa, RSIList* innerRegs);
   int build_node_for_instruction(addrtype pc, MIAMI::CFG::Node* b, float freq);
   void handle_register_dependencies(MIAMI::DecodedInstruction *dInst, 
            MIAMI::instruction_info& iiobj, SchedDG::Node* node, 
            MIAMI::CFG::Node* b, int firstIteration);
   void handle_intrinsic_register_dependencies (SchedDG::Node* node, 
               int firstIteration);
   void handle_inner_loop_register_dependencies (SchedDG::Node *node, 
            RSIList &loopRegs, int firstIteration);
   void handle_control_dependencies (SchedDG::Node *node, int type, 
            MIAMI::CFG::Node* b, float freq);
   void handle_memory_dependencies(SchedDG::Node* node, addrtype pc, int opidx, int type,
            int stackAccess);
   void memory_dependencies_for_node(SchedDG::Node* node, addrtype pc, int opidx,
            int is_store, UNPArray& stores, UNPArray& loads);
   bool computeMemoryDependenciesForOneIter(SchedDG::Node *node, 
            GFSliceVal& _formula, SchedDG::Node *nodeB, int depDir);
   bool computeMemoryDependenciesForManyIter(SchedDG::Node *node, 
            GFSliceVal& _formula, GFSliceVal& _iterFormula, int refIsScalar, 
            int refIsIndirect, SchedDG::Node *nodeB, int negDir, int posDir,
            int refTypes);
            
   bool isUnreordableNode(SchedDG::Node *node, MIAMI::CFG::Node *b) const;
   bool isInterproceduralJump(SchedDG::Node *node, MIAMI::CFG::Node *b) const;

   unsigned int readsGpRegister(register_info&, SchedDG::Node*, 
                int firstIteration, int cycle=0);
   unsigned int readsStackRegister(register_info&, SchedDG::Node*, 
                int firstIteration, int cycle=0);
   unsigned int readsTemporaryRegister(register_info&, SchedDG::Node*);

   unsigned int writesGpRegister(register_info&, SchedDG::Node*, 
                int firstIteration, int cycle=0);
   unsigned int writesStackRegister(register_info&, SchedDG::Node*, 
                int firstIteration, int cycle=0);
   unsigned int writesTemporaryRegister(register_info&, SchedDG::Node*);

   void computeRefsReuseGroups (unsigned int setIndex, NodeList &nlist,
            RGList &globalRGs);
   void findReuseGroupForNode (Node *node, unsigned int setIndex, 
            RGList &listRGs, RGList &undefRGs);

   // number of register stacks on this architecture
   int numRegStacks;
   int *stack_tops, *prev_stack_tops, *crt_stack_tops;
   
   RI2RDMultiMap gpMapper;  // use for general purpose registers (includes SIMD, MMX, etc.)
   RegMap inMapper;  // use for internal registers
   RI2RDMultiMap prevGpMapper;
   // do not need a previous map for internal registers (I think)
   RI2RDMultiMap *gpRegs;
   
   unsigned int nextGpReg;
   unsigned int nextInReg;
   RI2RDMultiMap externalGpReg;
   
   bool prev_inst_may_have_delay_slot;
   bool instruction_has_stack_operation;
   int stack_index;
   int top_increment;
   
   int nextUopIndex;  // assign a unique index to each uop, in the order they are decoded
   
   addrtype minPathAddress;
   addrtype maxPathAddress;
   
   HashMapDInstP builtNodes;
   
   UNPArray fpLoads;
   UNPArray fpStores;
   UNPArray gpLoads;
   UNPArray gpStores;
   // treat memory operations to stack differently. That is because on Sparc
   // conversion from float to int or reverse is performed by writing the
   // float register on stack and reading it in a int register.
   UNPArray stackLoads;
   UNPArray stackStores;
   
   MIAMI::CFG::Node *lastBranchBB;
   float lastBranchProb;
   ObjId lastBranchId;

   SchedDG::Node* prevBarrier;
   SchedDG::Node* lastBarrier;

   float prevBranchProb;
   ObjId prevBranchId;
   SchedDG::Node* prevBranch;
   
   // keep a set of nodes seen since the last branch. Remove the nodes which 
   // are used as a dependency source. When a new branch is encountered, 
   // create control dependencies from all the nodes in the set to the branch
   // node. Clear the set after that.
   NodeSet recentNodes;
   NodeSet recentBranches;
   
   int optimistic_memory_dep;
   int pessimistic_memory_dep;
   
   AddrSet memRefs;

   Routine* routine;
//ozgurS memory data
   MIAMI::MemListPerInst * memData;
   MemDataPerLvlList *mdplList;
//ozgurE
};

}  /* namespace MIAMI_DG */

#endif
