/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 1 "machine_description.y" /* yacc.c:339  */

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


#line 159 "machdesc.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "machdesc.tab.h".  */
#ifndef YY_YY_MACHDESC_TAB_H_INCLUDED
# define YY_YY_MACHDESC_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    T_OR = 258,
    T_TIMES = 259,
    T_PLUSPLUS = 260,
    T_PLUS = 261,
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

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 95 "machine_description.y" /* yacc.c:355  */

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

#line 277 "machdesc.tab.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_MACHDESC_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 294 "machdesc.tab.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  6
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   267

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  51
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  67
/* YYNRULES -- Number of rules.  */
#define YYNRULES  129
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  259

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   305

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
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
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
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

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_OR", "T_TIMES", "T_PLUSPLUS",
  "T_PLUS", "T_COLON", "T_SEMICOLON", "T_COMMA", "T_MACHINE", "T_VERSION",
  "T_ASSIGN", "T_REPLACE", "T_INSTRUCTION", "T_TEMPLATE", "T_CPU_UNITS",
  "T_WITH", "T_NOTHING", "T_ALL_UNITS", "T_LBRACKET", "T_RBRACKET",
  "T_LPAREN", "T_RPAREN", "T_ARROW", "T_MAXIMUM", "T_FROM", "T_LCPAREN",
  "T_RCPAREN", "T_ANY_INSTRUCTION", "T_BYPASS", "T_LATENCY", "T_FOR",
  "T_MEMORY", "T_CONTROL", "T_ADDR_REGISTER", "T_GP_REGISTER",
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
  "RangeIntervals", "OneInterval", "IntervalLimit", "UnitCount", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
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

#define YYPACT_NINF -190

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-190)))

#define YYTABLE_NINF -128

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      12,    60,    73,  -190,    79,    36,  -190,  -190,    75,    44,
    -190,    89,  -190,  -190,    48,    87,    57,    92,    54,  -190,
    -190,  -190,  -190,    82,    91,    38,    22,  -190,    56,    54,
    -190,    58,    64,  -190,  -190,    93,    95,  -190,  -190,  -190,
      38,  -190,  -190,  -190,  -190,  -190,    67,    61,  -190,  -190,
      84,    58,    49,    88,    85,    81,   101,    72,    74,  -190,
      90,    94,    22,  -190,    53,  -190,   -14,    19,    76,   100,
      99,     6,    83,    80,    96,   112,    97,   117,  -190,    58,
      58,  -190,    98,   -13,  -190,     7,   102,   115,   103,   114,
     104,  -190,  -190,    52,  -190,   105,  -190,    86,   106,   119,
    -190,   107,   108,  -190,  -190,    58,  -190,   121,     3,   -13,
     109,   116,   125,     2,  -190,     9,  -190,   113,   110,   126,
     -11,   118,  -190,   111,   128,  -190,  -190,  -190,    28,   120,
     122,  -190,   121,   129,   133,    27,  -190,  -190,  -190,    16,
     127,    77,  -190,   123,   104,   124,   131,  -190,  -190,  -190,
    -190,    11,  -190,    -1,  -190,  -190,  -190,   132,  -190,  -190,
     134,   135,   136,   144,   113,     2,  -190,     2,   137,   130,
     138,  -190,  -190,   149,   155,     8,  -190,   157,    58,   145,
     -11,  -190,  -190,   158,   118,  -190,   147,   139,   141,   140,
     143,   123,   127,    78,  -190,  -190,   148,     2,   131,  -190,
     146,  -190,   142,    63,     0,  -190,  -190,  -190,  -190,   159,
     156,  -190,   150,  -190,    66,  -190,  -190,   163,  -190,    59,
    -190,  -190,  -190,   165,   166,   151,  -190,  -190,   146,   152,
    -190,     1,   153,   161,  -190,  -190,  -190,  -190,   167,  -190,
    -190,     4,  -190,  -190,   176,    -2,  -190,  -190,  -190,   181,
      -3,  -190,  -190,   182,     5,  -190,  -190,   173,  -190
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
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
      25,     0,     0,    69,    68,     0,    76,    72,     0,    71,
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

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -190,  -190,  -190,  -190,  -190,  -190,  -190,  -190,  -190,  -190,
    -190,   168,  -190,  -190,   154,  -190,  -190,  -190,   -80,  -190,
      24,  -190,  -190,   160,  -190,  -190,  -190,  -190,  -190,   -40,
    -190,  -190,  -190,  -190,  -190,  -190,  -190,  -190,  -190,  -190,
     162,   -52,   -71,  -190,  -190,  -190,  -190,    15,  -190,  -190,
    -190,  -190,  -190,   -31,    10,    34,  -190,    62,  -190,    35,
      32,  -190,    39,  -190,   -32,  -189,    13
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

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      53,   251,   246,   181,   221,   236,    82,    82,   242,   255,
      83,   217,   107,   119,   178,   144,   108,   198,   150,   165,
     136,   137,     1,   130,   166,    91,    46,   103,   104,   199,
     145,   109,    52,    79,    80,   179,    84,    84,   132,   234,
     247,   182,   222,   237,   248,   252,   243,   256,   157,    47,
     138,    30,    31,   131,    92,    98,    67,    86,    79,    80,
      12,   118,   229,    32,    87,   162,   178,     5,    33,   165,
      81,   220,   163,     6,   227,    34,    68,    35,    36,    98,
     230,   168,   212,   169,   169,    10,    11,     7,     8,   152,
      27,    28,    63,    28,   122,   123,    14,    17,    19,    18,
      21,    52,    22,    29,    49,    57,    54,    58,   -91,    60,
      61,    71,    72,    73,    74,    89,    75,    76,    88,    90,
     100,   102,    77,    98,   111,    95,   113,   125,    96,   110,
     108,   112,   135,   146,   149,   126,   167,   120,   153,   173,
     185,   158,   148,   187,   205,   170,   101,   201,   106,   152,
     127,   133,   114,   190,   134,   124,   156,   160,    92,   155,
     161,   197,  -127,   189,   200,   202,   215,   204,   206,   208,
     228,   213,   159,   174,   231,   232,   241,   188,   138,   194,
     196,   210,   209,   225,   207,   245,   218,   224,   216,   240,
     250,   254,   226,   233,   258,   203,   235,    50,   239,   192,
      59,   195,   193,   191,   211,     0,   172,   214,     0,     0,
       0,     0,     0,     0,     0,     0,    78,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   128
};

