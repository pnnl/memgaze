/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: dense_container.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements a container for mostly dense data. However, some
 * elements may be missing without requring data compaction.
 */

#ifndef MIAMI_DENSE_CONTAINER_H
#define MIAMI_DENSE_CONTAINER_H

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <limits.h>

#include <vector>
#include "miami_types.h"
#include "miami_allocator.h"

namespace MIAMIU
{

// Implement a mostly dense, auto-growing container, with fast random access 
// to any element. I will store the elements in a dense, un-movable storage, 
// using the custom MIAMI allocator. I will maintain pointers to the elements
// in a standard STL vector, which allows the container to be resized by 
// copying only the pointers to the elements, while the elements themseleves 
// do not move. 
template <class ElemType, //ElemType *defValue, 
   unsigned int start_size = 64,
   class Allocator = MiamiAllocator <ElemType, 8> > 
class DenseContainer
{
private:
   // I am storing pointers to elements in the vector
   typedef std::vector<ElemType*> ElemPVector;
   typedef Allocator    alloc_type;
   typedef DenseContainer<ElemType, /*defValue,*/ start_size, Allocator> ContainerType;

public:
   /**
    * Constructor -allocates a new container.
    *
    * @param _entries: the initial number of entries in the container.
    * @param block_size: the chunk size for the memory allocator
    */
   DenseContainer(unsigned int _entries = start_size, int block_size = 0)
   {
      if (block_size == 0)   // compute a sensible value 
      {
         block_size = _entries * sizeof(ElemType) + 16;
         if (block_size < 1024)
            block_size = 1024;
      }
      if (_entries == 0)
         _entries = 4;
      elem_allocator = new alloc_type(block_size);
      container = new ElemPVector(_entries, 0);
      _capacity = container->capacity();
      num_elems = 0;
//      def_val = defValue;
   }

   
  /**
   * Destructor - deallocates the hash map.
   *
   */
   ~DenseContainer()
   {
      delete elem_allocator;
      delete container;
   }
   
  /**
   * Removes all the elements from this hash map.
   *
   */
   inline void clear()
   {
      // While this is a dense container, it can contain holes, 
      // which is different from the vector class.
      // I must traverse all entries, and mark as empty the storage
      // for all elements.
      unsigned int i;
      typename ElemPVector::iterator eit;
      for (eit=container->begin() ; eit!=container->end() ; ++eit)
      {
         if (*eit) 
         {
            elem_allocator->delete_elem(*eit);
            *eit = 0;
         }
      }
      num_elems = 0;
   }
   
   
  /**
   * Returns the actual number of elements in the hashmap.
   *
   * @return the size of the hashmap.
   */
   inline long size() const
   {
      return (num_elems);
   }
   

  /**
   * Returns the capacity of the hashmap.
   *
   * @return the number of entries of the hashmap times 8.
   */
   inline long capacity() const
   {
      return (_capacity);
   }
   
  /**
   * Access the element at a given index. If the element is not present,
   * insert it first, increasing the container's capacity if needed.
   *
   * @param index the position in the container
   *
   * @return reference to the value field of the element with the specified index.
   */
   inline ElemType& at(long index)
   {
#ifdef DEBUG_MIAMIU
      assert (index>=0);
#endif
      if (index >= _capacity)
      {
         do {
            _capacity <<= 1;
         } while (_capacity <= index);
         container->resize(_capacity, 0);
      }
      if (!(*container)[index])
      {
         // allocate this element
         (*container)[index] = elem_allocator->new_elem();
/*
         if (def_val)
            new ((*container)[index]) ElemType (*def_val);
         else
*/
            new ((*container)[index]) ElemType ();
         ++ num_elems;
      }
      return (*(*container)[index]);
   }

  /**
   * Access the element at a given index. If the element is not present,
   * insert it first, increasing the container's capacity if needed.
   *
   * @param index the position in the container
   *
   * @return reference to the value field of the element with the specified index.
   */
   inline ElemType& operator [](long index)
   {
#ifdef DEBUG_MIAMIU
      assert (index>=0);
#endif
      if (index >= _capacity)
      {
         do {
            _capacity <<= 1;
         } while (_capacity <= index);
         container->resize(_capacity, 0);
      }
      if (!(*container)[index])
      {
         // allocate this element
         (*container)[index] = elem_allocator->new_elem();
/*
         if (def_val)
            new ((*container)[index]) ElemType (*def_val);
         else
*/
            new ((*container)[index]) ElemType ();
         ++ num_elems;
      }
      return (*(*container)[index]);
   }
   
  /**
   * Check if we have any element at a given index.
   * Do not modify container if element is not present.
   *
   * @param index the position in the container.
   *
   * @return 1 if its present, 0 otherwise.
   */
   inline int is_member (long index) const
   {
      if (index>=0 && index<_capacity && (*container)[index])
         return (1);
      else
         return (0);
   }
   
  /**
   * Removes an element from the container.
   *
   * @param index the element position in the container.
   *
   * @return 1 if it was removed, 0 otherwise (if not found).
   */
   inline int erase(int index)
   {
      if (index>=0 && index<_capacity && (*container)[index])
      {
         elem_allocator->delete_elem((*container)[index]);
         (*container)[index] = 0;
         -- num_elems;
         return (1);
      }
      else
         return (0);
   }

