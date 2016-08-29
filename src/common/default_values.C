/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: default_values.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A central place to store various global defaults.
 */

#include "miami_types.h"

namespace MIAMI
{

unsigned long long defaultZeroULL = 0;
int defaultZeroInt = 0;
char defaultNACharPValue[8] = {"N/A"};
uint32_t defaultUI = 0;
unsigned int* const defaultUIP = 0;
unsigned long long defULL = 0;
float defFP = -1;
const char* defaultCharP = 0;
const char* defaultVarName = "unknown";

}
