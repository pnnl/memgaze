/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: TemplateExecutionUnit.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines data structure to hold information about the use pattern of an
 * execution unit type as part of an instruction execution template.
 */

#include "TemplateExecutionUnit.h"

using namespace MIAMI;

TemplateExecutionUnit::TemplateExecutionUnit (int _unit, BitSet* _umask, 
            int _count)
{
   index = _unit;
   count = _count;
   unit_mask = _umask;
}