   class const_iterator
   {
   public:
      const_iterator(const ElemPVector *cont, int idx) : 
            _container(cont), index(idx)
      { }

   public:
      inline bool operator== (const const_iterator &rhs) const
      {
         return (this->_container==rhs._container && this->index==rhs.index);
      }
      
      inline bool operator!= (const const_iterator &rhs) const
      {
         return (this->_container!=rhs._container || this->index!=rhs.index);
      }
      
      inline const ElemType& operator*() const
      {
         if (index<0 || index>=(long)(_container->capacity()))
         {
            assert (! "DenseContainer::operator*: Bad dereferencing of iterator outside of the container.");
         } 
         return (*((*_container)[index]));
      }
      
      inline int Index() const
      {
         if (index>=0 && index<(long)(_container->capacity()))
            return (index);
         else
            return (-1);
      }

      inline const_iterator& operator++()
      {
         ++index;
         while (index<(long)(_container->capacity()) && !(*_container)[index])
         {
            ++index;
         }
         return (*this);
      }

      inline const_iterator& operator--()
      {
         --index;
         while (index>=0 && !(*_container)[index])
         {
            --index;
         }
         return (*this);
      }

      inline operator bool () const
      { 
         return (_container!=NULL && index>=0 && index<(long)(_container->capacity()) && 
                 (*_container)[index]);
      }

      inline operator const ElemType* () const
      { 
         if (index>=0 && index<(long)(_container->capacity()))
            return ((*_container)[index]);
         else
            return (0);
      }
      
      inline const ElemType* operator-> () const
      { 
         if (index>=0 && index<(long)(_container->capacity()))
            return ((*_container)[index]);
         else
            return (0);
      }
      
   private:
      const ElemPVector *_container;
      int index;
   };
   
   class iterator
   {
   public:
      iterator(ElemPVector *_cont, int idx) : 
            _container(_cont), index(idx)
      { }
   
   public:
      inline bool operator== (const iterator &rhs) const
      {
         return (this->_container==rhs._container && this->index==rhs.index);
      }
      
      inline bool operator!= (const iterator &rhs) const
      {
         return (this->_container!=rhs._container || this->index!=rhs.index);
      }
      
      inline ElemType& operator*() const
      {
         if (index<0 || index>=_container->capacity())
         {
            assert (! "DenseContainer::operator*: Bad dereferencing of iterator outside of the container.");
         } 
         return (*((*_container)[index]));
      }
      
      inline int Index() const
      {
         if (index>=0 && index<_container->capacity())
            return (index);
         else
            return (-1);
      }

      inline iterator& operator++()
      {
         ++index;
         while (index<_container->capacity() && !(*_container)[index])
         {
            ++index;
         }
         return (*this);
      }

      inline iterator& operator--()
      {
         --index;
         while (index>=0 && !(*_container)[index])
         {
            --index;
         }
         return (*this);
      }

      inline operator bool () const
      { 
         return (_container!=NULL && index>=0 && index<_container->capacity() && 
                 (*_container)[index]);
      }
      
      inline operator ElemType* () const
      { 
         if (index>=0 && index<_container->capacity())
            return ((*_container)[index]);
         else
            return (0);
      }

      inline ElemType* operator-> () const
      { 
         if (index>=0 && index<_container->capacity())
            return ((*_container)[index]);
         else
            return (0);
      }
      
      inline operator const_iterator() 
      {
         return (const_iterator(_container, index));
      }

   private:
      ElemPVector *_container;
      int index;
   };
   
   // return an iterator pointing to the first valid element
   iterator first()
   {
      int index = 0;
      if (num_elems == 0)
         index = _capacity;
      // I want to skip over holes in my container
      while (index<(long)_capacity && !(*container)[index])
         ++index;
      return (iterator(container, index));
   }

   // return an iterator pointing to the last valid element
   iterator last()
   {
      int index = _capacity-1;
      if (num_elems == 0)
         index = -1;
      // I want to skip over holes in my container
      while (index>=0 && !(*container)[index])
         --index;
      return (iterator(container, index));
   }

   // return a const_iterator pointing to the first valid element
   const_iterator first() const
   {
      int index = 0;
      if (num_elems == 0)
         index = _capacity;
      // I want to skip over holes in my container
      while (index<(long)_capacity && !(*container)[index])
         ++index;
      return (const_iterator(container, index));
   }

   // return a const_iterator pointing to the last valid element
   const_iterator last() const
   {
      int index = _capacity-1;
      if (num_elems == 0)
         index = -1;
      // I want to skip over holes in my container
      while (index>=0 && !(*container)[index])
         --index;
      return (const_iterator(container, index));
   }
   
private:
   alloc_type   *elem_allocator;
   ElemPVector  *container;
   // pointer to default object; if pointer is NULL, call default
   // constructor, otherwose it calls the copy constructor with 
   // the default object.
//   ElemType    *def_val;
   
   long _capacity;
   long num_elems;
};

}  /* namespace MIAMIU */

#endif
