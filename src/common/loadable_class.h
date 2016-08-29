/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: loadable_class.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Simple interface for serializing objects.
 */

#ifndef _LOADABLE_CLASS_H
#define _LOADABLE_CLASS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

class LoadableClass
{
public:
   LoadableClass() {}
   virtual int SaveToFile(FILE* fd) const = 0;
   virtual int LoadFromFile(FILE* fd) = 0;
protected:
   virtual ~LoadableClass() {}
};

extern FILE* operator<< (FILE* fd, LoadableClass &gf);
extern FILE* operator>> (FILE* fd, LoadableClass &gf);

#endif
