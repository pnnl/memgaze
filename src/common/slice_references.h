/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: slice_references.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements data flow analysis. Slice on register names to 
 * compute symbolic formulas for memory reference addresses.
 */

#ifndef _SLICE_REFERENCE_H_
#define _SLICE_REFERENCE_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <string>

#include "uipair.h"
#include "miami_types.h"
#include "bucket_hashmap.h"
#include "register_class.h"

#ifdef NO_STD_CHEADERS
# include <stddef.h>
# include <stdlib.h>
#else
# include <cstddef> // for 'NULL'
# include <cstdlib>
using std::abort; // For compatibility with non-std C headers
using std::exit;
#endif

//using namespace std;
namespace MIAMI  // add everything to namespace MIAMI
{

typedef enum
{
  TY_UNKNOWN,
  TY_CALL,   // it is the result of a function call
  TY_REFERENCE,
  TY_STACK,
  TY_LOAD,
  TY_REGISTER,
  TY_CONSTANT,
  TY_LAST
} term_slice_type;

#define ANY_SUBTYPE  ((unsigned int)-1)


// a sliceVal object can be a int constant, a load, or a register value
// term_type specifies what type each object is
// value specifies the coefficient (how many times we have this term); it can
//     be only an integer
// pc, uop_idx identifies the address in the code where the value is created
//     - not used for constants
class sliceVal
{
public:
   
   sliceVal (coeff_t _value, term_slice_type _term_type, addrtype _pc = 1, int _uop_idx = 0,
            addrtype _auxinfo = 0, int _expand = 0) 
           : valueNum(_value), pc(_pc), auxinfo(_auxinfo), 
             uop_idx(_uop_idx), term_type(_term_type), expand(_expand)
   {
      valueDen = 1;
   }
   
   sliceVal (coeff_t _value, term_slice_type _term_type, addrtype _pc, int _uop_idx,
           const register_info& _reg, int _expand = 0) 
           : valueNum(_value), pc(_pc), reg(_reg),
             uop_idx(_uop_idx), term_type(_term_type), expand(_expand)
   {
      valueDen = 1;
   }
   
   sliceVal()
   {
      valueNum = 0; valueDen = 1;
      term_type = TY_UNKNOWN;
      pc = auxinfo = 0;
      uop_idx = 0;
      expand = 0;
   }
   
   sliceVal(const sliceVal& sv)
   {
      valueNum = sv.valueNum; valueDen = sv.valueDen;
      term_type = sv.term_type; pc = sv.pc;
      uop_idx = sv.uop_idx;
      auxinfo = sv.auxinfo; reg = sv.reg;
      expand = sv.expand;
   }
   
   inline sliceVal& operator= (const sliceVal& sv)
   {
      valueNum = sv.valueNum; valueDen = sv.valueDen;
      term_type = sv.term_type; pc = sv.pc;
      uop_idx = sv.uop_idx;
      auxinfo = sv.auxinfo; reg = sv.reg;
      expand = sv.expand;
      return (*this);
   }
   
   bool isNil () const  { return (valueNum==0); }

   sliceVal operator+ (const sliceVal& sv) const;
   sliceVal& operator+= (const sliceVal& sv);
   sliceVal operator- (const sliceVal& sv) const;
   sliceVal& operator-= (const sliceVal& sv);

   sliceVal operator* (coeff_t factor) const;
   sliceVal& operator*= (coeff_t factor);
   sliceVal operator/ (ucoeff_t factor) const;
   sliceVal& operator/= (ucoeff_t factor);
   
   inline bool operator== (coeff_t testValue) const
   {
      return (valueNum == (coeff_t)(testValue*valueDen));
   }

   // test if the two sliceVal's have the same type
   // we do not care about coefficient values in these tests
   bool operator== (const sliceVal& sv) const;
   bool operator!= (const sliceVal& sv) const;
   
   inline sliceVal operator- () const
   {
      sliceVal temp(*this);
      temp.valueNum = -(temp.valueNum);
      return (temp);
   }

