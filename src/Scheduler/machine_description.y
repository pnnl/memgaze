%{
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

%}

%union
{
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
}



%left T_OR
%left T_TIMES
%left T_PLUSPLUS T_PLUS


%token T_COLON T_SEMICOLON T_COMMA T_MACHINE T_VERSION T_ASSIGN T_REPLACE
%token T_INSTRUCTION T_TEMPLATE T_CPU_UNITS T_WITH T_NOTHING T_ALL_UNITS
%token T_LBRACKET T_RBRACKET T_LPAREN T_RPAREN T_ARROW T_MAXIMUM T_FROM
%token T_LCPAREN T_RCPAREN T_ANY_INSTRUCTION T_BYPASS T_LATENCY T_FOR
%token T_MEMORY T_CONTROL T_ADDR_REGISTER T_GP_REGISTER T_MEMORY_HIERARCHY
%token T_VECTOR_UNIT T_RETIRE T_WINDOW_SIZE T_ASYNC_RESOURCES

%token <int_value> T_INT_CONST T_INST_TYPE T_DEPENDENCY_TYPE T_UNIT_TYPE
%token <fl_value> T_FLOAT_CONST
%token <ch_value> T_CHAR_CONST
%token <p_char> T_IDENTIFIER T_STR_CONST T_GP_REG_NAME

%type <int_value> InstTemplates InstructionDescriptions InstructionReplacement
%type <int_value> InstructionBypass UnitsRestriction MemoryHierarchyDef
%type <int_value> IntervalLimit UnitCount OneInterval UnitRange RangeIntervals
%type <int_value> MemNumBlocks MemBlockSize MemEntrySize MemAssocLevel
%type <int_value> MemPenalty RetireRestriction WindowSize MachineRule
%type <fl_value> MemBandwidth

%type <p_char> TargetName TargetVersion RuleName MemNextLevel
%type <p_machine> MachineDescription
%type <p_bitset> ListDependencyTypes DependencyTypes
%type <p_icset> ListInstructionTypes
%type <p_itemplate> InstTemplate
%type <p_ucList> AsyncUsage
%type <ucPair> AsyncClass
%type <p_instruction> InstructionExecution 
%type <iclass> InstType
%type <p_EUAT> ExecUnits AsyncResources CpuUnitList
%type <p_MEU> UnitInformation
%type <p_MLAT> MemoryLevelsList
%type <p_MHL> MemLevelInformation
%type <p_TEU> UnitClass RestrictionUnit
%type <p_TEUList> CycleTemplate UnitsList
%type <p_reglist> ListRegisters
%type <p_geninst> GenericInstruction
%type <p_geninstlist> ListGenericInstructions

%%

start : MachineDescription
        {
#if DEBUG_MACHINE_CONSTRUCTION
           DEBUG_MACHINE (3, 
              printf("Machine description parsing is complete.\n");
              $1->print(stdout);
           )
#endif
           currentMachine = $1;
           // add_target($1);
        }
      ;

MachineDescription : 
         TargetName TargetVersion ExecUnits 
         {
            $<p_machine>$ = new Machine ($1, $2);
            $3->FinalizeMapping ();
            $<p_machine>$->addUnits ($3); cpuUnits = $3;
            numUnits = cpuUnits->NumberOfElements ();
         }
         AsyncResources
         {
            if ($5)
            {
               $5->FinalizeMapping ();
               $<p_machine>4->addAsyncUnits ($5); asyncUnits = $5;
            } else
            {
               asyncUnits = 0;
            }
         }
         InstructionDescriptions
         {
#if DEBUG_MACHINE_CONSTRUCTION
            DEBUG_MACHINE (1, 
               fprintf(stdout, "Finished parsing the instruction descriptions. Saw %d rules.\n",
                      $7);
            )
#endif
            $<p_machine>4->addInstructionDescriptions (instTemplates);
            instTemplates = NULL;
            $<p_machine>4->addReplacementRules (replacementRules);
            replacementRules = NULL;
            $<p_machine>4->addBypassRules (bypassRules);
            bypassRules = NULL;
            $<p_machine>4->addUnitRestrictions (restrictRules);
            restrictRules = NULL;
            $<p_machine>4->setMachineFeature (MachineFeature_WINDOW_SIZE, window_size);
            if (memLevels)
               memLevels->FinalizeMapping ();
            $<p_machine>4->addMemoryLevels (memLevels);
            memLevels = NULL;
            $$ = $<p_machine>4;
         } 
       ;

