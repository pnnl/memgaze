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
#include <cmath>
#include <string>
using std::string;

#include <vector>

//*************************** User Include Files ****************************

#include "Instruction.hpp"
#include "Address.hpp"
#include "MemgazeSource.hpp"
#include "metrics.hpp"

#include "lib/profile/attributes.hpp"
#include "lib/prof-lean/id-tuple.h"
#include "lib/prof-lean/hpcrun-fmt.h"


#include <lib/support/diagnostics.h>
#include <lib/support/RealPathMgr.hpp>
namespace fs = stdshim::filesystem;
using namespace std;

//****************************************************************************

// We had to add this function from HPCToolkit because our summary
// profile was empty. 'Sum' indicates aggregation across threads and we 
// use only one thread. That was just an hack to not get a db file with 
// missing information.
void MemgazeFinalizers::StatisticsExtender::appendStatistics(const Metric& m, Metric::StatsAccess mas) noexcept {
  if(m.visibility() == Metric::Settings::visibility_t::invisible) return;
  Metric::Statistics s;
  struct MemgazeFinalizers::Stats stats;  

  s.sum = stats.sum;
  //s.mean = stats.mean;
  //s.min = stats.min;
  //s.max = stats.max;
  //s.stddev = stats.stddev;
  //s.cfvar = stats.cfvar;
  mas.requestStatistics(std::move(s));
}

