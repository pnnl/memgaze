/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: generic_trio.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A generic trio data type with an ordering operator.
 */

#ifndef _GENERIC_TRIO_H
#define _GENERIC_TRIO_H

namespace MIAMIU
{

template <class T>
class GenericTrio
{
public:
   GenericTrio(T _first=0, T _second=0, T _third=0) :
        first(_first), second(_second), third(_third)
   {}
   
   GenericTrio(const GenericTrio& ut)
   {
      first=ut.first; second=ut.second; third=ut.third;
   }
   
   inline GenericTrio& operator= (const GenericTrio& ut)
   {
      first=ut.first; second=ut.second; third=ut.third;
      return (*this);
   }
   
   T first;
   T second;
   T third;

  class OrderTrio {
  public :
      int operator() (const GenericTrio& te1, const GenericTrio& te2) const
      {
         if (te1.first < te2.first)
            return true;
         if (te1.first > te2.first)
            return false;
      
         if (te1.second < te2.second)
            return true;
         if (te1.second > te2.second)
            return false;
         
         return (te1.third < te2.third);
      }
  };
};

}  /* namespace MIAMIU */

#endif
