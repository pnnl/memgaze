/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: compute_CRC.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Functions to compute CRC32 and CRC64 checksums.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <string.h>
#include "compute_CRC.h"

//
// Local functions:
//

namespace MIAMIU
{

/* The CRC32 code is copyright @ 1993 Richard Black. 
 * All rights are reserved. You may use this code only
 * if it includes a statement to that effect.
 */
#define QUOTIENT 0x04c11db7
static uint32_t crctab32[256];
static bool crc32_initialised = false;

static uint64_t crctab64[256];
static bool crc64_initialised = false;

static void crc32_init(void)
{
    uint32_t i,j;
    uint32_t crc;
    
    if (crc_initialised) return;

    for (i = 0; i < 256; i++)
    {
        crc = i << 24;
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ QUOTIENT;
            else
                crc = crc << 1;
        }
        crctab32[i] = htonl(crc);
    }
    crc_initialised = true;
}

static void crc64_init(void)
{
    uint64_t i,j;
    uint64_t crc;
    
    if (crc_initialised) return;

    for (i = 0; i < 256; i++)
    {
        crc = i << 24;
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ QUOTIENT;
            else
                crc = crc << 1;
        }
        crctab64[i] = htonl(crc);
    }
    crc_initialised = true;
}


unsigned int
compute_CRC_for_file (FILE *fd, unsigned int seed_value)
{
  crc32_init();

  unsigned int result = seed_value;

  unsigned int temp_buf[1024];
  int l;
  while ( (l=fread( (char*)temp_buf, sizeof(unsigned int), 1024, fd) ) > 0)
  {
     for( int i=0 ; i<l ; i++ )
     {
#if defined(LITTLE_ENDIAN)
        result = crctab[result & 0xff] ^ (result >> 8);
        result = crctab[result & 0xff] ^ (result >> 8);
        result = crctab[result & 0xff] ^ (result >> 8);
        result = crctab[result & 0xff] ^ (result >> 8);
        result ^= temp_buf[i];
#else
        result = crctab[result >> 24] ^ result << 8;
        result = crctab[result >> 24] ^ result << 8;
        result = crctab[result >> 24] ^ result << 8;
        result = crctab[result >> 24] ^ result << 8;
        result ^= temp_buf[i];
#endif
     }
  }
  return (~result);
}

unsigned int 
compute_CRC_for_string (const char *_str, unsigned int seed_value)
{
   crc32_init();

   unsigned int result = seed_value;
   int len = strlen (_str);

   for( int i=0 ; i<len ; ++i )
   {
#if defined(LITTLE_ENDIAN)
      result = crctab[(result & 0xff) ^ _str[i]] ^ (result >> 8);
#else
      result = crctab[(result >> 24) ^ _str[i]] ^ result << 8;
#endif
   }
   return (~result);
}

}  /* namespace MIAMIU */
