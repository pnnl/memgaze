/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: reuse_group.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data structure to hold information about a set of references with the same
 * access pattern that are likely to access the same cache lines.
 */

#ifndef _REUSE_GROUP_H
#define _REUSE_GROUP_H

#include <list>
#include "schedule_time.h"

namespace MIAMI
{

// A reuse group represents a group of references with similar access
// patterns that are part of the same path and are predicted to have
// temporal or spatial reuse on each other. A group is described by its
// leader, which represents the reference that is likely to first access 
// a cache line, and the list of all references in the group. For each
// reference we store information about the predicted number of clock 
// cycles that pass since the line is accessed by the leader, as well as
// a spatial offset between the location touched by the leader and the 
// location accessed by each member.

class RefReuseInfo
{
public:
   RefReuseInfo (unsigned int _pc, int _idx = 0, int _delay = 0, int _offset = 0) :
        pc(_pc), opidx(_idx), delay(_delay), offset(_offset)
   {}

   unsigned int pc;
   int opidx;   // store the index of the memory operand corresponding to this micro-op
   int delay;
   int offset;
};

typedef std::list<RefReuseInfo*> RefReuseInfoList;

class ReuseGroup
{
public:
   ReuseGroup (unsigned int _pc, int _idx, const ScheduleTime &_st, 
           unsigned int _setIndex, bool _constantStride = false,
           int _valueStride = 0);
   ~ReuseGroup ();
   
   int Size () { return (size); }
   unsigned int leaderAddress() const { return (leader_pc); }
   int leaderMemOpIndex() const { return (leader_opidx); }
   ScheduleTime& leaderSchedTime()    { return (leader_st); }
   unsigned int getSetIndex() const   { return (setIndex); }
   bool hasConstantStride() const     { return (constantStride); }
   int getStrideValue() const         { return (valueStride); }
   
   // add a new member; if delay<0, this will become the new leader;
   // must update delay and offset for all members;
   void addMember (unsigned int _pc, int _idx, int _delay, int _offset);
   
private:
   unsigned int leader_pc;
   int leader_opidx;
   unsigned int setIndex;
   int min_offset;
   int max_offset;
   int size;
   bool constantStride;
   int valueStride;
   ScheduleTime leader_st;
   
   RefReuseInfoList reflist;
};

typedef std::list<ReuseGroup*> RGList;

} /* namespace MIAMI */

#endif
