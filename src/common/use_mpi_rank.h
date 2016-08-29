/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: use_mpi_rank.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Adds generic PIN tool support for obtaining the MPI rank
 * of an application process.
 */

#include <pin.H>

extern KNOB<BOOL> KnobNoMpiRank;

namespace MIAMI
{
   extern int miami_process_rank;

   extern void MIAMI_MpiRankImgLoad(IMG img);
}
