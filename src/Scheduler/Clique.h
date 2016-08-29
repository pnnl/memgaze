/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: Clique.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines a data structure to hold cliques of nodes.
 */

#ifndef _CLIQUE_H
#define _CLIQUE_H

#include <stdlib.h>
#include <list>

namespace MIAMI
{

class Clique
{
public:
  Clique () { num_nodes = 0; vertices = NULL; }
  Clique (int _num_nodes, unsigned int *_vertices);
  Clique (const Clique &cq);
  ~Clique ();

  Clique& operator= (const Clique &cq);

  int num_nodes;
  unsigned int *vertices;
};

typedef std::list <Clique*> CliqueList;

} /* namespace MIAMI */

#endif
