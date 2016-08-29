/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: Machine.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the Machine class. Machines are built by
 * parsing files written in the Machine Description Language (MDL).
 */

#include <vector>
#include "Machine.h"
#include "debug_scheduler.h"

using namespace MIAMI;
namespace MIAMI {
extern int haveErrors;
}

unsigned int UnitRestriction::numRules = 0;
unsigned int ReplacementRule::numRules = 0;

Machine::Machine(const char* _name, const char* _version)
{
   name = _name;
   version = _version;
   units = NULL;
   asyncUnits = NULL;
   memLevels = NULL;
   instructions = NULL;
   instIndex = NULL;
   restrictions = NULL;
   replacements = NULL;
   bypasses = NULL;
   has_bypass_rules = false;
   // FIXME. set this field statically for now. Extend language to 
   // describe it
   has_branch_prediction = true; 
   unitsIndex = NULL;
   reversedUnitsIndex = NULL;
   totalUnitCount = 0;
}

const char* 
Machine::getNameForRestrictionWithId (int idx) const
{
   RestrictionList::iterator rlit = restrictions->begin();
   for ( ; rlit!=restrictions->end() ; ++rlit)
      if ( (*rlit)->getRuleId() == (unsigned)idx)
         return ( (*rlit)->RuleName());
   // if not found return NULL
   return (NULL);
}

const char* 
Machine::getNameForUnit (int idx, int& unitIdx) const
{
   assert (idx>=0 && idx<totalUnitCount);
   int unit = reversedUnitsIndex[idx];
   unitIdx = idx - unitsIndex[unit];
   return (units->getElementAtIndex(unit)->getLongName());
}

const char* 
Machine::getNameForUnitClass (int idx) const
{
   assert (idx>=0 && idx<units->NumberOfElements());
   return (units->getElementAtIndex(idx)->getLongName());
}

const char* 
Machine::getNameForAsyncResource (int idx) const
{
   assert (asyncUnits && idx>=0 && idx<asyncUnits->NumberOfElements());
   return (asyncUnits->getElementAtIndex(idx)->getLongName());
}

int 
Machine::getMultiplicityOfAsyncResource(int idx) const
{
   assert (asyncUnits && idx>=0 && idx<asyncUnits->NumberOfElements());
   return (asyncUnits->getElementAtIndex(idx)->getCount());
}

int 
Machine::hasBypassLatency(const InstructionClass& _source, int _edge, const InstructionClass& _sink) const
{
   if (has_bypass_rules)
   {
      BypassList::iterator bit = bypasses->begin();
      for ( ; bit!=bypasses->end() ; ++bit )
         if ( (*bit)->matchesRule(_source, _edge, _sink) )
            return ( (*bit)->bypassLatency() );
   }
   return (-1);  // no bypass rule for this pair of instructions
}

unsigned int 
Machine::computeLatency(addrtype pc, addrtype reloc, const InstructionClass& _source, 
          const InstructionClass& _sink, int templateIdx) const
{
   Instruction *temp = instructions->find(_source);
   if (!temp)
   {
      haveErrors += 1;
      fprintf (stderr, "ERROR: Machine::computeLatency: cannot find information for instruction at 0x%" PRIxaddr " of class %s\n",
           pc, _source.ToString().c_str());
      debug_decode_instruction_at_pc((void*)(pc+reloc), 100);
      assert (!"no latency information for this instruction class");
      return (0);
   } else
      return (temp->getMinLatency (_sink, templateIdx));
}

void 
Machine::addMemoryLevels (MemLevelAssocTable* _memLevels)
{
   memLevels = _memLevels;
}

void 
Machine::addUnits (ExecUnitAssocTable* _units) 
{
   int i, j;
   units = _units;
   int numUnits = units->NumberOfElements ();
   unitsIndex = new int[numUnits];
   totalUnitCount = 0;
   for (i=0 ; i<numUnits ; ++i)
   {
      // compute also partial sums of number of units in all classes with index
      // less than current one
      unitsIndex[i] = totalUnitCount;
      int crtCount = units->getElementAtIndex (i) ->getCount();
      totalUnitCount += crtCount;
   }

   reversedUnitsIndex = new int[totalUnitCount];
   for (i=0 ; i<numUnits ; ++i)
   {
      int uLimit;
      if (i<numUnits-1)
         uLimit = unitsIndex[i+1];
      else
         uLimit = totalUnitCount;
      for (j=unitsIndex[i] ; j<uLimit ; ++j)
         reversedUnitsIndex[j] = i;
   }
}

