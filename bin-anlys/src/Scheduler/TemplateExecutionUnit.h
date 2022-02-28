/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: TemplateExecutionUnit.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines data structure to hold information about the use pattern of an
 * execution unit type as part of an instruction execution template.
 */

#ifndef _TEMPLATE_EXECUTION_UNIT_H_
#define _TEMPLATE_EXECUTION_UNIT_H_

#include <stdio.h>
#include <stdlib.h>
#include "BitSet.h"
#include <list>

namespace MIAMI
{

#define EXACT_MATCH_ALLOCATE    0

class TemplateExecutionUnit
{
public:
   TemplateExecutionUnit (int _unit, BitSet* _umask, int _count);
   ~TemplateExecutionUnit ()
   {
      if (unit_mask)
         delete unit_mask;
   }

   inline BitSet* UnitsMask () const  { return (unit_mask); }
   inline int Count () const   { return (count); }
   
   int index;
   int count;
   BitSet *unit_mask;
};

typedef std::list<TemplateExecutionUnit*> TEUList;

} /* namespace MIAMI */

#endif
