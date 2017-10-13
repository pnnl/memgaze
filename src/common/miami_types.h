/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: miami_types.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Defines common data types used by MIAMI.
 */

#ifndef MIAMI_TYPES_H
#define MIAMI_TYPES_H

# ifndef __STDC_FORMAT_MACROS
#  define _UNDEF__STDC_FORMAT_MACROS
#  define __STDC_FORMAT_MACROS
# endif
# include <inttypes.h>
# ifdef _UNDEF__STDC_FORMAT_MACROS
#  undef __STDC_FORMAT_MACROS
#  undef _UNDEF__STDC_FORMAT_MACROS
# endif

#include <stdint.h>

#include <map>

namespace MIAMI
{
// I need to define address size to use in other macros
#ifndef SIZEOF_VOIP
# define SIZEOF_VOIDP  8
#endif

typedef unsigned long addrtype;
typedef int32_t ObjId;
typedef uint32_t usize_t;
typedef uint32_t card;
typedef int RegName;
typedef uint16_t width_t;  // type for operand bit width values

typedef int64_t  imm_signed_t;
typedef uint64_t imm_unsigned_t;

struct imm_value_t {
   imm_value_t()
   {
      is_signed = true;
      value.s = 0;
   }
   imm_value_t(imm_signed_t v)
   {
      is_signed = true;
      value.s = v;
   }
   imm_value_t(imm_unsigned_t v)
   {
      is_signed = false;
      value.u = v;
   }
   union {
      imm_signed_t   s;
      imm_unsigned_t u;
   } value;
   bool is_signed;
};
//ozgurS moving inslvlmap and mem struct to here
struct memStruct {
   int level;
   int hitCount;
   double latency;
};

typedef std::map<int,memStruct> InstlvlMap;
//ozgurE
typedef int64_t coeff_t;
typedef uint64_t ucoeff_t;

inline coeff_t abs_coeff(coeff_t x) {
  return (x < 0) ? -x : x;
}

// define the correct printf format for each custom type
#define PRIwidth  PRIu16
#define PRIobjid  PRId32
#define PRIaddr   "lu"
#define PRIxaddr  "lx"
#define PRIreg    "d"
#define PRIsize   PRIu32

#define PRIcoeff   PRId64
#define PRIxcoeff  PRIx64
#define PRIucoeff  PRIu64
#define PRIxucoeff PRIx64


#define MIAMI_NO_ADDRESS ((addrtype)-1)

/* declarations of global constants */
extern const char* persistent_db_magic_word;
extern int32_t     persistent_db_version;
extern int32_t     persistent_db_min_required_version;

}  /* namespace MIAMI */

#endif
