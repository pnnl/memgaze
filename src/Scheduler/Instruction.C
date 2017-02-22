/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: Instruction.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Object associated with an instruction class, incuding its
 * execution templates read from a machine file.
 */

#include <string.h>
#include <algorithm>
#include "Instruction.h"
#include "instr_bins.H"
#include "instr_info.H"
#include "InstTemplate.h"
#include "debug_scheduler.h"

using namespace MIAMI;
namespace MIAMI {
extern int haveErrors;
}

void 
Instruction::addDescription(Position& _pos, ITemplateList& itList, UnitCountList *_asyncUsage)
{
   if (flags & I_DEFINED)
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: File %s (%d, %d): Instruction %s (%d) has been defined before at (%d, %d)\n",
            haveErrors, _pos.FileName(), _pos.Line(), _pos.Column(), 
            Convert_InstrBin_to_string(iclass.type),
            iclass.type, position.Line(), position.Column());
      return;
   }
   flags |= I_DEFINED;
   position = _pos;
   numTemplates = itList.size();
   if (numTemplates == 0)
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: File %s (%d, %d): Instruction %s (%d) has been defined with no template\n",
            haveErrors, _pos.FileName(), _pos.Line(), _pos.Column(), 
            Convert_InstrBin_to_string(iclass.type), iclass.type);
      return;
   }
   templates = new InstTemplate* [numTemplates];
   ITemplateList::iterator itlit = itList.begin();
   int i = 0;
   for( ; itlit!=itList.end() ; ++itlit, ++i )
      templates[i] = *itlit;
   itList.clear();
   
   asyncUsage = _asyncUsage;

   // sort templates by length
   std::sort (templates, templates+numTemplates, AscendingInstTemplates());
}

/* return the length of the template of minimum latency;
 * eventually, if there are special cases defined for various consumers,
 * return the defined special latency for the consumerType specified.
 */
unsigned int
Instruction::getMinLatency(const InstructionClass &consumerType, int templateIdx)
{
   if ( !(flags & I_DEFINED) )  // instruction was not defined yet
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: Instruction %s (%d) is used, but has not been defined yet.\n",
            haveErrors, Convert_InstrBin_to_string(iclass.type), iclass.type);
      // If instruction was not defined, it will always print type UnknownOp because
      // the type is passed when the instruction is defined.
      return -1;
   }

   if (hasSpecialCases)
   {
      assert(!"Machine description language does not allow special cases yet.");
      return (0);
   }
#if VERBOSE_DEBUG_PALM
   DEBUG_PALM(1,
   std::cout <<"instruction: "<<Convert_InstrBin_to_string(iclass.type)<<" "<<iclass.type<<" "<<templates[templateIdx]->getLength()<<std::endl;
   )
#endif
   assert (templateIdx>=0 && templateIdx<(int)numTemplates);
   return (templates[templateIdx]->getLength());
}

/* return true if not all templates have the same length
 */
bool
Instruction::hasVariableLength ()
{
   if ( !(flags & I_DEFINED) )  // instruction was not defined yet
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: Instruction %s (%d) is used, but has not been defined yet.\n",
            haveErrors, Convert_InstrBin_to_string(iclass.type), iclass.type);
      // If instruction was not defined, it will always print type UnknownOp because
      // the type is passed when the instruction is defined.
      return -1;
   }

   return (templates[numTemplates-1]->getLength() != 
                templates[0]->getLength());
}


InstTemplate* 
Instruction::getTemplateWithIndex(unsigned int idx)
{
   if (idx<numTemplates)
      return (templates[idx]);
   else
      return NULL;
}

