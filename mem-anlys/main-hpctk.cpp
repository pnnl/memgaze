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

#include "Args.hpp"

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

static void
makeMetrics(Prof::CallPath::Profile& prof,
	    const Analysis::Args& args,
	    const Analysis::Util::NormalizeProfileArgs_t& nArgs);


//****************************************************************************

// tallent: opaque (FIXME)
class MyXFrame;

static Prof::CCT::ANode*
makeCCTPath(MyXFrame* leafFrame);



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
  // Two versions
  //
  // 1. Execution interval tree, where "time intervals" in execution
  //    interval tree form call paths. That is, a parent "time
  //    interval" is a caller and child "time interval" is a
  //    callee. For source information, use a pseudo load module that
  //    gives full regions. For callsite IP, a time interval (interior
  //    nodes) uses the midpoint time (which will be unique). Leaf
  //    nodes (loads) use load IP as address.
  //
  // 2. Call path profile, where LBR are call paths.
  //
  // To represent footprint metrics, insert final/summary metrics at
  // each node (interior + leaf). (Metrics cannot simply be at the
  // leaf.) Use computed metrics for the other views.
  //
  // Cf. `makeReturnCountMetric`, `Prof::Metric::ADesc::ComputedTy_Final`
  //
  // ------------------------------------------------------------

  // ------------------------------------------------------------
  // *. Create synthetic root in CCT
  // ------------------------------------------------------------
  // cf. <hpctk>/src/tool/hpcprof/main.cpp
  
  Args args;
  args.parse(argc, argv);

  // tallent: probably useful
  RealPathMgr::singleton().searchPaths(args.searchPathStr());

  // tallent: may not be useful
  Analysis::Util::NormalizeProfileArgs_t nArgs =
    Analysis::Util::normalizeProfileArgs(args.profileFiles);


  // ------------------------------------------------------------
  // 1. Create synthetic root in CCT
  // ------------------------------------------------------------

  // <hpctk>/src/lib/analysis/CallPath.cpp:141 read()
  // <hpctk>/src/lib/prof/CallPath-Profile.cpp:1297, fmt_epoch_fread()
  
  uint rFlags = 0;
  Prof::CallPath::Profile* prof = Prof::CallPath::Profile::make(rFlags);

  Prof::CCT::Tree* cct = prof->cct();

  // <hpctk>/src/lib/prof/CallPath-Profile.cpp:2092, cct_makeNode()
  // <hpctk>/src/lib/prof/CallPath-Profile.cpp:2120, fmt_cct_makeNode()

  const uint cpId = HPCRUN_FMT_CCTNodeId_NULL;
  ushort opIdx = 0;

  lush_assoc_info_t as_info = lush_assoc_info_NULL;
  as_info.u.as = LUSH_ASSOC_1_to_1;

  Prof::LoadMap::LMId_t lmId = Prof::LoadMap::LMId_NULL;
  VMA ip = HPCRUN_FMT_LMIp_NULL;

  lush_lip_t lip;
  lush_lip_init(&lip);

  uint mSz = 0; // (true) ? 0 : numMetricsDst;
  Prof::Metric::IData metrics(mSz);

  Prof::CCT::ANode* cct_root =
    new Prof::CCT::Call(NULL, cpId, as_info, lmId, ip, opIdx, &lip, metrics);

  prof->cct()->root(cct_root);


  // ------------------------------------------------------------
  // *. Create CCT from each call path fragment
  //    (CCT is CCT::Call and CCT::Stmt)
  // ------------------------------------------------------------

  // *** tallent: key new code #1 ***

  // <hpctk>/src/lib/prof/CallPath-Profile.cpp:192, Profile::merge()
  // <hpctk>/src/lib/prof/CCT-Tree.cpp:166, Tree::merge()
  // <hpctk>/src/lib/prof/CCT-Tree.cpp:816, ANode::mergeDeep()
  
  
  for (/* each LBR path */ int i = 0 ; i < 1 ; ++i) {
    Prof::CCT::ANode* path_root = makeCCTPath(/*LBR path*/ NULL);

    uint x_newMetricBegIdx = 0;
    Prof::CCT::MergeContext mergeCtxt(prof->cct(), /*doTrackCPIds*/false);
      
    Prof::CCT::MergeEffectList* mrgEffects =
      cct_root->mergeDeep(path_root, x_newMetricBegIdx, mergeCtxt);

    delete path_root;
    delete mrgEffects;
  }
  
  // tallent: unsure
  prof->disable_redundancy(args.remove_redundancy);

  // -------------------------------------------------------
  // Make empty Experiment database (ensure file system works)
  // -------------------------------------------------------

  args.makeDatabaseDir();

  // ------------------------------------------------------------
  // 2. Add static structure
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
  // 3a. Create footprint metrics for canonical CCT
  // -------------------------------------------------------

  // *** tallent: key new code #2 ***
  
  // Post-order traversal (i.e., children before parents):
  //   compute footprint metrics from sets of trace  samples

  // Example traversal: <hpctk>/src/lib/prof/CCT-Tree.cpp:468
  //   CCT::Tree::ANode::aggregateMetricsIncl(...);


  // -------------------------------------------------------
  // 3b. Prune and normalize canonical CCT
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
  // 4. Generate Experiment database
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


// Make CCT path from LBR path:
// - Outermost frame is near root,
// - Innermost frame is a leaf.
// - Each leaf has a set trace samples from merging.

// *** Need a structure to map between CCT leaf nodes and the a set of trace samples. The current  Prof::Metric::IData assumes a dense vector of doubles ***

static Prof::CCT::ANode*
makeCCTPath(MyXFrame* root)
{
  Prof::CCT::ANode* parent = create synthetic root from above;

  for (each frame) {

    Prof::CCT::ANode* node = NULL;
    
    if (leaf) {
      // <hpctk>/src/lib/prof/CallPath-Profile.cpp:2076
      node = new CCT::Stmt(NULL, cpId, nodeFmt.as_info, lmId, lmIP, opIdx, lip,
                        metricData);
      map node to trace sample;
    }
    else {
      // <hpctk>/src/lib/prof/CallPath-Profile.cpp:2096
      node = new CCT::Call(NULL, cpId0, nodeFmt.as_info, lmId, lmIP, opIdx,
                        lipCopy, metricData0);
    }

    if (parent) {
      node->link(parent);
    }

    parent = node; // change parent
  }

  return root_of_path
}
