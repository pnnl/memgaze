/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: report_time.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Simple time profiling API.
 */

#ifndef REPORT_TIME_H
#define REPORT_TIME_H

#include <stdio.h>

namespace MIAMIP
{
extern void report_time (FILE *fd, const char *format, ...);
}

#endif
