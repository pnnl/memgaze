/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: miami_data_sections.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Data section types for the MIAMI profile files.
 */

#ifndef _MIAMI_DATA_SECTIONS_H
#define _MIAMI_DATA_SECTIONS_H

namespace MIAMI
{

typedef enum {
   MIAMI_DATA_SECTION_INVALID = 0,
   MIAMI_DATA_SECTION_SCOPE_FOOTPRINT,
   MIAMI_DATA_SECTION_MEM_REUSE,
   MIAMI_DATA_SECTION_STREAM_INFO,
   MIAMI_DATA_SECTION_CFG_PROFILE,
   MIAMI_DATA_SECTION_LAST
} MiamiDataSectionType;

}  /* namespace MIAMI */

#endif
