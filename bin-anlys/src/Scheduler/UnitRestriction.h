/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: UnitRestriction.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data structure to hold a unit restrictions MDL construct.
 */

#ifndef _UNIT_RESTRICTION_H
#define _UNIT_RESTRICTION_H

#include <stdio.h>
#include <list>
#include "TemplateExecutionUnit.h"

namespace MIAMI
{

/* Specify issue restrictions between units. 
 * When the unit list is NULL, the restriction applies to the number of 
 * retired instructions (just a lower bound on kernel length).
 */
class UnitRestriction
{
public:
   UnitRestriction(TEUList* _units, int _maxCount, char* _ruleName = NULL);
   ~UnitRestriction();
   
   bool IsActive() { return (active); }
   int MaxCount() { return (maxCount); }
   const char* RuleName() { return (ruleName); }
   TEUList* UnitsList() { return (unitsList); }
   unsigned int getRuleId() { return (ruleId); }
   
private:
   TEUList* unitsList;
   int maxCount;
   unsigned int ruleId;
   char* ruleName;
   bool active;
   static unsigned int numRules;
};

typedef std::list<UnitRestriction*> RestrictionList;

} /* namespace MIAMI */

#endif
