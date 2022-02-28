/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: self_sorted_list.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements a linked list that auto-maintains a most recently 
 * used ordering.
 */

#ifndef SELF_SORTED_LIST_H
#define SELF_SORTED_LIST_H

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <limits.h>

#include "miami_allocator.h"


namespace MIAMIU  /* MIAMI Utils */
{

template <class ElemType> 
class ListElem
{
public:
   ListElem() : next(0)
   {}
   
   ElemType elem;
   ListElem *next;
};

// mapping from int to ValType, KeyType must be either 4 byte or 8 byte integer type
template <class ElemType, unsigned int def_entries = 16,
   class Allocator = MiamiAllocator <ListElem<ElemType>, 8> > 
class SelfSortedList
{
public:
   typedef ListElem<ElemType>  list_elem;
   typedef Allocator    alloc_type;

   /**
    * Constructor -allocates a new list.
    *
    * @param _entries: the initial number of entries in the list.
    * @param block_size: the chunk size for the memory allocator
    */
   SelfSortedList (unsigned int _entries = def_entries, int block_size = 0)
   {
      if (block_size == 0)   // compute a sensible value 
      {
         block_size = _entries*sizeof(list_elem)+16;
         if (block_size < 256)
            block_size = 256;
         else if (block_size>4096)
            block_size = 4096;
      }
      list_allocator = new alloc_type(block_size);
      first_elem = 0;
      num_elems = 0;
   }

   
  /**
   * Destructor - deallocates the hash map.
   *
   */
   ~SelfSortedList()
   {
      delete list_allocator;
   }
   
   
  /**
   * Removes all the elements from this hash map.
   *
   */
   inline void clear()
   {
      list_elem *iter = first_elem;
      while (iter)
      {
         list_elem *next = iter->next;
         list_allocator->delete_elem(iter);
         iter = next;
      }
      first_elem = 0;
      num_elems = 0;
   }
   
   
  /**
   * Returns the actual number of elements in the list.
   *
   * @return the size of the list.
   */
   inline long size() const
   {
      return (num_elems);
   }
   

  /**
   * Puts an element into the list if it does not exist.
   *
   * @param elem the element to search/insert into the list.
   *
   * @return reference to the referenced element.
   */
   inline ElemType& insert (const ElemType& elem)
   {
      return (insert_core(elem)->elem);
   }

  /**
   * Puts an element into the list if it does not exist.
   *
   * @param elem the element to search/insert into the list.
   *
   * @return reference to the referenced element.
   */
   inline ElemType& operator[] (const ElemType& elem)
   {
      return (insert_core(elem)->elem);
   }

  /**
   * Membership check for a list. It does not insert a new
   * element if elem is not found. However, elem is moved
   * to the head of the list if found.
   *
   * @param elem the element to check membership of.
   *
   * @return 1 if its a member, 0 otherwise.
   */
   inline int is_member (const ElemType& elem)
   {
      if (first_elem != 0)  // we have some elements mapping to this entry
      {
         list_elem *iter = first_elem;
         list_elem *prev_iter = 0;
         while (iter && iter->elem!=elem)
         {
            prev_iter = iter;
            iter = iter->next;
         }
         if (! iter)
            return (0);
         else 
            if (prev_iter)  // not in MRU position already
            {
               prev_iter->next = iter->next;
               iter->next = first_elem;
               first_elem = iter;
            }
      } else
         return (0);
      return (1);
   }

  /**
   * Membership check for a list. It does not insert a new
   * element if elem is not found. Use this method if you
   * do not wish for a found element to be moved to the front
   * of the list.
   *
   * @param elem the element to check membership of.
   *
   * @return 1 if its a member, 0 otherwise.
   */
   inline int is_member_const (const ElemType& elem) const
   {
      if (first_elem != 0)  // we have some elements mapping to this entry
      {
         list_elem *iter = first_elem;
         while (iter && iter->elem!=elem)
            iter = iter->next;
         if (iter)
            return (1);
      }
      return (0);
   }

