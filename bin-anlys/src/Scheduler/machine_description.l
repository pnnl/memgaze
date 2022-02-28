WS      [ \t\n]
INTNUM  0|([1-9][0-9]*)

%{
/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: machine_description.l
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: The lex parser for the MIAMI Machine Description Language.
 */

#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include "instr_bins.H"
#include "instr_info.H"
#include "position.h"
#include "Machine.h"
#include "BitSet.h"
#include "InstTemplate.h"
#include "Instruction.h"
#include "GenericInstruction.h"

using namespace MIAMI;
#include "machdesc.tab.h"

/*#undef yywrap; */
#define TAB_WIDTH 8

namespace MIAMI
{

Position currentPosition;
int haveErrors = 0;
}
%}


%x comment

%%

CpuUnits        { currentPosition += yyleng; return T_CPU_UNITS; }
AsyncResources  { currentPosition += yyleng; return T_ASYNC_RESOURCES; }
MemoryHierarchy { currentPosition += yyleng; return T_MEMORY_HIERARCHY; }
Machine         { currentPosition += yyleng; return T_MACHINE; }
Version         { currentPosition += yyleng; return T_VERSION; }
Instruction     { currentPosition += yyleng; return T_INSTRUCTION; }
template        { currentPosition += yyleng; return T_TEMPLATE; }
Replace         { currentPosition += yyleng; return T_REPLACE; }
with            { currentPosition += yyleng; return T_WITH; }
Maximum         { currentPosition += yyleng; return T_MAXIMUM; }
RetireRate      { currentPosition += yyleng; return T_RETIRE; }
WindowSize      { currentPosition += yyleng; return T_WINDOW_SIZE; }
from            { currentPosition += yyleng; return T_FROM; }
for             { currentPosition += yyleng; return T_FOR; }
Bypass          { currentPosition += yyleng; return T_BYPASS; }
latency         { currentPosition += yyleng; return T_LATENCY; }
vec             { currentPosition += yyleng; return T_VECTOR_UNIT; }

memory          { currentPosition += yyleng; 
                  yylval.int_value = MEMORY_TYPE; return T_DEPENDENCY_TYPE; }
control         { currentPosition += yyleng; 
                  yylval.int_value = CONTROL_TYPE; return T_DEPENDENCY_TYPE; }
addr_register   { currentPosition += yyleng; 
                  yylval.int_value = ADDR_REGISTER_TYPE; return T_DEPENDENCY_TYPE; }
gp_register     { currentPosition += yyleng; 
                  yylval.int_value = GP_REGISTER_TYPE; return T_DEPENDENCY_TYPE; }

fp              { currentPosition += yyleng; 
                  yylval.int_value = ExecUnitType_FP; return T_UNIT_TYPE; }
int             { currentPosition += yyleng; 
                  yylval.int_value = ExecUnitType_INT; return T_UNIT_TYPE; }

InnerLoop       { currentPosition += yyleng; 
                  yylval.int_value = IB_inner_loop; return T_INST_TYPE; }
IntrinsicCall   { currentPosition += yyleng; 
                  yylval.int_value = IB_intrinsic; return T_INST_TYPE; }
Load            { currentPosition += yyleng; 
                  yylval.int_value = IB_load; return T_INST_TYPE; }
LoadConf        { currentPosition += yyleng; 
                  yylval.int_value = IB_load_conf; return T_INST_TYPE; }
LoadStore       { currentPosition += yyleng; 
                  yylval.int_value = IB_load_store; return T_INST_TYPE; }
MemoryFence     { currentPosition += yyleng; 
                  yylval.int_value = IB_mem_fence; return T_INST_TYPE; }
PrivilegedOp    { currentPosition += yyleng; 
                  yylval.int_value = IB_privl_op; return T_INST_TYPE; }
Trap            { currentPosition += yyleng; 
                  yylval.int_value = IB_trap; return T_INST_TYPE; }
UncondBranch    { currentPosition += yyleng; 
                  yylval.int_value = IB_branch; return T_INST_TYPE; }
CondBranch      { currentPosition += yyleng; 
                  yylval.int_value = IB_br_CC; return T_INST_TYPE; }
