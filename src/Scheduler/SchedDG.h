/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: SchedDG.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the logic for scheduling the nodes of a dependence
 * graph on the resources of an abstract machine model in a modulo fashion. 
 * The dependence graph edges correspond to dependencies between instructions
 * and must be satisfied by the resulting instruction schedule.
 * The scheduler identifies and reports the factors the constrain the 
 * schedule length.
 */

#ifndef _SCHEDCFG_H
#define _SCHEDCFG_H

//--------------------------------------------------------------------------------------------------------------------
// standard headers
#ifdef NO_STD_CHEADERS
# include <string.h>
#else
# include <cstring>
#endif

// STL headers
#include <list>
#include <set>
#include <map>
#include <queue>
#include <functional>

// OpenAnalysis headers
#include "OAUtils/DGraph.h"

// MIAMI headers
#include "uipair.h"
#include "dependency_type.h"
#include "Machine.h"
#include "TimeAccount.h"
#include "schedule_time.h"
#include "printable_class.h"
#include "Clique.h"
#include "fast_hashmap.h"

#include "unionfind.h"
#include "CycleSetDAG.h"
#include "math_routines.h"
#include "path_id.h"
#include "instr_info.H"
#include "register_class.h"
// Include the LoadModule to check on references' names and what not
#include "load_module.h"

// implement a dependence graph on top of the Open Analysis 
// directed graph

// I use 1001 classes and types from namespace MIAMI;
// include the entire namespace.
using namespace MIAMI;

class BaseDominator;

namespace MIAMI_DG
{
// variable that determines how the priority of cycle edges is computed.
#define USE_SCC_PATHS  1

#define DRAW_GRAPH_ON_SUCCESS  1
#define DRAW_GRAPH_ON_FAILURE  2

#define NO_LATENCY   (-1)
#define NO_CYCLE_ID  (-1)
#define EPSILON_VALUE           0.000001
#define ONE_ITERATION_EPSILON   1.000001

// define bit flags for edges
#define PRUNED_EDGE                 1
#define LATENCY_COMPUTED_EDGE       2
#define HAS_BYPASS_RULE_EDGE        4
#define SCHEDULE_DONE_EDGE          8
#define EDGE_OF_FAILURE             0x10
#define EDGE_IS_INDIRECT_DEPENDENCE 0x20
#define INVISIBLE_EDGE              0x1000

// define bit flags for nodes
#define NODE_DONOT_DRAW             1
#define NODE_HAS_MIXED_EDGES        2
#define NODE_FROM_REPLACE           4
#define NODE_IS_REPLICATED          8
#define NODE_OF_FAILURE             0x10
#define NODE_IS_BARRIER             0x20
#define PRUNED_NODE                 0x40
#define NODE_IS_DG_ENTRY            0x80
#define NODE_IS_DG_EXIT             0x100
#define NODE_IS_DG_ENTRY_EXIT       0x180
#define NODE_IS_ADDRESS_CALCULATION 0x200
#define NODE_IS_LOOP_CONDITION      0x400

// define bit flags for cycles
#define LATENCY_COMPUTED_CYCLE  2

#define REPLACE_NODE_MASK       0x80000000
#define MAKE_REPLACE_NODE_ID(a) (REPLACE_NODE_MASK + (a))
#define IS_REPLACE_NODE(a)      ((a) & REPLACE_NODE_MASK)
#define REPLACEMENT_INDEX(a)    ((a) & (~(REPLACE_NODE_MASK)))

// define what we understand by high probability for a branch
// if a control edge is taken more often than the value defined here
// then I assume it is always predicted and instructions after it are 
// speculatively executed without any penalty
// This is a very subjective value. How should it be??? FIXME
#define BR_HIGH_PROBABILITY   0.75

#define DG_ENTRY_TYPE       -1
#define DG_EXIT_TYPE        -2
//#define DG_LOOP_ENTRY_TYPE  -3

typedef std::pair<int, uint32_t> schedule_result_t;
//ozgurS
   struct retValues
   {
     int ret;
     int memret;
     int cpuret;
   };

    struct depOffset{
      int level;
//      sliceVal offset;
      coeff_t offset;
      
      bool operator==(const depOffset& a) const
      {
          return (level == a.level && offset == a.offset);
      }
    
    };

//ozgurE


class PatternFoundException
{
public:
   PatternFoundException(MIAMIU::UIList &_iters) : iters(_iters)  
   { }
   ~PatternFoundException() { }
   MIAMIU::UIList& NumIterations()  { return (iters); }

private:
   MIAMIU::UIList& iters;
};

class RecomputePathsToExit
{
public:
   RecomputePathsToExit ()
   { }
   ~RecomputePathsToExit () { }

private:
};

class UnsuccessfulSchedulingException 
{
public:
   UnsuccessfulSchedulingException (int _schedId, int _exceptionMask, 
            int _unit, DGraph::Node *_node, DGraph::Edge *_edge, 
            int _extra_length, int _num_tries=-1, int _segmentId=NO_CYCLE_ID):
            schedId(_schedId), exceptionMask(_exceptionMask), unit(_unit), 
            node(_node), edge(_edge), extra_length(_extra_length), 
            num_tries(_num_tries), segmentId(_segmentId)
   { }
   ~UnsuccessfulSchedulingException () { }
   int ScheduleId () const     { return (schedId); }
   int ExceptionMask () const  { return (exceptionMask); }
   int Unit () const           { return (unit); }
   int SegmentId () const      { return (segmentId); }
   int ExtraLength () const    { return (extra_length); }
   int NumTries () const       { return (num_tries); }
   DGraph::Node* Node () const { return (node); }
   DGraph::Edge* Edge () const { return (edge); }
   
private:
   int schedId, exceptionMask, unit;
   // the node and/or edge that caused the failure
   // This information was added to help discover deadlock cases.
   // If the same failure happens at the same location after consecutive
   // increase of the target scheduling length, then we have a deadlock.
   DGraph::Node *node;
   DGraph::Edge *edge;
   int extra_length;
   int num_tries;
   int segmentId;  // the segment that must be extended
};

class SuccessfulSchedulingException
{
public:
   SuccessfulSchedulingException () { }
   ~SuccessfulSchedulingException () { }
};


/* Define some bit flags for SchedDG */
// - first flag must be set by the child class that constructs the initial
//   control flow graph
#define DG_CONSTRUCTED         0x1
// - graph have been modified
#define DG_GRAPH_IS_MODIFIED   0x2
// - next flag indicates if graph was freshly pruned
#define DG_GRAPH_PRUNED        0x200
// - indicates if cycles have been computed
#define DG_CYCLES_COMPUTED     0x400
// - indicates if graph was updated with machine information
#define DG_MACHINE_DEFINED     0x800
// - indicates the scheduling has been computed for this graph
#define DG_SCHEDULE_COMPUTED   0x1000

/* I have to check the graph's state in the various routines that work on 
 * the graph, and to update its state accordingly after each transformation
 */
// Following macros can work with one or multiple flags at a time.
// Multiple flags must be passed with + or | between them.
#define setCfgFlags(x)     cfgFlags |= (x)
#define removeCfgFlags(x)  cfgFlags &= (~(x))
#define HasNoFlags(x)      (! (cfgFlags & (x)) )
#define HasAllFlags(x)     (! ((~cfgFlags) & (x)) )
#define HasSomeFlags(x)    (cfgFlags & (x))


/* Define a class to store info about registers. For now I want a flag to 
 * tell me if the register is defined in this scope, or it is an input to
 * this scope. Problem: what if the register is loaded from stack (unspilled)
 * how do I tell that it is actually constant with respect to this scope?
 * Without testing for such cases, I could actually just test the number of 
 * input edges into that node. If there is only one, then I know that one
 * of the input registers is defined outside the loop.
 */

 
//--------------------------------------------------------------------------------------------------------------------
/** SchedDG is a DGraph (directed graph) with enhanced nodes and edges.  
    Each node in the SchedDG represents a machine instruction, and an edge
    represents a dependency between instructions. All nodes in the graph 
    represent a path in a loop that must be scheduled.
*/
class SchedDG : public DGraph {
public:
  class Node;
  friend class Node;
  class Edge;
  friend class Edge;
  class RemovedNodesIterator;
  friend class RemovedNodesIterator;
  
