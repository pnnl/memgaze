/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: BitSet.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements a bit set data structure.
 */

#include "BitSet.h"
#include <assert.h>

using namespace MIAMI;
/* TODO: FIX a bunch of signed / unsigned comparisons about which the compiler complains
 */
BitSet::BitSet(unsigned int _size) : size(_size)
{
   unsigned int i;
   assert(sizeof(uint32_t) == 4);
   num_words = (size + WORD_SIZE - 1) / WORD_SIZE;
   assert(num_words > 0);
   the_set = new uint32_t[num_words];
   for (i=0 ; i<num_words ; ++i)
      the_set[i] = 0;
}
   
BitSet::BitSet(const BitSet& bs)
{
   size = bs.size;
   num_words = bs.num_words;
   the_set = new uint32_t[num_words];
   for (unsigned int i=0 ; i<num_words ; ++i)
      the_set[i] = bs.the_set[i];
}

BitSet::~BitSet()
{
   delete[] the_set;
}

// the ASSIGNMENT operation (copy set)
BitSet& 
BitSet::operator= (const BitSet& bs)
{
   size = bs.size;
   num_words = bs.num_words;
   // delete the old occupied memory;
   delete[] the_set;
   the_set = new uint32_t[num_words];
   for (unsigned int i=0 ; i<num_words ; ++i)
      the_set[i] = bs.the_set[i];
   return (*this);
}

// the set OR operation (reunion)
BitSet 
BitSet::operator| (const BitSet& bs) const
{
   assert(size == bs.size);
   BitSet temp(bs);
   for (unsigned int i=0 ; i<num_words ; ++i)
      temp.the_set[i] |= the_set[i];
   return (temp);
}

BitSet& 
BitSet::operator|= (const BitSet& bs)
{
   assert(size == bs.size);
   for (unsigned int i=0 ; i<num_words ; ++i)
      the_set[i] |= bs.the_set[i];
   return (*this);
}

// the set AND operation (intersection)
BitSet 
BitSet::operator& (const BitSet& bs) const
{
   assert(size == bs.size);
   BitSet temp(bs);
   for (unsigned int i=0 ; i<num_words ; ++i)
      temp.the_set[i] &= the_set[i];
   return (temp);
}

BitSet& 
BitSet::operator&= (const BitSet& bs)
{
   assert(size == bs.size);
   for (unsigned int i=0 ; i<num_words ; ++i)
      the_set[i] &= bs.the_set[i];
   return (*this);
}

// the set XOR operation
BitSet 
BitSet::operator^ (const BitSet& bs) const
{
   assert(size == bs.size);
   BitSet temp(bs);
   for (unsigned int i=0 ; i<num_words ; ++i)
      temp.the_set[i] ^= the_set[i];
   return (temp);
}

BitSet& 
BitSet::operator^= (const BitSet& bs)
{
   assert(size == bs.size);
   for (unsigned int i=0 ; i<num_words ; ++i)
      the_set[i] ^= bs.the_set[i];
   return (*this);
}

// the PLUS operation, insert set in another set; equivalent to OR (reunion)
BitSet 
BitSet::operator+ (const BitSet& bs) const
{
   assert(size == bs.size);
   BitSet temp(bs);
   for (unsigned int i=0 ; i<num_words ; ++i)
      temp.the_set[i] |= the_set[i];
   return (temp);
}

BitSet& 
BitSet::operator+= (const BitSet& bs)
{
   assert(size == bs.size);
   for (unsigned int i=0 ; i<num_words ; ++i)
      the_set[i] |= bs.the_set[i];
   return (*this);
}

// set DIFFERENCE operation
BitSet 
BitSet::operator- (const BitSet& bs) const
{
   assert(size == bs.size);
   BitSet temp(*this);
   for (unsigned int i=0 ; i<num_words ; ++i)
      temp.the_set[i] &= ~(bs.the_set[i]);
   return (temp);
}

