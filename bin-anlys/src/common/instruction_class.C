/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: instruction_class.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Data type to encapsulate the parameters that uniquely define 
 * a MIAMI instruction type.
 */

#include <stdio.h>
#include "instruction_class.h"
#include <iostream>
#include <sstream>

namespace MIAMI {

const char*
ExecUnitToString(ExecUnit e)
{
    switch (e) {
    case ExecUnit_VECTOR:
        return "vector";
    case ExecUnit_SCALAR:
        return "scalar";
    default:
        return ("unknown ExecUnit");
    }
}

    
const char*
ExecUnitTypeToString(ExecUnitType e)
{
    switch (e) {
    case ExecUnitType_FP:
        return "fp";
    case ExecUnitType_INT:
        return "int";
    default:
        return ("unknown ExecUnitType");
    }
}

ICIMap& operator+= (ICIMap& f, const ICIMap& s)
{
   ICIMap::const_iterator sit = s.begin();
   for ( ; sit!=s.end() ; ++sit)
   {
      ICIMap::iterator fit = f.find(sit->first);
      if (fit == f.end())
         f.insert(*sit);
      else
         fit->second += sit->second;
   }
   return (f);
}

std::string
InstructionClass::ToString() const
{
   std::ostringstream oss;
   oss << Convert_InstrBin_to_string(type)
       << ":" << ExecUnitTypeToString(eu_type) << width;
   if (vec_width != width)
      oss << ":vec[" << vec_width << "]";
   return (oss.str());
}

}