void 
Machine::addInstructionDescriptions(InstructionContainer* insts)
{
   int i, j;
   instructions = insts;
   // create also an index where instructions are sorted by the maximum single 
   // unit score and by the number of templates they have
   numInstClasses = insts->NumberOfInstructions();
   float *scoreBuffer = new float[totalUnitCount];
   
   // compute a score for each instruction class that has been defined
   instIndex = new InstructionClass[numInstClasses];
   float *instScore = new float[numInstClasses];
   int *numTempl = new int[numInstClasses];
   i = 0;
   InstructionContainer::InstructionBinIterator ibit(instructions);
   while ((bool)ibit)
   {
      Instruction *ii = (Instruction*)ibit;
      assert (ii!=NULL);
      if (ii->isDefined())
      {
         instScore[i] = ii->getResourceScore (totalUnitCount, 
                      unitsIndex, scoreBuffer);
      } else
         instScore[i] = 0.0f;
      numTempl[i] = ii->getNumTemplates();
      instIndex[i] = ibit.getIClass();
      ++ ibit;
      ++ i;
   }
   // safety check: we should have seen numInstClasses instruction types
   if (i != numInstClasses)
   {
      std::cerr << "i=" << i << ", numInstClasses=" << numInstClasses << std::endl;
      assert (i == numInstClasses);
   }
#if DEBUG_MACHINE_CONSTRUCTION
   DEBUG_MACHINE (2,
   std::cerr << "Machine::addInstructionDescription: found " << numInstClasses 
             << " instruction classes." << std::endl;
   )
#endif
   
#if DEBUG_MACHINE_CONSTRUCTION
   DEBUG_MACHINE (6,
      fprintf (stderr, "Machine::addInstructionDescriptions: Order of instruction types sorted by resource usage score:\n");
   )
#endif   

   for (i=0 ; i<numInstClasses-1 ; ++i)
   {
      int maxValAt = i;
      for (j=i+1 ; j<numInstClasses ; ++j)
         if (instScore[maxValAt] < instScore[j])
            maxValAt = j;
      if (maxValAt > i)
      {
         float tempF = instScore[i];
         instScore[i] = instScore[maxValAt];
         instScore[maxValAt] = tempF;
         InstructionClass temp = instIndex[i];
         instIndex[i] = instIndex[maxValAt];
         instIndex[maxValAt] = temp;
      }

#if DEBUG_MACHINE_CONSTRUCTION
      DEBUG_MACHINE (6,
         fprintf (stderr, "%s-%s-%s-%" PRIwidth " (%f), ", 
                  Convert_InstrBin_to_string(instIndex[i].type), 
                  ExecUnitToString(instIndex[i].eu_style),
                  ExecUnitTypeToString(instIndex[i].eu_type), 
                  instIndex[i].width, instScore[i]);
      )
#endif

   }

#if DEBUG_MACHINE_CONSTRUCTION
   DEBUG_MACHINE (6,
      fprintf (stderr, "\n");
   )
#endif

   delete[] numTempl;
   delete[] scoreBuffer;
   delete[] instScore;
}


