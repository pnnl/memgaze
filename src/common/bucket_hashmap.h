/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: bucket_hashmap.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A bucket hash table implementation.
 */

#ifndef BUCKET_HASHMAP_H
#define BUCKET_HASHMAP_H

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <limits.h>

#include "miami_allocator.h"

#define HASHMAP_OPTIMIZE_MRU 1

namespace MIAMIU  /* MIAMI Utils */
{

template <class KeyType, class ValType, ValType *defValue> 
class BucketElem
{
public:
   BucketElem() : next(0)
   {}
   
   KeyType key;   // I need to find the exact entry in the list
   ValType val;
   BucketElem *next;
};

// mapping from int to ValType, KeyType must be either 4 byte or 8 byte integer type
template <class KeyType, class ValType, ValType *defValue, unsigned int def_entries = 128,
   unsigned int def_chunk_size = 0,
   class Allocator = MiamiAllocator <BucketElem<KeyType, ValType, defValue>, 8> > 
class BucketHashMap
{
public:
   typedef BucketElem<KeyType, ValType, defValue>  bucket_elem;

   class BucketEntry 
   {
   public:
      BucketEntry() : first(0)
      {}
      bucket_elem *first;
   };

   typedef Allocator    alloc_type;
  
   typedef int (*MapFunction)(void* arg0, KeyType key, void* value);

   /**
    * Constructor -allocates a new hash map.
    *
    * @param _entries: the initial number of buckets in the hashtable.
    * @param block_size: the chunk size for the memory allocator
    */
   BucketHashMap (unsigned int _entries = def_entries, int block_size = def_chunk_size)
   {
      if (block_size == 0)   // compute a sensible value 
      {
         if (_entries < 128)
            block_size = _entries << 5;   // multiply by 32
         else
            block_size = 4096;   // allocate at most 1 page at a time
      }
      bucket_allocator = new alloc_type(block_size);
      
      def_val = *defValue;
      
      //Find the closest power of 2 that is greater or equal to _entries
      num_entries = 2;  // Minimum size is 2 
      while (num_entries < _entries)
         num_entries <<= 1;
      mask = num_entries - 1;  // this assumes that table size is a power of 2;
      
      buckets = new BucketEntry[num_entries];
      used_entries = 0;
      num_elems = 0;
      cached = false;
   }

   
  /**
   * Destructor - deallocates the hash map.
   *
   */
   ~BucketHashMap()
   {
      delete bucket_allocator;
      delete[] buckets;
   }
   
   
  /**
   * Removes all the elements from this hash map.
   *
   */
   inline void clear()
   {
      // I must traverse all buckets, and mark as empty the BucketElems
      unsigned int i;
      for (i=0 ; i<num_entries ; ++i)
      {
         bucket_elem *iter = buckets[i].first;
         while (iter)
         {
            bucket_elem *next = iter->next;
            bucket_allocator->delete_elem(iter);
            iter = next;
         }
         buckets[i].first = 0;
      }
      used_entries = 0;
      num_elems = 0;
      cached = false;
   }
   
   
  /**
   * Returns the actual number of elements in the hashmap.
   *
   * @return the size of the hashmap.
   */
   inline unsigned int size() const
   {
      return (num_elems);
   }
   

  /**
   * Returns the capacity of the hashmap.
   *
   * @return the number of entries of the hashmap times 8.
   */
   inline unsigned int capacity() const
   {
      return (num_entries*8);
   }
   
   
  /**
   * Puts an element into hashmap.
   *
   * @param item the key to insert into the map.
   *
   * @return reference to the value field of the element with the key item.
   */
   inline ValType& insert (const KeyType& item)
   {
      return (insert_core(compute_hash(item), item)->val);
   }

   inline ValType& insert (const KeyType& item, unsigned int pre_hash)
   {
      return (insert_core(pre_hash & mask, item)->val);
   }

   inline ValType& insert_cached (const KeyType& item)
   {
      if (!cached || lastKey!=item)
         lastElem = insert_core(compute_hash(item), item);
      cached = true;
      return (lastElem->val);
   }

   inline ValType& insert_cached (const KeyType& item, unsigned int pre_hash)
   {
      if (!cached || lastKey!=item)
         lastElem = insert_core(pre_hash & mask, item);
      cached = true;
      return (lastElem->val);
   }

  /**
   * Puts an element into hashmap.
   *
   * @param item the key to insert into the map.
   *
   * @return reference to the value field of the element with the key item.
   */
   inline ValType& operator[] (const KeyType& item)
   {
      return (insert_core(compute_hash(item), item)->val);
   }

   inline ValType& operator() (const KeyType& item, unsigned int pre_hash)
   {
      return (insert_core(pre_hash & mask, item)->val);
   }

