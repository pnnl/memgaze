/* Author: Ram Samudrala (me@ram.org)
 *         extend() was implemented from the Bron and Kerbosch
 *         algorithm (Communications of the ACM 16:575-577, 1973) by
 *         Jean-Francios Gibrat.
 * 
 * April 19, 1995.
 *
 * Converted to C++ and modified: Gabriel Marin (mgabi@rice.edu)
 * 03/02/2006
 */

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include "FindCliques.h"

using namespace MIAMI;

/******************************************************************/

FindCliques::FindCliques (int _num_nodes, char *_connected)
{
   num_nodes = _num_nodes;
   connected = _connected;
   compsub = NULL;
   nel = 0;
}
/******************************************************************/

FindCliques::~FindCliques ()
{
}
/******************************************************************/

int 
FindCliques::compute_cliques (CliqueList &cliques)
{
  int i;
  int *all = new int[num_nodes];

  for(i = 0; i < num_nodes; ++i)
    all[i] = i;

  compsub = new unsigned int[num_nodes];
  nel = -1;
  
  int result = extend(all, -1, num_nodes-1, cliques);
  delete [] all;
  delete [] compsub;
  return (result);
}
/******************************************************************/

int 
FindCliques::extend(int old[], int ne, int ce, CliqueList &cliques)
/*
 * Author: Jean-Francois Gibrat
 *
 * This subroutine finds the maximal complete subgraphs (cliques) of a
 * given undirected graph. The algorithm consists in a recursively defined
 * extension operator (extend) applied to the three following sets:
 * set compsub: set to be extended by a new point or shrunk by one point as
 *              the algorithm travels along a branch of the backtracking tree.
 * set cand:    set of all vertices connected to the current vertex in compsub
 *              i.e., vertices which will be used as an extension of the
 *              present configuration of compsub.
 * set not:     set of all points which have already served as an extension and
 *              are now excluded.
 *
 * The basic mechanism consists in the following steps:
 * Step 1:  selection of a candidate.
 * Step 2:  creating new sets cand and not from the old sets by removing all points
 *          not connected to the selected candidate, keeping the old sets intact.
 * Step 3:  adding the selected candidate to compsub.
 * Step 4:  calling the extension operator to operate on the sets just formed.
 * Step 5:  upon return the selected candidate is transfered from compsub to the 
 *          old set not.
 *
 * The set compsub is a stack external to the subroutine.
 * The sets not and cand are stored in the local array old,
 * not from positions 0 to ne and cand from positions ne+1 to ce.
 * nvert x nvert array [connected] indicates which vertices are connected to which
 * vertices. This array is external to the subroutine.
 * 
 * Last modification: 13/07/94
 *
 * Reference: Bron C. and Kerbosch J., Commun. A.C.M., vol. 16, pp 575-577, 1973.
 *
 * Modified by mgabi on 03/02/2006.
 * Returns the number of recursive calls to extend;
 */

{
   int *newV = new int[num_nodes];
   int knod, nod, fixp=0;
   int newne, newce, i, j, count, pos=0, p, s=0, sel, minnod;
   int result = 0;

   minnod = ce+1;
   nod = 0;

   /* 
    * Look for the vertex that is connected to the maximum number of vertices,
    * i.e., the minimum number of disconnections (Step 1)
    */
   int fixpIdx = 0;
   for (i = 0; (i <= ce) && (minnod != 0); ++i) 
   {
      p = old[i];
      int baseIdx = p * num_nodes;
      count = 0;
      /* Count disconnections */
      for (j = ne + 1; (j <= ce) && (count < minnod); ++j) 
      {
         if (connected [baseIdx + old[j]] == 0)
         {
            count++;
            pos = j;  /* save position of potential candidate */
         }
      }
      // Test whether vertex at position i in the stack has a minimum number 
      //   of diconnections 
      if (count < minnod) 
      {
         fixp = p;
         fixpIdx = baseIdx;
         minnod = count;
         if (i <= ne) 
            s = pos;
         else 
         {
            s = i;
            nod = 1;  /* Number of disconnections preincreased by one */
         }
      }
   }
  
   /* 
    * Backtrack Cycle: the loop is executed as many times as there are 
    * vertices not connected to the selected point.
    */
   fprintf (stderr, "minnod=%d, nod=%d, fixp=%d, s=%d, pos=%d, ne=%d, ce=%d\n",
       minnod, nod, fixp, s, pos, ne, ce);
   for (knod = minnod + nod; knod >= 1; --knod)
   {
      /* 
       * Interchange 
       */
      if (s != (ne+1))
      {
         p = old[s];
         old[s] = old[ne+1];
         old[ne+1] = p;
      }
      sel = old[ne+1];
      int selIdx = sel * num_nodes;
      /* 
       * Fill new set not (Step 2)
       */
      newne = -1;
      for (i = 0; i <= ne; ++i)
         if (connected[selIdx+old[i]] == 1) 
            newV[++newne] = old[i];    
      /* 
       * Fill new set cand (Step 2)
       */
      newce = newne;
      for (i = ne + 2; i <= ce; ++i)
         if (connected[selIdx+old[i]] == 1)
            newV[++newce] = old[i];

      /* 
       * The selected vertex is added to compsub (Step 3)
       */
      compsub[++nel] = sel; // index nel is external to the subroutine
      /* 
       * Set 'cand' and 'not' are empty, a new clique has been found:
       * the recursive process terminates when this condition is met.
       */

      if (newce == -1) 
      { 
         /* The new clique is stored in compsub. */
         // Create a Clique object and save it in an array
         Clique *cl = new Clique (nel+1, compsub);
         cliques.push_back (cl);
      }    
      /*
       * Otherwise calls the extension operator to operate on the sets just 
       * formed  (Step 4)
       */
      else 
      { 
         if (newne < newce)
            result += (extend(newV, newne, newce, cliques) + 1);
      }

      /* 
       * Upon return, remove the selected candidate from compsub and add it
       * to the old set not (Step 5)
       */
      /* Remove from compsub */
      -- nel;
      /* Add to not */
      ++ ne;

      /* 
       * Select a candidate disconnected to the fixed point (Step 1)
       */
      if (knod > 1)
      {  
         s = ne + 1;
         while (connected[fixpIdx+old[s]] == 1)
            s++;
         assert (s <= ce);
      }
   } /* End of the loop with index knod */
   delete [] newV;
   return (result);
}

/******************************************************************/
