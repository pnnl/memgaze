/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: miami_containers.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A few commonly used containers.
 */

#ifndef MIAMI_CONTAINERS_H
#define MIAMI_CONTAINERS_H

// Define various STL based or otherwise containers commonly used
// by the MIAMI toolkit. Types that are used only locally, should be
// defined at the place where they are used.

#include <set>
#include "miami_types.h"
#include "generic_pair.h"

namespace MIAMI
{

typedef std::set<addrtype> AddrSet;

// An AddrIntPair can hold both a memory address (instruction PC), and 
// an integer, e.g. memory operand index.
typedef MIAMIU::GenericPair<addrtype, int32_t> AddrIntPair;
typedef std::set<AddrIntPair, AddrIntPair::OrderPairs> AddrIntSet;

}  /* namespace MIAMI */

#endif
