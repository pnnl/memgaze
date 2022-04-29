// -*-Mode: C++;-*-

//***************************************************************************
//
// File:
//   $HeadURL$
//
// Purpose:
//   [The purpose of this file]
//
// Description:
//   [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

//************************* System Include Files ****************************

#include <iostream>
#include <fstream>

#include <string>
using std::string;

#include <vector>

//*************************** User Include Files ****************************

#include <include/gcc-attr.h>

#include <tool/hpcprof/Args.hpp>

#include <lib/analysis/CallPath-CudaCFG.hpp>
#include <lib/analysis/CallPath.hpp>
#include <lib/analysis/Util.hpp>

#include <lib/prof/CallPath-Profile.hpp>
//#include <lib/analysis/CCT-Tree.hpp>


#include <lib/support/diagnostics.h>
#include <lib/support/RealPathMgr.hpp>


//*************************** Forward Declarations ***************************

static int
realmain(int argc, char* const* argv);

//****************************************************************************

// tallent: opaque (FIXME)
class MyXFrame;

static Prof::CCT::ANode*
makeCCTPath(MyXFrame* path, uint n_metrics);

static Prof::CCT::ANode*
makeCCTFrame(Prof::LoadMap::LMId_t lmId, VMA ip, uint n_metrics);

static Prof::CCT::ANode*
makeCCTLeaf(Prof::LoadMap::LMId_t lmId, VMA ip, uint n_metrics);

static Prof::CCT::ANode*
makeCCTRoot(uint n_metrics);


// tallent: typical exception wrapper

int
main(int argc, char* const* argv) 
{
  int ret;

  try {
    ret = realmain(argc, argv);
  }
  catch (const Diagnostics::Exception& x) {
    DIAG_EMsg(x.message());
    exit(1);
  } 
  catch (const std::bad_alloc& x) {
    DIAG_EMsg("[std::bad_alloc] " << x.what());
    exit(1);
  }
  catch (const std::exception& x) {
    DIAG_EMsg("[std::exception] " << x.what());
    exit(1);
  } 
  catch (...) {
    DIAG_EMsg("Unknown exception encountered!");
    exit(2);
  }

  return ret;
}