static const yytype_int16 yycheck[] =
{
      31,     4,     4,     4,     4,     4,    20,    20,     4,     4,
      24,   200,    83,    93,     3,     6,     9,     9,    29,     3,
      18,    19,    10,    20,     8,    19,     4,    79,    80,    21,
      21,    24,    43,     5,     6,    24,    50,    50,   109,   228,
      42,    42,    42,    42,    46,    48,    42,    42,   128,    27,
      48,    13,    14,    50,    48,    27,     7,    38,     5,     6,
      16,     9,     3,    25,    45,    38,     3,     7,    30,     3,
      17,     8,    45,     0,     8,    37,    27,    39,    40,    27,
      21,     4,     4,     6,     6,    49,    11,     8,     9,   120,
       8,     9,     8,     9,     8,     9,     7,    49,    41,    12,
       8,    43,    48,    12,    48,    12,    42,    12,    20,    42,
      49,    26,    31,    12,    42,    15,    42,    27,    42,    20,
       8,     4,    28,    27,     9,    42,    12,     8,    48,    27,
       9,    28,     7,    20,     8,    28,     9,    32,    20,    15,
       8,    21,   118,     9,   184,    22,    49,   178,    50,   180,
      42,    42,    48,     9,    38,    49,    28,    28,    48,    48,
      27,    12,     7,    27,     7,    20,   198,     9,    21,    28,
       7,    23,    50,    42,     9,     9,     9,    42,    48,    42,
      42,    38,    42,    27,    45,     9,    44,    28,    42,    28,
       9,     9,    42,    42,    21,   180,    44,    29,    45,   165,
      40,   169,   167,   164,   191,    -1,   144,   197,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   105
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
     106,   110,   111,   109,     6,    21,    20,   113,    71,     8,
      29,    98,   104,    20,    80,    48,    28,    69,    21,    50,
      28,    27,    38,    45,   112,     3,     8,     9,     4,     6,
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

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
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


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
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
  int yytoken = 0;
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

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
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
      if (yytable_value_is_error (yyn))
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
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

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
#line 170 "machine_description.y" /* yacc.c:1646  */
    {
#if DEBUG_MACHINE_CONSTRUCTION
           DEBUG_MACHINE (3, 
              printf("Machine description parsing is complete.\n");
              (yyvsp[0].p_machine)->print(stdout);
           )
#endif
           currentMachine = (yyvsp[0].p_machine);
           // add_target($1);
        }
#line 1576 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 184 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_machine) = new Machine ((yyvsp[-2].p_char), (yyvsp[-1].p_char));
            (yyvsp[0].p_EUAT)->FinalizeMapping ();
            (yyval.p_machine)->addUnits ((yyvsp[0].p_EUAT)); cpuUnits = (yyvsp[0].p_EUAT);
            numUnits = cpuUnits->NumberOfElements ();
         }
