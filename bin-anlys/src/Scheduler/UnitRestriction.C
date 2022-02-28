/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: UnitRestriction.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data structure to hold a unit restrictions MDL construct.
 */

#include "UnitRestriction.h"

using namespace MIAMI;

UnitRestriction::UnitRestriction(TEUList* _units, int _maxCount, char* _ruleName)
{
   unitsList = _units;
   maxCount = _maxCount;
   ruleId = ++numRules;
   if (_ruleName!=NULL)
      ruleName = _ruleName;
   else
   {
      ruleName = new char[20];
      sprintf(ruleName, "Restriction %d", ruleId);
   }
   active = true;
   // check if this rule is actually restrictive
   int numUnits = 0;
   if (unitsList)
   {
      TEUList::iterator teuit = unitsList->begin();
      while (numUnits<=maxCount && teuit!=unitsList->end())
      {
        numUnits += (*teuit)->UnitsMask()->Size ();
        ++ teuit;
      }
      if (numUnits <= maxCount)
      {
         fprintf (stderr, "WARNING: Restriction rule \"%s\" is not really restrictive. Number of units in the set (%d) is less or equal than the maximum number of units allowed (%d).\n",
             ruleName, numUnits, maxCount);
         active = false;
      }
   } else  // set active false for the retirement restriction because I do not want it applied
           // for each issued instruction. I only have to use this rule while computing the low
           // bound due to resources
      active = false;
}

UnitRestriction::~UnitRestriction()
{
   delete[] ruleName;
   if (unitsList)
   {
      TEUList::iterator teuit = unitsList->begin();
      for ( ; teuit!=unitsList->end() ; ++teuit)
         delete (*teuit);
      delete unitsList;
   }
}
