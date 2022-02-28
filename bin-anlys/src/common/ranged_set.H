/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: ranged_set.H
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Built on top of STL set. A data structure for aggregating
 * data with closby values. Bins correspond to ranges of values and the 
 * bins boundaries are determined dynamically.
 */

#ifndef _RANGED_SET_H
#define _RANGED_SET_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <set>

#ifndef RANGE_THRESHOLD
#define RANGE_THRESHOLD 0.06
#endif
const double minFactor = 1.0 - RANGE_THRESHOLD;
const double maxFactor = 1.0/(1.0 - RANGE_THRESHOLD);

class ValueRange
{
public:
   double min, max;
   double cmpmin, cmpmax;
   double total;
   unsigned int count;

   ValueRange ()
   {
      min = max = 0.0;
      cmpmin = cmpmax = 0.0;
      total = 0;
      count = 0;
   }

   ValueRange (double _val, unsigned int _cnt = 1)
   {
      min = max = _val;
      cmpmin = max*minFactor;
      cmpmax = min*maxFactor;
      if (_cnt == 1) {
         total = _val;
         count = 1;
      } else
      {
         total = _val * _cnt;
         count = _cnt;
      }
   }

   void addValue (double _val, unsigned int _cnt = 1)
   {
      if (_val<min || count==0) 
      {
         min = _val;
         cmpmax = min*maxFactor;
      }
      if (_val>max || count==0) 
      {
         max = _val;
         cmpmin = max*minFactor;
      }
      if (_cnt == 1) {
         total += _val;
         count += 1;
      } else
      {
         total += _val * _cnt;
         count += _cnt;
      }
   }

   void addRange (const ValueRange &vr)
   {
      if (vr.min<min || count==0)
      {
         min = vr.min;
         cmpmax = min*maxFactor;
      }
      if (vr.max>max || count==0) 
      {
         max = vr.max;
         cmpmin = max*minFactor;
      }
      total += vr.total;
      count += vr.count;
   }
   
   void print (FILE *fd) const
   {
      fprintf (fd, "VR [%g,%g], cmprange=[%g,%g], total=%g, count=%d, avgVal=%g\n",
         min, max, cmpmin, cmpmax, total, count, total/count);
   }
};

class CompareValueRanges
{
public:
   bool operator () (const ValueRange &vr1, const ValueRange &vr2) const
   {
      return (vr1.min < vr2.cmpmin);
   }
};

typedef std::multiset<ValueRange, CompareValueRanges> VRMSet;

class RangedSet : public VRMSet
{
   unsigned long numValues;
   
public:
   RangedSet ()
   {
      numValues = 0;
   }
   
   iterator insert (double val, unsigned int count = 1)
   {
      iterator vit;
      int i;
      ValueRange tmpVR (val, count);
      std::pair<iterator, iterator> eqit = equal_range (tmpVR);
      if (eqit.first == eqit.second)
      {
//         printf ("--> No match found, insert new VR.\n");
         vit = VRMSet::insert (tmpVR);
      } else
      {
//         printf ("--> Found the following matching VRs:\n");
         vit = eqit.first;
         double minRange = val;
         iterator fit = vit;
         for (i=0 ; vit!=eqit.second ; ++vit, ++i)
         {
//            vit->print (stdout);
            double dist;
            if (vit->min<=val && val<=vit->max)
               dist = 0.0;
            else if (val<vit->min)
            {
//               assert (val >= vit->cmpmin);
               dist = vit->max-val;
            } else
            {
//               assert (val <= vit->cmpmax);
//               assert (vit->min >= tmpVR.cmpmin);
               dist = val-vit->min;
            }
            if (dist < minRange)
            {
               minRange = dist;
               fit = vit;
            }
         }

         assert (minRange<val || val==0.0);
//         printf ("   Add %g to range ", val);
//         fit->print (stdout);
         tmpVR.addRange (*fit);
         erase (fit);
         vit = VRMSet::insert (tmpVR);
//         fit->addValue (val);
      }
      return (vit);
   }
   
   void print (FILE *fd)
   {
      int i=1;
      for (iterator vit=begin() ; vit!=end() ; ++vit, ++i)
      {
         fprintf (fd, "%d) ", i);
         vit->print (fd);
      }
   }
};

#endif
