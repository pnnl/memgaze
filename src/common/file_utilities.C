/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: file_utilities.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A few useful functions for working with files.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include "file_utilities.h"

// Function: fileExists
/**
    Check if a file exists
    @param[in] filename - the name of the file to check
    @return    true if the file exists, else false
*/
namespace MIAMI 
{

bool 
fileExists(const std::string& filename)
{
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1)
    {
        return true;
    }
    return false;
}

bool 
fileExists(const char* filename)
{
    struct stat buf;
    if (stat(filename, &buf) != -1)
    {
        return true;
    }
    return false;
}

/* There is no Unix API to get the name of the temporary directory.
 * POSIX recommends to get the name from environment variables.
 * Boost returns the path supplied by the first environment variable found 
 * in the list TMPDIR, TMP, TEMP, TEMPDIR. If none of these are found, "/tmp".
 */
const char* getTempDirName()
{
   const char* tdir = getenv("TMPDIR");
   if (tdir) return (tdir);
   
   tdir = getenv("TMP");
   if (tdir) return (tdir);

   tdir = getenv("TEMP");
   if (tdir) return (tdir);

   tdir = getenv("TEMPDIR");
   if (tdir) return (tdir);
   
   return ("/tmp");
}

}
