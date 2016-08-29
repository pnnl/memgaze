/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: block_path.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Data structure associated with a path in the CFG. A path
 * is defined by a vector of CFG basic blocks.
 */

#include "block_path.h"
#include "debug_scheduler.h"

#define EPSILON_VAL  0.00001

using namespace MIAMI;

int 
OrderBMs::operator() (const BMarker &x, const BMarker &y) const
{
   if (x.entry != y.entry) return (x.entry < y.entry);
   if (x.marker != y.marker) return (x.marker < y.marker);
   // if markers are equal, compare block addresses
   return (x.b < y.b);
}


BlockPath::BlockPath (CFG::NodeList &na, CFG::Node* _next_block, MIAMIU::FloatArray &fa, 
          RSIListList& _regList, bool _exit_path)
{
   size = na.size();
   if (size != fa.size())
   {
      fprintf (stderr, "Trying to create path with arrays of different size; size(na) %u, size(fa) %lu\n",
            size, fa.size());
      exit (-11);
   }
   if (size < 1)
   {
      printf ("WARNING: trying to add empty path");
      return;
   }
   hashVal = 0x10a8e423A7C4B3DEll;
   blocks = new CFG::Node*[size];
   probabs = new float[size];
   numLoops = 0;
   is_exit_path = _exit_path;
   int i = 0;
   for (MIAMIU::FloatArray::iterator fit=fa.begin() ; fit!=fa.end() ; ++fit, ++i)
   {
     probabs[i] = (*fit);
   }
   i = 0;
   for (CFG::NodeList::iterator it=na.begin() ; it!=na.end() ; ++it, ++i)
   {
     if ((*it)->is_inner_loop ())
     {
        ++ numLoops;
#if DEBUG_BLOCK_PATHS_PRINT
        DEBUG_PATHS (3, 
           fprintf (stderr, "BlockPath: found inner loop %d, at block index %d/%d, prob %f\n",
               numLoops, i, size, probabs[i]);
        )
#endif
     }
     // do not hash cfg entries since they have size zero and are going to annul
     // the next block which should start at the same address
     if ((*it)->isCfgEntry())
     {
#if DEBUG_BLOCK_PATHS_PRINT
        DEBUG_PATHS (3, 
           fprintf (stderr, "BlockPath: found cfg entry block at index %d/%d, prob %f\n",
               i, size, probabs[i]);
        )
#endif
     } else
     {
        hashVal ^= (*it)->getStartAddress();
        hashVal = (hashVal&0xffffffff00000000)>>32 | (hashVal&0x00000000ffffffff)<<32;
     }
     blocks[i] = (*it);
   }
   if (_next_block)
      hashVal ^= _next_block->getStartAddress();
   
   if (numLoops != _regList.size())
   {
      fprintf (stderr, "Trying to create path with register list size (%lu) different than the number of inner loops (%u).\n",
            _regList.size(), numLoops);
      exit (-11);
   }
   if (numLoops > 0)
      innerRegs = new RSIList [numLoops];
   else
      innerRegs = NULL;
   i = 0;
   for (RSIListList::iterator lit=_regList.begin() ; lit!=_regList.end() ; 
                  ++lit, ++i)
   {
      innerRegs[i] = (*lit);
   }
   next_block = _next_block;
}

BlockPath::BlockPath (BlockPath& bp)
{
   size = bp.size;
   hashVal = bp.hashVal;
   blocks = new CFG::Node*[size];
   memcpy (blocks, bp.blocks, sizeof(CFG::Node*)*size);
   probabs = new float[size];
   memcpy (probabs, bp.probabs, sizeof(float)*size);
   next_block = bp.next_block;
   numLoops = bp.numLoops;
   is_exit_path = bp.is_exit_path;
   if (numLoops>0)
   {
      innerRegs = new RSIList [numLoops];
      for (unsigned int i=0 ; i<numLoops ; ++i)
         innerRegs[i] = bp.innerRegs[i];
   } else
      innerRegs = NULL;
}

BlockPath::~BlockPath ()
{
   if (blocks)
      delete[] blocks;
   if (probabs)
      delete[] probabs;
   if (innerRegs)
      delete[] innerRegs;
}

void
BlockPath::trimFat ()
{
   if (blocks)
   {
      delete[] blocks;
      blocks = NULL;
   }
   if (probabs)
   {
      delete[] probabs;
      probabs = NULL;
   }
   if (innerRegs)
   {
      delete[] innerRegs;
      innerRegs = NULL;
   }
}
   
