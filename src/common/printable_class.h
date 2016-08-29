/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: printable_class.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Simple interface for printing objects.
 */

#ifndef _PRINTABLE_CLASS_H
#define _PRINTABLE_CLASS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

class PrintableClass
{
public:
   PrintableClass() {}
   virtual void PrintObject (std::ostream& os) const = 0;
protected:
   virtual ~PrintableClass() {}
};

extern std::ostream& operator<< (std::ostream& os, const PrintableClass &gf);

#endif