TargetName : 
         T_MACHINE T_COLON T_STR_CONST { $$ = $3; } ;

TargetVersion : 
         T_COMMA T_VERSION T_COLON T_STR_CONST T_SEMICOLON { $$ = $4; }
       | T_SEMICOLON  { $$ = NULL; }
       ;


ExecUnits: T_CPU_UNITS
         {
            $<position>$ = currentPosition - yyleng;
         }
         T_ASSIGN CpuUnitList T_SEMICOLON 
         { 
            $4->setStartPosition ($<position>2);
            $$ = $4; 
         } 
       ;

AsyncResources: /* empty */
         {
            $$ = 0;
         }
       | T_ASYNC_RESOURCES
         {
            $<position>$ = currentPosition - yyleng;
         }
         T_ASSIGN CpuUnitList T_SEMICOLON 
         { 
            $4->setStartPosition ($<position>2);
            $$ = $4; 
         } 
       ;

CpuUnitList: T_IDENTIFIER 
         { 
            startPosition = currentPosition - yyleng;
         } UnitInformation
         { 
            $$ = new MIAMI::ExecUnitAssocTable();
            $$->addElement ($1, $3, startPosition);
         }
       | CpuUnitList T_COMMA T_IDENTIFIER 
         { 
            startPosition = currentPosition - yyleng;
         } UnitInformation
         {
            $1->addElement ($3, $5, startPosition); 
            $$ = $1;
         }
       ;

UnitInformation: /* empty */
         {
            /* default constructor sets count to 1 */
            $$ = new MachineExecutionUnit (); 
         }
       | T_TIMES T_INT_CONST
         {
            $$ = new MachineExecutionUnit ($2, NULL, currentPosition); 
         }
       | T_LCPAREN T_STR_CONST T_RCPAREN
         {
            $$ = new MachineExecutionUnit (1, $2, currentPosition); 
         }
       | T_TIMES T_INT_CONST T_LCPAREN T_STR_CONST T_RCPAREN
         {
            $$ = new MachineExecutionUnit ($2, $4, currentPosition); 
         }
       | T_LCPAREN T_STR_CONST T_RCPAREN T_TIMES T_INT_CONST
         {
            $$ = new MachineExecutionUnit ($5, $2, currentPosition); 
         }
       ;

UnitsRestriction:
         T_MAXIMUM T_INT_CONST T_FROM UnitsList RuleName T_SEMICOLON
         {
            restrictRules->push_back (new UnitRestriction($4, $2, $5));
            $$ = 1;
         }
       ;

RetireRestriction:
         T_RETIRE T_ASSIGN T_INT_CONST RuleName T_SEMICOLON
         {
            restrictRules->push_back (new UnitRestriction(NULL, $3, $4));
            $$ = 1;
         }
       ;

WindowSize:
         T_WINDOW_SIZE T_ASSIGN T_INT_CONST T_SEMICOLON
         {
            window_size = $3;
            $$ = 1;
         }
       ;

RuleName: /* empty */
         {
            $$ = NULL;
         }
       | T_LCPAREN T_STR_CONST T_RCPAREN
         {
            $$ = $2;
         }
       ;
       
UnitsList:
         T_ALL_UNITS
         {
            startPosition = currentPosition - yyleng;
            $$ = new TEUList ();
            for (int i=0 ; i<numUnits ; ++i)
            {
               int _count = cpuUnits->getElementAtIndex (i) ->getCount ();
               BitSet *_umask = new BitSet (~BitSet(_count));
               $$->push_back (new TemplateExecutionUnit (i, _umask, 0));
            }
         }
       | RestrictionUnit
         {
            $$ = new TEUList ();
            $$->push_back ($1);
         }
       | UnitsList T_COMMA RestrictionUnit
         {
            $1->push_back ($3);
            $$ = $1;
         }
       ;

