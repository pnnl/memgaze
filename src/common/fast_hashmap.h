/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: fast_hashmap.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A hybrid and growing vector - hash table implementation.
 * Supports fast associative access, and linear traversal of the elements, 
 * but it is inefficient for element removal.
 */

#ifndef FAST_HASHMAP_H
#define FAST_HASHMAP_H

#ifdef __GNUC__
#pragma interface
#endif

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

namespace MIAMIU  /* MIAMI Utils */
{

#define HM_NIL (-1)
#define DMARK (-2)
#define C_ONE 7
#define C_TWO 11


#ifdef HASHMAP_ENABLE_ERASE
static const int MAX_DMARKS_ALLOWED = 4;
#endif


// mapping from int to ValType, KeyType must be either 4 byte or 8 byte integer type
template <class KeyType, class ValType, ValType *defValue, unsigned int def_capacity = 128> 
class HashMap
{
public:
  typedef int (*MapFunction)(void* arg0, KeyType key, void* value);

  /**
   * Constructor -allocates a new hash map.
   *
   * @param block the initial block size of the hashmap.
   */
   HashMap (int block = def_capacity)
   {
#if defined(DWORD_ALIGNED)
      unit_size = 8;
#else
      unit_size = sizeof(KeyType);
      if (unit_size<4)
         unit_size = 4;
#endif
      elem_offset = (unit_size-1)/sizeof(KeyType) + 1;
      
      def_val = *defValue;
      elem_size = (sizeof(ValType)-1)/unit_size + 2; // size in units
      elem_size = elem_size * unit_size / sizeof(KeyType);
//      printf ("HashMap, elem_size=%d, unit_size=%d, sizeof(KeyType)=%lu, elem_offset=%d\n",
//               elem_size, unit_size, sizeof(KeyType), elem_offset);
      
      _capacity = block;
      _size = 0;
      _mask = 2*block - 1;
      // align the map at 16 bytes
      _map = (KeyType*)memalign (unit_size, _capacity*elem_size*sizeof(KeyType));
      _hash = (KeyType*)malloc (_capacity*4*sizeof (KeyType) );
      
      // clear the hash table
      KeyType hnil = HM_NIL;
      for(unsigned int i=0 ; i<_capacity*4 ; ++i)
         _hash[i] = hnil;
//      memset (_hash, 0xff, _capacity*4*sizeof(KeyType) );
#ifdef HASHMAP_ENABLE_ERASE
      dmarkCount = 0;
      dmarkLimit = _capacity >> 2;
      freeList = -1;
      freeEntries = 0;
#endif
   }


   /** Deep copy constructor. It is not tested. It may work with simple
    * value types. However, if the values are pointers to other objects, 
    * it will just copy the pointers, it will not duplicate the objects.
    * Avoid copying hashtables, or expect to have to debug.
    */
   HashMap (const HashMap& ht)
   {
      unit_size = ht.unit_size;
      elem_offset = ht.elem_offset;
      
      def_val = ht.def_val;
      elem_size = ht.elem_size;
      
      _capacity = ht._capacity;
      _size = ht._size;
      _mask = ht._mask;
      // align the map at 16 bytes
      _map = (KeyType*)memalign (unit_size, _capacity*elem_size*sizeof(KeyType));
      _hash = (KeyType*)malloc (_capacity*4*sizeof (KeyType) );
      
      memcpy(_map, ht._map, _capacity*elem_size*sizeof(KeyType));
      memcpy(_hash, ht._hash, _capacity*4*sizeof (KeyType));

#ifdef HASHMAP_ENABLE_ERASE
      dmarkCount = ht.dmarkCount;
      dmarkLimit = ht.dmarkLimit;
      freeList = ht.freeList;
      freeEntries = ht.freeEntries;
#endif
   }

   
  /**
   * Destructor - deallocates the hash map.
   *
   */
   ~HashMap()
   {
      free(_map);
      free(_hash);
   }

