/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: load_module.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data bookkeeping at the load module level for a stream simulation tool.
 */

#ifndef MIAMI_SRDTOOL_LOAD_MODULE_H
#define MIAMI_SRDTOOL_LOAD_MODULE_H

// standard header
#include "private_load_module.h"
#include "bucket_hashmap.h"

#include <list>
#include <vector>
#include <map>

namespace MIAMI
{

extern int32_t defInt32;
typedef MIAMIU::BucketHashMap <uint64_t, int32_t, &defInt32, 128> BucketMapInt;

class LoadModule : public PrivateLoadModule
{
public:
   LoadModule (uint32_t _id, addrtype _load_offset, addrtype _low_addr , std::string _name)
       : PrivateLoadModule(_id, _load_offset, _low_addr, _name)
   {
      nextInstIndex = 1;
      reverseInstIndex = 0;
      savedInstIndex = 0;
   }
   virtual ~LoadModule();
   
   void saveToFile(FILE *fd) const;

   // The next four methods use and modify shared data. 
   // However, I invoke them only from PIN instrumentation callbacks
   // that are serialized by PIN, even when the application is multi-
   // threaded. Thus, they are safe within this use model, but be aware
   // of potential data races if they are invoked from an analysis step.
   int32_t getInstIndex(addrtype iaddr, int memop);
   addrtype getAddressForInstIndex(int32_t iindex, int32_t& memop);
   
   BucketMapInt instMapper;
   
private:
   int32_t nextInstIndex;
   
   uint64_t *reverseInstIndex;
   int32_t savedInstIndex;
};

}

#endif
