/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: mrd_file_info.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Store basic information about a mrd file received as input.
 */

#ifndef _MRD_FILE_INFO_H
#define _MRD_FILE_INFO_H

#include "miami_types.h"
#include <string>

namespace MIAMI
{
   using std::string;
   
   #define TEXT_MRD_FILE   1
   
   // memory reuse distance (MRD) files are associated with a specific 
   // memory block size. When computing reuse analysis for a memory level 
   // we need to use MRD data measured with a block size equal to the
   // cache line size.
   // The block size value is the first field in a MRD file. I will read it
   // and store MRD files in a map with the block size as key
   struct MrdInfo
   {
      MrdInfo(const string& _name, addrtype _mask, addrtype _val, uint32_t _bsize, bool isText = true) :
          name(_name), bmask(_mask), bvalue(_val), bsize(_bsize)
      { 
         if (isText)
            flags |= TEXT_MRD_FILE;
      }
      
      bool IsTextFile() const  { return (flags & TEXT_MRD_FILE); }
      
      string name;    // file name
      addrtype bmask; // block mask used during data collection
      addrtype bvalue;// block value used in conjunction with the mask
      uint32_t bsize; // block size
      int32_t  flags; // set various flags: is text file, etc.
   };
 
}  /* namespace MIAMI */

#endif