#line 1587 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 191 "machine_description.y" /* yacc.c:1646  */
    {
            if ((yyvsp[0].p_EUAT))
            {
               (yyvsp[0].p_EUAT)->FinalizeMapping ();
               (yyvsp[-1].p_machine)->addAsyncUnits ((yyvsp[0].p_EUAT)); asyncUnits = (yyvsp[0].p_EUAT);
            } else
            {
               asyncUnits = 0;
            }
         }
#line 1602 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 202 "machine_description.y" /* yacc.c:1646  */
    {
#if DEBUG_MACHINE_CONSTRUCTION
            DEBUG_MACHINE (1, 
               fprintf(stdout, "Finished parsing the instruction descriptions. Saw %d rules.\n",
                      (yyvsp[0].int_value));
            )
#endif
            (yyvsp[-3].p_machine)->addInstructionDescriptions (instTemplates);
            instTemplates = NULL;
            (yyvsp[-3].p_machine)->addReplacementRules (replacementRules);
            replacementRules = NULL;
            (yyvsp[-3].p_machine)->addBypassRules (bypassRules);
            bypassRules = NULL;
            (yyvsp[-3].p_machine)->addUnitRestrictions (restrictRules);
            restrictRules = NULL;
            (yyvsp[-3].p_machine)->setMachineFeature (MachineFeature_WINDOW_SIZE, window_size);
            if (memLevels)
               memLevels->FinalizeMapping ();
            (yyvsp[-3].p_machine)->addMemoryLevels (memLevels);
            memLevels = NULL;
            (yyval.p_machine) = (yyvsp[-3].p_machine);
         }
#line 1629 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 227 "machine_description.y" /* yacc.c:1646  */
    { (yyval.p_char) = (yyvsp[0].p_char); }
#line 1635 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 230 "machine_description.y" /* yacc.c:1646  */
    { (yyval.p_char) = (yyvsp[-1].p_char); }
#line 1641 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 231 "machine_description.y" /* yacc.c:1646  */
    { (yyval.p_char) = NULL; }
#line 1647 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 236 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.position) = currentPosition - yyleng;
         }
#line 1655 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 240 "machine_description.y" /* yacc.c:1646  */
    { 
            (yyvsp[-1].p_EUAT)->setStartPosition ((yyvsp[-3].position));
            (yyval.p_EUAT) = (yyvsp[-1].p_EUAT); 
         }
#line 1664 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 247 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_EUAT) = 0;
         }
#line 1672 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 251 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.position) = currentPosition - yyleng;
         }
#line 1680 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 255 "machine_description.y" /* yacc.c:1646  */
    { 
            (yyvsp[-1].p_EUAT)->setStartPosition ((yyvsp[-3].position));
            (yyval.p_EUAT) = (yyvsp[-1].p_EUAT); 
         }
#line 1689 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 262 "machine_description.y" /* yacc.c:1646  */
    { 
            startPosition = currentPosition - yyleng;
         }
#line 1697 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 265 "machine_description.y" /* yacc.c:1646  */
    { 
            (yyval.p_EUAT) = new MIAMI::ExecUnitAssocTable();
            (yyval.p_EUAT)->addElement ((yyvsp[-2].p_char), (yyvsp[0].p_MEU), startPosition);
         }
#line 1706 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 270 "machine_description.y" /* yacc.c:1646  */
    { 
            startPosition = currentPosition - yyleng;
         }
#line 1714 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 273 "machine_description.y" /* yacc.c:1646  */
    {
            (yyvsp[-4].p_EUAT)->addElement ((yyvsp[-2].p_char), (yyvsp[0].p_MEU), startPosition); 
            (yyval.p_EUAT) = (yyvsp[-4].p_EUAT);
         }
#line 1723 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 280 "machine_description.y" /* yacc.c:1646  */
    {
            /* default constructor sets count to 1 */
            (yyval.p_MEU) = new MachineExecutionUnit (); 
         }
#line 1732 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 285 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_MEU) = new MachineExecutionUnit ((yyvsp[0].int_value), NULL, currentPosition); 
         }
#line 1740 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 289 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_MEU) = new MachineExecutionUnit (1, (yyvsp[-1].p_char), currentPosition); 
         }
#line 1748 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 293 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_MEU) = new MachineExecutionUnit ((yyvsp[-3].int_value), (yyvsp[-1].p_char), currentPosition); 
         }
#line 1756 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 297 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_MEU) = new MachineExecutionUnit ((yyvsp[0].int_value), (yyvsp[-3].p_char), currentPosition); 
         }
