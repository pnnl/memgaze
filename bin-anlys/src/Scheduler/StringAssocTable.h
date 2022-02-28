/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: StringAssocTable.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements an associative container with keys of type string.
 */

#ifndef _STRING_ASSOC_TABLE_H
#define _STRING_ASSOC_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include "charless.h"
#include "position.h"

namespace MIAMI
{

#define NO_MAPPING (-1)

typedef std::map<const char*, int, CharLess> StringAssocMap;

template<class ElemType>
class StringAssocTable : public StringAssocMap
{
public:
   typedef std::vector<ElemType*> ElemVector;

public:
   StringAssocTable(int _strict = 1);
   ~StringAssocTable();
   
   // move information from a vector structure to an array for faster access
   void FinalizeMapping ();

   // the new method
   int addElement(const char* _name, ElemType *meu, Position& _pos);
   
   // get index associated with a name
   int getMappingForName(const char* _name, Position& _pos);

   // a quiet version of getMappingForName
   int findIndexForName(const char* _name);
   
   // get the element associated with an index
   ElemType* getElementAtIndex (int idx) const;

   // return number of classes of units
   int NumberOfElements() const { return (nextId); }
   
   void setStartPosition (Position &_pos) { position = _pos; }
   const Position& getStartPosition () const { return (position); }
   
private:
   int nextId;
   int strictCheck;
   bool finalized;
   union _elems {
      ElemVector* evector;
      ElemType** earray;
   } elements;
   Position position;
};

}  /* namespace MIAMI */

#endif
