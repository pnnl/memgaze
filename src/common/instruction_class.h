/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: instruction_class.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Data type to encapsulate the parameters that uniquely define 
 * a MIAMI instruction type.
 */

#ifndef _INSTRUCTION_CLASS_H
#define _INSTRUCTION_CLASS_H

#include "miami_types.h"
#include "instr_bins.H"

#include <set>
#include <map>
#include <string>

namespace MIAMI {


enum ExecUnit {
    ExecUnit_INVALID = 0,
    ExecUnit_VECTOR,
    ExecUnit_SCALAR,
    ExecUnit_LAST
};

enum ExecUnitType {
    ExecUnitType_INVALID = 0,
    ExecUnitType_FP,
    ExecUnitType_INT,
    ExecUnitType_LAST
};

const width_t max_operand_bit_width = (width_t)(-1);

class InstructionClass
{
public:
   /* eliminated the constructor because a non-trivial constructor or copy operator
    * prevents the class from being used as part of an union. I have a union field of
    * type InstructionClass inside the yacc parser for the machine description language.
    * Cannot have even a trivial constructor.
    *
    * Recommended usage: Declare an object of this type then invoke the Initialize 
    * method on it.
    */
/*
   InstructionClass() : type(IB_unknown), eu_style(ExecUnit_LAST), 
         eu_type(ExecUnitType_LAST), width(max_operand_bit_width)
   {}
*/
   
   void Initialize(InstrBin _type = IB_unknown, ExecUnit _eu = ExecUnit_SCALAR, 
         int _vwidth = 0, ExecUnitType _eu_type = ExecUnitType_INT, 
         width_t _width = max_operand_bit_width)
   {
      type = _type;
      eu_style = _eu;
      eu_type = _eu_type;
      width = _width;
      vec_width = _vwidth;
   }
   
   bool isValid() const
   {
      if  (type<=IB_INVALID || type>=IB_TOP_MARKER || type==IB_unknown ||
            eu_style<=ExecUnit_INVALID || eu_style>=ExecUnit_LAST || 
            eu_type<=ExecUnitType_INVALID || eu_type>=ExecUnitType_LAST)
         return (false);
      else
         return (true);
   }
   
   // partial equality and inequality operators; compare based on
   // type, eu_style and eu_type, but not by width or vec_width (these can have wildcard values)
   bool operator== (const InstructionClass &ic) const
   {
      return (type==ic.type && eu_style==ic.eu_style && eu_type==ic.eu_type); // && vec_width==ic.vec_width);
   }
   bool operator!= (const InstructionClass &ic) const
   {
      return (type!=ic.type || eu_style!=ic.eu_style || eu_type!=ic.eu_type); // || vec_width!=ic.vec_width);
   }

   // full equality operator; compare based on type, eu_style, eu_type, width and vec_width
   bool equals(const InstructionClass &ic) const
   {
      return (type==ic.type && eu_style==ic.eu_style && eu_type==ic.eu_type &&
              width==ic.width && vec_width==ic.vec_width);
   }
   
   std::string ToString() const;
   
   InstrBin type;         // instruction bin type
   ExecUnit eu_style;     // unit style: scalar, vector
   ExecUnitType eu_type;  // unit type: int, float
   width_t width;         // operand bit width: 32, 64, ...
   int vec_width;         // vector width in bits
};


class InstructionClassCompare
{
public:
   bool operator() (const InstructionClass &ic1, const InstructionClass &ic2) const
   {
      if (ic1.type != ic2.type)
         return (ic1.type < ic2.type);
      if (ic1.eu_style != ic2.eu_style)
         return (ic1.eu_style < ic2.eu_style);
      if (ic1.eu_type != ic2.eu_type)
         return (ic1.eu_type < ic2.eu_type);
      if (ic1.vec_width != ic2.vec_width)
         return (ic1.vec_width < ic2.vec_width);
      return (ic1.width < ic2.width);
   }
};

typedef std::map<InstructionClass, int64_t, InstructionClassCompare> ICIMap;
typedef std::set<InstructionClass, InstructionClassCompare> icset_t;

ICIMap& operator+= (ICIMap& f, const ICIMap& s);

class ICSet
{
public:
   typedef icset_t::const_iterator iterator;
   
   inline iterator begin() const
   {
      return (values.begin());
   }
   inline iterator end() const
   {
      return (values.end());
   }
   
   ICSet() {
      all_ic_defined = false;
   }
   
   void setAllValues() { all_ic_defined = true; }
   
   // return true if a given InstructionClass is in the set, false otherwise
   bool operator[] (const InstructionClass& ic) const
   {
      if (all_ic_defined)
         return (true);
      icset_t::const_iterator lbit = values.lower_bound(ic);
      return (lbit!=values.end() && ic==*lbit);
   }
   
   // returns true if a new element is inserted;
   // false if the element was already part of the set
   bool insert(const InstructionClass& ic)
   {
      std::pair<icset_t::const_iterator, bool> res = values.insert (ic);
      return (res.second);
   }
   
private:
   icset_t values;
   bool  all_ic_defined;
};


const char* ExecUnitToString(ExecUnit e);
const char* ExecUnitTypeToString(ExecUnitType e);

}   /* namespace MIAMI */


#endif