RestrictionUnit:
         T_IDENTIFIER
         {
            startPosition = currentPosition - yyleng;
            // set in a global variable the number of units of this type
            unit = cpuUnits->getMappingForName ($1, startPosition);
            if (unit != NO_MAPPING)
               unitReplication = cpuUnits->getElementAtIndex (unit) 
                          ->getCount ();
            else
               unitReplication = 1;
            templateMask = new BitSet (unitReplication);
         }
         UnitRange
         {
            int t_range = $3;
            if (t_range==0)
            {
               /* if range is not specified, this means all units of
                  the given class; set templateMask to all ones */
               *templateMask = ~(*templateMask);
            }
            $$ = new TemplateExecutionUnit (unit, templateMask, 0);
         }
       ;

InstructionDescriptions:
         MachineRule
         {
            $$ = 1;
         }
       | InstructionDescriptions MachineRule
         {
            $$ = $1 + 1;
         }
       ;

MachineRule:
         InstructionReplacement
         {
            $$ = 1;
         }
       | InstructionExecution
         {
            $$ = 1;
         }
       | InstructionBypass
         {
            $$ = 1;
         }
       | UnitsRestriction
         {
            $$ = 1;
         }
       | RetireRestriction
         {
            $$ = 1;
         }
       | WindowSize
         {
            $$ = 1;
         }
       | MemoryHierarchyDef
         {
            $$ = 1;
         }
       ;

MemoryHierarchyDef:
         T_MEMORY_HIERARCHY 
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
            $<position>$ = startPosition;
         }
         T_ASSIGN MemoryLevelsList T_SEMICOLON
         {
            memLevels = $4;
            memLevels->setStartPosition ($<position>2);
            $$ = 1;
         }
       ;

MemoryLevelsList:
         T_IDENTIFIER 
         { 
            startPosition = currentPosition - yyleng;
         } MemLevelInformation
         { 
            $$ = new MemLevelAssocTable();
            $$->addElement ($1, $3, startPosition);
         }
       | MemoryLevelsList T_COMMA T_IDENTIFIER 
         { 
            startPosition = currentPosition - yyleng;
         } MemLevelInformation
         {
            $1->addElement ($3, $5, startPosition); 
            $$ = $1;
         }
       ;

MemLevelInformation:
         T_LBRACKET MemNumBlocks T_COMMA MemBlockSize T_COMMA
         MemEntrySize T_COMMA MemAssocLevel T_COMMA MemBandwidth 
         T_COMMA MemNextLevel T_COMMA MemPenalty T_RBRACKET
         {
            $$ = new MemoryHierarchyLevel ($2, $4, $6, $8, $10, NULL, $12, $14);
         }
       ;

MemNumBlocks: T_INT_CONST { $$ = $1; }
       | T_TIMES { $$ = MHL_UNLIMITED; }
       ;
       

MemBlockSize: T_INT_CONST { $$ = $1; }
       | T_TIMES { $$ = MHL_UNLIMITED; }
       ;
       
MemEntrySize: T_INT_CONST { $$ = $1; }
       | T_TIMES { $$ = MHL_UNDEFINED; }
       ;
       
MemAssocLevel: T_INT_CONST { $$ = $1; }
       | T_TIMES { $$ = MHL_FULLY_ASSOCIATIVE; }
       ;
       
MemBandwidth: T_FLOAT_CONST { $$ = $1; }
       | T_INT_CONST { $$ = (float)$1; }
       | T_TIMES { $$ = MHL_UNDEFINED; }
       ;
       
MemNextLevel: T_IDENTIFIER { $$ = $1; }
       | T_TIMES { $$ = NULL; }
       ;
       
MemPenalty: T_INT_CONST { $$ = $1; }
       | T_TIMES { $$ = 0; }
       ;

InstructionReplacement:
         T_REPLACE
         {
            $<position>$ = currentPosition;
         }
         ListGenericInstructions T_WITH 
         {
            $<position>$ = currentPosition;
         }
         ListGenericInstructions 
             RuleName T_SEMICOLON
         {  
            replacementRules->push_back(new ReplacementRule($3, $<position>2, $6, $<position>5, $7));
            $$ = 1;
         }
       ;

ListGenericInstructions: GenericInstruction
         {
            $$ = new GenInstList();
            $$->push_back($1);
         }
       | ListGenericInstructions T_PLUS GenericInstruction
         {
            $1->push_back($3);
            $$ = $1;
         }
       /* use ++ between generic instruction to restrict macthing only
        * to patterns where the previous instruction has no other outgoing
        * dependencies besides the one matched by the source pattern.
        */
       | ListGenericInstructions T_PLUSPLUS GenericInstruction
         {
            $1->back()->setExclusiveOutputMatch();
            $1->push_back($3);
            $$ = $1;
         }
       ;

