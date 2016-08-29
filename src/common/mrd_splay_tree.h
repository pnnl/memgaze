/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: mrd_splay_tree.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A custom splay tree implementation for computing memory
 * reuse distances.
 */

#ifndef _MRD_SPLAY_TREE_H
#define _MRD_SPLAY_TREE_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <deque>

#include "miami_allocator.h"

namespace MIAMIU  /* MIAMI Utils */
{

class MRDTreeNode
{
public:
   MRDTreeNode()
   {
      left = right = parent = NULL;
      size = 0;
   }

   MRDTreeNode(MRDTreeNode* _parent) : parent(_parent)
   {
      left = right = NULL;
      size = 1;
   }
   
   MRDTreeNode(const MRDTreeNode& tn)
   {
#ifdef SPLAY_TREE_HAS_KEY_IN_NODE
      key = tn.key;
#endif
      left = tn.left; right = tn.right;
      parent = tn.parent;
      size = tn.size;
   }
   
   inline MRDTreeNode& operator= (const MRDTreeNode& tn)
   {
#ifdef SPLAY_TREE_HAS_KEY_IN_NODE
      key = tn.key;
#endif
      left = tn.left; right = tn.right;
      parent = tn.parent;
      size = tn.size;
      return (*this);
   }
   
   // Can create a set of inline functions to get/set each field.
   // For now they are public so anyone can access them.
   
#ifdef SPLAY_TREE_HAS_KEY_IN_NODE
   uint64_t key;
#endif
   MRDTreeNode* left;
   MRDTreeNode* right;
   MRDTreeNode* parent;
   unsigned int size;
};

#define PROFILE_REUSE 0

#ifdef SPLAY_TREE_HAS_KEY_IN_NODE
 static const int defAlign = 8;
#else
 static const int defAlign = 8;
#endif
template<unsigned int def_chunk_size = 4096,
         class Allocator = MiamiAllocator <MRDTreeNode, defAlign > >
class MRDSplayTree
{
public:
   typedef MRDTreeNode  node_type;
   typedef Allocator    alloc_type;
   int max_path_length;
#if PROFILE_REUSE
   int n_reuse_block, n_new_block;
   int *c_reuse_block;
   int *c_new_block;
#endif

   MRDSplayTree(unsigned int block_size = def_chunk_size)
   {
      splay_allocator = new alloc_type(block_size);
      tree_size = 0;
      lastN = rootN = NULL;
      char *path = getenv("SPLAY_PATH_LENGTH");
      if (path)
      {
         max_path_length = strtol(path, NULL, 10);
         fprintf(stderr, "Splay tree will use a path length threshold of %d\n", max_path_length);
      }
      else
         max_path_length = 50;
#if PROFILE_REUSE
      n_reuse_block = n_new_block = 64;
      c_reuse_block = (int*)malloc(n_reuse_block*sizeof(int));
      c_new_block = (int*)malloc(n_new_block*sizeof(int));
      for (int i=0 ; i<n_new_block ; ++i)
         c_new_block[i] = c_reuse_block[i] = 0;
#endif
   }
   
   ~MRDSplayTree()
   {
#if PROFILE_REUSE
      // print the collected statistics
      int i;
      fprintf(stderr, "Distribution of path lengths when reusing blocks (n=%d)\n", n_reuse_block);
      for (i=0 ; i<n_reuse_block ; ++i)
         if (c_reuse_block[i]>0)
            fprintf(stderr, " - %d = %d\n", i, c_reuse_block[i]);
      fprintf(stderr, "Distribution of path lengths when adding blocks (n=%d)\n", n_new_block);
      for (i=0 ; i<n_new_block ; ++i)
         if (c_new_block[i]>0)
            fprintf(stderr, " - %d = %d\n", i, c_new_block[i]);
      if (c_reuse_block)
         free(c_reuse_block);
      if (c_new_block)
         free(c_new_block);
#endif

      delete splay_allocator;
   }
   
   /*
    * this self-adjuting tree is tailored to computing reuse distance;
    * it implements only two public operations:
    *   - cold_miss = inserts a node as the right child of the last node
    *        and then propagates it to the root using the zig-zag rotations;
    *        in addition, it updates the size field for the nodes involved 
    *        in rotations, taking care to add 1, where necessary, for the 
    *        new node; this method returns a pointer to the new node;
    *   - reuse_block = receives a pointer to the node that is accessed;
    *        it must compute the reversed rank of the accessed node, going
    *        up in the tree as long as necessary, and then moves this node
    *        as the last node and do the rotations to move it to the root;
    *        this method returns the computed reversed rank (=reuse distance)
    * optionally, the tree has a third method:
    *   - count_blocks = receives a key and starts from the root of the tree
    *        searching for a node with the lowest key that is greater or equal
    *        to the received key. It computes the reversed rank (reuse 
    *        distance) for the found node. This method is optional because 
    *        it requires having the key field in each node, which is not 
    *        included by default.
    */

