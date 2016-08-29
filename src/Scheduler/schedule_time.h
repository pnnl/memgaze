/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: schedule_time.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Define data structure to identify a clock cycle in a modulo-schedule of 
 * a given length.
 */

#ifndef _SCHEDULE_TIME_H
#define _SCHEDULE_TIME_H

#include "printable_class.h"

namespace MIAMI
{

class ScheduleTime : public PrintableClass
{
public:
   ScheduleTime(unsigned int _schedLen = 1) : scheduleLength(_schedLen)
   { 
      iteration=0; clk_cycle=0; 
   }
   ScheduleTime(const ScheduleTime& st)
   {
      scheduleLength = st.scheduleLength;
      iteration = st.iteration;
      clk_cycle = st.clk_cycle;
   }

   ScheduleTime& operator= (const ScheduleTime& st)
   {
      scheduleLength = st.scheduleLength;
      iteration = st.iteration;
      clk_cycle = st.clk_cycle;
      return (*this);
   }
   
   void setScheduleLength(unsigned int _slength)
   {
      scheduleLength = _slength;
   }
   
   void setTime(int _iter, int _cyc)
   {
      iteration=_iter; clk_cycle=_cyc;
   }
   
   void PrintObject (std::ostream &os) const;
   
   inline int Iteration() const { return (iteration); }
   inline int ClockCycle() const { return (clk_cycle); }

   // increment clock
   ScheduleTime operator+ (int cycles) const;
   ScheduleTime& operator+= (int cycles);
   // decrement clock
   ScheduleTime operator- (int cycles) const;
   ScheduleTime& operator-= (int cycles);

   // increment iteration
   ScheduleTime operator% (int iters) const;
   ScheduleTime& operator%= (int iters);
   
   // compute difference of two scheduling times (in cycles)
   int operator- (const ScheduleTime& st) const;
   
   // test if two times are equal, greater, less than
   bool operator== (const ScheduleTime& st) const
   {
      return (iteration==st.iteration && clk_cycle==st.clk_cycle);
   }
   
   bool operator!= (const ScheduleTime& st) const
   {
      return (iteration!=st.iteration || clk_cycle!=st.clk_cycle);
   }
   
   bool operator> (const ScheduleTime& st) const
   {
      return (iteration>st.iteration || 
           (iteration==st.iteration && clk_cycle>st.clk_cycle));
   }
   
   bool operator>= (const ScheduleTime& st) const
   {
      return (iteration>st.iteration || 
           (iteration==st.iteration && clk_cycle>=st.clk_cycle));
   }
   
   bool operator< (const ScheduleTime& st) const
   {
      return (iteration<st.iteration || 
           (iteration==st.iteration && clk_cycle<st.clk_cycle));
   }
   
   bool operator<= (const ScheduleTime& st) const
   {
      return (iteration<st.iteration || 
           (iteration==st.iteration && clk_cycle<=st.clk_cycle));
   }
   
   // increment by one cycle
   ScheduleTime& operator++ ();   // pre-increment
   ScheduleTime& operator++ (int);  // post-increment
   
   // decrement by one cycle
   ScheduleTime& operator-- ();   // pre-decrement
   ScheduleTime& operator-- (int);  // post-decrement
   
private:
   int iteration;
   int clk_cycle;
   // next field must be set to the length of the scheduling
   // it is used for modulo arithmetic when advancing cycles;
   unsigned int scheduleLength;
};

} /* namespace MIAMI */

#endif
