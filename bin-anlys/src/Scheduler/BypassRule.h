/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: BypassRule.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines a data structure to hold MDL (machine description language)
 * bypass rules.
 */

#ifndef _BYPASS_RULE
#define _BYPASS_RULE

#include <stdio.h>
#include <list>
#include "BitSet.h"
#include "position.h"
#include "instruction_class.h"

namespace MIAMI
{

class BypassRule
{
public:
   BypassRule (ICSet* _source, BitSet* _edge, ICSet* _sink, 
         unsigned int _latency, Position& _pos);
   ~BypassRule ();
   bool matchesRule(const InstructionClass& _source, int _edge, const InstructionClass& _sink);
   unsigned int bypassLatency() { return (latency); }
   void print (FILE* fd);

private:
   ICSet* sourceType;
   BitSet* edgeType;
   ICSet* sinkType;
   unsigned int latency;
   Position pos;
};

typedef std::list<BypassRule*> BypassList;

} /* namespace MIAMI */

#endif
