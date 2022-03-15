/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: MemoryHierarchyLevel.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Stores information about a machine memory hierarchy level.
 */

#include "MemoryHierarchyLevel.h"
#include <stdlib.h>

using namespace MIAMI;

namespace MIAMI {
   float MemoryHierarchyLevel::defProbValue = -1.f;
}

static const double epsilon = 1e-5;
static const double negepsilon = -epsilon;

MemoryHierarchyLevel::MemoryHierarchyLevel ()
{
   num_blocks = MHL_UNLIMITED;
   block_size = MHL_UNLIMITED;
   entry_size = MHL_UNDEFINED;
   assoc = MHL_FULLY_ASSOCIATIVE;
   bdwth = MHL_UNDEFINED;
   num_sets = 1;
   rSets = 1.;
   longname = NULL;
   next_level = NULL;
   missPenalty = 0;
   hasMissInfo = false;
   mrd_idx = -1;   // negative value means no associated file.
   maxStoredExp = -1;
   sample_mask = 0;
   probCache = 0;
}

MemoryHierarchyLevel::~MemoryHierarchyLevel ()
{
   if (longname)
      free (longname);
   if (next_level)
      free (next_level);
   if (probCache)
      delete (probCache);
      
#if XXX
   TrioSIMap::iterator sia = setsInfo.begin ();
   for ( ; sia!=setsInfo.end () ; ++sia)
      delete (sia->second);
   setsInfo.clear ();
   
   ref2Set.clear ();
   ScopesFootPrint::iterator fpit = scopesFP.begin ();
   for ( ; fpit!=scopesFP.end() ; ++fpit)
      delete (fpit->second);
   scopesFP.clear ();
#endif
}

MemoryHierarchyLevel::MemoryHierarchyLevel (int _num_blocks, int _block_size, 
           int _entry_size, int _assoc, float _bdwth, char* _longname, 
           char *_next_level, int _missPenalty)
{
   num_blocks = _num_blocks;
   block_size = _block_size;
   entry_size = _entry_size;
   assoc = _assoc;
   bdwth = _bdwth;

   if (assoc == MHL_FULLY_ASSOCIATIVE)
      num_sets = 1;
   else
   {
      assert (assoc > 0);
      num_sets = num_blocks / assoc;
      assert (assoc*num_sets == num_blocks ||
              !"Associativity should divide exactly the number of blocks");
   }
   rSets = 1.0 / (double)num_sets;
   
   if (_longname == NULL)
      longname = _longname;
   else
      longname = strdup (_longname);
   if (_next_level == NULL)
      next_level = NULL;
   else
      next_level = strdup (_next_level);
   missPenalty = _missPenalty;
   hasMissInfo = false;
   mrd_idx = -1;   // negative value means no associated file.
   maxStoredExp = -1;
   sample_mask = 0;
   probCache = 0;
}

MemoryHierarchyLevel::MemoryHierarchyLevel (const MemoryHierarchyLevel& mhl)
{
   num_blocks = mhl.num_blocks;
   block_size = mhl.block_size;
   entry_size = mhl.entry_size;
   assoc = mhl.assoc;
   bdwth = mhl.bdwth;
   num_sets = mhl.num_sets;
   rSets = mhl.rSets;
   
   if (longname)
      free (longname);
   if (mhl.longname == NULL)
      longname = mhl.longname;
   else
      longname = strdup (mhl.longname);
   if (next_level)
      free (next_level);
   if (mhl.next_level == NULL)
      next_level = NULL;
   else
      next_level = strdup (mhl.next_level);
   missPenalty = mhl.missPenalty;
   position = mhl.position;
   hasMissInfo = false;
   mrd_idx = mhl.mrd_idx;
   maxStoredExp = -1;
   sample_mask = mhl.sample_mask;
   probCache = 0;
}

MemoryHierarchyLevel& 
MemoryHierarchyLevel::operator= (const MemoryHierarchyLevel& mhl)
{
   num_blocks = mhl.num_blocks;
   block_size = mhl.block_size;
   entry_size = mhl.entry_size;
   assoc = mhl.assoc;
   bdwth = mhl.bdwth;
   num_sets = mhl.num_sets;
   rSets = mhl.rSets;

   if (longname)
      free (longname);
   if (mhl.longname == NULL)
      longname = mhl.longname;
   else
      longname = strdup (mhl.longname);
   if (next_level)
      free (next_level);
   if (mhl.next_level == NULL)
      next_level = NULL;
   else
      next_level = strdup (mhl.next_level);
   missPenalty = mhl.missPenalty;
   position = mhl.position;
   hasMissInfo = false;   // I do not copy the miss counts info
   mrd_idx = mhl.mrd_idx;
   maxStoredExp = -1;
   sample_mask = mhl.sample_mask;
   probCache = 0;
   return (*this);
}

