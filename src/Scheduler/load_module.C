/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: load_module.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements data bookkeeping and analysis associated with an individual 
 * load module.
 */

#include <stdio.h>
#include <stdlib.h>
#include "load_module.h"
#include "routine.h"
#include "miami_utils.h"
#include "file_utilities.h"
#include "tarjans/MiamiRIFG.h"
#include "tarjans/TarjanIntervals.h"
#include "debug_scheduler.h"
#include "dyninst-insn-xlate.hpp"
#include "dyninst-cfg-xlate.hpp"

using namespace MIAMI;
using namespace std;

namespace MIAMI
{
   int32_t defInt32 = -1;
   extern const char* defaultVarName;
}

LoadModule::~LoadModule()
{
   RoutineList::iterator rit = funcList.begin();
   for ( ; rit!=funcList.end() ; ++rit)
   {
      delete (*rit);
   }
   funcList.clear();
   
   // Clear the names from nameStorage
   CharPSet::iterator nit = nameStorage.begin();
   for ( ; nit!=nameStorage.end() ; ++nit)
      free (const_cast<char*>(*nit));
   nameStorage.clear();
   
#if 0
   // The udnefScope is linked under the img_scope.
   // It will be deleted when I delete img_scope. 
   // I need to unlink it first if I want to delete explicitly before.
   if (undefScope)
      delete (undefScope);
#endif
   if (img_scope)
      delete (img_scope);
}

int 
LoadModule::loadFromFile(FILE *fd, bool parse_routines)
{
#define CHECK_COND(cond, err, ...) if (cond) \
   {fprintf(stderr, "ERROR: "err"\n", __VA_ARGS__); goto load_error; }
#define CHECK_COND0(cond, err) if (cond) \
   {fprintf(stderr, "ERROR: "err"\n"); goto load_error; }

   size_t res;
   int ires;
   uint32_t id;
   uint32_t len;
   char *name = 0;
      
   res = fread(&id, 4, 1, fd);
   CHECK_COND0(res!=1, "reading image ID")
   
   res = fread(&cfg_base_addr, sizeof(cfg_base_addr), 1, fd);
   CHECK_COND(res!=1, "reading base address for image ID %u", id)
   res = fread(&old_low_offset, sizeof(old_low_offset), 1, fd);
   CHECK_COND(res!=1, "reading low address offset for image ID %u", id)
   // now get the file name length
   res = fread(&len, 4, 1, fd);
   CHECK_COND(res!=1 || len<1 || len>1024, "reading name length for image ID %u, res=%ld, len=%u", 
               id, res, len)
   name = new char[len+1];
   res = fread(name, 1, len, fd);
   CHECK_COND(res!=len, "reading name for image ID %u", id)
   name[len] = 0;
#if VERBOSE_DEBUG_LOAD_MODULE
   DEBUG_LOAD_MODULE(3,
      fprintf (stderr, "Reading data for image %s at base address %p, low offset %p, prev base address %p, prev low offset %p\n",
          name, (void*)base_addr, (void*)low_addr_offset, (void*)cfg_base_addr, (void*)old_low_offset);
   )
#endif
   // test if the image was loaded at a different address. Compute a value that 
   // should act as a relocation offset for all routines
   reloc_offset = base_addr;
   // check that the offset of the low address is the same
   // print a WARNING and use the old offset if they are different
   if (low_addr_offset != old_low_offset)
   {
#if VERBOSE_DEBUG_LOAD_MODULE
      DEBUG_LOAD_MODULE(2,
         fprintf(stderr, "WARNING: Image %s low address is at a different offset 0x%" PRIxaddr 
             " from the base address than the offset 0x%" PRIxaddr 
             " recorded during the CFG construction. "
             "Use the previously recorded offset to work around a bug in PIN when using static tools.\n",
                  name, low_addr_offset, old_low_offset);
      )
#endif
      reloc_offset += low_addr_offset - old_low_offset;
   }
   
#if VERBOSE_DEBUG_LOAD_MODULE
   DEBUG_LOAD_MODULE(3,
      cerr << "Image " << name << ", base_addr=0x" << hex << base_addr
           << ", CFG base_addr=0x" << cfg_base_addr
           << ", reloc offset=0x" << reloc_offset << dec << endl;
   )
#endif
   
   // just ignore the read values. Image is already created (this object).
   //LoadModule *img = new LoadModule(id, cfg_base_addr, string(name));
   //CHECK_COND(img==NULL, "allocating object for image ID %u", id)
      
   // save the file offset where routine data for this image starts
   file_offset = ftell(fd);
      
   if (parse_routines)  // if parse routine data right here
   {
      ires = loadRoutineData(fd);
      if (ires < 0) 
         return (ires);
   }
   if (name) delete[] name;
   
   return (0);
   
load_error:
   if (name) delete[] name;
   return (-1);
}

