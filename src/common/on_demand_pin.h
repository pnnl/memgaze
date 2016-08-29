/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: on_demand_pin.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Adds generic PIN tool support for stopping and resuming
 * profiling.
 */

#include <pin.H>

extern KNOB<BOOL> KnobInstrumentOnRequest;
extern KNOB<string> KnobStartCaliperName;
extern KNOB<string> KnobStopCaliperName;

namespace MIAMI
{
   extern int instrumentation_is_active;

   extern void MIAMI_OnDemandInit();
   extern void MIAMI_OnDemandFini();
   extern void MIAMI_OnDemandImgLoad(IMG img);
}