GenericInstruction: InstType ListRegisters T_ARROW ListRegisters
         {
            $$ = new GenericInstruction($1, $2, $4);
         }
       | InstType ListRegisters T_ARROW 
         {
            $$ = new GenericInstruction($1, $2, NULL);
         }
       | InstType T_ARROW ListRegisters
         {
            $$ = new GenericInstruction($1, NULL, $3);
         }
       | InstType T_ARROW 
         {
            $$ = new GenericInstruction($1, NULL, NULL);
         }
       ;

ListRegisters: T_GP_REG_NAME
         {
            startPosition = currentPosition - yyleng;
            $$ = new GenRegList();
            $$->push_back(new GenericRegister($1, GP_REGISTER_TYPE, 
                       startPosition));
         }
       | ListRegisters T_COMMA T_GP_REG_NAME
         {
            startPosition = currentPosition - yyleng;
            $1->push_back(new GenericRegister($3, GP_REGISTER_TYPE, 
                       startPosition));
            $$ = $1;
         }
       | T_LBRACKET T_GP_REG_NAME { $<position>$ = currentPosition - yyleng;}
         T_RBRACKET
         {
            $$ = new GenRegList();
            $$->push_back(new GenericRegister($2, MEMORY_TYPE, $<position>3));
         }
       | ListRegisters T_COMMA T_LBRACKET T_GP_REG_NAME
         { $<position>$ = currentPosition - yyleng; } T_RBRACKET
         {
            $1->push_back(new GenericRegister($4, MEMORY_TYPE, $<position>5));
            $$ = $1;
         }
       ;

InstructionBypass:
         T_BYPASS { $<position>$ = currentPosition-yyleng; }
         T_LATENCY T_INT_CONST T_FOR ListInstructionTypes 
         DependencyTypes ListInstructionTypes T_SEMICOLON
         {
            assert ($4 >= 0);
            bypassRules->push_back(new 
                        BypassRule($6, $7, $8, $4, $<position>2));
            $$ = 1;
         }
       ;

ListInstructionTypes:
         InstType
         {
            $$ = new ICSet();
            $$->insert($1);
         }
       | ListInstructionTypes T_OR InstType
         {
            $$ = $1;
            $$->insert($3);
         }
       | T_ANY_INSTRUCTION
         {
            $$ = new ICSet();
            $$->setAllValues();
         }
       ;  

DependencyTypes:
         T_ARROW { $$ = new BitSet(~BitSet(DTYPE_TOP_MARKER)); }
       | T_ARROW T_LBRACKET ListDependencyTypes T_RBRACKET { $$ = $3; }
       ;
       
ListDependencyTypes:
         T_DEPENDENCY_TYPE
         {
            assert ($1 < DTYPE_TOP_MARKER);
            $$ = new BitSet (DTYPE_TOP_MARKER);
            *($$) += $1;
         }
       | ListDependencyTypes T_OR T_DEPENDENCY_TYPE
         {
            assert ($3 < DTYPE_TOP_MARKER);
            *($1) += $3;
            $$ = $1;
         }
       ;
       
InstructionExecution:
         T_INSTRUCTION InstType { $<position>$ = currentPosition-yyleng; }
         T_TEMPLATE T_ASSIGN InstTemplates T_SEMICOLON
         {
            $$ = instTemplates->addDescription($2, $<position>3, templateList);
         }
       | T_INSTRUCTION InstType { $<position>$ = currentPosition-yyleng; }
         T_LBRACKET AsyncUsage T_RBRACKET T_TEMPLATE T_ASSIGN InstTemplates T_SEMICOLON
         {
            $$ = instTemplates->addDescription($2, $<position>3, templateList, $5);
         }
       ;

/** Replace the use of T_INST_TYPE with a InstType non-terminal of type
 * InstructionClass, everywhere. That includes, Bypass rules and replacement
 * rules. Fix InstType first so it is of type InstructionClass.
 * InstructionContainer might have to contain a 3D array inside, instead of
 * exposing the first dimension (InstrBin type) outside of the container.
 * TODO
 */ 
