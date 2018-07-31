/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: routine.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Data bookkeeping and analysis at the routine level for the
 * MIAMI scheduler. Extends the PrivateRoutine implementation with 
 * functionality specific to the MIAMI post-processing tool.
 */

#ifndef MIAMI_SCHEDULER_ROUTINE_H
#define MIAMI_SCHEDULER_ROUTINE_H

#include <list>
#include <string>

#include "CFG.h"
#include "private_routine.h"
#include "load_module.h"
#include "prog_scope.h"
#include "slice_references.h"
#include "hashmaps.h"
#include "static_memory_analysis.h"
#include "miami_options.h"
#include "miami_containers.h"
#include "instruction_class.h"
#include <BPatch.h>
//OZGURS
//#include "routine.h"
#include "miami_types.h"
#include "DGBuilder.h"
using namespace MIAMI;
using namespace MIAMI_DG;
//OZGURE
namespace MIAMI
{
typedef std::list<std::string> StringList;
typedef std::map<AddrIntPair, InstructionClass, AddrIntPair::OrderPairs> RefIClassMap;
typedef std::map<std::pair<addrtype,addrtype>,BPatch_basicBlock*> DynBlkMap;
typedef std::map<int,PrivateCFG::Node*> BlkNoMap;
typedef std::map<std::pair<addrtype,addrtype>,int> BlkAddrMap;

class ScopeImplementation;

class RefStrideId
{
public:
   RefStrideId(addrtype _pc, int _idx, int _lev) :
       pc(_pc), opidx(_idx), level(_lev)
   { 
      visited = 0;
   }
   
   addrtype pc;
   int opidx;
   int level;
   int visited;
};

typedef std::map<AddrIntPair, RefStrideId, AddrIntPair::OrderPairs> RefInfoMap;



class Routine : public PrivateRoutine
{
public:
   Routine(LoadModule *_img, addrtype _start, usize_t _size, 
         const std::string& _name, addrtype _offset, addrtype _reloc);
   virtual ~Routine();
   
   int loadCFGFromFile(FILE *fd);
   CFG* ControlFlowGraph() const { return (static_cast<CFG*>(g)); }
   
   // main analysis function. Compute BB/edge counts, reconstruct executed paths,
   // compute schedule for paths.
   int main_analysis(ImageScope *img, const MiamiOptions *mo);
   LoadModule* InLoadModule() const;
   
   // check if routine should be analyzed
   bool is_valid_for_analysis();
   static int parse_include_exclude_files(const std::string& _include, const std::string& _exclude);

   RFormulasMap& getRefFormulas() {
      return (*rFormulas); 
   }
   
   uint64_t SaveStaticData(FILE *fd);
   void FetchStaticData(FILE *fd, uint64_t offset);


   BPatch_basicBlock* getBlockFromAddr(addrtype startAddr,addrtype endAddr);
   int getBlockNoFromAddr(addrtype startAddr,addrtype endAddr);
   BPatch_function* getDynFunction();
   
private:
   const char * ComputeObjectNameForRef(addrtype pc, int32_t memop);

   int build_paths_for_interval(ScopeImplementation *pscope, RIFGNodeId node, 
           TarjanIntervals *tarj, MiamiRIFG* mCfg, int marker, int level, 
           int no_fpga_acc, CFG::AddrEdgeMMap *entryEdges, CFG::AddrEdgeMMap *callEntryEdges);

   void myConstructPaths (ScopeImplementation *pscope, int no_fpga_acc, const std::string& path);
   void constructPaths(ScopeImplementation *pscope, CFG::Node *b, int marker,
            int no_fpga_acc, CFG::AddrEdgeMMap *entryEdges, CFG::AddrEdgeMMap *callEntryEdges,
            TarjanIntervals *tarj, RIFGNodeId node, MiamiRIFG* mCfg);//TODO Remove this line

   void constructOuterLoopDG(ScopeImplementation *pscope, CFG::Node *b, int marker,
            int no_fpga_acc, CFG::AddrEdgeMMap *entryEdges, CFG::AddrEdgeMMap *callEntryEdges, 
            SchedDG *sch);
   
