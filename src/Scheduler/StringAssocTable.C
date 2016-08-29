/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: StringAssocTable.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements an associative container with keys of type string.
 */

#include <assert.h>
#include "StringAssocTable.h"

using namespace MIAMI;
namespace MIAMI {
extern int haveErrors;
}

template<class ElemType> 
StringAssocTable<ElemType>::StringAssocTable (int _strict) : StringAssocMap()
{
   strictCheck = _strict;
   nextId = 0;
   finalized = false;
   elements.evector = new ElemVector();
}

template<class ElemType> 
StringAssocTable<ElemType>::~StringAssocTable()
{
   if (!finalized)
   {
      typename ElemVector::iterator meuit = elements.evector->begin();
      for( ; meuit!=elements.evector->end() ; ++meuit )
      {
         delete (*meuit);
      }
      elements.evector->clear();
      delete (elements.evector);
   } else
   {
      for (int i=0 ; i<nextId ; ++i)
         delete (elements.earray[i]);
      delete[] elements.earray;
   }
   
   StringAssocMap::iterator it = begin();
   for( ; it!=end() ; ++it )
      delete[] (it->first);
   
   clear();
}
   
template<class ElemType> int 
StringAssocTable<ElemType>::addElement (const char* _name, 
           ElemType *_meu, Position& _pos)
{
   if (finalized)
   {
      assert (!"StringAssocTable: Cannot add new elements after the configuration was finalized.");
      return (NO_MAPPING);
   }
   
   StringAssocMap::iterator it = find(_name);
   if (it == end() || !strictCheck)  // new name
   {
      int id = nextId++;
      if (it==end())
         insert(StringAssocMap::value_type(_name, id));
      else
         it->second = id;
      _meu->setNameAndPosition (_name, _pos);
      elements.evector->push_back (_meu);
      return (id);
   } else  // name is already in the table and strictCheck is set
   {
      haveErrors += 1;
      ElemType *meu = (*elements.evector)[it->second];
      fprintf(stderr, "Error %d (%d, %d): Identifier '%s' has been defined before at (%d,%d)\n",
             haveErrors, _pos.Line(), _pos.Column(), _name, 
             meu->getPosition().Line(), meu->getPosition().Column() );
      return (it->second);
   }
}
   
 
template<class ElemType> int 
StringAssocTable<ElemType>::getMappingForName(const char* _name, 
         Position& _pos)
{
   StringAssocMap::iterator it = find(_name);
   if (it == end())  // new name
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d (%d, %d): Identifier '%s' is used but was not defined.\n",
              haveErrors, _pos.Line(), _pos.Column(), _name);
      return (NO_MAPPING);
   } else  // name is already in the table
   {
      return (it->second);
   }
}

template<class ElemType> int 
StringAssocTable<ElemType>::findIndexForName(const char* _name)
{
   StringAssocMap::iterator it = find(_name);
   if (it == end())  // did not find
      return (NO_MAPPING);
   else  // found it
      return (it->second);
}


template<class ElemType> ElemType* 
StringAssocTable<ElemType>::getElementAtIndex (int idx) const
{
   if (idx >= nextId)
   {
      fprintf (stderr, "StringAssocTable: ERROR, received idx %d >= nextId %d\n List of keys: ",
            idx, nextId);
      StringAssocMap::const_iterator it = begin();
      for ( ; it!=end() ; ++it)
         fprintf (stderr, "%s; ", it->first);
      fprintf (stderr, "\n");
      assert (idx < nextId);
   }
   ElemType *meu = NULL;
   if (!finalized)
   {
      meu = (*elements.evector) [idx];
//      assert (!"Cannot interogate the database before the configuration was finalized.");
//      return (NULL);
   } else
      meu = elements.earray [idx];
   return (meu);
}

   
template<class ElemType> void 
StringAssocTable<ElemType>::FinalizeMapping ()
{
   if (finalized)
   {
      assert (!"Configuration was already finalized.");
      return;
   }
   if (nextId == 0)
   {
      assert (!"Configuration must contain at least one unit.");
      return;
   }
   
   ElemVector *tvector = elements.evector;
   elements.earray = new ElemType* [nextId];
   typename ElemVector::iterator vit = tvector->begin();
   int i = 0;
   for ( ; vit!=tvector->end() ; ++vit, ++i)
   {
      elements.earray[i] = *vit;
   }
   delete tvector;
   finalized = true;
}