float
Instruction::getResourceScore (int numAllUnits, const int *unitsIndex, 
        float *scoreBuffer)
{
   int i;
   if ( !(flags & I_DEFINED) )  // instruction was not defined yet
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: Instruction %s (%d) is used, but has not been defined yet.\n",
            haveErrors, Convert_InstrBin_to_string(iclass.type), iclass.type);
      // If instruction was not defined, it will always print type UnknownOp because
      // the type is passed when the instruction is defined.
      return 0.f;
   }

   if (flags & I_COST_ANALYZED)  // instruction was analyzed already
   {
      return (resourceScore);
   }
   
   assert (numTemplates > 0);  // at least one template must be defined
   float factor = 1.0 / numTemplates;
   for (i=0 ; i<numAllUnits ; ++i)
      scoreBuffer[i] = 0.0;
   
   for (i=0 ; i<(int)numTemplates ; ++i)
   {
      MultiCycTemplate& iTemp = templates[i]->getTemplates();
      MultiCycTemplate::iterator mctit = iTemp.begin();
      for ( ; mctit!=iTemp.end() ; ++mctit)
         if ((*mctit)!=NULL)  // if non empty set
         {
            TEUList::iterator tit = (*mctit)->begin();
            for ( ; tit!=(*mctit)->end() ; ++tit )
            {
               TemplateExecutionUnit *teu = *tit;
               int baseIdx = unitsIndex[teu->index];
               int numUnits = 0;
               MIAMIU::UIArray setUnits;
               BitSet::BitSetIterator bitit(*(teu->unit_mask));
               while((bool)bitit)
               {
                  int elem = bitit;
                  setUnits.push_back(elem);
                  ++ numUnits;
                  ++bitit;
               }
               
               assert (numUnits > 0);
               float weight;
               if (teu->count == 0)
                  weight = factor;
               else
                  weight = factor * teu->count / numUnits;
               MIAMIU::UIArray::iterator uait = setUnits.begin();
               for ( ; uait!=setUnits.end() ; ++uait)
                  scoreBuffer[baseIdx+(*uait)] += weight;
               setUnits.clear();
            }
         }
   }
   
   /* find the maximum and minimum score; the final score = maximum score +
      0.1 * minimum score (of a used unit)
    */
   resourceScore = 0;
   float min_score = 0;
   assert (resourceUsage == NULL);
   resourceUsage = new float[numAllUnits];
   numResources = numAllUnits;
   for (i=0 ; i<numAllUnits ; ++i)
   {
      resourceUsage[i] = scoreBuffer[i];
      if (scoreBuffer[i] > resourceScore)
         resourceScore = scoreBuffer[i];
      if (scoreBuffer[i]>0 && (scoreBuffer[i]<min_score || min_score<0.0001))
         min_score = scoreBuffer[i];
   }
   resourceScore += min_score / 10;
   
   flags |= I_COST_ANALYZED;
   return (resourceScore);
}

float* 
Instruction::getResourceUsage (float* scoreBuffer, unsigned int numAllUnits, 
                const int* unitsIndex)
{
   unsigned int i;
   if ( !(flags & I_DEFINED) )  // instruction was not defined yet
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: Instruction %s (%d) is used, but has not been defined yet.\n",
            haveErrors, Convert_InstrBin_to_string(iclass.type), iclass.type);
      // If instruction was not defined, it will always print type UnknownOp because
      // the type is passed when the instruction is defined.
      return NULL;
   }

   if (flags & I_COST_ANALYZED)  // instruction was analyzed already
   {
      assert (numAllUnits == numResources);
      for (i=0 ; i<numResources ; ++i)
         scoreBuffer[i] = resourceUsage[i];
      return (scoreBuffer);
   }

   assert (numTemplates > 0);  // at least one template must be defined
   float factor = 1.0 / numTemplates;
   for (i=0 ; i<numAllUnits ; ++i)
      scoreBuffer[i] = 0.0;
   
   for (i=0 ; i<numTemplates ; ++i)
   {
      MultiCycTemplate& iTemp = templates[i]->getTemplates();
      MultiCycTemplate::iterator mctit = iTemp.begin();
      for ( ; mctit!=iTemp.end() ; ++mctit)
         if ((*mctit)!=NULL)  // if non empty set
         {
            TEUList::iterator tit = (*mctit)->begin();
            for ( ; tit!=(*mctit)->end() ; ++tit )
            {
               TemplateExecutionUnit *teu = *tit;
               int baseIdx = unitsIndex[teu->index];
               int numUnits = 0;
               MIAMIU::UIArray setUnits;
               BitSet::BitSetIterator bitit(*(teu->unit_mask));
               while((bool)bitit)
               {
                  int elem = bitit;
                  setUnits.push_back(elem);
                  ++ numUnits;
                  ++bitit;
               }
               
               assert (numUnits > 0);
               float weight;
               if (teu->count == 0)
                  weight = factor;
               else
                  weight = factor * teu->count / numUnits;
               MIAMIU::UIArray::iterator uait = setUnits.begin();
               for ( ; uait!=setUnits.end() ; ++uait)
                  scoreBuffer[baseIdx+(*uait)] += weight;
               setUnits.clear();
            }
         }
   }
   
   /* find the maximum and minimum score; the final score = maximum score +
      0.1 * minimum score (of a used unit)
    */
   resourceScore = 0;
   float min_score = 0;
   assert (resourceUsage == NULL);
   resourceUsage = new float[numAllUnits];
   numResources = numAllUnits;
   for (i=0 ; i<numAllUnits ; ++i)
   {
      resourceUsage[i] = scoreBuffer[i];
      if (scoreBuffer[i] > resourceScore)
         resourceScore = scoreBuffer[i];
      if (scoreBuffer[i]>0 && (scoreBuffer[i]<min_score || min_score<0.0001))
         min_score = scoreBuffer[i];
   }
   resourceScore += min_score / 10;

   flags |= I_COST_ANALYZED;
   return (scoreBuffer);
}

