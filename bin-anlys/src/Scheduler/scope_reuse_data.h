/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: scope_reuse_data.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Handle data asociated with a data reuse pattern.
 */

#ifndef _SCOPE_REUSE_DATA_H
#define _SCOPE_REUSE_DATA_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "miami_types.h"
#include "scope_implementation.h"

namespace MIAMI
{

class CarryName
{
public:
   CarryName (uint64_t _carried, const char* _name) :
        carryId(_carried), name(_name)
   { }
   
   uint64_t carryId;
   const char* name;
};

class CarryNameLess
{
public:
   bool operator() (const CarryName& cn1, const CarryName& cn2) const
   {
      if (cn1.carryId != cn2.carryId)
         return (cn1.carryId < cn2.carryId);
      else
         if (!cn1.name || !cn2.name)
            return (cn1.name < cn2.name);
         else
            return (strcmp(cn1.name, cn2.name) < 0);
   }
};

typedef std::map <CarryName, double*, CarryNameLess> CNameDoublePMap;

class ScopeReuseData
{
public:
   ScopeReuseData (int _numLevels)  
   { 
      numLevels = _numLevels;
      numLevels2 = numLevels + numLevels;
      misses = 0; 
      numLevels3 = numLevels2 + numLevels;
   }
   ~ScopeReuseData () {
      if (misses)
         delete[] misses;
      
      // I need to deallocate the values map because now I allocate 
      // memory for those values;
      CNameDoublePMap::iterator cit = values.begin ();
      for ( ; cit!=values.end() ; ++cit)
         delete[] cit->second;
      values.clear();
   }
   
#if 0   // This is the old version where I add all the counts for a reuse pattern at once
   void addDataForCarrier (unsigned int _scopeId, const char* set_name, 
                 double *_values)
   {
      int i;
      double fragFactor = _values[numLevels+1];
      bool is_irreg = (_values[numLevels] > 0.0);
      bool is_frag = (fragFactor > 0.0);
      CarryName tempPair (_scopeId, set_name);
      CNameDoublePMap::iterator dit = values.find (tempPair);
      if (dit == values.end())
      {
         double *vals = new double[numLevels3];
         values.insert (CNameDoublePMap::value_type (tempPair, vals));
         for (i=0 ; i<numLevels ; ++i)
         {
            vals[i] = _values[i];
            if (is_irreg)
               vals[i+numLevels] = _values[i];
            else
               vals[i+numLevels] = 0.0;
            if (is_frag && _values[i]>0.0)
               vals[i+numLevels2] = _values[i]*fragFactor;
            else
               vals[i+numLevels2] = 0.0;
         }
      } else
         for (i=0 ; i<numLevels ; ++i)
         {
            dit->second[i] += _values[i];
            if (is_irreg)
               dit->second[i+numLevels] += _values[i];
            if (is_frag && _values[i]>0.0)
               dit->second[i+numLevels2] += _values[i]*fragFactor;
         }

      // I must add these counts to the totals by name
      tempPair = CarryName (0, set_name);
      dit = values.find (tempPair);
      if (dit == values.end())
      {
         double *vals = new double[numLevels3];
         values.insert (CNameDoublePMap::value_type (tempPair, vals));
         for (i=0 ; i<numLevels ; ++i)
         {
            vals[i] = _values[i];
            if (is_irreg)
               vals[i+numLevels] = _values[i];
            else
               vals[i+numLevels] = 0.0;
            if (is_frag && _values[i]>0.0)
               vals[i+numLevels2] = _values[i]*fragFactor;
            else
               vals[i+numLevels2] = 0.0;
         }
      } else
         for (i=0 ; i<numLevels ; ++i)
         {
            dit->second[i] += _values[i];
            if (is_irreg)
               dit->second[i+numLevels] += _values[i];
            if (is_frag && _values[i]>0.0)
               dit->second[i+numLevels2] += _values[i]*fragFactor;
         }
      
      // I must add these counts to the totals by carrier
      if (set_name)
      {
         tempPair = CarryName (_scopeId, 0);
         dit = values.find (tempPair);
         if (dit == values.end())
         {
            double *vals = new double[numLevels3];
            values.insert (CNameDoublePMap::value_type (tempPair, vals));
            for (i=0 ; i<numLevels ; ++i)
            {
               vals[i] = _values[i];
               if (is_irreg)
                  vals[i+numLevels] = _values[i];
               else
                  vals[i+numLevels] = 0.0;
               if (is_frag && _values[i]>0.0)
                  vals[i+numLevels2] = _values[i]*fragFactor;
               else
                  vals[i+numLevels2] = 0.0;
            }
         } else
            for (i=0 ; i<numLevels ; ++i)
            {
               dit->second[i] += _values[i];
               if (is_irreg)
                  dit->second[i+numLevels] += _values[i];
               if (is_frag && _values[i]>0.0)
                  dit->second[i+numLevels2] += _values[i]*fragFactor;
            }
      }
      
      // I must also add these values to the overall counts.
      if (! misses)
      {
         misses = new double [numLevels3];
         for (i=0 ; i<numLevels ; ++i)
         {
            misses[i] = _values [i];
            if (is_irreg)
               misses[i+numLevels] = _values[i];
            else
               misses[i+numLevels] = 0.0;
            if (is_frag && _values[i]>0.0)
               misses[i+numLevels2] = _values[i]*fragFactor;
            else
               misses[i+numLevels2] = 0.0;
         }
      } else
         for (i=0 ; i<numLevels ; ++i)
         {
            misses [i] += _values [i];
            if (is_irreg)
               misses[i+numLevels] += _values[i];
            if (is_frag && _values[i]>0.0)
               misses[i+numLevels2] += _values[i]*fragFactor;
         }
   }
#endif

