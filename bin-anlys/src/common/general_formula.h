/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: general_formula.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A generic math formula template with parameterized operand
 * types.
 */

#ifndef _GENERAL_FORMULA_H
#define _GENERAL_FORMULA_H

#include <assert.h>
#include <set>
#include <iostream>
#include <stdio.h>
#include <string>

#include "uipair.h"
#include "miami_types.h"
#include "miami_containers.h"

#include "printable_class.h"
#include "loadable_class.h"

namespace MIAMI 
{

#define UNINITIALIZED_FORMULA 0x3baaccdd
#define GUARANTEED_INDIRECT  ((addrtype)-3)

#define ACCESS_STRIDED 0
#define ACCESS_INDIRECT 1
#define ACCESS_IRREGULAR 2


template<class VarType,
         class Compare,
         class NeutralType,
         const NeutralType &neutralValue >
class GeneralFormula : public PrintableClass, public LoadableClass
{
public:
   typedef Compare compare_type;
   typedef std::set<VarType, compare_type> VTSet;
   typedef typename VTSet::const_iterator iterator;
   
   GeneralFormula() : PrintableClass(), LoadableClass()
   {
      _size = 0;
      _loaded = UNINITIALIZED_FORMULA;
      _formula_access_mode = 0;
      _marker = 0;
   }

   GeneralFormula(const GeneralFormula& gf)
   {
      _vtset = gf._vtset;
      _size = gf._size;
      _loaded = gf._loaded;
      _formula_access_mode = gf._formula_access_mode;
      _marker = gf._marker;
      _indirect_pcs = gf._indirect_pcs;
   }

   GeneralFormula(const VarType& vt)
   {
      if (vt == neutralValue)
      {
         _size = 0;
      } else
      {
         _vtset.insert(vt);
         _size = 1;
      }
      _loaded = 0;
      _marker = 0;
      _formula_access_mode = 0;
   }

   ~GeneralFormula()
   {
      _vtset.clear();
      _size = 0;
      _loaded = UNINITIALIZED_FORMULA;
      _formula_access_mode = 0;
      _marker = 0;
      _indirect_pcs.clear();
   }
   
   int SaveToFile(FILE* fd) const
   {
      // Some terms can be zero. So instead of just saving the formulas as
      // they are, I will check how many non-zero terms there actually are.
      unsigned int newSize = 0;
      typename VTSet::const_iterator it;
      for (it=_vtset.begin() ; it!=_vtset.end() ; ++it)
      {
         if (! it->isNil())
            newSize += 1;
      }
      assert (newSize <= _size);
      fwrite(&newSize, 4, 1, fd);
      for (it=_vtset.begin() ; it!=_vtset.end() ; ++it)
      {
         if (!it->isNil())
            fd << (*it);
      }
      fwrite(&_formula_access_mode, 4, 1, fd);
      return 0;
   }

   int LoadFromFile(FILE* fd)
   {
      _vtset.clear();
      int newSize;
      _size = 0;
      if (fread(&newSize, 4, 1, fd) < 1)
         newSize = 0;
      for (int i=0 ; i<newSize ; ++i)
      {
         VarType val;
         fd >> val;
         if (!val.isNil())
         {
            _vtset.insert (val);
            ++ _size;
         }
      }
      if (fread(&_formula_access_mode, 4, 1, fd) < 1)
         _formula_access_mode = 0;
      _loaded = 1;
      _marker = 0;
      return 0;
   }
   
   GeneralFormula& operator= (const GeneralFormula& gf)
   {
      _vtset = gf._vtset;
      _size = gf._size;
      _loaded = gf._loaded;
      _formula_access_mode = gf._formula_access_mode;
      _marker = gf._marker;
      _indirect_pcs = gf._indirect_pcs;
      return (*this);
   }
   