void 
MemoryHierarchyLevel::setNameAndPosition (const char* _longname, 
              const Position& _pos)
{
   if (longname==NULL && _longname!=NULL)
      longname = strdup (_longname);
   position = _pos;
}

double
MemoryHierarchyLevel::compute_pow (double base, unsigned int expon)
{
   double finalVal = 1.0;
   if (maxStoredExp < 0)
   {
      maxStoredExp = 0;
      pow_vals[0] = base;
   }
   int crtBit = 0;
   while (expon > 0)
   {
      if (crtBit>maxStoredExp) { 
         ++ maxStoredExp;
         if (pow_vals[maxStoredExp-1] < 1.e-150)
            pow_vals[maxStoredExp] = 0.0;
         else
            pow_vals[maxStoredExp] = pow_vals[maxStoredExp-1] * 
                     pow_vals[maxStoredExp-1];
      }
         
      int bitVal = expon & 1;
      if (bitVal)
      {
         if (pow_vals[crtBit] < 1.e-150)
            finalVal = 0.0;
         else
            finalVal *= pow_vals[crtBit];
      }
      expon >>= 1;
      ++ crtBit;
   }
   return (finalVal);
}

float
MemoryHierarchyLevel::computeProbability(unsigned int n /* distance */, 
      unsigned int k /* assoc */, int s /* numSets */, 
      double rs /* 1/numSets */)
{
   double ds = 1 - rs;  // (s-1)/s
   if (ds<epsilon)   // fully assoc cache
   {
      if (n<k)
         return (1.0);
      else
         return (0.0);
   }
   double p1 = 1.0;  // (1/s)^i * (n i) part
   double p2 = 0;
   if (n == 0)
      p2 = 1;
   else 
      if (ds != 0)
         p2 = compute_pow(ds, n);
   double sum = p2;
   n -= 1;
   for (unsigned int i=1 ; i<k && n>=0 ; i+=1, n-=1)
   {
      p1 *= (rs*(n+1) / i);
      if (n == 0)
         p2 = 1;
      else
         if (ds == 0)
            p2 = 0;
         else
            p2 = compute_pow(ds, n);
      sum += (p1*p2);
   }
            
   // sum is the probability of a hit in this set-assoc cache
   // Probability should be between 0 and 1
   if (sum<negepsilon || sum>(1.0+epsilon))
   {
      fprintf(stderr, "Probability for dist %u, assoc %u, numSets %d, equal %lf\n",
              n, k, s, sum);
      assert (sum >= 0.0 && sum <= 1.0);
   }
   if (sum < 0.0)
      sum = 0.0;
   if (sum > 1.0)
      sum = 1.0;
   return ((float)sum);
}

void 
MemoryHierarchyLevel::SetSampleMask(int _mask)
{
   sample_mask = _mask;
   // clear the probability cache if we change the sample mask
   if (probCache)
      probCache->clear();
   /* Must adjust cache parameters to account for effects of sampling. */
   /***** FIXME:old *******/
}

void 
MemoryHierarchyLevel::ComputeMissesForReuseDistance(float dist, 
                double& fullMiss, double& setMiss)
{
   // I have to deal with sampled reuse distance as well.
   // First write the case for no sampling, update afterwards
   fullMiss = (dist>=num_blocks?1.:0.);
   
   setMiss = fullMiss;
   if (assoc!=MHL_FULLY_ASSOCIATIVE && dist>=assoc)  // if distance < assoc, all accesses are hits even in set-assoc cache
   {
      // compute probability that an access with distance 'dist' in a 
      // fully associative cache will conflict with less than 'assoc'
      // other blocks into the same set
      double missProb = 1.0;
      if (dist <= 4e9)
      {
         if (!probCache)
            probCache = new ProbabilityTable();
         unsigned int idist = (unsigned int) (dist + 0.5);
         float& crtProb = probCache->insert(idist);
         if (crtProb < 0.f)   // it was not computed
            crtProb = computeProbability(idist, assoc, num_sets, rSets);
         missProb = 1.0 - crtProb;
      }
      setMiss = missProb;
   }
}

