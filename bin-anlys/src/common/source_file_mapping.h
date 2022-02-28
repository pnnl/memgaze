/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: source_file_mapping.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Generic API for mapping PC addresses to source code 
 * line numbers and for function name demangling.
 */

#ifndef MIAMI_SOURCE_FILE_MAPPING_H
#define MIAMI_SOURCE_FILE_MAPPING_H

#include <string>
#include "miami_types.h"

using namespace MIAMI;

namespace MIAMIP
{

/* find the source location for the provided pc address
 */
int
get_source_file_location (void *data, addrtype pc, int32_t* column, int32_t* line, std::string* file,
          std::string* func);

/* Process an image file.  Returns 0 on success, <0 for erroer.  */
int
source_file_info_open_image (const char *file_name, void** data);

void
source_file_info_close_image (void *data);

/* Source file location initialization functions.
 * You must call this function once before calling any other method
 * in this file.
 * It performs the implementation dependent initialization.
 */
int
source_file_info_init();

/* return a best effort demangling of the specified function name
 */
const char* 
get_best_function_name(const char* name, char* outbuf, int buflen);

/* demangle function name specified in inbuf. Output is stored in
   outbuf, up to buflen bytes.
   Use function get_max_demangled_name_size to obtain the maximum size 
   of a demangled name.
 */
const char*
get_demangled_func_name(const char* inbuf, char* outbuf, int buflen);

/* obtain the maximum size of a demangled function name.
 */
int
get_max_demangled_name_size();

}  /* namespace MIAMIP */

#endif
