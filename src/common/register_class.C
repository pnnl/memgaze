/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: register_class.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements a generic register representation.
 */

#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <sstream>
#include "register_class.h"
#include "InstructionDecoder.hpp"

namespace MIAMI
{

const register_info MIAMI_NO_REG;

void
register_info::SaveToFile(FILE* fd) const
{
   fwrite(&name, sizeof(name), 1, fd);
   fwrite(&type, sizeof(type), 1, fd);
   fwrite(&lsb, sizeof(lsb), 1, fd);
   fwrite(&msb, sizeof(msb), 1, fd);
   fwrite(&stack, sizeof(stack), 1, fd);
}

void
register_info::LoadFromFile(FILE* fd)
{
   fread(&name, sizeof(name), 1, fd);
   fread(&type, sizeof(type), 1, fd);
   fread(&lsb, sizeof(lsb), 1, fd);
   fread(&msb, sizeof(msb), 1, fd);
   fread(&stack, sizeof(stack), 1, fd);
}

const char*
RegisterClassToString(RegisterClass rc)
{
   switch (rc)
   {
      case RegisterClass_INVALID:         return "Invalid";
      case RegisterClass_REG_OP:          return "RegOp";
      case RegisterClass_MEM_OP:          return "MemOp";
      case RegisterClass_LEA_OP:          return "LEAOp";
      case RegisterClass_STACK_REG:       return "StackReg";
      case RegisterClass_STACK_OPERATION: return "StackOp";
      case RegisterClass_TEMP_REG:        return "TempReg";
      case RegisterClass_PSEUDO:          return "PseudoReg";
      default: assert(!"Invalid RegisterClass. Should not be here.");
   }
   return (NULL);
}

std::string 
register_info::ToString() const
{
   std::stringstream oss;
   oss << "Name ";
   if (type == RegisterClass_TEMP_REG)
      oss << "T" << name;
   else
      oss << register_name_to_string(name);
   oss << " {type " << RegisterClassToString(type) << "(" << type 
       << "), stack=" << stack << ", [" << lsb << "," << msb << "]}";
   return (oss.str());
}

std::string 
register_info::RegNameToString() const
{
   std::stringstream oss;
   if (type == RegisterClass_TEMP_REG)
      oss << "T" << name;
   else
      oss << register_name_to_string(name);
   return (oss.str());
}

std::ostream& operator<< (std::ostream& os, const register_info& ri)
{
   os << "Name ";
   if (ri.type == RegisterClass_TEMP_REG)
      os << "T" << ri.name;
   else
      os << register_name_to_string(ri.name);
   os << " {type " << RegisterClassToString(ri.type) << "("
      << ri.type << "), stack=" << ri.stack << ", [" << ri.lsb << "," << ri.msb << "]}";
   return (os);
}


}  /* namespace MIAMI */