Jump            { currentPosition += yyleng; 
                  yylval.int_value = IB_jump; return T_INST_TYPE; }
Return          { currentPosition += yyleng; 
                  yylval.int_value = IB_ret; return T_INST_TYPE; }
Convert         { currentPosition += yyleng; 
                  yylval.int_value = IB_cvt; return T_INST_TYPE; }
ConvertPrec     { currentPosition += yyleng; 
                  yylval.int_value = IB_cvt_prec; return T_INST_TYPE; }
Copy            { currentPosition += yyleng; 
                  yylval.int_value = IB_copy; return T_INST_TYPE; }
Move            { currentPosition += yyleng; 
                  yylval.int_value = IB_move; return T_INST_TYPE; }
MoveCC          { currentPosition += yyleng; 
                  yylval.int_value = IB_move_cc; return T_INST_TYPE; }
Shuffle         { currentPosition += yyleng; 
                  yylval.int_value = IB_shuffle; return T_INST_TYPE; }
Blend           { currentPosition += yyleng; 
                  yylval.int_value = IB_blend; return T_INST_TYPE; }
Misc            { currentPosition += yyleng; 
                  yylval.int_value = IB_misc; return T_INST_TYPE; }
Cmp             { currentPosition += yyleng; 
                  yylval.int_value = IB_cmp; return T_INST_TYPE; }
Add             { currentPosition += yyleng; 
                  yylval.int_value = IB_add; return T_INST_TYPE; }
LEA             { currentPosition += yyleng; 
                  yylval.int_value = IB_lea; return T_INST_TYPE; }
AddCC           { currentPosition += yyleng; 
                  yylval.int_value = IB_add_cc; return T_INST_TYPE; }
Sub             { currentPosition += yyleng; 
                  yylval.int_value = IB_sub; return T_INST_TYPE; }
Mult            { currentPosition += yyleng; 
                  yylval.int_value = IB_mult; return T_INST_TYPE; }
Div             { currentPosition += yyleng; 
                  yylval.int_value = IB_div; return T_INST_TYPE; }
Sqrt            { currentPosition += yyleng; 
                  yylval.int_value = IB_sqrt; return T_INST_TYPE; }
MultAdd         { currentPosition += yyleng; 
                  yylval.int_value = IB_madd; return T_INST_TYPE; }
Xor             { currentPosition += yyleng; 
                  yylval.int_value = IB_xor; return T_INST_TYPE; }
LogicalOp       { currentPosition += yyleng; 
                  yylval.int_value = IB_logical; return T_INST_TYPE; }
Shift           { currentPosition += yyleng; 
                  yylval.int_value = IB_shift; return T_INST_TYPE; }
NOP             { currentPosition += yyleng; 
                  yylval.int_value = IB_nop; return T_INST_TYPE; }
Prefetch        { currentPosition += yyleng; 
                  yylval.int_value = IB_prefetch; return T_INST_TYPE; }
GetF            { currentPosition += yyleng; 
                  yylval.int_value = IB_getf; return T_INST_TYPE; }
SetF            { currentPosition += yyleng; 
                  yylval.int_value = IB_setf; return T_INST_TYPE; }
Store           { currentPosition += yyleng; 
                  yylval.int_value = IB_store; return T_INST_TYPE; }
StoreConf       { currentPosition += yyleng; 
                  yylval.int_value = IB_store_conf; return T_INST_TYPE; }
UnknownOp       { currentPosition += yyleng; 
                  yylval.int_value = IB_unknown; return T_INST_TYPE; }
PopCnt          { currentPosition += yyleng; 
                  yylval.int_value = IB_popcnt; return T_INST_TYPE; }
PortRead        { currentPosition += yyleng; 
                  yylval.int_value = IB_port_rd; return T_INST_TYPE; }
PortWrite       { currentPosition += yyleng; 
                  yylval.int_value = IB_port_wr; return T_INST_TYPE; }
Reciprocal      { currentPosition += yyleng; 
                  yylval.int_value = IB_reciprocal; return T_INST_TYPE; }
Insert          { currentPosition += yyleng; 
                  yylval.int_value = IB_insert; return T_INST_TYPE; }
Extract         { currentPosition += yyleng; 
                  yylval.int_value = IB_extract; return T_INST_TYPE; }
