/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: unionfind.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the union-find data structure with path 
 * compression.
 */

#ifndef _UNIONFIND_SET_H_
#define _UNIONFIND_SET_H_


class DisjointSet
{
public:
   DisjointSet(unsigned int size) : _size(size)
   {
      elm = new SetElement[_size];
      for (unsigned int i=0 ; i<_size ; ++i)
      {
         elm[i].iParent = i;
         elm[i].iRank = 0;
      }
   }
   
   ~DisjointSet()
   {
      delete[] elm;
   }
   
   inline int Find(int idx)
   {
      int parent = idx, k;
      // find the parent of the set
      while(elm[parent].iParent != parent)
         parent = elm[parent].iParent;
      
      // compress the path
      while( (k=elm[idx].iParent) != parent )
      {
         elm[idx].iParent = parent;
         idx = k;
      }
      return (parent);   
   }
   
   inline void Union(int idx1, int idx2)
   {
      if (elm[idx1].iRank > elm[idx2].iRank)
         elm[idx2].iParent = idx1;
      else
      {
         elm[idx1].iParent = idx2;
         if (elm[idx1].iRank == elm[idx2].iRank)
            elm[idx2].iRank ++;
      }
   }

private:
   class SetElement
   {
   public:
      int iRank;
      int iParent;
   };
   
   SetElement* elm;
   unsigned int _size;
};

#endif
