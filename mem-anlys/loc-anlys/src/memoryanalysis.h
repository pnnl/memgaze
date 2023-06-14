#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "structure.h"
#include "TraceLine.hpp"
#include "BlockInfo.hpp"
#include <stdio.h>


//Data stored as <IP addr core initialtime\n>
// Core is not processed in RUD or spatial correlational analysis
int readTrace(string filename, int *intTotalTraceLine,  vector<TraceLine *>& vecInstAddr, uint32_t *windowMin, uint32_t *windowMax, 
                            double *windowAvg, uint64_t * max, uint64_t * min) ;
void getInstInRange(std::ofstream *outFile, vector<TraceLine *>& vecInstAddr,MemArea memarea) ;

/*  Get access count only - NO RUD analysis */
int getAccessCount(vector<TraceLine *>& vecInstAddr,  MemArea memarea,  int coreNumber , vector<BlockInfo *>& vecBlockInfo );

/*  Get highest access cache-lines in region */
int getTopAccessCountLines(vector<TraceLine *>& vecInstAddr,  Memblock memRegion, vector<pair<uint64_t, uint64_t>> vecParentChild,
                                  vector<TopAccessLine *>& vecLineInfo , uint64_t pageSize, uint64_t lineSize,uint8_t regionId) ;

/* RUD analysis if spatialResult == 0 */

/*
303 * RUD analysis if spatialResult == 0
304 * Spatial Correlational analysis if spatialResult == 1
305 * Inter-region Spatial Correlational analysis between set of regions if flagSetRegion ==1
306 * Intra-region Spatial Correlational analysis between blocks in a page (reference), all other addresses (affinity)
307 *   flagIncludePages - set to 0 - all other addresses lumped into one bucket
308 *   flagIncludePages - set to 1 - all other addresses bucketed based on exponential spatial location
309 *   flagPagesRegion - set to 1 - all other addresses bucketed based hot pages in region, other regions, non-hot and stack
310 *   flagHotLines - set to 1 - all other address bucketed based top 10 hot lines, other regions, non-hot and stack
311 * --- There is spill code from original whole trace analysis (not considering samples),
312 *      will have to clean it up for overhead analysis
313 */

int spatialAnalysis(vector<TraceLine *>& vecInstAddr,  MemArea memarea,  int coreNumber, int spatialResult, 
                        vector<BlockInfo *>& vecBlockInfo,  vector<pair<uint64_t, uint64_t>> setRegionAddr,
                        MemArea memIncludeArea, 
                        vector<pair<uint64_t, uint64_t>> vecParentChild,
                        vector<uint64_t> vecHotLines, uint8_t affinityOption);

int updateTraceRegion(vector<TraceLine *>& vecInstAddr ,vector<pair<uint64_t, uint64_t>> setRegionAddr, uint64_t heapAddrEnd);
#endif