   GeneralFormula operator+ (const GeneralFormula& gf) const
   {
      if (gf._size == 0)
         return (*this);
      if (_size == 0)
         return (gf);
      typename VTSet::const_iterator itf;
      if (_size > gf._size)
      {
         GeneralFormula temp(*this);
         temp._loaded = 0;
         temp._marker = 0;
         if (_loaded==UNINITIALIZED_FORMULA || gf._loaded==UNINITIALIZED_FORMULA)
            temp._loaded = UNINITIALIZED_FORMULA;
         temp._formula_access_mode = _formula_access_mode | 
                   gf._formula_access_mode;
         if (! (temp._formula_access_mode & ACCESS_INDIRECT))
         {
            temp._indirect_pcs = _indirect_pcs;
            temp._indirect_pcs.insert(gf._indirect_pcs.begin(),
                       gf._indirect_pcs.end());
         }
         for (typename VTSet::const_iterator it=gf._vtset.begin() ; it!=gf._vtset.end() ; 
                         it++)
         {
            VarType val2 = *it;
            itf = temp._vtset.find(val2);
            if (itf != temp._vtset.end())  // found
            {
               VarType val1 = *itf;
               val1 += val2;
               temp._vtset.erase(itf);
               if (val1 == neutralValue)
               {
                  (temp._size)--;
               } else
               {
                  temp._vtset.insert(val1);
               }
            } else   // not found; add it as it is
            {
               temp._vtset.insert(val2);
               (temp._size)++;
            }
         }
         return (temp);
      } else
      {
         GeneralFormula temp(gf);
         temp._loaded = 0;
         temp._marker = 0;
         if (_loaded==UNINITIALIZED_FORMULA || gf._loaded==UNINITIALIZED_FORMULA)
            temp._loaded = UNINITIALIZED_FORMULA;
         temp._formula_access_mode = _formula_access_mode | 
                   gf._formula_access_mode;
         if (! (temp._formula_access_mode & ACCESS_INDIRECT))
         {
            temp._indirect_pcs = _indirect_pcs;
            temp._indirect_pcs.insert(gf._indirect_pcs.begin(),
                       gf._indirect_pcs.end());
         }
         for (typename VTSet::const_iterator it=_vtset.begin() ; it!=_vtset.end() ; it++)
         {
            VarType val2 = *it;
            itf = temp._vtset.find(val2);
            if (itf != temp._vtset.end())  // found
            {
               VarType val1 = *itf;
               val1 += val2;
               temp._vtset.erase(itf);
               if (val1 == neutralValue)
               {
                  (temp._size)--;
               } else
               {
                  temp._vtset.insert(val1);
               }
            } else   // not found; add it as it is
            {
               temp._vtset.insert(val2);
               (temp._size)++;
            }
         }
         return temp;
      }
   }
   
   GeneralFormula& operator+= (const GeneralFormula& gf)
   {
      typename VTSet::const_iterator itf;
      for (typename VTSet::const_iterator it=gf._vtset.begin() ; it!=gf._vtset.end() ; it++)
      {
         VarType val2 = *it;
         itf = _vtset.find(val2);
         if (itf != _vtset.end())  // found
         {
            VarType val1 = *itf;
            val1 += val2;
            _vtset.erase(itf);
            if (val1 == neutralValue)
            {
               _size--;
            } else
            {
               _vtset.insert(val1);
            }
         } else   // not found; add it as it is
         {
            _vtset.insert(val2);
            _size++;
         }
      } 
      if (_loaded==UNINITIALIZED_FORMULA || gf._loaded==UNINITIALIZED_FORMULA)
         _loaded = UNINITIALIZED_FORMULA;
      else
         _loaded = 0;
      _formula_access_mode |= gf._formula_access_mode;
      if (! (_formula_access_mode & ACCESS_INDIRECT))
      {
         _indirect_pcs.insert(gf._indirect_pcs.begin(),
                       gf._indirect_pcs.end());
      }
      
      return (*this);
   }
   
   GeneralFormula operator- (const GeneralFormula& gf) const
   {
      if (gf._size == 0)
         return (*this);
      GeneralFormula temp(*this);
      temp._loaded = 0;
      temp._marker = 0;
      if (_loaded==UNINITIALIZED_FORMULA || gf._loaded==UNINITIALIZED_FORMULA)
         temp._loaded = UNINITIALIZED_FORMULA;
      temp._formula_access_mode = _formula_access_mode | 
                   gf._formula_access_mode;
      if (! (temp._formula_access_mode & ACCESS_INDIRECT))
      {
         temp._indirect_pcs = _indirect_pcs;
         temp._indirect_pcs.insert(gf._indirect_pcs.begin(),
                    gf._indirect_pcs.end());
      }
      typename VTSet::const_iterator itf;
      for (typename VTSet::const_iterator it=gf._vtset.begin() ; it!=gf._vtset.end() ; ++it)
      {
         VarType val2 = *it;
         itf = temp._vtset.find(val2);
         if (itf != temp._vtset.end())  // found
         {
            VarType val1 = *itf;
            val1 -= val2;
            temp._vtset.erase(itf);
            if (val1 == neutralValue)
            {
               (temp._size)--;
            } else
            {
               temp._vtset.insert(val1);
            }
         } else   // not found; add it as it is
         {
            temp._vtset.insert(-val2);
            (temp._size)++;
         }
      } 
      return temp;
   }
   