typedef std::multimap<float, int> FloatIntMap;

float* 
Instruction::getBalancedResourceUsage(float* sumBuffer, unsigned int bsize,
             const int* unitsIndex, unsigned int numInstances, 
             const RestrictionList* restrict, unsigned int asyncSize, float* asyncBuffer)
{
   unsigned int i;
   int j;
   if ( !(flags & I_DEFINED) )  // instruction was not defined yet
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: Instruction %s (%d) is used, but has not been defined yet.\n",
            haveErrors, Convert_InstrBin_to_string(iclass.type), iclass.type);
      // If instruction was not defined, it will always print type UnknownOp because
      // the type is passed when the instruction is defined.
      return NULL;
   }

   // in the balanced version of the function, I do not use precomputed
   // resource usage. The template to be used depends on the resources
   // used up to this point that are passed in sumBuffer. I will choose the
   // cheapest template at this point and allocate resources only for it.
   // This must be done iteratively for each instance, because the cost of
   // a template may change after I do allocation for the previous instances.
   // The cost of a template is defined as the cost of its most expensive 
   // needed unit. The cost of a unit is how many times that resource is 
   // used by previously allocated instructions.
   // Test also the restriction list when computing the template cost
   float* tempBuffer = new float[bsize];
   assert (numTemplates > 0);  // at least one template must be defined
   
   // iterate over all instances
   for (unsigned int idx = 0 ; idx<numInstances ; ++idx)
   {
      MIAMIU::PairArray bestUnits;
      int bestTemplate = -1;
      float bestCost = 0;
      float bestTotalCost = 0;
      
      // There may be many dimensions to the allocation process: 
      // - we can have multiple templates
      // - each template has a sequence of cycles
      // - each cycle may have a list of units
      // - for each unit I can choose from several equivalent units
      // On top of this I have restriction rules. I will not check the 
      // restrictions when the score of each individual unit is computed, but I 
      // will do it only after I selected the minimum score units for a template.
      // Thus, I may not get the best allocation, but it is simpler this way.
      for (i=0 ; i<numTemplates ; ++i)
      {
         MIAMIU::PairArray selectedUnits;
         float maxCost = 0;
         float totalCost = 0;
         memcpy(tempBuffer, sumBuffer, bsize*sizeof(float) );
         
         MultiCycTemplate& iTemp = templates[i]->getTemplates();
         MultiCycTemplate::iterator mctit = iTemp.begin();
         for ( ; mctit!=iTemp.end() ; ++mctit)
            if ((*mctit)!=NULL)     // if non empty set
            {
               TEUList::iterator tit = (*mctit)->begin();
               for ( ; tit!=(*mctit)->end() ; ++tit )
               {
//                  float extraCost = 0;
                  TemplateExecutionUnit *teu = *tit;
                  int baseIdx = unitsIndex[teu->index];
                  int count = teu->count;
                  int numUnits = 0;

                  FloatIntMap allCosts;
                  BitSet::BitSetIterator bitit(*(teu->unit_mask));
                  while((bool)bitit)
                  {
                     int elem = bitit;
                     allCosts.insert (FloatIntMap::value_type 
                               (tempBuffer[baseIdx+elem], elem));
                     ++numUnits;
                     ++bitit;
                  }
                  if (count==0)  // allocate all
                     count = numUnits;
                  // select the 'count' copies with minimum score
                  FloatIntMap::iterator fit = allCosts.begin();
                  for (j=0 ; fit!=allCosts.end() && j<count ; ++fit, ++j)
                  {
                     selectedUnits.push_back (MIAMIU::UIPair (teu->index, fit->second));
                     int arrayIdx = baseIdx + fit->second;
                     tempBuffer[arrayIdx] += 1.0;
                     totalCost += tempBuffer[arrayIdx];
                     if (tempBuffer[arrayIdx] > maxCost)
                        maxCost = tempBuffer[arrayIdx];
                  }
               }
            }


         // for all restriction that are active, check if they restrict the 
         // lower bound even more
         RestrictionList::const_iterator rlit = restrict->begin();
         for ( ; rlit!=restrict->end() ; ++rlit)
         {
            // if the rule is not really restrictive do not check it
            if ( ! (*rlit)->IsActive())
               continue;
            double sumUsage = 0;
            TEUList *uList = (*rlit)->UnitsList();
            TEUList::iterator teuit = uList->begin();
            for ( ; teuit!=uList->end() ; ++teuit)
            {
               TemplateExecutionUnit *teu = *teuit;
               int baseIdx = unitsIndex[teu->index];
               BitSet::BitSetIterator bitit(*(teu->unit_mask));
               while ((bool)bitit)
               {
                  int elem = bitit;
                  sumUsage += tempBuffer[baseIdx+elem];
                  ++ bitit;
               }
            }
            int maxC = (*rlit)->MaxCount();
            assert (maxC > 0);
            sumUsage /= maxC;
            if (sumUsage > maxCost)
               maxCost = sumUsage;  // (int)(ceil(sumUsage)+0.01);
         }
         
         // if this is the first template seen, or its maximum cost is less than
         // the best template so far, then this becomes the best template
         if ( (bestTemplate==-1) ||
              (bestCost>maxCost || (bestCost==maxCost &&
                                    bestTotalCost>totalCost)))
         {
            bestTemplate = i;
            bestCost = maxCost;
            bestTotalCost = totalCost;
            bestUnits = selectedUnits;
         }
      }

      assert (bestTemplate >= 0);
      // Now that we know the best template, record in sumBuffer the resources
      // it needs
      MIAMIU::PairArray::iterator pait = bestUnits.begin();
      for ( ; pait!=bestUnits.end() ; ++pait)
      {
         int baseIdx = unitsIndex[pait->first];
         sumBuffer[baseIdx+pait->second] += 1.0;
      }
   }
   delete[] tempBuffer;
   
   // Record also the usage of asynchronous resources
   if (asyncUsage)
   {
      assert (asyncSize>0 && asyncBuffer);
      UnitCountList::iterator ucit = asyncUsage->begin();
      for ( ; ucit!=asyncUsage->end() ; ++ucit)
      {
         assert (ucit->first>=0 && ucit->first<asyncSize);
         asyncBuffer[ucit->first] += ucit->second * numInstances;
      }
   }
   return (sumBuffer);
}


