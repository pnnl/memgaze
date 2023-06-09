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
int getTopAccessCountLines(vector<TraceLine *>& vecInstAddr,  vector<pair<uint64_t, uint64_t>> vecParentChild,
                                  vector<pair<uint64_t, string>>& vecLineInfo , uint64_t pageSize, uint64_t lineSize) ;

/* RUD analysis if spatialResult == 0 */
int spatialAnalysis(vector<TraceLine *>& vecInstAddr,  MemArea memarea,  int coreNumber, int spatialResult, 
                        vector<BlockInfo *>& vecBlockInfo, bool flagSetRegion, vector<pair<uint64_t, uint64_t>> setRegionAddr,
                        bool flagIncludePages, MemArea memIncludeArea, 
                        bool flagPagesRegion, vector<pair<uint64_t, uint64_t>> vecParentChild);

int updateTraceRegion(vector<TraceLine *>& vecInstAddr ,vector<pair<uint64_t, uint64_t>> setRegionAddr, uint64_t heapAddrEnd);
#endif
