/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: slice_references.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements data flow analysis. Slice on register names to 
 * compute symbolic formulas for memory reference addresses.
 */

#include <stdio.h>
#include <assert.h>
#include "math_routines.h"

#include "slice_references.h"
#include "InstructionDecoder.hpp"

namespace MIAMI
{
#define DEBUG_REFERENCE_SLICE 0
#define TRACE_REFERENCE_SLICE 0

//GFSliceVal defaultGFS;
RefFormulas *defaultRFP = 0;
coeff_t defaultZeroCoeff = 0;

#ifndef min
#define min(x,y)  ((x)<(y)?(x):(y))
#endif

static coeff_t 
greatestCommonDenominator(ucoeff_t arg1, ucoeff_t arg2)
{  // use Euclid's algorithm
   ucoeff_t a, b, r;
   if (arg1==0 || arg2==0)
      return 0;
   if (arg1 > arg2)
   {
      a = arg2;
      b = arg1 % arg2;
   } else
   {
      a = arg1;
      b = arg2 % arg1;
   }
   while(b!=0)
   {
      r = a % b;
      a = b; b = r;
   }
   return (a);
}


sliceVal
sliceVal::operator+ (const sliceVal& sv) const
{
   assert(term_type == sv.term_type);
   sliceVal temp(*this);
   temp.valueNum = temp.valueNum*sv.valueDen + sv.valueNum*temp.valueDen;
   temp.valueDen = temp.valueDen*sv.valueDen;
   temp.expand |= sv.expand;
   return (temp);
}

sliceVal
sliceVal::operator- (const sliceVal& sv) const
{
   assert(term_type == sv.term_type);
   sliceVal temp(*this);
   temp.valueNum = temp.valueNum*sv.valueDen - sv.valueNum*temp.valueDen;
   temp.valueDen = temp.valueDen*sv.valueDen;
   temp.expand |= sv.expand;
   return (temp);
}

sliceVal&
sliceVal::operator+= (const sliceVal& sv)
{
   assert(term_type == sv.term_type);
   valueNum = valueNum*sv.valueDen + sv.valueNum*valueDen;
   valueDen = valueDen*sv.valueDen;
   expand |= sv.expand;
   return (*this);
}

sliceVal&
sliceVal::operator-= (const sliceVal& sv)
{
   assert(term_type == sv.term_type);
   valueNum = valueNum*sv.valueDen - sv.valueNum*valueDen;
   valueDen = valueDen*sv.valueDen;
   expand |= sv.expand;
   return (*this);
}

sliceVal
sliceVal::operator* (coeff_t factor) const
{
   sliceVal temp(*this);
   temp.valueNum = temp.valueNum*factor;
   if (temp.valueNum != 0)
   {
      coeff_t gcd = greatestCommonDenominator(abs_coeff(temp.valueNum), 
                   temp.valueDen);
      if (gcd && gcd!=1)
      {
         temp.valueNum /= gcd; temp.valueDen /= gcd;
      }
   }
   return (temp);
}

sliceVal&
sliceVal::operator*= (coeff_t factor)
{
   valueNum = valueNum*factor;
   if (valueNum != 0)
   {
      coeff_t gcd = greatestCommonDenominator(abs_coeff(valueNum), 
                   valueDen);
      if (gcd && gcd != 1)
      {
         valueNum /= gcd; valueDen /= gcd;
      }
   }
   return (*this);
}

sliceVal
sliceVal::operator/ (ucoeff_t factor) const
{
   sliceVal temp(*this);
   temp.valueDen = temp.valueDen*factor;
   if (temp.valueNum != 0)
   {
      coeff_t gcd = greatestCommonDenominator(abs_coeff(temp.valueNum), 
                    temp.valueDen);
      if (gcd && gcd!=1)
      {
         temp.valueNum /= gcd; temp.valueDen /= gcd;
      }
      if (temp.term_type == TY_CONSTANT)
         temp.valueNum /= temp.valueDen;
   }
   return (temp);
}

sliceVal&
sliceVal::operator/= (ucoeff_t factor)
{
   valueDen = valueDen*factor;
   if (valueNum != 0)
   {
      coeff_t gcd = greatestCommonDenominator(abs_coeff(valueNum), 
                   valueDen);
      if (gcd && gcd!=1)
      {
         valueNum /= gcd; valueDen /= gcd;
      }
      if (term_type == TY_CONSTANT)
         valueNum /= valueDen;
   }
   return (*this);
}

// With Sun studio 11 compilers and xarch=v8plus (32 bit addresses, 64 bit
// physical registers), 64 bit registers are logically used as two 32 bit 
// registers: their HI and LOW word halfs. This breaks slice_references where
// a physical register was assumed to hold one 32 bit value.
// Now I will compute separate formulas, in parallel, for the hi and low 
// parts of a register. Only SRLX and SLLX of 32 bits can move formulas
// from the hi to the low and vice-versa respectively.

std::string
sliceVal::toString () const
{
   char buf[128];
   std::string s = " ";
   if (valueNum<0) s += "-";
   if (abs_coeff(valueNum)!=1 || term_type==TY_CONSTANT) 
   {
      sprintf(buf, "0x%" PRIxucoeff, (abs_coeff(valueNum)));
      s += buf;
   }
   if (term_type == TY_REGISTER)
   {
      sprintf(buf, " Reg_%s/%d{%s}[%d,%d](0x%" PRIxaddr ",%d)", 
           reg.RegNameToString().c_str(), reg.stack, 
           RegisterClassToString(reg.type), reg.lsb, reg.msb, pc, uop_idx);
      s += buf;
   }
   if (term_type == TY_LOAD)
   {
      sprintf(buf, " LoadToReg_%s/%d{%s}[%d,%d](0x%" PRIxaddr ",%d)", 
           reg.RegNameToString().c_str(), reg.stack, 
           RegisterClassToString(reg.type), reg.lsb, reg.msb, pc, uop_idx);
      s += buf;
   }
   if (term_type == TY_CALL)
   {
      sprintf(buf, " CallTo[0x%" PRIxaddr "](0x%" PRIxaddr ",%d)", auxinfo, pc, uop_idx);
      s += buf;
   }
   if (term_type == TY_REFERENCE)
   {
      sprintf(buf, " Load[0x%" PRIxaddr "](0x%" PRIxaddr ",%d)", auxinfo, pc, uop_idx);
      s += buf;
   }
   if (term_type == TY_STACK)
   {
      sprintf(buf, " Stack[0x%" PRIxaddr "](0x%" PRIxaddr ",%d)", auxinfo, pc, uop_idx);
      s += buf;
   }
   if (term_type == TY_UNKNOWN)
   {
      s += " UNKN";
   }
   if (valueDen != 1) 
   {
      sprintf(buf, "/0x%" PRIxucoeff, valueDen);
      s += buf;
   }
   if (expand)
   {
      s += "(*X*)";
   }
   s += " ";
   return s;
}

bool 
sliceVal::operator== (const sliceVal& sv) const
{
   if (term_type != sv.term_type)
      return (false);
      
   if (term_type == TY_CONSTANT)
      return (true);   // nothing else to test for constants
         
   // else (sv1.TermType() == sv2.TermType())
   // references and stack accesses are both loads from well defined 
   // locations (sort of constant). Compare the locations for these
   // terms. It does not matter the PC of the load.
   if (term_type==TY_REFERENCE || term_type==TY_STACK)
      return (auxinfo == sv.auxinfo);
      
   // Otherwise, should check the addresses where the (symbolic) values are defined
   if (pc!=sv.pc || uop_idx!=sv.uop_idx)
      return (false);
      
   // PCs are the same
   if (term_type==TY_REGISTER || term_type==TY_LOAD)
      return (reg == sv.reg);
   
   return (true);
}

bool 
sliceVal::operator!= (const sliceVal& sv) const
{
   if (term_type != sv.term_type)
      return (true);
      
   if (term_type == TY_CONSTANT)
      return (false);   // nothing else to test for constants
         
   // else (term_type == sv.term_type)
   // references and stack accesses are both loads from well defined 
   // locations (sort of constant). Compare the locations for these
   // terms. We do not care about the PC of the load.
   if (term_type==TY_REFERENCE || term_type==TY_STACK)
      return (auxinfo != sv.auxinfo);
      
   // Otherwise, should check the addresses where the (symbolic) values are defined
   if (pc!=sv.pc || uop_idx!=sv.uop_idx)
      return (true);
      
   // PCs are the same
   if (term_type==TY_REGISTER || term_type==TY_LOAD)
      return (reg != sv.reg);
   
   return (false);
}

void
sliceVal::SaveToFile(FILE* fd) const
{
   fwrite(&valueNum, sizeof(valueNum), 1, fd);
   fwrite(&valueDen, sizeof(valueDen), 1, fd);
   unsigned int type2 = term_type;
   fwrite(&type2, sizeof(type2), 1, fd);
   fwrite(&pc, sizeof(pc), 1, fd);
   fwrite(&uop_idx, sizeof(uop_idx), 1, fd);
   fwrite(&auxinfo, sizeof(auxinfo), 1, fd);
   if (term_type==TY_LOAD || term_type==TY_REGISTER)
      reg.SaveToFile(fd);
   // do not save the expand info; it is useful only when the formulas
   // are being built;
}

void
sliceVal::LoadFromFile(FILE* fd)
{
   fread(&valueNum, sizeof(valueNum), 1, fd);
   fread(&valueDen, sizeof(valueDen), 1, fd);
   unsigned int type2;
   fread(&type2, sizeof(type2), 1, fd);
   term_type = (term_slice_type)type2;
   fread(&pc, sizeof(pc), 1, fd);
   fread(&uop_idx, sizeof(uop_idx), 1, fd);
   fread(&auxinfo, sizeof(auxinfo), 1, fd);
   expand = 0;
   if (term_type==TY_LOAD || term_type==TY_REGISTER)
      reg.LoadFromFile(fd);
}

std::ostream& operator<< (std::ostream& os, sliceVal& sv)
{
   os << sv.toString();
   return (os);
}

std::ostream& operator<< (std::ostream& os, const sliceVal& sv)
{
   os << sv.toString();
   return (os);
}
/*
FILE* operator<< (FILE* fd, sliceVal& sv)
{
   sv.SaveToFile(fd);
   return (fd);
}
*/
FILE* operator<< (FILE* fd, const sliceVal& sv)
{
   sv.SaveToFile(fd);
   return (fd);
}

FILE* operator>> (FILE* fd, sliceVal& sv)
{
   sv.LoadFromFile(fd);
   return (fd);
}

static int 
deleteFormulasInMap(void *arg0, addrtype key, void *value)
{
//   fprintf(stderr, "Deleting atg0=%p, pc=0x%" PRIxaddr ", value=%p\n", arg0, pc, (void*)(*(addrtype*)value)
   RefFormulas *rf = static_cast<RefFormulas*>((void*)(*(addrtype*)value));
   delete (rf);
   return (0); 
}

void
RFormulasMap::clear()
{
   storage.map(deleteFormulasInMap, 0);
   storage.clear();
}

/* check if two formulas are identical; by that I mean the same
 * term types and the same coefficients, but also the same irregular 
 * and indirect flags. We test formula equality by computing their
 * difference and comparing it to zero. This approach allows for
 * a superfluous zero constant term in any of the formulas.
 */
bool
FormulasAreIdentical(GFSliceVal const &formula1, GFSliceVal const &formula2)
{
   if (formula1.is_uninitialized() || formula2.is_uninitialized())
   {
      fprintf(stderr, "WARNING: FormulasAreIdentical: comparing uninitialized formula\n");
      return (false);
   }
   coeff_t valueNum;
   ucoeff_t valueDen;
   if ( formula1.has_irregular_access()!= formula2.has_irregular_access() || 
        formula1.has_indirect_access() != formula2.has_indirect_access())
      return (false);
   return (IsConstantFormula(formula1-formula2, valueNum, valueDen) && valueNum==0);
}

/* check if the two formulas are equal; by that I mean the same
 * term types and the same coefficients. I could check if their
 * difference is zero, but try to have a direct test.
 * Computing the formulas difference is more general, as it
 * allows for a superfluous zero constant term in any of the formulas, 
 * while this direct test requires perfect equality of terms.
 */
bool
FormulasAreEqual(GFSliceVal const &formula1, GFSliceVal const &formula2)
{
   if (formula1.is_uninitialized() || formula2.is_uninitialized())
   {
      fprintf(stderr, "WARNING: FormulasAreEqual, formulas are uninitialized\n");
      return (false);
   }
   GFSliceVal::iterator it1, it2;
   coeff_t coef1, coef2;
   for (it1=formula1.begin(), it2=formula2.begin() ; 
        it1!=formula1.end() && it2!=formula2.end() ; 
        ++it1, ++it2)
   {
      if (*it1 != *it2)
         return (false);
      coef1 = it1->ValueNum()*it2->ValueDen();
      coef2 = it2->ValueNum()*it1->ValueDen();
      if (coef1 != coef2)
         return (false);
   }
   // both formulas should end at the same time
   // check if this is the case;
   if (it1==formula1.end() && it2==formula2.end())
      return (true);
   else
      return (false);
}

// check if the two formulas have an integer ratio, that is, 
// formula1 == a*formula2 + b. The 2 formulas must have the
// same base for this to be true, less a constant term, therefore the terms 
// must be sorted in the same order; take advantage of this natural sorting.
bool
HasIntegerRatioAndRemainder (GFSliceVal const &formula1, GFSliceVal const &formula2,
          coeff_t &ratio, coeff_t &remainder)
{
   if (formula1.is_uninitialized() || formula2.is_uninitialized())
   {
      fprintf(stderr, "WARNING: HasIntegerRatioAndRemainder, formulas are uninitialized\n");
      return (false);
   }
   GFSliceVal::iterator it1, it2;
   GFSliceVal::iterator const1, const2;
   
   coeff_t numConst1=0, numConst2=0;
   coeff_t coef1, coef2;
   coeff_t gcdVal;
   bool firstTerm = true;
   coeff_t factor1 = 1, factor2 = 1;
   
   it1=formula1.begin();
   it2=formula2.begin(); 
   while (it1!=formula1.end() && it2!=formula2.end())
   {
      if (it1->TermType()==TY_CONSTANT || it2->TermType()==TY_CONSTANT)
      {
         assert (numConst1==0 && numConst2==0);
         if (it1->TermType()==TY_CONSTANT)
         {
            ++ numConst1;
            const1 = it1;
            ++it1; // = formula1.next ();
         }
         if (it2->TermType()==TY_CONSTANT)
         {
            ++ numConst2;
            const2 = it2;
            ++it2; // = formula2.next ();
         }
      }
      else
      {
         if (*it1 != *it2)
            return (false);
         
         coef1 = it1->ValueNum()*it2->ValueDen();
         coef2 = it2->ValueNum()*it1->ValueDen();
         if (coef1<0 && coef2<0)
         {
            coef1 = -coef1;
            coef2 = -coef2;
         }
         // if both coefs are 0, then skip this term. We should not see unless
         // for constant terms in some instances.
         if (coef1==0 && coef2==0) 
         {
            ++it1;
            ++it2;
            continue;
         }
         // However, if we found the same term, but one has coeff 0, then it is not an
         // integer ratio.
         if (coef1==0 || coef2==0) 
            return (false);
         
         gcdVal = greatestCommonDenominator(
                       abs_coeff(coef1),
                       abs_coeff(coef2) );
         if (firstTerm)
         {
            factor1 = coef1 / gcdVal;  // should divide perfectly, it is gcd
            factor2 = coef2 / gcdVal;
            // I also care only if factor2 is 1 or -1.
            if (factor2!=1 && factor2!=(-1))
               return (false);
            firstTerm = false;
         } else
         {
            if (factor1 != (coef1/gcdVal))
               return (false);
            if (factor2 != (coef2/gcdVal))
               return (false);
         }
         ++it1;  // = formula1.next ();
         ++it2;  // = formula2.next ();
      }  // no term is constant
   }
   // it will exit when at least one iterator reaches formula's end
   // test if any formula has a remainder factor. Only constant is accepted
   assert (it1==formula1.end() || it2==formula2.end());
   if (it1!=formula1.end() || it2!=formula2.end())
   {
      if ( (it1!=formula1.end() && it1->TermType()!=TY_CONSTANT) || 
           (it2!=formula2.end() && it2->TermType()!=TY_CONSTANT) )
         return (false);
      // if we are here, it means the current term of the longer formula is
      // a constant term. I should not have seen any other constant term:
      assert (numConst1==0 && numConst2==0);
      if (it1!=formula1.end())
      {
         ++ numConst1;
         const1 = it1;
         ++it1; // = formula1.next ();
         // there should not be any other additional term
         if (it1!=formula1.end())
            return (false);
      }
      if (it2!=formula2.end())
      {
         ++ numConst2;
         const2 = it2;
         ++it2; // = formula2.next ();
         // there should not be any other additional term
         if (it2!=formula2.end())
            return (false);
      }
   }

   remainder = 0;
   if (!firstTerm)
      ratio = factor1 / factor2;
   else
      ratio = 0;
   if (numConst1 || numConst2)   // I have seen a constant term
   {
      if (numConst2==0)
      {
         remainder = const1->ValueNum() / const1->ValueDen();
      } 
      else
         if (numConst1==0)
         {
            if (!firstTerm)
            {
               remainder = -ratio * const2->ValueNum() / const2->ValueDen();
            }
         }
         else  // both formulas have some constant term
         {
            if (firstTerm)
            {
               remainder = const1->ValueNum() / const1->ValueDen();
            } else
            {
               
               coef1 = const1->ValueNum() / const1->ValueDen();
               coef2 = ratio * const2->ValueNum() / const2->ValueDen();
               remainder = coef1 - coef2;
            }
         }
   } 
   else
      if (firstTerm)
      {
         // I did not see any term, not even a constant term.
         // I should assert maybe?
         assert (! "HasIntegerRatioAndRemainder called with empty formulas.");
         return (false);
      }
   
   // we have everything computed at this point and formulas have integer ratio
   return (true);
}


// check if the two formulas have an integer ratio, that is, 
// formula1/factor1 == formula2/factor2. The 2 formula must have the
// same base for this to be true, therefore the terms must be sorted in
// the same order; take advantage of this natural sorting. Formulas may
// differ by a term which is zero. Sometimes such terms are not removed
// from the end of a formula.
bool
HasIntegerRatio(GFSliceVal const &formula1, GFSliceVal const &formula2,
          coeff_t &factor1, coeff_t &factor2)
{
   if (formula1.is_uninitialized() || formula2.is_uninitialized())
   {
      fprintf(stderr, "WARNING: HasIntegerRatio, formulas are uninitialized\n");
      return (false);
   }
   GFSliceVal::iterator it1, it2;
   coeff_t coef1, coef2;
   coeff_t gcdVal;
   bool firstTerm = true;
   factor1 = 1; factor2 = 1;
   for (it1=formula1.begin(), it2=formula2.begin() ; 
        it1!=formula1.end() && it2!=formula2.end() ; 
        ++it1, ++it2)
   {
      if (*it1 != *it2)
         return (false);
      
      coef1 = it1->ValueNum()*it2->ValueDen();
      coef2 = it2->ValueNum()*it1->ValueDen();
      if (coef1<0 && coef2<0)
      {
         coef1 = -coef1;
         coef2 = -coef2;
      }
      // if both coefs are 0, then skip this term. We should not see unless
      // for constant terms in some instances.
      if (coef1==0 && coef2==0) 
         continue;
      // However, if we found the same term, but one has coeff 0, then it is not an
      // integer ratio.
      if (coef1==0 || coef2==0) 
         return (false);
      
      gcdVal = greatestCommonDenominator(
                    abs_coeff(coef1),
                    abs_coeff(coef2) );
      if (firstTerm)
      {
         factor1 = coef1 / gcdVal;  // should divide perfectly, it is gcd
         factor2 = coef2 / gcdVal;
         firstTerm = false;
      } else
      {
         if (factor1 != (coef1/gcdVal))
            return (false);
         if (factor2 != (coef2/gcdVal))
            return (false);
      }
   }
   // we may have only a constant term zero at the end of one formula
   // check if this is such a case;
   assert (it1==formula1.end() || it2==formula2.end());
   if (it1!=formula1.end() || it2!=formula2.end())
   {
      if (it1!=formula1.end())
      {
         if (it1->TermType()!=TY_CONSTANT || it1->ValueNum()!=0)
            return (false);
         // now I just want to make sure that there is no other term
         // after this
         ++it1;
         // there should not be any other additional term
         if (it1!=formula1.end())
            return (false);
      } else   // (it2!=formula2.end())
      {
         if (it2->TermType()!=TY_CONSTANT || it2->ValueNum()!=0)
            return (false);
         // now I just want to make sure that there is no other term
         // after this
         ++it2;
         // there should not be any other additional term
         if (it2!=formula2.end())
            return (false);
      }
   }
   if (firstTerm)
   {
      factor1 = factor2 = 0;
   }
   // the final test: we should have parsed completely both formulas
   return (true);
}


/* return -1, 0, or 1 is the coeff of the first term is negative, zero 
 * (formula is zero in this case), or positive
 */
int
SignOfFormula(GFSliceVal const &formula)
{
   if (formula.is_uninitialized())
   {
      fprintf(stderr, "WARNING: SignOfFormula, formula is uninitialized\n");
      return (0);
   }
   GFSliceVal::iterator it = formula.begin();
   if (it!=formula.end())
   {
      coeff_t valueNum = it->ValueNum();
      if (valueNum < 0)
         return (-1);
      else 
         if (valueNum > 0)
            return (1);
         else
            return (0);
   }
   return (0);
}

// OZGUR PALM
// check if a GFSliceVal formula has one constant terms 
bool
hasConstantFormula(GFSliceVal const &formula, coeff_t &valueNum, ucoeff_t &valueDen)
{
   if (formula.is_uninitialized())
   {
      fprintf(stderr, "WARNING: IsConstantFormula, formula is uninitialized\n");
      assert (false);
      return (false);
   }
   GFSliceVal::iterator it;
   int numConst = 0;
   int notConst = 0;
   valueNum=0;
   valueDen=1;
   for (it=formula.begin() ; it!=formula.end() ; ++it)
   {
      if (it->TermType() == TY_CONSTANT)
      {
         assert(numConst==0);
         numConst++;
         valueNum = it->ValueNum();
         valueDen = it->ValueDen();
         std::cout<<"Bakalim ValueNum: "<<valueNum<<" valueDen: "<<valueDen<<std::endl;
      } else
         notConst = 1;
   }
   if (!numConst)
      return (false);
   return (true);
}

// check if a GFSliceVal formula has only constant terms (one term at most
// is possible) and if yes, set the value of that term in the output 
// argument value
bool
IsConstantFormula(GFSliceVal const &formula, coeff_t &valueNum, ucoeff_t &valueDen)
{
   if (formula.is_uninitialized())
   {
      fprintf(stderr, "WARNING: IsConstantFormula, formula is uninitialized\n");
      assert (false);
      return (false);
   }
   GFSliceVal::iterator it;
   int numConst = 0;
   int notConst = 0;
   valueNum=0;
   valueDen=1;
   for (it=formula.begin() ; it!=formula.end() ; ++it)
   {
      if (it->TermType() == TY_CONSTANT)
      {
         assert(numConst==0);
         numConst++;
         valueNum = it->ValueNum();
         valueDen = it->ValueDen();
      } else
         notConst = 1;
   }
   if (notConst)
      return (false);
   return (true);
}

coeff_t
ConstantTermOfFormula (GFSliceVal const &formula)
{
   if (formula.is_uninitialized())
   {
      fprintf(stderr, "WARNING: ConstantTermOfFormula, formula is uninitialized\n");
      return (0);
   }
   GFSliceVal::iterator it;
   for (it=formula.begin() ; it!=formula.end() ; ++it)
   {
      if (it->TermType()==TY_CONSTANT)
      {
         return (it->ValueNum()/it->ValueDen());
      }
   }
   return (0);  // no explicit constant term
}

bool
FormulaContainsRegister(GFSliceVal const &formula)
{
   if (formula.is_uninitialized())
   {
      fprintf(stderr, "WARNING: FormulaContainsRegister, formula is uninitialized\n");
      return (false);
   }
   GFSliceVal::iterator it;
   for (it=formula.begin() ; it!=formula.end() ; ++it)
   {
      if (it->TermType()==TY_REGISTER)  // should I count TY_LOADs as registers as well?
      {
         return (true);
      }
   }
   return (false);
}

bool
FormulaContainsReferenceTerm(GFSliceVal const &formula)
{
   if (formula.is_uninitialized())
   {
      fprintf(stderr, "WARNING: FormulaContainsReferenceTerm, formula is uninitialized\n");
      return (false);
   }
   GFSliceVal::iterator it;
   for (it=formula.begin() ; it!=formula.end() ; ++it)
   {
      if (it->TermType()==TY_REFERENCE) 
      {
         return (true);
      }
   }
   return (false);
}

/* Check if the formula represents a scalar address relative to the stack pointer.
 * The formula must contain the stack pointer register and at most a
 * constant offset. No other terms are permitted.
 */
bool
FormulaIsStackReference(GFSliceVal const &formula, coeff_t& offset)
{
   if (formula.is_uninitialized())
   {
      fprintf(stderr, "WARNING: FormulaIsStackReference, formula is uninitialized\n");
      return (false);
   }
   GFSliceVal::iterator it;
   bool has_sp = false;
   offset = 0;
   for (it=formula.begin() ; it!=formula.end() ; ++it)
   {
      if (it->TermType()==TY_REGISTER)
      {
         if (is_stack_pointer_register(it->Register()))
            has_sp = true;
         else
            return (false);
      } else
      if (it->TermType() == TY_CONSTANT)
      {
         assert (offset == 0);
         offset = it->ValueNum();
      } else
         return (false);
   }
   return (has_sp);
}

bool
FormulaHasComplexDenominator (GFSliceVal const &formula)
{
   if (formula.is_uninitialized())
   {
      fprintf(stderr, "WARNING: FormulaHasComplexDenominator, formula is uninitialized\n");
      return (false);
   }
   GFSliceVal::iterator it;
   for (it=formula.begin() ; it!=formula.end() ; ++it)
   {
      coeff_t valueNum = it->ValueNum();
      ucoeff_t valueDen = it->ValueDen();
      if (it->TermType()!=TY_CONSTANT && valueNum!=0 && valueDen!=1)
      {
         assert (valueDen != 0);
         ucoeff_t gcd = greatestCommonDenominator(abs_coeff(valueNum), valueDen);
         if (gcd != valueDen)
            return (true);
      }
   }
   return (false);
}

bool
FormulaDefinedBeforePC(GFSliceVal const &formula, addrtype to_pc)
{
   if (formula.is_uninitialized())
   {
      fprintf(stderr, "WARNING: FormulaDefinedBeforePC, formula is uninitialized\n");
      return (false);
   }
   GFSliceVal::iterator it;
   for (it=formula.begin() ; it!=formula.end() ; ++it)
   {
      if (it->TermType()!=TY_CONSTANT && it->Address()<to_pc)
         return (true);
   }
   return (false);
}

bool
FormulaDefinedAfterPC(GFSliceVal const &formula, addrtype end_pc)
{
   if (formula.is_uninitialized())
   {
      fprintf(stderr, "WARNING: FormulaDefinedAfterPC, formula is uninitialized\n");
      return (false);
   }
   GFSliceVal::iterator it;
   for (it=formula.begin() ; it!=formula.end() ; ++it)
   {
      if (it->TermType()!=TY_CONSTANT && it->Address()>=end_pc)
         return (true);
   }
   return (false);
}

/* Routines to save and restore symbolic formulas.
 */
static int
saveRefFormulas(void* arg0, addrtype key, void* value)
{
   FILE* fd = (FILE *)arg0;
   RefFormulas *rf = static_cast<RefFormulas*>((void*)(*(addrtype*)value));
   fwrite(&key, sizeof(key), 1, fd);
   fd << (rf->base);
   int32_t nstrides = rf->strides.size();
   fwrite(&nstrides, sizeof(nstrides), 1, fd);
   FormulaVector::iterator vit = rf->strides.begin();
   for ( ; vit!=rf->strides.end() ; ++vit)
      fd << (*vit);
   return (0);
}

void
InitializeSaveStaticAnalysisData (FILE *fd, uint32_t module_check_sum)
{
   uint64_t dummy_offset = -1;
   fwrite(MIAMI::persistent_db_magic_word, sizeof(char), 8, fd);
   fwrite(&MIAMI::persistent_db_version, sizeof(int32_t), 1, fd);
   
   fwrite(&module_check_sum, sizeof(module_check_sum), 1, fd);
   fwrite(&dummy_offset, sizeof(dummy_offset), 1, fd);
}

// return the offset into the file where I save these formulas
// save formulas separately for each routine ...
uint64_t
SaveStaticAnalysisData (FILE *fd, RFormulasMap& refFormulas)
{
   uint64_t crt_offset = ftell (fd);
   
   int32_t rsize = refFormulas.size();
   fwrite (&rsize, sizeof(rsize), 1, fd);
   refFormulas.map (saveRefFormulas, fd);
   
   return (crt_offset);
}

void
FinalizeSaveStaticAnalysisData (FILE *fd, AddrOffsetMap *routines_table)
{
   int l;
   uint64_t crt_offset = ftell (fd);

   int32_t rsize = routines_table->size();
   fwrite (&rsize, sizeof(rsize), 1, fd);
   
   AddrOffsetMap::iterator uit = routines_table->begin ();
   for ( ; uit!=routines_table->end() ; ++uit)
   {
      addrtype pc = uit->first;
      uint64_t r_offset = uit->second;
      fwrite (&pc, sizeof(pc), 1, fd);
      fwrite (&r_offset, sizeof(r_offset), 1, fd);
   }

   l = fseek (fd, 8*sizeof(char)+sizeof(int32_t)+sizeof(uint32_t), SEEK_SET);
   assert (l == 0);
   fwrite (&crt_offset, sizeof(crt_offset), 1, fd);
}

void
InitializeLoadStaticAnalysisData (FILE *fd, uint32_t module_check_sum, 
       AddrOffsetMap *routines_table)
{
   int l;
   int32_t rsize, i, db_ver;
   uint32_t checksum;
   uint64_t t_offset;
   char magic_word[9];
   routines_table->clear ();
   l = fseek (fd, 0, SEEK_SET);
   if (l < 0)
   {
      fprintf(stderr, "ERROR: Cannot move to the start of the formulas file.\n");
      return;
   }
   
   // parse and check the magic key word
   fread(&magic_word, sizeof(char), 8, fd);
   magic_word[8] = 0;
   if (strcmp(magic_word, MIAMI::persistent_db_magic_word))  // magic word does not match
   {
      fprintf(stderr, "ERROR: Incorrect persistent db file format. Magic word not found.\n");
      return;
   }
   
   // next check the format version
   fread (&db_ver, sizeof(int32_t), 1, fd);
   if (db_ver > persistent_db_version)
   {
      fprintf(stderr, "ERROR: DB file format is newer than the current version supported by the program. "
                  "File was generated by a newer version of the code, or the file format is invalid.\n");
      return;
   } else if (db_ver < persistent_db_min_required_version)
   {
      fprintf(stderr, "ERROR: DB file format is older than the minimum required version by the code. File will be ignored.\n");
      return;
   }
   
   // check the hash key
   fread (&checksum, sizeof(checksum), 1, fd);
   if (checksum != module_check_sum)
   {
      fprintf(stderr, "ERROR: The analyzed binary/library has been recompiled since the formulas file was generated or the file is not a valid formulas file. Starting with an empty database.\n");
      return;
   }
   
   // checksum is correct. Read the index table whose offset is right after
   // the checksum.
   fread (&t_offset, sizeof(t_offset), 1, fd);
   l = fseek (fd, t_offset, SEEK_SET);
   if (l < 0)
   {
      fprintf(stderr, "ERROR: Cannot move to the offset of the index table (%" PRIu64 ") inside the formulas file.\n",
             t_offset);
      return;
   }

   fread (&rsize, sizeof(rsize), 1, fd);
   for (i=0 ; i<rsize ; ++i)
   {
      addrtype pc;
      fread (&pc, sizeof(pc), 1, fd);
      l = fread (&t_offset, sizeof(t_offset), 1, fd);
      if (l<=0)  // this is not good. Clear the entire table
      {
         routines_table->clear ();
         fprintf(stderr, "ERROR: The symbolic formulas file has an invalid format. Start with an empty database.\n");
         return;
      }
      routines_table->insert (AddrOffsetMap::value_type (pc, t_offset));
   }
   // if we are here, then all is good
}

void
LoadStaticAnalysisData (FILE *fd, uint64_t f_offset, RFormulasMap& refFormulas)
{
#define CHECK_COND(cond, err, ...) if (cond) \
   {fprintf(stderr, "ERROR: " err "\n", __VA_ARGS__); goto load_error; }
#define CHECK_COND0(cond, err) if (cond) \
   {fprintf(stderr, "ERROR: " err "\n"); goto load_error; }

   int32_t rsize, l, i, j;
   refFormulas.clear();
   l = fseek (fd, f_offset, SEEK_SET);
   CHECK_COND( l<0, "ERROR: Cannot move to offset %" PRIu64 " into the formulas file.\n", f_offset)
   
   // I have the base and then the stride formulas for one memory operand.
   // But first, I need to read the reference's key
   l = fread (&rsize, sizeof(rsize), 1, fd);
   CHECK_COND( l!=1, "reading the number of reference entries at file offset %" PRIu64, f_offset)
   
   for (i=0 ; i<rsize ; ++i)
   {
      addrtype key;
      int32_t nstrides;
      l = fread (&key, sizeof(key), 1, fd);
      CHECK_COND( l!=1, "reading the reference key for entry %d of %d", i, rsize)

      RefFormulas*& rf = refFormulas.atKey(key);
      CHECK_COND( rf!=0, "Found existing entry for memory operand with key %" PRIxaddr 
                ". Formula file is invalid. Do not trust any formulas for this routine.\n", key)
      rf = new RefFormulas();
      fd >> rf->base;
      l = fread (&nstrides, sizeof(nstrides), 1, fd);
      CHECK_COND( l!=1, "reading the number of stride formulas for reference with key %" PRIxaddr 
                ", entry %d of %d", key, i, rsize)
      CHECK_COND( nstrides<0, "the number of stride formulas %d for reference with key %" PRIxaddr 
                ", entry %d of %d, is negative", nstrides, key, i, rsize)
      if (nstrides > 0)
      {
         rf->strides.resize(nstrides);
         for (j=0 ; j<nstrides ; ++j)
            fd >> rf->strides[j];
      }
   }
   return;
   
load_error:
   // clear the refFormulas for this routine
   refFormulas.clear();
   return;
}

}  /* namespace MIAMI */