BitSet& 
BitSet::operator-= (const BitSet& bs)
{
   assert(size == bs.size);
   for (unsigned int i=0 ; i<num_words ; ++i)
      the_set[i] &= ~(bs.the_set[i]);
   return (*this);
}

// insert one element
BitSet 
BitSet::operator+ (unsigned int e) const
{
   if (e>=size)  // outside the range
      return (*this);   // return the current set unmodified
   BitSet temp(*this);
   int wordIdx = e >> WORD_SHIFT;
   int bit = e & WORD_MASK;
   temp.the_set[wordIdx] |= (1<<bit);
   return (temp);
}

BitSet& 
BitSet::operator+= (unsigned int e)
{
   if (e<size)  // if elem in range
   {
      int wordIdx = e >> WORD_SHIFT;
      int bit = e & WORD_MASK;
      the_set[wordIdx] |= (1<<bit);
   }
   return (*this);
}

// remove an element
BitSet 
BitSet::operator- (unsigned int e) const
{
   if (e>=size)  // outside the range
      return (*this);   // return the current set unmodified
   BitSet temp(*this);
   int wordIdx = e >> WORD_SHIFT;
   int bit = e & WORD_MASK;
   temp.the_set[wordIdx] &= (~(1<<bit));
   return (temp);
}

BitSet& 
BitSet::operator-= (unsigned int e)
{
   if (e<size)  // if elem in range
   {
      int wordIdx = e >> WORD_SHIFT;
      int bit = e & WORD_MASK;
      the_set[wordIdx] &= (~(1<<bit));
   }
   return (*this);
}

bool 
BitSet::operator== (const BitSet& bs) const
{
   if (size != bs.size)
      return (false);
   for (int i=0 ; i<(int)num_words-1 ; ++i)
      if (the_set[i] != bs.the_set[i])
         return (false);
   
   // the last word is special
   int bit = size & WORD_MASK;
   if (bit==0)
      return (the_set[num_words-1]==bs.the_set[num_words-1]);
   else
      return ((the_set[num_words-1] & ((1<<bit)-1)) == 
              (bs.the_set[num_words-1] & ((1<<bit)-1)));
}

   
BitSet::operator bool() const
{
   for (int i=0 ; i<(int)num_words-1 ; ++i)
      if (the_set[i])
         return (true);
   // the last word is special
   int bit = size & WORD_MASK;
   if (bit==0)
      return (the_set[num_words-1]!=0);
   else
      return ((the_set[num_words-1] & ((1<<bit)-1)) != 0);
}

// the complement of a set
BitSet 
BitSet::operator~ () const
{
   BitSet temp(*this);
   for (unsigned int i=0 ; i<num_words ; ++i)
      temp.the_set[i] = ~the_set[i];
   return (temp);
}
      
// test if element i is in set
bool 
BitSet::operator[] (unsigned int e) const
{
   if (e>=size)  // outside the range
      return (false); 
   int wordIdx = e >> WORD_SHIFT;
   int bit = e & WORD_MASK;
   return ((the_set[wordIdx] & (1<<bit)) != 0);
}

// non member operator that adds an element to a set
BitSet operator+ (unsigned int e, const BitSet& bs) 
{
   return (bs + e);
}

// number of elements in set
unsigned int 
BitSet::Size ()
{
   unsigned int _size = 0;
   unsigned int wordIdx = 0;
   do
   {
      while (wordIdx<num_words && the_set[wordIdx]==0)
         wordIdx += 1;
      if (wordIdx>=num_words)
         break;
      /* else, we have a non-zero word; careful, the last word might not be full
       */
      unsigned int iter = wordIdx << WORD_SHIFT;
      int bit = 0;
      uint32_t crtWord = the_set[wordIdx];
      while (crtWord && iter<size)
      {
         if (crtWord & 1)
            _size += 1;
         bit += 1; iter += 1; crtWord >>= 1;
      }
      if (iter>=size)
         break;
      wordIdx += 1;
   } while (wordIdx < num_words);
   return (_size);
}