int 
Instruction::findAndUseTemplate (BitSet** sched_template, int schedLen, 
         int startPos, int maxExtraLatency, int startFromIdx, 
         const RestrictionList* restrict, const int* unitsIndex, 
         MIAMIU::PairList& selectedUnits, int &unit)
{
   /* do not use maxLatency information for now. I do not know how to compute
    * it in the caller.
    */
   int maxLatency = -1;
   if (maxExtraLatency >= 0)
      maxLatency = templates[0]->getLength() + maxExtraLatency;
   int i = startFromIdx;
   selectedUnits.clear();
   while (i<(int)numTemplates && (templates[i]->getLength()<=maxLatency
              || maxLatency==-1))
   {
      bool failed = false;

      MultiCycTemplate& iTemp = templates[i]->getTemplates();
      MultiCycTemplate::iterator mctit = iTemp.begin();
      int k = startPos;
      int cycle = 0;
      for ( ; mctit!=iTemp.end() && !failed ; ++mctit)
      {
         if ((*mctit)!=NULL)     // if non empty set
         {
            TEUList::iterator tit = (*mctit)->begin();
            for ( ; tit!=(*mctit)->end() && !failed ; ++tit )
            {
               TemplateExecutionUnit *teu = *tit;
               int baseIdx = unitsIndex[teu->index];
               int count = teu->count;
               int j = 0;
               BitSet::BitSetIterator bitit(*(teu->unit_mask));
               while((bool)bitit && (j<count || count==0))
               {
                  bool semifailed = false;
                  unsigned int elem = baseIdx + (unsigned)bitit;
                  if ( (*sched_template[k])[elem])
                  {
                     unit = elem;
                     if (count==0)  // needed all of them
                     {
                        failed = true;
                     }
                     else
                        semifailed = true;
                  }
                  
                  if (!failed && !semifailed)
                  {
                     // check if all active restriction rules hold
                     RestrictionList::const_iterator rlit = restrict->begin();
                     int rid = -1;
                     for ( ; rlit!=restrict->end() ; ++rlit, --rid)
                     {
                        // if the rule is not really restrictive do not check it
                        if ( ! (*rlit)->IsActive())
                           continue;
                        int sumUsed = 0;
                        TEUList *uList = (*rlit)->UnitsList();
                        TEUList::iterator teuit = uList->begin();
                        for ( ; teuit!=uList->end() ; ++teuit)
                        {
                           TemplateExecutionUnit *teu2 = *teuit;
                           int baseIdx2 = unitsIndex[teu2->index];
                           BitSet::BitSetIterator bitit2(*(teu2->unit_mask));
                           while ((bool)bitit2)
                           {
                              unsigned int elem2 = baseIdx2 + (unsigned)bitit2;
                              if (elem==elem2 || (*(sched_template[k]))[elem2])
                                 sumUsed += 1;
                              ++ bitit2;
                           }
                        }
                        int maxC = (*rlit)->MaxCount();
                        if (sumUsed > maxC)  // restriction does not hold
                        {
                           unit = rid;   // set unit to the restriction rule index (neg)
                           if (count==0)
                           {
                              failed = true;
                           }
                           else
                              semifailed = true;
                           break;
                        }
                     }
                  }

                  if (failed)
                     break;
                  
                  // if not semifailed either, allocate unit
                  if (!semifailed)
                  {
                     *(sched_template[k]) += elem;
                     selectedUnits.push_back (MIAMIU::UIPair (elem, cycle));
                     ++j;
                  }
                  ++bitit;
               }
               if (count>0 && j!=count)
               {
#if VERBOSE_DEBUG_SCHEDULER
                  DEBUG_SCHED (7, 
                     fprintf (stderr, "findAndUseTemplate, type = %d, count = %d, j = %d, teu->index = %d, unit = %d, baseIdx = %d\n",
                           iclass.type, count, j, teu->index, unit, baseIdx);
                  )
#endif
                  failed = true;
               }
            }
         }
         
         // advance in the schedule template on every cycle, even if template
         // is NULL
         cycle += 1;
         k += 1;
         if (k>=schedLen)
            k = 0;
      }
      
      if (! failed)   // found a matching template
         return (i);
      
      // else, this template did not succeed
      // clear resources allocated for this template up to this 
      // cycle, and try the next template
      MIAMIU::PairList::iterator plit = selectedUnits.begin();
      for ( ; plit!=selectedUnits.end() ; ++plit)
      {
         int ll = startPos + plit->second;
         while (ll>=schedLen)
            ll -= schedLen;
         *(sched_template[ll]) -= plit->first;  // deallocate resources
      }
      selectedUnits.clear();
      // try next template
      ++ i;
   }
   return (-1);
}

