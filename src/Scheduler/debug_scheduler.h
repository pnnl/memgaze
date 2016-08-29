/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: debug_scheduler.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Logging verbosity knobs, useful for debugging.
 */

#ifndef _DEBUG_SCHEDULER_H_
#define _DEBUG_SCHEDULER_H_

#include "debug_miami.h"

extern int cfg_counts_debug_level;
extern int sched_debug_level;
extern int graph_debug_level;
extern int paths_debug_level;
extern int machine_debug_level;
extern int dom_debug_level;
extern int memory_debug_level;
extern int output_debug_level;
extern int stream_debug_level;
extern int module_debug_level;

#define DEBUG_CFG_COUNTS  0
#define DEBUG_CFG(x, y)   if ((x)<=cfg_counts_debug_level) {y}

#define VERBOSE_DEBUG_SCHEDULER  1
#define DEBUG_SCHED(x, y)   if ((x)<=sched_debug_level) {y}

#define VERBOSE_DEBUG_DOMINATOR  0
#define DEBUG_DOM(x, y)   if ((x)<=dom_debug_level) {y}

#define VERBOSE_DEBUG_MEMORY  0
#define DEBUG_MEM(x, y)   if ((x)<=memory_debug_level) {y}

#define VERBOSE_DEBUG_OUTPUT  0
#define DEBUG_OUT(x, y)   if ((x)<=output_debug_level) {y}

#define DEBUG_STREAM_REUSE 0
#define DEBUG_STREAMS(x, y)   if ((x)<=stream_debug_level) {y}

#define DEBUG_GRAPH_CONSTRUCTION 0
#define DEBUG_GRAPH(x, y)   if ((x)<=graph_debug_level) {y}

#define DEBUG_BLOCK_PATHS_PRINT 0
#define DEBUG_PATHS(x, y)   if ((x)<=paths_debug_level) {y}

#define DEBUG_MACHINE_CONSTRUCTION 1
#define DEBUG_MACHINE(x, y)   if ((x)<=machine_debug_level) {y}

#define VERBOSE_DEBUG_LOAD_MODULE 1
#define DEBUG_LOAD_MODULE(x, y)   if ((x)<=module_debug_level) {y}

#define PROFILE_SCHEDULER 0

#endif
