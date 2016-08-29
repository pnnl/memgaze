/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: debug_miami.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Logging verbosity knobs useful for debugging.
 */

#ifndef _DEBUG_MIAMI_H_
#define _DEBUG_MIAMI_H_

extern int static_mem_debug_level;
extern int indirect_debug_level;
extern int inst_decode_debug_level;
extern int cfg_debug_level;

#define DEBUG_STATIC_MEMORY  0
#define DEBUG_STMEM(x, y)   if ((x)<=static_mem_debug_level) {y}

#define DEBUG_INDIRECT_TRANSFER  0
#define DEBUG_INDIRECT(x, y)   if ((x)<=indirect_debug_level) {y}

#define DEBUG_INST_DECODING  0
#define DEBUG_IDECODE(x, y)   if ((x)<=inst_decode_debug_level) {y}

#define DEBUG_CFG_DISCOVERY  0
#define LOG_CFG(x, y)   if ((x)<=cfg_debug_level) {y}

#endif
