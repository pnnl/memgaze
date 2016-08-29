/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: file_utilities.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A few useful functions for working with files.
 */

#include <string>

namespace MIAMI
{

bool fileExists(const std::string& filename);
bool fileExists(const char* filename);

const char* getTempDirName();

}
