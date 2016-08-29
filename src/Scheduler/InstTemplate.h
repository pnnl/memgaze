/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: InstTemplate.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data structure to hold an instruction execution template defined in 
 * a machine description file.
 */

#ifndef _INST_TEMPLATE_H
#define _INST_TEMPLATE_H

#include <assert.h>
#include "TemplateExecutionUnit.h"
#include <list>

namespace MIAMI
{

typedef std::list <TEUList*> MultiCycTemplate;

class InstTemplate
{
public:
   InstTemplate () { length = 0; }
   
   ~InstTemplate ();
   
   void addCycles (TEUList* teul, int repeat = 1);
   
   int getLength () const { return (length); }
   MultiCycTemplate& getTemplates () { return (iTemplate); }

private:
   int length;
   MultiCycTemplate iTemplate;
};

typedef std::list<InstTemplate*> ITemplateList;

class AscendingInstTemplates
{
public:
   int operator () (const InstTemplate* it1, const InstTemplate* it2) const
   {
      return (it1->getLength() < it2->getLength());
   }
};

} /* namespace MIAMI */

#endif
