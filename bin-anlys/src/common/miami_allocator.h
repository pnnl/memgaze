/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: miami_allocator.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Custom memory allocator for the MIAMI containers.
 */

#ifndef _MIAMI_ALLOCATOR_H
#define _MIAMI_ALLOCATOR_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <malloc.h>

#include "miami_types.h"

namespace MIAMIU  /* MIAMI Utils */
{
#ifdef ADDR
# undefine ADDR
#endif
typedef MIAMI::addrtype ADDR;

#define DEFAULT_CHUNK_SIZE   16384

// simple_allocator improves over malloc/free by allocating the memory in 
// chunks of a specified number of ElemType objects; 
template<class ElemType, int _alignment=8>
class MiamiAllocator
{
public:
   MiamiAllocator(unsigned int _b_size = DEFAULT_CHUNK_SIZE) : 
        chunk_capacity(_b_size), alignment(_alignment)
   {
      if (alignment <= 0 || (alignment&0x3))
      {
         fprintf(stderr, "MiamiAllocator: invalid alignment specified (%d); valid values are strictly positive multiple of 4 integers.\n",
             alignment);
         exit(-1);
      }
      if (chunk_capacity < 4*sizeof(ElemType) || chunk_capacity < 4*(unsigned)alignment
               || chunk_capacity < 32)
      {
         fprintf(stderr, "MiamiAllocator: specified block size is too small (%" PRIaddr "); block size must be larger than the maximum of (%ld,%d,%d)\n",
               chunk_capacity, 4*sizeof(ElemType), 4*alignment, 32);
         exit(-1);
      }
      first_chunk = (ADDR*)memalign(sizeof(ADDR), chunk_capacity);
#ifdef DEBUG_ALLOCATOR
      printf ("MiamiAllocator, allocated first chunk\n");
#endif
      if (first_chunk == NULL)
      {
         fprintf(stderr, "Out of memory in MiamiAllocator when trying to allocate %" PRIaddr " bytes. Abort.\n",
              chunk_capacity);
         fflush(stderr);
         exit(-1);
      }
      aligned_elem_size = ((sizeof(ElemType) + alignment - 1) / alignment) * alignment;
      first_chunk[0] = 0;  // first word in each chunk is a pointer to the 
                           // next chunk;
      // construct a list of available element blocks
      next_address = (ADDR*)((((ADDR)first_chunk + sizeof(ADDR) 
                      + alignment - 1) / alignment) * alignment);
      assert(next_address>=first_chunk);
      available_size = chunk_capacity - (ADDR)next_address 
                       + (ADDR)first_chunk;
      free_list = 0;
   }
   
   ~MiamiAllocator()
   {
      ADDR* tmp_p;
      while(first_chunk != NULL)
      {
         tmp_p = first_chunk;
         first_chunk = (ADDR*)first_chunk[0];
         free(tmp_p);
      }
   }
   
   ElemType* new_elem()  //ElemType* p = NULL)
   {
      ElemType* aux;
      if (free_list)
      {
         aux = (ElemType*)free_list;
         free_list = (ADDR*)free_list[0];
      } else
      {
         if (available_size < (ADDR)sizeof(ElemType))
         // this chunk is full; allocate another one
         {
            ADDR *temp = first_chunk;
            first_chunk = (ADDR*)memalign(sizeof(ADDR), chunk_capacity);

#ifdef DEBUG_ALLOCATOR
            printf ("MiamiAllocator::new_elem, allocated new chunk\n");
#endif
            if (first_chunk == NULL)
            {
               fprintf(stderr, "Out of memory in MiamiAllocator when trying to allocate %" PRIaddr " bytes. Abort.\n",
                    chunk_capacity );
               fflush(stderr);
               exit(-1);
            }
            first_chunk[0] = (ADDR)temp;
            next_address = (ADDR*)((((ADDR)first_chunk + sizeof(ADDR) 
                           + alignment - 1) / alignment) * alignment);
            assert(next_address>=first_chunk);
            available_size = chunk_capacity - (ADDR)next_address 
                             + (ADDR)first_chunk;
         }
         aux = (ElemType*)next_address;
         next_address = (ADDR*)((ADDR)aux + aligned_elem_size);
         available_size = available_size - aligned_elem_size;
      }
      // aux points to a valid block of memory of size at least 
      // the necessary amount
//      new ((void*)aux) ElemType;
      return (aux);
   }
   
   void delete_elem(ElemType* elem)
   {  // I can only hope that elem points to a valid memory block of correct 
      // size; even better if it was obtained by calling the new_elem method
      *((ADDR*)elem) = (ADDR)free_list;
      free_list = (ADDR*)elem;
   }
   
private:
   ADDR available_size;
   ADDR aligned_elem_size;
   ADDR chunk_capacity;
   int alignment;
   ADDR* free_list;
   ADDR* next_address;
   ADDR* first_chunk;
};

} /* namespace MIAMIU */

#endif
