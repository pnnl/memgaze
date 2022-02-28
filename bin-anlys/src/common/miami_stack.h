/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: miami_stack.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements a growing stack data structure.
 */

#ifndef _MIAMI_GROWING_STACK_H
#define _MIAMI_GROWING_STACK_H

#include <new>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "miami_types.h"


namespace MIAMIU  /* MIAMI Utils */
{

using MIAMI::addrtype;

// I need a fast growing stack that has only operations of push, pop, top and
// a reversed iterator (from top to base). The iterator is missing from the STL
// stack implementation, so I wrote my own.
// This class manages memory internally. It is not using the standard MiamiAllocator.
template <class ElemType, unsigned int def_chunk_size = 32>
class GrowingStack
{
public:
   typedef ElemType     elem_type;
   
   class ElementsIterator;
   friend class ElementsIterator;
   
   GrowingStack (unsigned int block_size = def_chunk_size)
   {
      if (block_size < 2)
         block_size = 2;
      
      elems_in_chunk = block_size;
      topIndex = 0;
      top_chunk = free_chunk = NULL;
      // elem_type size should be a multiple of addrtype?
      chunk_size = block_size * sizeof(elem_type);
      // make sure the end of the chunk_size is aligned by addrtype
      chunk_size = ((chunk_size + sizeof(addrtype) - 1) / sizeof(addrtype)) * sizeof(addrtype);
      // add 2 addrtypes at the end, to use as links to previous and next chunks
      pointer_idx = chunk_size / sizeof(addrtype);
      chunk_size += 2*sizeof(addrtype);
      allocate_chunk();
   }
   
   ~GrowingStack ()
   {
      addrtype* tmp_p;
      int i;
      while (top_chunk != NULL)
      {
         tmp_p = top_chunk;
         top_chunk = (addrtype*)(top_chunk[pointer_idx]);
         elem_type *tmpArray = (elem_type*)tmp_p;
         for (i=topIndex-1 ; i>=0 ; --i)
         {
            elem_type *tmp_obj = &(tmpArray[i]);
            tmp_obj->~elem_type ();
         }
         free (tmp_p);
         topIndex = elems_in_chunk;
      }
      while (free_chunk != NULL)
      {
         tmp_p = free_chunk;
         free_chunk = (addrtype*)(free_chunk[pointer_idx]);
         free (tmp_p);
      }
   }
   
   inline void push (const elem_type& elem)
   {
      if (!top_chunk || topIndex>=(int)elems_in_chunk)  // chunk is full
      {
         if (free_chunk)  // do we have any chunk on the free list?
         {
            addrtype *tmp_p = (addrtype*)(free_chunk[pointer_idx]);
            free_chunk[pointer_idx] = (addrtype)top_chunk;
            free_chunk[pointer_idx+1] = 0;
            if (top_chunk)
               top_chunk[pointer_idx+1] = (addrtype)free_chunk;
            top_chunk = free_chunk;
            free_chunk = tmp_p;
            topIndex = 0;
         } else   // we do not; must allocate a new chunk
         {
            allocate_chunk ();
         }
      }
      elem_type *tempArray = (elem_type*)top_chunk;
      new (&tempArray[topIndex]) elem_type (elem);
      ++ topIndex;
   }
   
   inline bool empty ()
   {
      if (!topIndex)
         assert(!top_chunk[pointer_idx]);
      return (!top_chunk || topIndex==0);
   }
   
   inline elem_type& top ()
   {
//      assert (top_chunk && topIndex>0); // || ((addrtype*)top_chunk)[0]));
      // I make no checks that the data structure is not empty.
      // User is responsible of checking correctness.
      assert (topIndex>0);
      return (((elem_type*)top_chunk) [topIndex-1]);
   }
   