#if XXX
int
MemoryHierarchyLevel::parseCacheMissFile (const char* fileName, MIAMIU::PairUiMap &unkValues)
{
   FILE *fd;
   
   fd = fopen (fileName, "r");
   if (fd == NULL)
   {
      fprintf (stderr, "WARNING: Cannot open the file %s containing miss counts for level %s. Will continue without any miss count information for this level.\n",
             fileName, longname);
      return (-1);
   }
   // OLD: this file contains an unspecified number of records. Each record has
   // a number of labels, then the list of reference labels (in hex), then 
   // three floating point values representing the execution count, 
   // full-associative misses and the set-associativie misses.
   //
   // NEW: In the new version, this file has two sections separated by a line 
   // with two zeros. First section contains labels (pc address of references)
   // that are part of each set. Format is: setIndex numberOfLabels listLabels
   // Second part contains data about execution count and number of misses
   // for each set. Format is: setIndex reuseOn value1 value2. Reuse on info
   // represents a pc address in hexa which determines the scope that have
   // accessed memory blocks before being accessed by the current set and 
   // resulting in a miss. Value1 represents the number of misses for a full-
   // associative cache, and value2 represents the number of misses for a
   // set-associative cache. There are also special entries, one for each set,
   // where reuseOn is zero and specify the total execution frequency for that
   // set in value1 and the number of cold misses in value2.
   //
   // CHANGED AGAIN: Add a new section at the beginning of the file. This
   // new section contains footprint information for each scope and is 
   // terminated by a line of 5 zeros. After that we have the scope labels
   // section followed by a line of 2 zeros and then the last section 
   // containing the actual miss values for each instruction set. For each
   // set we have multiple histograms distinguished by two values: reuseOn
   // and carriedBy. Both values are unsigned ints and represent scope ids.
   // ReuseOn specifies which loop accessed a memory location before it was
   // evicted from cache (the source of the reuse).
   // CarriedBy specifies which loop (scope) carried the reuse. This is a 
   // dynamic information. 
   // In new versions (0.95 and newer), the carrier scope
   // ID is multiplied by 2, and the least significant bit specifies if the
   // reuse is loop independent (same iteration) if bit is 1, or carried reuse
   // if bit is 0. This makes sense only for loops. For routine scopes, even if the
   // bit is set to 0, the reuse is loop independent (there are no other iterations).
   //
   // NEWEST 2010/02/16 gmarin: A fourth section is added at the end of the 
   // file separated from the 3rd section by a line of five zeros.
   // This section contains global data not associated with any particular 
   // scope. These are touples of three values: pc_address reg_num value,
   // representing dynamically determined values for some registers at 
   // given points in the program.
   
   int l, i;
   int setIndex, numLabels;
   float totCount;
   float avgVal, sdVal, minVal, maxVal;
   l = fscanf (fd, "%d %g %g %g %g %g", &setIndex, &totCount, 
             &avgVal, &sdVal, &minVal, &maxVal);
//   fprintf (stderr, "Parsing footprint data: l=%d, scopeId=%d, count=%g, avg=%g, sd=%g, min=%g, max=%g\n",
//        l, setIndex, totCount, avgVal, sdVal, minVal, maxVal);
   while (l==6 && (setIndex || totCount))
   {
      scopesFP.insert (ScopesFootPrint::value_type (setIndex, 
             new FootPrintInfo (totCount, avgVal, sdVal, minVal, maxVal)) );
      
      l = fscanf (fd, "%d %g %g %g %g %g", &setIndex, &totCount, 
                &avgVal, &sdVal, &minVal, &maxVal);
//      fprintf (stderr, "Parsing footprint data: l=%d, scopeId=%d, count=%g, avg=%g, sd=%g, min=%g, max=%g\n",
//          l, setIndex, totCount, avgVal, sdVal, minVal, maxVal);
   }
   if (l!=6)
   {
      fprintf (stderr, "Miss count file %s, footprint section has invalid format.\n", 
               fileName);
      fclose (fd);
      return (-1);
   }
   
   // Finished parsing the first section of the file. 
   // Next section contains entries of two ints and list of hex values
   l = fscanf (fd, "%d %d", &setIndex, &numLabels);
//   fprintf (stderr, "Parsing scope info data: l=%d, setId=%d, numLabels=%d\n",
//          l, setIndex, numLabels);
   while (l==2 && (setIndex || numLabels))
   {
      if (setIndex==0 || numLabels<1)
      {
         fprintf (stderr, "Miss count file %s, scope labels section has invalid format.\n", 
                  fileName);
         fclose (fd);
         return (-1);
      }
      unsigned int label;
      for (i=0 ; i<numLabels ; ++i)
      {
         l = fscanf (fd, "%x", &label);
         assert (l == 1);
         unsigned int &refSet = ref2Set[label];
         assert (refSet == 0);
         refSet = setIndex;
      }

      l = fscanf (fd, "%d %d", &setIndex, &numLabels);
//      fprintf (stderr, "Parsing scope info data: l=%d, setId=%d, numLabels=%d\n",
//            l, setIndex, numLabels);
   }
   if (l != 2)
   {
      fprintf (stderr, "Miss count file %s, scope labels section has invalid format.\n", 
               fileName);
      fclose (fd);
      return (-1);
   }
   
   // Finished parsing the second section of the file. 
   // Next section contains entries of three unsigneds and two doubles
   unsigned int reuseOn, carriedBy;
   double value1 = 0.0, value2 = 0.0;
   l = fscanf (fd, "%d %u %u %lg %lg", &setIndex, &reuseOn, &carriedBy, 
                    &value1, &value2);
   while (l==5 && (setIndex || reuseOn || carriedBy))
   {
//      fprintf (stderr, "PARSED: setIndex %d, reuseOn %x, value1 %lg, value2 %lg\n",
//          setIndex, reuseOn, value1, value2);
      if (l != 5)
      {
         fprintf (stderr, "Miss count file %s has invalid format.\n", 
                  fileName);
         fclose (fd);
         return (-1);
      }
      setsInfo.insert (TrioSIMap::value_type (MIAMIU::UITrio (setIndex, reuseOn, carriedBy), 
             new SetInfo (setIndex, reuseOn, carriedBy, value1, value2)) );

      l = fscanf (fd, "%d %u %u %lg %lg", &setIndex, &reuseOn, &carriedBy, 
              &value1, &value2);
   }

   // Finished parsing the third section of the file. 
   // Next section contains entries of two unsigneds and one int
   unsigned int pc_addr, reg;
   int value;
   l = fscanf (fd, "%x %u %d", &pc_addr, &reg, &value); 
   while (l==3)
   {
      MIAMIU::UIPair tmp(pc_addr, reg);
      MIAMIU::PairUiMap::iterator pumit = unkValues.find (tmp);
      if (pumit == unkValues.end())  // not found, insert
      {
         unkValues.insert (MIAMIU::PairUiMap::value_type (tmp, value));
      } else
      {
         if ((int)pumit->second != value)
         {
            fprintf (stderr, "Warning: Found different value %d for UnkValue term at pc %x, register %u. Previous value %d\n",
                     value, pc_addr, reg, pumit->second);
            // keep the smallest value in absolute terms
            if (abs((int)pumit->second) > abs(value))
               pumit->second = value;
         }
      }
      
      l = fscanf (fd, "%x %u %d", &pc_addr, &reg, &value); 
   }
   if (l > 0)
   {
      fprintf (stderr, "Miss count file %s has invalid format.\n", 
               fileName);
      fclose (fd);
      return (-1);
   }
   
   fclose (fd);
   hasMissInfo = true;
   return (0);
}