BlockPath& 
BlockPath::operator= (const BlockPath& bp)
{
   size = bp.size;
   hashVal = bp.hashVal;
   if (blocks)
      delete[] blocks;
   blocks = new CFG::Node*[size];
   memcpy (blocks, bp.blocks, sizeof(CFG::Node*)*size);
   probabs = new float[size];
   memcpy (probabs, bp.probabs, sizeof(float)*size);
   next_block = bp.next_block;
   numLoops = bp.numLoops;
   is_exit_path = bp.is_exit_path;
   if (numLoops>0)
   {
      innerRegs = new RSIList [numLoops];
      for (unsigned int i=0 ; i<numLoops ; ++i)
         innerRegs[i] = bp.innerRegs[i];
   } else
      innerRegs = NULL;
   return (*this);
}

// I think any patch I add should be between inner loops. It does not affect
// the innerRegs array.
void 
BlockPath::include (int pos, CFG::NodeList& na, MIAMIU::FloatArray& fa)
{
   assert (pos>=0 && pos<(int)size);
   unsigned int size2 = na.size();
   if (size2 + 1 != fa.size())
   {
      fprintf (stderr, "Trying to include path patch with arrays of incorrect size; size(na) %u, size(fa) %lu\n",
            size2, fa.size());
      exit (-11);
   }
   // the fa array has one more element because I need to recompute the probability
   // of the block after which I insert this new patch. From that block I am going
   // to take a different edge than I initially thought. That means the first
   // probability value in the array is going to replace an existing value just at
   // the insertion point.
   
   if (size2 == 0) return;

   CFG::Node** temp = blocks;
   blocks = new CFG::Node*[size+size2];
   memcpy (blocks, temp, sizeof(CFG::Node*)*(pos+1));

   float* ftemp = probabs;
   probabs = new float[size+size2];
   memcpy (probabs, ftemp, sizeof(float)*(pos));

   int i=pos+1;
   for (CFG::NodeList::iterator it=na.begin() ; it!=na.end() ; ++it, ++i)
   {
     if ((*it)->isCfgEntry())
        fprintf (stderr, "BlockPath: include cfg entry block into the path at position %d\n",
            i);
     else
        hashVal ^= (*it)->getStartAddress();
     blocks[i] = (*it);
   }
   i = pos;
   for (MIAMIU::FloatArray::iterator fit=fa.begin() ; fit!=fa.end() ; ++fit, ++i)
   {
     probabs[i] = (*fit);
   }

   memcpy (&blocks[pos+size2+1], &temp[pos+1], sizeof(CFG::Node*)*(size-pos-1));
   memcpy (&probabs[pos+size2+1], &ftemp[pos+1], sizeof(float)*(size-pos-1));
   size += size2;
   delete[] temp;
   delete[] ftemp;
}

bool 
BlockPath::CompareKey::operator() (const BlockPath* k1, const BlockPath* k2) const
{
   // sort paths such that paths with same prefix are consecutive
   // we can sort more optimal if we use the hash value
   // but in this way we can optimize the scheduling of the paths
   // mgabi 8/12/2004: I do not think we optimize the scheduling or that
   // we could even optimize it. We could try to revert to use the hashVal
   if (k1->size < k2->size)
      return true;
   if (k1->size > k2->size)
      return false;
   if (k1->hashVal < k2->hashVal)
      return true;
   if (k1->hashVal > k2->hashVal)
      return false;

   for (unsigned int i=0 ; i<k1->size && i<k2->size ; ++i)
   {
      if (k1->blocks[i]->getStartAddress() < k2->blocks[i]->getStartAddress())
         return true;
      if (k1->blocks[i]->getStartAddress() > k2->blocks[i]->getStartAddress())
         return false;
   }
   
   return (k1->next_block->getStartAddress() < k2->next_block->getStartAddress());
}


void 
BlockPath::Dump(FILE *fout)
{
   unsigned int i;
   fprintf (fout, "Path %016" PRIx64 " blocks (%" PRIu32 "):", hashVal, size);
   for (i=0 ; i<size ; ++i)
   {
      fprintf (fout, " - [0x%lx,0x%lx](%g)", 
              blocks[i]->getStartAddress(), blocks[i]->getEndAddress(),
              probabs[i]);
   }
   fprintf (fout, "  :: [0x%lx,0x%lx]", next_block->getStartAddress(), next_block->getEndAddress());
   if (is_exit_path)
      fprintf (fout, " [exit]");
   fprintf (fout, "\n");
}


