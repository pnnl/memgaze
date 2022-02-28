/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: generic_math_expressions.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Support for generic simple math formulas.
 */

#ifndef GENERIC_MATH_EXPRESSION_H
#define GENERIC_MATH_EXPRESSION_H

#include <stdio.h>
#include <stdlib.h>
#include "miami_types.h"
#include "register_class.h"

namespace MIAMI
{

// support description of machine independent mathematical expressions
// Right now we only need + and * operators
enum ExpressionOperator {
   ExpressionOperator_INVALID,
   ExpressionOperator_ADD,
   ExpressionOperator_MUL,
   ExpressionOperator_LAST
};

enum ExpressionOperandType {
   ExpressionOperandType_INVALID,
   ExpressionOperandType_IMM,
   ExpressionOperandType_REG,
   ExpressionOperandType_LAST
};


class ExpressionOperand
{
public:
   ExpressionOperand()
   {
      type = ExpressionOperandType_INVALID;
      val.imm = imm_value_t();
   }
   ExpressionOperand(const imm_value_t &v)
   {
      type = ExpressionOperandType_IMM;
      val.imm = v;
   }
   ExpressionOperand(const register_info &v)
   {
      type = ExpressionOperandType_REG;
      val.reg = v;
   }
   
   ExpressionOperandType type;
   union {
      imm_value_t imm;
      register_info reg;
   } val;
};

// base class for an expression
class ExpressionBase
{
public:
   // define a function type used as a callback by the Traverse method of an expression
   typedef int (*TraverseCallback)(void* arg0);

   ExpressionBase(const ExpressionBase *l = 0, const ExpressionBase *r = 0) 
           : left(l), right(r)
   {}
   virtual ~ExpressionBase()
   {
      if (left)  delete (left);
      if (right) delete (right);
   }
   virtual int Traverse(void *arg0) = 0;
   
   ExpressionBase *left, *right;
};

class ExpressionTerm: public ExpressionBase
{
public:
   ExpressionTerm(const ExpressionOperand &op) : ExpressionBase()
   {
      term = op;
   }
   virtual ~ExpressionTerm()
   { }

   virtual int Traverse(void *arg0);
   
   ExpressionOperand term;
};

class Expression : public ExpressionBase
{
public:
   Expression(const ExpressionBase *op1, ExpressionOperator opt, const ExpressionBase *op2) 
          : ExpressionBase(op1, op2)
   {
      optype = opt;
   }
   virtual ~Expression()
   { }

   virtual int Traverse(void *arg0);
   
   ExpressionOperator optype;
};

} /* namespace MIAMI */

#endif
