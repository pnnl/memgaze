/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: compute_CRC.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Functions to compute CRC32 and CRC64 checksums.
 */

#ifndef _COMPUTE_CRC_H
#define _COMPUTE_CRC_H

#include <stdio.h>
#include "miami_types.h"

namespace MIAMIU  /* MIAMI Utils */
{

uint32_t compute_CRC32_for_file (FILE *fd, uint32_t seed_value);
uint32_t compute_CRC32_for_string (const char *_str, 
               uint32_t seed_value);

uint64_t compute_CRC64_for_file (FILE *fd, uint64_t seed_value);
uint64_t compute_CRC64_for_string (const char *_str, 
               uint64_t seed_value);
}

#endif
