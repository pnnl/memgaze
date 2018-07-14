
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "machine_description.y"

/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: machine_description.y
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: The yacc part of the parser for the MIAMI Machine 
 * Description Language.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <instr_bins.H>

#include "position.h"
#include "MachineExecutionUnit.h"
#include "MemoryHierarchyLevel.h"
#include "TemplateExecutionUnit.h"
#include "Machine.h"
#include "BitSet.h"
#include "InstTemplate.h"
#include "Instruction.h"
#include "GenericInstruction.h"
#include "ReplacementRule.h"
#include "BypassRule.h"

#include "debug_scheduler.h"

using namespace MIAMI;

namespace MIAMI
{

extern Position currentPosition;
static Position startPosition;  // temporary variable to keep the start 
                                // of an expression

// other temporary variables to help parse the instruction templates
static int unit;
static int unitReplication;
static BitSet* templateMask;

extern int haveErrors;

// keep the defined execution units in a global variable so I can check 
// the correctness of the instruction templates
static ExecUnitAssocTable *cpuUnits;
static ExecUnitAssocTable *asyncUnits;
static MemLevelAssocTable *memLevels;
static int numUnits;
//static Instruction* instTemplates;
static InstructionContainer* instTemplates;
static ITemplateList templateList;
static ReplacementList* replacementRules;
static BypassList* bypassRules;
static RestrictionList *restrictRules;
static int window_size;

static Machine* currentMachine;
} /* namespace MIAMI */

extern int yyleng;
extern char *yytext;

#if defined(__cplusplus) || defined(__STDC__)

#if defined(__cplusplus) && defined(__EXTERN_C__)
extern "C" {
#endif
#ifndef yyerror
#if defined(__cplusplus)
        void yyerror(const char *);
#endif
#endif
#ifndef yylex
#ifdef YYLEX_PARAM
        int yylex (void *YYLEX_PARAM)
#else
        int yylex(void); 
#endif
#endif
#if defined(__cplusplus) && defined(__EXTERN_C__)
}
#endif

#endif



/* Line 189 of yacc.c  */
#line 167 "machdesc.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_OR = 258,
     T_TIMES = 259,
     T_PLUS = 260,
     T_PLUSPLUS = 261,
     T_COLON = 262,
     T_SEMICOLON = 263,
     T_COMMA = 264,
     T_MACHINE = 265,
     T_VERSION = 266,
     T_ASSIGN = 267,
     T_REPLACE = 268,
     T_INSTRUCTION = 269,
     T_TEMPLATE = 270,
     T_CPU_UNITS = 271,
     T_WITH = 272,
     T_NOTHING = 273,
     T_ALL_UNITS = 274,
     T_LBRACKET = 275,
     T_RBRACKET = 276,
     T_LPAREN = 277,
     T_RPAREN = 278,
     T_ARROW = 279,
     T_MAXIMUM = 280,
     T_FROM = 281,
     T_LCPAREN = 282,
     T_RCPAREN = 283,
     T_ANY_INSTRUCTION = 284,
     T_BYPASS = 285,
     T_LATENCY = 286,
     T_FOR = 287,
     T_MEMORY = 288,
     T_CONTROL = 289,
     T_ADDR_REGISTER = 290,
     T_GP_REGISTER = 291,
     T_MEMORY_HIERARCHY = 292,
     T_VECTOR_UNIT = 293,
     T_RETIRE = 294,
     T_WINDOW_SIZE = 295,
     T_ASYNC_RESOURCES = 296,
     T_INT_CONST = 297,
     T_INST_TYPE = 298,
     T_DEPENDENCY_TYPE = 299,
     T_UNIT_TYPE = 300,
     T_FLOAT_CONST = 301,
     T_CHAR_CONST = 302,
     T_IDENTIFIER = 303,
     T_STR_CONST = 304,
     T_GP_REG_NAME = 305
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 95 "machine_description.y"

  int int_value;
  float fl_value;
  char ch_value;
  MIAMI::Position position;
  MIAMI::UnitCountPair ucPair;
  char*  p_char;
  MIAMI::InstructionClass iclass;
  MIAMI::Machine* p_machine;
  MIAMI::ExecUnitAssocTable* p_EUAT;
  MIAMI::MachineExecutionUnit* p_MEU;
  MIAMI::MemLevelAssocTable* p_MLAT;
  MIAMI::MemoryHierarchyLevel* p_MHL;
  MIAMI::TemplateExecutionUnit* p_TEU;
  MIAMI::TEUList* p_TEUList;
  MIAMI::UnitCountList *p_ucList;
  MIAMI::ICSet* p_icset;
  MIAMI::BitSet* p_bitset;
  MIAMI::InstTemplate* p_itemplate;
  MIAMI::Instruction* p_instruction;
  MIAMI::GenRegList* p_reglist;
  MIAMI::GenericInstruction* p_geninst;
  MIAMI::GenInstList* p_geninstlist;
  MIAMI::UnitRestriction* p_restriction;
  MIAMI::RestrictionList* p_restrlist;