   GeneralFormula& operator-= (const GeneralFormula& gf)
   {
      typename VTSet::const_iterator itf;
      for (typename VTSet::const_iterator it=gf._vtset.begin() ; it!=gf._vtset.end() ; ++it)
      {
         VarType val2 = *it;
         itf = _vtset.find(val2);
         if (itf != _vtset.end())  // found
         {
            VarType val1 = *itf;
            val1 -= val2;
            _vtset.erase(itf);
            if (val1 == neutralValue)
            {
               _size--;
            } else
            {
               _vtset.insert(val1);
            }
         } else   // not found; add it as it is
         {
            _vtset.insert(-val2);
            _size++;
         }
      } 
      if (_loaded==UNINITIALIZED_FORMULA || gf._loaded==UNINITIALIZED_FORMULA)
         _loaded = UNINITIALIZED_FORMULA;
      else
         _loaded = 0;
      _formula_access_mode |= gf._formula_access_mode;
      if (! (_formula_access_mode & ACCESS_INDIRECT))
      {
         _indirect_pcs.insert(gf._indirect_pcs.begin(),
                       gf._indirect_pcs.end());
      }
      return (*this);
   }

   GeneralFormula operator* (const coeff_t factor) const
   {
      GeneralFormula temp;
      temp._loaded = 0;
      temp._marker = 0;
      if (_loaded==UNINITIALIZED_FORMULA)
         temp._loaded = UNINITIALIZED_FORMULA;
      if (factor == 0)
         return (temp);
      for (typename VTSet::const_iterator it=_vtset.begin() ; it!=_vtset.end() ; ++it)
      {
         VarType val2 = *it;
         val2 *= factor;
         temp._vtset.insert(val2);
         (temp._size)++;
      } 
      temp._formula_access_mode = _formula_access_mode;
      temp._indirect_pcs = _indirect_pcs;
      return temp;
   }

   GeneralFormula& operator*= (const coeff_t factor)
   {
      if (_loaded!=UNINITIALIZED_FORMULA)
         _loaded = 0;
      if (factor == 0)
      {
         _vtset.clear();
         _size = 0;
         _formula_access_mode = 0;
         _indirect_pcs.clear();
         return (*this);
      }
      typename VTSet::const_iterator it=_vtset.begin();
      VTSet temp;
      while (it!=_vtset.end())
      {
         VarType val2 = *it;
         val2 *= factor;
         typename VTSet::const_iterator itf = it;
         it++;
         _vtset.erase(itf);
         temp.insert(val2);
      } 
      assert(_vtset.empty());
      assert(temp.size() == _size);
      _vtset = temp;
      return (*this);
   }
   
   GeneralFormula operator/ (const ucoeff_t factor) const
   {
      if (factor==1)
         return (*this);
      GeneralFormula temp;
      if (_loaded!=UNINITIALIZED_FORMULA)
         temp._loaded = 0;
      temp._formula_access_mode = _formula_access_mode;
      temp._indirect_pcs = _indirect_pcs;
      if (factor == 0)
      {
         std::cerr << "ERROR: GeneralFormula: Division by zero\n";
         return (temp);
      }
      for (typename VTSet::const_iterator it=_vtset.begin() ; it!=_vtset.end() ; ++it)
      {
         VarType val2 = *it;
         val2 /= factor;
         temp._vtset.insert(val2);
         (temp._size)++;
      } 
      return temp;
   }

   GeneralFormula& operator/= (const ucoeff_t factor)
   {
      if (factor==1)
         return (*this);
      if (factor == 0)
      {
         std::cerr << "ERROR: GeneralFormula: Division by zero\n";
         _vtset.clear();
         _size = 0;
         _loaded = UNINITIALIZED_FORMULA;
         return (*this);
      }
      typename VTSet::const_iterator it=_vtset.begin();
      VTSet temp;
      while (it!=_vtset.end())
      {
         VarType val2 = *it;
         val2 /= factor;
         typename VTSet::const_iterator itf = it;
         ++it;
         _vtset.erase(itf);
         temp.insert(val2);
      } 
      assert (_vtset.empty() && temp.size() == _size);
      _vtset = temp;
      if (_loaded!=UNINITIALIZED_FORMULA)
         _loaded = 0;
      return (*this);
   }
   
