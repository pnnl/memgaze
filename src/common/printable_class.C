/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: printable_class.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Simple interface for printing objects.
 */

#include "printable_class.h"

std::ostream& operator<< (std::ostream& os, const PrintableClass &pc)
{
   pc.PrintObject (os);
   return (os);
}