void 
Instruction::deallocateTemplate (BitSet** sched_template, int schedLen, 
               int startPos, MIAMIU::PairList& selectedUnits)
{
   MIAMIU::PairList::iterator plit = selectedUnits.begin();
   for ( ; plit!=selectedUnits.end() ; ++plit)
   {
      int ll = startPos + plit->second;
      while (ll>=schedLen)
         ll -= schedLen;
      *(sched_template[ll]) -= plit->first;  // deallocate resources
   }
   selectedUnits.clear();
}

TEUList* 
Instruction::getUnits (int templIdx, int offset)
{
   if (templIdx<0 || templIdx>=(int)numTemplates)
   {
      fprintf (stderr, "WARNING: Instruction::getUnits: Invalid template index\n");
      return NULL;
   }
   int tleng = templates[templIdx]->getLength();
   if (offset<0 || offset>=tleng)
   {
      fprintf (stderr, "WARNING: Instruction::getUnits: Invalid offset in template\n");
      return NULL;
   }
   
   MultiCycTemplate& iTemp = templates[templIdx]->getTemplates();
   MultiCycTemplate::iterator mctit = iTemp.begin();
   int k = 0;
   for ( ; mctit!=iTemp.end() && k<offset ; ++mctit, ++k);
   assert (mctit!=iTemp.end());
   return (*mctit);
}