InstType:
         T_INST_TYPE
         {
            assert($1 < IB_TOP_MARKER);
            $$.Initialize((MIAMI::InstrBin)$1);
         }
       | T_INST_TYPE T_COLON T_UNIT_TYPE
         {
            assert($1 < IB_TOP_MARKER);
            $$.Initialize((MIAMI::InstrBin)$1, ExecUnit_SCALAR, 0, (MIAMI::ExecUnitType)$3);
         }
       | T_INST_TYPE T_COLON T_UNIT_TYPE T_COMMA T_VECTOR_UNIT T_LCPAREN T_INT_CONST T_RCPAREN
         {
            assert($1 < IB_TOP_MARKER);
            $$.Initialize((MIAMI::InstrBin)$1, ExecUnit_VECTOR, $7, (MIAMI::ExecUnitType)$3);
         }
       | T_INST_TYPE T_COLON T_VECTOR_UNIT T_LCPAREN T_INT_CONST T_RCPAREN T_COMMA T_UNIT_TYPE
         {
            assert($1 < IB_TOP_MARKER);
            $$.Initialize((MIAMI::InstrBin)$1, ExecUnit_VECTOR, $5, (MIAMI::ExecUnitType)$8);
         }
       | T_INST_TYPE T_COLON T_VECTOR_UNIT T_LCPAREN T_INT_CONST T_RCPAREN
         {
            assert($1 < IB_TOP_MARKER);
            $$.Initialize((MIAMI::InstrBin)$1, ExecUnit_VECTOR, $5, ExecUnitType_INT);
         }
       | T_INST_TYPE T_LCPAREN T_INT_CONST T_RCPAREN
         {
            assert($1 < IB_TOP_MARKER);
            $$.Initialize((MIAMI::InstrBin)$1, ExecUnit_SCALAR, 0, ExecUnitType_INT, (width_t)$3);
         }
       | T_INST_TYPE T_LCPAREN T_INT_CONST T_RCPAREN T_COLON T_UNIT_TYPE
         {
            assert($1 < IB_TOP_MARKER);
            $$.Initialize((MIAMI::InstrBin)$1, ExecUnit_SCALAR, 0, (MIAMI::ExecUnitType)$6, (width_t)$3);
         }
       | T_INST_TYPE T_LCPAREN T_INT_CONST T_RCPAREN T_COLON T_UNIT_TYPE T_COMMA T_VECTOR_UNIT T_LCPAREN T_INT_CONST T_RCPAREN
         {
            assert($1 < IB_TOP_MARKER);
            $$.Initialize((MIAMI::InstrBin)$1, ExecUnit_VECTOR, $10, (MIAMI::ExecUnitType)$6, (width_t)$3);
         }
       | T_INST_TYPE T_LCPAREN T_INT_CONST T_RCPAREN T_COLON T_VECTOR_UNIT T_LCPAREN T_INT_CONST T_RCPAREN T_COMMA T_UNIT_TYPE
         {
            assert($1 < IB_TOP_MARKER);
            $$.Initialize((MIAMI::InstrBin)$1, ExecUnit_VECTOR, $8, (MIAMI::ExecUnitType)$11, (width_t)$3);
         }
       | T_INST_TYPE T_LCPAREN T_INT_CONST T_RCPAREN T_COLON T_VECTOR_UNIT T_LCPAREN T_INT_CONST T_RCPAREN
         {
            assert($1 < IB_TOP_MARKER);
            $$.Initialize((MIAMI::InstrBin)$1, ExecUnit_VECTOR, $8, ExecUnitType_INT, (width_t)$3);
         }
       ;
       
InstTemplates:
         InstTemplate
         {
            /* start a new template list */
            templateList.clear();
            templateList.push_back($1);
            $$ = 1;   // length 1
         }
       | InstTemplates T_OR InstTemplate
         {
            templateList.push_back($3);
            $$ = $1 + 1;
         }
       ;
       
InstTemplate:
         CycleTemplate
         {
            $$ = new InstTemplate();
            $$->addCycles($1);
         }
       | InstTemplate T_COMMA CycleTemplate
         {
            $1->addCycles($3);
            $$ = $1;
         }
       | CycleTemplate T_TIMES T_INT_CONST
         {
            $$ = new InstTemplate();
            $$->addCycles($1, $3);
         }
       | InstTemplate T_COMMA CycleTemplate T_TIMES T_INT_CONST
         {
            $1->addCycles($3, $5);
            $$ = $1;
         }
       ;