  /**
   * Returns the value of the element with the specified index.
   *
   * @param index the index of the element we want.
   *
   * @return reference to the value field of the element with the index index.
   */
   ValType& atIndex (const int index) const
   {
      assert(! "Method atIndex does not make sense for a bucket hash table!");
      return (def_val);
   }
   

  /**
   * Finds the index of an element into hashmap.
   *
   * @param item the key to search into the map.
   *
   * @return if element is found then returns its index (from 0 to SIZE-1).
   * If not found then return (-1).
   */
   int indexOf (const KeyType& item) const
   {
      assert(! "Method indexOf does not make sense for a bucket hash table!");
      return (-1);
   }


  /**
   * Membership check for a hashmap.
   *
   * @param item the key to check membership of.
   *
   * @return 1 if its a member, 0 otherwise.
   */
   inline int is_member (const KeyType& item)
#if !HASHMAP_OPTIMIZE_MRU
       const
#endif
   {
      unsigned int e = compute_hash(item);
      bucket_elem *iter = buckets[e].first;
      if (iter != 0)  // we have some elements mapping to this entry
      {
#if HASHMAP_OPTIMIZE_MRU
         bucket_elem *prev_iter = 0;
#endif
         while (iter && iter->key!=item)
         {
#if HASHMAP_OPTIMIZE_MRU
            prev_iter = iter;
#endif
            iter = iter->next;
         }
         if (! iter)
            return (0);
#if HASHMAP_OPTIMIZE_MRU
         else 
         {
            if (prev_iter)  // not in MRU position already
            {
               prev_iter->next = iter->next;
               iter->next = buckets[e].first;
               buckets[e].first = iter;
            }
         }
#endif
      } else
         return (0);
      return (1);
   }

   inline int is_member (const KeyType& item, unsigned int pre_hash)
#if !HASHMAP_OPTIMIZE_MRU
       const
#endif
   {
      unsigned int e = pre_hash & mask;
      bucket_elem *iter = buckets[e].first;
      if (iter != 0)  // we have some elements mapping to this entry
      {
#if HASHMAP_OPTIMIZE_MRU
         bucket_elem *prev_iter = 0;
#endif
         while (iter && iter->key!=item)
         {
#if HASHMAP_OPTIMIZE_MRU
            prev_iter = iter;
#endif
            iter = iter->next;
         }
         if (! iter)
            return (0);
#if HASHMAP_OPTIMIZE_MRU
         else 
         {
            if (prev_iter)  // not in MRU position already
            {
               prev_iter->next = iter->next;
               iter->next = buckets[e].first;
               buckets[e].first = iter;
            }
         }
#endif
      } else
         return (0);
      return (1);
   }
   
   
  /**
   * Removes an element from the hashmap.
   *
   * @param item the item to remove.
   *
   * @return 1 if it was removed, 0 otherwise.
   */
   inline int erase (const KeyType& item)
   {
      unsigned int e = compute_hash(item);
      bucket_elem *iter = buckets[e].first;
      if (iter != 0)  // we have some elements mapping to this entry
      {
         bucket_elem *prev_iter = 0;
         while (iter && iter->key!=item)
         {
            prev_iter = iter;
            iter = iter->next;
         }
         
         if (! iter)
         {
            return (0);
            // We must have at least one other element in the bucket
            // and the "last" pointer does not change.
         } else
         {
            if (prev_iter)
               prev_iter->next = iter->next;
            else
            {
               buckets[e].first = iter->next;
               if (! iter->next)
                  used_entries -= 1;
            }
            
            num_elems -= 1;
            if (cached && iter->key==lastKey)
               cached = false;
            bucket_allocator->delete_elem(iter);
            return (1);   // it is already in the MRU position
         }
      } else
      {
         return (0);
      }
      return (0);
   }