Instruction::~Instruction()
{
   for (unsigned int i=0 ; i<numTemplates ; ++i)
      delete (templates[i]);
   delete[] templates;
   numTemplates = 0;
   flags = 0;
   if (resourceUsage)
      delete[] resourceUsage;
   if (asyncUsage)
      delete asyncUsage;
}

InstructionContainer::~InstructionContainer()
{
   int i, j, k;
   for (k=0 ; k<IB_TOP_MARKER ; ++k)
      for (i=0 ; i<ExecUnit_LAST ; ++i)
         for (j=0 ; j<ExecUnitType_LAST ; ++j)
         {
            InstructionStorage::iterator isit;
            for (isit = istore[k][i][j].begin() ; isit!=istore[k][i][j].end() ; ++isit)
            {
               if (isit->second)
                  delete (isit->second);
            }
         }
}

Instruction*
InstructionContainer::findMaxVector(InstrBin type, ExecUnit eu, int _vwidth, ExecUnitType eu_type, width_t _width)
{
   if (  type<0 || type>=IB_TOP_MARKER ||
         eu<0 || eu>=ExecUnit_LAST || 
         eu_type<0 || eu_type>=ExecUnitType_LAST)
      return (NULL);
   // if we receive a scalar unit style, we should chech if we have any 
   // vector style templates for this instruction type
   InstructionStorage& is = istore[type][ExecUnit_VECTOR][eu_type];
   if (eu==ExecUnit_SCALAR && is.empty())
      // no vector definitions
      return (find(type, eu, _vwidth, eu_type, _width));
   
   // iterate over templates in reverse order; stop at the first match
   InstructionStorage::reverse_iterator rit = is.rbegin();
   for ( ; rit!=is.rend() ; ++rit)
   {
      if (_width <= rit->first.second)  // found it
      {
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (4, 
            fprintf(stderr, "FindMaxVector found template with vwidth %d, width %d\n", 
                    rit->first.first, rit->first.second);
         )
#endif
         return (rit->second);
      }
   }
   
   // did not find any vector template for elements of requested bit width
   // return the regular template for this instruction type
   return (find(type, eu, _vwidth, eu_type, _width));
}

Instruction*
InstructionContainer::findMaxVector(const InstructionClass &iclass)
{
   if (!iclass.isValid())
      return (NULL);
   return (findMaxVector(iclass.type, iclass.eu_style, iclass.vec_width, iclass.eu_type, iclass.width));
}

