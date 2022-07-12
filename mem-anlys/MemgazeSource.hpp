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
#ifndef MG_HPCTK_H
#define MG_HPCTK_H

#include <iostream>
#include <fstream>

#include <string>
using std::string;

#include <vector>

#include <lib/profile/source.hpp>
#include <tool/hpcprof/args.hpp>

#include <lib/profile/pipeline.hpp>
#include <lib/profile/source.hpp>
#include <lib/profile/scope.hpp>
#include <lib/profile/module.hpp>
#include <lib/profile/sinks/sparsedb.hpp>
#include <lib/profile/sinks/experimentxml4.hpp>
#include <lib/profile/finalizers/denseids.hpp>
#include <lib/profile/finalizers/directclassification.hpp>
#include "lib/profile/finalizers/struct.hpp"
#include <lib/prof-lean/hpcrun-fmt.h>
#include <lib/prof/CallPath-Profile.hpp>
#include <lib/profile/stdshim/filesystem.hpp>
using namespace hpctoolkit;

int hpctk_realmain(int argc, char* const* argv, std::string struct_file);
int hpctk_main(int argc, char* const* argv, std::string struct_file);

class MemgazeSource final : public ProfileSource {
public:
  MemgazeSource();
  ~MemgazeSource();

  // You shouldn't create these directly, use ProfileSource::create_for
  // Hpcrun4() = delete;

  bool valid() const noexcept override;

  // Read in enough data to satisfy a request or until a timeout is reached.
  // See `ProfileSource::read(...)`.
  void read(const DataClass&) override;

  DataClass provides() const noexcept override;
  DataClass finalizeRequest(const DataClass&) const noexcept override;
  
//private:
};
#endif
