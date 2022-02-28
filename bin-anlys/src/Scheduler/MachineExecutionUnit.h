/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: MachineExecutionUnit.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements a data structure to store information about one type of machine 
 * execution units.
 */

#ifndef _MACHINE_EXECUTION_UNIT_H_
#define _MACHINE_EXECUTION_UNIT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "StringAssocTable.h"
#include "position.h"

namespace MIAMI
{

class MachineExecutionUnit
{
public:
   MachineExecutionUnit ()
   {
      count = 1;
      longname = NULL;
   }

   ~MachineExecutionUnit ()
   {
      if (longname)
         free (longname);
   }

   MachineExecutionUnit (int _count, char* _longname, Position& _pos)
   {
      count = _count;
      if (_longname == NULL)
         longname = _longname;
      else
         longname = strdup (_longname);
      position = _pos;
   }

   MachineExecutionUnit (const MachineExecutionUnit& meu)
   {
      count = meu.count;
      if (longname)
         free (longname);
      if (meu.longname)
         longname = strdup (meu.longname);
      else
         longname = NULL;
      position = meu.position;
   }

   MachineExecutionUnit& operator= (const MachineExecutionUnit& meu)
   {
      count = meu.count;
      if (longname)
         free (longname);
      if (meu.longname)
         longname = strdup (meu.longname);
      else
         longname = NULL;
      position = meu.position;
      return (*this);
   }
   
   void setNameAndPosition (const char* _longname, const Position& _pos)
   {
      if (longname==NULL && _longname!=NULL)
         longname = strdup (_longname);
      position = _pos;
   }

   int getCount () const                { return (count); }
   const char* getLongName () const     { return (longname); }
   const Position& getPosition () const { return (position); }
   
private:   
   int count;
   char* longname;
   Position position;
};

typedef StringAssocTable <MachineExecutionUnit> ExecUnitAssocTable;

} /* namespace MIAMI */

#endif