  typedef bool (*DependencyFilterType)(Edge*);

  SchedDG(const char* exec_name, PathID &_pathId, uint64_t _pathFreq, 
	  float _avgNumIters, RFormulasMap& _refAddr, LoadModule *_img);
  virtual ~SchedDG();
  
  unsigned int new_marker() { return (topMarker++); }
  
  class SortNodesByDependencies;
  class SortEdgesByDependencies;
  
  //------------------------------------------------------------------------------------------------------------------
  class Node : public DGraph::Node, public PrintableClass {
  public:
    Node (SchedDG* _g, addrtype _address, int _idx, int _type) : DGraph::Node()
    { 
       _in_cfg = _g; address = _address; index = _idx; type = _type;
       // initialize other fields with invalid values;
       bit_width = 0; vec_len = 0; exec_unit = ExecUnit_LAST;
       exec_unit_type = ExecUnitType_LAST;
       vec_width = 0;
       Ctor();
       lvlMap = NULL;
    }
    
    Node (SchedDG* _g, addrtype _address, int _idx, const InstructionClass& _ic) : DGraph::Node()
    { 
       _in_cfg = _g; address = _address; index = _idx; type = _ic.type;
       exec_unit = _ic.eu_style;
       exec_unit_type = _ic.eu_type;
       bit_width = _ic.width;
       vec_width = _ic.vec_width;
       // initialize other fields with default values;
       if (bit_width!=max_operand_bit_width && bit_width!=0 && vec_width!=0)
          vec_len = vec_width / bit_width;
       else
          vec_len = 0;
       Ctor();
       lvlMap = NULL;
    }
    
    Node (SchedDG* _g, addrtype _address, int _idx, MIAMI::instruction_info& iiobj) :
          DGraph::Node()
    { 
       _in_cfg = _g; address = _address; index = _idx; 
       type = iiobj.type;
       bit_width = iiobj.width; vec_len = iiobj.vec_len; 
       exec_unit = iiobj.exec_unit;
       exec_unit_type = iiobj.exec_unit_type;
       if (bit_width != max_operand_bit_width)
          vec_width = bit_width * vec_len;
       else
          vec_width = 0;
       Ctor();
       lvlMap = NULL;
    }
    
    virtual ~Node () {  }
    
    inline MIAMI::ExecUnitType getExecUnitType() const  { return (exec_unit_type); }
    
    // getId: An id unique within instances of SchedDG::Node
    virtual ObjId getId() const { return id; }

    void dump (std::ostream& os) { }
    void longdump (SchedDG *, std::ostream& os);
    void longdump (SchedDG * _cfg) { longdump(_cfg, std::cout); } 

    virtual void draw (std::ostream& os, int info = 0);
    void draw (int info = 0) 
    { 
       draw(std::cout, info); 
    }
    void PrintObject (ostream& os) const;
    void draw_scheduling (std::ostream& os);
//ozgurS
   void setLvlMap (MIAMI::InstlvlMap * myLvlMap){ this->lvlMap = myLvlMap;}
   MIAMI::InstlvlMap * getLvlMap (){return this->lvlMap;}    
//ozgurE 
    void visit(unsigned int m)         { _visited = m; }
    bool visited(unsigned int m) const { return (m==_visited); }
    bool visited(unsigned int mL, unsigned int mU) const 
    {
       return (mL<=_visited && _visited<mU); 
    }
    
    addrtype getAddress() const { return (address); }
    int getIndex() const  { return (index); }
    
    int getType() const   { return (type); }
    int getLevel() const  { return (_level); }
    void setLevel(int level)  { _level=level; }
    int getLoopIndex() const  { return (_loopIdx); }
    void setLoopIndex(int loopIdx)  { _loopIdx=loopIdx; }

    width_t getBitWidth() const { return bit_width;}
    width_t getVecWidth() const { return vec_width;}
    double getExecCount() const  { return (_execCount); }
    void setExecCount(double execCount)  { _execCount=execCount; }

    
    void setNotDraw()     { _flags = (_flags | NODE_DONOT_DRAW); }
    unsigned int Degree() const   { return (fanin+fanout); }

    void setHasMixedEdges()    { _flags = (_flags | NODE_HAS_MIXED_EDGES); }
    void resetHasMixedEdges()  { _flags = _flags & (~NODE_HAS_MIXED_EDGES); }
    bool HasMixedEdges() const { return (_flags & NODE_HAS_MIXED_EDGES); }

    void setCreatedByReplace()    { _flags = (_flags | NODE_FROM_REPLACE); }
    bool CreatedByReplace() const { return (_flags & NODE_FROM_REPLACE); }
    void setNodeIsReplicated()    { _flags = (_flags | NODE_IS_REPLICATED); }
    bool NodeIsReplicated() const { return (_flags & NODE_IS_REPLICATED); }

    bool isCfgNode () const { return (_flags & NODE_IS_DG_ENTRY_EXIT); }
    bool isCfgEntry () const { return (_flags & NODE_IS_DG_ENTRY); }
    bool isCfgExit () const { return (_flags & NODE_IS_DG_EXIT); }

    bool isInstructionNode() const { return (type>=0); }
    bool is_intrinsic_type() const { return (type == IB_intrinsic); }
    
    int memoryOpIndex() const  { return (memopidx); }
    void setMemoryOpIndex(int idx)   { memopidx = idx; }
    
    friend class SchedDG;
    friend class SchedDG::SortNodesByDependencies;
    
    void addRegReads(const register_info& reg)   { srcRegs.push_back(reg); }
    void addRegWrites(const register_info& reg)  { destRegs.push_back(reg); }
    void addImmValue(const imm_value_t& imm)   { immVals[num_imm_values++] = imm; }

    inline SchedDG* in_cfg() const { return (_in_cfg); }

    void addCycle (unsigned int cycId, bool _is_segment);
    int removeCycle (unsigned int cycId);  // return 1 if success, 0 otherwise
    
    void resetSchedule (unsigned int sched_len)
    {
       scheduled = 0;
       numUnschedules = 0;
       _flags = _flags & (~NODE_HAS_MIXED_EDGES);
       schedTime.setScheduleLength (sched_len);
    }
    bool isScheduled () const { return (scheduled!=0); }
    int scheduleId () { return (scheduled); }
    const ScheduleTime& getScheduleTime() { return (schedTime); }
    const MIAMIU::PairList& getAllocatedUnits()   { return (allocatedUnits); }

    // following operators compare two nodes by their main priority criterias 
    // but return true only if they are significantly different
    //
    bool operator << (const Node& n2);
    bool operator >> (const Node& n2);
    
    inline bool is_load_instruction() const { 
       return (InstrBinIsLoadType((InstrBin)type)); 
    }
    inline bool is_store_instruction() const { 
       return (InstrBinIsStoreType((InstrBin)type)); 
    }
    inline bool is_memory_reference() const { 
       return (InstrBinIsMemoryType((InstrBin)type)); 
    }
    inline bool is_nop_instruction() const { 
       return (InstrBinIsNOP((InstrBin)type)); 
    }
    inline bool is_data_movement() const {
       return (InstrBinIsRegisterMove((InstrBin)type)); 
    }
    inline bool is_prefetch_op() const {
       return (InstrBinIsPrefetch((InstrBin)type)); 
    }
    inline bool is_branch_instruction() const { 
       return (InstrBinIsBranchType((InstrBin)type)); 
    }
    
