/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: Instruction.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Object associated with an instruction class, incuding its
 * execution templates read from a machine file.
 */

#ifndef _INSTRUCTION_H
#define _INSTRUCTION_H

#include "InstTemplate.h"
#include "position.h"
#include "instr_bins.H"
#include "instr_info.H"
#include "uipair.h"
#include "UnitRestriction.h"

namespace MIAMI
{

#define I_DEFINED        1
#define I_ANALYZED       2
#define I_COST_ANALYZED  4

class UnitCountPair
{
public:
   unsigned int first;
   unsigned int second;
   
   void Assign(unsigned int _f, unsigned int _s) {
      first = _f; second = _s;
   }
};
typedef std::list<UnitCountPair> UnitCountList;


class Instruction
{
public:
   Instruction()
   { 
      iclass.Initialize(IB_unknown, ExecUnit_LAST, 0, ExecUnitType_LAST, 0);
      Ctor();
   }
   
   Instruction(const InstructionClass &ic) : iclass(ic)
   { 
      Ctor();
   }

   Instruction(InstrBin _type, ExecUnit _eu = ExecUnit_SCALAR, int _vwidth = 0,
         ExecUnitType _eu_type = ExecUnitType_INT, width_t _width = max_operand_bit_width)
   {
      iclass.type = _type;
      iclass.eu_style = _eu;
      iclass.vec_width = _vwidth;
      iclass.eu_type = _eu_type;
      iclass.width = _width;
      Ctor();
   }
   
   ~Instruction();
   
   const InstructionClass& getIClass() const { return (iclass); }
   
   void addDescription (Position& _pos, ITemplateList& itList, UnitCountList *_asyncUsage);
   int getNumTemplates () { return (numTemplates); }
   InstTemplate* getTemplateWithIndex (unsigned int idx);
   const UnitCountList* getAsyncResourceUsage() const  { return (asyncUsage); }
   
   unsigned int getMinLatency (const InstructionClass &consumerType, int templateIdx = 0);
   bool hasVariableLength ();
   bool isDefined () { return (flags & I_DEFINED); }
   
   float getResourceScore (int numAllUnits, const int *unitsIndex, 
                float *scoreBuffer);
   float* getResourceUsage (float* buffer, unsigned int bsize, 
                const int* unitsIndex);
   float* getBalancedResourceUsage (float* sumBuffer, unsigned int bsize,
             const int* unitsIndex, unsigned int numInstances, 
             const RestrictionList* restrict, unsigned int asyncSize, 
             float* asyncBuffer);
   
   // this routine is used by the scheduler to select and allocate a template 
   // for an instruction given the current allocation of units on the machine,
   // position where instruction should be scheduled, maximum latency accepted
   // for this allocation, and template index from where to start searching
   // (used by successive calls in a backtracking approach). It returns the 
   // index of the template allocated, or -1 if failure.
   int findAndUseTemplate (BitSet** sched_template, int schedLen, int startPos, 
               int maxExtraLatency, int startFromIdx, const RestrictionList* restrict, 
               const int* unitsIndex, MIAMIU::PairList& selectedUnits, int &unit);
   void deallocateTemplate (BitSet** sched_template, int schedLen, 
               int startPos, MIAMIU::PairList& selectedUnits);
   
   TEUList* getUnits (int templIdx, int offset);

   class CompareInstructionWidth
   {
   public:
      bool operator() (const Instruction *&i1, const Instruction *&i2) const
      {
         return (i1->iclass.width < i2->iclass.width);
      }
   };
   
   class CompareInstructionVecAndWidth
   {
   public:
      bool operator() (const Instruction *&i1, const Instruction *&i2) const
      {
         if (i1->iclass.vec_width != i2->iclass.vec_width)
            return (i1->iclass.vec_width < i2->iclass.vec_width);
         return (i1->iclass.width < i2->iclass.width);
      }
   };
   
private:
   void Ctor()
   {
      flags = 0; 
      numTemplates = 0; templates = NULL;
      hasSpecialCases = false;
      resourceUsage = NULL;
      numResources = 0;
      resourceScore = 0;
   }
   
   int flags;
   InstructionClass iclass;
   
   Position position;  // position in file where it is defined;
   unsigned int numTemplates;
   bool hasSpecialCases;       // set this flag if instruction has bypasses defined
   InstTemplate **templates;   // an array of templates
   float resourceScore;        // maximum usage score of any unit
   float *resourceUsage;       // the weighted use of each unit
   unsigned int numResources;  // length of resourceUsage array
   UnitCountList *asyncUsage;  // list of asynchronous resources used by an instruction
};

typedef std::pair<int, width_t> InstWidth;  // Instruction width type (vector width, elem width);
class InstWidthCompare
{
public:
   bool operator() (const InstWidth& iw1, const InstWidth& iw2) const
   {
      if (iw1.first != iw2.first)
         return (iw1.first < iw2.first);
      return (iw1.second < iw2.second);
   }
};

typedef std::map<InstWidth, Instruction*, InstWidthCompare> InstructionStorage;

class InstructionContainer
{
public:
   InstructionContainer()
   {
      numInstructions = 0;
   }
   
   ~InstructionContainer();
   
   /* search for an instruction with the given characteristics */
   Instruction* find(InstrBin type, ExecUnit eu, int _vwidth, ExecUnitType eu_type, width_t _width = 0);
   /* similar to the previous method, but search based on an InstructionClass object */
   Instruction* find(const InstructionClass &iclass);

   /* search for an instruction class of the largest vector size that accomodates 
    * the given instruction characteristics */
   Instruction* findMaxVector(InstrBin type, ExecUnit eu, int _vwidth, ExecUnitType eu_type, width_t _width = 0);
   /* similar to the previous method, but search based on an InstructionClass object */
   Instruction* findMaxVector(const InstructionClass &iclass);

   /* search for an instruction with the given characteristics and create a new one if
    * a match is not found */
   Instruction* insert(InstrBin type, ExecUnit eu, int _vwidth, ExecUnitType eu_type, 
               width_t _width = max_operand_bit_width);
   /* similar to the previous method, but search based on an InstructionClass object */
   Instruction* insert(const InstructionClass &iclass);

   Instruction* addDescription (const InstructionClass &iclass, Position& _pos, ITemplateList& itList,
           UnitCountList *asyncUsage=0);
   
   int NumberOfInstructions() const  { return (numInstructions); }
   
   class InstructionBinIterator
   {
   public:
      InstructionBinIterator(InstructionContainer* _ic);
      InstructionClass getIClass() const;
      Instruction* operator-> () const;
      void operator++ ();
      operator bool () const  { return (valid); }
      operator Instruction* () const;
      
   private:
      InstructionContainer *ic;
      InstructionStorage::iterator iter;
      InstrBin crt_bin;
      ExecUnit crt_eu;
      ExecUnitType crt_eut; 
      bool valid;
   };
   
   friend class InstructionBinIterator;
   
private:
   // for each combination of execution unit style (scalar / vector) and 
   // type (int / fp), I keep a set of templates, sorted by bit width
   InstructionStorage istore[IB_TOP_MARKER][ExecUnit_LAST][ExecUnitType_LAST];
   
   int numInstructions;
};

}

#endif