int 
LoadModule::loadRoutineData(FILE *fd)
{
#undef CHECK_COND
#undef CHECK_COND0

#define CHECK_COND(cond, err, ...) if (cond) \
   {fprintf(stderr, "ERROR: "err"\n", __VA_ARGS__); goto load_error; }
#define CHECK_COND0(cond, err) if (cond) \
   {fprintf(stderr, "ERROR: "err"\n"); goto load_error; }

   size_t res;
   int ires;
   ires = fseek(fd, file_offset, SEEK_SET);
   if (ires < 0)  // error
   {
      perror ("Changing offset to start of routine area failed");
      return (-1);
   }
   
   // next get the number of routines
   uint32_t numRoutines = 0;
   res = fread(&numRoutines, 4, 1, fd);
   CHECK_COND(res!=1 || numRoutines>1024*1024, "reading routine count res=%ld, numRoutines=%u", 
            res, numRoutines)
      
   for (uint32_t r=0 ; r<numRoutines ; ++r)
   {
      Routine *rout = loadOneRoutine(fd, r);
      if (rout == NULL)
         return (-1);

      // add routines to a list
      // Later I will iterate over the list and process them
      //funcMap[_offset] = rout;  // add it to the function map
      funcList.push_back(rout);  // add it to the function list
   }
   return (0);
   
load_error:
   return (-1);
}

Routine* 
LoadModule::loadOneRoutine(FILE *fd, uint32_t r)
{
#undef CHECK_COND
#undef CHECK_COND0

#define CHECK_COND(cond, err, ...) if (cond) \
   {fprintf(stderr, "ERROR: "err"\n", __VA_ARGS__); goto load_error; }
#define CHECK_COND0(cond, err) if (cond) \
   {fprintf(stderr, "ERROR: "err"\n"); goto load_error; }

   size_t res;
#if DEBUG_CFG_COUNTS
   int ires = 0;
#endif
   Routine *rout = NULL;
   
   addrtype _offset, _start, _end;
   size_t _size = 0;
   char *_name = 0;
   // save start/end addresses and name in prefix format (len followed by name)
   res = fread(&_offset, sizeof(addrtype), 1, fd);
   CHECK_COND(res!=1, "reading offset for routine %u", r)
   res = fread(&_start, sizeof(addrtype), 1, fd);
   CHECK_COND(res!=1, "reading start addr for routine %u", r)
   res = fread(&_end, sizeof(addrtype), 1, fd);
   CHECK_COND(res!=1, "reading end addr for routine %u", r)
   _size = _end - _start;
   
   // now read the routine name
   uint32_t len;
   res = fread(&len, 4, 1, fd);
   CHECK_COND(res!=1 || len<1 || len>1024, "reading name length for routine %u, res=%ld, len=%u", 
            r, res, len)
   _name = new char[len+1];
   res = fread(_name, 1, len, fd);
   CHECK_COND(res!=len, "reading name for routine %u", r)
   _name[len] = 0;
#if VERBOSE_DEBUG_LOAD_MODULE
   DEBUG_LOAD_MODULE(4,
      fprintf (stderr, "Creating Routine %s w/ start %p, end %p, offset %p, img_base+offset %p\n",
          _name, (void*)_start, (void*)_end, (void*)_offset, (void*)(base_addr+_offset));
   )
#endif
   //rout = new Routine(this, _start, _size, string(_name), _offset, reloc_offset);

   CHECK_COND(rout==NULL, "allocating object for routine %u", r)
   
#if DEBUG_CFG_COUNTS
   DEBUG_CFG(3,
      fprintf (stderr, "Loading CFG for routine %s\n", _name);
   )
#endif
   // next parse the entries and the cfg
#if DEBUG_CFG_COUNTS
   ires = 
#endif
   //rout->loadCFGFromFile(fd);
   // get_dyninst_cfg(rout);
#if DEBUG_CFG_COUNTS
   DEBUG_CFG(3,
      fprintf (stderr, "CFG loaded with result %d\n", ires);
   )
#endif
//   if (ires < 0) 
//      return (ires);
   if (_name) delete[] _name;
   
   return (rout);
   
load_error:
   if (_name) delete[] _name;
   return (NULL);
}