static int
realmain(int argc, char* const* argv) 
{
  // ------------------------------------------------------------
  // Two interpretations of HPCToolkit's CCT for call path profiles.
  // 
  // HPCToolkit's CCT goes through two phases. Our conversion targets
  // the "pre-structure" tree.
  // - Pre-structure: CCT consists of callsites (return addresses) for
  //   interior nodes; and samples for leaf nodes.
  // - Post-structure: Procedure frames, etc are added.
  //
  //
  // 1. A CCT is MemGaze's execution interval tree, where nested time
  //    intervals form "time" call paths. Thus:
  //    - A MemGaze "time interval" node forms a CCT "t-interval" callsite
  //      node in the pre-structure CCT, which will be given a "t-interval"
  //      frame node in the post-structure CCT.
  //    - A MemGaze "sample" (load) node forms a CCT "sample" node.
  //
  //    - ??? Most samples have loads from many procedures. To provide
  //      some "calling" context, add "temporal" callsites (inducing
  //      "temporal" frames) to represent temporally associated frames
  //      (and likely callers). The "temporal" frame is a sequencing
  //      relationship. Could continue to add "temporal" ancestors by
  //      more distant time relationships, assuming a sensible method.
  //   
  //      A sample's "temporal" callsite is its "likely temporal
  //      neighbor (before|after)". It refers to code.
  //
  //      ??? A load-group will be a series of loads with (mostly) the
  //      same frame, gaps < g loads
  //
  //      ??? The "temporal" callsites within a sample correspond to:
  //      Find most frequent frame between the load-groups. This frame
  //      forms a "temporal" callsite for the samples within its
  //      lifetime. The sample remainder is formed from the leftover
  //      loads, i.e., outside the lifetime. Iterate until no
  //      remainder.
  //
  //    * "Callers View": For samples, view calling "temporal frames"
  //      and "t-interval frames".
  //
  //    * "Flat View" has normal meaning for exclusive. Its
  //      "inclusive" metrics are now w.r.t. time correspondence.
  //
  //    The static source code information for a "t-interval frame" is found
  //    in a "t-interval load module" (pseudo) where the callsite IP for a
  //    t-interval is the interval's midpoint (which will be
  //    unique). The t-interval's procedure name is the time interval
  //    string.
  //
  //    Metrics: 
  //
  //    * For CCT, directly represent footprint metrics at each each
  //      node of "CCT" (interior + leaf).
  //
  //      Prof::Metric::ADesc::ComputedTy_Final, Prof::Metric::ADesc::TyIncl
  //      <hpctk>/src/lib/analysis/CallPath.cpp:1152 makeReturnCountMetric()
  //      - Interval id: time-interval midpoint to sort by time/order
  //      - Footprint*: 1-1 correspondance with execution interval tree metrics
  //
  //    * For other views, metrics can be *average* for all aggregated scopes.
  //
  //
  // 2. A CCT is a forest of partial call paths from LBR. The call
  //    path will be correct for the first load; after that we could
  //    miss a return/call.
  //
  //    ??? Try to infer return/call?
  //
  //    ??? instantiate "temporal frames" within LBR's leaf or around
  //    it? Develop notion of "Temporal-calling context".
  //
  // 
  //
  //
  // <hpctk>/src/lib/prof/Metric-Mgr.cpp:274, Mgr::makeSummaryMetric()
  // <hpctk>/src/lib/prof/Metric-Mgr.cpp:409, Mgr::makeSummaryMetricIncr()
  //
  // <hpctk>/src/lib/analysis/CallPath-MetricComponentsFact.cpp:246
  // MPIBlameShiftIdlenessFact::make()
  //
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // 0. Parse arguments
  // ------------------------------------------------------------
  // cf. <hpctk>/src/tool/hpcprof/main.cpp
  
  Args args;
  args.parse(argc, argv);

  // tallent: probably useful
  RealPathMgr::singleton().searchPaths(args.searchPathStr());

  // tallent: may not be useful
  Analysis::Util::NormalizeProfileArgs_t nArgs =
    Analysis::Util::normalizeProfileArgs(args.profileFiles);

  uint n_metrics = 0; // TODO: number expected metrics

  // ------------------------------------------------------------
  // 1a. Create synthetic root in CCT
  // ------------------------------------------------------------

  // <hpctk>/src/lib/analysis/CallPath.cpp:141 read()
  // <hpctk>/src/lib/prof/CallPath-Profile.cpp:1297, fmt_epoch_fread()
  
  uint rFlags = 0;
  Prof::CallPath::Profile* prof = Prof::CallPath::Profile::make(rFlags);

  Prof::CCT::Tree* cct = prof->cct();

  Prof::CCT::ANode* cct_root = makeCCTRoot(n_metrics);

  prof->cct()->root(cct_root);


  // ------------------------------------------------------------
  // 1b. Create CCT from each call path fragment
  //     (CCT is CCT::Call and CCT::Stmt)
  // ------------------------------------------------------------

  // *** TODO: key new code #1 ***

  // <hpctk>/src/lib/prof/CallPath-Profile.cpp:192, Profile::merge()
  // <hpctk>/src/lib/prof/CCT-Tree.cpp:166, Tree::merge()
  // <hpctk>/src/lib/prof/CCT-Tree.cpp:816, ANode::mergeDeep()
  
  if (false /*interval tree*/) {
    // convert interval tree; cf makeCCTPath()
  }
  
  // ------------------------------------------------------------
  // 1c. Create CCT from each LBR call path fragment
  //     (CCT is CCT::Call and CCT::Stmt)
  // ------------------------------------------------------------

  // <hpctk>/src/lib/prof/CallPath-Profile.cpp:192, Profile::merge()
  // <hpctk>/src/lib/prof/CCT-Tree.cpp:166, Tree::merge()
  // <hpctk>/src/lib/prof/CCT-Tree.cpp:816, ANode::mergeDeep()

  if (false /*LBR*/) {
    for (/* each LBR path */ int i = 0 ; i < 1 ; ++i) {
      Prof::CCT::ANode* path_root = makeCCTPath(/*LBR path*/ NULL, n_metrics);
      
      uint x_newMetricBegIdx = 0;
      Prof::CCT::MergeContext mergeCtxt(prof->cct(), /*doTrackCPIds*/false);
      
      Prof::CCT::MergeEffectList* mrgEffects =
        cct_root->mergeDeep(path_root, x_newMetricBegIdx, mergeCtxt);
      
      delete path_root;
      delete mrgEffects;
    }
  }
  
  // tallent: unsure
  prof->disable_redundancy(args.remove_redundancy);

  // -------------------------------------------------------
  // Make empty Experiment database (ensure file system works)
  // -------------------------------------------------------

  args.makeDatabaseDir();

  // ------------------------------------------------------------
  // 3. Add static structure
  //    (Add CCT::Proc and CCT::Loop)
  // ------------------------------------------------------------

  Prof::Struct::Tree* structure = new Prof::Struct::Tree("");
  if (!args.structureFiles.empty()) {
    Analysis::CallPath::readStructure(structure, args);
  }
  prof->structure(structure);

  bool printProgress = true;

  Analysis::CallPath::overlayStaticStructureMain(*prof, args.agent,
						 args.doNormalizeTy, printProgress);

  // tallent: probably delete
  Analysis::CallPath::transformCudaCFGMain(*prof);
  
  // -------------------------------------------------------
  // 4a. Create footprint metrics for canonical CCT
  // -------------------------------------------------------

  // *** TODO: key new code #2 ***

  // Post-order traversal (i.e., children before parents):
  //   compute footprint metrics from sets of trace  samples

  // Example traversal: <hpctk>/src/lib/prof/CCT-Tree.cpp:468
  //   CCT::Tree::ANode::aggregateMetricsIncl(...);


  // -------------------------------------------------------
  // 4b. Prune and normalize canonical CCT
  // -------------------------------------------------------

  // tallent: may need to update
  if (Analysis::Args::MetricFlg_isSum(args.prof_metrics)) {
    Analysis::CallPath::pruneBySummaryMetrics(*prof, NULL);
  }

  Analysis::CallPath::normalize(*prof, args.agent, args.doNormalizeTy);

  if (Analysis::Args::MetricFlg_isSum(args.prof_metrics)) {
    // Apply after all CCT pruning/normalization is completed.
    //TODO: Analysis::CallPath::applySummaryMetricAgents(*prof, args.agent);
  }

  prof->cct()->makeDensePreorderIds();


  // ------------------------------------------------------------
  // 5. Generate Experiment database
  //    INVARIANT: database dir already exists
  // ------------------------------------------------------------

  Analysis::CallPath::pruneStructTree(*prof);

  if (args.title.empty()) {
    args.title = prof->name();
  }

  if (!args.db_makeMetricDB) {
    prof->metricMgr()->zeroDBInfo();
  }

  Analysis::CallPath::makeDatabase(*prof, args);


  // -------------------------------------------------------
  // Cleanup
  // -------------------------------------------------------
  nArgs.destroy();

  delete prof;

  return 0;
}


