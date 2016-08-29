/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: schedule_time.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Define data structure to identify a clock cycle in a modulo-schedule of 
 * a given length.
 */

#include "schedule_time.h"
#include <assert.h>

using namespace MIAMI;

// increment clock
ScheduleTime 
ScheduleTime::operator+ (int cycles) const
{ 
   if (cycles<0)
      return (operator- (-cycles));
   
   ScheduleTime temp(*this);
   temp.clk_cycle += cycles;
   while(temp.clk_cycle>=(int)scheduleLength)
   {
      temp.iteration += 1;
      temp.clk_cycle -= scheduleLength;
   }
   return (temp);
}
   
ScheduleTime& 
ScheduleTime::operator+= (int cycles)
{
   if (cycles<0)
      return (operator-= (-cycles));

   clk_cycle += cycles;
   // I think it is faster to compute the modulo arithmetic in this way,
   // than to use divide and modulo computations
   while(clk_cycle>=(int)scheduleLength)
   {
      iteration += 1;
      clk_cycle -= scheduleLength;
   }
   return (*this);
}

// decrement clock
ScheduleTime 
ScheduleTime::operator- (int cycles) const
{ 
   if (cycles<0)
      return (operator+ (-cycles));

   ScheduleTime temp(*this);
   temp.clk_cycle -= cycles;
   while(temp.clk_cycle<0)
   {
      temp.iteration -= 1;
      temp.clk_cycle += scheduleLength;
   }
   return (temp);
}

ScheduleTime& 
ScheduleTime::operator-= (int cycles)
{
   if (cycles<0)
      return (operator+= (-cycles));
     
   clk_cycle -= cycles;
   // I think it is faster to compute the modulo arithmetic in this way,
   // than to use divide and modulo computations
   while(clk_cycle<0)
   {
      iteration -= 1;
      clk_cycle += scheduleLength;
   }
   return (*this);
}

// increment iteration
ScheduleTime 
ScheduleTime::operator% (int iters) const
{ 
   ScheduleTime temp(*this);
   temp.iteration += iters;
   return (temp);
}
   
ScheduleTime& 
ScheduleTime::operator%= (int iters)
{
   iteration += iters;
   return (*this);
}
   
// compute difference of two scheduling times (in cycles)
int 
ScheduleTime::operator- (const ScheduleTime& st) const
{
   assert (scheduleLength == st.scheduleLength);
   int _cycles = (iteration-st.iteration)*scheduleLength +
             (clk_cycle - st.clk_cycle);
   return (_cycles);
}
   
// increment by one cycle
ScheduleTime& 
ScheduleTime::operator++ ()   // pre-increment
{ 
   clk_cycle+=1;
   if (clk_cycle>=(int)scheduleLength)
   {
      clk_cycle=0; iteration+=1;
   }
   return (*this); 
}

ScheduleTime& 
ScheduleTime::operator++ (int)  // post-increment
{
   ScheduleTime temp(*this);
   clk_cycle+=1;
   if (clk_cycle>=(int)scheduleLength)
   {
      clk_cycle=0; iteration+=1;
   }
   return (*this); 
}
   
// decrement by one cycle
ScheduleTime& 
ScheduleTime::operator-- ()   // pre-decrement
{
   clk_cycle-=1;
   if (clk_cycle<0)
   {
      clk_cycle=scheduleLength-1; iteration-=1;
   }
   return (*this); 
}

ScheduleTime& 
ScheduleTime::operator-- (int)  // post-decrement
{
   ScheduleTime temp(*this);
   clk_cycle-=1;
   if (clk_cycle<0)
   {
      clk_cycle=scheduleLength-1; iteration-=1;
   }
   return (*this); 
}
   
void
ScheduleTime::PrintObject (std::ostream &os) const
{
   os << "ScheduleTime (" << iteration << "," << clk_cycle << ")";
}