   CFG::NodeList::iterator addBlock(ScopeImplementation *pscope, CFG::Node *pentry, 
           CFG::Node *thisb, CFG::Edge *lastE, CFG::NodeList& ba, CFG::EdgeList &ea, 
           RSIListList &rl, int marker, BPMap *bpm, int64_t& iCount);

   void compute_lineinfo_for_block(ScopeImplementation *pscope, CFG::Node* b);
   void decode_instructions_for_block (ScopeImplementation *pscope, CFG::Node *b, int64_t count,
           AddrIntSet& memRefs, RefIClassMap& refsClass);

   void computeBaseFormulas(ReferenceSlice *rslice, CFG *g, RFormulasMap& refFormulas);
   
   void computeStrideFormulasForRoutine(RIFGNodeId node, TarjanIntervals *tarj, 
            MiamiRIFG* mCfg, int marker, int level, ReferenceSlice *rslice);
   void computeStrideFormulasForScope(StrideSlice &sslice, ReferenceSlice *rslice, 
               RIFGNodeId node, TarjanIntervals *tarj, MiamiRIFG* mCfg, int level,
               RFormulasMap& refFormulas, RefInfoMap& memRefs);

   bool clarifyIndirectAccessForRef(RefStrideId& rsi, GFSliceVal& _formula, 
               RFormulasMap& refFormulas, RefInfoMap& memRefs);

   static StringList includePatterns, excludePatterns;
   addrtype reloc_offset;
   
   const MiamiOptions *mo;

   // store symbolic formulas associated with each memory instruction
   RFormulasMap *rFormulas;
   
   // use two bitflags to compute and cache if the routine is valid
   // for analysis
   char valid_for_analysis : 1;
   char validity_computed : 1;


   void createDyninstFunction();

   void createBlkNoToMiamiBlkMap(CFG* cfg);

   int dyninst_discover_CFG(addrtype pc);

   BPatch_function* dyn_func;
   DynBlkMap* dyn_addrToBlock; //get malloc error unless this is a pointer?
   BlkNoMap* dyn_blkNoToMiamiBlk;
   BlkAddrMap* dyn_addrToBlkNo;

};
//LoopScope
////this vector will keep the dependencies of each indiriect
////or strided load but not the frame loads
//typedef std::vector <FPdependencies* > FPDepVector;
//
//struct FPdependencies{
//   coeff_t offset;
//   int level;
//};
//
//class FP{
//public:
//   FP(double _fp=-1){
//      fp = _fp;
//   }
//   
//   virtual ~FP(){}
//
//   double getFP(){return fp;}
//   void setFP(double _fp){fp = _fp;}
//   //This fuction will add a dependency to FP
//   //it will also update the fp based on dependencies if it requires
//   void addFPDependency(FPdependencies *_fpDep);
//   FPDepMap * getFPDependencies(){return &fpDep;}
//private:
//   double fp;
//   FPDepVector fpDep;
//};
//
//class LoopFP {
//public:
//   LoopFP(FP *_fp, addrtype _loopId, addrtype _startAddress, 
//            int _level, int _index, long int _loopCount, LoopFP *_innerLoop = NULL){
//      fp = _fp;
//      loopId = _loopId;
//      startAddress = _startAddress;
//      level = _level;
//      index = _index;
//      loopCount = _loopCount;
//      innerLoop = _innerLoop;
//   }
//
//   virtual ~LoopFP() {}
//
//   FP * getLoopFP(){return fp;}
//   void setLoopFP(FP *_fp){fp = _fp;}
//
//   addrtype getLoopId(){return loopId;}
//   addrtype getStartAddress(){return startAddress;}
//   int getLevel () {return level;}
//   int getIndex () {return index;}
//   long int getLoopCount(){return loopCount;}
//
//   LoopFP * getInnerLoop(){return innerLoop;}
//   void setInnerLoop(LoopFP * _innerLoop){innerLoop = _innerLoop;}
//
//private:
//   FP *fp;
//   addrtype loopId;
//   addrtype startAddress;
//   int level;
//   int index;
//   long int loopCount;
//   LoopFP *innerLoop;
//};

};

#endif