//****************************************************************************


// Make CCT path from LBR path. 'path' represents outermost frame.
// - Outermost frame is nearest to root.
// - Innermost frame is a leaf.
// - Each leaf has a set trace samples from merging.

// *** TODO: Need a structure to map between CCT leaf nodes and the a set of trace samples. The current  Prof::Metric::IData assumes a dense vector of doubles ***

static Prof::CCT::ANode*
makeCCTPath(MyXFrame* path, uint n_metrics)
{
  MyXFrame* frame_outer = path;

  Prof::CCT::ANode* path_root = makeCCTRoot(n_metrics);
  
  Prof::CCT::ANode* parent = path_root;

  // FIXME: Iterator is conceptually iterating through frames + sample
  // even though the pseudo type has frames.
  
  for (MyXFrame* frame = frame_outer; frame != NULL ;
        /* TODO: frame = frame->next */) {

    bool isSampleOrLoad = false; // TODO
    
    Prof::CCT::ANode* node = NULL;

    // ----------------------------------------
    // Leaf: Sample/load
    // ----------------------------------------
    if (isSampleOrLoad) {
      // TODO:
      Prof::LoadMap::LMId_t lmId = Prof::LoadMap::LMId_NULL; // TODO: pseudo-LM
      VMA ip = HPCRUN_FMT_LMIp_NULL; // TODO: pseudo ip within pseudo-LM

      node = makeCCTLeaf(lmId, ip, n_metrics);
    }
    // ----------------------------------------
    // Interior: Frame
    // ----------------------------------------
    else {
      // TODO:
      Prof::LoadMap::LMId_t lmId = Prof::LoadMap::LMId_NULL; // TODO: LM for load
      VMA ip = HPCRUN_FMT_LMIp_NULL; // TODO load's ip within LM

      node = makeCCTFrame(lmId, ip, n_metrics);
    }

    if (parent) {
      // TODO: With current iterator, 'parent' could be passed to the
      // constructor. But may need to create children before 'parent'
      // and use link().
      node->link(parent);
    }

    parent = node; // change parent
  }

  return path_root;
}


static Prof::CCT::ANode*
makeCCTFrame(Prof::LoadMap::LMId_t lmId, VMA ip, uint n_metrics)
{
  // <hpctk>/src/lib/prof/CallPath-Profile.cpp:2096, cct_makeNode()
  // <hpctk>/src/lib/prof/CallPath-Profile.cpp:2120, fmt_cct_makeNode()

  const uint cpId = HPCRUN_FMT_CCTNodeId_NULL; // used for traces
  ushort opIdx = 0;

  lush_assoc_info_t as_info = lush_assoc_info_NULL;
  as_info.u.as = LUSH_ASSOC_1_to_1;

  lush_lip_t lip;
  lush_lip_init(&lip);

  Prof::Metric::IData metricData(n_metrics);

  return new Prof::CCT::Call(NULL, cpId, as_info, lmId, ip, opIdx, &lip, metricData);
}


static Prof::CCT::ANode*
makeCCTLeaf(Prof::LoadMap::LMId_t lmId, VMA ip, uint n_metrics)
{
  // <hpctk>/src/lib/prof/CallPath-Profile.cpp:2076

  const uint cpId = HPCRUN_FMT_CCTNodeId_NULL; // used for traces
  ushort opIdx = 0;
  
  lush_assoc_info_t as_info = lush_assoc_info_NULL;
  as_info.u.as = LUSH_ASSOC_1_to_1;

  lush_lip_t lip;
  lush_lip_init(&lip);

  Prof::Metric::IData metricData(n_metrics);

  // TODO: map 'node' to trace sample
  
  return new Prof::CCT::Stmt(NULL, cpId, as_info, lmId, ip, opIdx, &lip,
                             metricData);
}


static Prof::CCT::ANode*
makeCCTRoot(uint n_metrics)
{
  Prof::LoadMap::LMId_t lmId = Prof::LoadMap::LMId_NULL;
  VMA ip = HPCRUN_FMT_LMIp_NULL;

  return makeCCTFrame(lmId, ip, n_metrics);
}