Instruction*
InstructionContainer::find(InstrBin type, ExecUnit eu, int _vwidth, ExecUnitType eu_type, width_t _width)
{
   if (  type<0 || type>=IB_TOP_MARKER ||
         eu<0 || eu>=ExecUnit_LAST || 
         eu_type<0 || eu_type>=ExecUnitType_LAST)
      return (NULL);
   InstWidth iwidth(_vwidth, _width);
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (4, 
      fprintf(stderr, "Search for vwidth %d, bwidth %d\n", _vwidth, _width);
   )
#endif
   InstructionStorage& is = istore[type][eu][eu_type];
   InstructionStorage::iterator isit = is.lower_bound(iwidth);
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (4, 
      if (isit!=is.end())
         fprintf(stderr, "Search result: vwidth %d, bwidth %d\n", isit->first.first, isit->first.second);
   )
#endif
   if (isit!=is.end() && iwidth.first<=isit->first.first)
   {
      // found a vector width at least as large. Now make sure that we find the right width element;
      if (iwidth.second <= isit->first.second)
         return (isit->second);
      else
      {
         for (++isit ; isit!=is.end() ; ++isit)
            if (iwidth.first<=isit->first.first && iwidth.second<=isit->first.second)
            {
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (4, 
                  fprintf(stderr, "Final search result: vwidth %d, bwidth %d\n", isit->first.first, isit->first.second);
               )
#endif
               return (isit->second);
            }
      }
   }
   return (NULL);
}

Instruction*
InstructionContainer::find(const InstructionClass &iclass)
{
   if (!iclass.isValid())
      return (NULL);
   return (find(iclass.type, iclass.eu_style, iclass.vec_width, iclass.eu_type, iclass.width));
#if 0
   InstWidth iwidth(iclass.vec_width, iclass.width);
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (4, 
      fprintf(stderr, "Search for vwidth %d, bwidth %d\n", iclass.vec_width, iclass.width);
   )
#endif
   InstructionStorage& is = istore[iclass.type][iclass.eu_style][iclass.eu_type];
   InstructionStorage::iterator isit = is.lower_bound(iwidth);
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (4, 
      if (isit!=is.end())
         fprintf(stderr, "Search result: vwidth %d, bwidth %d\n", isit->first.first, isit->first.second);
   )
#endif
   if (isit!=is.end() && iwidth.first<=isit->first.first)
   {
      // found a vector width at least as large. Now make sure that we find the right width element;
      if (iwidth.second <= isit->first.second)
         return (isit->second);
      else
      {
         for (++isit ; isit!=is.end() ; ++isit)
            if (iwidth.first<=isit->first.first && iwidth.second<=isit->first.second)
            {
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (4, 
                  fprintf(stderr, "Final search result: vwidth %d, bwidth %d\n", isit->first.first, isit->first.second);
               )
#endif
               return (isit->second);
            }
      }
   }
   return (NULL);
#endif
}

Instruction*
InstructionContainer::insert(InstrBin type, ExecUnit eu, int _vwidth, ExecUnitType eu_type, width_t _width)
{
   if (  type<0 || type>=IB_TOP_MARKER ||
         eu<0 || eu>=ExecUnit_LAST || 
         eu_type<0 || eu_type>=ExecUnitType_LAST)
      return (NULL);
   InstWidth iwidth(_vwidth, _width);
   InstructionStorage::iterator isit = istore[type][eu][eu_type].find(iwidth);
   if (isit!=istore[type][eu][eu_type].end())
      return (isit->second);
   else
   {
      Instruction *newInst = new Instruction(type, eu, _vwidth, eu_type, _width);
      istore[type][eu][eu_type].insert(InstructionStorage::value_type(iwidth, newInst));
      numInstructions += 1;
      return (newInst);
   }
}