   inline void clear()
   {
      _vtset.clear();
      _size = 0;
      _loaded = 0;
      _marker = 0;
      _formula_access_mode = 0;
      _indirect_pcs.clear();
   }
   
   GeneralFormula& operator+= (const VarType& newVal)
   {
      typename VTSet::iterator itf;
      itf = _vtset.find(newVal);
      if (itf != _vtset.end())  // found
      {
         VarType val1 = *itf;
         val1 += newVal;
         _vtset.erase(itf);
         if (val1 == neutralValue)
         {
            _size--;
         } else
         {
            _vtset.insert(val1);
         }
      } else   // not found; add it as it is
      {
         _vtset.insert(newVal);
         _size++;
      }
      if (_loaded!=UNINITIALIZED_FORMULA)
         _loaded = 0;
      return (*this);
   }

   GeneralFormula& operator-= (const VarType& newVal)
   {
      typename VTSet::const_iterator itf;
      itf = _vtset.find(newVal);
      if (itf != _vtset.end())  // found
      {
         VarType val1 = *itf;
         val1 -= newVal;
         _vtset.erase(itf);
         if (val1 == neutralValue)
         {
            _size--;
         } else
         {
            _vtset.insert(val1);
         }
      } else   // not found; add it as it is
      {
         _vtset.insert(-newVal);
         _size++;
      }
      if (_loaded!=UNINITIALIZED_FORMULA)
         _loaded = 0;
      return (*this);
   }

   inline iterator begin() const
   {
      return (_vtset.begin());
   }
   
   inline iterator end() const
   {
      return (_vtset.end());
   }
   
   virtual void PrintObject (std::ostream &os) const
   {
      if (_loaded == UNINITIALIZED_FORMULA)
      {
         os << " UNINITIALIZED_FORMULA ";
      } else
      if (_vtset.empty())
      {
         os << " 0 ";
      } else
      {
         typename VTSet::const_iterator it = _vtset.begin();
         for ( ; it!=_vtset.end() ; ++it)
         {
            VarType val = *it;
            os << val << "+";
         }
      }
      if (_formula_access_mode & ACCESS_INDIRECT)
         os << "[HAS_INDIRECT]";
      else
         if (! _indirect_pcs.empty())
         {
            os << "[HAS_INDIRECT_MAYBE:";
            for (AddrIntSet::const_iterator sit=_indirect_pcs.begin() ; 
                  sit!=_indirect_pcs.end() ; ++sit)
               os << std::hex << sit->first << std::dec << "/" 
                  << sit->second << ", ";
            os << "]";
         }
      if (_formula_access_mode & ACCESS_IRREGULAR)
         os << "[HAS_IRREGULAR]";
//      os << endl;
   }
   
   bool is_uninitialized() const
   {
      return (_loaded == UNINITIALIZED_FORMULA);
   }
   
   int has_indirect_access() const
   {
      if (_formula_access_mode & ACCESS_INDIRECT)
         return (1);
      if (! _indirect_pcs.empty())
         return (2);
      return (0);
   }

   void set_indirect_access(addrtype _pc, int _idx)
   {
      if (_pc==GUARANTEED_INDIRECT || (_formula_access_mode&ACCESS_INDIRECT))
      {
         _formula_access_mode |= ACCESS_INDIRECT;
         _indirect_pcs.clear();
      } else
         _indirect_pcs.insert(AddrIntPair(_pc, _idx));
   }
   
   AddrIntSet& get_indirect_pcs()   { return (_indirect_pcs); }
   void set_indirect_pcs(AddrIntSet& uset)
   { 
      _indirect_pcs.insert(uset.begin(), uset.end()); 
   }

   int has_irregular_access() const
   {
      return (_formula_access_mode & ACCESS_IRREGULAR);
   }

   void set_irregular_access()
   {
      _formula_access_mode |= ACCESS_IRREGULAR;
   }
   
   void visit(int m) { _marker = m; }
   bool visited(int m) const  { return (_marker==m); }
   int getMarker() const      { return (_marker); }
   
   static int newMarker()  { 
      return (++top_marker);
   }
   
private:
   VTSet _vtset;
   unsigned int _size;
   int _loaded;
   int _formula_access_mode;
   int _marker;
   AddrIntSet _indirect_pcs;
   static int top_marker;
};

// initialize the static template member
template<class VarType, class Compare, class NeutralType, const NeutralType &neutralValue>
      int GeneralFormula<VarType, Compare, NeutralType, neutralValue>::top_marker = 0;

}  /* namespace MIAMI */

#endif