   void* cold_miss()
   {
      register node_type *n = splay_allocator->new_elem();  //lastN);
      n->left = n->right = NULL;
      n->parent = lastN;
      n->size = 1;
      ++ tree_size;
      if (lastN==NULL)
      {
         rootN = lastN = n;
      } else
      {
         lastN->right = n;
         // I have to propagate up to root to update the size field at least
         // should I splay the tree also? I am worried that if I keep 
         // inserting nodes without reusing them, the tree will degenerate to
         // a list in both cases. So better do not spend time on rotations now
         register node_type* iter = lastN;
         register int lengthPath = 0;
         do 
         {
            iter->size += 1;
            iter = iter->parent;
            ++ lengthPath;
         } while (iter != NULL);
         lastN = n;
         if (lengthPath > max_path_length)  
         // splay from the new node only if we have at
         // least max_path_length nodes on the path to the root
         {
            // splay starting from n (lastN)
            // I already know a few things. lastN must be on the right side of
            // its parent, and its parent must be on the right side of its 
            // grandparent (if it has one), and we know it does because the
            // lengthPath is at least 10
            // I also know the lastN has no children because it is a new node
            register node_type* p = n->parent;
            register node_type* subn = 0; // the left child of n is initially nil
            register node_type* g = p->parent;

            // compute first rotation without any conditionals because we know
            // all the relevant info
            p->right = 0;
            g->right = p->left;
            if (p->left)
               p->left->parent = g;
            p->left = g;
            p->size = g->size - 1;
            g->size -= 2;
            subn = p;    // not NULL for the remaining rotations
            // n->left = subn;  I will set it only at the end
            p = g->parent;
            g->parent = subn;

            while(p)
            {
               p->right = subn;
               subn->parent = p;
               g = p->parent;  // the grandparent node
               if (g!=NULL)
               {
                  g->right = p->left;
                  if (p->left)
                     p->left->parent = g;
                  p->left = g;
                  p->size = g->size - 1;
                  g->size -= (subn->size + 2);
                  subn = p;
                  p = g->parent;
                  g->parent = subn;
                  
               } else  // g is NULL, p is the root node
               {
                  p->size -= 1;
                  subn = p;
                  p = p->parent;   // should be NULL
               }
            } /* while(p)  ; DID YOU FIX SIZE FOR subn and the other nodes? */
            // at this point subn holds the old root with size updated for
            // all nodes underneath; I just have to make subn as the left
            // child of lastN
            subn->parent = lastN;
            lastN->left = subn;
            lastN->parent = 0;
            lastN->size = subn->size + 1;
            rootN = lastN;
         }
      }
      return (n);
   }

#ifdef SPLAY_TREE_HAS_KEY_IN_NODE
   unsigned long count_blocks (uint64_t _key)
   {
      if (!rootN)
         return (0);
      // check that rootN is correct
//      assert (!rootN->parent);
      register node_type *n = rootN;
      register unsigned long distance = 0;
      register node_type *iter = NULL;
      do {
         if (n->key == _key)
         {
            iter = n->right;
            if (iter)
               distance += iter->size;
            break;
         }
         if (_key < n->key)   // we are looking for a smaller key
         {
            iter = n->left;
            distance += n->size;
            if (iter)
               distance -= iter->size;
            n = iter;
         } else    // we are looking for a larger key
            n = n->right;
      } while (n);
      return (distance);
   }
#endif
   