Instruction*
InstructionContainer::insert(const InstructionClass &iclass)
{
   if (!iclass.isValid())
      return (NULL);
   return (insert(iclass.type, iclass.eu_style, iclass.vec_width, iclass.eu_type, iclass.width));
#if 0   
   InstWidth iwidth(iclass.vec_width, iclass.width);
   InstructionStorage::iterator isit = 
           istore[iclass.type][iclass.eu_style][iclass.eu_type].find(iwidth);
   if (isit!=istore[iclass.type][iclass.eu_style][iclass.eu_type].end())
      return (isit->second);
   else
   {
      Instruction *newInst = new Instruction(iclass);
      istore[iclass.type][iclass.eu_style][iclass.eu_type].insert(
                InstructionStorage::value_type(iwidth, newInst));
      numInstructions += 1;
      return (newInst);
   }
#endif
}

Instruction* 
InstructionContainer::addDescription (const InstructionClass &iclass, Position& _pos, 
           ITemplateList& itList, UnitCountList *asyncUsage)
{
   Instruction *inst = insert(iclass);
   if (inst == NULL)
   {
      haveErrors += 1;
      fprintf(stderr, "Error %d: File %s (%d, %d): trying to add description for instruction with invalid attributes: %s (%d), %s (%d), %s (%d), %" PRIwidth "\n",
            haveErrors, _pos.FileName(), _pos.Line(), _pos.Column(), 
            Convert_InstrBin_to_string((InstrBin)iclass.type), iclass.type, 
            ExecUnitToString(iclass.eu_style), iclass.eu_style,
            ExecUnitTypeToString(iclass.eu_type), iclass.eu_type,
            iclass.width);
   } else
      inst->addDescription(_pos, itList, asyncUsage);
   return (inst);
}


InstructionContainer::InstructionBinIterator::InstructionBinIterator(InstructionContainer* _ic)
{
   ic = _ic;
   valid = false;
   for (int ib=0 ; ib<IB_TOP_MARKER && !valid ; ++ib)
      for (int eu = 0 ; eu<ExecUnit_LAST && !valid ; ++eu)
         for (int eut = 0 ; eut<ExecUnitType_LAST && !valid ; ++eut)
            if (!ic->istore[ib][eu][eut].empty())
            {
               valid = true;
               iter = ic->istore[ib][eu][eut].begin();
               crt_bin = (InstrBin)ib;
               crt_eu = (ExecUnit)eu;
               crt_eut = (ExecUnitType)eut;
               break;
            }
}


void
InstructionContainer::InstructionBinIterator::operator++ ()
{
   // works only on a valid existing iterator
   if (valid)
   {
      assert (iter != ic->istore[crt_bin][crt_eu][crt_eut].end());
      ++ iter;
      if (iter != ic->istore[crt_bin][crt_eu][crt_eut].end())
         return;  // this is a valid state
         
      // if we are at the end of this InstructionStorage map
      // then I need to find the next valid instruction class
      valid = false;  // mark the iterator as invalid until we find a 
                      // new good position
      int ib = crt_bin;
      int eu = crt_eu;
      int eut = crt_eut+1;
      for ( ; ib<IB_TOP_MARKER && !valid ; ++ib)
      {
         for ( ; eu<ExecUnit_LAST && !valid ; ++eu)
         {
            // try the next ExecUnitType value
            for ( ; eut<ExecUnitType_LAST && !valid ; ++eut)
               if (!ic->istore[ib][eu][eut].empty())
               {
                  valid = true;
                  iter = ic->istore[ib][eu][eut].begin();
                  crt_bin = (InstrBin)ib;
                  crt_eu = (ExecUnit)eu;
                  crt_eut = (ExecUnitType)eut;
                  break;
               }
            eut = 0;
         }
         eu = 0;
      }
   }
}


InstructionClass 
InstructionContainer::InstructionBinIterator::getIClass() const
{
   InstructionClass ic;
   if (valid)
      ic.Initialize(crt_bin, crt_eu, iter->first.first, crt_eut, iter->first.second);
   else
      ic.Initialize(IB_unknown, ExecUnit_LAST, 0, ExecUnitType_LAST, 0);
   return (ic);
}


Instruction* 
InstructionContainer::InstructionBinIterator::operator-> () const
{
   if (valid)
      return (iter->second);
   else
      return (NULL);
}

InstructionContainer::InstructionBinIterator::operator Instruction* () const
{
   if (valid)
      return (iter->second);
   else
      return (NULL);
}