#line 1764 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 304 "machine_description.y" /* yacc.c:1646  */
    {
            restrictRules->push_back (new UnitRestriction((yyvsp[-2].p_TEUList), (yyvsp[-4].int_value), (yyvsp[-1].p_char)));
            (yyval.int_value) = 1;
         }
#line 1773 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 312 "machine_description.y" /* yacc.c:1646  */
    {
            restrictRules->push_back (new UnitRestriction(NULL, (yyvsp[-2].int_value), (yyvsp[-1].p_char)));
            (yyval.int_value) = 1;
         }
#line 1782 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 320 "machine_description.y" /* yacc.c:1646  */
    {
            window_size = (yyvsp[-1].int_value);
            (yyval.int_value) = 1;
         }
#line 1791 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 327 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_char) = NULL;
         }
#line 1799 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 331 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_char) = (yyvsp[-1].p_char);
         }
#line 1807 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 338 "machine_description.y" /* yacc.c:1646  */
    {
            startPosition = currentPosition - yyleng;
            (yyval.p_TEUList) = new TEUList ();
            for (int i=0 ; i<numUnits ; ++i)
            {
               int _count = cpuUnits->getElementAtIndex (i) ->getCount ();
               BitSet *_umask = new BitSet (~BitSet(_count));
               (yyval.p_TEUList)->push_back (new TemplateExecutionUnit (i, _umask, 0));
            }
         }
#line 1822 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 349 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_TEUList) = new TEUList ();
            (yyval.p_TEUList)->push_back ((yyvsp[0].p_TEU));
         }
#line 1831 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 354 "machine_description.y" /* yacc.c:1646  */
    {
            (yyvsp[-2].p_TEUList)->push_back ((yyvsp[0].p_TEU));
            (yyval.p_TEUList) = (yyvsp[-2].p_TEUList);
         }
#line 1840 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 362 "machine_description.y" /* yacc.c:1646  */
    {
            startPosition = currentPosition - yyleng;
            // set in a global variable the number of units of this type
            unit = cpuUnits->getMappingForName ((yyvsp[0].p_char), startPosition);
            if (unit != NO_MAPPING)
               unitReplication = cpuUnits->getElementAtIndex (unit) 
                          ->getCount ();
            else
               unitReplication = 1;
            templateMask = new BitSet (unitReplication);
         }
#line 1856 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 374 "machine_description.y" /* yacc.c:1646  */
    {
            int t_range = (yyvsp[0].int_value);
            if (t_range==0)
            {
               /* if range is not specified, this means all units of
                  the given class; set templateMask to all ones */
               *templateMask = ~(*templateMask);
            }
            (yyval.p_TEU) = new TemplateExecutionUnit (unit, templateMask, 0);
         }
#line 1871 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 388 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = 1;
         }
#line 1879 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 392 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = (yyvsp[-1].int_value) + 1;
         }
#line 1887 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 399 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = 1;
         }
#line 1895 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 403 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = 1;
         }
#line 1903 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 407 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = 1;
         }
#line 1911 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 411 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = 1;
         }
#line 1919 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 415 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = 1;
         }
#line 1927 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 419 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = 1;
         }
#line 1935 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 423 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = 1;
         }
#line 1943 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 430 "machine_description.y" /* yacc.c:1646  */
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
         }
#line 1965 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 448 "machine_description.y" /* yacc.c:1646  */
    {
            memLevels = (yyvsp[-1].p_MLAT);
            memLevels->setStartPosition ((yyvsp[-3].position));
            (yyval.int_value) = 1;
         }
#line 1975 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 457 "machine_description.y" /* yacc.c:1646  */
    { 
            startPosition = currentPosition - yyleng;
         }
#line 1983 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 460 "machine_description.y" /* yacc.c:1646  */
    { 
            (yyval.p_MLAT) = new MemLevelAssocTable();
            (yyval.p_MLAT)->addElement ((yyvsp[-2].p_char), (yyvsp[0].p_MHL), startPosition);
         }
#line 1992 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 465 "machine_description.y" /* yacc.c:1646  */
    { 
            startPosition = currentPosition - yyleng;
         }
#line 2000 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 468 "machine_description.y" /* yacc.c:1646  */
    {
            (yyvsp[-4].p_MLAT)->addElement ((yyvsp[-2].p_char), (yyvsp[0].p_MHL), startPosition); 
            (yyval.p_MLAT) = (yyvsp[-4].p_MLAT);
         }
