/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: uipair.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Several commonly used, template based, data types.
 */

#ifndef _UI_PAIR_H
#define _UI_PAIR_H

#include <vector>
#include <map>
#include <set>
#include <list>

#include "generic_pair.h"

namespace MIAMIU
{

typedef GenericPair<unsigned int, unsigned int> UIPair;
//typedef GenericPair<unsigned int, unsigned long long> ULLPair;
//typedef GenericPair<unsigned int, int> IntPair;
//typedef GenericPair<unsigned int, char*> CharPPair;

typedef std::set <UIPair, UIPair::OrderPairs> UIPairSet;
typedef std::map <UIPair,  unsigned int, UIPair::OrderPairs> PairUiMap;
typedef std::map <UIPair,  unsigned long long, UIPair::OrderPairs> PairULLMap;
typedef std::map <unsigned int, unsigned int> UiUiMap;
typedef std::map <unsigned int, long> UiLongMap;
typedef std::map <unsigned int, unsigned long long> UiUllMap;
typedef std::map <unsigned int, double> UiDoubleMap;
//typedef std::map <unsigned int, double*> UiDoublePMap;
typedef std::map <unsigned int, const char*> UiCharPMap;
typedef std::map <long, unsigned int> LongUiMap;

typedef std::map <UIPair, double, UIPair::OrderPairs> PairDoubleMap;
typedef std::map <UIPair, double*, UIPair::OrderPairs> PairDoublePMap;

typedef std::vector<UIPair> PairArray;
typedef std::vector<unsigned int> UIArray;
typedef std::vector<unsigned long long> ULLArray;
typedef std::vector<float> FloatArray;
//typedef std::vector<ULLPair> LongPairArray;

typedef std::list<unsigned int> UIList;
typedef std::list<UIPair> PairList;
//typedef std::list<CharPPair> CharPList;

// add a new entry XsCOPES_INDEX which is an extended SCOPES_INDEX that has 
// an additional unsigned int for the binary pc address. Keep the old type as
// well to maintain compatibility with old recovery files.
typedef enum 
{ BLOCKS_MAPPING, CALLS_MAPPING, COUNTER_FORMULAS, INSTRUMENTED_CALLS, 
  EDGE_FORMULAS, LOAD_STORE_MAPPING, SCOPES_INDEX, LOOP_SETS, XsCOPES_INDEX,
  SETS_NAMES
} FileParts;

typedef std::set<unsigned int> UISet;
typedef std::set<unsigned long> ULSet;
typedef std::map<unsigned int, UISet> Ui2SetMap;
typedef std::map<unsigned int, UISet*> Ui2PSetMap;

typedef std::map<unsigned int, unsigned int*> UiPMap;
typedef std::map<unsigned int, unsigned long long*> UiPllMap;

typedef std::map<unsigned int, UiUllMap*> UiPMapMap;
typedef std::map<unsigned int, UiUiMap> UiUMapMap;

typedef std::map<unsigned int, PairArray> UiPAMap;
typedef std::map<unsigned int, UIPair> UiPairMap;

#if 0
class UITrio
{
public:
   UITrio(unsigned int _first=0, unsigned int _second=0, 
                  unsigned int _third=0) :
        first(_first), second(_second), third(_third)
   {}
   
   UITrio(const UITrio& ut)
   {
      first=ut.first; second=ut.second; third=ut.third;
   }
   
   UITrio& operator= (const UITrio& ut)
   {
      first=ut.first; second=ut.second; third=ut.third;
      return (*this);
   }
   
   unsigned int first;
   unsigned int second;
   unsigned int third;

  class OrderTrio {
  public :
      int operator() (const UITrio& te1, const UITrio& te2) const
      {
         if (te1.first < te2.first)
            return true;
         if (te1.first > te2.first)
            return false;
      
         if (te1.second < te2.second)
            return true;
         if (te1.second > te2.second)
            return false;
         
         return (te1.third < te2.third);
      }
  };
};

typedef std::vector<UITrio> TrioArray;

static 
bool compare_trio(const UITrio& te1, const UITrio& te2)
{
   if (te1.first < te2.first)
      return true;
   if (te1.first > te2.first)
      return false;

   if (te1.second < te2.second)
      return true;
   if (te1.second > te2.second)
      return false;
   
   return (te1.third < te2.third);
}

typedef std::map <UITrio, double*, UITrio::OrderTrio> TrioDoublePMap;
#endif

class UIQuatro
{
public:
   UIQuatro (unsigned int _first=0, unsigned int _second=0, 
             unsigned int _third=0, unsigned int _fourth=0) :
        first(_first), second(_second), third(_third), fourth(_fourth)
   {}
   
   UIQuatro (const UIQuatro& uq)
   {
      first=uq.first; second=uq.second; third=uq.third; fourth=uq.fourth;
   }
   
   UIQuatro& operator= (const UIQuatro& uq)
   {
      first=uq.first; second=uq.second; third=uq.third; fourth=uq.fourth;
      return (*this);
   }
   
   unsigned int first;
   unsigned int second;
   unsigned int third;
   unsigned int fourth;
};

typedef std::vector<UIQuatro> QuatroArray;

static 
bool compare_quatro (const UIQuatro& qe1, const UIQuatro& qe2) 
{
   if (qe1.first < qe2.first)
      return true;
   if (qe1.first > qe2.first)
      return false;

   if (qe1.second < qe2.second)
      return true;
   if (qe1.second > qe2.second)
      return false;

   if (qe1.third < qe2.third)
      return true;
   if (qe1.third > qe2.third)
      return false;
      
   return (qe1.fourth < qe2.fourth);
}

}  /* namespace MIAMIU */
  
#endif
