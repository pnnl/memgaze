/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: GenericInstruction.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines a machine independent representation of an instruction with type, 
 * bit width, vector length, and lists of source and destination operands.
 */

#ifndef _GENERIC_INSTRUCTION_H
#define _GENERIC_INSTRUCTION_H

#include <stdlib.h>
#include "dependency_type.h"
#include "position.h"
#include "instruction_class.h"
#include <list>

namespace MIAMI
{

class GenericRegister
{
public:
   GenericRegister(const char* _regname, int _type, Position& _pos) :
        regName(_regname), type(_type)
   { 
      pos = _pos;
   }
   ~GenericRegister() { delete[] regName; }
   
   const char* RegName() { return (regName); }
   int Type() { return (type); }
   const Position& FilePosition() { return (pos); };
   
private:
   const char* regName;
   int type;
   Position pos;
};

typedef std::list<GenericRegister*> GenRegList;

class GenericInstruction
{
public:
   GenericInstruction(InstructionClass _iclass, GenRegList* _srcs, GenRegList* _dests) :
        iclass(_iclass), srcRegs(_srcs), destRegs(_dests)
   {
      exclusiveOutput = false;
      if (!srcRegs) srcRegs = new GenRegList();
      if (!destRegs) destRegs = new GenRegList();
   }
   ~GenericInstruction() 
   {
      GenRegList::iterator nlit;
      if (srcRegs != NULL)
      {
         for (nlit=srcRegs->begin() ; nlit!=srcRegs->end() ; ++nlit)
            delete (*nlit);
         delete srcRegs;
      }
      if (destRegs != NULL)
      {
         for (nlit=destRegs->begin() ; nlit!=destRegs->end() ; ++nlit)
            delete (*nlit);
         delete destRegs;
      }
   }
//   int Type() const      { return (iclass.type); }
   const InstructionClass& getIClass() const  { return (iclass); }
   GenRegList* SourceRegisters() { return (srcRegs); }
   GenRegList* DestRegisters() { return (destRegs); }
   
   // match only patterns where the result of this instruction is used
   // only by instructions matched by the source pattern
   void setExclusiveOutputMatch()     { exclusiveOutput = true; }
   bool ExclusiveOutputMatch() const  { return (exclusiveOutput); }
   
private:
   InstructionClass iclass;
   GenRegList* srcRegs;
   GenRegList* destRegs;
   bool exclusiveOutput;
};

typedef std::list<GenericInstruction*> GenInstList;

} /* namespace MIAMI */

#endif