#line 2009 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 478 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_MHL) = new MemoryHierarchyLevel ((yyvsp[-13].int_value), (yyvsp[-11].int_value), (yyvsp[-9].int_value), (yyvsp[-7].int_value), (yyvsp[-5].fl_value), NULL, (yyvsp[-3].p_char), (yyvsp[-1].int_value));
         }
#line 2017 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 483 "machine_description.y" /* yacc.c:1646  */
    { (yyval.int_value) = (yyvsp[0].int_value); }
#line 2023 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 484 "machine_description.y" /* yacc.c:1646  */
    { (yyval.int_value) = MHL_UNLIMITED; }
#line 2029 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 488 "machine_description.y" /* yacc.c:1646  */
    { (yyval.int_value) = (yyvsp[0].int_value); }
#line 2035 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 489 "machine_description.y" /* yacc.c:1646  */
    { (yyval.int_value) = MHL_UNLIMITED; }
#line 2041 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 492 "machine_description.y" /* yacc.c:1646  */
    { (yyval.int_value) = (yyvsp[0].int_value); }
#line 2047 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 493 "machine_description.y" /* yacc.c:1646  */
    { (yyval.int_value) = MHL_UNDEFINED; }
#line 2053 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 496 "machine_description.y" /* yacc.c:1646  */
    { (yyval.int_value) = (yyvsp[0].int_value); }
#line 2059 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 497 "machine_description.y" /* yacc.c:1646  */
    { (yyval.int_value) = MHL_FULLY_ASSOCIATIVE; }
#line 2065 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 500 "machine_description.y" /* yacc.c:1646  */
    { (yyval.fl_value) = (yyvsp[0].fl_value); }
#line 2071 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 501 "machine_description.y" /* yacc.c:1646  */
    { (yyval.fl_value) = (float)(yyvsp[0].int_value); }
#line 2077 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 502 "machine_description.y" /* yacc.c:1646  */
    { (yyval.fl_value) = MHL_UNDEFINED; }
#line 2083 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 505 "machine_description.y" /* yacc.c:1646  */
    { (yyval.p_char) = (yyvsp[0].p_char); }
#line 2089 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 506 "machine_description.y" /* yacc.c:1646  */
    { (yyval.p_char) = NULL; }
#line 2095 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 509 "machine_description.y" /* yacc.c:1646  */
    { (yyval.int_value) = (yyvsp[0].int_value); }
#line 2101 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 510 "machine_description.y" /* yacc.c:1646  */
    { (yyval.int_value) = 0; }
#line 2107 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 515 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.position) = currentPosition;
         }
#line 2115 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 519 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.position) = currentPosition;
         }
#line 2123 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 524 "machine_description.y" /* yacc.c:1646  */
    {  
            replacementRules->push_back(new ReplacementRule((yyvsp[-5].p_geninstlist), (yyvsp[-6].position), (yyvsp[-2].p_geninstlist), (yyvsp[-3].position), (yyvsp[-1].p_char)));
            (yyval.int_value) = 1;
         }
#line 2132 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 531 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_geninstlist) = new GenInstList();
            (yyval.p_geninstlist)->push_back((yyvsp[0].p_geninst));
         }
#line 2141 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 536 "machine_description.y" /* yacc.c:1646  */
    {
            (yyvsp[-2].p_geninstlist)->push_back((yyvsp[0].p_geninst));
            (yyval.p_geninstlist) = (yyvsp[-2].p_geninstlist);
         }
#line 2150 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 545 "machine_description.y" /* yacc.c:1646  */
    {
            (yyvsp[-2].p_geninstlist)->back()->setExclusiveOutputMatch();
            (yyvsp[-2].p_geninstlist)->push_back((yyvsp[0].p_geninst));
            (yyval.p_geninstlist) = (yyvsp[-2].p_geninstlist);
         }
#line 2160 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 553 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_geninst) = new GenericInstruction((yyvsp[-3].iclass), (yyvsp[-2].p_reglist), (yyvsp[0].p_reglist));
         }
#line 2168 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 557 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_geninst) = new GenericInstruction((yyvsp[-2].iclass), (yyvsp[-1].p_reglist), NULL);
         }
#line 2176 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 561 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_geninst) = new GenericInstruction((yyvsp[-2].iclass), NULL, (yyvsp[0].p_reglist));
         }
#line 2184 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 565 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_geninst) = new GenericInstruction((yyvsp[-1].iclass), NULL, NULL);
         }
#line 2192 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 571 "machine_description.y" /* yacc.c:1646  */
    {
            startPosition = currentPosition - yyleng;
            (yyval.p_reglist) = new GenRegList();
            (yyval.p_reglist)->push_back(new GenericRegister((yyvsp[0].p_char), GP_REGISTER_TYPE, 
                       startPosition));
         }