/* Line 214 of yacc.c  */
#line 282 "machdesc.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 294 "machdesc.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  6
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   272

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  51
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  67
/* YYNRULES -- Number of rules.  */
#define YYNRULES  129
/* YYNRULES -- Number of states.  */
#define YYNSTATES  259

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   305

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     6,     7,    15,    19,    25,    27,
      28,    34,    35,    36,    42,    43,    47,    48,    54,    55,
      58,    62,    68,    74,    81,    87,    92,    93,    97,    99,
     101,   105,   106,   110,   112,   115,   117,   119,   121,   123,
     125,   127,   129,   130,   136,   137,   141,   142,   148,   164,
     166,   168,   170,   172,   174,   176,   178,   180,   182,   184,
     186,   188,   190,   192,   194,   195,   196,   205,   207,   211,
     215,   220,   224,   228,   231,   233,   237,   238,   243,   244,
     251,   252,   262,   264,   268,   270,   272,   277,   279,   283,
     284,   292,   293,   304,   306,   310,   319,   328,   335,   340,
     347,   359,   371,   381,   383,   387,   389,   393,   397,   403,
     405,   409,   410,   414,   416,   418,   420,   424,   425,   430,
     431,   435,   437,   441,   443,   447,   453,   454,   456,   457
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      52,     0,    -1,    53,    -1,    -1,    -1,    56,    57,    58,
      54,    60,    55,    73,    -1,    10,     7,    49,    -1,     9,
      11,     7,    49,     8,    -1,     8,    -1,    -1,    16,    59,
      12,    62,     8,    -1,    -1,    -1,    41,    61,    12,    62,
       8,    -1,    -1,    48,    63,    65,    -1,    -1,    62,     9,
      48,    64,    65,    -1,    -1,     4,    42,    -1,    27,    49,
      28,    -1,     4,    42,    27,    49,    28,    -1,    27,    49,
      28,     4,    42,    -1,    25,    42,    26,    70,    69,     8,
      -1,    39,    12,    42,    69,     8,    -1,    40,    12,    42,
       8,    -1,    -1,    27,    49,    28,    -1,    19,    -1,    71,
      -1,    70,     9,    71,    -1,    -1,    48,    72,   113,    -1,
      74,    -1,    73,    74,    -1,    88,    -1,   101,    -1,    96,
      -1,    66,    -1,    67,    -1,    68,    -1,    75,    -1,    -1,
      37,    76,    12,    77,     8,    -1,    -1,    48,    78,    80,
      -1,    -1,    77,     9,    48,    79,    80,    -1,    20,    81,
       9,    82,     9,    83,     9,    84,     9,    85,     9,    86,
       9,    87,    21,    -1,    42,    -1,     4,    -1,    42,    -1,
       4,    -1,    42,    -1,     4,    -1,    42,    -1,     4,    -1,
      46,    -1,    42,    -1,     4,    -1,    48,    -1,     4,    -1,
      42,    -1,     4,    -1,    -1,    -1,    13,    89,    91,    17,
      90,    91,    69,     8,    -1,    92,    -1,    91,     5,    92,
      -1,    91,     6,    92,    -1,   104,    93,    24,    93,    -1,
     104,    93,    24,    -1,   104,    24,    93,    -1,   104,    24,
      -1,    50,    -1,    93,     9,    50,    -1,    -1,    20,    50,
      94,    21,    -1,    -1,    93,     9,    20,    50,    95,    21,
      -1,    -1,    30,    97,    31,    42,    32,    98,    99,    98,
       8,    -1,   104,    -1,    98,     3,   104,    -1,    29,    -1,
      24,    -1,    24,    20,   100,    21,    -1,    44,    -1,   100,
       3,    44,    -1,    -1,    14,   104,   102,    15,    12,   105,
       8,    -1,    -1,    14,   104,   103,    20,   107,    21,    15,
      12,   105,     8,    -1,    43,    -1,    43,     7,    45,    -1,
      43,     7,    45,     9,    38,    27,    42,    28,    -1,    43,
       7,    38,    27,    42,    28,     9,    45,    -1,    43,     7,
      38,    27,    42,    28,    -1,    43,    27,    42,    28,    -1,
      43,    27,    42,    28,     7,    45,    -1,    43,    27,    42,
      28,     7,    45,     9,    38,    27,    42,    28,    -1,    43,
      27,    42,    28,     7,    38,    27,    42,    28,     9,    45,
      -1,    43,    27,    42,    28,     7,    38,    27,    42,    28,
      -1,   106,    -1,   105,     3,   106,    -1,   110,    -1,   106,
       9,   110,    -1,   110,     4,    42,    -1,   106,     9,   110,
       4,    42,    -1,   108,    -1,   107,     5,   108,    -1,    -1,
      48,   109,   117,    -1,    18,    -1,    19,    -1,   111,    -1,
     110,     5,   111,    -1,    -1,    48,   112,   113,   117,    -1,
      -1,    20,   114,    21,    -1,   115,    -1,   114,     9,   115,
      -1,    42,    -1,   116,     7,   116,    -1,   116,     7,   116,
       7,   116,    -1,    -1,    42,    -1,    -1,    22,    42,    23,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   169,   169,   184,   191,   183,   227,   230,   231,   236,
     235,   247,   251,   250,   262,   261,   270,   269,   280,   284,
     288,   292,   296,   303,   311,   319,   327,   330,   337,   348,
     353,   362,   361,   387,   391,   398,   402,   406,   410,   414,
     418,   422,   430,   429,   457,   456,   465,   464,   475,   483,
     484,   488,   489,   492,   493,   496,   497,   500,   501,   502,
     505,   506,   509,   510,   515,   519,   514,   530,   535,   544,
     552,   556,   560,   564,   570,   577,   584,   584,   591,   590,
     599,   599,   611,   616,   621,   629,   630,   634,   640,   649,
     649,   654,   654,   669,   674,   679,   684,   689,   694,   699,
     704,   709,   714,   722,   729,   737,   742,   747,   752,   760,
     765,   774,   773,   797,   802,   813,   818,   833,   832,   887,
     890,   897,   901,   908,   913,   923,   938,   941,   948,   951
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_OR", "T_TIMES", "T_PLUS",
  "T_PLUSPLUS", "T_COLON", "T_SEMICOLON", "T_COMMA", "T_MACHINE",
  "T_VERSION", "T_ASSIGN", "T_REPLACE", "T_INSTRUCTION", "T_TEMPLATE",
  "T_CPU_UNITS", "T_WITH", "T_NOTHING", "T_ALL_UNITS", "T_LBRACKET",
  "T_RBRACKET", "T_LPAREN", "T_RPAREN", "T_ARROW", "T_MAXIMUM", "T_FROM",
  "T_LCPAREN", "T_RCPAREN", "T_ANY_INSTRUCTION", "T_BYPASS", "T_LATENCY",
  "T_FOR", "T_MEMORY", "T_CONTROL", "T_ADDR_REGISTER", "T_GP_REGISTER",
  "T_MEMORY_HIERARCHY", "T_VECTOR_UNIT", "T_RETIRE", "T_WINDOW_SIZE",
  "T_ASYNC_RESOURCES", "T_INT_CONST", "T_INST_TYPE", "T_DEPENDENCY_TYPE",
  "T_UNIT_TYPE", "T_FLOAT_CONST", "T_CHAR_CONST", "T_IDENTIFIER",
  "T_STR_CONST", "T_GP_REG_NAME", "$accept", "start", "MachineDescription",
  "@1", "$@2", "TargetName", "TargetVersion", "ExecUnits", "@3",
  "AsyncResources", "@4", "CpuUnitList", "$@5", "$@6", "UnitInformation",
  "UnitsRestriction", "RetireRestriction", "WindowSize", "RuleName",
  "UnitsList", "RestrictionUnit", "$@7", "InstructionDescriptions",
  "MachineRule", "MemoryHierarchyDef", "@8", "MemoryLevelsList", "$@9",
  "$@10", "MemLevelInformation", "MemNumBlocks", "MemBlockSize",
  "MemEntrySize", "MemAssocLevel", "MemBandwidth", "MemNextLevel",
  "MemPenalty", "InstructionReplacement", "@11", "@12",
  "ListGenericInstructions", "GenericInstruction", "ListRegisters", "@13",
  "@14", "InstructionBypass", "@15", "ListInstructionTypes",
  "DependencyTypes", "ListDependencyTypes", "InstructionExecution", "@16",
  "@17", "InstType", "InstTemplates", "InstTemplate", "AsyncUsage",
  "AsyncClass", "$@18", "CycleTemplate", "UnitClass", "$@19", "UnitRange",
  "RangeIntervals", "OneInterval", "IntervalLimit", "UnitCount", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    51,    52,    54,    55,    53,    56,    57,    57,    59,
      58,    60,    61,    60,    63,    62,    64,    62,    65,    65,
      65,    65,    65,    66,    67,    68,    69,    69,    70,    70,
      70,    72,    71,    73,    73,    74,    74,    74,    74,    74,
      74,    74,    76,    75,    78,    77,    79,    77,    80,    81,
      81,    82,    82,    83,    83,    84,    84,    85,    85,    85,
      86,    86,    87,    87,    89,    90,    88,    91,    91,    91,
      92,    92,    92,    92,    93,    93,    94,    93,    95,    93,
      97,    96,    98,    98,    98,    99,    99,   100,   100,   102,
     101,   103,   101,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   104,   105,   105,   106,   106,   106,   106,   107,
     107,   109,   108,   110,   110,   110,   110,   112,   111,   113,
     113,   114,   114,   115,   115,   115,   116,   116,   117,   117
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     0,     7,     3,     5,     1,     0,
       5,     0,     0,     5,     0,     3,     0,     5,     0,     2,
       3,     5,     5,     6,     5,     4,     0,     3,     1,     1,
       3,     0,     3,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     0,     5,     0,     3,     0,     5,    15,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     0,     0,     8,     1,     3,     3,
       4,     3,     3,     2,     1,     3,     0,     4,     0,     6,
       0,     9,     1,     3,     1,     1,     4,     1,     3,     0,
       7,     0,    10,     1,     3,     8,     8,     6,     4,     6,
      11,    11,     9,     1,     3,     1,     3,     3,     5,     1,
       3,     0,     3,     1,     1,     1,     3,     0,     4,     0,
       3,     1,     3,     1,     3,     5,     0,     1,     0,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     2,     0,     0,     1,     8,     0,     0,
       6,     0,     9,     3,     0,     0,    11,     0,     0,    12,
       4,     7,    14,     0,     0,     0,    18,    10,     0,     0,
      64,     0,     0,    80,    42,     0,     0,    38,    39,    40,
       5,    33,    41,    35,    37,    36,     0,     0,    15,    16,
       0,     0,    93,    89,     0,     0,     0,     0,     0,    34,
      19,     0,    18,    13,     0,    67,     0,     0,     0,     0,
       0,     0,     0,     0,    26,     0,     0,    20,    17,     0,
       0,    65,     0,    73,    74,     0,     0,    94,     0,     0,
       0,    28,    31,    26,    29,     0,    44,     0,     0,     0,
      25,     0,     0,    68,    69,     0,    76,    72,     0,    71,
       0,     0,    98,     0,   111,     0,   109,   119,     0,     0,
       0,     0,    43,     0,     0,    24,    21,    22,    26,     0,
       0,    75,    70,     0,     0,     0,   113,   114,   117,     0,
     103,   105,   115,   128,     0,     0,   126,    32,    30,    23,
      84,     0,    82,     0,    45,    46,    27,     0,    77,    78,
      97,     0,     0,    99,   119,     0,    90,     0,     0,     0,
       0,   112,   110,     0,   123,     0,   121,     0,     0,    85,
       0,    50,    49,     0,     0,    66,     0,     0,     0,     0,
       0,   128,   104,   106,   107,   116,     0,     0,   126,   120,
     126,    83,     0,     0,     0,    47,    79,    96,    95,     0,
       0,   118,     0,   129,     0,   122,   127,   124,    87,     0,
      81,    52,    51,     0,   102,     0,   108,    92,   126,     0,
      86,     0,     0,     0,   125,    88,    54,    53,     0,   101,
     100,     0,    56,    55,     0,     0,    59,    58,    57,     0,
       0,    61,    60,     0,     0,    63,    62,     0,    48
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,    16,    25,     4,     9,    13,    15,    20,
      24,    23,    26,    62,    48,    37,    38,    39,    99,    93,
      94,   117,    40,    41,    42,    56,    97,   121,   184,   154,
     183,   223,   238,   244,   249,   253,   257,    43,    51,   105,
      64,    65,    85,   129,   186,    44,    55,   151,   180,   219,
      45,    69,    70,    66,   139,   140,   115,   116,   143,   141,
     142,   164,   147,   175,   176,   177,   171
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -190
static const yytype_int16 yypact[] =
{
      14,    20,    67,  -190,    73,    21,  -190,  -190,    48,    80,
    -190,    90,  -190,  -190,    24,    87,    60,    92,    50,  -190,
    -190,  -190,  -190,    75,    91,    38,    22,  -190,    54,    50,
    -190,    61,    64,  -190,  -190,    93,    95,  -190,  -190,  -190,
      38,  -190,  -190,  -190,  -190,  -190,    68,    62,  -190,  -190,
      77,    61,    49,    89,    82,    81,   101,    72,    74,  -190,
      94,    96,    22,  -190,    13,  -190,   -14,   -16,    76,   100,
      97,     6,    78,    71,    98,   114,    83,   119,  -190,    61,
      61,  -190,    84,   -13,  -190,     8,    99,   118,   102,   116,
      85,  -190,  -190,    52,  -190,   103,  -190,    79,   104,   121,
    -190,   108,   106,  -190,  -190,    61,  -190,   122,     3,   -13,
     109,   105,   130,     2,  -190,    10,  -190,   120,   107,   131,
      29,   124,  -190,   110,   113,  -190,  -190,  -190,    28,   117,
     111,  -190,   122,   126,   115,    19,  -190,  -190,  -190,    57,
     136,    86,  -190,   128,    85,   137,   123,  -190,  -190,  -190,
    -190,    11,  -190,    -1,  -190,  -190,  -190,   138,  -190,  -190,
     147,   125,   132,   148,   120,     2,  -190,     2,   127,   112,
     129,  -190,  -190,   150,   156,     7,  -190,   157,    61,   146,
      29,  -190,  -190,   159,   124,  -190,   149,   133,   144,   134,
     135,   128,   136,    88,  -190,  -190,   151,     2,   123,  -190,
     139,  -190,   140,    63,     0,  -190,  -190,  -190,  -190,   152,
     155,  -190,   141,  -190,    66,  -190,  -190,   168,  -190,    59,
    -190,  -190,  -190,   170,   176,   145,  -190,  -190,   139,   142,
    -190,     1,   143,   161,  -190,  -190,  -190,  -190,   181,  -190,
    -190,     4,  -190,  -190,   182,    -2,  -190,  -190,  -190,   183,
      -3,  -190,  -190,   184,     5,  -190,  -190,   173,  -190
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -190,  -190,  -190,  -190,  -190,  -190,  -190,  -190,  -190,  -190,
    -190,   166,  -190,  -190,   153,  -190,  -190,  -190,   -80,  -190,
     154,  -190,  -190,   158,  -190,  -190,  -190,  -190,  -190,    -7,
    -190,  -190,  -190,  -190,  -190,  -190,  -190,  -190,  -190,  -190,
     160,    15,   -71,  -190,  -190,  -190,  -190,    16,  -190,  -190,
    -190,  -190,  -190,   -31,     9,    32,  -190,    55,  -190,    33,
      34,  -190,    37,  -190,    12,  -189,    17
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -128
static const yytype_int16 yytable[] =
{
      53,   251,   246,   181,   221,   236,    82,    82,   242,   255,
      83,   217,   107,   119,   178,   144,   198,   108,    79,    80,
     136,   137,    86,   130,     1,    91,    46,     5,   199,    87,
      81,   145,   109,    79,    80,   179,    84,    84,   132,   234,
     247,   182,   222,   237,   248,   252,   243,   256,   157,    47,
     138,    30,    31,   131,    92,    98,    67,   162,   150,    11,
     165,   118,   229,    32,   163,   166,   178,     6,    33,   165,
      10,   220,    52,    17,   227,    34,    68,    35,    36,    98,
     230,     7,     8,    27,    28,    63,    28,   122,   123,   152,
     168,   169,   212,   169,   103,   104,    12,    14,    22,    18,
      21,    19,    49,    29,    52,    57,    54,    58,    71,   -91,
      60,    61,    72,    73,    74,    89,    75,    90,    88,    96,
      95,    76,   100,   102,    77,    98,   110,   111,   113,   125,
     112,   108,   101,   114,   106,   120,   126,   135,   158,   149,
     146,   156,   161,   134,   153,   167,   185,   201,   127,   152,
     170,   133,   173,   124,   160,    92,   187,   190,   155,   189,
     138,   159,   197,  -127,   200,   174,   202,   188,   204,   194,
     206,   196,   208,   210,   213,   228,   209,   205,   207,   231,
     224,   216,   225,   226,   218,   232,   235,   233,   239,   240,
     241,   245,   250,   254,   258,    50,   203,   192,    59,   172,
     193,   191,     0,   195,     0,     0,   214,     0,   211,     0,
     215,     0,     0,     0,     0,    78,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   128,     0,     0,     0,     0,
       0,     0,   148
};

static const yytype_int16 yycheck[] =
{
      31,     4,     4,     4,     4,     4,    20,    20,     4,     4,
      24,   200,    83,    93,     3,     5,     9,     9,     5,     6,
      18,    19,    38,    20,    10,    19,     4,     7,    21,    45,
      17,    21,    24,     5,     6,    24,    50,    50,   109,   228,
      42,    42,    42,    42,    46,    48,    42,    42,   128,    27,
      48,    13,    14,    50,    48,    27,     7,    38,    29,    11,
       3,     9,     3,    25,    45,     8,     3,     0,    30,     3,
      49,     8,    43,    49,     8,    37,    27,    39,    40,    27,
      21,     8,     9,     8,     9,     8,     9,     8,     9,   120,
       4,     5,     4,     5,    79,    80,    16,     7,    48,    12,
       8,    41,    48,    12,    43,    12,    42,    12,    26,    20,
      42,    49,    31,    12,    42,    15,    42,    20,    42,    48,
      42,    27,     8,     4,    28,    27,    27,     9,    12,     8,
      28,     9,    49,    48,    50,    32,    28,     7,    21,     8,
      20,    28,    27,    38,    20,     9,     8,   178,    42,   180,
      22,    42,    15,    49,    28,    48,     9,     9,    48,    27,
      48,    50,    12,     7,     7,    42,    20,    42,     9,    42,
      21,    42,    28,    38,    23,     7,    42,   184,    45,     9,
      28,    42,    27,    42,    44,     9,    44,    42,    45,    28,
       9,     9,     9,     9,    21,    29,   180,   165,    40,   144,
     167,   164,    -1,   169,    -1,    -1,   197,    -1,   191,    -1,
     198,    -1,    -1,    -1,    -1,    62,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   105,    -1,    -1,    -1,    -1,
      -1,    -1,   118
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    10,    52,    53,    56,     7,     0,     8,     9,    57,
      49,    11,    16,    58,     7,    59,    54,    49,    12,    41,
      60,     8,    48,    62,    61,    55,    63,     8,     9,    12,
      13,    14,    25,    30,    37,    39,    40,    66,    67,    68,
      73,    74,    75,    88,    96,   101,     4,    27,    65,    48,
      62,    89,    43,   104,    42,    97,    76,    12,    12,    74,
      42,    49,    64,     8,    91,    92,   104,     7,    27,   102,
     103,    26,    31,    12,    42,    42,    27,    28,    65,     5,
       6,    17,    20,    24,    50,    93,    38,    45,    42,    15,
      20,    19,    48,    70,    71,    42,    48,    77,    27,    69,
       8,    49,     4,    92,    92,    90,    50,    93,     9,    24,
      27,     9,    28,    12,    48,   107,   108,    72,     9,    69,
      32,    78,     8,     9,    49,     8,    28,    42,    91,    94,
      20,    50,    93,    42,    38,     7,    18,    19,    48,   105,
     106,   110,   111,   109,     5,    21,    20,   113,    71,     8,
      29,    98,   104,    20,    80,    48,    28,    69,    21,    50,
      28,    27,    38,    45,   112,     3,     8,     9,     4,     5,
      22,   117,   108,    15,    42,   114,   115,   116,     3,    24,
      99,     4,    42,    81,    79,     8,    95,     9,    42,    27,
       9,   113,   106,   110,    42,   111,    42,    12,     9,    21,
       7,   104,    20,    98,     9,    80,    21,    45,    28,    42,
      38,   117,     4,    23,   105,   115,    42,   116,    44,   100,
       8,     4,    42,    82,    28,    27,    42,     8,     7,     3,
      21,     9,     9,    42,   116,    44,     4,    42,    83,    45,
      28,     9,     4,    42,    84,     9,     4,    42,    46,    85,
       9,     4,    48,    86,     9,     4,    42,    87,    21
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 170 "machine_description.y"
    {
#if DEBUG_MACHINE_CONSTRUCTION
           DEBUG_MACHINE (3, 
              printf("Machine description parsing is complete.\n");
              (yyvsp[(1) - (1)].p_machine)->print(stdout);
           )
#endif
           currentMachine = (yyvsp[(1) - (1)].p_machine);
           // add_target($1);
        ;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 184 "machine_description.y"
    {
            (yyval.p_machine) = new Machine ((yyvsp[(1) - (3)].p_char), (yyvsp[(2) - (3)].p_char));
            (yyvsp[(3) - (3)].p_EUAT)->FinalizeMapping ();
            (yyval.p_machine)->addUnits ((yyvsp[(3) - (3)].p_EUAT)); cpuUnits = (yyvsp[(3) - (3)].p_EUAT);
            numUnits = cpuUnits->NumberOfElements ();
         ;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 191 "machine_description.y"
    {
            if ((yyvsp[(5) - (5)].p_EUAT))
            {
               (yyvsp[(5) - (5)].p_EUAT)->FinalizeMapping ();
               (yyvsp[(4) - (5)].p_machine)->addAsyncUnits ((yyvsp[(5) - (5)].p_EUAT)); asyncUnits = (yyvsp[(5) - (5)].p_EUAT);
            } else
            {
               asyncUnits = 0;
            }
         ;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 202 "machine_description.y"
    {
#if DEBUG_MACHINE_CONSTRUCTION
            DEBUG_MACHINE (1, 
               fprintf(stdout, "Finished parsing the instruction descriptions. Saw %d rules.\n",
                      (yyvsp[(7) - (7)].int_value));
            )
#endif
            (yyvsp[(4) - (7)].p_machine)->addInstructionDescriptions (instTemplates);
            instTemplates = NULL;
            (yyvsp[(4) - (7)].p_machine)->addReplacementRules (replacementRules);
            replacementRules = NULL;
            (yyvsp[(4) - (7)].p_machine)->addBypassRules (bypassRules);
            bypassRules = NULL;
            (yyvsp[(4) - (7)].p_machine)->addUnitRestrictions (restrictRules);
            restrictRules = NULL;
            (yyvsp[(4) - (7)].p_machine)->setMachineFeature (MachineFeature_WINDOW_SIZE, window_size);
            if (memLevels)
               memLevels->FinalizeMapping ();
            (yyvsp[(4) - (7)].p_machine)->addMemoryLevels (memLevels);
            memLevels = NULL;
            (yyval.p_machine) = (yyvsp[(4) - (7)].p_machine);
         ;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 227 "machine_description.y"
    { (yyval.p_char) = (yyvsp[(3) - (3)].p_char); ;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 230 "machine_description.y"
    { (yyval.p_char) = (yyvsp[(4) - (5)].p_char); ;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 231 "machine_description.y"
    { (yyval.p_char) = NULL; ;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 236 "machine_description.y"
    {
            (yyval.position) = currentPosition - yyleng;
         ;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 240 "machine_description.y"
    { 
            (yyvsp[(4) - (5)].p_EUAT)->setStartPosition ((yyvsp[(2) - (5)].position));
            (yyval.p_EUAT) = (yyvsp[(4) - (5)].p_EUAT); 
         ;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 247 "machine_description.y"
    {
            (yyval.p_EUAT) = 0;
         ;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 251 "machine_description.y"
    {
            (yyval.position) = currentPosition - yyleng;
         ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 255 "machine_description.y"
    { 
            (yyvsp[(4) - (5)].p_EUAT)->setStartPosition ((yyvsp[(2) - (5)].position));
            (yyval.p_EUAT) = (yyvsp[(4) - (5)].p_EUAT); 
         ;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 262 "machine_description.y"
    { 
            startPosition = currentPosition - yyleng;
         ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 265 "machine_description.y"
    { 
            (yyval.p_EUAT) = new MIAMI::ExecUnitAssocTable();
            (yyval.p_EUAT)->addElement ((yyvsp[(1) - (3)].p_char), (yyvsp[(3) - (3)].p_MEU), startPosition);
         ;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 270 "machine_description.y"
    { 
            startPosition = currentPosition - yyleng;
         ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 273 "machine_description.y"
    {
            (yyvsp[(1) - (5)].p_EUAT)->addElement ((yyvsp[(3) - (5)].p_char), (yyvsp[(5) - (5)].p_MEU), startPosition); 
            (yyval.p_EUAT) = (yyvsp[(1) - (5)].p_EUAT);
         ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 280 "machine_description.y"
    {
            /* default constructor sets count to 1 */
            (yyval.p_MEU) = new MachineExecutionUnit (); 
         ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 285 "machine_description.y"
    {
            (yyval.p_MEU) = new MachineExecutionUnit ((yyvsp[(2) - (2)].int_value), NULL, currentPosition); 
         ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 289 "machine_description.y"
    {
            (yyval.p_MEU) = new MachineExecutionUnit (1, (yyvsp[(2) - (3)].p_char), currentPosition); 
         ;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 293 "machine_description.y"
    {
            (yyval.p_MEU) = new MachineExecutionUnit ((yyvsp[(2) - (5)].int_value), (yyvsp[(4) - (5)].p_char), currentPosition); 
         ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 297 "machine_description.y"
    {
            (yyval.p_MEU) = new MachineExecutionUnit ((yyvsp[(5) - (5)].int_value), (yyvsp[(2) - (5)].p_char), currentPosition); 
         ;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 304 "machine_description.y"
    {
            restrictRules->push_back (new UnitRestriction((yyvsp[(4) - (6)].p_TEUList), (yyvsp[(2) - (6)].int_value), (yyvsp[(5) - (6)].p_char)));
            (yyval.int_value) = 1;
         ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 312 "machine_description.y"
    {
            restrictRules->push_back (new UnitRestriction(NULL, (yyvsp[(3) - (5)].int_value), (yyvsp[(4) - (5)].p_char)));
            (yyval.int_value) = 1;
         ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 320 "machine_description.y"
    {
            window_size = (yyvsp[(3) - (4)].int_value);
            (yyval.int_value) = 1;
         ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 327 "machine_description.y"
    {
            (yyval.p_char) = NULL;
         ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 331 "machine_description.y"
    {
            (yyval.p_char) = (yyvsp[(2) - (3)].p_char);
         ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 338 "machine_description.y"
    {
            startPosition = currentPosition - yyleng;
            (yyval.p_TEUList) = new TEUList ();
            for (int i=0 ; i<numUnits ; ++i)
            {
               int _count = cpuUnits->getElementAtIndex (i) ->getCount ();
               BitSet *_umask = new BitSet (~BitSet(_count));
               (yyval.p_TEUList)->push_back (new TemplateExecutionUnit (i, _umask, 0));
            }
         ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 349 "machine_description.y"
    {
            (yyval.p_TEUList) = new TEUList ();
            (yyval.p_TEUList)->push_back ((yyvsp[(1) - (1)].p_TEU));
         ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 354 "machine_description.y"
    {
            (yyvsp[(1) - (3)].p_TEUList)->push_back ((yyvsp[(3) - (3)].p_TEU));
            (yyval.p_TEUList) = (yyvsp[(1) - (3)].p_TEUList);
         ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 362 "machine_description.y"
    {
            startPosition = currentPosition - yyleng;
            // set in a global variable the number of units of this type
            unit = cpuUnits->getMappingForName ((yyvsp[(1) - (1)].p_char), startPosition);
            if (unit != NO_MAPPING)
               unitReplication = cpuUnits->getElementAtIndex (unit) 
                          ->getCount ();
            else
               unitReplication = 1;
            templateMask = new BitSet (unitReplication);
         ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 374 "machine_description.y"
    {
            int t_range = (yyvsp[(3) - (3)].int_value);
            if (t_range==0)
            {
               /* if range is not specified, this means all units of
                  the given class; set templateMask to all ones */
               *templateMask = ~(*templateMask);
            }
            (yyval.p_TEU) = new TemplateExecutionUnit (unit, templateMask, 0);
         ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 388 "machine_description.y"
    {
            (yyval.int_value) = 1;
         ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 392 "machine_description.y"
    {
            (yyval.int_value) = (yyvsp[(1) - (2)].int_value) + 1;
         ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 399 "machine_description.y"
    {
            (yyval.int_value) = 1;
         ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 403 "machine_description.y"
    {
            (yyval.int_value) = 1;
         ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 407 "machine_description.y"
    {
            (yyval.int_value) = 1;
         ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 411 "machine_description.y"
    {
            (yyval.int_value) = 1;
         ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 415 "machine_description.y"
    {
            (yyval.int_value) = 1;
         ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 419 "machine_description.y"
    {
            (yyval.int_value) = 1;
         ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 423 "machine_description.y"
    {
            (yyval.int_value) = 1;
         ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 430 "machine_description.y"
    {
            startPosition = currentPosition - yyleng;
            // check if this is the first MemoryHierarchy definition
            // we do not allow more than one
            if (memLevels != NULL)   // we have seen one already
            {
               // report error
               haveErrors += 1;
               fprintf(stderr, "Error %d (%d, %d): Multiple MemoryHierarchy declarations. Previous declaration at (%d, %d).\n",
                    haveErrors, startPosition.Line(), startPosition.Column(),
                    memLevels->getStartPosition().Line(), 
                    memLevels->getStartPosition().Column());
               delete (memLevels);
               memLevels = NULL;
            }
            (yyval.position) = startPosition;
         ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 448 "machine_description.y"
    {
            memLevels = (yyvsp[(4) - (5)].p_MLAT);
            memLevels->setStartPosition ((yyvsp[(2) - (5)].position));
            (yyval.int_value) = 1;
         ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 457 "machine_description.y"
    { 
            startPosition = currentPosition - yyleng;
         ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 460 "machine_description.y"
    { 
            (yyval.p_MLAT) = new MemLevelAssocTable();
            (yyval.p_MLAT)->addElement ((yyvsp[(1) - (3)].p_char), (yyvsp[(3) - (3)].p_MHL), startPosition);
         ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 465 "machine_description.y"
    { 
            startPosition = currentPosition - yyleng;
         ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 468 "machine_description.y"
    {
            (yyvsp[(1) - (5)].p_MLAT)->addElement ((yyvsp[(3) - (5)].p_char), (yyvsp[(5) - (5)].p_MHL), startPosition); 
            (yyval.p_MLAT) = (yyvsp[(1) - (5)].p_MLAT);
         ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 478 "machine_description.y"
    {
            (yyval.p_MHL) = new MemoryHierarchyLevel ((yyvsp[(2) - (15)].int_value), (yyvsp[(4) - (15)].int_value), (yyvsp[(6) - (15)].int_value), (yyvsp[(8) - (15)].int_value), (yyvsp[(10) - (15)].fl_value), NULL, (yyvsp[(12) - (15)].p_char), (yyvsp[(14) - (15)].int_value));
         ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 483 "machine_description.y"
    { (yyval.int_value) = (yyvsp[(1) - (1)].int_value); ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 484 "machine_description.y"
    { (yyval.int_value) = MHL_UNLIMITED; ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 488 "machine_description.y"
    { (yyval.int_value) = (yyvsp[(1) - (1)].int_value); ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 489 "machine_description.y"
    { (yyval.int_value) = MHL_UNLIMITED; ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 492 "machine_description.y"
    { (yyval.int_value) = (yyvsp[(1) - (1)].int_value); ;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 493 "machine_description.y"
    { (yyval.int_value) = MHL_UNDEFINED; ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 496 "machine_description.y"
    { (yyval.int_value) = (yyvsp[(1) - (1)].int_value); ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 497 "machine_description.y"
    { (yyval.int_value) = MHL_FULLY_ASSOCIATIVE; ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 500 "machine_description.y"
    { (yyval.fl_value) = (yyvsp[(1) - (1)].fl_value); ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 501 "machine_description.y"
    { (yyval.fl_value) = (float)(yyvsp[(1) - (1)].int_value); ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 502 "machine_description.y"
    { (yyval.fl_value) = MHL_UNDEFINED; ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 505 "machine_description.y"
    { (yyval.p_char) = (yyvsp[(1) - (1)].p_char); ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 506 "machine_description.y"
    { (yyval.p_char) = NULL; ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 509 "machine_description.y"
    { (yyval.int_value) = (yyvsp[(1) - (1)].int_value); ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 510 "machine_description.y"
    { (yyval.int_value) = 0; ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 515 "machine_description.y"
    {
            (yyval.position) = currentPosition;
         ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 519 "machine_description.y"
    {
            (yyval.position) = currentPosition;
         ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 524 "machine_description.y"
    {  
            replacementRules->push_back(new ReplacementRule((yyvsp[(3) - (8)].p_geninstlist), (yyvsp[(2) - (8)].position), (yyvsp[(6) - (8)].p_geninstlist), (yyvsp[(5) - (8)].position), (yyvsp[(7) - (8)].p_char)));
            (yyval.int_value) = 1;
         ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 531 "machine_description.y"
    {
            (yyval.p_geninstlist) = new GenInstList();
            (yyval.p_geninstlist)->push_back((yyvsp[(1) - (1)].p_geninst));
         ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 536 "machine_description.y"
    {
            (yyvsp[(1) - (3)].p_geninstlist)->push_back((yyvsp[(3) - (3)].p_geninst));
            (yyval.p_geninstlist) = (yyvsp[(1) - (3)].p_geninstlist);
         ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 545 "machine_description.y"
    {
            (yyvsp[(1) - (3)].p_geninstlist)->back()->setExclusiveOutputMatch();
            (yyvsp[(1) - (3)].p_geninstlist)->push_back((yyvsp[(3) - (3)].p_geninst));
            (yyval.p_geninstlist) = (yyvsp[(1) - (3)].p_geninstlist);
         ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 553 "machine_description.y"
    {
            (yyval.p_geninst) = new GenericInstruction((yyvsp[(1) - (4)].iclass), (yyvsp[(2) - (4)].p_reglist), (yyvsp[(4) - (4)].p_reglist));
         ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 557 "machine_description.y"
    {
            (yyval.p_geninst) = new GenericInstruction((yyvsp[(1) - (3)].iclass), (yyvsp[(2) - (3)].p_reglist), NULL);
         ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 561 "machine_description.y"
    {
            (yyval.p_geninst) = new GenericInstruction((yyvsp[(1) - (3)].iclass), NULL, (yyvsp[(3) - (3)].p_reglist));
         ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 565 "machine_description.y"
    {
            (yyval.p_geninst) = new GenericInstruction((yyvsp[(1) - (2)].iclass), NULL, NULL);
         ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 571 "machine_description.y"
    {
            startPosition = currentPosition - yyleng;
            (yyval.p_reglist) = new GenRegList();
            (yyval.p_reglist)->push_back(new GenericRegister((yyvsp[(1) - (1)].p_char), GP_REGISTER_TYPE, 
                       startPosition));
         ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 578 "machine_description.y"
    {
            startPosition = currentPosition - yyleng;
            (yyvsp[(1) - (3)].p_reglist)->push_back(new GenericRegister((yyvsp[(3) - (3)].p_char), GP_REGISTER_TYPE, 
                       startPosition));
            (yyval.p_reglist) = (yyvsp[(1) - (3)].p_reglist);
         ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 584 "machine_description.y"
    { (yyval.position) = currentPosition - yyleng;;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 586 "machine_description.y"
    {
            (yyval.p_reglist) = new GenRegList();
            (yyval.p_reglist)->push_back(new GenericRegister((yyvsp[(2) - (4)].p_char), MEMORY_TYPE, (yyvsp[(3) - (4)].position)));
         ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 591 "machine_description.y"
    { (yyval.position) = currentPosition - yyleng; ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 592 "machine_description.y"
    {
            (yyvsp[(1) - (6)].p_reglist)->push_back(new GenericRegister((yyvsp[(4) - (6)].p_char), MEMORY_TYPE, (yyvsp[(5) - (6)].position)));
            (yyval.p_reglist) = (yyvsp[(1) - (6)].p_reglist);
         ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 599 "machine_description.y"
    { (yyval.position) = currentPosition-yyleng; ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 602 "machine_description.y"
    {
            assert ((yyvsp[(4) - (9)].int_value) >= 0);
            bypassRules->push_back(new 
                        BypassRule((yyvsp[(6) - (9)].p_icset), (yyvsp[(7) - (9)].p_bitset), (yyvsp[(8) - (9)].p_icset), (yyvsp[(4) - (9)].int_value), (yyvsp[(2) - (9)].position)));
            (yyval.int_value) = 1;
         ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 612 "machine_description.y"
    {
            (yyval.p_icset) = new ICSet();
            (yyval.p_icset)->insert((yyvsp[(1) - (1)].iclass));
         ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 617 "machine_description.y"
    {
            (yyval.p_icset) = (yyvsp[(1) - (3)].p_icset);
            (yyval.p_icset)->insert((yyvsp[(3) - (3)].iclass));
         ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 622 "machine_description.y"
    {
            (yyval.p_icset) = new ICSet();
            (yyval.p_icset)->setAllValues();
         ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 629 "machine_description.y"
    { (yyval.p_bitset) = new BitSet(~BitSet(DTYPE_TOP_MARKER)); ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 630 "machine_description.y"
    { (yyval.p_bitset) = (yyvsp[(3) - (4)].p_bitset); ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 635 "machine_description.y"
    {
            assert ((yyvsp[(1) - (1)].int_value) < DTYPE_TOP_MARKER);
            (yyval.p_bitset) = new BitSet (DTYPE_TOP_MARKER);
            *((yyval.p_bitset)) += (yyvsp[(1) - (1)].int_value);
         ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 641 "machine_description.y"
    {
            assert ((yyvsp[(3) - (3)].int_value) < DTYPE_TOP_MARKER);
            *((yyvsp[(1) - (3)].p_bitset)) += (yyvsp[(3) - (3)].int_value);
            (yyval.p_bitset) = (yyvsp[(1) - (3)].p_bitset);
         ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 649 "machine_description.y"
    { (yyval.position) = currentPosition-yyleng; ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 651 "machine_description.y"
    {
            (yyval.p_instruction) = instTemplates->addDescription((yyvsp[(2) - (7)].iclass), (yyvsp[(3) - (7)].position), templateList);
         ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 654 "machine_description.y"
    { (yyval.position) = currentPosition-yyleng; ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 656 "machine_description.y"
    {
            (yyval.p_instruction) = instTemplates->addDescription((yyvsp[(2) - (10)].iclass), (yyvsp[(3) - (10)].position), templateList, (yyvsp[(5) - (10)].p_ucList));
         ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 670 "machine_description.y"
    {
            assert((yyvsp[(1) - (1)].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[(1) - (1)].int_value));
         ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 675 "machine_description.y"
    {
            assert((yyvsp[(1) - (3)].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[(1) - (3)].int_value), ExecUnit_SCALAR, 0, (MIAMI::ExecUnitType)(yyvsp[(3) - (3)].int_value));
         ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 680 "machine_description.y"
    {
            assert((yyvsp[(1) - (8)].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[(1) - (8)].int_value), ExecUnit_VECTOR, (yyvsp[(7) - (8)].int_value), (MIAMI::ExecUnitType)(yyvsp[(3) - (8)].int_value));
         ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 685 "machine_description.y"
    {
            assert((yyvsp[(1) - (8)].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[(1) - (8)].int_value), ExecUnit_VECTOR, (yyvsp[(5) - (8)].int_value), (MIAMI::ExecUnitType)(yyvsp[(8) - (8)].int_value));
         ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 690 "machine_description.y"
    {
            assert((yyvsp[(1) - (6)].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[(1) - (6)].int_value), ExecUnit_VECTOR, (yyvsp[(5) - (6)].int_value), ExecUnitType_INT);
         ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 695 "machine_description.y"
    {
            assert((yyvsp[(1) - (4)].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[(1) - (4)].int_value), ExecUnit_SCALAR, 0, ExecUnitType_INT, (width_t)(yyvsp[(3) - (4)].int_value));
         ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 700 "machine_description.y"
    {
            assert((yyvsp[(1) - (6)].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[(1) - (6)].int_value), ExecUnit_SCALAR, 0, (MIAMI::ExecUnitType)(yyvsp[(6) - (6)].int_value), (width_t)(yyvsp[(3) - (6)].int_value));
         ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 705 "machine_description.y"
    {
            assert((yyvsp[(1) - (11)].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[(1) - (11)].int_value), ExecUnit_VECTOR, (yyvsp[(10) - (11)].int_value), (MIAMI::ExecUnitType)(yyvsp[(6) - (11)].int_value), (width_t)(yyvsp[(3) - (11)].int_value));
         ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 710 "machine_description.y"
    {
            assert((yyvsp[(1) - (11)].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[(1) - (11)].int_value), ExecUnit_VECTOR, (yyvsp[(8) - (11)].int_value), (MIAMI::ExecUnitType)(yyvsp[(11) - (11)].int_value), (width_t)(yyvsp[(3) - (11)].int_value));
         ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 715 "machine_description.y"
    {
            assert((yyvsp[(1) - (9)].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[(1) - (9)].int_value), ExecUnit_VECTOR, (yyvsp[(8) - (9)].int_value), ExecUnitType_INT, (width_t)(yyvsp[(3) - (9)].int_value));
         ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 723 "machine_description.y"
    {
            /* start a new template list */
            templateList.clear();
            templateList.push_back((yyvsp[(1) - (1)].p_itemplate));
            (yyval.int_value) = 1;   // length 1
         ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 730 "machine_description.y"
    {
            templateList.push_back((yyvsp[(3) - (3)].p_itemplate));
            (yyval.int_value) = (yyvsp[(1) - (3)].int_value) + 1;
         ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 738 "machine_description.y"
    {
            (yyval.p_itemplate) = new InstTemplate();
            (yyval.p_itemplate)->addCycles((yyvsp[(1) - (1)].p_TEUList));
         ;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 743 "machine_description.y"
    {
            (yyvsp[(1) - (3)].p_itemplate)->addCycles((yyvsp[(3) - (3)].p_TEUList));
            (yyval.p_itemplate) = (yyvsp[(1) - (3)].p_itemplate);
         ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 748 "machine_description.y"
    {
            (yyval.p_itemplate) = new InstTemplate();
            (yyval.p_itemplate)->addCycles((yyvsp[(1) - (3)].p_TEUList), (yyvsp[(3) - (3)].int_value));
         ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 753 "machine_description.y"
    {
            (yyvsp[(1) - (5)].p_itemplate)->addCycles((yyvsp[(3) - (5)].p_TEUList), (yyvsp[(5) - (5)].int_value));
            (yyval.p_itemplate) = (yyvsp[(1) - (5)].p_itemplate);
         ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 761 "machine_description.y"
    {
            (yyval.p_ucList) = new UnitCountList();
            (yyval.p_ucList)->push_back((yyvsp[(1) - (1)].ucPair));
         ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 766 "machine_description.y"
    {
            (yyvsp[(1) - (3)].p_ucList)->push_back((yyvsp[(3) - (3)].ucPair));
            (yyval.p_ucList) = (yyvsp[(1) - (3)].p_ucList);
         ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 774 "machine_description.y"
    {
            startPosition = currentPosition - yyleng;
            // set in a global variable the number of units of this type
            unit = asyncUnits->getMappingForName ((yyvsp[(1) - (1)].p_char), startPosition);
            if (unit == NO_MAPPING)
            {
               haveErrors += 1;
               fprintf(stderr, "Error %d (%d, %d): The requested asyncronous resource \"%s\" has not been defined.\n",
                    haveErrors, startPosition.Line(), startPosition.Column(), (yyvsp[(1) - (1)].p_char));
            }
         ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 786 "machine_description.y"
    {
            int t_count = (yyvsp[(3) - (3)].int_value);
            /* in this context, if count was not specified, this means 
               only one unit is needed. */
            if (t_count==0)
               t_count = 1;
            (yyval.ucPair).Assign(unit, t_count);
         ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 798 "machine_description.y"
    {
            /* use a NULL set when Nothing is specified */
            (yyval.p_TEUList) = NULL;
         ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 803 "machine_description.y"
    {
            startPosition = currentPosition - yyleng;
            (yyval.p_TEUList) = new TEUList ();
            for (int i=0 ; i<numUnits ; ++i)
            {
               int _count = cpuUnits->getElementAtIndex (i) ->getCount ();
               BitSet *_umask = new BitSet (~BitSet(_count));
               (yyval.p_TEUList)->push_back (new TemplateExecutionUnit (i, _umask, 0));
            }
         ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 814 "machine_description.y"
    {
            (yyval.p_TEUList) = new TEUList ();
            (yyval.p_TEUList)->push_back ((yyvsp[(1) - (1)].p_TEU));
         ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 819 "machine_description.y"
    {
            if ((yyvsp[(1) - (3)].p_TEUList) == NULL)
            {
               haveErrors += 1;
               fprintf(stderr, "Error %d (%d, %d): You cannot use another unit if you specified 'NOTHING' for this cycle already.\n",
                    haveErrors, startPosition.Line(), startPosition.Column());
            } else
               (yyvsp[(1) - (3)].p_TEUList)->push_back ((yyvsp[(3) - (3)].p_TEU));
            (yyval.p_TEUList) = (yyvsp[(1) - (3)].p_TEUList);
         ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 833 "machine_description.y"
    {
            startPosition = currentPosition - yyleng;
            // set in a global variable the number of units of this type
            unit = cpuUnits->getMappingForName ((yyvsp[(1) - (1)].p_char), startPosition);
            if (unit != NO_MAPPING)
               unitReplication = cpuUnits->getElementAtIndex (unit) 
                         ->getCount ();
            else
               unitReplication = 1;
            templateMask = new BitSet (unitReplication);
         ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 845 "machine_description.y"
    {
            int t_range = (yyvsp[(3) - (4)].int_value);
            int t_count = (yyvsp[(4) - (4)].int_value);
            if (t_range==0)
            {
               /* if range is not specified, this means any COUNT units of
                  the given class; set templateMask to all ones */
               *templateMask = ~(*templateMask);
               /* in this context, if count was not specified, this means 
                  only one unit is needed. */
               if (t_count==0)
                  t_count = 1;
            }
            /* otherwise, if range is specified and count is not, it means
               match ALL units given in the range. If count is specified too, 
               it means match any COUNT units from the given range. 
             */
            /* I should test here if the requested count is less or equal 
               than the number of units in the specified range.
               To count the number of units in the range is not a cheap thing.
               I have to iterate through all elements of the BitSet and count
               them. But this is done just once at parsing time.
             */
            int unitsInRange = 0;
            BitSet::BitSetIterator bitit (*templateMask);
            while ((bool)bitit)
            {
               ++ unitsInRange;
               ++ bitit;
            }
            if (unitsInRange<t_count)
            {
               haveErrors += 1;
               fprintf(stderr, "Error %d (%d, %d): The requested number of units (%d) in instruction template, is greater than the number of units in the specified range (%d).\n",
                    haveErrors, startPosition.Line(), startPosition.Column(),
                    t_count, unitsInRange);
            }
            (yyval.p_TEU) = new TemplateExecutionUnit (unit, templateMask, t_count);
         ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 887 "machine_description.y"
    {
            (yyval.int_value) = 0;
         ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 891 "machine_description.y"
    {
            (yyval.int_value) = (yyvsp[(2) - (3)].int_value);
         ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 898 "machine_description.y"
    {
            (yyval.int_value) = 1;
         ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 902 "machine_description.y"
    {
            (yyval.int_value) = (yyvsp[(1) - (3)].int_value) + 1;
         ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 909 "machine_description.y"
    {
            *templateMask += (yyvsp[(1) - (1)].int_value);
            (yyval.int_value) = 1;
         ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 914 "machine_description.y"
    {
            int lLimit = (yyvsp[(1) - (3)].int_value);
            if (lLimit == -1) lLimit = 0;
            int uLimit = (yyvsp[(3) - (3)].int_value);
            if (uLimit == -1) uLimit = unitReplication-1;
            for (int i=lLimit ; i<=uLimit ; ++i)
               *templateMask += i;
            (yyval.int_value) = 2;
         ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 924 "machine_description.y"
    {  /* strided interval */
            int lLimit = (yyvsp[(1) - (5)].int_value);
            if (lLimit == -1) lLimit = 0;
            int uLimit = (yyvsp[(5) - (5)].int_value);
            if (uLimit == -1) uLimit = unitReplication-1;
            int stride = (yyvsp[(3) - (5)].int_value);
            if (stride == -1) stride = 1;
            for (int i=lLimit ; i<=uLimit ; i+=stride)
               *templateMask += i;
            (yyval.int_value) = 3;
         ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 938 "machine_description.y"
    {
            (yyval.int_value) = -1;
         ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 942 "machine_description.y"
    {
            (yyval.int_value) = (yyvsp[(1) - (1)].int_value);
         ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 948 "machine_description.y"
    {
            (yyval.int_value) = 0;
         ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 952 "machine_description.y"
    {
            (yyval.int_value) = (yyvsp[(2) - (3)].int_value);
         ;}
    break;



/* Line 1455 of yacc.c  */
#line 3058 "machdesc.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 957 "machine_description.y"



Machine* 
parseMachineDescription( const char *fileName )
{
    extern FILE *yyin;
    currentPosition.initPosition(fileName, 1, 1);
    startPosition = currentPosition;
    haveErrors = 0;
    cpuUnits = NULL;
    numUnits = 0;
    memLevels = NULL;
    
    /* try to open the file specified as argument */
    if( (yyin  = fopen (fileName, "rt")) == NULL)
    {
       fprintf (stderr, "Error when trying to open the machine description file '%s'\n", fileName );
       exit (-2);
    }
    
    instTemplates = new InstructionContainer[IB_TOP_MARKER];
    replacementRules = new ReplacementList();
    bypassRules = new BypassList();
    restrictRules = new RestrictionList();
                                   
    yyparse();
    if (haveErrors>0)
    {
       fprintf (stderr, "Found %d errors in the machine description file '%s'\n", 
              haveErrors, fileName );
       exit (-3);
    }
    return (currentMachine);
}

void yyerror( const char * s)
{
    haveErrors += 1;
    fprintf(stderr, "Error %d: File %s (%d,%d): %s is not allowed here.\n", 
       haveErrors, currentPosition.FileName(),
       currentPosition.Line(), currentPosition.Column()-yyleng, yytext );
}