   inline int pop ()
   {
      // topIndex should be 0 only with an empty container
      if (!topIndex)
         assert(!top_chunk[pointer_idx]);
      if (!top_chunk || topIndex==0)
      {
         return (0);   // do nothing, return 0 - we did not pop anything
      }
      -- topIndex;
      elem_type *tmp_obj = &(((elem_type*)top_chunk) [topIndex-1]);
      tmp_obj->~elem_type ();
      if (topIndex==0 && top_chunk[pointer_idx])
      {
         addrtype *tmp_p = top_chunk;
         top_chunk = (addrtype*)(top_chunk[pointer_idx]);
         top_chunk[pointer_idx+1] = 0;
         tmp_p[pointer_idx] = (addrtype)free_chunk;
         free_chunk = tmp_p;
         topIndex = elems_in_chunk;
      }
      return (1);
   }
   
   
   // create also an iterator; I will need to traverse all elements of 
   // the stack, and it is cheaper to use an iterator than to index each
   // position;

   class ElementsIterator
   {
   public:
      ElementsIterator (GrowingStack<ElemType> *_stk) 
      { 
         pstk = _stk;
         crt_chunk = _stk->top_chunk; crt_index = _stk->topIndex;
         beyond_top = false;
      }
      ~ElementsIterator () { }
      
      inline void Reset ()
      {
         crt_chunk = pstk->top_chunk; crt_index = pstk->topIndex;
         beyond_top = false;
      }

      inline bool IsTop ()
      {
         return (crt_chunk==pstk->top_chunk && crt_index==pstk->topIndex);
      }

      inline bool IsBeyondTop ()
      {
         // this only works if the iterator moves but the container does
         // not change. If the container changes, I need to test when 
         // iterator points to the Top.
         return (beyond_top);
      }
      
      // traverse stack from top to bottom
      inline void operator ++ ()
      {
         // If we are above the stack top, we cannot use the ++ operator to move 
         // within range. Only Reset() will work.
         if (beyond_top || !crt_chunk || !crt_index)  // nothing to do
            return;
         -- crt_index;
         if (crt_index==0 && crt_chunk[pstk->pointer_idx])   // go to next chunk
         {
            crt_chunk = (addrtype*)(crt_chunk[pstk->pointer_idx]);
            crt_index = pstk->elems_in_chunk;
         }
      }

      // add an operator to move the iterator closer to the stack top
      inline void operator -- ()
      {
         if (beyond_top)
            return;
         if (crt_chunk==pstk->top_chunk && crt_index==pstk->topIndex)  // we are at the top
         {
            beyond_top = true;
            return;
         }
         if (crt_index == (int)(pstk->elems_in_chunk))   // go to previous chunk
         {
            crt_chunk = (addrtype*)(crt_chunk[pstk->pointer_idx+1]);
            crt_index = 0;
         }
         ++ crt_index;
      }
      
      inline operator bool () 
      { 
         return (!beyond_top && crt_chunk && crt_index>0);
      }
      
      inline ElemType* operator-> () { 
         assert (crt_index > 0);
         return &(((ElemType*)crt_chunk)[crt_index-1]);
      }
      inline operator ElemType* () { 
         assert (crt_index > 0);
         return &(((ElemType*)crt_chunk)[crt_index-1]);
      }
      
   private:
      GrowingStack<ElemType> *pstk;
      addrtype *crt_chunk;
      int crt_index;
      bool beyond_top;  // special flag indicating a position above the stack top
   };
   
private:
   inline void allocate_chunk ()
   {
      addrtype *new_chunk = (addrtype*)memalign (sizeof(addrtype), chunk_size);
      // first element of each chunk is used to store a pointer to the next
      // chunk
#ifdef DEBUG_CARRY_LIB
      fprintf (stdout, "stack::allocate_chunk -> allocated new chunk\n");
#endif
      new_chunk[pointer_idx] = (addrtype)top_chunk;
      new_chunk[pointer_idx+1] = 0;
      if (top_chunk)
         top_chunk[pointer_idx+1] = (addrtype)new_chunk;
      topIndex = 0;
      top_chunk = new_chunk;
   }

   unsigned int chunk_size;
   unsigned int elems_in_chunk;
   unsigned int pointer_idx;
   int topIndex;
   addrtype *top_chunk;
   addrtype *free_chunk;
};

}  /* namespace MIAMIU */

#endif
