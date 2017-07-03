
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
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

/* Line 1676 of yacc.c  */
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



/* Line 1676 of yacc.c  */
#line 131 "machdesc.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


