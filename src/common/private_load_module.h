/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: private_load_module.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Base class for storing image specific data.
 */

#ifndef MIAMI_PRIVATE_LOAD_MODULE_H
#define MIAMI_PRIVATE_LOAD_MODULE_H

#include "miami_types.h"

// standard header
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string>
#include <map>

namespace MIAMI
{

class PrivateLoadModule
{
public:
   PrivateLoadModule (uint32_t _id, addrtype _load_offset, addrtype _low_addr_offset, std::string _name)
   {
      img_id = _id; 
      base_addr = _load_offset;
      low_addr_offset = _low_addr_offset;
      img_name = _name;
      file_offset = 0;
      source_data = 0;
//      fprintf(stderr, "Loading Image %d with path %s, base_addr=0x%" PRIxaddr ", low_addr_offset=0x%" PRIxaddr "\n",
//            img_id, img_name.c_str(), base_addr, low_addr_offset);
   }
   virtual ~PrivateLoadModule() { }
   virtual void saveToFile(FILE *fd) const  // save img info in binary format
   {
      fwrite(&img_id, 4, 1, fd);
      fwrite(&base_addr, sizeof(base_addr), 1, fd);
      fwrite(&low_addr_offset, sizeof(low_addr_offset), 1, fd);
      // now save the file name
      uint32_t len = img_name.length();
      fwrite(&len, 4, 1, fd);
      fwrite(img_name.c_str(), 1, len, fd);
   }

   virtual void printToFile(FILE *fd) const  // save img info in text format
   {
      fprintf(fd, "%" PRIu32 " 0x%" PRIxaddr " 0x%" PRIxaddr " %s\n",
          img_id, base_addr, low_addr_offset, img_name.c_str());
   }
   
   addrtype BaseAddr() const  { return (base_addr); }
   addrtype LowOffsetAddr() const  { return (low_addr_offset); }
   const std::string& Name() const  { return (img_name); }
   uint64_t FileOffset() const  { return (file_offset); }
   void setFileOffset(uint64_t _fof)  { file_offset = _fof; }
   uint32_t ImageID() const  { return (img_id); }
   
   virtual addrtype RelocationOffset() const    { return (0); }
   
   void InitializeSourceFileInfo();
   void FinalizeSourceFileInfo();
   void GetSourceFileInfo(addrtype startAddr, addrtype endAddr,
           std::string &file, std::string &func, int32_t &startLine, int32_t& endLine);
   
protected:
   addrtype base_addr;
   addrtype low_addr_offset;
   std::string img_name;
   uint32_t img_id;
   uint64_t file_offset;   // use this field to store the offset in the cfg file 
                           // where this image starts. Used while loading the CFGs.

   // Next fields is used to store implementation dependent persistent info (if needed)
   // It is passed by the SourceFileInfo family of methods to the system specific functions
   void *source_data;
};

}

#endif
