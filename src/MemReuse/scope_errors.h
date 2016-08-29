/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: scope_errors.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines stack states that can be encountered during data reuse analysis.
 */

#ifndef _SCOPE_ERRORS_H
#define _SCOPE_ERRORS_H

#include "miami_types.h"
#include "bucket_hashmap.h"

using MIAMI::addrtype;

namespace MIAMI_MEM_REUSE
{
   typedef enum {
       STACK_ERROR_NO_ERROR = 0, 
       STACK_ERROR_SCOPE_NOT_IN_STACK,
       STACK_ERROR_SCOPE_NOT_FOUND,
       STACK_ERROR_SCOPE_NOT_ON_TOP
   } StackErrorType;
   
   typedef MIAMIU::BucketHashMap<addrtype, 

}  /* namespace MIAMI_MEM_REUSE */

#endif
