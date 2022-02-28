/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: charless.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Sorting less (<) operator for strings.
 */

#ifndef _CHAR_LESS_H
#define _CHAR_LESS_H

#include <string.h>
#include <map>
#include <set>

namespace MIAMI
{

class CharLess
{
public:
   bool operator() (const char* str1, const char* str2) const
   {
      return (strcmp(str1, str2) < 0);
   }
};

typedef std::set <const char*, CharLess> CharPSet;

}

#endif
