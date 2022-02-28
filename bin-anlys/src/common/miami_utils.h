/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: miami_utils.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A few useful functions.
 */

#ifndef _MIAMI_UTILS_H
#define _MIAMI_UTILS_H

#include "miami_types.h"
#include <ctype.h>
#include <string>

namespace MIAMIU {
   using std::string;
   
   extern void ExtractNameAndPath(const string& input, string& path, string& name);
   extern std::string StripPath (const std::string&  path);
   extern const char * StripPath (const char * path);
   extern int CopyFile(const char *to, const char *from);

   /*!
    *  @brief Computes floor(log2(n))
    *  Works by finding position of MSB set.
    *  @returns -1 if n == 0.
    */
   inline uint32_t FloorLog2 (uint32_t n)
   {
       uint32_t p = 0;

       if (n == 0) return -1;

       if (n & 0xffff0000) { p += 16; n >>= 16; }
       if (n & 0x0000ff00) { p +=  8; n >>=  8; }
       if (n & 0x000000f0) { p +=  4; n >>=  4; }
       if (n & 0x0000000c) { p +=  2; n >>=  2; }
       if (n & 0x00000002) { p +=  1; }

       return p;
   }

   /*
    * Return the number of consecutive set bits in the
    * least significant position of n
    */
   inline uint32_t CountSetLSBits (MIAMI::addrtype n)
   {
       static int bit_counts[16] = {0, 1, 0, 2, 0, 1, 0, 3,
            0, 1, 0, 2, 0, 1, 0, 4};
       uint32_t c = 0, x = 0;
       do {
          x = n & 0xf; n >>= 4;
          c += bit_counts[x];
       } while (x==0xf);
       return (c);
   }
   
   /* Next two utilities are copied from the PIN utilities file util.PH */
   template <typename T> bool IsPowerOf2(T value)
   {
       return ((value & (value - 1)) == 0);
   }

   template <typename T> T RoundToNextPower2(T value)
   {
       //This algorithm rounds up to the next power of 2 by setting all the significant digits
       //of the number in binary representation to 1. The rest of the digits are set to be 0.
       //It increments the number, thus getting the next power of two. This handles all numbers
       //that aren't powers of two. To handle the case of powers of two, 1 is decremented from
       //the number before the process begins. This makes no difference if the number is not a
       //power of 2, and when it is it makes sure we get the same power of two we had before.
       //For details see "http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2"
       T width = sizeof(T)*8; //bit width
       --value;
       
       for (T i=1 ; i < width ; i <<= 1) //runs log2(width) times
       {
           value = (value | value >> i); //Turn i^2 bytes to 1.
       }
       ++value;
       return value;
   }

   /* return true if the string inludes only white space, false otherwise
    */
   inline int is_empty_string(const char *s) 
   {
      while (isspace((unsigned char)*s))
         ++ s;
      return (*s == '\0');
   }
}

#endif