// Taken from HPCToolkit.
template<class T, class... Args>
static std::unique_ptr<T> make_unique_x(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// TODO: Remove. Added for testing.
void MemgazeSource::numWindows(Window* node) {
  if (node == NULL) return;
  num_windows++;
  numWindows(node->left);
  numWindows(node->right);
}

// TODO: Remove all variables start with 'num_'. Added for testing.
MemgazeSource::MemgazeSource(Window* fullT) 
  : ProfileSource() {
  memgaze_root = fullT;
  num_windows = 0;
  num_contexts = 0;
  num_functions = 0;
  num_leaves = 0;
  num_samples = 0;
  num_of_sample_children = 0;

  numWindows(fullT);
}

// TODO: Remove print statements. Added for testing.
MemgazeSource::~MemgazeSource() {
  cout << "Clean MemgazeSource" << endl;
  cout << "Num contexts: " << num_contexts << endl;
  cout << "Num windows: " << num_windows << endl;
  cout << "Num funcs: " << num_functions << endl;
  cout << "Num leaves: " << num_leaves << endl;
  cout << "Num samples: " << num_samples << endl;
  cout << "Total windows after sample: " << num_of_sample_children << endl;

  for (auto function : functions) delete function;
  functions.clear(); 
}

//https://github.com/HPCToolkit/hpctoolkit/blob/develop/src/tool/hpcprof/main.cpp#L64
// tallent: typical exception wrapper
// TODO: function can be renamed.
int hpctk_main(int argc, char* const* argv, std::string struct_file, Window* fullT) {
  int ret;

  try {
    ret = hpctk_realmain(argc, argv, struct_file, fullT);
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

// TODO: function can be renamed.
int hpctk_realmain(int argc, char* const* argv, std::string struct_file, Window* fullT) {
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
  // 0. Parse args and create pipeline settings.
  // ------------------------------------------------------------
  // cf. hpctoolkit/src/tool/hpcprof/main.cpp
  // https://github.com/HPCToolkit/hpctoolkit/blob/develop/src/tool/hpcprof/main.cpp

  // We should do this before driving any Prof code. 
  // https://github.com/HPCToolkit/hpctoolkit/blob/develop/src/tool/hpcprof/args.cpp#L183
  util::log::Settings logSettings(false, false, false);
  util::log::Settings::set(std::move(logSettings));

  // create fs::path from 'struct_file`
  fs::path struct_path(struct_file);

  // Get the main core of the Pipeline set up.
  ProfilePipeline::Settings pipeSettings;

  // 'PipelineSource' objects for each input source
  //for(auto& sp : args.sources) pipeSettings << std::move(sp.first);
  // TODO: Need to update this if we have multiple sources.
  std::unique_ptr<ProfileSource> memgazesource; 
  memgazesource.reset(new MemgazeSource(fullT));
  pipeSettings << std::move(memgazesource);

  // 'ProfileFinalizer' objects for hpcstruct [[we can reuse]]
  // for(auto& sp : args.structs) pipeSettings << std::move(sp.first); // add structure files
  // TODO: Need to update this if we have multiple struct files.
  std::unique_ptr<finalizers::StructFile> c;
  c.reset(new finalizers::StructFile(struct_path));
  pipeSettings << std::move(c);
  
  // Provide Ids for things from the void
  finalizers::DenseIds dids;
  pipeSettings << dids;

  // Insert the proper Finalizer for drawing data directly from the Modules.
  // This is used as a fallback if the Structfiles aren't available.
  // finalizers::DirectClassification dc(args.dwarfMaxSize);
  finalizers::DirectClassification dc(100*1024*1024);
  // TODO: Gives error in the following line. Ask Jonathon.
  //pipeSettings << dc;

  // Temporary fix to get rid of unresolved path errors.
  // Permanent fix should be done in HPCToolkit.
  // std::unique_ptr<ProfileFinalizer> temp_finalizer;
  // temp_finalizer.reset(new UnresolvedPaths());
  MemgazeFinalizers::UnresolvedPaths temp_finalizer;
  pipeSettings << temp_finalizer;

  MemgazeFinalizers::StatisticsExtender stat_extender;
  pipeSettings << stat_extender;

  // The "experiment.xml" file
  // The last parameter is for traceDB. We should use nullptr.
  // pipeSettings << make_unique_x<sinks::ExperimentXML4>(args.output, args.include_sources, nullptr);
  // TODO: take as argument
  fs::path working_dir = fs::current_path();
  fs::path dummy_db("memgaze-database-test-2");
  pipeSettings << make_unique_x<sinks::ExperimentXML4>(working_dir / dummy_db, false, nullptr);
  // "profile.db", "cct.db"
  pipeSettings << make_unique_x<sinks::SparseDB>(working_dir / dummy_db);

  // ------------------------------------------------------------
  // 1. Reuse ProfilePipeline
  // ------------------------------------------------------------

  // Create the Pipeline
  // ProfilePipeline pipeline(std::move(pipeSettings), args.threads);
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

// Walks down on the tree after reaching the sample and gets the leaf windows 
// with the IP of their address that has the highest footprint. 
void MemgazeSource::summarizeSample(Window* node, vector<Window*> &leaves) {
  if (node == NULL) {
    return;
  }
  num_of_sample_children++;

  // TODO: improve the way we select the leaf window
  if (node->left == NULL && node->right == NULL) {
    leaves.push_back(node);
  }

  summarizeSample(node->left, leaves);
  summarizeSample(node->right, leaves);
}

// Creates the context recursively using preorder traversal. 
void MemgazeSource::createCCT(Window* node, Module& lm, Context& parent_context) {
  if (node == NULL) return; 

  // do not continue after the sample window.
  if (node->parent)
    if (node->parent->sampleHead) return; 

  // TODO: for testing.
  //cout << "parent: " << node->windowID.second << endl;
  //if (node->left) {
  //  cout << "left child: " << node->left->windowID.second << endl;
  //  node->left->setFuncName();
  //}
  //if (node->right) {
  //  cout << "right child: " << node->right->windowID.second << endl;
  //  node->right->setFuncName();
  //}
  
  // TODO: Use this instead of 'lm' variable.
  //cout << node->maxFP_addr->ip->dso_name << endl;

  // calculates start, mid, and end times for each window.
  // example: stime = node->addresses[node->addresses[0]->time->time;
  node->calcTime();
  // name format for function scopes = mid_time:start_time-end_time.
  double stime = (node->stime - start_time) / pow(10, 6);
  double mtime = (node->mtime - start_time) / pow(10, 6);
  double etime = (node->etime - start_time) / pow(10, 6);
  string name = to_string(mtime) + ":" + to_string(stime) + "-" + to_string(etime);

  // setFuncName() sets the name and the maxFP_addr which is 
  // the address that has the highest FP. HPCToolkit requires an IP.
  // TODO: might be changed in future.
  node->setFuncName();
  uint64_t ip = node->maxFP_addr->ip->ip;

  // Create function scopes for the time intervals. We have to use heap for 
  // Function scopes and clean them in destructor.
  Scope new_scope;
  Function* function = new Function(lm, ip, name); 
  functions.push_back(function);
  new_scope = Scope(*function); 
  num_functions++; // TODO: remove. for testing

  // create new context and set parent-child relationship.
  Context& new_context = sink.context(parent_context, {Relation::call, new_scope}).second;
  num_contexts++; // TODO: remove. for testing
  //window_context.insert({node, new_context}); TODO: remove

  // get metrics from each window.
  map <int, double> diagMap;
  map<int, double> metrics_values = node->getMetrics(diagMap); 
  context_metrics.push_back({new_context, metrics_values});

  if (node->sampleHead) {
    //cout << "sample" << endl; // TODO: Remove. For testing.
    num_samples++; // TODO: remove. for testing
    vector<Window*> leaves;
    summarizeSample(node, leaves);

    // iterate over the leaves and find the function that has the most accesses. 
    // Can be changed in future.
    map <string, int> func_occ;
    map<int, double> agg_metrics;
    for (size_t idx = 0; idx != leaves.size(); idx++) {  
      int window_size = leaves[idx]->getSize();

      // get the frequency of accesses for each function.
      for(auto& addr: leaves[idx]->addresses) {
        string addr_funcName = addr->getFuncName();
        func_occ[addr_funcName]++;
      }

      // get metrics from each window.
      map <int, double> diagMap;
      map<int, double> metrics_values = leaves[idx]->getMetrics(diagMap);      
      // aggregate metrics.       
      agg_metrics = accumulate( metrics_values.begin(), metrics_values.end(), agg_metrics,
        []( std::map<int, double> &m, const std::pair<const int, double> &p )
        {   
            return (m[p.first] += p.second, m);
        });
    }

    // find the function that has the most accesses.
    auto max_freq_func = std::max_element
    (
      std::begin(func_occ), std::end(func_occ),
      [] (const pair<string, int>& p1, const  pair<string, int>& p2) {
        return p1.second < p2.second;
      }
    );
    
    // create the context for the function that has the most accesses.
    // TODO: THIS GIVES ERROR ON BIGNUKE!! SHOULD BE FIXED WHEN AGGREGATION 
    // IS PROPERLY FIXED!!
    //Function* leaf_function = new Function(lm, ip, max_freq_func->first);
    //functions.push_back(leaf_function);
    //Scope leaf_scope = Scope(*leaf_function);
    //Context& leaf_context = sink.context(new_context, {Relation::call, leaf_scope}).second;

    // TODO: HPCViewer requires point scopes. We use the ip of the first address of the first 
    // leaf for now. This is a hack because lines 423-426 didn't work. 
    // Should be fixed after properly aggregating the functions:
    Scope leaf_scope = Scope(lm, leaves[0]->addresses[0]->ip->ip);
    Context& leaf_context = sink.context(new_context, {Relation::call, leaf_scope}).second;
    num_contexts++; // TODO: remove. for testing
    num_leaves++; 

    // use the aggregate metrics for the context. aggregate of all leaves in a sample.
    // TODO: will be changed in future.
    //context_metrics.push_back({leaf_context, agg_metrics});
    context_metrics.push_back({leaf_context, agg_metrics});
  }
  
  createCCT(node->left, lm, new_context);
  createCCT(node->right, lm, new_context);
}

// Add metrics for hpcviewer. Takes name, description and id for the metric.
void MemgazeSource::addMetrics(string name, string description, int id) {
  // hpctoolkit uses metric_desc_t m; m.name; m.description;
  Metric::Settings settings{name, description};
  //settings.scopes[MetricScope::execution] = false;
  Metric& metric = sink.metric(settings);
  metrics.insert({id, metric});
  sink.metricFreeze(metric);
}

// hpctoolkit/src/tool/hpcprof/hpcrun4.*
// https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.hpp
// https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp
void MemgazeSource::read(const DataClass& needed) { 
  // ------------------------------------------------------------
  // Load modules
  // ------------------------------------------------------------
  //https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/source.hpp#L103 ??? 
  //https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L323 ???
  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L325 
  // for each load module:
  //{
  if (needed.hasReferences()) {
    cout << "has REFERENCES" << endl;
    loadmap_entry_t lm_test;
    // For old traces, go to the struct file, get the name of the LM and paste it here. Examples:
    //string hardcoded_lm = "/home/cank560/memgaze/install/bin/memgaze-miniVite-v1/miniVite-v1-memgaze";
    //string hardcoded_lm = "/home/kili337/Projects/IPPD/gitlab/palm/intelPT_FP/Experiments/IPDPS/UBENCH_O3_500k_8kb_P10k/ubench-500k_O3_PTW";
    //string hardcoded_lm = "/home/kili337/Projects/IPPD/gitlab/palm/intelPT_FP/Experiments/IPDPS/MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1_PTW";
    //lm_test.name = &hardcoded_lm[0];

    // TODO: We should get LM seperately for each IP we use for context
    // creation if we have multiple LMs. For now, added just one LM case 
    // since we don't have data for multiple LMs.
    lm_test.name = &(memgaze_root->addresses[0]->ip->dso_name[0]);

    // create HPCToolkit module using the load module from Memgaze.
    Module& lm = sink.module(lm_test.name);
    // TODO: keeping a list of modules in case we need it. Can be removed in future 
    // if we use line 380 for each window. 
    modules.push_back(lm);
  }
  //}

  // ------------------------------------------------------------
  // CCT root (from ProfileSource())
  // ------------------------------------------------------------
  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L342
  if (needed.hasContexts()) {
    cout << "has CONTEXTS" << endl;
    auto& root_context = sink.global();

    memgaze_root->setFuncName();
    start_time = memgaze_root->maxFP_addr->time->time;
    cout << "first start_time: " << start_time << endl;

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

    //Scope s1 = Scope(lm, lm_ip); // creates Scope::point
    //Context& n1 = sink.context(root, {Relation::call, s1}).second;

    //Scope scope = Scope(lm, lm_ip); // creates Scope::point
    //Context& node = sink.context(n1, {Relation::call, scope}).second;
    // TODO: change this when we are able to test new trace format. see line 380.
    cout << "createCCT" << endl;
    Module& lm = modules[0];
    createCCT(memgaze_root, lm, root_context);
    //}

    std::vector<pms_id_t> ids;
    // https://github.com/HPCToolkit/hpctoolkit/blob/develop/src/lib/prof-lean/id-tuple.h#L116
    // struct pms_id_t: uint16_t kind, uint64_t physical index, uint64_t logical index
    pms_id_t id = {IDTUPLE_COMPOSE(IDTUPLE_RANK, IDTUPLE_IDS_BOTH_VALID), 0, 0};
    ids.push_back(id);
    tattrs.idTuple(ids);
  }

  // ------------------------------------------------------------
  // Create "Metric" object
  // ------------------------------------------------------------

  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L255

  // settings: name and description
  if (needed.hasAttributes()) {
    cout << "has ATTRIBUTES" << endl;
  
    // Create idTuple types. Took from hpctoolkit.
    attrs.idtupleName(IDTUPLE_SUMMARY, HPCRUN_IDTUPLE_SUMMARY);
    attrs.idtupleName(IDTUPLE_NODE, HPCRUN_IDTUPLE_NODE);
    attrs.idtupleName(IDTUPLE_RANK, HPCRUN_IDTUPLE_RANK);
    attrs.idtupleName(IDTUPLE_THREAD, HPCRUN_IDTUPLE_THREAD);
    attrs.idtupleName(IDTUPLE_GPUDEVICE, HPCRUN_IDTUPLE_GPUDEVICE);
    attrs.idtupleName(IDTUPLE_GPUCONTEXT, HPCRUN_IDTUPLE_GPUCONTEXT);
    attrs.idtupleName(IDTUPLE_GPUSTREAM, HPCRUN_IDTUPLE_GPUSTREAM);
    attrs.idtupleName(IDTUPLE_CORE, HPCRUN_IDTUPLE_CORE);
    sink.attributes(std::move(attrs));

    addMetrics("STRIDED", "strided description", STRIDED);
    addMetrics("CONSTANT", "constant description", CONSTANT);
    addMetrics("INDIRECT", "indirect description", INDIRECT);
    addMetrics("FP", "FP description", FP);
    //addMetrics("UNKNOWN", "unknown description", UNKNOWN);
    addMetrics("WINDOW_SIZE", "window_size desc", WINDOW_SIZE);
    addMetrics("TOTAL_LOADS", "total_loads desc", TOTAL_LOADS);
    addMetrics("CONSTANT2LOAD_RATIO", "contload_ratio desc", CONSTANT2LOAD_RATIO);
    addMetrics("NPF_RATE", "npf_rate desc", NPF_RATE);
    addMetrics("NPF_GROWTH_RATE", "npf_growth_rate desc", NPF_GROWTH_RATE);
    addMetrics("GROWTH_RATE", "growth_rate desc", GROWTH_RATE);
  }

  // https://github.com/HPCToolkit/hpctoolkit/blob/1aa82a66e535b5c1f28a1a46bebeac1a78616be0/src/lib/profile/sources/hpcrun4.cpp#L381
  if (needed.hasThreads()) {
    cout << "has THREADS" << endl;
    thread = &sink.thread(std::move(tattrs));
  }

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
  if (needed.hasMetrics()) {
    cout << "hasMetrics" << endl;
    std::optional<ProfilePipeline::Source::AccumulatorsRef> accum;

    // Add metrics one by one. 
    // HPCToolkit doesn't allow for adding them as we create metrics.
    for (auto ctx : context_metrics) {
      accum = sink.accumulateTo(*thread, ctx.context);
      for (auto metric = ctx.metrics.begin(); metric != ctx.metrics.end(); metric++) {
        if (metric->second != 0 && !isnan(metric->second) && !isinf(metric->second)) {
          accum->add(metrics.find(metric->first)->second, metric->second);
        }
      }
    }
  }

  // add post-processing?
}

// Below implementations are taken from Hpcrun4.cpp.
bool MemgazeSource::valid() const noexcept { return false; }

DataClass MemgazeSource::provides() const noexcept {
  using namespace hpctoolkit::literals::data;
  Class ret = attributes + references + contexts + DataClass::metrics + threads;
  //if(!tracepath.empty()) ret += ctxTimepoints;
  return ret;
}

DataClass MemgazeSource::finalizeRequest(const DataClass& d) const noexcept {
  using namespace hpctoolkit::literals::data;
  DataClass o = d;
  if(o.hasMetrics()) o += attributes + contexts + threads;
  if(o.hasCtxTimepoints()) o += contexts + threads;
  if(o.hasThreads()) o += contexts;  // In case of outlined range trees
  if(o.hasContexts()) o += references;
  return o;
}

