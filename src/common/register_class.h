/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: register_class.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Defines a generic register representation.
 */

#ifndef _REGISTER_CLASS_H
#define _REGISTER_CLASS_H

#include <list>
#include <iostream>
#include "miami_types.h"


namespace MIAMI {

enum RegisterClass {
    RegisterClass_INVALID = 0,
    RegisterClass_REG_OP,
    RegisterClass_MEM_OP,
    RegisterClass_LEA_OP,
    RegisterClass_STACK_REG,
    RegisterClass_STACK_OPERATION,
    RegisterClass_TEMP_REG,
    RegisterClass_PSEUDO,
    RegisterClass_LAST
};

typedef int16_t  stack_index_t;

const int max_reg_msb = 65535;

class register_info
{
public:
   // stack 0 is the default stack; actual register stacks start at index 1
   register_info(RegName r, RegisterClass t, int _lsb=0, int _msb=max_reg_msb, stack_index_t _stack=0)
   {
      name = r;
      type = t;
      lsb = _lsb;
      msb = _msb;
      stack = _stack;
   }

   register_info()
   {
      name = 0;
      type = RegisterClass_INVALID;
      lsb = 0;
      msb = max_reg_msb;
      stack = 0;
   }
   
   register_info(const register_info& ri)
   {
      name = ri.name;
      type = ri.type;
      lsb = ri.lsb;
      msb = ri.msb;
      stack = ri.stack;
   }

   register_info& operator= (const register_info& ri)
   {
      name = ri.name;
      type = ri.type;
      lsb = ri.lsb;
      msb = ri.msb;
      stack = ri.stack;
      return (*this);
   }
   
   // some classes are equivalent; they just define how the registers are used
   // i.e. part of a mem operand, part of a LEA operand, or general reg use
   inline static bool equal_classes(RegisterClass rc1, RegisterClass rc2)
   {
      switch(rc1)
      {
         case RegisterClass_REG_OP:
         case RegisterClass_MEM_OP:
         case RegisterClass_LEA_OP:
            return (rc2==RegisterClass_REG_OP || rc2==RegisterClass_MEM_OP ||
                   rc2==RegisterClass_LEA_OP);
         
         default:
            return (rc1 == rc2);
      }
   }
   
   inline bool operator== (const register_info& ri) const
   {
      return (name==ri.name && stack==ri.stack && 
              lsb==ri.lsb && msb==ri.msb && 
              equal_classes(type, ri.type));
   }
   inline bool operator!= (const register_info& ri) const
   {
      return (name!=ri.name || stack!=ri.stack ||
              lsb!=ri.lsb || msb!=ri.msb || 
              !equal_classes(type, ri.type));
   }

   inline bool operator< (const register_info& ri) const
   {
      if (stack != ri.stack)
         return (stack < ri.stack);

      // we ignore the register type for sorting. General purpose
      // registers can be defined as GP_REG or as ADDR_REG

      // I cannot find a way to partially sort based on bit range while providing 
      // consistent results. I will have to stop my testing at register names. 
      // Use a multimap and additional outside testing to process the results.
      return (name < ri.name);
   }
   
   inline void SubtractRange(int xlsb, int xmsb)
   {
      if (xlsb<=lsb && xmsb<=msb)  // remove lower range
         lsb = xmsb+1;
      else
         if (lsb<=xlsb && msb<=xmsb)  // remove upper range
            msb = xlsb-1;
   }
   
   // return true if the two bit ranges overlap even partially
   inline bool OverlapsRange(const register_info& ri) const
   {
      return (lsb<=ri.msb && ri.lsb<=msb);
   }
   // return true if this bit range completely includes ri's bit range
   inline bool IncludesRange(const register_info& ri) const
   {
      return (lsb<=ri.lsb && ri.msb<=msb);
   }
   // return true if this bit range is fully included in ri's bit range
   inline bool isIncludedInRange(const register_info& ri) const
   {
      return (ri.lsb<=lsb && msb<=ri.msb);
   }
   
   // return true if the reg names match and the two bit ranges overlap even partially
   inline bool Overlaps(const register_info& ri) const
   {
      return (name==ri.name && stack==ri.stack && equal_classes(type, ri.type) && lsb<=ri.msb && ri.lsb<=msb);
   }
   // return true if the reg names match and this bit range completely includes ri's bit range
   inline bool Includes(const register_info& ri) const
   {
      return (name==ri.name && stack==ri.stack && equal_classes(type, ri.type) && lsb<=ri.lsb && ri.msb<=msb);
   }
   // return true if the reg names match and this bit range is fully included in ri's bit range
   inline bool isIncludedIn(const register_info& ri) const
   {
      return (name==ri.name && stack==ri.stack && equal_classes(type, ri.type) && ri.lsb<=lsb && msb<=ri.msb);
   }
   
   std::string ToString() const;
   std::string RegNameToString() const;

   void SaveToFile(FILE* fd) const;
   void LoadFromFile(FILE* fd);

   RegName name;
   RegisterClass type;
   // I will need to store bit fields perhaps; store first and last accessed bits;
   int lsb;
   int msb;
   stack_index_t stack;  // pass index of register stack
                         // relevant only for STACK_REG and STACK_OPERATION types
};

typedef std::list<register_info> RInfoList;

/* I will need a way to compare register bit fields that overlap or not for the
 * same register name.
 */
class RegisterInfoCompare
{
public:
   bool operator() (const register_info& ri1, const register_info& ri2) const
   {
      if (ri1.stack != ri2.stack)
         return (ri1.stack < ri2.stack);

      // we ignore the register type for sorting. General purpose
      // registers can be defined as GP_REG or as ADDR_REG

      // I cannot find a way to partially sort based on bit range while providing 
      // consistent results. I will have to stop my testing at register names. 
      // Use a multimap and additional outside testing to process the results.
      return (ri1.name < ri2.name);
   }
};

extern const char* RegisterClassToString(RegisterClass rc);
extern std::ostream& operator<< (std::ostream& os, const register_info& ri);
extern const register_info MIAMI_NO_REG;

}   /* namespace MIAMI */


#endif