    inline bool isBarrierNode () const  { return (_flags & (NODE_IS_BARRIER)); }

    Edge* findIncomingEdge (Node *src, int edirection, int etype, 
                      int eiters, int elevel);
    Edge* findOutgoingEdge (Node *dest, int edirection, int etype, 
                      int eiters, int elevel);
    inline bool IsRemoved()    { return (_flags & PRUNED_NODE); }
    inline Node* getReplacementNode () const 
    { 
       Node *tempNode = replNode;
       while (tempNode!=NULL && tempNode->IsRemoved())
       {
          tempNode = tempNode->replNode;
       }
       return (tempNode); 
    }
    
    inline void setEntryValue (EntryValue *_val) { entryVal = _val; }
    inline EntryValue *getEntryValue () const    { return (entryVal); }
    
    InstructionClass makeIClass() const
    {
       InstructionClass ic;
       int vlen = 0;
       if (exec_unit == ExecUnit_VECTOR)
          vlen = bit_width * vec_len;   // should I use the new vec_width field here??
       ic.Initialize((InstrBin)type, exec_unit, vlen, exec_unit_type, bit_width);
       return (ic);
    }
    
    inline void setSccId(int _scc) { sccId = _scc; }
    inline int getSccId() const    { return (sccId); }
    
    inline bool isLoopBoilerplate() const { 
       return ((_flags & (NODE_IS_LOOP_CONDITION)) || 
               (_flags & (NODE_IS_ADDRESS_CALCULATION))
              ); 
    }

    inline bool isLoopCondition() const { 
       return (_flags & (NODE_IS_LOOP_CONDITION));
    }
    inline bool isAddressCalculation() const { 
       return (_flags & (NODE_IS_ADDRESS_CALCULATION));
    }
    
    void propagateAddressCalculation();
    void propagateLoopCondition();
    inline void setNumUopsInInstruction(int n_uops) { num_uops = n_uops; }
    inline int getNumUopsInInstruction() const { return (num_uops); }
    
    inline void setInOrderIndex(int _idx) { in_order = _idx; }
    inline int getInOrderIndex() const { return (in_order); }
    // return true if all dependencies from nodes with lower
    // in_order index are satisfied
    // if initial is set, check just the presence of dependencies, not the
    // scheduling status of the uops it depends on. None of them should 
    // be scheduled, but their status may not reflect this.
    bool InOrderDependenciesSatisfied(bool initial=false, Edge **dep = 0);
    
    bool is_scalar_stack_reference();
//OZGURS
    RInfoList getSrcReg(){return srcRegs;}
    RInfoList getDestReg(){return destRegs;}
    bool is_strided_reference();
    int checkDependencies(std::map<Node* ,std::list<depOffset>> *depMap ,int level, int index);
    bool recursive_check_dep_to_this_loop(register_info inSrcReg ,Node *nn, int level, 
                        int index ,std::list<depOffset> *depLevels);
    bool is_registers_set_in_loop(sliceVal in_val,Node *nn ,Node* firstNode ,int level, int index);
    bool is_registers_set_in_level(sliceVal in_val,Node *nn ,Node* firstNode ,int level);
    bool is_dependent_to_upper_loops();
//OZGURE    

    inline RetiredUopType getRetiredUopType() const
    {
       if (is_branch_instruction())
          return (UOP_BRANCH);
       else if (is_prefetch_op())
          return (UOP_PREFETCH);
       else if (is_data_movement())
          return (UOP_DATA_MOVE);
       else if (is_memory_reference())
          return (UOP_MEMORY_UTIL);
       else if (exec_unit_type == ExecUnitType_FP)
          return (UOP_FLOATING_UTIL);
       else
          return (UOP_INTEGER_UTIL);
    }

    void write_label (std::ostream& os, int info=0, bool html_escape=false);
   
    inline bool isSeenInFP(){ return seenInFP;} 
    inline void setSeenInFP(){ seenInFP=true;} 
  private:
    inline void setAddressCalculation() { 
       _flags |= NODE_IS_ADDRESS_CALCULATION;
    }
    inline void setLoopCondition() { 
       _flags |= NODE_IS_LOOP_CONDITION;
    }

    inline void MarkRemoved ()  { _flags |= PRUNED_NODE; }
    inline void setReplacementNode (Node *_repl) { replNode = _repl; }
    
    void Ctor() 
    { 
       _flags = 0; _level = 0; _loopIdx = 0;
       sccId = 0;
       resourceScore = 0.f;
//       dependencyScore = 0;
       longestCycle = 0;
       sumAllCycles = 0;
       scheduled = 0;
       hasCycles = 0;
       pathFromRoot = 0;
       edgesFromRoot = 0;
       pathToLeaf = 0;
//ozgurS
       longestCycleMem = 0;
       longestCycleCPU = 0;
       pathToLeafCPU = 0;
       pathToLeafMem = 0;
//ozgurE
       edgesToLeaf = 0;
#if USE_SCC_PATHS
       sccPathFromRoot = 0;
       sccEdgesFromRoot = 0;
       sccPathToLeaf = 0;
       sccEdgesToLeaf = 0;
#endif
       pathFromCycle = 0;
       edgesFromCycle = 0;
       pathToCycle = 0;
       edgesToCycle = 0;
       pathFromEntry = 0;
       pathFromEntryCPU = 0;//ozgurS
       pathFromEntryMem = 0;//ozgurE
       edgesFromEntry = 0;
       distFromEntry = 0;
       pathToExit = 0;
       pathToExitCPU = 0;//ozgurS
       pathToExitMem = 0;//ozgurE
       isHitCalculated = false;
       edgesToExit = 0;
       distToExit = 0;
       edgesDownstream = 0;
       edgesUpstream = 0;
       minSchedIdUpstream = 0;
       minSchedIdDownstream = 0;
       superE = NULL;
       replNode = NULL;
       entryVal = 0;
       structureId = 0;
       fanin = 0;
       fanout = 0;
       memopidx = -1;
       numUnschedules = 0;
       maxRemaining = 0;
       remainingEdges = 0;
       num_imm_values = 0;
       unit = DEFAULT_UNIT;
       num_uops = 1;
       in_order = 0;  // assign an index in the order in which instructions are found in the path
       id = _in_cfg->nextNodeId++; _visited = 0; tempVal = 0; 
       seenInFP = false;
       _execCount = 0;
    }
    
#if USE_SCC_PATHS
    int computeSccPathToLeaf (unsigned int marker, int sccId);
    int computeSccPathFromRoot (unsigned int marker, int sccId);
#endif
//ozgurS
    retValues myComputePathToLeaf (unsigned int marker, bool manyBarriers, DisjointSet *dsi, float &memLatency, float &cpuLatency , MIAMI::MemListPerInst * memData);
//ozgurE
    int computePathToLeaf (unsigned int marker, bool manyBarriers, 
                  DisjointSet *ds);
    int computePathFromRoot (unsigned int marker, bool manyBarriers, 
                  DisjointSet *ds);
    bool computeLowerBound (ScheduleTime &lbSt, bool& mixedEdges, bool superE);
    bool computeUpperBound (ScheduleTime &ubSt, int &upperSlack,
               bool &only_bypass_edges, bool& mixedEdges, bool superE);
    void setNodeOfFailure()    { _flags = _flags | (NODE_OF_FAILURE); }
    void resetNodeOfFailure()  { _flags = _flags & ~(NODE_OF_FAILURE); }
    bool NodeOfFailure() const { return (_flags & (NODE_OF_FAILURE)); }

