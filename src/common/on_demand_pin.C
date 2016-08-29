/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: on_demand_pin.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Adds generic PIN tool support for stopping and resuming
 * profiling.
 */

#include <stdio.h>
#include <fcntl.h>
#include "on_demand_pin.h"

KNOB<BOOL> KnobInstrumentOnRequest (KNOB_MODE_WRITEONCE,    "pintool",
    "q", "0", "instrument on request; use start/stop calipers to control instrumentation");
KNOB<string> KnobStartCaliperName (KNOB_MODE_WRITEONCE,    "pintool",
    "start", "miami_start_instrumentation", "specify name of caliper for starting/resuming instrumentation");
KNOB<string> KnobStopCaliperName (KNOB_MODE_WRITEONCE,    "pintool",
    "stop", "miami_stop_instrumentation", "specify name of caliper for stopping/pausing instrumentation");

int calipers_debug_level = 2;
#define DEBUG_CALIPERS  1
#define LOG_CALI(x, y)   if ((x)<=calipers_debug_level) {y}

namespace MIAMI
{
   int instrumentation_is_active = 1;
   static int invalidate_codecache = 1;

   typedef void (*on_demand_hook_funptr_t)(void);

   on_demand_hook_funptr_t real_start_hook, real_stop_hook; 

   static void MIAMI_OnDemandStart()
   {
      real_start_hook();
      
      instrumentation_is_active = 1;
      // method 1, just invalidate the code cache
      if (invalidate_codecache)
      {
         if (CODECACHE_FlushCache()) // success
         {
#if DEBUG_CALIPERS
            LOG_CALI(3,
               fprintf(stderr, "Successfully invalidated the entire code cache. Resuming instrumentation.\n");
            )
#endif
         }
         else
         {
#if DEBUG_CALIPERS
            LOG_CALI(2,
               fprintf(stderr, "Failed to invalidate the entire code cache while resuming instrumentation.\n");
            )
#endif
         }
      }
   }

   static void MIAMI_OnDemandStop()
   {
      instrumentation_is_active = 0;
      // method 1, just invalidate the code cache
      if (invalidate_codecache)
      {
         if (CODECACHE_FlushCache()) // success
         {
#if DEBUG_CALIPERS
            LOG_CALI(3,
               fprintf(stderr, "Successfully invalidated the entire code cache. Pausing instrumentation.\n");
            )
#endif
         }
         else
         {
#if DEBUG_CALIPERS
            LOG_CALI(2,
               fprintf(stderr, "Failed to invalidate the entire code cache while pausing instrumentation.\n");
            )
#endif
         }
      }
      
      real_stop_hook();
   }
   
   // Initialization function for the on demand hooks, called from the tool's Main
   void MIAMI_OnDemandInit()
   {
      if (KnobInstrumentOnRequest.Value()) { 
         // user requested on demand instrumentation
         // start instrumenting only when the start hook is executed
         instrumentation_is_active = 0;
      }
   }
   
   // Finalization function for the on demand hooks, called from the tool's Fini
   void MIAMI_OnDemandFini()
   {
      // Nothing to do here ... I think
   }
   
   // Hook called from the ImageLoad callback to setup the on demand hooks
   void MIAMI_OnDemandImgLoad(IMG img)
   {
      if (KnobInstrumentOnRequest.Value()) { 
         // user requested on demand instrumentation
         // search for the start and stop calipers
         RTN rtn = RTN_FindByName(img, KnobStartCaliperName.Value().c_str());
         if (RTN_Valid(rtn))
         {
#if DEBUG_CALIPERS
            LOG_CALI(1,
               fprintf(stderr, "Found START hook %s in %s\n",
                   KnobStartCaliperName.Value().c_str(), IMG_Name(img).c_str());
            )
#endif
            real_start_hook = (on_demand_hook_funptr_t)RTN_Replace(rtn, AFUNPTR(MIAMI_OnDemandStart));
         }

         rtn = RTN_FindByName(img, KnobStopCaliperName.Value().c_str());
         if (RTN_Valid(rtn))
         {
#if DEBUG_CALIPERS
            LOG_CALI(1,
               fprintf(stderr, "Found STOP hook %s in %s\n",
                   KnobStopCaliperName.Value().c_str(), IMG_Name(img).c_str());
            )
#endif
            real_stop_hook = (on_demand_hook_funptr_t)RTN_Replace(rtn, AFUNPTR(MIAMI_OnDemandStop));
         }

      }
   }

}
