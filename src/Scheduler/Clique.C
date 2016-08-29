/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: Clique.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements a data structure to hold cliques of nodes.
 */

#include "Clique.h"
#include <string.h>

using namespace MIAMI;

Clique::Clique (int _num_nodes, unsigned int *_vertices)
{
   num_nodes = _num_nodes;
   vertices = new unsigned int [num_nodes];
   memcpy (vertices, _vertices, num_nodes*sizeof(unsigned int));
}

Clique::~Clique ()
{
   delete[] vertices;
}

Clique::Clique (const Clique &cq)
{
   num_nodes = cq.num_nodes;
   vertices = new unsigned int [num_nodes];
   memcpy (vertices, cq.vertices, num_nodes*sizeof(unsigned int));
}

Clique& 
Clique::operator= (const Clique &cq)
{
   num_nodes = cq.num_nodes;
   vertices = new unsigned int [num_nodes];
   memcpy (vertices, cq.vertices, num_nodes*sizeof(unsigned int));
   return (*this);
}

