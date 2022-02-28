/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: ReplacementRule.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines data structure to hold information about an MDL replacement rule.
 */

#ifndef _REPLACEMENT_RULE_H
#define _REPLACEMENT_RULE_H

#include <stdio.h>
#include "GenericInstruction.h"
#include "PatternGraph.h"

#include <list>
#include <map>

namespace MIAMI
{

/* For each replacement rule do a DFS traversal of the dependency graph.
 * Parse only register and memory dependencies. Use a separate traversal 
 * for each rule in case I want to use the result of a previous rule in a
 * future search.
 */
class ReplacementRule
{
public:
   ReplacementRule(GenInstList* _srcPattern, const Position& _srcPos, 
         GenInstList* _destPattern, const Position& _destPos,
         char* _ruleName = NULL) :
            srcPattern(_srcPattern), destPattern(_destPattern),
            srcPos(_srcPos), destPos(_destPos)
   {
      if (_ruleName!=NULL)
         ruleName = _ruleName;
      else
      {
         ruleName = new char[20];
         sprintf(ruleName, "Replacement %d", ++numRules);
      }
      srcGraph = new PatternGraph(srcPattern, srcPos, regInfo);
   }

   ~ReplacementRule()
   {
      GenInstList::iterator git = srcPattern->begin();
      for ( ; git!=srcPattern->end() ; ++git)
         delete (*git);
      delete srcPattern;
      git = destPattern->begin();
      for ( ; git!=destPattern->end() ; ++git)
         delete (*git);
      delete destPattern;
      
      delete[] ruleName;
      delete srcGraph;
   }

   GenInstList* SourcePattern() const { return (srcPattern); }
   GenInstList* DestinationPattern() const { return (destPattern); }
   const char* RuleName()  { return (ruleName); }
   PatternGraph* TemplateGraph()  { return (srcGraph); }
   PRegMap& RegisterMap()  { return (regInfo); }
   const Position& SourcePatternPosition() const  { return (srcPos); }
   const Position& DestPatternPosition() const  { return (destPos); }
   
private:
   GenInstList *srcPattern;
   GenInstList *destPattern;
   Position srcPos, destPos;
   char* ruleName;
   static unsigned int numRules;
   PRegMap regInfo;
   PatternGraph* srcGraph;
};

typedef std::list<ReplacementRule*> ReplacementList;

}

#endif
