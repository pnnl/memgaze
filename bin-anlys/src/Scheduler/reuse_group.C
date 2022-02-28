/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: reuse_group.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data structure to hold information about a set of references with the same
 * access pattern that are likely to access the same cache lines.
 */

#include "reuse_group.h"

using namespace MIAMI;

ReuseGroup::ReuseGroup (unsigned int _pc, int _idx, const ScheduleTime &_st, 
             unsigned int _setIndex, bool _constantStride,
             int _valueStride)
{
   leader_pc = _pc;
   leader_opidx = _idx;
   min_offset = max_offset = 0;
   leader_st = _st;
   setIndex = _setIndex;
   constantStride = _constantStride;
   valueStride = _valueStride;
   reflist.push_back (new RefReuseInfo (_pc, _idx));
   size = 1;
}

ReuseGroup::~ReuseGroup ()
{
   RefReuseInfoList::iterator riit = reflist.begin ();
   for ( ; riit!=reflist.end() ; ++riit )
      delete (*riit);
   reflist.clear ();
   size = 0;
}
   
// add a new member; if delay<0, this will become the new leader;
// must update delay and offset for all members;
void 
ReuseGroup::addMember (unsigned int _pc, int _idx, int _delay, int _offset)
{
   reflist.push_back (new RefReuseInfo (_pc, _idx, _delay, _offset));
   if (_offset < min_offset)
      min_offset = _offset;
   if (_offset > max_offset)
      max_offset = _offset;
   size += 1;

   if (_delay < 0)
   {
      // this ref becomes the new leader; long live the king
      // adjust all members' offsets, as well as the global offsets
      leader_st += _delay;
      min_offset -= _offset;
      max_offset -= _offset;
      RefReuseInfoList::iterator riit = reflist.begin ();
      for ( ; riit!=reflist.end() ; ++riit )
      {
         RefReuseInfo *ri = *riit;
         ri->delay -= _delay;
         ri->offset -= _offset;
      }
   }
}

