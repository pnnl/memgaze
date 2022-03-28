/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: source_file_mapping_binutils.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Maps PC address to source code line numbers using binutils.
 */

/* Contains binutils specific code for parsing an image file
   to read the source file mapping for given object addresses.
*/

#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include "source_file_mapping.h"
#include "config.h"

/* binutils specific header files */
#include "bfd.h"
#define HAVE_DECL_BASENAME 1
#include "libiberty.h"

namespace MIAMIP
{
#define DEBUG_FIND_NEAREST_LINE 0

/* Define a map from rapid searching of sections based on a pc address.
 * Map a section to its end address and use upper_bound to find the 
 * closest section. We only need to check that the start address of the 
 * returned section is lower or equal than the pc.
 */
typedef std::map<addrtype, asection*> AddrSecMap;

struct bfd_info
{
   bfd *abfd;
   asymbol **syms;
   AddrSecMap secs;
};


/**
  The function returns the size of the named file.  If the file does not
  exist, or if it is not a real file, then a suitable error message is printed 
  and zero is returned.
  
  @param file_name - string with file name

  @retval 0 - in case of failure
  @retval file size in bytes in case of success 
*/
static off_t 
get_file_size(const char * file_name)
{
  struct stat statbuf;
    
  if (stat(file_name, &statbuf) < 0) 
  {
    if (errno == ENOENT)
      fprintf (stderr, "Error: no such file: %s\n", file_name);
    else
      fprintf(stderr, "Warning: could not locate '%s'.  reason: %s",
                    file_name, strerror (errno));
  } 
  else 
  {
    if (! S_ISREG (statbuf.st_mode))
      fprintf(stderr, "Warning: '%s' is not an ordinary file", file_name);
    else
      return statbuf.st_size;
  }
  
  return 0;
}

/**
  The function prints the possible matching targets after a FALSE return 
  from bfd_check_format_matches with bfd_get_error () == bfd_error_file_ambiguously_recognized.

  @param p - pointer to array of strings with target names terminated by NULL pointer

  @retval The function does not return anything.
*/
static void 
list_matching_formats(char **p)
{
  if (!p || !*p)
    return;

  char **x = p;
  char *buf;
  char *aux;
  long l = 0;

  while (*p)
  {
    l += strlen(*p)+1;
    p++;
  }
  p = x;
  buf = new char[l];
  aux = buf;
  while (*p)
  {
    sprintf(aux, " %s", *p);
    aux += strlen(*p)+1;
    p++;
  }
  fprintf(stderr, "matching formats:%s\n", buf);
  delete [] buf;
}


/* Read in the symbol table.  */
static int
parse_symbols (bfd_info *info)
{
  long storage;
  long symcount;
  bfd_boolean dynamic = FALSE;

  if ((bfd_get_file_flags (info->abfd) & HAS_SYMS) == 0)
    return (0);

  storage = bfd_get_symtab_upper_bound (info->abfd);
  if (storage == 0)
  {
     storage = bfd_get_dynamic_symtab_upper_bound (info->abfd);
     dynamic = TRUE;
  }
  if (storage < 0)
  {
     fprintf (stderr, "Error: Cannot read number of symbols for image file %s\n",
          bfd_get_filename (info->abfd));
     return (-1);
  }

  info->syms = (asymbol **) xmalloc (storage);
  if (dynamic)
    symcount = bfd_canonicalize_dynamic_symtab (info->abfd, info->syms);
  else
    symcount = bfd_canonicalize_symtab (info->abfd, info->syms);
  if (symcount < 0)
  {
     fprintf (stderr, "Error: Cannot canonicalize symbols for image file %s\n",
          bfd_get_filename (info->abfd));
     return (-2);
  }
  return (0);
}


/* Add a section to the map.  This is called via
   bfd_map_over_sections.  */
static void
add_section_to_map (bfd *abfd, asection *section, void *data)
{
   bfd_vma vma;
   bfd_size_type size;

   if ((bfd_section_flags (section) & SEC_ALLOC) == 0)
      return;

   bfd_info *info = static_cast<bfd_info*>(data);
   if (!info)
      return;

   vma = bfd_section_vma (section);
   size = bfd_section_size (section);
   if (size>0)
   {
      info->secs.insert(AddrSecMap::value_type(vma+size, section));
   }
}

/* find the source location for the provided pc address
 */
int
get_source_file_location (void *data, addrtype pc, int32_t* column, int32_t* line, std::string* file,
           std::string* func)
{
//   const struct elf_backend_data * bed;
   const char *filename;
   const char *functionname;
   unsigned int lineval;
   
   *column = 0;
   *line = 0;
   
   bfd_info *info = static_cast<bfd_info*>(data);
   if (!info)
      return (-13);

#if 0    
   if (bfd_get_flavour (info->abfd) == bfd_target_elf_flavour
	  && (bed = get_elf_backend_data (info->abfd)) != NULL
	  && bed->sign_extend_vma
	  && (pc & (bfd_vma) 1 << (bed->s->arch_size - 1)))
      pc |= ((bfd_vma) -1) << bed->s->arch_size;
#endif

   // First, find the section that includes this pc
   AddrSecMap::iterator sit = info->secs.upper_bound(pc);
   if (sit==info->secs.end())
      return (-1);
   asection *section = sit->second;
   
   // Found a section whose ending address is greater than my pc
   // Test that this section starts before the pc.
   addrtype vma = bfd_section_vma ( section);
   if (pc < vma)
      return (-2);
           
   // bfd_find_nearest_line returns the wrong path for certain files. Problem
   // was observed for a simple test program that invokes a PLASMA kernel.
   // addr2line from binutils uses a more involved algorithm and reports the
   // the correct file path in the observed case. On the other hand, 
   // objdump -l reports the same bad file paths. 
   // I should either adopt the addr2line algorithm (I copied the file in 
   // the MIAMI/src/common/ folder, or change MIAMI to use SymtabAPI, if
   // the later one handles debug information correctlty.
   // ***** FIXME:old *****
   bfd_boolean found = bfd_find_nearest_line (info->abfd, section, info->syms, pc - vma,
				 &filename, &functionname, &lineval);

   if (found)
   {
#if DEBUG_FIND_NEAREST_LINE
      fprintf(stderr, "0x%" PRIxaddr " (offset 0x%" PRIxaddr ") debug info line %d, file %p", 
               pc, pc-vma, lineval, filename);
      if (filename)
         fprintf (stderr, ": %s\n", filename);
      else
         fprintf (stderr, ": NULL\n");
#endif
      if (filename)
         *file = filename;
      if (functionname && func)
         *func = functionname;
      *line = lineval;
   }
   return (0);
}

/* Process an image file.  Returns 0 on success, <0 for erroer.  */
int
source_file_info_open_image (const char *file_name, void** data)
{
  bfd *abfd;
  char **matching;
  
  *data = 0;

  if (get_file_size (file_name) < 1)
     return (-1);

  abfd = bfd_openr (file_name, NULL);
  if (abfd == NULL)
     return (-2);
  
  /* Decompress sections.  */
  abfd->flags |= BFD_DECOMPRESS;

  if (bfd_check_format (abfd, bfd_archive))
  {
     fprintf (stderr, "Warning: %s: cannot get addresses from archive.\n", file_name);
  }

  if (! bfd_check_format_matches (abfd, bfd_object, &matching))
  {
     fprintf (stderr, "Warning: %s: does not match any of the target file formats\n", file_name);
     if (bfd_get_error () == bfd_error_file_ambiguously_recognized)
     {
        list_matching_formats (matching);
        free (matching);
     }
     return (-3);
  }

  bfd_info *info = new bfd_info();  // use new to initialize the map
  // (struct bfd_info*)malloc(sizeof(struct bfd_info));
  info->abfd = abfd;

  parse_symbols (info);
  
  bfd_map_over_sections (abfd, add_section_to_map, info);
  
  *data = info;

  return 0;
}

void
source_file_info_close_image (void *data)
{
   bfd_info *info = static_cast<bfd_info*>(data);
   if (info)
   {
      info->secs.clear();
   
      if (info->syms != NULL)
      {
         free (info->syms);
         info->syms = NULL;
      }

      if (info->abfd)
         bfd_close (info->abfd);
      
      delete (info);
//      free (info);
   }
}

/* Source file location initialization functions.
 * You must call this function once before calling any other method
 * in this file.
 * It performs the implementation dependent initialization.
 */
int
source_file_info_init()
{
   bfd_init ();
//   set_default_bfd_target ();
   
   return (0);
}


/* return a best effort demangling of the specified function name
 */
const char* 
get_best_function_name(const char* name, char* outbuf, int buflen)
{
   if (!name) { return NULL; }

   const char* demangledName = MIAMIP::get_demangled_func_name(name, outbuf, buflen);
   if (demangledName && (strlen(demangledName) > 0)) {
      return demangledName;  
   } else {
      return name;
   }
}

// Include GNU's demangle
#include "gnu_demangle.h" // GNU's demangle

// Include the system demangle
#if (__sgi && __unix)
# include <dem.h>      // demangle
  const int DEMANGLE_BUF_SZ = MAXDBUF;
#elif (__digital__ && __unix)
# include <../../usr/include/demangle.h> // demangle (don't confuse with GNU)
  const int DEMANGLE_BUF_SZ = 32768; // see MAXDBUF in SGI's dem.h
#elif (__sun && __unix)
# include </usr/include/demangle.h> // demangle (don't confuse with GNU)
  const int DEMANGLE_BUF_SZ = 32768; // see MAXDBUF in SGI's dem.h
#elif (__linux)
  // the system demangle is GNU's demangle
  const int DEMANGLE_BUF_SZ = 32768; // see MAXDBUF in SGI's dem.h
#else
# error "SourceFileLocation.C does not recognize your system"
#endif

int
get_max_demangled_name_size()
{
   return (DEMANGLE_BUF_SZ);
}

// Returns the demangled function name.  The return value is stored in
// 'outbuf' and will be clobbered the next time the function is called.
const char*
get_demangled_func_name(const char* inbuf, char* outbuf, int buflen)
{
/*
  static char inbuf[1024];
  static char outbuf[DEMANGLE_BUF_SZ];
  strcpy(inbuf, name);
*/

  // -------------------------------------------------------
  // Try the system demangler first
  // -------------------------------------------------------
  strcpy(outbuf, ""); // clear outbuf
  bool demSuccess = false;
#if defined(__sgi)
  /* SGI implementation is the only one who does not take as a parameter
     the size of the output buffer, resulting in possible overflow.
     The SGI implementation, however, has an internal max demangled buffer 
     size. We can check if the provided buffer is large enough, and use a 
     temporary buffer if not. It is less efficient, but safer.
   */
  char* temp = outbuf;
  if (buflen < DEMANGLE_BUF_SZ)
     temp = (char*) malloc(DEMANGLE_BUF_SZ);
  if (demangle(inbuf, temp) == 0) 
  {
     demSuccess = true;
  } // else: demangling failed
  if (temp != outbuf)  // we are using temporary buffer, copy content
  {
     if (demSuccess)
        strncpy(outbuf, temp, buflen);
     if (buflen>0)
        outbuf[buflen-1] = '\0';
     free(temp);
  }
#elif defined(__digital__)
  if (MLD_demangle_string(inbuf, outbuf,
			  buflen, MLD_SHOW_DEMANGLED_NAME) == 1) {
    demSuccess = true; 
  } // else: error or useless
#elif defined(__sun)
  if (cplus_demangle(inbuf, outbuf, buflen) == 0) {
    demSuccess = true;
  } // else: invalid mangled name or output buffer too small
#elif defined(__linux)
  // System demangler is same as GNU's
#endif

  // Definitions of a 'valid encoded name' differ.  This causes a
  // problem if the system demangler deems a string to be encoded,
  // reports a successful demangle, but simply *copies* the string into
  // 'outbuf' (the 'identity demangle').  (Sun, in particular, has a
  // liberal interpretation of 'valid encoded name'.)  In such cases,
  // we want to give the GNU demangler a chance.
  if (strcmp(inbuf, outbuf) == 0) { demSuccess = false; }
  
  if (demSuccess) { return outbuf; }

#if 1  // I need to include binutils demangle.h and link with libiberty for this
       // test what we get from PIN first.
  // -------------------------------------------------------
  // Now try GNU's demangler
  // -------------------------------------------------------
  strcpy(outbuf, ""); // clear outbuf
  char* ret = GNU_CPLUS_DEMANGLE(inbuf, DMGL_PARAMS | DMGL_ANSI);
  if (!ret) {
    return NULL; // error!
  }
  strncpy(outbuf, ret, buflen); 
  if (buflen>0)
     outbuf[buflen-1] = '\0';
  free(ret); // gnu_cplus_demangle caller is responsible for memory cleanup
#endif   // demangle.h needed
  return outbuf; // success!
}

}  /* namespace MIAMIP */