    SchedDG* _in_cfg;
    int _level;
    int _loopIdx;
    int hasCycles;
    unsigned int _visited;
    unsigned int tempVal;
    // track registers used by an instruction. We do not really differentiate
    // between the different types any more. They will be stored in a single 
    // array for reads, and another array for writes.
    RInfoList srcRegs;
    RInfoList destRegs;
    // before we kept separate arrays for each type
    
    ObjId id; // 0 is reserved; address of instruction or unique id
    addrtype address;
    int index;  // on CISC architectures we decode a native instruction into
                // multiple RISC micro-ops, with one graph node for each one; 
                // index represents the index of this node's micro-op
    
    // Machine independent instruction characteristics
    int type;   // InstrBin type
    width_t bit_width;
    int vec_len;
    int vec_width;
    ExecUnit exec_unit;
    ExecUnitType exec_unit_type;
    
    int sccId;           // the ID of the SCC this node is part of, or 0 if none.
    int _flags;
    int unit;            // store the unit that prevented this node from being scheduled closer to
                         // its source or sink
    int structureId;     // 0 if part of no structure; else the structure id
    int structLevel;     // node level inside a structure; level grows from struct entry to exit
    int scheduled;       // 0 means unscheduled, else it is schedule order
    int templIdx;        // idx of template used; meaningless if not scheduled
    unsigned int fanin;  // number of incomming scheduling edges
    unsigned int fanout; // number of outgoing scheduling edges
    int memopidx;        // index of memory operand (only applicable to nodes
                         // corresponding to memory micro-ops)
    
    MIAMIU::PairList allocatedUnits;
    ScheduleTime schedTime;
    
    Node *replNode;   // if this node is replaced by another one, set it here
    Edge* superE;     // shortest segment containing this node in the sense
                      // that its ends are the closest to this node.
    
    EntryValue *entryVal;
    
    float resourceScore;
    int longestCycle;
    int sumAllCycles;
    int pathFromRoot;
    int edgesFromRoot;
    int pathToLeaf;
//ozgurS
    int longestCycleMem;
    int longestCycleCPU;
    int pathToLeafCPU;
    int pathToLeafMem;
    MIAMI::InstlvlMap * lvlMap;
    MIAMI::memStruct memData;
    bool isInLoop;

//ozgurE
    int edgesToLeaf;
#if USE_SCC_PATHS
    int sccPathFromRoot;
    int sccEdgesFromRoot;
    int sccPathToLeaf;
    int sccEdgesToLeaf;
#endif

    int pathFromCycle;
    int edgesFromCycle;
    int pathToCycle;
    int edgesToCycle;
    int pathFromEntry;
//ozgurS
    int pathFromEntryCPU;
    int pathFromEntryMem;
//ozgurE
    int edgesFromEntry;
    int distFromEntry;
    int pathToExit;
//ozgurS
    int pathToExitCPU;
    int pathToExitMem;
   bool isHitCalculated;

//ozgurE
    int edgesToExit;
    int distToExit;
    
    int edgesDownstream;       // represents the downstream distance, or
                               // # edges I went towards the graph's roots
    int edgesUpstream;         // represents the upstream distance, or
                               // # edges I went towards the leaves
    int minSchedIdDownstream;  // the earliest sched id limiting
                               // this node's upper bound
    int minSchedIdUpstream;    // the earliest sched id limiting
                               // this node's lower bound
    int numUnschedules;        // how many times I started to unschedule nodes
                               // starting from current node
    
    int maxRemaining;
    int remainingEdges;
    int num_uops;
    int in_order;              // decoding order index, used for list scheduling
    MIAMIU::UISet cyclesIds;
    
    MIAMIU::UISet inCycleSets;
    MIAMIU::UISet outCycleSets;
    int num_imm_values;
    imm_value_t immVals[max_num_imm_values];
   //OzgurS
    bool seenInFP;
    double _execCount;
   //E
  };
  //------------------------------------------------------------------------------------------------------------------
  typedef std::map <unsigned int, Edge*> UiEdgeMap;
  typedef std::set <Edge*> EdgeSet;
  
  class Edge : public DGraph::Edge, public PrintableClass {
  public:
    Edge (Node* n1, Node* n2, int _ddirection, int _dtype, 
            int _ddistance, int _dlevel, int _overlapped, 
            float _execProbab = 1.0);
    ~Edge () {
       cyclesIds.clear ();
       if (subEdges)
       {
          assert (dtype == SUPER_EDGE_TYPE);
          delete (subEdges);
       }
#if 0
       if (pathsIds)
       {
          assert (dtype==SUPER_EDGE_TYPE);
          delete (pathsIds);
       }
#endif
#if 0
       if (superEdges)
          delete (superEdges);
#endif
    }
    
    int getType() const      { return dtype; }
    int getDirection() const { return ddirection; }
    int getDistance() const  { return ddistance; }
    int getLevel() const     { return dlevel; }
    
    // getId: An id unique within instances of DG::Edge
    virtual ObjId getId() const { return id; }
    
    Node* source () const { return dynamic_cast<Node*>(n1); }
    Node* sink () const   { return dynamic_cast<Node*>(n2); }
    
    // this method is used to filter out non trivial edges when iterating over
    // all edges from/to a node. For example, anti- and output- register 
    // dependencies can be removed by register renaming, therefore they are
    // considered trivial. It is better to create this method, so we have one
    // point of control in case the types of dependencies change in the future.
    inline bool isSchedulingEdge() const
    { 
       return ((dtype==MEMORY_TYPE) || (ddirection==TRUE_DEPENDENCY));
    }
    inline bool isSuperEdge() const { 
       return (dtype==SUPER_EDGE_TYPE);
    }
    inline bool isStructureEdge() const { 
       return (dtype==STRUCTURE_TYPE);
    }
            
    void dump (std::ostream& os);
    void dump () { dump(std::cout); }

    void draw (std::ostream& os, DependencyFilterType f, int maxCycle);
    void draw_scheduling (std::ostream& os, DependencyFilterType f, 
            int maxCycle);
    void draw (DependencyFilterType f, int maxCycle) 
    { 
       draw(std::cout, f, maxCycle);
    }
    
    void PrintObject (ostream& os) const;

    void addCycle (unsigned int cycId, bool _is_segment);
    int removeCycle(unsigned int cycId);  // return 1 if success, 0 otherwise
    
    // return true if edge is marked as invisible and its visibility flag is
    // different than _visib
    inline bool IsInvisible (unsigned int _visib=0) const
    { 
       return ((_flags&INVISIBLE_EDGE) && (_visib!=visibility_flag)); 
    }
    inline void MarkInvisible (unsigned int _visib)
    {
       // it should be marked with a visibility flag > 0
       assert (_visib > 0);
       visibility_flag = _visib;
       _flags |= INVISIBLE_EDGE;
    }

    void visit(unsigned int m)         { _visited = m; }
    bool visited(unsigned int m) const { return (m==_visited); }
    bool visited(unsigned int mL, unsigned int mU) const 
    {
       return (mL<=_visited && _visited<mU); 
    }

    // a second set of markers that I need for some ugly heuristics
    void visit2(unsigned int m)         { _visited2 = m; }
    bool visited2(unsigned int m) const { return (m==_visited2); }

    inline bool IsRemoved() const    { return (_flags & PRUNED_EDGE); }
    int getLatency() const  { return (minLatency); }
    int getActualLatency() const
    {
       if (realLatency == NO_LATENCY)
          return (minLatency);
       else
          return (realLatency);
    }
/*
 * ozgurS
 * adding getMemLatency, getCPULatency, getActualMemLatency and getActualCPULatency 
 * function to return new variables
 */ 
    int getMemLatency() const  { return (minMemLatency); }
    int getActualMemLatency() const
    {
       if (realMemLatency == NO_LATENCY)
          return (minMemLatency);
       else
          return (realMemLatency);
    }