#line 2203 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 578 "machine_description.y" /* yacc.c:1646  */
    {
            startPosition = currentPosition - yyleng;
            (yyvsp[-2].p_reglist)->push_back(new GenericRegister((yyvsp[0].p_char), GP_REGISTER_TYPE, 
                       startPosition));
            (yyval.p_reglist) = (yyvsp[-2].p_reglist);
         }
#line 2214 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 584 "machine_description.y" /* yacc.c:1646  */
    { (yyval.position) = currentPosition - yyleng;}
#line 2220 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 586 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_reglist) = new GenRegList();
            (yyval.p_reglist)->push_back(new GenericRegister((yyvsp[-2].p_char), MEMORY_TYPE, (yyvsp[-1].position)));
         }
#line 2229 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 591 "machine_description.y" /* yacc.c:1646  */
    { (yyval.position) = currentPosition - yyleng; }
#line 2235 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 592 "machine_description.y" /* yacc.c:1646  */
    {
            (yyvsp[-5].p_reglist)->push_back(new GenericRegister((yyvsp[-2].p_char), MEMORY_TYPE, (yyvsp[-1].position)));
            (yyval.p_reglist) = (yyvsp[-5].p_reglist);
         }
#line 2244 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 599 "machine_description.y" /* yacc.c:1646  */
    { (yyval.position) = currentPosition-yyleng; }
#line 2250 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 602 "machine_description.y" /* yacc.c:1646  */
    {
            assert ((yyvsp[-5].int_value) >= 0);
            bypassRules->push_back(new 
                        BypassRule((yyvsp[-3].p_icset), (yyvsp[-2].p_bitset), (yyvsp[-1].p_icset), (yyvsp[-5].int_value), (yyvsp[-7].position)));
            (yyval.int_value) = 1;
         }
#line 2261 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 612 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_icset) = new ICSet();
            (yyval.p_icset)->insert((yyvsp[0].iclass));
         }
#line 2270 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 617 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_icset) = (yyvsp[-2].p_icset);
            (yyval.p_icset)->insert((yyvsp[0].iclass));
         }
#line 2279 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 622 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_icset) = new ICSet();
            (yyval.p_icset)->setAllValues();
         }
#line 2288 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 629 "machine_description.y" /* yacc.c:1646  */
    { (yyval.p_bitset) = new BitSet(~BitSet(DTYPE_TOP_MARKER)); }
#line 2294 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 630 "machine_description.y" /* yacc.c:1646  */
    { (yyval.p_bitset) = (yyvsp[-1].p_bitset); }
#line 2300 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 635 "machine_description.y" /* yacc.c:1646  */
    {
            assert ((yyvsp[0].int_value) < DTYPE_TOP_MARKER);
            (yyval.p_bitset) = new BitSet (DTYPE_TOP_MARKER);
            *((yyval.p_bitset)) += (yyvsp[0].int_value);
         }
#line 2310 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 641 "machine_description.y" /* yacc.c:1646  */
    {
            assert ((yyvsp[0].int_value) < DTYPE_TOP_MARKER);
            *((yyvsp[-2].p_bitset)) += (yyvsp[0].int_value);
            (yyval.p_bitset) = (yyvsp[-2].p_bitset);
         }
#line 2320 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 649 "machine_description.y" /* yacc.c:1646  */
    { (yyval.position) = currentPosition-yyleng; }
#line 2326 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 651 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_instruction) = instTemplates->addDescription((yyvsp[-5].iclass), (yyvsp[-4].position), templateList);
         }
#line 2334 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 654 "machine_description.y" /* yacc.c:1646  */
    { (yyval.position) = currentPosition-yyleng; }
#line 2340 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 656 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_instruction) = instTemplates->addDescription((yyvsp[-8].iclass), (yyvsp[-7].position), templateList, (yyvsp[-5].p_ucList));
         }
#line 2348 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 670 "machine_description.y" /* yacc.c:1646  */
    {
            assert((yyvsp[0].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[0].int_value));
         }
#line 2357 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 675 "machine_description.y" /* yacc.c:1646  */
    {
            assert((yyvsp[-2].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[-2].int_value), ExecUnit_SCALAR, 0, (MIAMI::ExecUnitType)(yyvsp[0].int_value));
         }
#line 2366 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 95:
#line 680 "machine_description.y" /* yacc.c:1646  */
    {
            assert((yyvsp[-7].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[-7].int_value), ExecUnit_VECTOR, (yyvsp[-1].int_value), (MIAMI::ExecUnitType)(yyvsp[-5].int_value));
         }