  /**
   * Removes an element from the list.
   *
   * @param elem the item to remove.
   *
   * @return 1 if it was removed, 0 otherwise.
   */
   inline int erase (const ElemType& elem)
   {
      if (first_elem != 0)  // we have some elements mapping to this entry
      {
         list_elem *iter = first_elem;
         list_elem *prev_iter = 0;
         while (iter && iter->elem!=elem)
         {
            prev_iter = iter;
            iter = iter->next;
         }
         
         if (iter)
         {
            if (prev_iter)
               prev_iter->next = iter->next;
            else
               first_elem = iter->next;
            
            num_elems -= 1;
            list_allocator->delete_elem(iter);
            return (1);   // it is already in the MRU position
         }
      } 
      return (0);
   }

   class const_iterator
   {
   public:
      const_iterator(list_elem *_iter) : iter(_iter)
      { }
   
   public:
      inline bool operator== (const const_iterator &rhs) const
      {
         return (this->iter == rhs.iter);
      }
      
      inline bool operator!= (const const_iterator &rhs) const
      {
         return (this->iter != rhs.iter);
      }
      
      inline const ElemType& operator*() const
      {
         if (! iter)
         {
            assert (! "SelfSortedList::operator*: Dereferencing of invalid iterator.");
         } 
         return (iter->elem);
      }
      
      inline const_iterator& operator++()
      {
         if (iter)
            iter = iter->next;
         return (*this);
      }

      inline operator bool () const
      { 
         return (iter!=NULL);
      }
      
      inline operator const ElemType* () const
      { 
         if (iter)
            return (&(iter->elem));
         else
            return (0);
      }

      inline const ElemType* operator-> () const
      { 
         if (iter)
            return (&(iter->elem));
         else
            return (0);
      }
      
   private:
      list_elem *iter;
   };

   class iterator
   {
   public:
      iterator(list_elem *_iter) : iter(_iter)
      { }
   
   public:
      inline bool operator== (const iterator &rhs) const
      {
         return (this->iter == rhs.iter);
      }
      
      inline bool operator!= (const iterator &rhs) const
      {
         return (this->iter != rhs.iter);
      }
      
      inline ElemType& operator*() const
      {
         if (! iter)
         {
            assert (! "SelfSortedList::operator*: Dereferencing of invalid iterator.");
         } 
         return (iter->elem);
      }
      
      inline iterator& operator++()
      {
         if (iter)
            iter = iter->next;
         return (*this);
      }

      inline operator bool () const
      { 
         return (iter!=NULL);
      }
      
      inline operator ElemType* () const
      { 
         if (iter)
            return (&(iter->elem));
         else
            return (0);
      }

      inline ElemType* operator-> () const
      { 
         if (iter)
            return (&(iter->elem));
         else
            return (0);
      }
      
      inline operator const_iterator() 
      {
         return (const_iterator(iter));
      }

   private:
      list_elem *iter;
   };
   
   // return an iterator pointing to the first valid element
   iterator first()
   {
      return (iterator(first_elem));
   }

   const_iterator first() const
   {
      return (const_iterator(first_elem));
   }

   
private:
   inline list_elem* insert_core(const ElemType& elem)
   {
      list_elem *iter = first_elem;
      if (iter != 0)  // we have some elements mapping to this entry
      {
         list_elem *prev_iter = 0;
         while (iter && iter->elem!=elem)
         {
            prev_iter = iter;
            iter = iter->next;
         }
         if (! iter)
         {
            iter = list_allocator->new_elem();
            num_elems += 1;
            iter->next = first_elem;
            iter->elem = elem;
            first_elem = iter;
         }
         else 
         {
            if (prev_iter)  // not in MRU position already
            {
               prev_iter->next = iter->next;
               iter->next = first_elem;
               first_elem = iter;
            }
         }
      } else
      {
         // this is the first hit to this list
         iter = list_allocator->new_elem();
         num_elems += 1;
         iter->next = 0;
         iter->elem = elem;
         first_elem = iter;
      }
      return (iter);
   }

   alloc_type *list_allocator;
   list_elem  *first_elem;

   long num_elems;    // total number of elements in the list
};

} /* namespace MIAMIU */

#endif
