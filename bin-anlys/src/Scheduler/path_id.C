/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: path_id.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements a path ID class.
 */

#include "path_id.h"

namespace MIAMI
{

std::ostream& operator<< (std::ostream& os, PathID &pId)
{
   os << pId.Name();
   return (os);
}

}  /* namespace MIAMI */