    int getCPULatency() const  { return (minCPULatency); }
    int getActualCPULatency() const
    {
       if (realCPULatency == NO_LATENCY)
          return (minCPULatency);
       else
          return (realCPULatency);
    }
/*
 * ozgurE
 */

    float getProbability() const   { return (_prob); }
    void computeLatency(const Machine* _mach, addrtype reloc,SchedDG* sdg);
    inline bool HasLatency() const { return (_flags & LATENCY_COMPUTED_EDGE); }
    
    bool isBypassEdge() const  { return (_flags & HAS_BYPASS_RULE_EDGE); }
    
    // check if the edge is advancing on the path from Entry to Exit inside a structure
    bool is_forward_edge(int entryLevel, int exitLevel);

    void resetSchedule () 
    { 
       _flags = _flags & (~SCHEDULE_DONE_EDGE);
       usedSlack = 0; prioTimeStamp = 0;
       if ( !(_flags & HAS_BYPASS_RULE_EDGE) )
       {
          realLatency = NO_LATENCY;
//ozgurS
          realMemLatency = NO_LATENCY;
          realCPULatency = NO_LATENCY;
//ozgurE
       }
    }
    bool isScheduled () const { return (_flags & SCHEDULE_DONE_EDGE); }
    void markScheduled ()     { _flags = (_flags | SCHEDULE_DONE_EDGE); }
    void markUnScheduled ()   { _flags = (_flags & ~(SCHEDULE_DONE_EDGE)); }

    bool isIndirect() const { return (_flags & EDGE_IS_INDIRECT_DEPENDENCE); }
    void markIndirect()     { _flags = (_flags | EDGE_IS_INDIRECT_DEPENDENCE); }
    
    friend class SchedDG;
    friend class SchedDG::SortEdgesByDependencies;
    
  private:
    inline void MarkRemoved()  { _flags |= PRUNED_EDGE; }
    void write_label (std::ostream& os);
    void write_style (std::ostream& os, DependencyFilterType f, unsigned int maxCycle);
    void setEdgeOfFailure()    { _flags = _flags | (EDGE_OF_FAILURE); }
    void resetEdgeOfFailure()  { _flags = _flags & ~(EDGE_OF_FAILURE); }
    bool EdgeOfFailure() const { return (_flags & (EDGE_OF_FAILURE)); }

//    static unsigned int nextCycleId;
    int _flags;
    int minLatency;
    int realLatency;
/*
 * ozgurS adding minMemLatency, minCPULatency , realMemLatency and realCPULatency
 */
    int minMemLatency;
    int minCPULatency;
    int realMemLatency;
    int realCPULatency;
    int longestCycleMem;
    int longestCycleCPU;
//memory hit informations
    double hitsPerPath;
    double hitsPerBlock;
    double hitsPerInstuction;
    /*
 * ozgurE
 */


    float resourceScore;
    int longestCycle;
    int sumAllCycles;
    int longestPath;
    int neighbors;
    int extraPriority;
    int maxRemaining;
    int remainingEdges;
    int hasCycles;
    int structureId;
    int longestStructPath;
    int prioTimeStamp;     // store the time step of when the priority 
                           // has been computed
    
    int usedSlack;
    unsigned int visibility_flag;
    unsigned int _visited;
    unsigned int _visited2;
    double numPaths;
    
    int ddirection;
    int dtype;
    int ddistance;
    int dlevel;    // I will look only at innermost loops, and thus level can
                   // be at most 1 (at least for now)
    int overlapped;
    float _prob;   // probability: it is used for control dependencies
    ObjId id; // 0 is reserved; first instance is 1
    
    // Next few fields are associated with super-edges
#if 0
    int numSuperEdges; // how many super-edges associated with this edge?
    UiEdgeMap *superEdges;
#endif
    Edge* superE;   // pointer to the strictest inner-most super edge (in the 
                    // sense that its two ends are consecutive barrier nodes),
                    // and of the shortest distance, where applicable, 
                    // otherwise = NULL
    MIAMIU::UISet cyclesIds; // tracks cycles/paths that this edge is a member of
    EdgeSet *subEdges; // tracks which edges are included in each 
                       // super-structure. Not valid for non super edges.
#if 0
    UISet *pathsIds; // tracks which paths are represented by this super edge.
                     // Not valid for non super edges.
#endif
  };
  //------------------------------------------------------------------------------------------------------------------
  
  class SortNodesByDependencies : public std::binary_function<Node*, Node*, bool>
  {
  public:
     SortNodesByDependencies (bool _heavyOnResources, 
             bool _resourceLimited = false,
             bool _limpMode = false) : 
             heavyOnResources (_heavyOnResources),
             resourceLimited (_resourceLimited),
             limpMode (_limpMode)
     { }
     
     bool operator () (Node*& n1, Node*& n2) const;

  private:
     bool heavyOnResources;
     bool resourceLimited;
     bool limpMode;
  };

  class SortEdgesByDependencies : public std::binary_function<Edge*, Edge*, bool>
  {
  public:
     SortEdgesByDependencies (bool _SWP_enabled, bool _heavyOnResources,
           float _resourceLimitedScore)
         : SWP_enabled(_SWP_enabled), heavyOnResources(_heavyOnResources), 
           resourceLimitedScore(_resourceLimitedScore)
     {
        if (resourceLimitedScore >= 0.7)
           hiResourceScore = true;
        else
           hiResourceScore = false;
        
        if (resourceLimitedScore < 0.5)
           lowResourceScore = true;
        else
           lowResourceScore = false;
     }
     
     bool operator () (Edge*& e1, Edge*& e2) const;

  private:
     bool SWP_enabled;
     bool heavyOnResources;
     bool hiResourceScore;
     bool lowResourceScore;
     float resourceLimitedScore;
  };

  class SortNodesByDegree : public std::binary_function <Node*, Node*, bool>
  {
  public:
     SortNodesByDegree ()
     { }
     
     bool operator () (Node*& n1, Node*& n2) const;

  private:
  };

  
  typedef std::priority_queue <SchedDG::Node*, std::vector<SchedDG::Node*>,
            SchedDG::SortNodesByDependencies > NodePrioQueue;
  typedef std::priority_queue <SchedDG::Edge*, std::vector<SchedDG::Edge*>,
            SchedDG::SortEdgesByDependencies > EdgePrioQueue;

  typedef std::priority_queue <SchedDG::Node*, std::vector<SchedDG::Node*>,
            SchedDG::SortNodesByDegree > NDegreePrioQueue;
  
  typedef std::list <Node*> NodeList;
  typedef std::list <Edge*> EdgeList;
  typedef std::vector <Node*> NodeArray;
  typedef std::map <Node*, int> NodeIntMap;
  typedef std::set <Node*> NodeSet;
  typedef std::map <unsigned int, NodeSet> Ui2NSMap;
  
