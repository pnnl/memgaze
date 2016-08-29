/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: reg_sched_info.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data structure to hold scheduling information for input and output
 * registers relative to a program scope.
 */

#ifndef _REG_SCHED_INFO_H
#define _REG_SCHED_INFO_H

#include <list>
#include "miami_types.h"
#include "register_class.h"

namespace MIAMI
{

typedef struct _reg_sched_info
{
//   reg_type reg;
   register_info reg;
   uint32_t cycles;     /* clock cycles from start of schedule for src regs
                           and cycles to end of schedule for dest regs */
   bool dest;           /* it is destination register */
} reg_sched_info;

typedef std::list <reg_sched_info> RSIList;

typedef std::list <RSIList> RSIListList;

} /* namespace MIAMI */

#endif