Xchg            { currentPosition += yyleng; 
                  yylval.int_value = IB_xchg; return T_INST_TYPE; }
CmpXchg         { currentPosition += yyleng; 
                  yylval.int_value = IB_cmp_xchg; return T_INST_TYPE; }
Rotate          { currentPosition += yyleng; 
                  yylval.int_value = IB_rotate; return T_INST_TYPE; }
RotateCC        { currentPosition += yyleng; 
                  yylval.int_value = IB_rotate_cc; return T_INST_TYPE; }
Function        { currentPosition += yyleng; 
                  yylval.int_value = IB_fn; return T_INST_TYPE; }


\=              { currentPosition += yyleng; return T_ASSIGN; }
\|              { currentPosition += yyleng; return T_OR; }
\:              { currentPosition += yyleng; return T_COLON; }
\;              { currentPosition += yyleng; return T_SEMICOLON; }
\,              { currentPosition += yyleng; return T_COMMA; }
\[              { currentPosition += yyleng; return T_LBRACKET; }
\]              { currentPosition += yyleng; return T_RBRACKET; }
\(              { currentPosition += yyleng; return T_LPAREN; }
\)              { currentPosition += yyleng; return T_RPAREN; }
\{              { currentPosition += yyleng; return T_LCPAREN; }
\}              { currentPosition += yyleng; return T_RCPAREN; }
\+\+            { currentPosition += yyleng; return T_PLUSPLUS; }
\+              { currentPosition += yyleng; return T_PLUS; }
\*              { currentPosition += yyleng; return T_TIMES; }
\-\>            { currentPosition += yyleng; return T_ARROW; }

NOTHING         { currentPosition += yyleng; return T_NOTHING; }
ALL_UNITS       { currentPosition += yyleng; return T_ALL_UNITS; }
ANY_INSTRUCTION { currentPosition += yyleng; return T_ANY_INSTRUCTION; }

\'[^\'\n]\'                  { currentPosition += yyleng;
                               yylval.ch_value = yytext[1];
                               return T_CHAR_CONST; }
\"[^\"\n]*\"                 { currentPosition += yyleng; 
                               yylval.p_char = new char[yyleng-1];
                               strncpy(yylval.p_char, yytext+1, yyleng-2);
                               yylval.p_char[yyleng-2] = 0;
                               return T_STR_CONST; }
[_a-zA-Z]([_a-zA-Z]|[0-9])*  { currentPosition += yyleng; 
                               yylval.p_char = new char[yyleng+1];
                               strcpy(yylval.p_char, yytext);
                               return T_IDENTIFIER; }
\$[a-zA-Z]([a-zA-Z0-9])*     { currentPosition += yyleng; 
                               yylval.p_char = new char[yyleng];
                               strcpy(yylval.p_char, yytext+1);
                               return T_GP_REG_NAME; }

{INTNUM}                     { currentPosition += yyleng; 
                               yylval.int_value = atoi(yytext);
                               return T_INT_CONST; }

{INTNUM}(\.)(0*{INTNUM}?)((e|E)(\+|\-)?[0-9]*)?/[^\.]   { currentPosition += yyleng;
                                                          yylval.fl_value = atof(yytext);
                                                          return T_FLOAT_CONST; }

\/\*                         { currentPosition += yyleng;
                               BEGIN comment;
                             }
<comment>\n                  { currentPosition++; }
<comment>\t*                 { currentPosition.tab(TAB_WIDTH, yyleng); }
<comment>.                   { currentPosition += yyleng; }
<comment>\*\/                { currentPosition += yyleng;
                               BEGIN INITIAL;
                             }
[ ]*                         { currentPosition += yyleng; }
\t*                          { currentPosition.tab(TAB_WIDTH, yyleng); }
\n                           { currentPosition++; }

.        {  
           haveErrors += 1;
           fprintf(stderr, "Error %d: File %s (%d,%d): illegal characters '%s'.\n", 
               haveErrors, currentPosition.FileName(), 
               currentPosition.Line(), currentPosition.Column(), yytext);
           currentPosition += yyleng;
         }

%%

int
yywrap()
{
  return 1;
}