   /** Deep copy operator. It is not tested. It may work with simple
    * value types. However, if the values are pointers to other objects, 
    * it will just copy the pointers, it will not duplicate the objects.
    * Avoid copying hashtables, or expect to have to debug.
    */
   HashMap& operator= (const HashMap& ht)
   {
      unit_size = ht.unit_size;
      elem_offset = ht.elem_offset;
      
      def_val = ht.def_val;
      elem_size = ht.elem_size;
      
      _capacity = ht._capacity;
      _size = ht._size;
      _mask = ht._mask;
      // align the map at 16 bytes
      _map = (KeyType*)memalign (unit_size, _capacity*elem_size*sizeof(KeyType));
      _hash = (KeyType*)malloc (_capacity*4*sizeof (KeyType) );
      
      memcpy(_map, ht._map, _capacity*elem_size*sizeof(KeyType));
      memcpy(_hash, ht._hash, _capacity*4*sizeof (KeyType));

#ifdef HASHMAP_ENABLE_ERASE
      dmarkCount = ht.dmarkCount;
      dmarkLimit = ht.dmarkLimit;
      freeList = ht.freeList;
      freeEntries = ht.freeEntries;
#endif
      return (*this);
   }

   
   
  /**
   * Removes all the elements from this hash map.
   *
   */
   inline void clear()
   {
      _size = 0;
#ifdef HASHMAP_ENABLE_ERASE
      freeEntries = 0;
      freeList = -1;
#endif
      KeyType hnil = HM_NIL;
      for(unsigned int i=0 ; i<_capacity*4 ; ++i)
         _hash[i] = hnil;
//      memset(_hash, 0xff, _capacity*4*sizeof(KeyType) );
   }
   
   
  /**
   * Returns the actual number of elements in the hashmap.
   *
   * @return the size of the hashmap.
   */
   inline unsigned int size() const
   {
#ifdef HASHMAP_ENABLE_ERASE
//      fprintf(stderr, "HASHMAP size=%u, freeEntries=%d\n", _size, freeEntries);
      return (_size - freeEntries);
#else
//      fprintf(stderr, "HASHMAP size=%u\n", _size);
      return _size;
#endif
   }
   

  /**
   * Returns the capacity of the hashmap.
   *
   * @return the capacity of the hashmap.
   */
   inline unsigned int capacity() const
   {
      return _capacity;
   }
   
   
  /**
   * Returns the size in units of one map element.
   *
   * @return the size in units of one map element.
   */
   inline int element_size() const
   {
      return elem_size;
   }
   

  /**
   * Puts an element into hashmap.
   *
   * @param item the key to insert into the map.
   *
   * @return reference to the value field of the element with the key item.
   */
   ValType& insert (KeyType item)
   {
      unsigned int m;
#ifdef HASHMAP_ENABLE_ERASE
      if (freeEntries>0)
         m = freeList*elem_size;
      else
#endif
      {
         if (_size >= (_capacity) )
         {
            grow_capacity();
         }
      
         m = _size*elem_size;
      }
#ifdef DEBUG_HASHMAP
      if (m<0 || m>=(_capacity*elem_size))
         assert (false);
#endif
      *((KeyType*)(_map+m)) = item;
      
      while(1)
      {
         int r = hash_insert(m);
         if (r>=0)
         {
#ifdef DEBUG_HASHMAP
            if (r<0 || r>=(_capacity*elem_size))
               assert (false);
#endif
#ifdef HASHMAP_ENABLE_ERASE
            if (freeEntries>0)
               _map[m] = (KeyType)DMARK;
#endif
            return (ValType&)_map[r];
         }
         if (r==(-2))
         {
#ifdef DEBUG_HASHMAP
            if ((m+1)<0 || (m+1)>=(_capacity*elem_size))
               assert (false);
#endif
#ifdef HASHMAP_ENABLE_ERASE
            if (freeEntries>0) {
               freeList = (KeyType)_map[m+elem_offset];
               freeEntries -= 1;
            } else
#endif
            _size++;
            (ValType&)_map[m+elem_offset] = def_val;
            return (ValType&)_map[m+elem_offset];
         }
         else if (r==(-1))
         {
            grow_capacity();
            continue;
         }
      }
   }


