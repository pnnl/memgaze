/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: routine.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data bookkeeping and analysis at the routine level for a data reuse 
 * analysis tool. Extends the PrivateRoutine implementation with 
 * functionality specific to the data reuse tool.
 */

#ifndef MIAMI_MRDTOOL_ROUTINE_H
#define MIAMI_MRDTOOL_ROUTINE_H

#include "CFG.h"
#include "private_routine.h"
#include "load_module.h"

namespace MIAMI
{

class Routine : public PrivateRoutine
{

public:
   Routine(LoadModule *_img, addrtype _start, usize_t _size, 
           const std::string& _name, addrtype _offset);
   virtual ~Routine();

   void DetermineInstrumentationPoints();
   
   inline LoadModule* InLoadModule() const
   {
      return (static_cast<LoadModule*>(img));
   }
   
private:
};

};

#endif