unsigned int 
MemoryHierarchyLevel::addReuseGroupData (unsigned int _pc, 
          unsigned long long _pathId, unsigned int _numRefs, 
          unsigned long long _execFreq)
{
   unsigned int refSet = ref2Set[_pc];
   // If I do not find any info about this ref (set==0), then it must be a 
   // spill/unspill. There is no cache miss for refs in set zero.
   if (refSet > 0)
   {
      double _freq = (double)_numRefs * _execFreq;
      // add the frequency only to the histogram with reuseOn = 0
      MIAMIU::UITrio tempP (refSet, 0, 0);
      TrioSIMap::iterator pit = setsInfo.find (tempP);
      assert (pit != setsInfo.end());
      pit->second->addFreqForPath (_freq, _pathId);
   }

   return (refSet);
}

void 
MemoryHierarchyLevel::fillMissCounts (unsigned long long pathId, 
                  MIAMIU::UiDoubleMap &missTable, double &totalMissCount, 
                  int numLevels, MIAMIU::TrioDoublePMap& reuseMap, int memLevel)
{
   totalMissCount = 0;
   assert (memLevel<numLevels && memLevel>=0);
   MIAMIU::UiDoubleMap::iterator uit = missTable.begin ();
//   fprintf (stderr, "fillMissCounts: pathId %llx, memLevel %d\n",
//          pathId, memLevel);
   for ( ; uit!=missTable.end() ; ++uit )
   {
      unsigned int setIdx = uit->first;
      uit->second = 0;
      if (setIdx != 0)
      {
         // each set may have associated multiple entries, based on the
         // source of the reuse and who is carrying it. Traverse all of 
         // them when computing the number of misses, and store them also 
         // separately into the reuseMap
         MIAMIU::UITrio tempP (setIdx, 0, 0);
         TrioSIMap::iterator pit = setsInfo.find (tempP);
         assert (pit != setsInfo.end());
         double ratio = pit->second->getFrequencyRatioForPath (pathId);
//         fprintf (stderr, "RATIO=%lg\n", ratio);
         for ( ; pit!=setsInfo.end() && pit->first.first==setIdx ; ++pit)
         {
            unsigned int reuseOn = pit->first.second;
            unsigned int carriedBy = pit->first.third;
            double localVal = pit->second->getMissCountForRatio (ratio);
//            fprintf (stderr, "For setIndex=%d, reuseOn=%x, level=%d, got miss count %lg\n",
//                setIdx, reuseOn, memLevel, localVal);
            if (localVal > 0.001)
            {
               uit->second += localVal;
            
               // add current number of misses to the reuseMap
               MIAMIU::UITrio tempTrio (reuseOn, carriedBy, setIdx);
               MIAMIU::TrioDoublePMap::iterator dip = reuseMap.find (tempTrio);
               if (dip == reuseMap.end())
               {
                  dip = reuseMap.insert (reuseMap.begin(), 
                       MIAMIU::TrioDoublePMap::value_type (tempTrio,
                                     new double[numLevels+2]));
                  for (int i=0 ; i<numLevels+2 ; ++i)
                     dip->second[i] = 0.0;
               }
               dip->second[memLevel] += localVal;
            }
         }
      }
      totalMissCount += uit->second;
   }
}