   inline int erase (const KeyType& item, unsigned int pre_hash)
   {
      unsigned int e = pre_hash & mask;
      bucket_elem *iter = buckets[e].first;
      if (iter != 0)  // we have some elements mapping to this entry
      {
         bucket_elem *prev_iter = 0;
         while (iter && iter->key!=item)
         {
            prev_iter = iter;
            iter = iter->next;
         }
         
         if (! iter)
         {
            return (0);
            // We must have at least one other element in the bucket
            // and the "last" pointer does not change.
         } else
         {
            if (prev_iter)
               prev_iter->next = iter->next;
            else
            {
               buckets[e].first = iter->next;
               if (! iter->next)
                  used_entries -= 1;
            }
            
            num_elems -= 1;
            if (cached && iter->key==lastKey)
               cached = false;
            bucket_allocator->delete_elem(iter);
            return (1);   // it is already in the MRU position
         }
      } else
      {
         return (0);
      }
      return (0);
   }

   
   /**
    * Prints statistics about the ocupancy of the hashtable in the specified 
    * file. It prints a distribution of the element counts across all buckets.
    *
    * @param fd the output file pointer, default stdout.
    *
    * @return the number of elements in the largest bucket.
    */
   int print_stats(FILE *fd = stdout) const
   {
      int max_len = 0;
      int i;
      int *lengths = new int[num_entries];  // I think this zeroes them out
      for (i=0 ; i<(int)num_entries ; ++i)
      {
         lengths[i] = 0;
         if (buckets[i].first)
         {
            bucket_elem *iter = buckets[i].first;
            while (iter)
            {
               lengths[i] += 1;
               iter = iter->next;
            }
         }
         if (lengths[i] > max_len)
            max_len = lengths[i];
      }

      int *histo = new int[max_len+1];  // I think this zeroes them out
      memset(histo, 0, (max_len+1)*sizeof(int));
      for (i=0 ; i<(int)num_entries ; ++i)
      {
         assert (lengths[i]>=0 && lengths[i]<=max_len);
         histo[lengths[i]] += 1;
      }
      fprintf(fd, "Number of elements in the table %u, number of entries in use %d (out of %d).\n" 
                  "Max bucket occupancy %d. Number of buckets organized by occupancy:\n", 
                  num_elems, used_entries, num_entries, max_len);
      assert(histo[max_len] > 0);
      fprintf(fd, "%d: %d", max_len, histo[max_len]);
      for (i=max_len-1 ; i>=0 ; --i)
         if (histo[i] > 0)
            fprintf(fd, ",  %d: %d", i, histo[i]);
      fprintf(fd, "\n");
      delete[] histo;
      delete[] lengths;
      return (max_len);
   }


  /**
   * Saves all the elements (key, value) in the specified file.
   *
   * @param fd the output file pointer.
   *
   * @return the number of elements effectively written in the file.
   */
   int dump_to_file_as_text(FILE* fd) const
   {
      fprintf (fd, "Size %d\n", num_elems);
      for (int i=0 ; i<num_entries ; ++i)
      {
         if (buckets[i].first)
         {
            bucket_elem *iter = buckets[i].first;
            while (iter)
            {
               if (sizeof(KeyType) == 8)
                  fprintf (fd, "%016x: ", iter->key);
               else  // it should be 64 bit value?
                  fprintf (fd, "%08x: ", iter->key);
               iter->val.dump_to_file (fd);
               
               iter = iter->next;
            }
         }
      }
      return (num_elems);
   }

   int dump_to_file (FILE* fd) const
   {
      int res = 0;
      fwrite (&num_elems, sizeof(unsigned int), 1, fd);
      for (int i=0 ; i<num_entries ; ++i)
      {
         if (buckets[i].first)
         {
            bucket_elem *iter = buckets[i].first;
            while (iter)
            {
               int r = fwrite(&iter->key, sizeof(KeyType), 1, fd);
               if (r == fwrite(&iter->val, sizeof(KeyType), 1, fd))
                  res += r;
               
               iter = iter->next;
            }
         }
      }

      return (res);
   }


   int dump_to_file_special (FILE* fd, int level) const
   {
      if (level>1)
      {
         fprintf (fd, "Size %d\n", num_elems);
         for (int i=0 ; i<num_entries ; ++i)
         {
            if (buckets[i].first)
            {
               bucket_elem *iter = buckets[i].first;
               while (iter)
               {
                  fprintf (fd, "%08x: ", iter->key);
                  ((BucketHashMap*)iter->val)->dump_to_file_special(fd, level-1);
                  iter = iter->next;
               }
            }
         }
      }
      else   // level == 1
      {
         for (int i=0 ; i<num_entries ; ++i)
         {
            if (buckets[i].first)
            {
               bucket_elem *iter = buckets[i].first;
               while (iter)
               {
                  fprintf(fd, "%d (%d),  ", iter->key, iter->val);
                  iter = iter->next;
               }
            }
         }
         fprintf(fd, "\n");
      }
      return (num_elems);
   }
   

   int map (MapFunction func, void* arg0) const
   {
      for (unsigned int i=0 ; i<num_entries ; ++i)
      {
         if (buckets[i].first)
         {
            bucket_elem *iter = buckets[i].first;
            while (iter)
            {
               func (arg0, iter->key, &iter->val);
               iter = iter->next;
            }
         }
      }
      return (num_elems);
   }
   
   inline static unsigned int* pre_hash_vec(int n, const KeyType* items, unsigned int* out)
   {
      for (int i=0 ; i<n ; ++i)
      {
#pragma Loop_Optimize (Unroll, Vector);
//         register KeyType key = items[i];
         register uint64_t key = (uint64_t)items[i];
         key += ~(key << 15);
         key ^=  (key >> 10);
         key +=  (key << 3);
         key ^=  (key >> 6);
         key += ~(key << 11);
         out[i] = key ^ (key >> 16);
      }
      return (out);
   }

