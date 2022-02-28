/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: FindCliques.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines a C++ interface to the Ram Samudrala implementation of the 
 * algorithm for finding cliques in a graph.
 */

#ifndef _FIND_CLIQUES_H
#define _FIND_CLIQUES_H

#include "Clique.h"

namespace MIAMI
{

class FindCliques
{
public:
   FindCliques (int _num_nodes, char *_connected);
   ~FindCliques ();
   
   int compute_cliques (CliqueList &cliques);

private:
   int extend(int old[], int ne, int ce, CliqueList &cliques);

   int num_nodes;
   char *connected;
   unsigned int *compsub;
   int nel;
};

}  /* namespace MIAMI */

#endif