#line 2375 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 96:
#line 685 "machine_description.y" /* yacc.c:1646  */
    {
            assert((yyvsp[-7].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[-7].int_value), ExecUnit_VECTOR, (yyvsp[-3].int_value), (MIAMI::ExecUnitType)(yyvsp[0].int_value));
         }
#line 2384 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 690 "machine_description.y" /* yacc.c:1646  */
    {
            assert((yyvsp[-5].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[-5].int_value), ExecUnit_VECTOR, (yyvsp[-1].int_value), ExecUnitType_INT);
         }
#line 2393 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 695 "machine_description.y" /* yacc.c:1646  */
    {
            assert((yyvsp[-3].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[-3].int_value), ExecUnit_SCALAR, 0, ExecUnitType_INT, (width_t)(yyvsp[-1].int_value));
         }
#line 2402 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 700 "machine_description.y" /* yacc.c:1646  */
    {
            assert((yyvsp[-5].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[-5].int_value), ExecUnit_SCALAR, 0, (MIAMI::ExecUnitType)(yyvsp[0].int_value), (width_t)(yyvsp[-3].int_value));
         }
#line 2411 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 705 "machine_description.y" /* yacc.c:1646  */
    {
            assert((yyvsp[-10].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[-10].int_value), ExecUnit_VECTOR, (yyvsp[-1].int_value), (MIAMI::ExecUnitType)(yyvsp[-5].int_value), (width_t)(yyvsp[-8].int_value));
         }
#line 2420 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 710 "machine_description.y" /* yacc.c:1646  */
    {
            assert((yyvsp[-10].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[-10].int_value), ExecUnit_VECTOR, (yyvsp[-3].int_value), (MIAMI::ExecUnitType)(yyvsp[0].int_value), (width_t)(yyvsp[-8].int_value));
         }
#line 2429 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 715 "machine_description.y" /* yacc.c:1646  */
    {
            assert((yyvsp[-8].int_value) < IB_TOP_MARKER);
            (yyval.iclass).Initialize((MIAMI::InstrBin)(yyvsp[-8].int_value), ExecUnit_VECTOR, (yyvsp[-1].int_value), ExecUnitType_INT, (width_t)(yyvsp[-6].int_value));
         }
#line 2438 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 723 "machine_description.y" /* yacc.c:1646  */
    {
            /* start a new template list */
            templateList.clear();
            templateList.push_back((yyvsp[0].p_itemplate));
            (yyval.int_value) = 1;   // length 1
         }
#line 2449 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 730 "machine_description.y" /* yacc.c:1646  */
    {
            templateList.push_back((yyvsp[0].p_itemplate));
            (yyval.int_value) = (yyvsp[-2].int_value) + 1;
         }
#line 2458 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 738 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_itemplate) = new InstTemplate();
            (yyval.p_itemplate)->addCycles((yyvsp[0].p_TEUList));
         }
#line 2467 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 743 "machine_description.y" /* yacc.c:1646  */
    {
            (yyvsp[-2].p_itemplate)->addCycles((yyvsp[0].p_TEUList));
            (yyval.p_itemplate) = (yyvsp[-2].p_itemplate);
         }
#line 2476 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 748 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_itemplate) = new InstTemplate();
            (yyval.p_itemplate)->addCycles((yyvsp[-2].p_TEUList), (yyvsp[0].int_value));
         }
#line 2485 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 753 "machine_description.y" /* yacc.c:1646  */
    {
            (yyvsp[-4].p_itemplate)->addCycles((yyvsp[-2].p_TEUList), (yyvsp[0].int_value));
            (yyval.p_itemplate) = (yyvsp[-4].p_itemplate);
         }
#line 2494 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 761 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_ucList) = new UnitCountList();
            (yyval.p_ucList)->push_back((yyvsp[0].ucPair));
         }
#line 2503 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 766 "machine_description.y" /* yacc.c:1646  */
    {
            (yyvsp[-2].p_ucList)->push_back((yyvsp[0].ucPair));
            (yyval.p_ucList) = (yyvsp[-2].p_ucList);
         }
#line 2512 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 774 "machine_description.y" /* yacc.c:1646  */
    {
            startPosition = currentPosition - yyleng;
            // set in a global variable the number of units of this type
            unit = asyncUnits->getMappingForName ((yyvsp[0].p_char), startPosition);
            if (unit == NO_MAPPING)
            {
               haveErrors += 1;
               fprintf(stderr, "Error %d (%d, %d): The requested asyncronous resource \"%s\" has not been defined.\n",
                    haveErrors, startPosition.Line(), startPosition.Column(), (yyvsp[0].p_char));
            }
         }