int 
LoadModule::analyzeRoutines(FILE *fd, ProgScope *prog, const MiamiOptions *mo)
{
   size_t res;
   int ires;
   ires = fseek(fd, file_offset, SEEK_SET);
   if (ires < 0)  // error
   {
      perror ("Changing offset to start of routine area failed");
      return (-1);
   }
   
   // next get the number of routines
   uint32_t numRoutines = 0;
   //res = fread(&numRoutines, 4, 1, fd);
   //if (res!=1 || numRoutines>1024*1024)
      //fprintf(stderr, "ERROR while reading routine count res=%ld, numRoutines=%u\n", 
            //res, numRoutines);
   numRoutines = get_routine_number();
   
   /* create program scope for this load module */   
   img_scope = new ImageScope(prog, img_name, img_id);
   
   /* parse the debug info for this image */
   if (mo->do_linemap)
      InitializeSourceFileInfo();
   
   /* check if we have any cached static analysis results */
   AddrOffsetMap  rOffsets, newROffsets;
   string db_name;
   FILE *fstan=0, *newstan=0;
   char* temp_file_name = 0;
   if (mo->do_staticmem)
   {
      // first, determine the name and the location of the persistent cache
      db_name = mo->persistent_cache;
      char hashbuf[16];
      if (db_name.length()<1) {
         // use the default name; build it from the executable path, then image name followed 
         // by its hash key and a file extension.
         db_name = mo->getExecutablePath();
      }
      if (db_name.length()>0 && db_name[db_name.length()-1]!='/')
      /* TODO: use an OS independent path separator character */
      {
         db_name += '/';
      }
      sprintf(hashbuf, "-0x%08x", hash);
      db_name += MIAMIU::StripPath(img_name) + hashbuf + ".stAn"; 
      
#if VERBOSE_DEBUG_LOAD_MODULE
      DEBUG_LOAD_MODULE(0,
         cerr << "Image " << img_name << ", persistent db name is " << db_name << endl;
      )
#endif
      // check if we have the file and start parsing it
      fstan = fopen(db_name.c_str(), "rb");
      if (fstan == NULL)
      {
#if VERBOSE_DEBUG_LOAD_MODULE
         DEBUG_LOAD_MODULE(0,
            fprintf(stderr, "WARNING: Persistent cache file %s cannot be opened for read access. Start with an empty database.\n",
                  db_name.c_str() );
         )
#endif
      } else
         InitializeLoadStaticAnalysisData (fstan, hash, &rOffsets);
   
      // save the formulas in a temporary file first, to catch all the previously computed formulas. 
      // We will then move the file to the right place.
      // This is a bit involved: get the name of the temp folder, create a template,
      // create a temporary file based on that template, translate file descriptor
      // to file pointer.
      const char* tdir = getTempDirName();
      temp_file_name = new char[strlen(tdir) + 16];
      sprintf(temp_file_name, "%s/miamiXXXXXX", tdir);
      int ftid = mkstemp(temp_file_name);
#if VERBOSE_DEBUG_LOAD_MODULE
      DEBUG_LOAD_MODULE(1,
         fprintf (stderr, "Attempting to use temporary file %s\n", temp_file_name);
      )
#endif
      if (ftid < 0)  // error
      {
         fprintf (stderr, "ERROR: Could not create a unique temporary file. Try using temp_miami.stan in current directory instead.\n");
         strcpy(temp_file_name, "temp_miami.stan");
         newstan = fopen(temp_file_name, "w+");
      } else  // we have the file descriptor, get the FILE stream
      {
         newstan = fdopen(ftid, "w+");
      }
      if (newstan == NULL)
      {
         fprintf (stderr, "WARNING: Could not open the temporary file name (%s) for writing. Any newly computed formulas will not be cached.\n",
                 temp_file_name);
      } else
         InitializeSaveStaticAnalysisData (newstan, hash);
   }
   initialize_dyninst((const char*)this->Name().c_str());


   for (uint32_t r=0 ; r<numRoutines ; ++r)
   {

      Routine *rout = create_routine((LoadModule*)this, r);
      if (rout == NULL)
      {
         std::cout << "analyzeRoutines: routine is NULL\n";
         if (mo->do_linemap)
            FinalizeSourceFileInfo();
         return (-1);
      }
      addrtype rstart = rout->Start();
      if (mo->do_staticmem)
      {
         //std::cout << "analyzeRoutines: we need to do staticmem\n";
         AddrOffsetMap::iterator ait = rOffsets.find(rstart);
         if (ait != rOffsets.end())
         {
            assert(ait->second != 0);
            rout->FetchStaticData(fstan, ait->second);
            ait->second = 0;  // markit as used
         }
      }
      
      if (1/*rout->is_valid_for_analysis()*/)
      {

#if 0  //VERBOSE_DEBUG_LOAD_MODULE
         DEBUG_LOAD_MODULE(1,
            fprintf (stderr, "Starting analysis for routine %s\n", rout->Name().c_str());
         )
#endif   
         ires = rout->main_analysis(img_scope, mo);
         if (ires < 0)
         {
            fprintf (stderr, "Error while analyzing routine %s\n", rout->Name().c_str());
            if (mo->do_linemap)
               FinalizeSourceFileInfo();
            return (-1);
         }
      }
      
      /*if (mo->do_staticmem && newstan)
      {
         std::cout << "analyzeRoutines: 1\n";
         uint64_t foffset = rout->SaveStaticData(newstan);
         newROffsets.insert(AddrOffsetMap::value_type(rstart, foffset));
      }*/
      
      delete (rout);
   }

   //std::cout << "load_module: analyzeRoutines: lineMapping size is " << img_scope->GetLineMappings().size() << endl;
   if (mo->do_staticmem)
   {
      // first iterate over all the entries in rOffsets and save the information about the
      // routines that were not analyzed during the main step
      AddrOffsetMap::iterator ait = rOffsets.begin();
      for ( ; ait!=rOffsets.end() ; ++ait)
      {
         // I set the offset to 0 for entries that I process, so I have to consider only 
         // entries with non-zero offsets
         if (ait->second != 0)
         {
            //std::cout << "analyzeRoutines: 5\n";
            RFormulasMap tempFormulas(0);
            LoadStaticAnalysisData(fstan, ait->second, tempFormulas);
            uint64_t foffset = SaveStaticAnalysisData(newstan, tempFormulas);
            newROffsets.insert(AddrOffsetMap::value_type(ait->first, foffset));
         }
      }
      if (fstan)
         fclose(fstan);
      if (newstan)
      {
         //std::cout << "analyzeRoutines: 6\n";
         FinalizeSaveStaticAnalysisData (newstan, &newROffsets);
         fclose(newstan);
         
         // I just need to copy the content of the temp file to the permanent location
         // we should have a temporary file name because the file descriptor was valid
         assert(temp_file_name);
        // std::cout << "analyzeRoutines: 7\n";
         if (MIAMIU::CopyFile(db_name.c_str(), temp_file_name) < 0)  // error
         {
            perror("MIAMIU::CopyFile");
            fprintf(stderr, "ERROR: Copying the content of file '%s' to '%s'. Static formulas cannot be saved in the persistent cache.\n", 
                  temp_file_name, db_name.c_str());
         }
         unlink(temp_file_name);
      }
      if (temp_file_name)
         delete[] temp_file_name;
   }
   
   /* clean up any memory used by the debug info for this image */
   if (mo->do_linemap)
      FinalizeSourceFileInfo();

   // Set the prog's lineMappings from image's lineMappings
   prog->setLineMappings(img_scope->GetLineMappings());
   //std::cout << "load_module: analyzeRoutines: lineMapping size2 is " << prog->GetLineMappings().size() << endl;
   return (0);
}

