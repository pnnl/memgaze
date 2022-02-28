/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: block_mapper.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A two levels data structure that maps 64-bit addresses
 * to objects.
 */

#ifndef MIAMI_BLOCK_MAPPER_H
#define MIAMI_BLOCK_MAPPER_H

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <limits.h>

#include "miami_types.h"
#include "miami_utils.h"
#include "miami_allocator.h"
#include "bucket_hashmap.h"

using MIAMI::addrtype;

namespace MIAMIU
{

#define ENTRIES_2ND_LEVEL_MAPPER 64

template <class T, unsigned int VecLen>
class Level1Elem
{
public:
   T lev2[VecLen];
};

// mapping from int to ValType, KeyType must be either 4 byte or 8 byte integer type
template <class T, unsigned int lev2_entries = ENTRIES_2ND_LEVEL_MAPPER,
   class Allocator = MiamiAllocator <Level1Elem<T, lev2_entries>, 8> > 
class BlockMapper
{
private:
   typedef Level1Elem<T, lev2_entries> level_1_elem;
   typedef Allocator    alloc_type;
   static level_1_elem* defLev1ElemP;
   typedef BucketHashMap<addrtype, level_1_elem*, &defLev1ElemP> Level1Mapper;

public:
   BlockMapper(addrtype addr_mask = 0)
   {
      // I want to count the number of cosecutive 1 bits in the least 
      // significant position of the mask
      mask_bits = CountSetLSBits(addr_mask);
      
//      fprintf(stdout, "SizeOf(level_1_elem) = %ld, LSBits in mask (%" PRIxaddr ") = %d\n", 
//            sizeof(level_1_elem), addr_mask, mask_bits);
      if (!lev2_entries || !IsPowerOf2(lev2_entries))
      {
         fprintf(stderr, "Number of entries for level 2 mapper (%d) is not a power of 2. Fix it!\n",
               lev2_entries);
         exit(-3);
      }
      lev1Shift = FloorLog2(lev2_entries) + mask_bits;
      lev2Mask = lev2_entries - 1;
      
      mapper_allocator = new alloc_type(32*sizeof(level_1_elem)+16);
      bmapper = new Level1Mapper(512);
      
      lastLev1Key = (addrtype)(-1);
      lastLev1Value = 0;
   }
   
   virtual ~BlockMapper()
   {
      delete (mapper_allocator);
      delete (bmapper);
   }
   
   // Test performance using the pre_hash_vec method of the BucketHashTable
   inline T* operator[] (addrtype addr)
   {
      addrtype lev1Key = addr >> lev1Shift;
      addr >>= mask_bits;
      if (lev1Key == lastLev1Key)  // same block
         return (& lastLev1Value->lev2[addr & lev2Mask]);
      
      // not the previously accessed element, must go the long way
      level_1_elem* &elem1 = bmapper->insert(lev1Key);
      if (elem1 == 0)
      {
         elem1 = mapper_allocator->new_elem();
         new (elem1) level_1_elem ();
      }
      lastLev1Key = lev1Key;
      lastLev1Value = elem1;
      return (& elem1->lev2[addr & lev2Mask]);
   }
   
private:
   alloc_type   *mapper_allocator;
   Level1Mapper *bmapper;
   
   /* cache the last accessed entry into the hashtable */
   addrtype      lastLev1Key;
   level_1_elem* lastLev1Value;
   
   uint32_t lev1Shift;
   uint32_t lev2Mask;
   uint32_t mask_bits;
};

// initialize the static template member
template <typename T, unsigned int lev2_entries, typename Allocator> 
  Level1Elem<T, lev2_entries>* BlockMapper<T, lev2_entries, Allocator>::defLev1ElemP = NULL;

}  /* namespace MIAMIU */

#endif
