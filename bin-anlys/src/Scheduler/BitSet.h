/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: BitSet.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines a bit set data structure.
 */

#ifndef _BITSET_H
#define _BITSET_H

#include <stdio.h>
#include <sys/types.h>
#include <list>
#include "OAUtils/Iterator.h"
#include "miami_types.h"

namespace MIAMI
{

// define a bit set class
class BitSet
{
#define WORD_SIZE 32
#define WORD_MASK 31
#define WORD_SHIFT 5

public:
   // create an empty set for elements from 0 to _size-1
   BitSet(unsigned int _size);
   // create a copy of a set
   BitSet(const BitSet& bs);
   ~BitSet();
   
   // assignment operator
   BitSet& operator= (const BitSet& bs);
   
   // OR (set reunion)
   BitSet operator| (const BitSet& bs) const;
   BitSet& operator|= (const BitSet& bs);

   // AND (set intersection)
   BitSet operator& (const BitSet& bs) const;
   BitSet& operator&= (const BitSet& bs);

   // XOR between sets
   BitSet operator^ (const BitSet& bs) const;
   BitSet& operator^= (const BitSet& bs);

   // set difference A-B = A - (A&B) = A&(~B)
   BitSet operator- (const BitSet& bs) const;
   BitSet& operator-= (const BitSet& bs);

   // insert a set (reunion)
   BitSet operator+ (const BitSet& bs) const;
   BitSet& operator+= (const BitSet& bs);

   // insert an element
   BitSet operator+ (unsigned int e) const;
   BitSet& operator+= (unsigned int e);

   // remove an element
   BitSet operator- (unsigned int e) const;
   BitSet& operator-= (unsigned int e);
   
   // complement of a set (elements not in set)
   BitSet operator~ () const;
   
   // check if two sets are equal
   bool operator== (const BitSet& bs) const;
   
   // set is not empty
   operator bool() const;
   // element i is in set
   bool operator[] (unsigned int e) const;
   
   unsigned int Capacity() { return (size); }
   unsigned int Size();
   
   class BitSetIterator;
   friend class BitSetIterator;
   
   class BitSetIterator : public Iterator
   {
#define END_BIT_SET 0xffffffff

   public:
      BitSetIterator(BitSet& _bs)
      {
         bs = &_bs;
         wordIdx = 0;
         bit = 0;
         while(wordIdx<bs->num_words && bs->the_set[wordIdx]==0)
            wordIdx += 1;
         if (wordIdx>=bs->num_words)  // set is empty
            iter = END_BIT_SET;
         else
         {
            // found a non empty word; 
            // must be extra careful if this is the last word
            bit = 0;
            iter = wordIdx << WORD_SHIFT;
            uint32_t crtWord = bs->the_set[wordIdx];
            while( (crtWord & 1)==0 && iter<bs->size)
            {
               bit += 1; iter += 1; crtWord >>= 1;
            }
            if (iter>=bs->size)
               iter = END_BIT_SET;
         }
      }
      ~BitSetIterator () {}
      void operator++ () 
      {
         if (iter == END_BIT_SET) return;
         uint32_t crtWord = bs->the_set[wordIdx] >> bit;
         bit += 1;
         iter += 1;
         crtWord >>= 1;
         if (crtWord==0)
         {
            wordIdx += 1;
            while(wordIdx<bs->num_words && bs->the_set[wordIdx]==0)
               wordIdx += 1;
            if (wordIdx>=bs->num_words)  // set is empty
            {
               iter = END_BIT_SET;
               return;
            }
            // else found a non empty word
            bit = 0;
            iter = wordIdx << WORD_SHIFT;
            crtWord = bs->the_set[wordIdx];
         }
         // crtWord has some 1 bit; find the first one
         while( (crtWord & 1)==0 && iter<bs->size)
         {
            bit += 1; iter += 1; crtWord >>= 1;
         }
         if (iter>=bs->size)
            iter = END_BIT_SET;
      }
      operator bool () const { return (iter != END_BIT_SET); }  
      operator unsigned int () const { return iter; }
      operator int () const { return iter; }
      
      BitSetIterator& operator= (const BitSetIterator& bsi)
      {
         bs = bsi.bs;
         wordIdx = bsi.wordIdx;
         bit = bsi.bit;
         iter = bsi.iter;
         return (*this);
      }

   private:
      BitSet* bs;
      unsigned int wordIdx;
      int bit;
      unsigned int iter;
   };
   
private:
   unsigned int size;
   unsigned int num_words;
   uint32_t *the_set;
};

BitSet operator+ (unsigned int e, const BitSet& bs);

typedef std::list<BitSet*> BSetList;

} /* namespace MIAMI */

#endif