  /**
   * Puts an element into hashmap.
   *
   * @param item the key to insert into the map.
   *
   * @return reference to the value field of the element with the key item.
   */
   ValType& operator[](const KeyType item)
   {
      unsigned int m;
#ifdef HASHMAP_ENABLE_ERASE
      if (freeEntries>0)
         m = freeList*elem_size;
      else
#endif
      {
         if (_size >= (_capacity) )
         {
            grow_capacity();
         }
      
         m = _size*elem_size;
      }
#ifdef DEBUG_HASHMAP
      if (m<0 || m>=(_capacity*elem_size))
         assert (false);
#endif
      *((KeyType*)(_map+m)) = item;
      
      while(1)
      {
         int r = hash_insert(m);
         if (r>=0)
         {
#ifdef DEBUG_HASHMAP
            if (r<0 || r>=(_capacity*elem_size))
               assert (false);
#endif
#ifdef HASHMAP_ENABLE_ERASE
            if (freeEntries>0)
               _map[m] = (KeyType)DMARK;
#endif
            return (ValType&)_map[r];
         }
         if (r==(-2))
         {
#ifdef DEBUG_HASHMAP
            if ((m+1)<0 || (m+1)>=(_capacity*elem_size))
               assert (false);
#endif
#ifdef HASHMAP_ENABLE_ERASE
            if (freeEntries>0) {
               freeList = (KeyType)_map[m+elem_offset];
               freeEntries -= 1;
            } else
#endif
            _size++;
            (ValType&)_map[m+elem_offset] = def_val;
            return (ValType&)_map[m+elem_offset];
         }
         else if (r==(-1))
         {
            grow_capacity();
            continue;
         }
      }
   }

  /**
   * Returns the value of the element with the specified index.
   *
   * @param index the index of the element we want.
   *
   * @return reference to the value field of the element with the index index.
   */
   ValType& atIndex (const int index) 
   {
      if (index<0 || index>=(int)_size)
      {
         fprintf(stderr, "HashMap::atIndex Called with invalid index %d. Expected index in interval [0,%d]\n",
                 index, _size-1 );
         assert(! "Invalid index!");
      }
      
      register unsigned int m = index*elem_size + elem_offset;
      return ((ValType&)_map[m]);
   }
   

  /**
   * Finds the index of an element into hashmap.
   *
   * @param item the key to search into the map.
   *
   * @return if element is found then returns its index (from 0 to SIZE-1).
   * If not found then return (-1).
   */
   int indexOf (KeyType item) 
#ifndef HASHMAP_ENABLE_ERASE
         const
#endif
   {
      unsigned int n = _capacity << 1;

      register uint64_t key = (uint64_t)item;
      key += ~(key << 15);
      key ^=  (key >> 10);
      key +=  (key << 3);
      key ^=  (key >> 6);
      key += ~(key << 11);
      key ^=  (key >> 16);

      unsigned int hc = key & _mask;
      unsigned int hc2 = hc << 1;
      unsigned int i = 0;
#ifdef HASHMAP_ENABLE_ERASE
      register int firstDmark = -1;
#endif      
      
      while (i < n) 
      {
         if (_hash[hc2] == item) 
         {
            int index = _hash[hc2+1];
#ifdef HASHMAP_ENABLE_ERASE
            if (firstDmark>=0)
               // move the found entry into an earlier position
               // compact the path
            {
               _hash[firstDmark] = item;
               _hash[firstDmark+1] = index;
               _hash[hc2] = DMARK;
            }
#endif
            return (index/elem_size);
         }
         else if (_hash[hc2] == (KeyType)HM_NIL)
         {
            return (-1);
         }
#ifdef HASHMAP_ENABLE_ERASE
         else if (_hash[hc2] == (KeyType)DMARK && firstDmark<0) 
            firstDmark = hc2;
#endif

         hc = key + i * C_ONE + i * i * C_TWO;
         hc = hc & _mask;    //hc % n;
         hc2 = hc << 1;
         i++;
      }

      // hashtable is full
      return (-1);
   }