AsyncUsage:
         AsyncClass
         {
            $$ = new UnitCountList();
            $$->push_back($1);
         }
       | AsyncUsage T_PLUS AsyncClass
         {
            $1->push_back($3);
            $$ = $1;
         }
       ;

AsyncClass: 
         T_IDENTIFIER 
         {
            startPosition = currentPosition - yyleng;
            // set in a global variable the number of units of this type
            unit = asyncUnits->getMappingForName ($1, startPosition);
            if (unit == NO_MAPPING)
            {
               haveErrors += 1;
               fprintf(stderr, "Error %d (%d, %d): The requested asyncronous resource \"%s\" has not been defined.\n",
                    haveErrors, startPosition.Line(), startPosition.Column(), $1);
            }
         }
         UnitCount
         {
            int t_count = $3;
            /* in this context, if count was not specified, this means 
               only one unit is needed. */
            if (t_count==0)
               t_count = 1;
            $$.Assign(unit, t_count);
         }
       ;

CycleTemplate:
         T_NOTHING
         {
            /* use a NULL set when Nothing is specified */
            $$ = NULL;
         }
       | T_ALL_UNITS
         {
            startPosition = currentPosition - yyleng;
            $$ = new TEUList ();
            for (int i=0 ; i<numUnits ; ++i)
            {
               int _count = cpuUnits->getElementAtIndex (i) ->getCount ();
               BitSet *_umask = new BitSet (~BitSet(_count));
               $$->push_back (new TemplateExecutionUnit (i, _umask, 0));
            }
         }
       | UnitClass
         {
            $$ = new TEUList ();
            $$->push_back ($1);
         }
       | CycleTemplate T_PLUS UnitClass
         {
            if ($1 == NULL)
            {
               haveErrors += 1;
               fprintf(stderr, "Error %d (%d, %d): You cannot use another unit if you specified 'NOTHING' for this cycle already.\n",
                    haveErrors, startPosition.Line(), startPosition.Column());
            } else
               $1->push_back ($3);
            $$ = $1;
         }
       ;

UnitClass: 
         T_IDENTIFIER 
         {
            startPosition = currentPosition - yyleng;
            // set in a global variable the number of units of this type
            unit = cpuUnits->getMappingForName ($1, startPosition);
            if (unit != NO_MAPPING)
               unitReplication = cpuUnits->getElementAtIndex (unit) 
                         ->getCount ();
            else
               unitReplication = 1;
            templateMask = new BitSet (unitReplication);
         }
         UnitRange UnitCount
         {
            int t_range = $3;
            int t_count = $4;
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
            $$ = new TemplateExecutionUnit (unit, templateMask, t_count);
         }
       ;

UnitRange: /* empty */
         {
            $$ = 0;
         }
       | T_LBRACKET RangeIntervals T_RBRACKET
         {
            $$ = $2;
         }
       ;

RangeIntervals:
         OneInterval
         {
            $$ = 1;
         }
       | RangeIntervals T_COMMA OneInterval
         {
            $$ = $1 + 1;
         }
       ;

OneInterval: 
         T_INT_CONST
         {
            *templateMask += $1;
            $$ = 1;
         }
       | IntervalLimit T_COLON IntervalLimit   /* stride 1 interval */
         {
            int lLimit = $1;
            if (lLimit == -1) lLimit = 0;
            int uLimit = $3;
            if (uLimit == -1) uLimit = unitReplication-1;
            for (int i=lLimit ; i<=uLimit ; ++i)
               *templateMask += i;
            $$ = 2;
         }
       | IntervalLimit T_COLON IntervalLimit T_COLON IntervalLimit
         {  /* strided interval */
            int lLimit = $1;
            if (lLimit == -1) lLimit = 0;
            int uLimit = $5;
            if (uLimit == -1) uLimit = unitReplication-1;
            int stride = $3;
            if (stride == -1) stride = 1;
            for (int i=lLimit ; i<=uLimit ; i+=stride)
               *templateMask += i;
            $$ = 3;
         }
       ;

IntervalLimit: /* empty */
         {
            $$ = -1;
         }
       | T_INT_CONST
         {
            $$ = $1;
         }
       ;

UnitCount: /* empty */
         {
            $$ = 0;
         }
       | T_LPAREN T_INT_CONST T_RPAREN
         {
            $$ = $2;
         }
       ;

%%


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