   void addDataForCarrier (uint64_t _scopeId, const char* set_name, int level,
            double missCount, double fragF, bool is_irreg)
   {
      int i;
      bool is_frag = (fragF > 0.0);
      CarryName tempPair (_scopeId, set_name);
      CNameDoublePMap::iterator dit = values.find (tempPair);
      double *vals = 0;
      if (dit == values.end())
      {
         vals = new double[numLevels3];
         values.insert (CNameDoublePMap::value_type (tempPair, vals));
         for (i=0 ; i<numLevels3 ; ++i)
            vals[i] = 0.0;
      } else
         vals = dit->second;
      
      vals[level] += missCount;
      if (is_irreg)
         vals[level+numLevels] += missCount;
      if (is_frag)
         vals[level+numLevels2] += missCount*fragF;

      // I must add these counts to the totals by name
      tempPair = CarryName (0, set_name);
      dit = values.find (tempPair);
      if (dit == values.end())
      {
         vals = new double[numLevels3];
         values.insert (CNameDoublePMap::value_type (tempPair, vals));
         for (i=0 ; i<numLevels3 ; ++i)
            vals[i] = 0.0;
      } else
         vals = dit->second;
         
      vals[level] += missCount;
      if (is_irreg)
         vals[level+numLevels] += missCount;
      if (is_frag)
         vals[level+numLevels2] += missCount*fragF;

      // I must add these counts to the totals by carrier
      if (set_name)
      {
         tempPair = CarryName (_scopeId, 0);
         dit = values.find (tempPair);
         if (dit == values.end())
         {
            vals = new double[numLevels3];
            values.insert (CNameDoublePMap::value_type (tempPair, vals));
            for (i=0 ; i<numLevels3 ; ++i)
               vals[i] = 0.0;
         } else
            vals = dit->second;

         vals[level] += missCount;
         if (is_irreg)
            vals[level+numLevels] += missCount;
         if (is_frag)
            vals[level+numLevels2] += missCount*fragF;
      }
      
      // I must also add these values to the overall counts.
      if (! misses)
      {
         misses = new double [numLevels3];
         for (i=0 ; i<numLevels ; ++i)
            misses[i] = 0.0;
      }
      misses[level] += missCount;
      if (is_irreg)
         misses[level+numLevels] += missCount;
      if (is_frag)
         misses[level+numLevels2] += missCount*fragF;
   }


   CNameDoublePMap values;
   double* misses;
   int numLevels;
   int numLevels2;
   int numLevels3;
};

// The key of this map is a Pair of 64-bit integers, representing the source and
// destination of a reuse pattern. For each source/dest pair, I store a map of
// misses organized by carrier scope and source code array touched at destination.
typedef std::map <MIAMI::Pair64, ScopeReuseData*, MIAMI::Pair64::OrderPairs> PairSRDMap;

// In the new MIAMI, I want to also show the path from the carrier scope to the
// source and the destination of a reuse pattern. In this case, it makes more sense to
// display each reuse pattern individually. Perhaps, I can just use the carriedMaps
// stored with each program scope. The carried map stores information about the
// source and the destination of a reuse (what about the names??). I just need to 
// expand the flat source and destination IDs into the full dynamic context sub-tree
// rooted at the carrier scope.

}  /* namespace MIAMI */

#endif