  /**
   * Membership check for a hashmap.
   *
   * @param item the key to check membership of.
   *
   * @return 1 if its a member, 0 otherwise.
   */
   int is_member (KeyType item) 
#ifndef HASHMAP_ENABLE_ERASE
         const
#endif
   {
      unsigned int n = _capacity << 1;

      register uint64_t key = (uint64_t)item;
      key += ~(key << 15);
      key ^=  (key >> 10);
      key +=  (key << 3);
      key ^=  (key >> 6);
      key += ~(key << 11);
      key ^=  (key >> 16);

      unsigned int hc = key & _mask;
      unsigned int hc2 = hc << 1;
      unsigned int i = 0;
#ifdef HASHMAP_ENABLE_ERASE
      register int firstDmark = -1;
#endif      

      while (i < n) 
      {
         if (_hash[hc2] == item) 
         {
#ifdef HASHMAP_ENABLE_ERASE
            if (firstDmark>=0)
               // move the found entry into an earlier position
               // compact the path
            {
               _hash[firstDmark] = item;
               _hash[firstDmark+1] = _hash[hc2+1];
               _hash[hc2] = DMARK;
            }
#endif
            return 1;
         }
         else if (_hash[hc2] == (KeyType)HM_NIL) 
         {
            return 0;
         }
#ifdef HASHMAP_ENABLE_ERASE
         else if (_hash[hc2] == DMARK && firstDmark<0) 
            firstDmark = hc2;
#endif
         hc = key + i * C_ONE + i * i * C_TWO;
         hc = hc & _mask;    //hc % n;
         hc2 = hc << 1;
         i++;
      }

      return 0;
   }
   
   
#ifdef HASHMAP_ENABLE_ERASE
  /**
   * Removes an element from the hashmap.
   *
   * @param item the item to remove.
   *
   * @return 1 if it was removed, 0 otherwise.
   */
   int erase (KeyType item)
   {
      // not implemented now
      if (_size < 1)
         return (0);
      unsigned int n = _capacity << 1;

      register uint64_t key = (uint64_t)item;
      key += ~(key << 15);
      key ^=  (key >> 10);
      key +=  (key << 3);
      key ^=  (key >> 6);
      key += ~(key << 11);
      key ^=  (key >> 16);

      unsigned int hc = key & _mask;
      unsigned int hc2 = hc << 1;
      unsigned int i = 0;
      
      while (i < n) 
      {
         if (_hash[hc2] == item) 
         {
            // compute the index of the found entry
            register unsigned int position = _hash[hc2+1];
            int crtIndex = (position/elem_size);
            _hash[hc2] = DMARK;  // mark the hash entry as empty, but
                                 // DMARK means that an element was removed
            ++ dmarkCount;

            // if this is not the last element in the map array, then
            // we have to move the last element in the gap created;
            // we cannot have holes in the map array
            // gmarin 04/26/2012: what if we kept holes in the map array?
            // Just mark them with the special DMARK value. Those entries 
            // can also store a linked list of free entries. Use these entries
            // first when adding new elements
            // if it is the last element, then just decrement the _size
            if (crtIndex == (int)(_size-1))
               -- _size;
            else
            {
               assert (crtIndex < (int)(_size-1));
               // otherwise, mark the entry as deleted
               _map[position] = (KeyType)DMARK;
               _map[position+elem_offset] = (KeyType)freeList;
               freeList = (KeyType)crtIndex;
               freeEntries += 1;
            } 
            if (dmarkCount>dmarkLimit)
               clean_dmark_entries();
            // we deleted one element; Should I initialize the Value object
            // to the defaultValue, or is this done on insertion? It is done on insertion.
            return (1);  // found and removed the element
         }
         else if (_hash[hc2] == (KeyType)HM_NIL)
         {
            return (0);
         }

         hc = key + i * C_ONE + i * i * C_TWO;
         hc = hc & _mask;    //hc % n;
         hc2 = hc << 1;
         i++;
      }

      // did not find it
      return (0);
   }

#endif

  /**
   * Saves all the elements (key, value) in the specified file.
   *
   * @param fd the output file pointer.
   *
   * @return the number of elements effectively written in the file.
   */
   int dump_to_file_as_text(FILE* fd) const
   {
      fprintf (fd, "Size %d\n", _size);
      for (int i=0 ; i<_size ; ++i)
      {
#ifdef HASHMAP_ENABLE_ERASE
         // print if not erased
         if (_map[i*elem_size] == (KeyType)DMARK)
            continue;
#endif
         if (sizeof(KeyType) == 8)
            fprintf (fd, "%016x: ", *((KeyType*)(_map+i*elem_size)) );
         else  // it should be 64 bit value?
            fprintf (fd, "%08x: ", *((KeyType*)(_map+i*elem_size)) );
         ((ValType&)_map[i*elem_size+elem_offset]).dump_to_file (fd);
      }
      return _size;
   }

   int dump_to_file (FILE* fd) const
   {
      fwrite (&_size, sizeof(unsigned int), 1, fd);
      int r = fwrite(_map, elem_size*sizeof(KeyType), _size, fd);
      return r;
   }