// The Allocate versions generate a new index if no entry found. Should be
// used when populating the tables.
// The Get versions return -1 if no entry found. Should be used when trying
// to detect if we have data for a particular entry.
// The PC versions receive the absolute instruction or scope address.
// The Offset version receive an offset into the image.
int32_t 
LoadModule::AllocateIndexForInstPC(addrtype iaddr, int memop)
{
   uint64_t key = ((iaddr-base_addr-low_addr_offset)<<2) + memop;
   int32_t &index = instMapper [key];
   if (index < 0)
   {
      index = nextInstIndex;
      nextInstIndex += 1;
   }  // new instruction index needed
   return (index);
}

int32_t 
LoadModule::AllocateIndexForInstOffset(addrtype iaddr, int memop)
{
   uint64_t key = ((iaddr)<<2) + memop;
   int32_t &index = instMapper [key];
   if (index < 0)
   {
      index = nextInstIndex;
      nextInstIndex += 1;
   }  // new instruction index needed
   return (index);
}

int32_t 
LoadModule::GetIndexForInstPC(addrtype iaddr, int memop)
{
   uint64_t key = ((iaddr-base_addr-low_addr_offset)<<2) + memop;
   unsigned int keyhash = instMapper.pre_hash(key);
   if (instMapper.is_member(key, keyhash))
      return (instMapper(key, keyhash));
   else
      return (-1);
}

