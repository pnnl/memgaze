/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: use_mpi_rank.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Adds generic PIN tool support for obtaining the MPI rank
 * of an application process.
 */

#include <stdio.h>
#include <fcntl.h>
#include "use_mpi_rank.h"

KNOB<BOOL> KnobNoMpiRank (KNOB_MODE_WRITEONCE,    "pintool",
    "no_mpi_rank", "0", "do not add the mpi rank to the output file name");

int mpi_debug_level = 2;
#define DEBUG_MPI_RANK  1
#define LOG_MPI(x, y)   if ((x)<=mpi_debug_level) {y}

namespace MIAMI
{
   int miami_process_rank = 0;
   bool first_rank_call = true;

   typedef int (*mpi_rank_funptr_t)(void *comm, int *rank); 

   mpi_rank_funptr_t real_mpi_rank; 

   static int MIAMI_comm_rank(void *comm, int *rank)
   {
      int res = real_mpi_rank(comm, rank); 
   
#if DEBUG_MPI_RANK
      LOG_MPI(3,
         fprintf(stderr, "Real MPI_Comm_rank returned %d, rank=%d\n", res, *rank);
      )
#endif
      if (first_rank_call) {
         first_rank_call = false;
         miami_process_rank = *rank;
      }
      
      return res; 
   }

   // Function called from the ImageLoad callback to setup the mpi rank hook
   void MIAMI_MpiRankImgLoad(IMG img)
   {
      if (! KnobNoMpiRank.Value()) { 
         // search for the start and stop calipers
         RTN rtn = RTN_FindByName(img, "MPI_Comm_rank");
         if (RTN_Valid(rtn))
         {
#if DEBUG_MPI_RANK
            LOG_MPI(1,
               fprintf(stderr, "Found MPI_Comm_rank in %s\n",
                   IMG_Name(img).c_str());
            )
#endif
            real_mpi_rank = (mpi_rank_funptr_t)RTN_Replace(rtn, AFUNPTR(MIAMI_comm_rank));
         }
      }
   }

}
