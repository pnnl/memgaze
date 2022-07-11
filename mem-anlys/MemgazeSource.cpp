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

//#include <include/gcc-attr.h>

//#include <tool/hpcprof/args.hpp>

//#include <lib/profile/pipeline.hpp>
//#include <lib/profile/source.hpp>
//#include <lib/profile/scope.hpp>
//#include <lib/profile/module.hpp>
//#include <lib/profile/sinks/sparsedb.hpp>
//#include <lib/profile/sinks/experimentxml4.hpp>
//#include <lib/profile/finalizers/denseids.hpp>
//#include <lib/profile/finalizers/directclassification.hpp>
//#include <lib/prof-lean/hpcrun-fmt.h>
//#include <lib/prof/CallPath-Profile.hpp>
//#include <lib/profile/stdshim/filesystem.hpp>
#include "MemgazeSource.hpp"

#include <lib/support/diagnostics.h>
#include <lib/support/RealPathMgr.hpp>
namespace fs = stdshim::filesystem;
//*************************** Forward Declarations ***************************
//using namespace hpctoolkit;
// Moved to MemgazeSource.hpp
//static int
//realmain(int argc, char* const* argv);

//****************************************************************************

// tallent: opaque (FIXME)
//class MyXFrame;

//static Prof::CCT::ANode*
//makeCCTPath(MyXFrame* path, uint n_metrics);

//static Prof::CCT::ANode*
//makeCCTFrame(Prof::LoadMap::LMId_t lmId, VMA ip, uint n_metrics);

//static Prof::CCT::ANode*
//makeCCTLeaf(Prof::LoadMap::LMId_t lmId, VMA ip, uint n_metrics);

//static Prof::CCT::ANode*
//makeCCTRoot(uint n_metrics);

