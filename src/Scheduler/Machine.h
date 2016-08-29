/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: Machine.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Defines the Machine class. Machines are built by
 * parsing files written in the Machine Description Language (MDL).
 */

#ifndef _MACHINE_H_
#define _MACHINE_H_

#include <stdio.h>
#include <map>
#include <list>
#include "position.h"
#include "Instruction.h"
#include "ReplacementRule.h"
#include "UnitRestriction.h"
#include "BypassRule.h"
#include "MachineExecutionUnit.h"
#include "MemoryHierarchyLevel.h"
#include "instruction_decoding.h"

namespace MIAMI
{

// keep a map of values for various machine characteristics.
// This applies only to those machine characteristics that have
// an integer value;
enum MachineFeature { 
        MachineFeature_INVALID, 
        MachineFeature_WINDOW_SIZE,
        MachineFeature_VECTOR_LENGTH,
        MachineFeature_LAST
};

typedef std::map<MachineFeature, int32_t> MachineFeatureMap;

#define MACHINE_ASYNC_UNIT_BASE  0x10000

class Machine
{
public:
   Machine (const char* _name, const char* _version);
   ~Machine ();
   
   void addUnits (ExecUnitAssocTable* _units);
   void addAsyncUnits (ExecUnitAssocTable* _asyncUnits) { asyncUnits = _asyncUnits; }
   
   void addMemoryLevels (MemLevelAssocTable* _memLevels);
   MemLevelAssocTable *getMemoryLevels () const { return (memLevels); }

   void addInstructionDescriptions(InstructionContainer* insts);

   void addReplacementRules (ReplacementList* rRules)
   {
      replacements = rRules;
   }
   const ReplacementList* Replacements() const  { return (replacements); }
   
   void addBypassRules (BypassList* bRules)
   {
      bypasses = bRules;
      if (bypasses->size() > 0)
         has_bypass_rules = true;
   }
   const BypassList* Bypasses() const  { return (bypasses); }
   
   Instruction* InstructionOfType(InstrBin _type, ExecUnit _eu, int _vwidth, 
                ExecUnitType _eu_type, uint16_t _width) const
   { 
      return (instructions->find(_type, _eu, _vwidth, _eu_type, _width)); 
   }
   Instruction* InstructionOfType(const InstructionClass &ic) const
   { 
      return (instructions->find(ic)); 
   }

   Instruction* InstructionOfLongestVectorType(InstrBin _type, ExecUnit _eu, int _vwidth, 
                ExecUnitType _eu_type, uint16_t _width) const
   { 
      return (instructions->findMaxVector(_type, _eu, _vwidth, _eu_type, _width)); 
   }
   Instruction* InstructionOfLongestVectorType(const InstructionClass &ic) const
   { 
      return (instructions->findMaxVector(ic)); 
   }

   const InstructionClass* getInstructionsIndex() const { return (instIndex); }

   void addUnitRestrictions (RestrictionList* restr)
   {
      restrictions = restr;
   }
   const RestrictionList* Restrictions() const { return (restrictions); }

   // return true if the element was inserted, false if there was already a 
   // value set for the same feature
   bool setMachineFeature(MachineFeature key, int32_t value)
   {
      std::pair<MachineFeatureMap::iterator, bool> res = 
              machineFeatures.insert(MachineFeatureMap::value_type(key, value));
      return (res.second);
   }

   // get the value associated with a machine feature
   int32_t getMachineFeature(MachineFeature key)
   {
      MachineFeatureMap::iterator res = machineFeatures.find(key);
      if (res == machineFeatures.end())
         return (-1);
      else
         return (res->second);
   }
   
   int hasBypassLatency(const InstructionClass& _source, int _edge, 
             const InstructionClass& _sink) const;
   
   // pass the address of the source instruction for debugging / error 
   // reporting purposes
   unsigned int computeLatency(addrtype pc, addrtype reloc, 
             const InstructionClass& _source, const InstructionClass& _sink, 
             int templateIdx = 0) const;
   
   inline int hasBranchPrediction() const { return (has_branch_prediction); }
   inline int NumberOfUnitClasses() const { return (units->NumberOfElements()); }
   inline int TotalNumberOfUnits() const  { return (totalUnitCount); }
   inline int NumberOfAsyncResources() const { 
      if (asyncUnits)
         return (asyncUnits->NumberOfElements()); 
      else
         return (0);
   }
   int getMultiplicityOfAsyncResource(int idx) const;
   
   inline int NumberOfInstructionClasses() const { return (numInstClasses); };
   inline const int* getUnitsIndex() const     { return (unitsIndex); }
   inline const int* getReversedUnitsIndex() const { return (reversedUnitsIndex); }
   
   const char* getNameForUnit(int idx, int& unitIdx) const;
   const char* getNameForUnitClass(int idx) const;
   const char* getNameForAsyncResource (int idx) const;
   const char* getNameForRestrictionWithId(int idx) const;
   
   void print(FILE* fd) const;
   const char* MachineName() const { return (name); }
   const char* MachineVersion() const { return (version); }
   
private:
   ExecUnitAssocTable *units, *asyncUnits;
   MemLevelAssocTable *memLevels;
   int *unitsIndex;
   int *reversedUnitsIndex;
   int totalUnitCount;
   int numInstClasses;
   InstructionContainer* instructions;
   InstructionClass *instIndex;
   ReplacementList* replacements;
   RestrictionList* restrictions;
   BypassList* bypasses;
   const char* name;
   const char* version;
   bool has_branch_prediction;
   bool has_bypass_rules;
   
   // keep a map for simple machine characteristics that can be expressed by 
   // an integer value
   MachineFeatureMap machineFeatures;
};

typedef std::list<Machine*> MachineList;


} /* namespace MIAMI */

// this method comes from yacc. Have it in global namespace
extern MIAMI::Machine* parseMachineDescription(const char *fileName);

#endif