SetInfo::SetInfo (int _setIndex, unsigned int _reuseOn, unsigned int _carrier,
           double _value1, double _value2)
{
   setIndex = _setIndex;
   reuseOn = _reuseOn;
//   loopIndep = _carrier & 0x1;
//   carriedBy = _carrier >> 1;
   carriedBy = _carrier;
   value1 = _value1;
   value2 = _value2;
   totalExecCount = 0;
}

SetInfo::~SetInfo ()
{
}

void 
SetInfo::addFreqForPath (double _freq, unsigned long long _pathId)
{
   totalExecCount += _freq;
   UllDoubleMap::iterator uit = pathsFreq.find (_pathId);
   if (uit == pathsFreq.end())   // first info about this path
      pathsFreq.insert (UllDoubleMap::value_type (_pathId, _freq));
   else
      uit->second += _freq;
//   fprintf (stderr, "SetInfo::addFreqForPath: add %lg freq to pathId %llx set %d, reuseOn %d, carriedBy %d; new freq for path is %lg\n",
//        _freq, _pathId, setIndex, reuseOn, carriedBy, pathsFreq[_pathId]);
}

double 
SetInfo::getFrequencyRatioForPath (unsigned long long _pathId)
{
//   fprintf (stderr, "getFrequencyRatio: for pathId %llx, set %d, reuseOn %d, carriedBy %d\n",
//             _pathId, setIndex, reuseOn, carriedBy);
   UllDoubleMap::iterator uit = pathsFreq.find (_pathId);
   // I should always find the path id in this map
   assert (uit != pathsFreq.end());
   // compute a ratio proportional with a path's execution frequency
//   fprintf (stderr, "SetInfo::getMissCountForPath: received request for path %llx. Found freq %lg / total freq %lg\n", 
//         _pathId, uit->second, totalExecCount);
   return ((uit->second)/totalExecCount);
}

double 
SetInfo::getMissCountForRatio (double ratio)
{
   // divide value2 proportionally with the specified ratio
//   fprintf (stderr, "SetInfo::getMissCountForPath: Set %d, reuseOn %x, ratio %lg, value1=%lg, value2=%lg\n", 
//         setIndex, reuseOn, ratio, value1, value2);
   return (ratio * value2);
}
#endif