   /* Do we need a multiply or divide operation? (does not make sense
    *   if it is not a constant).
    * Or just stop if such an instruction occurs?
    * And there are shifts also (multiply or divide with a constant)
    */
    
   std::string toString () const;
   void SaveToFile(FILE* fd) const;
   void LoadFromFile(FILE* fd);

   inline coeff_t ValueNum() const           { return (valueNum); }
   inline ucoeff_t ValueDen() const          { return (valueDen); }
   inline term_slice_type TermType() const   { return (term_type); }
   inline addrtype Address() const           { return (pc); }
   inline int UopIndex() const               { return (uop_idx); }
   inline unsigned int AuxInfo() const       { return (auxinfo); }
   inline const register_info& Register() const { return (reg); }
   inline int MustExpand() const             { return (expand); }
   inline double Value() const                
   { 
      return (((double)valueNum)/valueDen); 
   }
   inline bool IsDefined() const      { return (term_type!=TY_UNKNOWN); }
   
private:
   coeff_t valueNum;
   ucoeff_t valueDen;
   addrtype pc;
   addrtype auxinfo;
   register_info reg;
   int uop_idx;
   term_slice_type term_type : 24;
   int expand : 8;
};

extern std::ostream& operator<< (std::ostream& os, sliceVal& sv);
extern std::ostream& operator<< (std::ostream& os, const sliceVal& sv);

extern FILE* operator<< (FILE* fd, sliceVal& sv);
extern FILE* operator<< (FILE* fd, const sliceVal& sv);

extern FILE* operator>> (FILE* fd, sliceVal& sv);

class compareSliceVals
{
public:
   bool operator()(const sliceVal& sv1, const sliceVal& sv2) const
   {
      term_slice_type ty = sv1.TermType();
      if (ty != sv2.TermType())
         return (ty < sv2.TermType());
      
      if (ty == TY_CONSTANT)
         return (false);   // nothing else to test for constants
         
      // else (sv1.TermType() == sv2.TermType())
      // references and stack accesses are both loads from well defined 
      // locations (sort of constant). Compare the locations for these
      // terms. It does not matter the PC of the load.
      if (ty==TY_REFERENCE || ty==TY_STACK)
         return (sv1.AuxInfo() < sv2.AuxInfo());
      
      // Otherwise, should check the addresses where the (symbolic) values are defined
      addrtype sv1pc = sv1.Address();
      addrtype sv2pc = sv2.Address();
      if (sv1pc != sv2pc)
         return (sv1pc < sv2pc);
      
      int sv1idx = sv1.UopIndex();
      int sv2idx = sv2.UopIndex();
      if (sv1idx != sv2idx)
         return (sv1idx < sv2idx);
      
      // PCs are the same
      if (sv1.TermType()==TY_REGISTER || sv1.TermType()==TY_LOAD)
      {
         return (sv1.Register() < sv2.Register());
      } else
         return (false);
   }
};

} /* namespace MIAMI */

#include "general_formula.h"

namespace MIAMI
{

extern coeff_t defaultZeroCoeff;
typedef GeneralFormula<sliceVal, compareSliceVals, coeff_t, defaultZeroCoeff> GFSliceVal;
// GFSliceVal represents one symbolic formula which has multiple sliceVal terms

// Store formulas for all memory instructions into a hash map. For this, I will
// have a container class that holds a reference formula and a vector of stride
// formulas for easy access.

typedef std::vector<GFSliceVal> FormulaVector;
class RefFormulas
{
public:
   RefFormulas()
   {
   }
   
   size_t NumberOfStrides() const  { return (strides.size()); }

   GFSliceVal base;
   FormulaVector strides;
};

//typedef std::map<unsigned int, GFSliceVal> UiGFSVMap;

extern RefFormulas* defaultRFP;

// create an interface for accessing the reference formulas hash table
// One x86 instruction may have multiple memory operands and instruction can
// be 1 byte long. We must be able to refer to any of the operands.
// The hashmap key is a 64-bit integer. To create the key, we shift 
// the instruction's address left by 2 bits, and use the lowest 2 bits 
// for the operand index. No instruction has more than 4 memory operands.
// In fact, the maximum might be only 2 (or at most 3).
class RFormulasMap
{
   typedef MIAMIU::BucketHashMap<addrtype, RefFormulas*, &defaultRFP, 128> AddrRFPMap;

public:
   RFormulasMap(addrtype _base) : base_addr(_base)
   { }
   
