/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: load_module.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data bookkeeping at the load module level for a stream simulation tool.
 */

#include <stdio.h>
#include "load_module.h"
#include "stream_reuse.h"

using namespace MIAMI;
using namespace std;

namespace MIAMI
{
   int32_t defInt32 = 0;
}

static int
placeKeysInArray (void* arg0, uint64_t key, void* value)
{
   uint64_t *keyArray = static_cast<uint64_t*>(arg0);
   int32_t *v = static_cast<int32_t*>(value);
   keyArray[*v] = key;
   return (0);
}

LoadModule::~LoadModule()
{
   if (StreamRD::verbose_level>0)
   {
      fprintf(stderr, "Unloading image %s\n", img_name.c_str());
      instMapper.print_stats(stderr);
   }
   
   if (reverseInstIndex)
      delete[] reverseInstIndex;
}

addrtype 
LoadModule::getAddressForInstIndex(int32_t iindex, int32_t& memop)
{
   if (reverseInstIndex && savedInstIndex!=nextInstIndex)
   {
      delete[] reverseInstIndex;
      reverseInstIndex = 0;
   }
   if (! reverseInstIndex)
   {
      reverseInstIndex = new uint64_t[nextInstIndex];
      savedInstIndex = nextInstIndex;
      instMapper.map(placeKeysInArray, reverseInstIndex);
   }
   memop = reverseInstIndex[iindex] & 0x3;
   // return the offset into the image for now
   return ((addrtype)(reverseInstIndex[iindex]>>2)); // + base_addr + low_addr_offset);
}

int32_t 
LoadModule::getInstIndex(addrtype iaddr, int memop)
{
   uint64_t key = ((iaddr-base_addr-low_addr_offset)<<2) + memop;
   int32_t &index = instMapper[key];
   if (index == 0)
   {
      index = nextInstIndex;
      nextInstIndex += 1;
   }  // new instruction index needed
   return (index);
}

void 
LoadModule::saveToFile(FILE *fd) const
{
   // save the image basic info only
   PrivateLoadModule::saveToFile(fd);
}

