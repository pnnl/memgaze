/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: report_time.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Simple time profiling API.
 */

#include <sys/resource.h>
#include <sys/time.h>
#include <stdarg.h>
#include "report_time.h"

namespace MIAMIP  /* MIAMI Profile */
{
  static struct rusage last_time;
  static long numCalls = 0;

void
report_time (FILE *fd, const char *format, ...)
{
   struct rusage crt_time;
   double secs_u, secs_s;
   va_list ap;
   va_start (ap, format);
   fprintf (fd, "ReportTime: ");
   vfprintf (fd, format, ap);
   va_end(ap);

   getrusage (RUSAGE_SELF, &crt_time);
   if (numCalls)
   {
      secs_u = (crt_time.ru_utime.tv_sec - last_time.ru_utime.tv_sec) +
         (crt_time.ru_utime.tv_usec - last_time.ru_utime.tv_usec) / 1000000.;
      secs_s = (crt_time.ru_stime.tv_sec - last_time.ru_stime.tv_sec) +
         (crt_time.ru_stime.tv_usec - last_time.ru_stime.tv_usec) / 1000000.;
   } else
   {
      secs_u = (crt_time.ru_utime.tv_sec) +
         (crt_time.ru_utime.tv_usec) / 1000000.;
      secs_s = (crt_time.ru_stime.tv_sec) +
         (crt_time.ru_stime.tv_usec) / 1000000.;
   }
   ++ numCalls;
   fprintf (fd, " in %lg sec (%lg user, %lg system)\n",
             secs_u+secs_s, secs_u, secs_s);
   last_time = crt_time;
}

}  /* namespace MIAMIP */