   int dump_to_file_special (FILE* fd, int level) const
   {
      if (level>1)
      {
         fprintf (fd, "Size %d\n", _size);
         for( int i=0 ; i<_size ; i++ )
         {
#ifdef HASHMAP_ENABLE_ERASE
            // print if not erased
            if (_map[i*elem_size] == (KeyType)DMARK)
               continue;
#endif
            fprintf(fd, "%08x: ", _map[i*elem_size] );
            ((HashMap*)((ValType&)_map[i*elem_size+elem_offset]))->dump_to_file_special(fd, level-1);
         }
      }
      else   // level == 1
      {
         for( int i=0 ; i<_size ; i++ )
         {
#ifdef HASHMAP_ENABLE_ERASE
            // print if not erased
            if (_map[i*elem_size] == (KeyType)DMARK)
               continue;
#endif
            fprintf(fd, "%d (%d),  ", _map[i*elem_size], _map[i*elem_size+elem_offset] );
         }
         fprintf(fd, "\n");
      }
      return _size;
   }

   int map (MapFunction func, void* arg0) const
   {
      for (unsigned int i=0 ; i<_size ; ++i)
      {
#ifdef HASHMAP_ENABLE_ERASE
         // print if not erased
         if (_map[i*elem_size] == (KeyType)DMARK)
            continue;
#endif
         func (arg0, *((KeyType*)(_map+i*elem_size)), 
                 (&_map[i*elem_size+elem_offset]));
      }
      return _size;
   }
   
private:
   void grow_capacity()
   {
      unsigned int i;
      unsigned int num_elem = _size*elem_size;
#ifdef HASHMAP_ENABLE_ERASE
      assert(freeList == KeyType(-1));
#endif
      while (1) 
      {
         _capacity <<= 1;          // should be power of 2 always
#ifdef HASHMAP_ENABLE_ERASE
         dmarkLimit = _capacity >> 2;
         dmarkCount = 0;
#endif
         // try to realloc _map first; Maybe it has more chances if I do it first
         _map = (KeyType*)realloc (_map, _capacity*elem_size*sizeof(KeyType));
         if (!_map || ((long)_map & (unit_size-1)) )
            printf ("HashMap::grow_capacity to %d, ERROR allocating _map\n", _capacity);
         free (_hash);
         _hash = (KeyType*) malloc (_capacity*4*sizeof(KeyType));
         if (!_hash || ((long)_hash & 3) )
            printf ("HashMap::grow_capacity to %d, ERROR allocating _hash\n", _capacity);

//         printf ("HashMap::grow_capacity to %d, _map=0x%08x, _hash=0x%08x\n", 
//               _capacity, _map, _hash);

//         _hash = (unsigned int*)realloc(_hash, _capacity*4*sizeof(unsigned int));

//         register void* _tempmap = malloc(_capacity*elem_size*sizeof(UnitType)+16);
//         register UnitType* _auxmap = (UnitType*)( ((((long)_tempmap)+15)/16)*16 );
//         memcpy(_auxmap, _map, sizeof(UnitType)*num_elem);
//         free(_locmap);
//         _locmap = _tempmap;
//         _map = _auxmap;

         _mask = 2*_capacity - 1;
    
         KeyType hnil = HM_NIL;
         for(i=0 ; i<_capacity*4 ; ++i)
            _hash[i] = hnil;
//         memset (_hash, 0xff, _capacity*4*sizeof(KeyType));

         for (i=0; i<num_elem; i+=elem_size) 
            if (hash_insert(i) == (-1)) 
            {
                printf("GROW C failed, size %u, cap %u, idx %u\n", 
                    _size, _capacity, i);
                continue;
            }
         return;
      }
   }

   
   int hash_insert (unsigned int index)
   {
      unsigned int n = _capacity << 1;
      KeyType item = *((KeyType*)(_map+index));

      register uint64_t key = (uint64_t)item;
      key += ~(key << 15);
      key ^=  (key >> 10);
      key +=  (key << 3);
      key ^=  (key >> 6);
      key += ~(key << 11);
      key ^=  (key >> 16);

      unsigned int hc = key & _mask;
      unsigned int hc2 = hc << 1;
      unsigned int i = 0;
#ifdef HASHMAP_ENABLE_ERASE
      register int firstDmark = -1;
#endif
      
      while (i < n) 
      {
#ifdef DEBUG_HASHMAP
         if ((hc2)<0 || (hc2+1)>=(_capacity*4))
            assert (false);
#endif
         if (_hash[hc2] == item) 
         {
            int found_index = _hash[hc2+1];
#ifdef HASHMAP_ENABLE_ERASE
            if (firstDmark>=0)
            {
               _hash[firstDmark] = item;
               _hash[firstDmark+1] = found_index;
               _hash[hc2] = DMARK;
            }
#endif
            return (found_index+elem_offset);
         }
         else if (_hash[hc2] == (KeyType)HM_NIL)
         {
#ifdef HASHMAP_ENABLE_ERASE
            if (firstDmark>=0)
            {
               _hash[firstDmark] = item;
               _hash[firstDmark+1] = index;
               -- dmarkCount;
            } else
#endif
            {
               _hash[hc2] = item;
               _hash[hc2+1] = index;
            }
            return (-2);
         }
#ifdef HASHMAP_ENABLE_ERASE
         else if (_hash[hc2] == (KeyType)DMARK && firstDmark<0)
            firstDmark = hc2;
#endif

         hc = key + i * C_ONE + i * i * C_TWO;
         hc = hc & _mask;    //hc % n;
         hc2 = hc << 1;
         i++;
      }

      // hashtable is full
      return (-1);
   }
   
#ifdef HASHMAP_ENABLE_ERASE
   void clean_dmark_entries()
   {
      unsigned int i;
      unsigned int num_elem = _size*elem_size;
#if DEBUG_STREAM_DETECTION
      fprintf(stderr, "HashMap::clean_dmark_entries, dmarkCount=%d\n", dmarkCount);
      fflush(stderr);
#endif
      dmarkCount = 0;

      KeyType hnil = HM_NIL;
      for(i=0 ; i<_capacity*4 ; ++i)
         _hash[i] = hnil;
//      memset (_hash, 0xff, _capacity*4*sizeof(KeyType));

      for (i=0; i<num_elem; i+=elem_size) 
         if (hash_insert(i) == (-1)) 
         {
             assert(!"Cannot reinsert all the elements back");
             return;
         }
      return;
   }