   inline static unsigned int pre_hash(const KeyType& item)
   {
//      register KeyType key = item;
      register uint64_t key = (uint64_t)item;
      key += ~(key << 15);
      key ^=  (key >> 10);
      key +=  (key << 3);
      key ^=  (key >> 6);
      key += ~(key << 11);
      return (key ^ (key >> 16));
   }
   
private:
   inline unsigned int compute_hash(const KeyType& item) const
   {
      register uint64_t key = (uint64_t)item;
      key += ~(key << 15);
      key ^=  (key >> 10);
      key +=  (key << 3);
      key ^=  (key >> 6);
      key += ~(key << 11);
      key ^=  (key >> 16);
      return (key & mask);
   }
   
   inline void grow_table()
   {
      unsigned int oldne = num_entries;
      BucketEntry *temp = buckets;
      
      num_entries <<= 2;
      mask = num_entries - 1;  // this assumes that table size is a power of 2;
      buckets = new BucketEntry[num_entries];
      used_entries = 0;

      // Rehash all the elements
      for (unsigned int i=0 ; i<oldne ; ++i)
      {
         if (temp[i].first)
         {
            bucket_elem *elem = temp[i].first;
            while (elem)
            {
               bucket_elem *next = elem->next;
               
               // move iter to the correct position in the new table
               unsigned int e = compute_hash(elem->key);
               bucket_elem *iter = buckets[e].first;
               if (iter != 0)  // we have some elements mapping to this entry
               {
                  // since we are just reinserting the old elements, we
                  // cannot have any duplicates
                  if (iter->key == elem->key)
                  {
                     fprintf(stderr, "BucketHashMap::grow_table: elem=%d/%d, oldne=%d, num_entries=%d, mask=0x%x, e=%d, iter(key,val)=(%llx,%p), elem(key,val)=(%llx,%p), iter=%p, elem=%p\n",
                        i, num_elems, oldne, num_entries, mask, e, 
                        (long long)iter->key, &(iter->val), 
                        (long long)elem->key, &(elem->val), 
                        iter, elem);
                     assert (iter->key != elem->key);
                  }
                  elem->next = iter;
                  buckets[e].first = elem;
               } else
               {
                  // this is the first hit to this bucket
                  used_entries += 1;
                  buckets[e].first = elem;
                  elem->next = 0;
               }
               
               elem = next;
            }  /* loop over the linked elements */
         }  /* if the entry contains any element */
      }  /* iterate over the old entries */
      
      delete[] temp;
   }
   
   inline bucket_elem* insert_core(unsigned int e, const KeyType& item)
   {
      bucket_elem *iter = buckets[e].first;
      if (iter != 0)  // we have some elements mapping to this entry
      {
#if HASHMAP_OPTIMIZE_MRU
         bucket_elem *prev_iter = 0;
#endif
         while (iter && iter->key!=item)
         {
#if HASHMAP_OPTIMIZE_MRU
            prev_iter = iter;
#endif
            iter = iter->next;
         }
         if (! iter)
         {
            iter = bucket_allocator->new_elem();
            num_elems += 1;
            iter->next = buckets[e].first;
            iter->key = item;
            iter->val = def_val;
            buckets[e].first = iter;
            // We must have at least one other element in the bucket
            // and the "last" pointer does not change.
            //
            // grow the table after we inserted this element;
            // if we grow before inserting, we would use a stale
            // table entry
            if (num_elems >= (num_entries<<3))  // average occupancy is > 8
               grow_table();
         }
#if HASHMAP_OPTIMIZE_MRU
         else 
         {
            if (prev_iter)  // not in MRU position already
            {
               prev_iter->next = iter->next;
               iter->next = buckets[e].first;
               buckets[e].first = iter;
            }
         }
#endif
      } else
      {
         // this is the first hit to this bucket
         iter = bucket_allocator->new_elem();
         num_elems += 1;
         used_entries += 1;
         iter->next = buckets[e].first;
         iter->key = item;
         iter->val = def_val;
         buckets[e].first = iter;
      }
      return (iter);
   }

   alloc_type* bucket_allocator;
   BucketEntry *buckets;
   unsigned int used_entries; // number of buckets with at least 1 elem
   unsigned int num_elems;    // total number of elements in the table
   unsigned int num_entries;
   unsigned int mask;
   ValType def_val;
   
   // implement a one element cache
   bucket_elem* lastElem;
   KeyType lastKey;
   bool cached;

public:
   class iterator
   {
   };
};

} /* namespace MIAMIU */

#endif
