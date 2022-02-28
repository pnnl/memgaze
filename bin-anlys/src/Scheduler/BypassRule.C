/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: BypassRule.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements a data structure to hold MDL (machine description language)
 * bypass rules.
 */

#include "BypassRule.h"
#include "instr_bins.H"
#include "dependency_type.h"

using namespace MIAMI;

BypassRule::BypassRule (ICSet* _source, BitSet* _edge, ICSet* _sink, 
         unsigned int _latency, Position& _pos) : 
       sourceType(_source), edgeType(_edge), sinkType(_sink),
       latency(_latency)
{
   pos = _pos;
}

BypassRule::~BypassRule ()
{
   delete (sourceType);
   delete (edgeType);
   delete (sinkType);
}

bool 
BypassRule::matchesRule(const InstructionClass& _source, int _edge, const InstructionClass& _sink)
{
   /** search in ICSet. Should I include the width in the search? 
    ** I may need to change the Comparator clas 
    ** TODO 
    */
   return ((*edgeType)[_edge] && (*sourceType)[_source] && (*sinkType)[_sink]);
}

void
BypassRule::print (FILE* fd)
{
   BitSet::BitSetIterator bsit(*edgeType);
   fprintf (fd, "A dependency of type {%s", dependencyTypeToString (bsit));
   ++ bsit;
   while ((bool)bsit)
   {
      fprintf (fd, "|%s", dependencyTypeToString (bsit) );
      ++ bsit;
   }
   
   ICSet::iterator icit = sourceType->begin();
   if (icit != sourceType->end())  // I should not have empty sets, but check nevertheless
   {
      fprintf (fd, "} from an instruction of one of the types {%s-%s-%s-%" PRIwidth, 
          Convert_InstrBin_to_string(icit->type), ExecUnitToString(icit->eu_style),
          ExecUnitTypeToString(icit->eu_type), icit->width );
      ++ icit;
   }
   for ( ; icit!=sourceType->end() ; ++icit)
   {
      fprintf (fd, " | %s-%s-%s-%" PRIwidth, 
          Convert_InstrBin_to_string(icit->type), ExecUnitToString(icit->eu_style),
          ExecUnitTypeToString(icit->eu_type), icit->width );
   }

   icit = sinkType->begin();
   if (icit != sinkType->end())  // I should not have empty sets, but check nevertheless
   {
      fprintf (fd, "} to an instruction of type {%s-%s-%s-%" PRIwidth, 
          Convert_InstrBin_to_string(icit->type), ExecUnitToString(icit->eu_style),
          ExecUnitTypeToString(icit->eu_type), icit->width );
      ++ icit;
   }
   for ( ; icit!=sinkType->end() ; ++icit)
   {
      fprintf (fd, " | %s-%s-%s-%" PRIwidth, 
          Convert_InstrBin_to_string(icit->type), ExecUnitToString(icit->eu_style),
          ExecUnitTypeToString(icit->eu_type), icit->width );
   }
   fprintf (fd, "} has %u cycles latency.\n", latency);
}
