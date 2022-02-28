/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: math_routines.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Support for Fortran intrinsics.
 */

#include <map>
#include <list>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math_routines.h"

namespace MIAMI_DG
{

typedef std::list <int> IntList;

EntryValue::EntryValue (const char* in_regs, const char* out_regs, 
        const char* _name) : rname (_name)
{
   gpSrcRegs = fpSrcRegs = refSrcRegs = 0;
   parse_registers (in_regs, gpSrcRegs, fpSrcRegs, refSrcRegs);
   gpDestRegs = fpDestRegs = refDestRegs = 0;
   parse_registers (out_regs, gpDestRegs, fpDestRegs, refDestRegs);
#if VERBOSE_MATH_ROUTINES
   if (gpSrcRegs)
   {
      int i;
      printf ("Found %d gp src regs:", gpSrcRegs[0]);
      for (i=1 ; i<=gpSrcRegs[0] ; ++i)
         printf (" r%d", gpSrcRegs[i]);
      printf ("\n");
   }
   if (fpSrcRegs)
   {
      int i;
      printf ("Found %d fp src regs:", fpSrcRegs[0]);
      for (i=1 ; i<=fpSrcRegs[0] ; ++i)
         printf (" f%d", fpSrcRegs[i]);
      printf ("\n");
   }
   if (refSrcRegs)
   {
      int i;
      printf ("Found %d ref src regs:", refSrcRegs[0]);
      for (i=1 ; i<=refSrcRegs[0] ; ++i)
         printf (" [r%d]", refSrcRegs[i]);
      printf ("\n");
   }
   if (gpDestRegs)
   {
      int i;
      printf ("Found %d gp dest regs:", gpDestRegs[0]);
      for (i=1 ; i<=gpDestRegs[0] ; ++i)
         printf (" r%d", gpDestRegs[i]);
      printf ("\n");
   }
   if (fpDestRegs)
   {
      int i;
      printf ("Found %d fp dest regs:", fpDestRegs[0]);
      for (i=1 ; i<=fpDestRegs[0] ; ++i)
         printf (" f%d", fpDestRegs[i]);
      printf ("\n");
   }
   if (refDestRegs)
   {
      int i;
      printf ("Found %d ref dest regs:", refDestRegs[0]);
      for (i=1 ; i<=refDestRegs[0] ; ++i)
         printf (" [r%d]", refDestRegs[i]);
      printf ("\n");
   }
#endif
}

EntryValue::~EntryValue ()
{
   if (gpSrcRegs)
      delete[] gpSrcRegs;
   if (fpSrcRegs)
      delete[] fpSrcRegs;
   if (refSrcRegs)
      delete[] refSrcRegs;
   if (gpDestRegs)
      delete[] gpDestRegs;
   if (fpDestRegs)
      delete[] fpDestRegs;
   if (refDestRegs)
      delete[] refDestRegs;
}

void 
EntryValue::parse_registers (const char *in_regs, int*& gpRegsArray, 
          int*&fpRegsArray, int*&refRegsArray)
{
   const char *paux = in_regs;
   char reg [32];
   IntList gpRegs, fpRegs, refRegs;
   int numGp = 0, numFp = 0, numRef = 0;
   
   IntList::iterator lit;
   int l;
   while ((l=sscanf (paux, "%30s", reg)) == 1)
   {
      int len = strlen (reg);
      if (reg[0]=='r')  // general purpose register
      {
         int val = atoi (reg+1);
         ++ numGp;
         gpRegs.push_back (val);
      } else if (reg[0]=='f')  // floating point register
      {
         int val = atoi (reg+1);
         ++ numFp;
         fpRegs.push_back (val);
      } else if (reg[0]=='[' && reg[1]=='r' && reg[len-1]==']')  
      // reference register
      {
         int val = atoi (reg+2);
         ++ numRef;
         refRegs.push_back (val);
      } else   // invalid token
      {
         fprintf (stderr, "ERROR: Invalid token %s in parsing math routine register list: %s\n",
             reg, in_regs);
      }
      while (*paux==' ' || *paux=='\t' || *paux=='\n')
         ++ paux;
      paux = paux + len;
   }
   if (numGp)
   {
      gpRegsArray = new int[numGp+1];
      gpRegsArray[0] = numGp;
      for (lit=gpRegs.begin(), l=1 ; lit!=gpRegs.end() ; ++lit, ++l)
         gpRegsArray[l] = *lit;
      assert (l == numGp+1);
      gpRegs.clear (); numGp = 0;
   }
   if (numFp)
   {
      fpRegsArray = new int[numFp+1];
      fpRegsArray[0] = numFp;
      for (lit=fpRegs.begin(), l=1 ; lit!=fpRegs.end() ; ++lit, ++l)
         fpRegsArray[l] = *lit;
      assert (l == numFp+1);
      fpRegs.clear (); numFp = 0;
   }
   if (numRef)
   {
      refRegsArray = new int[numRef+1];
      refRegsArray[0] = numRef;
      for (lit=refRegs.begin(), l=1 ; lit!=refRegs.end() ; ++lit, ++l)
         refRegsArray[l] = *lit;
      assert (l == numRef+1);
      refRegs.clear (); numRef = 0;
   }
}
  
typedef std::map <unsigned int, EntryValue*> IntrinsicRoutines;

/* define a struct holding the math routines names and their input and 
 * output registers.
 */
 
typedef struct _math_routine_entry
{
   const char * _name;
   const char * _in_regs;  // specify them as a list sepaarated by commas
   const char * _out_regs;
} RoutineEntry;

static IntrinsicRoutines intrinsicMap;
static int special_names_parsed = 0;

#if 0  // I need an x86 implementation of this method, TODO
static RoutineEntry spec_routines [] = {
  { "__r_sign", "[r8] [r9]", "f0" },
  { "__r_dim",  "[r8] [r9]", "f0" },
//  { "__r_exp",  "r8",    "f0" },
  { "__r_imag", "[r8]",    "f0" },
  { "__r_int",  "[r8]",    "f0" },
//  { "__r_log",  "r8",    "f0" },
  { "__r_mod",  "[r8] [r9]", "f0" },
  { "__aint",  "r8 r9", "f0" },
  { "__r_nint", "[r8]",    "f0" },
/*
  { "__r_sin",  "r8",    "f0" },
  { "__r_sind", "r8",    "f0" },
  { "__r_sinh", "r8",    "f0" },
  { "__r_sqrt", "r8",    "f0" },
  { "__r_tan" , "r8",    "f0" },
  { "__r_tand", "r8",    "f0" },
  { "__r_tanh", "r8",    "f0" },
*/
  { 0, 0, 0 }    /* last entry, do not modify */
};

EntryValue*
is_intrinsic_routine_call (executable *exec, addr target)
{
   // fill the map on first call
   if (! special_names_parsed)
   {
      int idx = 0;
      while (spec_routines[idx]._name)
      {
         addr rstart = 0;
         routine *r = exec->which_routine (spec_routines[idx]._name);
         if (r)
            rstart = r->start ();
         else
            rstart = exec->find_symbol_with_name (spec_routines[idx]._name);
         if (rstart)
         {
#if VERBOSE_MATH_ROUTINES
            printf ("Parsing register lists for entry %d, name %s ...\n",
                 idx, spec_routines[idx]._name);
#endif
            intrinsicMap.insert (IntrinsicRoutines::value_type (rstart, 
                 new EntryValue (spec_routines[idx]._in_regs, 
                      spec_routines[idx]._out_regs, 
                      spec_routines[idx]._name) ));
         }
         ++ idx;
      }
      fprintf (stderr, "Parsed special routines names and found %d entries.\n",
          idx);
      special_names_parsed = 1;
   }
   IntrinsicRoutines::iterator sit = intrinsicMap.find (target);
   if (sit != intrinsicMap.end ())
      return (sit->second);
   return (0);
}
#endif  // if 0

void
free_intrinsic_routine_list ()
{
   IntrinsicRoutines::iterator sit = intrinsicMap.begin ();
   for ( ; sit!=intrinsicMap.end () ; ++sit)
      delete (sit->second);
   intrinsicMap.clear ();
   
   special_names_parsed = 0;
}


#if _TEST_MATH_ROUTINES
int 
main (int argc, char **argv)
{
    const char *ss[3] = { "foo", "__r_tan", "bar" };
    int i;
    for (i=0 ; i<3 ; ++i)
       fprintf (stdout, "Is %s a special name? Answer = %p\n",
          ss[i], is_intrinsic_routine_call (ss[i]));
    
    return (0);
}
#endif

} /* namespace MIAMI_DG */