int32_t 
LoadModule::GetIndexForInstOffset(addrtype iaddr, int memop)
{
   uint64_t key = (iaddr<<2) + memop;
   unsigned int keyhash = instMapper.pre_hash(key);
   if (instMapper.is_member(key, keyhash))
      return (instMapper(key, keyhash));
   else
      return (-1);
}

#define DEBUG_SCOPE_DETECTION 0
int32_t 
LoadModule::AllocateIndexForScopePC(addrtype saddr, int level)
{
   assert(level < (1<<16));
   if (saddr < (base_addr+low_addr_offset))
   {
      fprintf(stderr, "Error: Img %d, received scope addr 0x%" PRIxaddr " less than base_addr 0x%"
                  PRIxaddr " + low_addr_offset 0x%" PRIxaddr " = 0x%" PRIxaddr "\n",
            img_id, saddr, base_addr, low_addr_offset, base_addr+low_addr_offset);
   }
   uint64_t key = ((saddr-base_addr-low_addr_offset)<<16) + level;
   int32_t &index = scopeMapper [key];
   if (index < 0)
   {
#if DEBUG_SCOPE_DETECTION
         fprintf(stderr, "Image %d, offset=0x%" PRIxaddr ", level=%d --> get ScopeId=%d\n",
            img_id, saddr-base_addr-low_addr_offset, level, nextScopeIndex);
#endif
      index = nextScopeIndex;
      nextScopeIndex += 1;
   }  // new scope index needed
   return (index);
}

int32_t 
LoadModule::AllocateIndexForScopeOffset(addrtype saddr, int level)
{
   assert(level < (1<<16));
   uint64_t key = ((saddr)<<16) + level;
   int32_t &index = scopeMapper [key];
   if (index < 0)
   {
#if DEBUG_SCOPE_DETECTION
         fprintf(stderr, "Image %d, offset=0x%" PRIxaddr ", level=%d --> get ScopeId=%d\n",
            img_id, saddr, level, nextScopeIndex);
#endif
      index = nextScopeIndex;
      nextScopeIndex += 1;
   }  // new scope index needed
   return (index);
}

int32_t 
LoadModule::GetIndexForScopePC(addrtype saddr, int level)
{
   assert(level < (1<<16));
   uint64_t key = ((saddr-base_addr-low_addr_offset)<<16) + level;
   unsigned int keyhash = scopeMapper.pre_hash(key);
   if (scopeMapper.is_member(key, keyhash))
      return (scopeMapper(key, keyhash));
   else
      return (-1);
}