#line 2528 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 786 "machine_description.y" /* yacc.c:1646  */
    {
            int t_count = (yyvsp[0].int_value);
            /* in this context, if count was not specified, this means 
               only one unit is needed. */
            if (t_count==0)
               t_count = 1;
            (yyval.ucPair).Assign(unit, t_count);
         }
#line 2541 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 798 "machine_description.y" /* yacc.c:1646  */
    {
            /* use a NULL set when Nothing is specified */
            (yyval.p_TEUList) = NULL;
         }
#line 2550 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 803 "machine_description.y" /* yacc.c:1646  */
    {
            startPosition = currentPosition - yyleng;
            (yyval.p_TEUList) = new TEUList ();
            for (int i=0 ; i<numUnits ; ++i)
            {
               int _count = cpuUnits->getElementAtIndex (i) ->getCount ();
               BitSet *_umask = new BitSet (~BitSet(_count));
               (yyval.p_TEUList)->push_back (new TemplateExecutionUnit (i, _umask, 0));
            }
         }
#line 2565 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 814 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.p_TEUList) = new TEUList ();
            (yyval.p_TEUList)->push_back ((yyvsp[0].p_TEU));
         }
#line 2574 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 819 "machine_description.y" /* yacc.c:1646  */
    {
            if ((yyvsp[-2].p_TEUList) == NULL)
            {
               haveErrors += 1;
               fprintf(stderr, "Error %d (%d, %d): You cannot use another unit if you specified 'NOTHING' for this cycle already.\n",
                    haveErrors, startPosition.Line(), startPosition.Column());
            } else
               (yyvsp[-2].p_TEUList)->push_back ((yyvsp[0].p_TEU));
            (yyval.p_TEUList) = (yyvsp[-2].p_TEUList);
         }
#line 2589 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 833 "machine_description.y" /* yacc.c:1646  */
    {
            startPosition = currentPosition - yyleng;
            // set in a global variable the number of units of this type
            unit = cpuUnits->getMappingForName ((yyvsp[0].p_char), startPosition);
            if (unit != NO_MAPPING)
               unitReplication = cpuUnits->getElementAtIndex (unit) 
                         ->getCount ();
            else
               unitReplication = 1;
            templateMask = new BitSet (unitReplication);
         }
#line 2605 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 845 "machine_description.y" /* yacc.c:1646  */
    {
            int t_range = (yyvsp[-1].int_value);
            int t_count = (yyvsp[0].int_value);
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
         }
#line 2649 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 887 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = 0;
         }
#line 2657 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 120:
#line 891 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = (yyvsp[-1].int_value);
         }
#line 2665 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 121:
#line 898 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = 1;
         }
#line 2673 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 902 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = (yyvsp[-2].int_value) + 1;
         }
#line 2681 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 909 "machine_description.y" /* yacc.c:1646  */
    {
            *templateMask += (yyvsp[0].int_value);
            (yyval.int_value) = 1;
         }
#line 2690 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 124:
#line 914 "machine_description.y" /* yacc.c:1646  */
    {
            int lLimit = (yyvsp[-2].int_value);
            if (lLimit == -1) lLimit = 0;
            int uLimit = (yyvsp[0].int_value);
            if (uLimit == -1) uLimit = unitReplication-1;
            for (int i=lLimit ; i<=uLimit ; ++i)
               *templateMask += i;
            (yyval.int_value) = 2;
         }
#line 2704 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 125:
#line 924 "machine_description.y" /* yacc.c:1646  */
    {  /* strided interval */
            int lLimit = (yyvsp[-4].int_value);
            if (lLimit == -1) lLimit = 0;
            int uLimit = (yyvsp[0].int_value);
            if (uLimit == -1) uLimit = unitReplication-1;
            int stride = (yyvsp[-2].int_value);
            if (stride == -1) stride = 1;
            for (int i=lLimit ; i<=uLimit ; i+=stride)
               *templateMask += i;
            (yyval.int_value) = 3;
         }
#line 2720 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 126:
#line 938 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = -1;
         }
#line 2728 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 127:
#line 942 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = (yyvsp[0].int_value);
         }
#line 2736 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 128:
#line 948 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = 0;
         }
#line 2744 "machdesc.tab.c" /* yacc.c:1646  */
    break;

  case 129:
#line 952 "machine_description.y" /* yacc.c:1646  */
    {
            (yyval.int_value) = (yyvsp[-1].int_value);
         }
#line 2752 "machdesc.tab.c" /* yacc.c:1646  */
    break;


#line 2756 "machdesc.tab.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
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
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
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
  return yyresult;
}
#line 957 "machine_description.y" /* yacc.c:1906  */



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
