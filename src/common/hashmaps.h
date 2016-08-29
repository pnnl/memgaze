/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: hashmaps.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Common HashMap definitions.
 */

#ifndef _MIAMI_HASHMAPS_H
#define _MIAMI_HASHMAPS_H

#include "miami_types.h"
#include "fast_hashmap.h"

namespace MIAMI
{
extern uint32_t defaultUI;
typedef MIAMIU::HashMap<uint32_t, uint32_t, &defaultUI> HashMapUI;

/* Hashmap for mapping memory instructions into logical sets.
 */
//typedef MIAMIU::HashMap<addrtype, uint32_t, &defaultUI> AddrUIMap;

/* hashmap for mapping variable names to memory instruction indecies.
 */
extern const char* defaultCharP;
typedef MIAMIU::HashMap<int32_t, const char*, &defaultCharP> RefNameMap;

} /* namespace MIAMI */

#endif
