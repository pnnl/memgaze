/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: loadable_class.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Simple interface for serializing objects.
 */

#include "loadable_class.h"

FILE* operator<< (FILE* fd, LoadableClass &lc)
{
   lc.SaveToFile(fd);
   return (fd);
}

FILE* operator>> (FILE* fd, LoadableClass &lc)
{
   lc.LoadFromFile(fd);
   return (fd);
}
