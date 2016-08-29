/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: memory_latency_histograms.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements the logic for parsing memory latency simulation results, and 
 * the mechanistic modeling approach to combine them with the instruction 
 * execution rate predictions from MIAMI, to compute total execution times.
 */

#ifndef _MEMORY_LATENCY_HISTOGRAMS_H
#define _MEMORY_LATENCY_HISTOGRAMS_H

#include <vector>
#include "miami_types.h"
#include "fast_hashmap.h"

namespace MIAMI
{

// I need a data structure to keep the memory latency histograms.
// I can have fixed length arrays for storing latency/count histograms
// because I know the number of bins apriori
typedef float mem_latency_t;
typedef uint32_t access_count_t;

// define also the printf format strings to be used the for the types defined above
#define PRImlat "g"
#define PRIacnt PRIu32

typedef std::pair<mem_latency_t, access_count_t> latency_count_bin;
typedef std::vector<latency_count_bin> MemoryLatencyHistogram;

extern MemoryLatencyHistogram* defaultMLHP;
typedef MIAMIU::HashMap<addrtype, MemoryLatencyHistogram*, &defaultMLHP> RefLatencyHistMap;

}  /* namespace MIAMI */

#endif