int32_t 
LoadModule::GetIndexForScopeOffset(addrtype saddr, int level)
{
   assert(level < (1<<16));
   uint64_t key = (saddr<<16) + level;
   unsigned int keyhash = scopeMapper.pre_hash(key);
   if (scopeMapper.is_member(key, keyhash))
      return (scopeMapper(key, keyhash));
   else
      return (-1);
}


void
LoadModule::SetScopeIndexForReference(int32_t iidx, int32_t sidx)
{
   assert(iidx>=0 && sidx>=0);
   int32_t capac = refParent.size();
   if (capac <= iidx)
   {
     if (capac < 64)
        capac = 64;
     while (capac <= iidx)
        capac <<= 1;
     refParent.resize(capac);  // resize initializes the values to 0 (I think)
   }
   refParent[iidx] = sidx;
}

void 
LoadModule::SetSIForScope(int child, ScopeImplementation *pscope)
{
   assert(child >= 0);
   long sz = scopeSIPs.size();
   if (sz <= child)
   {
      if (sz < 32)
         sz = 32;
      while (sz <= child)
         sz <<= 1;
      scopeSIPs.resize(sz);  // resize initializes the values to 0 (I think)
   }
   scopeSIPs[child] = pscope;
}

int32_t 
LoadModule::GetSetIndexForReference(int32_t iidx)
{
   assert(iidx > 0);
   unsigned int hash = HashIntMap::pre_hash(iidx);
   if (ref2Set.is_member(iidx, hash))
   {
      return (ref2Set(iidx, hash));
   } else
      return (0);
}

int32_t 
LoadModule::AddReferenceToSet(int32_t iidx, int32_t setId)
{
   assert(setId >= 0);
   if (setId == 0)  // I must allocate a new set and add this reference to it
   {
      setId = nextSetIndex++;
   }
   set2Refs[setId].push_back(iidx);
   ref2Set[iidx] = setId;
   return (setId);
}

void 
LoadModule::SetNameForSetIndex(int32_t setId, const char* name)
{
   // We should duplicate the memory for the variable names.
   // Variable names are probably stored in memory associated with the image. 
   // This memory will get deallocated when the image is unloaded. 
   // In miami, we are loading images manually, one at a time, and then
   // we are closing each image when we are done processing it,
   // Q: Should we store only unique names? Keep a map of names. On a new name
   // search our map, add the name if it is not found, and only store a 
   // pointer to this unique location.
   assert (setId > 0);
   assert (name);
   const char* newname = 0;
   CharPSet::iterator nit = nameStorage.find(name);
   if (nit == nameStorage.end())
   {
      newname = strdup(name);
      nameStorage.insert(newname);
   } else
      newname = *nit;
   assert (newname);
   refNames[setId] = newname;
}

const char* 
LoadModule::GetNameForSetIndex(int32_t setId)
{
   int idx = refNames.indexOf(setId);
   if (idx < 0)
      return (defaultVarName);
   else
      return (refNames.atIndex(idx));
}

/* Names are stored per set. This is a shortcut method
 * to get the name for a reference. It find first the set
 * to which the reference belongs, and then gets its name.
 */
const char* 
LoadModule::GetNameForReference(int32_t iidx)
{
   assert(iidx > 0);
   unsigned int hash = HashIntMap::pre_hash(iidx);
   if (ref2Set.is_member(iidx, hash))
   {
      int idx = refNames.indexOf(ref2Set(iidx, hash));
      if (idx < 0)
         return (defaultVarName);
      else
         return (refNames.atIndex(idx));
   } else
      return (defaultVarName);
}

#define LOOP_IRREG_SHIFT_BITS 32
void 
LoadModule::SetIrregularAccessForSet(int32_t setId, int dist)
{
   assert (setId>0 && dist>=0);
   uint64_t key = ((uint64_t)dist << LOOP_IRREG_SHIFT_BITS) + setId;
   irregSetInfo[key] = 1;
}

bool 
LoadModule::SetHasIrregularAccessAtDistance(int32_t setId, int dist)
{
   if (setId == 0)   // unknown set
      return (false);
   assert (setId>0 && dist>=0);
   uint64_t key = ((uint64_t)dist << LOOP_IRREG_SHIFT_BITS) + setId;
   return (irregSetInfo.is_member(key));
}

