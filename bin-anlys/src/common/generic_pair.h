/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: generic_pair.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A generic pair data type with various ordering operators.
 */

#ifndef _GENERIC_PAIR_H
#define _GENERIC_PAIR_H

#ifdef __GNUC__
#pragma interface
#endif

#include <stdio.h>

namespace MIAMIU
{

template<class Type1, class Type2>
class GenericPair
{
public:
  GenericPair (Type1 i1 = 0, Type2 i2 = 0) 
  { 
     first = i1;
     second = i2;
  }
  
  GenericPair (const GenericPair& uip)
  {
     first = uip.first; second = uip.second;
  }
  
  inline GenericPair& operator= (const GenericPair& uip)
  {
     first = uip.first; second = uip.second;
     return (*this);
  }
  
  inline void dump (FILE* fd)
  {
     fwrite(&first, sizeof(Type1), 1, fd);
     fwrite(&second, sizeof(Type2), 1, fd);
  }

  Type1 first;
  Type2 second;

  class OrderPairs {
  public :
      int operator() (const GenericPair &x, const GenericPair &y) const
      {
        if (x.first != y.first)  
           return (x.first < y.first);
        else
           // if 'first' elements are equal, compare 'second' ones
           return (x.second < y.second);
      }
  };

  class OrderAscByKey1 {
  public :
      bool operator() (const GenericPair &x, const GenericPair &y) const
      {
        return (x.first < y.first);
      }
  };

  class OrderDescByKey1 {
  public :
      bool operator() (const GenericPair &x, const GenericPair &y) const
      {
        return (x.first > y.first);
      }
  };

  class OrderAscByKey2 {
  public :
      bool operator() (const GenericPair &x, const GenericPair &y) const
      {
        return (x.second < y.second);
      }
  };
  
  class OrderDescByKey2 // : public std::binary_function<int,int,bool>
  {
  public :
      bool operator() (const GenericPair &x, const GenericPair &y) const
      {
        return (x.second > y.second);
      }
  };

};

}  /* namespace MIAMIU */

#endif
