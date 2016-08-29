/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: path_id.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines a path ID class.
 */

#ifndef _MIAMI_PATH_ID_H
#define _MIAMI_PATH_ID_H

#include <stdio.h>
#include <iostream>

#include "miami_types.h"

namespace MIAMI
{

class PathID
{
public:
   PathID(addrtype _start = 0, uint32_t _extra = 0) :
       start(_start)
   {
      pathIdx = _extra & 0xffff;
      scopeIdx = (_extra >> 16) & 0xffff;
   }
   PathID(addrtype _start, int _lIdx, int _pIdx) :
       start(_start), scopeIdx(_lIdx), pathIdx(_pIdx)
   {}
   
   inline addrtype Start() const  { return (start); }
   inline int ScopeIndex() const  { return (scopeIdx); }
   inline int PathIndex()  const  { return (pathIdx); }
   // generate name for printing
   const char* Name() {
      snprintf(buf, 32, "0x%" PRIxaddr "/%u/%u", start, scopeIdx, pathIdx);
      return (buf);
   }
   // generate name to be used as part of a file name
   const char* Filename() {
      snprintf(buf, 32, "0x%" PRIxaddr "-%u-%u", start, scopeIdx, pathIdx);
      return (buf);
   }
   
   bool operator== (const PathID &pi) const
   {
      return (start==pi.start && scopeIdx==pi.scopeIdx && pathIdx==pi.pathIdx);
   }
   bool operator!= (const PathID &pi) const
   {
      return (start!=pi.start || scopeIdx!=pi.scopeIdx || pathIdx!=pi.pathIdx);
   }
   operator bool () const
   {
      return (start || scopeIdx || pathIdx);
   }
   bool operator! () const
   {
      return (start==0 && scopeIdx==0 && pathIdx==0);
   }
   
   addrtype start;
   uint32_t scopeIdx : 16;  // encode scope idx on 16 bits and path number on 16 bits
   uint32_t pathIdx : 16;
private:
  char buf[32];
};

extern std::ostream& operator<< (std::ostream& os, PathID &pId);

}  /* namespace MIAMI */

#endif
