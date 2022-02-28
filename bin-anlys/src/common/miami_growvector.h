/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: miami_growvector.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements a growing vector optimized for element additions
 * and access, and no support for removing elements.
 */

#ifndef _MIAMI_GROWING_VECTOR_H
#define _MIAMI_GROWING_VECTOR_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "miami_types.h"


namespace MIAMIU  /* MIAMI Utils */
{

// I need a fast growing vector that has only operations of push_back and
// fast access to each element. It does not have remove operations.
template <class ElemType, unsigned int def_chunk_size = 32>
class GrowingVector
{
public:
   typedef ElemType     elem_type;
   typedef MIAMI::addrtype ADDR;
   
   class ElementsIterator;
   friend class ElementsIterator;
   
   GrowingVector (unsigned int block_size = def_chunk_size)
   {
      if (block_size < 2)
         block_size = 2;

      chunk_size = block_size;
      size = num_chunks = 0;
      unused_capacity = lastIndex = 0;
      first_chunk = last_chunk = NULL;
      allocate_chunk ();
   }
   
   ~GrowingVector ()
   {
      ADDR* tmp_p;
      while(first_chunk != NULL)
      {
         tmp_p = first_chunk;
         first_chunk = (ADDR*)first_chunk[0];
         free(tmp_p);
      }
   }
   
   inline void push_back (const elem_type& elem)
   {
      if (!unused_capacity)   // I need a new chunk
         allocate_chunk ();
      elem_type *tempArray = (elem_type*)last_chunk;
      tempArray[++lastIndex] = elem;
      -- unused_capacity;
      ++ size;
   }

   inline elem_type& operator [] (int i)
   {
      assert ((i>=0 && i<size) ||
               !"GrowingVector: Received array index out of bounds.");
      // I must find the correct chunk first
      register unsigned int actual_chunk = chunk_size - 1; // first elem is link
      register int crt_max_idx = actual_chunk;
      ADDR *crt_chunk = first_chunk;
      while (crt_max_idx <= i)
      {
         crt_max_idx += actual_chunk;
         crt_chunk = (ADDR *)crt_chunk[0];
      }
      assert (crt_chunk != NULL);
      crt_max_idx = actual_chunk - (crt_max_idx - i);
      return ( *(((elem_type*)crt_chunk) [crt_max_idx + 1]) );
   }
   
   // create also an iterator; often I will need to traverse all elements of 
   // the array, and it is cheaper to use an iterator than to index each
   // position;
   class ElementsIterator
   {
   public:
      ElementsIterator (GrowingVector<ElemType> &_vec) 
      { 
         vec = &_vec; crt_chunk = vec->first_chunk; crt_index = 1;
      }
      ~ElementsIterator () { }
      
      inline void Reset ()
      {
         crt_chunk = vec->first_chunk; crt_index = 1;
      }
      
      inline void operator ++ ()
      {
         if (crt_chunk == NULL)  // nothing to do
            return;
         ++ crt_index;
         if (crt_index >= vec->chunk_size)   // go to next chunk
         {
            crt_chunk = (ADDR*)(((ADDR*)crt_chunk)[0]);
            crt_index = 1;
         }
      }
      
      inline operator bool () 
      { 
         return (crt_chunk!=NULL && 
                 ( (crt_chunk==vec->last_chunk && crt_index<=vec->lastIndex) ||
                   (crt_chunk!=vec->last_chunk && crt_index<vec->chunk_size)
                 )); 
      }
      inline ElemType* operator-> () { 
         return &(((ElemType*)crt_chunk)[crt_index]); 
      }
      inline operator ElemType* () { 
         return &(((ElemType*)crt_chunk)[crt_index]); 
      }
      
   private:
      GrowingVector<ElemType> *vec;
      ADDR *crt_chunk;
      int crt_index;
   };
   
private:
   inline void allocate_chunk ()
   {
      void *new_chunk = memalign (sizeof(elem_type), 
                        chunk_size * sizeof(elem_type));
      // first element of each chunk is used to store a pointer to the next
      // chunk
//      fprintf (stdout, "growvector::allocate_chunk -> allocated new chunk\n");
      if (!num_chunks)  // if first chunk
      {
         first_chunk = last_chunk = (ADDR*)new_chunk;
      } else
      {
         ((ADDR*)last_chunk)[0] = (ADDR)new_chunk;
         last_chunk = (ADDR*)new_chunk;
      }
      ((ADDR*)last_chunk)[0] = NULL;
      unused_capacity = chunk_size - 1;
      lastIndex = 0;
      ++ num_chunks;
   }

   ADDR *first_chunk, *last_chunk;
   unsigned int chunk_size;
   unsigned int size;
   unsigned int num_chunks;
   unsigned int unused_capacity;
   unsigned int lastIndex;
};

}  /* namespace MIAMIU */

#endif
