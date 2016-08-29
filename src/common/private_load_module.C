/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: private_load_module.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Base class for storing image specific data.
 */


#include "private_load_module.h"
#include "source_file_mapping.h"

using namespace MIAMI;

#undef MIN
#undef MAX
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define MAX(A,B) ((A) > (B) ? (A) : (B))

static bool 
IsValidLine(int32_t startLine, int32_t endLine)
{
   return ((startLine > 0) && (endLine > 0)); 
}

static bool 
IsValidLine(int32_t line)
{
   return (line > 0); 
}

void 
PrivateLoadModule::InitializeSourceFileInfo()
{
   MIAMIP::source_file_info_open_image(img_name.c_str(), &source_data);
}

void 
PrivateLoadModule::FinalizeSourceFileInfo()
{
   MIAMIP::source_file_info_close_image(source_data);
   source_data = NULL;
}

void 
PrivateLoadModule::GetSourceFileInfo(addrtype startAddr, addrtype endAddr,
       std::string &file, std::string &func, int32_t &startLine, int32_t& endLine)
{
  // Follows the following convention: The starting address of an
  // item in memory is the address of its first byte. The ending
  // address of an item is the address of the first byte beyond the
  // object, so an item's size is its ending address minus its
  // starting address.

  // If file is NULL, both startLine
  // and endLine will be 0; if file is not NULL, the line numbers may
  // or may not be non-0.  It is possible for startLine to be valid
  // but for endLine to be 0.

  file = "";
  startLine = endLine = 0;

  // To find ending line number, decrement by one instruction so we
  // aren't looking at beginning of next object.
  // We should never have an object of length 0, but we want to give
  // the user the option of specifying the start address for endAddr.

  // assume variable width instructions !!
  // this is not helpful: mach_inst_size(exe->get_mach_inst(endAddr))
  std::string file1, file2;
  std::string func1, func2;
  int32_t column = 0;
  MIAMIP::get_source_file_location (source_data, startAddr, &column, &startLine, &file1, &func1);
  if (endAddr>0)
  {
    endAddr = MAX(startAddr, endAddr - 1);
    MIAMIP::get_source_file_location (source_data, endAddr, &column, &endLine, &file2, &func2);
  }
  
  // How should we deal with strange cases?
  // Always provide the file from the start address
  if (file2.length()>0 && file1.length()==0)
     file = file2;
  else
     file = file1;

  if (func2.length()>0 && func1.length()==0)
     func = func2;
  else
     func = func1;

  if (IsValidLine(startLine) && !IsValidLine(endLine)) {   
    endLine = startLine; //???
  }
  if (startLine > endLine) { //???
    int32_t tmp = startLine;
    startLine = endLine;
    endLine = tmp;
  }  
}