void 
Machine::print(FILE* fd) const
{
   fprintf(fd, "======== Machine Description ========\n");
   fprintf(fd, "TargetName: %s, Version %s\n", name,
         (version?version:"Undefined") );
   int numUnits = units->NumberOfElements ();
   fprintf(fd, "  Machine has %d classes of units:\n", numUnits );
   for (int i=0 ; i<numUnits ; ++i)
   {
      const MachineExecutionUnit *meu = units->getElementAtIndex (i);
      fprintf(fd, "    - unit %d: %s has %d replicas\n", 
                  i, meu->getLongName (), meu->getCount () );
   }
   
   fprintf(fd, "========= Unit Restrictions =========\n");
   int restrRules = restrictions?restrictions->size():0;
   fprintf(fd, "There are %d unit restriction rules.\n", restrRules);
   if (restrRules)
   {
      RestrictionList::iterator rit = restrictions->begin();
      for ( ; rit!=restrictions->end() ; ++rit)
      {
         TEUList *uList = (*rit)->UnitsList ();
         if (uList == 0)  // retirement rule
         {
            fprintf (fd, "Rule %s: At most %d instructions retired per cycle.\n",
                 (*rit)->RuleName(), (*rit)->MaxCount() );
         } else
         {
            bool first = true;
            TEUList::iterator teuit = uList->begin();
            fprintf (fd, "Rule %s: At most %d units out of ",
                 (*rit)->RuleName(), (*rit)->MaxCount() );
            for ( ; teuit!=uList->end() ; ++teuit)
            {
               const MachineExecutionUnit *meu = units->
                            getElementAtIndex ((*teuit)->index);
               if (first)
               {
                  fprintf (fd, "%s[", meu->getLongName () );
                  first = false;
               } else
                  fprintf (fd, ", %s[", meu->getLongName () );
               
               BitSet::BitSetIterator bsit(*((*teuit)->unit_mask));
               int elem = bsit;
               fprintf (fd, "%d", elem);
               ++ bsit;
               while((bool)bsit)
               {
                  int elem = bsit;
                  fprintf (fd, ",%d", elem);
                  ++ bsit;
               }
               fprintf(fd, "]");
            }
            fprintf(fd, " can be active at any time.\n" );
         }
      }
   }
   
   fprintf(fd, "====== Instruction Description ======\n");
   int numInsts = instructions->NumberOfInstructions();
   fprintf(fd, "  There are %d classes of instructions:\n", numInsts );
   int idx = 0;
   InstructionContainer::InstructionBinIterator ibit(instructions);
   while ((bool)ibit)
   {
      Instruction *ii = (Instruction*)ibit;
      assert (ii!=NULL);
      InstTemplate* temp = ii->getTemplateWithIndex(0);
      InstructionClass ic = ibit.getIClass();
      fprintf(fd, "    - (%d) class %d-%d-%d-%" PRIwidth " has %d templates; first template has length %d cycles.\n", 
           idx, ic.type, ic.eu_style, ic.eu_type, ic.width, 
           ii->getNumTemplates(), (temp?temp->getLength():-1) );

      ++ ibit;
      ++ idx;
   }


   fprintf(fd, "========= Bypass Rules =========\n");
   int bypassCnt = bypasses?bypasses->size():0;
   fprintf(fd, "There are %d bypass latency rules.\n", bypassCnt);
   if (bypasses)
   {
      BypassList::iterator blit = bypasses->begin();
      int i=1;
      for ( ; blit!=bypasses->end() ; ++blit, ++i )
      {
         fprintf( fd, "Bypass %d: ", i);
         (*blit)->print(fd);
      }
   }

   fprintf(fd, "========= Replacement Rules =========\n");
   int replRules = replacements?replacements->size():0;
   fprintf(fd, "There are %d instruction replacement rules.\n", replRules);

   fprintf(fd, "========== End Description ==========\n");
   
}


Machine::~Machine()
{
#if DEBUG_MACHINE_CONSTRUCTION
   DEBUG_MACHINE (1,
      fprintf(stderr, "In Machine destructor.\n");
   )
#endif
   
   /* MUST DEALLOCATE both the units and the instructions */
   /* and perhaps the name and the version which were allocated in lex */
   delete[] name;
   if (version)
      delete[] version;
   if (units)
      delete units;
   if (unitsIndex)
      delete[] unitsIndex;
   if (reversedUnitsIndex)
      delete[] reversedUnitsIndex;
   if (instructions)
      delete[] instructions;
   if (instIndex)
      delete[] instIndex;
   if (memLevels)
      delete memLevels;
   if (replacements)
   {
      ReplacementList::iterator rit = replacements->begin();
      for ( ; rit!=replacements->end() ; ++rit)
         delete (*rit);
      delete replacements;
   }
   if (bypasses)
   {
      BypassList::iterator bit = bypasses->begin();
      for ( ; bit!=bypasses->end() ; ++bit)
         delete (*bit);
      delete bypasses;
   }
   if (restrictions)
   {
      RestrictionList::iterator rit = restrictions->begin();
      for( ; rit!=restrictions->end() ; ++rit)
         delete (*rit);
      delete restrictions;
   }
}