   inline unsigned long reuse_block(void *_n)
   {
      register node_type *n = (node_type*)_n;
      // if we access the last accessed node, no change is needed.
      // Do not even propagate this node to the root
      if (n==lastN)
      {
#if PROFILE_REUSE
         c_reuse_block[0] += 1;
#endif
         return (0);
      }
      
      register unsigned long distance = 0;
#if PROFILE_REUSE
      int treeHeight = 1;
#endif

      // if (n) has children, I have to join the left and right subtrees
      register node_type* subn;
      if (n->right == NULL)
         subn = n->left;
      else 
      {
         subn = n->right;
         distance += subn->size;
         // If both children are not NULL, then
         // find the succesor of (n) and make it the replacement for n;
         if (n->left != NULL)
         {
            register node_type* iter = subn->left;
            if (iter != NULL)  // subn (n->right) is not the successor
            {
               subn->size -= 1;
               while (iter->left != NULL)
               {
                  subn = iter;
                  iter = iter->left;
                  subn->size -= 1;
               }
               // at this point, iter is the successor node
               subn->left = iter->right;  // iter->left is NULL
               if (iter->right)
                  iter->right->parent = subn;
               subn = iter;
               subn->right = n->right;
               subn->right->parent = subn;
            }
            subn->left = n->left;
            subn->left->parent = subn;
            subn->size = n->size - 1;
         }
      }

      // I create a new syntax block, to "force" some variable allocation into
      // registers
      {
         register node_type* p = n->parent;
         if (subn != NULL)
            subn->parent = p;

         if (p)
         {
            if (p->left == n)
            {
               p->left = subn;
               distance += (p->size - n->size);
            } else
               p->right = subn;

            subn = p;
            p = subn->parent;
            register int pleft = 0;
            if (p)
               pleft = (p->left==subn);
            else
               subn->size -= 1;
#if PROFILE_REUSE
            treeHeight += 1;
#endif

            // I have to substract 1 from the size of all nodes that were
            // on the path to the root in the initial tree, starting from subn
            while(p != NULL)
            {
               register node_type* g = p->parent;  // the grandparent node
               if (g!=NULL)
               {
                  register int gleft = (g->left == p);
                  register int tempsize1 = g->size - p->size;
                  register int tempsize2 = p->size - subn->size;
                  
                  subn->parent = g->parent;
                  p->parent = subn;
                  subn->size = g->size;
                  g->size = tempsize1;
                  p->size = tempsize2;
                  if (pleft)
                  {
                     distance += tempsize2;
                     p->left = subn->right;
                     subn->right = p;
                     if (p->left)
                     {
                        p->left->parent = p;
                        p->size += p->left->size;
                     }
                        
                     if (gleft)
                     {
                        distance += tempsize1;
                        p->size += tempsize1;
                        g->parent = p;
                        g->left = p->right;
                        p->right = g;
                        if (g->left)
                        {
                           g->left->parent = g;
                           g->size += (tempsize2 - 1);
                        }
                     } else
                     {
                        g->parent = subn;
                        g->right = subn->left;
                        subn->left = g;
                        if (g->right)
                        {
                           g->right->parent = g;
                           g->size += g->right->size;
                        }
                     }
                  }
                  else
                  {
                     p->right = subn->left;
                     subn->left = p;
                     if (p->right)
                     {
                        p->right->parent = p;
                        p->size += p->right->size;
                     }
                        
                     if (gleft)
                     {
                        distance += tempsize1;
                        g->parent = subn;
                        g->left = subn->right;
                        subn->right = g;
                        if (g->left)
                        {
                           g->left->parent = g;
                           g->size += g->left->size;
                        }
                     } else
                     {
                        p->size += tempsize1;
                        g->parent = p;
                        g->right = p->left;
                        p->left = g;
                        if (g->right)
                        {
                           g->right->parent = g;
                           g->size += (tempsize2 - 1);
                        }
                     }
                  }
                  
                  p = subn->parent;
                  if (p)
                     pleft = (p->left == g);
                  else
                  {
                     subn->size -= 1;
//                     rootN = subn;
                  }
#if PROFILE_REUSE
                  treeHeight += 1;
#endif
               } else  // g is NULL, p is the root node
               {
                  register int tempsize = p->size - subn->size;
                  subn->parent = NULL;
                  p->parent = subn;
                  subn->size = p->size - 1;
                  p->size = tempsize;
#if PROFILE_REUSE
                  treeHeight += 1;
#endif
                  if (pleft)
                  {
                     distance += tempsize;
                     p->left = subn->right;
                     subn->right = p;
                     if (p->left != NULL)
                     {
                        p->left->parent = p;
                        p->size += p->left->size;
                     }
                  } else
                  {
                     p->right = subn->left;
                     subn->left = p;
                     if (p->right != NULL)
                     {
                        p->right->parent = p;
                        p->size += p->right->size;
                     }
                  }
                  break;
               }
            } /* while(p)  ; DID YOU FIX SIZE FOR subn and the other nodes? */
         }  /* if (p) */
         if (subn)
         {
//            assert (! subn->parent);
            rootN = subn;
         }
      }  /* syntax block */

#if PROFILE_REUSE
      if (n_reuse_block <= treeHeight)
      {
         int old_size = n_reuse_block;
         do {
            n_reuse_block <<= 1;   // double in size
         } while (n_reuse_block <= treeHeight);
         c_reuse_block = (int*)realloc(c_reuse_block, n_reuse_block*sizeof(int));
         for (int ij=old_size ; ij<n_reuse_block ; ++ij)
            c_reuse_block[ij] = 0;
      }
      c_reuse_block[treeHeight] += 1;
#endif

      // in the end, place node (n) as the last node and update the size 
      // information on the path to the root
      lastN->right = n;
      n->parent = lastN;
      n->size = 1;
      n->left = n->right = NULL;
      register node_type* iter = lastN;
      register int lengthPath = 0;
      do 
      {
         iter->size += 1;
         iter = iter->parent;
         ++ lengthPath;
      } while( iter != NULL);
      lastN = n;

#if PROFILE_REUSE
      if (n_new_block <= lengthPath)
      {
         int old_size = n_new_block;
         do {
            n_new_block <<= 1;   // double in size
         } while (n_new_block <= lengthPath);
         c_new_block = (int*)realloc(c_new_block, n_new_block*sizeof(int));
         for (int ij=old_size ; ij<n_new_block ; ++ij)
            c_new_block[ij] = 0;
      }
      c_new_block[lengthPath] += 1;
#endif

      // in case I access a lot of old blocks, and then I do not reuse them,
      // I will end up with a long list as I append them to the end of the
      // tree. I should apply the 'splay' function every once in a while.
      // I will use the same algorithm as in cold_miss, as we have the same
      // preconditions and that algorithm is targeted to this particular
      // situation.
      if (lengthPath > max_path_length)  
      // splay from the new node only if we have at
      // least max_path_length nodes on the path to the root
      {
         // splay starting from n (lastN)
         // I already know a few things. lastN must be on the right side of
         // its parent, and its parent must be on the right side of its 
         // grandparent (if it has one), and we know it does because the
         // lengthPath is at least 10
         // I also know the lastN has no children because it is a new node
         register node_type* p = n->parent;
         register node_type* subn = 0; // the left child of n is initially nil
         register node_type* g = p->parent;

         // compute first rotation without any conditionals because we know
         // all the relevant info
         p->right = 0;
         g->right = p->left;
         if (p->left)
            p->left->parent = g;
         p->left = g;
         p->size = g->size - 1;
         g->size -= 2;
         subn = p;    // not NULL for the remaining rotations
         // n->left = subn;  I will set it only at the end
         p = g->parent;
         g->parent = subn;

         while(p)
         {
            p->right = subn;
            subn->parent = p;
            g = p->parent;  // the grandparent node
            if (g!=NULL)
            {
               g->right = p->left;
               if (p->left)
                  p->left->parent = g;
               p->left = g;
               p->size = g->size - 1;
               g->size -= (subn->size + 2);
               subn = p;
               p = g->parent;
               g->parent = subn;
               
            } else  // g is NULL, p is the root node
            {
               p->size -= 1;
               subn = p;
               p = p->parent;   // should be NULL
            }
         } /* while(p)  ; DID YOU FIX SIZE FOR subn and the other nodes? */
         // at this point subn holds the old root with size updated for
         // all nodes underneath; I just have to make subn as the left
         // child of lastN
         subn->parent = lastN;
         lastN->left = subn;
         lastN->parent = 0;
         lastN->size = subn->size + 1;
         rootN = lastN;
      }
      
      return (distance);
   }

#if 1
   // We do not keep values in the tree, so it does not quite make sense to 
   // print it; We can try though for debugging if the program does not work
   void printTree(FILE* fd)
   {
      typedef std::deque<node_type*>  NodeList;
      NodeList nl;
      node_type* spec = splay_allocator->new_elem();
      node_type* rootN = NULL;
      if (lastN)
      {
         rootN = lastN;
         while(rootN->parent)
            rootN = rootN->parent;
      }
      if (rootN)
         nl.push_back(rootN);
      nl.push_back(spec);
      node_type* y;
      fprintf(fd, "******************\nAt this point the tree has %d nodes:\n", tree_size);
      while(!nl.empty())
      {
         node_type* n = nl.front();
         nl.pop_front();
         if (n!=spec)
         {
            unsigned int val1, val2;
            if ((y = n->left) )
            {
               val1 = y->size;
               nl.push_back(y);
            }
            else
               val1 = 0; //NODE_NOT_FOUND;
            if ((y = n->right) )
            {
               val2 = y->size;
               nl.push_back(y);
            }
            else
               val2 = 0; //NODE_NOT_FOUND;

            fprintf(fd, "{%u <%u, %u>}   ", n->size, 
                val1, val2);
         }
         else
         {
            fprintf(fd, "\n\n");
            if (!nl.empty())
               nl.push_back(spec);
         }
      
      }
      splay_allocator->delete_elem(spec);
   }
#endif
   
private:
   alloc_type* splay_allocator;
   unsigned int tree_size;
   node_type *lastN, *rootN;
};

}  /* namespace MIAMIU */

#endif
