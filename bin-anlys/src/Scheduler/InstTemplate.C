/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: InstTemplate.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data structure to hold an instruction execution template defined in 
 * a machine description file.
 */

#include "InstTemplate.h"

using namespace MIAMI;

InstTemplate::~InstTemplate ()
{
   // We must be careful because we can have the same pointer copied in
   // multiple consecutive elements of the list. But they are always
   // consecutive, so I can test easily for these cases.
   TEUList *lastCyc = NULL;
   MultiCycTemplate::iterator mctit = iTemplate.begin();
   for( ; mctit!=iTemplate.end() ; ++mctit )
   {
      if (*mctit!=NULL && *mctit!=lastCyc)
      {
         TEUList::iterator teuit = (*mctit)->begin();
         for ( ; teuit!=(*mctit)->end() ; ++teuit)
            // all are non NULL and unique pointers in this list
            delete (*teuit);
         delete (*mctit);
      }
      lastCyc = *mctit;
   }
   iTemplate.clear();
}

void 
InstTemplate::addCycles (TEUList* teul, int repeat)
{
   length += repeat;
   assert(repeat > 0);
   iTemplate.push_back(teul);
   for( int i=1 ; i<repeat ; ++i )
      iTemplate.push_back (teul);
}