   ~RFormulasMap() 
   {
      clear();
   }

   // returns the formulas for the given pc & memory operand index
   RefFormulas*& operator() (addrtype _pc, int idx)
   {
      addrtype key = ((_pc-base_addr)<<2) + (idx&3);
      return (storage[key]);
   }

   RefFormulas* hasFormulasFor (addrtype _pc, int idx)
   {
      addrtype key = ((_pc-base_addr)<<2) + (idx&3);
      RefFormulas *rf = 0;
      unsigned int hashv = storage.pre_hash(key);
      if (storage.is_member(key, hashv))
         rf = storage(key, hashv);
      return (rf);
   }

   RefFormulas*& atKey (addrtype key)
   {
      return (storage[key]);
   }
   
   void clear();
   
   int size() const  { return (storage.size()); }
   
   int map(AddrRFPMap::MapFunction func, void* arg0) const
   {
      return (storage.map(func, arg0));
   }
   
private:
   AddrRFPMap storage;
   addrtype base_addr;
};

typedef std::map<addrtype, uint64_t> AddrOffsetMap;

/* return -1, 0, or 1 is the coeff of the first term is negative, zero 
 * (formula is zero in this case), or positive
 */
int 
SignOfFormula(GFSliceVal const &formula);

bool
hasConstantFormula(GFSliceVal const &formula, coeff_t &valueNum, ucoeff_t &valueDen);

bool
IsConstantFormula(GFSliceVal const &formula, coeff_t &valueNum, ucoeff_t &valueDen);

bool
FormulasAreIdentical(GFSliceVal const &formula1, GFSliceVal const &formula2);

bool
FormulasAreEqual(GFSliceVal const &formula1, GFSliceVal const &formula2);

bool
HasIntegerRatio (GFSliceVal const &formula1, GFSliceVal const &formula2,
          coeff_t &factor1, coeff_t &factor2);

bool
HasIntegerRatioAndRemainder (GFSliceVal const &formula1, GFSliceVal const &formula2,
          coeff_t &ratio, coeff_t &remainder);

bool
FormulaContainsRegister (GFSliceVal const &formula);

bool
FormulaContainsReferenceTerm (GFSliceVal const &formula);

/* Check if the formula represents a scalar address relative to the stack pointer.
 * The formula must contain the stack pointer register and at most a
 * constant offset. No other terms are permitted.
 */
bool
FormulaIsStackReference(GFSliceVal const &formula, coeff_t& offset);

long
ConstantTermOfFormula (GFSliceVal const &formula);

bool
FormulaHasComplexDenominator (GFSliceVal const &formula);

bool
FormulaDefinedBeforePC (GFSliceVal const &formula, addrtype to_pc);

bool
FormulaDefinedAfterPC(GFSliceVal const &formula, addrtype end_pc);

//typedef enum {RF_REFADDRESS, RF_REFSTRIDE, RF_INDEX_TABLE} RegFormulaType;

void
InitializeSaveStaticAnalysisData (FILE *fd, uint32_t module_check_sum);

// return the offset into the file where I save these formulas
// save formulas separately for each routine ...
uint64_t
SaveStaticAnalysisData (FILE *fd, RFormulasMap& refFormulas);

void
FinalizeSaveStaticAnalysisData (FILE *fd, AddrOffsetMap *routines_table);

void
InitializeLoadStaticAnalysisData (FILE *fd, uint32_t module_check_sum, 
             AddrOffsetMap *routines_table);

void
LoadStaticAnalysisData (FILE *fd, uint64_t f_offset, RFormulasMap& refFormulas);


}  /* namespace MIAMI */

#endif
