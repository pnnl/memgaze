/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: miami_globals.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A few MIAMI globals dealing with static analysis dbs.
 */

#include "miami_types.h"

namespace MIAMI {
   /* magic word is saved at the beginning of each db file; must be 8 characters */
   const char* persistent_db_magic_word = "MIAMI-DB";
   
   /* This is the current format version of the db file; used while saving the file,
    * but it acts also as an upper bound on the expected file version. 
    */
   int32_t     persistent_db_version = 0x00000001;
   
   /* the minimum required version for the current analysis; used at load time */
   int32_t     persistent_db_min_required_version = 0x00000001;

}