   int update_hash(unsigned int index, unsigned int newIndex)
   {
      unsigned int n = _capacity << 1;
#ifdef DEBUG_HASHMAP
      if ((index)<0 || (index)>=(_capacity*elem_size))
         assert (false);
#endif
      KeyType item = *((KeyType*)(_map+index));

      register uint64_t key = (uint64_t)item;
      key += ~(key << 15);
      key ^=  (key >> 10);
      key +=  (key << 3);
      key ^=  (key >> 6);
      key += ~(key << 11);
      key ^=  (key >> 16);

      unsigned int hc = key & _mask;
      unsigned int hc2 = hc << 1;
      unsigned int i = 0;
//      register int numDmarks = 0;
      
      while (i < n) 
      {
         if (_hash[hc2] == item) 
         {
            // we should always get here eventually
            _hash[hc2+1] = newIndex;
            return (1);
         }
         else if (_hash[hc2] == (KeyType)HM_NIL)
         {
            assert(! "we should always find the key. Error.");
            return (0);
         }
#if 0
         else if (_hash[hc2] == DMARK)
         {
            ++ numDmarks;
            if (numDmarks>MAX_DMARKS_ALLOWED)
            {
               clean_dmark_entries();
               return (1);
            }
         }
#endif

         hc = key + i * C_ONE + i * i * C_TWO;
         hc = hc & _mask;    //hc % n;
         hc2 = hc << 1;
         i++;
      }

      // hashtable is full
      assert(! "we should always find the key. Error.");
      return (0);
   }
#endif

public:   
   KeyType *_map;
private:
   unsigned int elem_size;
   unsigned int elem_offset;
   unsigned int unit_size;
   KeyType *_hash;
   unsigned int _size;
   unsigned int _capacity;
   uint64_t _mask;
   ValType def_val;
#ifdef HASHMAP_ENABLE_ERASE
   int dmarkCount;
   int dmarkLimit;
   KeyType freeList;
   int freeEntries;
#endif

public:
   class iterator
   {
   };
};

}  /* namespace MIAMIU */

#endif