  class NodeListIterator : public Iterator {
  public:
    NodeListIterator (NodeList* nl) { list = nl; iter = list->begin(); }
    virtual ~NodeListIterator () {}
    void operator ++ () { if (iter != list->end()) ++iter; }
    operator bool () const { return (iter != list->end()); }
    operator Node* () const { return *iter; }
  private:
    NodeList* list;
    std::list<Node*>::iterator iter;
  };
  //------------------------------------------------------------------------------------------------------------------
  class EdgeListIterator : public Iterator {
  public:
    EdgeListIterator (EdgeList* el) { list = el; iter = list->begin(); }
    virtual ~EdgeListIterator () {}
    void operator ++ () { if (iter != list->end()) ++iter; }
    operator bool () const { return (iter != list->end()); }
    operator Edge* () const { return *iter; }
  private:
    EdgeList* list;
    std::list<Edge*>::iterator iter;
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate nodes in DFS order
  //------------------------------------
  class DFSIterator : public DGraph::DFSIterator {
  public:
    DFSIterator (SchedDG  &g) : DGraph::DFSIterator(g) {}
    virtual ~DFSIterator () {}
    operator Node* () const { return dynamic_cast<SchedDG::Node*>(p); }
    void operator++ () { ++p; }
    operator bool () const { return (bool)p; }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate incoming edges
  //------------------------------------
  class IncomingEdgesIterator : public DGraph::IncomingEdgesIterator {
  public:
    IncomingEdgesIterator (Node  *n) : DGraph::IncomingEdgesIterator(n) {}
    virtual ~IncomingEdgesIterator () {}
    Edge* operator-> () const { return dynamic_cast<SchedDG::Edge*>(*iter); }
    operator Edge* () const { return dynamic_cast<SchedDG::Edge*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate outgoing edges
  //------------------------------------
  class OutgoingEdgesIterator : public DGraph::OutgoingEdgesIterator {
  public:
    OutgoingEdgesIterator (Node  *n) : DGraph::OutgoingEdgesIterator(n) {}
    virtual ~OutgoingEdgesIterator () {}
    Edge* operator-> () const { return dynamic_cast<SchedDG::Edge*>(*iter); }
    operator Edge* () const { return dynamic_cast<SchedDG::Edge*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate predecessor nodes (only over scheduling edges)
  //------------------------------------
  class PredecessorNodesIterator {
  public:
    PredecessorNodesIterator (DGraph::Node* n, bool _incl_struct=false) { 
       iter = new IncomingEdgesIterator (dynamic_cast<Node*>(n));
       include_struct = _incl_struct;
       while ((bool)(*iter) && 
              ((*iter)->IsRemoved() || 
               !((*iter)->isSchedulingEdge() || 
                 (include_struct && (*iter)->isStructureEdge())) 
              ))
          ++ (*iter);
    }
    virtual ~PredecessorNodesIterator () {
       delete iter;
    }
    void operator++ () { 
       if ((bool)(*iter))
          ++ (*iter);
       while ((bool)(*iter) && 
              ((*iter)->IsRemoved() || 
               !((*iter)->isSchedulingEdge() || 
                 (include_struct && (*iter)->isStructureEdge())) 
              ))
//            ((*iter)->IsRemoved() || !(*iter)->isSchedulingEdge()) )
          ++ (*iter);
    }
    operator bool () const { return (bool)(*iter); }
    Node* operator-> () const { 
       return dynamic_cast<SchedDG::Node*>((*iter)->source()); 
    }
    operator Node* () const { 
       return dynamic_cast<SchedDG::Node*>((*iter)->source()); 
    }
  private:
    IncomingEdgesIterator *iter;
    bool include_struct;
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate succesor nodes (only over scheduling edges)
  //------------------------------------
  class SuccessorNodesIterator {
  public:
    SuccessorNodesIterator (DGraph::Node* n, bool _incl_struct=false) { 
       iter = new OutgoingEdgesIterator (dynamic_cast<Node*>(n));
       include_struct = _incl_struct;
       while ((bool)(*iter) && 
              ((*iter)->IsRemoved() || 
               !((*iter)->isSchedulingEdge() || 
                 (include_struct && (*iter)->isStructureEdge())) 
              ))
//            ((*iter)->IsRemoved() || !(*iter)->isSchedulingEdge()) )
          ++ (*iter);
    }
    virtual ~SuccessorNodesIterator () {
       delete iter;
    }
    void operator++ () { 
       if ((bool)(*iter))
          ++ (*iter);
       while ((bool)(*iter) && 
              ((*iter)->IsRemoved() || 
               !((*iter)->isSchedulingEdge() || 
                 (include_struct && (*iter)->isStructureEdge())) 
              ))
//            ((*iter)->IsRemoved() || !(*iter)->isSchedulingEdge()) )
          ++ (*iter);
    }
    operator bool () const { return (bool)(*iter); }
    Node* operator-> () const { 
       return dynamic_cast<SchedDG::Node*>((*iter)->sink()); 
    }
    operator Node* () const { 
       return dynamic_cast<SchedDG::Node*>((*iter)->sink()); 
    }
  private:
    OutgoingEdgesIterator *iter;
    bool include_struct;
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate nodes in no particular order
  //------------------------------------
  class NodesIterator : public BaseGraph::NodesIterator {
  public:
    NodesIterator (SchedDG &g) : BaseGraph::NodesIterator(g) {}
    virtual ~NodesIterator () {}
    Node* operator-> () const { return dynamic_cast<SchedDG::Node*>(*iter); }
    operator Node* ()  const { return dynamic_cast<SchedDG::Node*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate removed nodes in no particular order
  //------------------------------------
  class RemovedNodesIterator : public Iterator {
  public:
     RemovedNodesIterator (SchedDG& g) { gr = &g;  
                          iter = gr->removed_node_set.begin(); }
     ~RemovedNodesIterator ()  {}
     void operator++ ()        { ++iter; }
     operator bool () const    { return (iter != gr->removed_node_set.end()); }
     void Reset()              { iter = gr->removed_node_set.begin(); }
     Node* operator-> () const { return (*iter); }
     operator Node* ()  const  { return (*iter); }
  protected:
     NodeSet::iterator iter;
     SchedDG* gr;
  };
  //------------------------------------------------------------------------------------------------------------------
  /** The EdgesIterator is just an extension of BaseGraph::EdgesIterator */
  class EdgesIterator : public BaseGraph::EdgesIterator {
  public:
    EdgesIterator (SchedDG& g) : BaseGraph::EdgesIterator(g) {}
    virtual ~EdgesIterator () {}
    operator Edge* () const { return dynamic_cast<Edge*>(*iter); }
    Edge* operator -> () const { return dynamic_cast<Edge*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------

  // ATTENTION PLEASE: Cycles are fragile with respect to the structure of 
  // the graph. Each cycle keeps track of its edges. Moreover, the 
  // construction and destruction of a Cycle will invoke a method of each edge. 
  // Therefore, cycles must be discovered after all the transformations on the 
  // graph have been completed. If new transformations must be applied, remove 
  // all cycles first, then transform the graph and recompute the cycles again.
  // Do not delete edges from the graph before all cycles have been removed.
  // We use the Cycle class to store paths through super-structures as well.
   class Cycle : public PrintableClass
   {
   public:
      Cycle(unsigned int _id, EdgeList& eList, DisjointSet *ds, 
              bool _is_segment=false);
      ~Cycle();
      ObjId getId()   { return cycleId; }
      bool equalTo(EdgeList &el);

      void PrintObject (ostream& os) const;
      
      // next 3 methods deal with cycle's latency
      // getLength returns the latency of the cycle if it was computed,
      // or the value computed when the cycle was constructed (== number 
      // of edges if edges do not have latency information computed)
      // This method does not make sure the latency was computed
      int getLength() { return (iterLength); }
      int getRawLength() { return (cLength); }

      // HasLatency returns true if the cycle has latency information 
      // computed already
      bool HasLatency()  { return (_flags & LATENCY_COMPUTED_CYCLE); }

      // getLatency checks if the latency information is computed. If not,
      // it attempts to contruct it. If successfull, it also sets the flag
      // that latency information is computed. If unsuccessfull, it does not
      // set the flag and it will attempt to construct it again next time it
      // is called. If latency info is constructed, it just returns it.
      int getLatency();     // returns latency per iteration
      int getRawLatency();  // returns total cycle length
//ozgurS      
      int getMemLatency();     // returns mem latency per iteration
      int getRawMemLatency();  // returns mem total cycle length
      int getCPULatency();     // returns cpu latency per iteration
      int getRawCPULatency();  // returns cpu total cycle length

//ozgurE

      int getNumEdges() const { return (numEdges); }
      
      // number of crossed iterations
      int numIterations () { return (iters); }
      int getUsedSlack() { return (usedSlack); }
      int findMaxSchedID ();
      
      // is this cycle a segment? check if first and last nodes are different
      bool isSegment () const { return (is_segment || (firstNode!=lastNode)); }
      bool isCycle () const { return (firstNode==lastNode && !is_segment); }
      Node *FirstNode () const { return (firstNode); }
      Node *LastNode () const { return (lastNode); }
      
      void resetScheduleInfo () 
      {
         usedSlack=0; remainingLength=cLength; remainingEdges=numEdges;
         fixedNodes = 0;
      }
      int getRemainingLength () const { return (remainingLength); }
      int getRemainingEdges () const { return (remainingEdges); }
      
      int numFixedNodes () const { return (fixedNodes); }
      double numberOfPaths () const  {
         return (numDiffPaths);
      }
      
      friend class SchedDG;
      
   private:
      double numDiffPaths;
      ObjId cycleId;
      int cLength;  // keep the latency of the cycle
      int iters;    // number of iterations crossed by this cycle
      int iterLength; // cycle length per iteration crossed
      int numEdges;
      int usedSlack;
      int remainingLength;
      int remainingEdges;
      int _flags;
      int fixedNodes;
      float revIters;
      EdgeList *edges;
      Node *firstNode;
      Node *lastNode;
      bool is_segment;
   };
   //------------------------------------------------------------------------------------------------------------------
   class EdgeListCompare
   {
   public:
      bool operator() (const EdgeList* el1, const EdgeList* el2) const;
   };
   //------------------------------------------------------------------------------------------------------------------

#if 0
   class PathSlackInfo
   {
   public:
      PathSlackInfo ()
      {
         pLength = 0; usedSlack = 0; iters = 1;
      }

      unsigned int getLength() const { return (pLength); }
      unsigned int numIterations () const { return (iters); }
      int getUsedSlack() { return (usedSlack); }
      
      unsigned int pLength;   // path length; for cycles is the total length
      unsigned int iters;     // number of iterations; for non-cycles is 1
      int usedSlack;          // how much slack has been used
   };
#endif
   //------------------------------------------------------------------------------------------------------------------
//   typedef std::map<unsigned int, Cycle*> CycleMap;
   typedef std::multimap <EdgeList*, Cycle*, EdgeListCompare> CycleMap;

   Edge* addDependency(Node* n1, Node* n2, int _ddirection, int _dtype, 
            int _ddistance, int _dlevel, int _overlapped, float _execProb=1.0);
   Edge* addUniqueDependency(Node* n1, Node* n2, int _ddirection, 
            int _dtype, int _ddistance, int _dlevel, int _overlapped,
            float _execProb=1.0 );
   const char* name() const { return routine_name; }

   // use arguments of type DGraph::Edge* and DGraph::Node*, to avoid the
   // warnings
   virtual void add (DGraph::Edge* e);
   virtual void add (DGraph::Node* n);
   virtual void remove (DGraph::Edge* e);
   virtual void remove (DGraph::Node* n);
      
   void dump (std::ostream& os);
   void dump () { dump(std::cout); }

   void draw (std::ostream& os, const char* name, 
              DependencyFilterType f, int maxCycle);
   void draw (const char* name, DependencyFilterType f, int maxCycle) 
   { 
      draw(std::cout, name, f, maxCycle); 
   }
   void draw_legend (std::ostream& os);

   void draw_scheduling (std::ostream& os, const char* name);
   void write_machine_units (std::ostream& os, MIAMIU::PairList& selectedUnits, 
            int offset);

   void computeSuperStructures (CycleMap *all_cycles);

   // resetIds: reset id numbering
   void resetIds() { nextNodeId = 1; nextEdgeId = 1; }
   virtual ObjId HighWaterMarker() const { return nextNodeId; }
   
   /* remove redundant dependencies such when an instruction is dependent on
    * another instruction through one chain of dependencies and through
    * one edge directly. Remove the direct edge as it is less strict than
    * the dependency chain. Return number of deleted dependencies.
    * Another option is to implement pruning on the graph with edge weights.
    */
   int pruneDependencyGraph ();

   void updateGraphForMachine (const Machine* _mach);
   Node* CfgTop()    { return (cfg_top); }
   Node* CfgBottom() { return (cfg_bottom); }

   schedule_result_t computeScheduleLatency (int graph_level, int detailed);
//ozgurS   
   schedule_result_t myComputeScheduleLatency (int graph_level, int detailed, float &memLatency, float &cpuLatency);
//ozgurE
   void draw_min (ostream& os, const char* name, DependencyFilterType f);
   void draw_debug_min_graphics (const char* prefix);
   void draw_debug_graphics (const char* prefix, bool draw_reg_only = false,
          int max_cycles = 0);
   
   const TimeAccount& getTimeStats () const { return (timeStats); }
   const TimeAccount& getUnitUsage () const { return (unitUsage); }

   void printTimeAccount();
   
private:
   int availableSlack (Edge *edge);
   int canAccomodateSlack (Edge *edge, unsigned int &slack);
   int allocateSlack (Edge *edge, unsigned int &slack, Edge* &supEdge);
   int deallocateSlack (Edge *edge, Edge *supEdge=0);

   void undo_partial_node_scheduling (Node *nn, EdgeList *newEdges, 
           EdgeList::iterator *endit);
   bool computeMaxRemaining (Edge *crtE);
   void updateLongestCycleOfSubEdges (Edge *supE, int cycLen);

   void recursive_buildSuperStructures_header (int crtIdx, Node **sEntry, 
              Node **sExit);
   void recursive_buildSuperStructures (int crtIdx, Node **sEntry, 
              Node **sExit);
   int lowerPathDistances (Node* node, unsigned int m, unsigned int m2,
              Node *finalN, int sId, int toLower, bool superEntry);
   void correctEdgeDistances (Edge *ee, int &dist, EdgeList &finishedEdges, 
        int &oldDist, unsigned int m, Node *finalN, int sId)
        throw (RecomputePathsToExit);
//   int findStructPaths (Node* node, unsigned int m, EdgeList& _edges, 
//              Node *finalN, int sId, CycleMap *all_cycles);

   int findLongestPathToExit (Node* node, unsigned int m, unsigned int m2,
              Node *finalN, int sId, Edge *segmE, Edge *lastE, 
              int entryLevel, int exitLevel);
   int findLongestPathFromEntry (Node* node, unsigned int m, unsigned int m2,
              Node *finalN, int sId, Edge *segmE, Edge *lastE,
              int entryLevel, int exitLevel);

   void draw_dominator_tree (BaseDominator *bdom, const char *prefix);

   void draw_cycle_header (std::ostream& os, int cycle);
   void draw_cycle_footer (std::ostream& os, int cycle, NodeList &nl);
   void draw_time_account (std::ostream& os, int minCycIndex, 
                int lastNodeMaxCyc, bool before);
   void draw_cluster (ostream& os, int idx, int indent);

   void findDependencyCycles();
   int numberOfCycles()   { return (nextCycleId-1); }

   unsigned int addCycle (EdgeList& el, CycleMap* all_cycles, 
             bool is_segment=false);
   unsigned int addUniqueCycle (EdgeList& el, CycleMap* all_cycles, 
             bool is_segment=false);
   int  removeCycle(unsigned int cycId);
   void removeAllCycles();

//   Edge* updateSuperEdge (Node *src, Node *dest, EdgeList &el);
   int findCycle (SchedDG::Node* node, unsigned int m, unsigned int m2,
           EdgeList& _edges, CycleMap* all_cycles, unsigned int lastIdx,
           unsigned int &minIdx, int iters, int sccId);
   int findSuperEdgeCycle (Node* node, unsigned int m, EdgeList& _edges,
           CycleMap* all_cycles);

   int pruneGraphRecursively(SchedDG::Node* node, unsigned int cMark,
         int lastChild, int maxLatency, unsigned int vMark, int numEdges, 
         int numIters, int latency, int barriers, int backEdges, 
         Edge** inEdges, int toRemove, bool loadToReg);

   void patternSearch (PatternGraph::Node* findNode, MIAMIU::UIList& iterCrossed) 
         throw (PatternFoundException);
   void recursivePatternSearch (Node* node, PatternGraph::Node* findNode,
               MIAMIU::UIList& iterCrossed, int maxLevel, int backedges) 
         throw (PatternFoundException);
   void replacePattern(ReplacementRule* rule, unsigned int replacementIndex,
               MIAMIU::UIList& iterCrossed);

   void ComputeNodeLevels();
   int computeTopNodes();
   void recursiveNodeLevels(Node* node, int lev, unsigned int m);
   void computeNodeDegrees();

   void computeEdgeLatencies();
   void pruneControlDependencies();

   int minSchedulingLengthDueToResources(int& bottleneckUnit, 
             unsigned int marker, Edge *superE, double& vecBound, int& vecUnit);
   int minSchedulingLengthDueToDependencies();
//ozgurS
   retValues  myMinSchedulingLengthDueToDependencies(float &memLatency, float &cpuLatency);
//ozgurE
   void markLoopBoilerplateNodes();
   
   bool create_schedule_main(int& unit, Node* &fNode, Edge* &fEdge, 
          int& extra_length, int& numTries, int& segmentId);
   bool create_schedule() 
       throw (UnsuccessfulSchedulingException, SuccessfulSchedulingException);
   int processIncidentEdges(Node *newNode, int templIdx, int i, int unit)
       throw (UnsuccessfulSchedulingException);
   int computeBoundsForNode(Node *newNode, Edge *e, int &ss, int &pp, 
          bool &concentric, int &numTries, ScheduleTime &newTime, 
          int &maxExtraLength, int &unit, int &trycase, bool isSrcNode, 
          bool relaxed = false)
       throw (UnsuccessfulSchedulingException);


   void undo_schedule(Node* newNode, bool aggressive);  // Node* oldNode);
   void undo_schedule_upstream(Node* startNode);
   void undo_schedule_downstream(Node* startNode);
   void unscheduleNode(Node *nn);
   void requeueAllEdges();

   bool edgePartOfAnyFixedCycle(Edge *ee, int &cycId, int &niters);
   bool edgePartOfAnyFixedSegment(Edge *ee);

   int find_memory_parallelism_for_interval(int minLev, int maxLev, 
             CliqueList &allCliques, HashMapUI &refsDist2Use);
   void markDependentRefs(int refIdx, Node *node, char *connected, int N, 
        NodeIntMap &nodeIndex, int uBound, int crtDist, unsigned int mm);

   int computeUnitUsage(int numUnits);
   void countRetiredUops();
   
   void DiscoverSCCs(Node *nn, unsigned int mm, NodeList &nodesStack, int *dfsIndex, int *lowIndex);

protected:
   int pruneMemoryDependencies (NodeArray *memNodes);
   int pruneMemoryRecursively (Node* node, unsigned int cMark,
         int lastChild, unsigned int vMark, int numEdges, int numIters, 
         int backEdges, Edge** inEdges, int toRemove);

   void finalizeGraphConstruction ();
   void switchNodeIds (Node *n1, Node *n2)
   {
      unsigned int tempid = n1->id;
      n1->id = n2->id;
      n2->id = tempid;
   }

   void markBarrierNode (Node *node)
   {
      node->_flags |= (NODE_IS_BARRIER);
      barrierNodes.push_back (node);
   }

   bool hasBarrierNodes () const { return (!barrierNodes.empty()); }
   int find_memory_parallelism (CliqueList &allCliques, 
             HashMapUI &refsDist2Use);
   
private:
   const char* routine_name;
   NodeList bottomNodes;
   unsigned int topMarker;
   ObjId nextNodeId;
   ObjId nextEdgeId;
   
   int nextCycleId;
   // The heuristics used by the scheduling algorithm have bad worse case
   // scenario performance. Detect when the scheduling goes into one of these
   // bad cases, and switch to simpler heuristics that may produce less optimal
   // results but have a better worst-case behavior.
   //
   // 1. When we have a very dense graph and the number of cycles goes over
   // a threashold, switch to limp mode, where we just make sure that every 
   // edge of an SCC is part of at least one cycle, but we do not enumerate
   // all cycles
   bool limpModeFindCycle;
   // 2. When scheduling fails due to diamond-like structure deadlocks, go to 
   // limp mode where we make scheduling one-directional instead of two-ways, 
   // which should avoid deadlocks. It resembles a list scheduler in limp mode.
   bool limpModeScheduling;
   
   Cycle** cycles;      // store the cycles into an array
   double numActualCycles;
   const Machine *mach;
   NodeList topNodes;
   NodeList barrierNodes;
   int maxPathRootToLeaf;
   int longestCycle;
//ozgurS
   int longestCycleMem;
   int longestCycleCPU;
//ozgurE    
   bool resourceLimited;
   bool heavyOnResources;
   float resourceLimitedScore;
   
   BitSet **schedule_template;
   NodePrioQueue *nodesQ;
   EdgePrioQueue *edgesQ;
   EdgePrioQueue *mixedQ;
   
   TimeAccount timeStats;
   TimeAccount unitUsage;
   
   int nextScheduleId;
   int globalTries;
   int maximumGraphLevel;
   int *nextBarrierLevel;
   Node **lastBarrierNode;
   Node **nextBarrierNode;
   
   NodeSet removed_node_set;
   
   CycleSetDAG *cycleDag;
   DisjointSet *ds  ;     // union-find data structure to compute disjoint 
                          // node sets
   Ui2NSMap cycleNodes;   // it will organize nodes into disjoint sets, based 
                          // on their leaders.
   MIAMIU::UiUiMap  setsLongestCycle;
   MIAMIU::PairArray longestCycles;
   int numCycleSets;
   bool longCyclesLeft;
   
   int numStructures;
   NodeList **listOfNodes;
   int *childIdx;
   int *next_sibling;
   
   int globalIndex;  // a generic global index to be used for different numbering 
                     // needs, such as DFS indexing and others.
   int numSchedEdges;
   
protected:
   PathID pathId;
   addrtype reloc_offset;
   int schedule_length;
//ozgurS
   int carryCPULatency;
   int carryMemLatency;
   int schedule_lengthCPU;
   int schedule_lengthMem;
//ozgurE
   int num_instructions;  // count distinct instructions, not micro-ops; must be done at decode time
   unsigned int cfgFlags;
   uint64_t pathFreq;
   float avgNumIters;
   Node *cfg_top, *cfg_bottom;
   Node *lastBranch;
public: //FIXME TODO this should be protected
   // information to determine more precise memory dependencies
   RFormulasMap &refFormulas;
protected:
   LoadModule *img;
   /*  refNames and refsTable have been deprecated. The info is stored in
    *  the LoadModule object now. Use the API to check ref names.
   RefNameMap& refNames;
   AddrUIMap& refsTable;
   */
};
//--------------------------------------------------------------------------------------------------------------------

// a few DependencyFilterType functions
extern bool draw_regonly(SchedDG::Edge* e);
extern bool draw_regmem(SchedDG::Edge* e);
extern bool draw_all(SchedDG::Edge* e);

const char* regTypeToString(int tt);

}  /* namespace MIAMI_DG */

#endif
