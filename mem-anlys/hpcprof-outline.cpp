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

// Grabbed source and build from spack, 2021.10.15

// <hpctoolkit-src> = /files0/tallent/build/spack/opt/spack/linux-centos7-skylake_avx512/gcc-7.1.0/hpctoolkit-2021.10.15-2lh5l4ipm6jtuqt2fvsmf25g3o7o6ugk/share/hpctoolkit/src/


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
  Args args;
  args.parse(argc, argv);

  // tallent: probably useful
  RealPathMgr::singleton().searchPaths(args.searchPathStr());

  // tallent: may not be useful
  Analysis::Util::NormalizeProfileArgs_t nArgs =
    Analysis::Util::normalizeProfileArgs(args.profileFiles);


  // ------------------------------------------------------------
  // 1. Create CCT from each call path fragment
  //    (CCT is CCT::Call and CCT::Stmt)
  // ------------------------------------------------------------

  // *** tallent: key new code #1 ***

  uint rFlags = 0;
  Prof::CallPath::Profile* prof = Prof::CallPath::Profile::make(rFlags);
  // cf. <hpctoolkit-src>/src/lib/prof/CallPath-Profile.cpp:1297

  Prof::CCT::ANode* cct = prof.cct();

  // *** create synthetic root in cct ***
  Prof::CCT::ANode* cct_root =
    new CCT::Call(NULL, cpId0, nodeFmt.as_info, LoadMap::LMId_NULL, HPCRUN_FMT_LMIp_NULL, opIdx, lipCopy, metricData0);

  prof.cct()->root(cct_root);

  for (each LBR path) {
    Prof::CCT::ANode* path = makeCCTPath(...); // see below
    MergeEffectList* mrgEffects = cct->mergeDeep(path,..);

    delete path;
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

  // Example traversal: <hpctoolkit-src>/src/lib/prof/CCT-Tree.cpp:468
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
makeCCTPath(...)
{
  Prof::CCT::ANode* parent = create synthetic root from above;

  for (each frame) {

    Prof::CCT::ANode* node = NULL;
    
    if (leaf) {
      // <hpctoolkit-src>/src/lib/prof/CallPath-Profile.cpp:2076
      node = new CCT::Stmt(NULL, cpId, nodeFmt.as_info, lmId, lmIP, opIdx, lip,
                        metricData);
      map node to trace sample;
    }
    else {
      // <hpctoolkit-src>/src/lib/prof/CallPath-Profile.cpp:2096
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