template<class T, class... Args>
static std::unique_ptr<T> make_unique_x(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

MemgazeSource::MemgazeSource() 
  : ProfileSource() {
  std::cout << "in MemgazeSource constructor" << std::endl;
}

MemgazeSource::~MemgazeSource() {

}

//https://github.com/HPCToolkit/hpctoolkit/blob/develop/src/tool/hpcprof/main.cpp#L64
// tallent: typical exception wrapper
int hpctk_main(int argc, char* const* argv) {
  int ret;

  try {
    ret = hpctk_realmain(argc, argv);
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

int hpctk_realmain(int argc, char* const* argv) {
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
  // ------------------------------------------------------------


  // https://github.com/HPCToolkit/hpctoolkit/tree/develop
  // hpctoolkit/src/tool/hpcprof/
  // hpctoolkit/src/lib/profile/sources/

  // ------------------------------------------------------------
  //
  // ProfilePipeline: The monolithic thing that drives everything. Can reuse.
  //   A ProfilePipeline consists of pieline Sources, pipeline Sinks,
  //   and pipeline Finalizers that transform the Sources to Sinks.
  //   https://github.com/HPCToolkit/hpctoolkit/blob/develop/src/lib/profile/pipeline.hpp
  // 
  // ProfileSource: A virtual class we should implement.
  //   https://github.com/HPCToolkit/hpctoolkit/blob/develop/src/lib/profile/source.hpp
  // 
  // Hpcrun4: Complex example of a ProfileSource:
  //   https://github.com/HPCToolkit/hpctoolkit/blob/develop/src/lib/profile/sources/hpcrun4.cpp
  // 
  // ------------------------------------------------------------



  // ------------------------------------------------------------
  // 0. Parse args and create pipeline settings.
  // ------------------------------------------------------------
  // cf. hpctoolkit/src/tool/hpcprof/main.cpp
  // https://github.com/HPCToolkit/hpctoolkit/blob/develop/src/tool/hpcprof/main.cpp

  //ProfArgs args(argc, argv);

  // Get the main core of the Pipeline set up.
  ProfilePipeline::Settings pipeSettings;

  // 'PipelineSource' objects for each input source
  //for(auto& sp : args.sources) pipeSettings << std::move(sp.first);
  std::unique_ptr<ProfileSource> memgazesource;
  memgazesource.reset(new MemgazeSource());
  pipeSettings << std::move(memgazesource);

  // 'ProfileFinalizer' objects for hpcstruct [[we can reuse]]
  // for(auto& sp : args.structs) pipeSettings << std::move(sp.first); // add structure files
  fs::path path("/files0/cank560/UBENCH_O3_500k_8kb_P10k_THISONE_O3/ubench-500k_O3_PTW_P0ms_B8192_run_1.hpcstruct");
  std::unique_ptr<finalizers::StructFile> c;
  c.reset(new finalizers::StructFile(path));
  pipeSettings << std::move(c);

  // Provide Ids for things from the void
  finalizers::DenseIds dids;
  pipeSettings << dids;

  // Insert the proper Finalizer for drawing data directly from the Modules.
  // This is used as a fallback if the Structfiles aren't available.
  // finalizers::DirectClassification dc(args.dwarfMaxSize);
  //finalizers::DirectClassification dc(100*1024*1024);
  //pipeSettings << dc;

  // The "experiment.xml" file
  // The last parameter is for traceDB. We should use nullptr.
  // pipeSettings << make_unique_x<sinks::ExperimentXML4>(args.output, args.include_sources, nullptr);
  
  // "profile.db", "cct.db"
  //pipeSettings << make_unique_x<sinks::SparseDB>(fs::path());


  // ------------------------------------------------------------
  // 1. Reuse ProfilePipeline
  // ------------------------------------------------------------

  // Create the Pipeline, let the fun begin
  //ProfilePipeline pipeline(std::move(pipeSettings), args.threads);
  ProfilePipeline pipeline(std::move(pipeSettings), 1);
  // Drain the Pipeline, and make everything happen.
  pipeline.run();

  // -------------------------------------------------------
  // Cleanup
  // -------------------------------------------------------

  return 0;
}

//****************************************************************************
// New version of "class Hpcrun4": MemGazeSource (implements ProfileSource)
//****************************************************************************

// hpctoolkit/src/tool/hpcprof/hpcrun4.*

// https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.hpp
// https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp


void MemgazeSource::read(const DataClass& needed) { 
  // ------------------------------------------------------------
  // Load modules
  // ------------------------------------------------------------
  std::cout << "in MemgazeSource::read" << std::endl;
  //https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/source.hpp#L103 ??? 
  //https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L323 ???
  //loadmap_entry_t lm;
  std::string lm_name = "dummy name";  
  uint64_t lm_ip = 11;

  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L325 
  // for each load module:
  //{
  Module& lm = sink.module(lm_name);
  //}
  // ------------------------------------------------------------
  // CCT root (from ProfileSource())
  // ------------------------------------------------------------

  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L342

  auto& root = sink.global();

  // ------------------------------------------------------------
  // Create CCT
  // 
  //
  // A Context's 'Relation' is w.r.t. node and its parent
  //
  // A Context's scope is w.r.t. itself
  //   https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/scope.hpp
  //
  // ------------------------------------------------------------

  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L448

  // for each node in tree, where parent begins with 'root'
  //{
  // Initially all nodes have type 'Scope::point'. When structure is
  // added, they are converted to Scope::line.

  // Context: The context of a node with its parent.
  
  // https://github.com/HPCToolkit/hpctoolkit/blob/2092e539f5584528abec371e8c4bf6f4076ffe14/src/lib/profile/pipeline.hpp#L317

  // '<sink>.context(...).first' is the node's parent Context after expansion (corresponds to Relation (edge property))
  // '<sink>.context(...).second' is the node's Context after expansion

  Scope s1 = Scope(lm, lm_ip); // creates Scope::point
  Context& n1 = sink.context(root, {Relation::call, s1}).second;

  Scope scope = Scope(lm, lm_ip); // creates Scope::point
  Context& node = sink.context(n1, {Relation::call, scope}).second;
  //}
  
  // ------------------------------------------------------------
  // Create "Metric" object
  // ------------------------------------------------------------

  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L255

  // settings: name and description
  
  // hpctoolkit uses metric_desc_t m; m.name; m.description;
  Metric::Settings settings{"name", "description"};
  Metric& metric = sink.metric(settings);

  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L381
  std::vector<pms_id_t> ids;
  // https://github.com/HPCToolkit/hpctoolkit/blob/develop/src/lib/prof-lean/id-tuple.h#L116
  // struct pms_id_t: uint16_t kind, uint64_t physical index, uint64_t logical index
  pms_id_t id = {1, 1, 0};
  ids.push_back(id);
  
  ThreadAttributes tattrs;
  tattrs.idTuple(ids);
  PerThreadTemporary& thread = sink.thread(tattrs);
  
  // ------------------------------------------------------------
  // Attribute metric values
  // ------------------------------------------------------------

  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L519

  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L526

  // cid = context-id-from-file;
  // auto node_it = nodes.find(cid);

  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L534

  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L539

  // Metric "location"
  //std::optional<ProfilePipeline::Source::AccumulatorsRef> accum;
  //accum = sink.accumulateTo(thread, node /* context ref */);

  // Metric value
  //double v = 1;
  //accum->add(metric, v);

  // add post-processing?

}

// Below implementations are taken from Hpcrun4.cpp .
bool MemgazeSource::valid() const noexcept { return false; }

DataClass MemgazeSource::provides() const noexcept {
  using namespace literals::data;
  Class ret = attributes + references + contexts + DataClass::metrics + threads;
  //if(!tracepath.empty()) ret += ctxTimepoints;
  return ret;
}

DataClass MemgazeSource::finalizeRequest(const DataClass& d) const noexcept {
  std::cout << "IN FINALIZE REQUEST" << std::endl;
  using namespace literals::data;
  DataClass o = d;
  o += references; 
  //if(o.hasMetrics()) o += attributes + contexts + threads;
  //if(o.hasCtxTimepoints()) o += contexts + threads;
  //if(o.hasThreads()) o += contexts;  // In case of outlined range trees
  //if(o.hasContexts()) o += references;
  return o;
}

//void MemgazeSource::read(const DataClass& needed) {
  //if(!fileValid) return;  // We don't have anything more to say
  //if(!realread(needed)) {
  //  util::log::error{} << "Error while parsing measurement profile " << path.string();
  //  fileValid = false;
  //}
//  return;
//}
