#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "structure.h"
#include "TraceLine.hpp"
#include "BlockInfo.hpp"
#include <stdio.h>

using namespace std;
using std::cerr;
using std::dec;
using std::endl;
using std::hex;
using std::ofstream;
using std::setw;
using std::string;
using std::vector;

int printDebug =0;
int printProgress =1;

//Data stored as <IP addr core initialtime\n>
// Core is not processed in RUD or spatial correlational analysis
int readTrace(string filename, int *intTotalTraceLine,  vector<TraceLine *>& vecInstAddr, uint32_t *windowMin, uint32_t *windowMax, 
                            double *windowAvg, uint64_t * max, uint64_t * min)
{
	// File pointer 
  fstream fin; 
  fin.open(filename, ios::in); 
  if (!(fin.is_open())) {
    cout <<"Error in file open - " << filename << endl;
    return -1; 
  }
  // read the state line
  string line,ip,addr,core, inittime, sampleIdStr;
  string dso_id, dso_value; 
  uint64_t insPtrAddr, loadAddr, instTime; 
  uint32_t  coreNum; 
  int  sampleId, curSampleId, prevSampleId; 
  uint32_t  curSampleCnt, numSamples; 
  map <int, string> dsoMap;
  curSampleCnt = 0;
  numSamples=0;
  prevSampleId=-1;
  curSampleId=-1;  
	core = "0";
  bool isDSO = false, anyDSO = false;
  bool isTrace = false;
  TraceLine *ptrTraceLine;
  uint64_t addrLowThreshold = *min; 
  uint64_t addrHighThreshold = *max; 
  *min = UINT64_MAX;
  *max = 0;

  if(fin.is_open()){
    while(getline(fin, line)){
		  std::stringstream s(line);
      if (line.find("DSO:") != std::string::npos){
        isDSO = true;
        isTrace = false;
        continue;
      } else if (line.find("TRACE:") != std::string::npos){
        isDSO = false;
        isTrace = true;
        continue;
      }
      if (isDSO){
        //split(line, dso_elems);
        getline(s, dso_value,' '); 
        getline(s, dso_id);
        int dsoID1 = stoi(dso_id);
        dsoMap.insert({dsoID1, dso_value});
        anyDSO = true;
      }
      if ((isTrace && !isDSO)|| (!isDSO && !isTrace && !anyDSO) ){  
        (*intTotalTraceLine)++;
		    getline(s,ip,' ');
     		getline(s,addr,' ');
        loadAddr = stoull(addr,0,16);
        if ((loadAddr > addrLowThreshold) && (loadAddr < addrHighThreshold)) {
          if (loadAddr > (*max)) (*max) = loadAddr; //check max
          if (loadAddr < (*min)) (*min) = loadAddr; //check min
          insPtrAddr = stoull(ip,0,16);
      	  getline(s,core,' ');
          coreNum= stoi(core);
        	getline(s,inittime,' ');
          instTime= stoull(inittime);
        	getline(s,sampleIdStr,' ');
          sampleId= stoull(sampleIdStr);
          if ( (curSampleId ==-1) && (prevSampleId ==-1)) {
             curSampleId=sampleId;
            prevSampleId=sampleId;
            numSamples++;
          }
          curSampleId=sampleId;
          if ( curSampleId != prevSampleId) {
            if(curSampleCnt < (*windowMin) || (*windowMin ==0))
               *windowMin = curSampleCnt;
            if(curSampleCnt > (*windowMax) || (*windowMax ==0))
              *windowMax = curSampleCnt;
            *windowAvg = ((double)((*windowAvg)*(numSamples-1))+(double)curSampleCnt)/(double)numSamples;
            //printf(" curSampleId %d prevSampleId %d average %f curSampleCnt %d numSamples %d\n", curSampleId, prevSampleId, *windowAvg, +curSampleCnt, numSamples);
            curSampleCnt=0;
            numSamples++;
          } else {
            curSampleCnt++;
          }  
          prevSampleId = curSampleId;
          // USED in GAP verification of access counts - experimental
          //uint64_t GAP_pr_low_ip = stoull("30a32c", 0, 16);  //uint64_t GAP_pr_high_ip= stoull("30a578", 0, 16);
          //uint64_t GAP_cc_low_ip = stoull("300ad0", 0, 16); // [0x300ad0-0x300be6)  //uint64_t GAP_cc_high_ip= stoull("300be6", 0, 16);
          //uint64_t GAP_CSR_low = stoull("3008fe", 0, 16); // [0x300ad0-0x300be6)   //uint64_t GAP_CSR_high = stoull("300a5b", 0, 16);
          //if((insPtrAddr >= GAP_pr_low_ip) && (insPtrAddr < GAP_pr_high_ip))
          //{
            ptrTraceLine=new TraceLine(insPtrAddr, loadAddr, coreNum, instTime, sampleId);
            vecInstAddr.push_back(ptrTraceLine);
          //}
        }
      } 
    }
  }
  return 0;
}

/*
Get IP for data addresses in memarea
*/
void getInstInRange(std::ofstream *outFile, vector<TraceLine *>& vecInstAddr,MemArea memarea)
{
	uint64_t loadAddr =0;  //used for offsit check
  std::map<uint64_t,uint32_t> insMap;
  std::map<uint64_t,uint32_t>::iterator it;
  uint64_t insPtrAddr;
  TraceLine *ptrTraceLine;
  for (uint32_t itr=0; itr<vecInstAddr.size(); itr++){
      ptrTraceLine=vecInstAddr.at(itr);
      //ptrTraceLine->printTraceLine();    
      loadAddr = ptrTraceLine->getLoadAddr();
    	if((loadAddr>=memarea.min)&&(loadAddr<=memarea.max)){
        insPtrAddr=ptrTraceLine->getInsPtAddr();  
        //printf( "%08lx\n", insPtrAddr);
        if(insMap.find(insPtrAddr)!=insMap.end())
          (insMap.find(insPtrAddr)->second)++;
        else
         insMap[insPtrAddr]=1;
      }
  }
  vector <pair<uint32_t,uint64_t>> sortInstr;
  vector <pair<uint32_t,uint64_t>>::iterator itr;
  for (it=insMap.begin(); it!=insMap.end(); it++){
    //printf("%ld\t0x%lx \n", it->second, it->first);
    sortInstr.push_back(make_pair(it->second, it->first));
  }
  sort(sortInstr.begin(), sortInstr.end(),greater<>()); 
  for (itr=sortInstr.begin(); itr!=sortInstr.end(); itr++){
    if(outFile==nullptr)
      printf("%u\t0x%lx \n", itr->first, itr->second);
    else 
      *outFile << std::dec <<itr->first<<"\t0x"<< std::hex<< itr->second<<endl;
  }
  for (itr=sortInstr.begin(); itr!=sortInstr.end(); itr++)
    printf("%lx\\|", itr->second);
  printf("\n");
}

/* 
Get access count only - NO RUD analysis
*/
int getAccessCount(vector<TraceLine *>& vecInstAddr,  MemArea memarea,  int coreNumber , vector<BlockInfo *>& vecBlockInfo ){
  TraceLine *ptrTraceLine;
	uint32_t * totalAccess = new uint32_t [memarea.blockCount]; //Total memory access
	uint64_t loadAddr =0;  
  uint32_t i;
  uint32_t pageID = 0 ;
	for(i = 0; i < memarea.blockCount; i++){
	  totalAccess[i] = 0;
  }
  for (uint32_t itr=0; itr<vecInstAddr.size(); itr++){
    if(printProgress) {
      if(itr%1000000==0) printf("in getAccessCount procesing trace line %d\n", itr);
    }
    ptrTraceLine=vecInstAddr.at(itr);
    loadAddr = ptrTraceLine->getLoadAddr();
    //if(printDebug) printf("previous Addr %08lx less than %d greater than %d \n", loadAddr, (loadAddr<=memarea.max), (loadAddr>=memarea.min));
    if(( (loadAddr>=memarea.min)&&(loadAddr<=memarea.max) ) ){
      pageID = floor((loadAddr-memarea.min)/memarea.blockSize);
      // Number of blocks does not evenly divide address space - so the last one includes spill-over address range
      if(pageID == memarea.blockCount) {
         pageID--;
      }
      totalAccess[pageID]++;
    }
  }
  if(printDebug) printf("Size of vector %ld\n", vecBlockInfo.size());
  if(!vecBlockInfo.empty()){
    for(i = 0; i < memarea.blockCount; i++){
      BlockInfo *curBlock = vecBlockInfo.at(i);
      //printf("BLOCK i is %d access is %ld \n", i,totalAccess[i]);
      curBlock->setAccess(totalAccess[i]);
      //curBlock->printBlockRUD();
    }
  }
  return 0;
}

/* 
* RUD analysis if spatialResult == 0
* Spatial Correlational analysis if spatialResult == 1
* Spatial Correlational analysis between set of regions if flagSetRegion ==1
* --- There is spill code from original whole trace analysis (not considering samples), 
*      will have to clean it up for overhead analysis
*/
int spatialAnalysis(vector<TraceLine *>& vecInstAddr,  MemArea memarea,  int coreNumber, int spatialResult, 
                        vector<BlockInfo *>& vecBlockInfo, bool flagSetRegion, vector<pair<uint64_t, uint64_t>> setRegionAddr,
                        bool flagIncludePages, MemArea memIncludeArea){
  TraceLine *ptrTraceLine;
  uint32_t numBlocks =0;
  numBlocks = memarea.blockCount+memIncludeArea.blockCount; // memIncludeArea.blockCount should be set to least value of 1 
	int * distance = new int [numBlocks];   
	int * sampleDistance = new int [numBlocks];   
  if(printProgress) printf("in memory analysis before array\n");
	//uint32_t * refdistance = new uint32_t [numBlocks]; // ref distance
	uint32_t * sampleRefdistance = new uint32_t [numBlocks]; // ref distance
	//uint32_t * lifetime = new uint32_t [numBlocks]; // lifetime of block
	uint32_t * sampleTotalLifetime = new uint32_t [numBlocks]; // total intra-sample lifetime of block
	uint32_t * sampleLifetime = new uint32_t [numBlocks]; // intra-sample lifetime of block
	uint32_t * inSampleLifetimeCnt = new uint32_t [numBlocks];  //Counter for intra-sample lifetime & spatial metric calculations 
                                                              //increase counter if a sample has valid lifetime (non-zero) for a block 
	//int * minDistance = new int [numBlocks]; //min RUD 
	//int * maxDistance = new int [numBlocks]; //maximum RUD
	uint32_t * totalDistance = new uint32_t [numBlocks]; //Total RUD to calculate the average 
	uint32_t * totalAccess = new uint32_t [numBlocks]; //Total memory access
	uint32_t * lastAccess = new uint32_t [numBlocks]; // Record the temp access time
	uint32_t * sampleLastAccess = new uint32_t [numBlocks]; //Record the temp access time
	uint32_t * stack = new uint32_t [numBlocks]; //stacked distance buffer
	uint32_t stacklength = 0;
	uint32_t * sampleStack = new uint32_t [numBlocks]; //stacked distance buffer
	uint32_t sampleStackLength = 0;
  uint32_t i, j;

	uint32_t * inSampleAccess = new uint32_t [numBlocks];    //Total memory access inside a sample 
	uint32_t * inSampleTotalRUD = new uint32_t [numBlocks];  //Total RUD inside a sample   
	uint32_t * inSampleRUDAvgCnt = new uint32_t [numBlocks];  //Average counter for inSampleAvgRUD calculations - increase counter if a sample has more than one access to a block and hence has valid RUD values
	double * inSampleAvgRUD = new double [numBlocks];  //Average of RUD between samples
  int curSampleId, prevSampleId; 
  prevSampleId = -1;
  curSampleId = -1;
	uint64_t loadAddr =0;  
  std::map<uint32_t, class SpatialRUD*> spatialRUD;
  std::map<uint32_t, class SpatialRUD*>::iterator itrSpRUD;
  SpatialRUD *curSpatialRUD; 
  uint32_t * totalCAccess = new uint32_t [coreNumber]; // Ununsed - stored as 0 - Seg faults
  bool blNewSample=1;
  bool blProcessTrace=1;
	
	for(int i = 0; i < coreNumber; i++) totalCAccess[i] = 0; // Ununsed - coreNumber is 0, Seg faults

  if(printProgress) printf("in memory analysis before for loop\n");
	for(i = 0; i < numBlocks; i++){
		stack[i] = 0;
		distance[i] = 0;
		sampleStack[i] = 0;
		sampleDistance[i] = 0;
		//refdistance[i] = 0;
		sampleRefdistance[i] = 0;
		//lifetime[i] = 0;
		sampleTotalLifetime[i] = 0;
		sampleLifetime[i] = 0;
	  //minDistance[i] = -1;
	  //maxDistance[i] = -1;
	  totalDistance[i] = 0;
	  totalAccess[i] = 0;
	  lastAccess[i] = 1;	
	  sampleLastAccess[i] = 0;	
	  inSampleAccess[i] = 0; 
	  inSampleRUDAvgCnt[i] = 0; 
	  inSampleLifetimeCnt[i] = 0; 
	  inSampleTotalRUD[i] = 0; 
	  inSampleAvgRUD[i] = -1; 
	}
  printf("Spatial analysis before vector procesing address range %08lx - %08lx \n", memarea.min, memarea.max);
	int lastPage = 0;
  uint64_t totalinst = 0;
	int time = 0 ;
  uint32_t pageID =0;
  if(printDebug) printf("Size of vector %ld\n", vecBlockInfo.size()); 
  // START - Vector FOR loop 
  for (uint32_t itr=0; itr<vecInstAddr.size(); itr++){
    if(printProgress) {
      if(itr%1000000==0) printf("in spatial analysis procesing trace line %d\n", itr);
    }
    ptrTraceLine=vecInstAddr.at(itr);
    //ptrTraceLine->printTraceLine();    
    loadAddr = ptrTraceLine->getLoadAddr();
    //totalCAccess[ptrTraceLine->getCoreNum()]++; // Ununsed - coreNumber (index) is 0 - Seg faults
    totalinst++; // Time does not gets filtered 
    // minivite v3  55c45da0c170-55c45da1416f 55c45da0c170-55c45da1416f
    /*
    uint64_t low_addr = stoull("55c45da0c170", 0, 16); uint64_t high_addr = stoull("55c45da1416f", 0, 16);
    if ((((loadAddr>= low_addr) && (loadAddr <= high_addr)) ) && (spatialResult ==1))
      printDebug=1;
    else
      printDebug=0;
    */

     /* REMOVE - filtering by address for spatial analysis - handle blindspots
      if(( (loadAddr>=memarea.min)&&(loadAddr<=memarea.max)) 
      || ((flagSetRegion==1) && (loadAddr>=setRegionAddr[0].first)&&(loadAddr<=setRegionAddr[(setRegionAddr.size()-1)].second))
      || (flagIncludePages == 1 && (loadAddr >= memIncludeArea.min && loadAddr <= memIncludeArea.max))){
      // START - FILTER trace for load addresses in region correlation analysis - exclude stack and holes
      if(flagSetRegion ==1) {
          blProcessTrace=0;
          for (uint32_t k=0; k<setRegionAddr.size(); k++)  {
           if((loadAddr>=setRegionAddr[k].first)&&(loadAddr<=setRegionAddr[k].second)) {
              blProcessTrace=1;
            }
          }
        } else {
          blProcessTrace=1;
        }
      if (blProcessTrace ==1) {  
      */
        //totalinst++; // Filter by region trace - Time also gets filtered would not be correct for SI or SD
      	curSampleId = ptrTraceLine->getSampleId();
        if ((printDebug)) printf(" %08lx totalinst %ld \n", loadAddr,totalinst);
        if ((((itr!=0) && (curSampleId != prevSampleId))) ) 
        {
          if(printDebug) printf(" ----- NEW SAMPLE %08lx prevSampleID %d curSampleId %d\n", loadAddr, prevSampleId, curSampleId);
            for(i = 0; i < memarea.blockCount; i++){
              if(sampleLifetime[i]!=0) {
                sampleLifetime[i]++; // WHY - Lifetime = total distance between first and last access +1 
                sampleTotalLifetime[i]++;
                inSampleLifetimeCnt[i]++;
              }
            }
            for( itrSpRUD = spatialRUD.begin(); itrSpRUD!= spatialRUD.end(); ++itrSpRUD){
              curSpatialRUD = itrSpRUD->second;
              uint32_t curPageID = floor(itrSpRUD->first/numBlocks);
              uint32_t corrPageID = floor(itrSpRUD->first%numBlocks);
              if(sampleLifetime[curPageID]!=0) {
                curSpatialRUD->smplAvgSpatialMiddle= (( curSpatialRUD->smplAvgSpatialMiddle * (inSampleLifetimeCnt[curPageID]-1))+((double)((curSpatialRUD->smplMiddle))/((double)sampleLifetime[curPageID])))/ (double) inSampleLifetimeCnt[curPageID];
                if (printDebug == 1) printf(" after curSampleId %d curPageID %d corrPageID %d inSampleLifetimeCnt[%d] %d sampleLifetime %d smplMiddle %d smplAvgSpatialMiddle %f \n", 
                      curSampleId, curPageID, corrPageID, curPageID, inSampleLifetimeCnt[curPageID], sampleLifetime[curPageID], curSpatialRUD->smplMiddle, curSpatialRUD->smplAvgSpatialMiddle);
              }
              curSpatialRUD->spatialAccessMid=0;
              curSpatialRUD->smplMiddle =0;
            }
    
            for(i = 0; i < numBlocks; i++){
              if (inSampleAccess[i] > 1) {
                if(inSampleAvgRUD[i] == -1)
                    inSampleAvgRUD[i] = 0; 
                inSampleRUDAvgCnt[i]++;
                // RUD Average in a sample added to total - averaged over the count of samples with valid RUD values
                inSampleAvgRUD[i] = ((inSampleAvgRUD[i]*(inSampleRUDAvgCnt[i]-1)) + (double)inSampleTotalRUD[i]/(double)(inSampleAccess[i] -1)) / inSampleRUDAvgCnt[i];
              }
              inSampleTotalRUD[i] =0;
              inSampleAccess[i] = 0;
              sampleStackLength=0;
              sampleStack[i]=0;
              sampleLastAccess[i]=0;
              sampleRefdistance[i]=0;
              sampleLifetime[i]=0;
            }
            blNewSample=1;
          }
          if(flagSetRegion==0) { 
            if ( loadAddr >= memarea.min && loadAddr <= memarea.max) {
          		pageID = floor((loadAddr-memarea.min)/memarea.blockSize);
              // Number of blocks does not evenly divide address space - so the last one includes spill-over address range
        		  if(pageID == memarea.blockCount) {
                pageID--;
              }
            // NEW include blindspots
            } else if (flagIncludePages ==0 ) { 
                // Put all other accesses into one bucket
                pageID = memarea.blockCount;
            } else { 
              // Assign appropriate buckets based on address range
              // For memarea.blockCount = 256, memIncludeArea.blockCount=64
              // PageID  0-255 = regular cache blocks, 256-287 = -(2^32)p to -p, 288-319 = p to (2^31)p
              if ( loadAddr < memarea.min) {
                  pageID = memarea.blockCount - ceil(log(ceil(((double)(memarea.min - loadAddr)/(double)memIncludeArea.blockSize))) / log(2)) + (memIncludeArea.blockCount/2) ;
                  if( pageID < memarea.blockCount) pageID++; // Do not overwrite spill into actual results
              } else if ( loadAddr > memarea.max) {
                  pageID = memarea.blockCount + ceil(log(ceil(((double)(loadAddr-memarea.max)/(double)memIncludeArea.blockSize)))/log(2)) + (memIncludeArea.blockCount/2)  ;
                  if( pageID >= (memarea.blockCount+(memIncludeArea.blockCount))) pageID--; // Beyond 2^31 put in the same bucket
                  if(printDebug) printf( "more loadAddr %ld memarea.max %ld diff %lf ceil %lf  add %d pageID %d \n", loadAddr, memarea.max, 
                    ((double)(loadAddr-memarea.max)/(double)memIncludeArea.blockSize), ceil(((double)(loadAddr-memarea.max)/(double)memIncludeArea.blockSize)), memarea.blockCount, pageID);
              }
            } 
            if(printDebug) printf( "loadAddr %08lx memarea.min %08lx  memarea.max %08lx pageID %d\n", loadAddr, memarea.min, memarea.max, pageID );
          } else {
            bool flInRegion = false;
            for (uint32_t k=0; k<setRegionAddr.size(); k++) {
             if((loadAddr>=setRegionAddr[k].first)&&(loadAddr<=setRegionAddr[k].second)) {
                pageID = k;
                flInRegion=true;
                break;
             } 
            }
            if( flInRegion == false) pageID = memarea.blockCount; // Put all other accesses into one bucket
          }
      		totalAccess[pageID]++;
          inSampleAccess[pageID]++;
    			time = totalinst; 
          /*
    			//Old RUD analysis - does not accoutn for samples
    			if (totalAccess[pageID] == 1){
    				stack[stacklength] = pageID;
    				stacklength++;
    				distance[pageID] = -1;
    			}				
    			else if(totalAccess[pageID] > 1){
    				//search the page in stack	
    				int pos = 0;
  	  			for(uint32_t j = 0; j< stacklength; ++j){
    					if(stack[j] == pageID) {
    						pos = j;
  	  					break;
  		  			}
  			  	}
    				refdistance[pageID] = time - lastAccess[pageID];
    				distance[pageID] = stacklength - pos-1;
  	  			//update the page in stack
  		  		for(uint32_t j = pos; j< stacklength-1; ++j){
    					stack[j]=stack[j+1];
  			  	}
    				stack[stacklength-1]= pageID;
    				//if(printDebug)printf("time %d pageID %d, last access %d stacklength %d pos %d distance %d\n", time, pageID, lastAccess[pageID], stacklength, pos, distance[pageID]  );
  	  			totalDistance[pageID] = totalDistance[pageID]+distance[pageID];
    				if ((distance[pageID]>maxDistance[pageID])||(maxDistance[pageID]==-1)) maxDistance[pageID] = distance[pageID];
    				if ((distance[pageID]<minDistance[pageID])||(minDistance[pageID]==-1)) minDistance[pageID] = distance[pageID];
    			} */
      		if (inSampleAccess[pageID] == 1){
      			sampleStack[sampleStackLength] = pageID;
      			sampleStackLength++;
      			sampleDistance[pageID] = -1;
      		} else if(inSampleAccess[pageID] > 1){
    	  		//search the page in stack	
      			int samplePos = 0;
      			for(uint32_t j = 0; j< sampleStackLength; ++j){
      				if(sampleStack[j] == pageID) {
    	  				samplePos = j;
    		  			break;
    			  	}
    			  }
      			sampleDistance[pageID] = sampleStackLength - samplePos-1;
      			//update the page in stack
      			for(uint32_t j = samplePos; j< sampleStackLength-1; ++j){
    	  			sampleStack[j]=sampleStack[j+1];
    		  	}
      			sampleStack[sampleStackLength-1]= pageID;
            inSampleTotalRUD[pageID] = inSampleTotalRUD[pageID] + sampleDistance[pageID];  
            //if(flagIncludePages ==1 ) printf(" pageID %d, sampleRef[%d] %d samplelasAccess[%d] %d \n", pageID, pageID, sampleRefdistance[pageID], pageID, sampleLastAccess[pageID]);
      			sampleRefdistance[pageID] = time - sampleLastAccess[pageID];
          }
          if((spatialResult == 1) && (blNewSample==0) && (lastPage < memarea.blockCount)) {
         		if(totalAccess[lastPage] >= 1) {
               itrSpRUD = spatialRUD.find((lastPage*numBlocks)+pageID);
               if (itrSpRUD != spatialRUD.end()) {
                 curSpatialRUD = itrSpRUD->second;
                 (curSpatialRUD->spatialNext)++;
               } else {
                 curSpatialRUD = new SpatialRUD((lastPage*numBlocks)+pageID);
                 spatialRUD[(lastPage*numBlocks)+pageID] = curSpatialRUD;
                 (curSpatialRUD->spatialNext)++;
               } 
              //printf("spatialNext[%d][%d] %d \n", lastPage, pageID, spatialNext[lastPage][pageID]);
    	 		  }
          }
       		lastPage = pageID;
     				
          if((spatialResult == 1) && (blNewSample==0) ) {
       	  	//record the access in mid
     	  	  if(printDebug) printf("******* %ld: page ID %d\n", totalinst, pageID);
            if(pageID < memarea.blockCount) {
     		  	  for(j = 0; j < numBlocks; j++){
                if(totalAccess[j] !=0) {
                  itrSpRUD = spatialRUD.find((pageID*numBlocks)+j);
                  if (itrSpRUD != spatialRUD.end()) {
                    curSpatialRUD = itrSpRUD->second;
                  } else {
                    curSpatialRUD = new SpatialRUD((pageID*numBlocks)+j);
                    spatialRUD[(pageID*numBlocks)+j] = curSpatialRUD;
                    (curSpatialRUD->spatialAccessMid) =0;
                  }
                  (curSpatialRUD->spatialAccessTotalMid)+=(curSpatialRUD->spatialAccessMid);
                  (curSpatialRUD->smplMiddle)+=(curSpatialRUD->spatialAccessMid);
                  (curSpatialRUD->spatialAccessMid) =0;
                }
       			  }
            }
      			//record spatial distance after i
      			for(i = 0; i < memarea.blockCount; i++){
      			//for(i = 0; i < numBlocks; i++){
      				if(sampleLastAccess[i]!=0){
                if (totalAccess[i] !=0) {
                  itrSpRUD = spatialRUD.find((i*numBlocks)+pageID);
                  if (itrSpRUD != spatialRUD.end()) {
                    curSpatialRUD = itrSpRUD->second;
                  } else {
                    curSpatialRUD = new SpatialRUD((i*numBlocks)+pageID);
                    spatialRUD[(i*numBlocks)+pageID] = curSpatialRUD;
                  }
          				curSpatialRUD->spatialDistance= time - sampleLastAccess[i]-1; // Calculate interval distance - access counts between the two
        	  			if(curSpatialRUD->spatialAccessMid==0){
      	  	  			curSpatialRUD->spatialTotalDistance += curSpatialRUD->spatialDistance;
        						curSpatialRUD->spatialAccess++;
                  }
    			  		  curSpatialRUD->spatialAccessMid++;
                  if(printDebug) printf(" time %d, pageID %d, i %d, curSpatialRUD->spatialAccessMid %d curSpatialRUD->spatialAccessMid %d\n", time, pageID, i,  
                                        curSpatialRUD->spatialAccessMid, curSpatialRUD->smplMiddle );
                  if(printDebug ) printf(" totalAccess[%d] %d time %d, pageID %d, i %d, curSpatialRUD->spatialAccess %d curSpatialRUD->spatialTotalDistance %d\n", i, 
                                        totalAccess[i], time, pageID, i,  curSpatialRUD->spatialAccess, curSpatialRUD->spatialTotalDistance );
                }
    	  			}
    		  	}
          }
          if(pageID < memarea.blockCount){
        		sampleTotalLifetime[pageID] +=  sampleRefdistance[pageID];
        		sampleLifetime[pageID] +=  sampleRefdistance[pageID];
          }
          //if(printDebug) printf("time %d, pageID %d, sampleRef[%d] %d samplelife[%d] %d \n", time, pageID, pageID, sampleRefdistance[pageID], pageID, sampleTotalLifetime[pageID]);
      		lastAccess[pageID] = time; //update the page access time
      		sampleLastAccess[pageID] = time; //update the page access time
        /*
        }
			} 
      */
      // END - REMOVE - filtering by address for spatial analysis - handle blindspots
      prevSampleId = curSampleId;
      blNewSample = 0;
  } // END - Vector FOR loop 
  // Add the last sample RUD to the averages
  for(i = 0; i < memarea.blockCount; i++){
    if (inSampleAccess[i] > 1) {
      if(inSampleAvgRUD[i] == -1)
        inSampleAvgRUD[i] = 0; 
      inSampleRUDAvgCnt[i]++;
      // RUD Average in a sample added to total - averaged over the count of samples with valid RUD values
      inSampleAvgRUD[i] = (inSampleAvgRUD[i]*(inSampleRUDAvgCnt[i]-1) + (double)inSampleTotalRUD[i]/(double)(inSampleAccess[i] -1)) / inSampleRUDAvgCnt[i];
    }
    if(inSampleAvgRUD[i] != -1)
    if(printDebug) printf(" inSampleAccess[%d] %d inSampleTotalRUD[%d] %d inSampleAvgRUD[%d] %f \n", i, inSampleAccess[i],
                                      i, inSampleTotalRUD[i], i, inSampleAvgRUD[i] );
  }
  if(spatialResult == 1) {
    for(i = 0; i < memarea.blockCount; i++){
       if(sampleLifetime[i]!=0) {
         sampleLifetime[i]++; // WHY - Lifetime = total distance between first and last access +1 
         sampleTotalLifetime[i]++;
         inSampleLifetimeCnt[i]++;
       }
    }
    for( itrSpRUD = spatialRUD.begin(); itrSpRUD!= spatialRUD.end(); ++itrSpRUD){
      curSpatialRUD = itrSpRUD->second;
      uint32_t curPageID = floor(itrSpRUD->first/numBlocks);
      uint32_t corrPageID = floor(itrSpRUD->first%numBlocks);
      if(printDebug) printf(" curSampleId %d curPageID %d corrPageID %d sampleLifetime %d smplMiddle %d smplAvgSpatialMiddle %f \n", 
                              curSampleId, curPageID, corrPageID, sampleLifetime[curPageID], curSpatialRUD->smplMiddle, curSpatialRUD->smplAvgSpatialMiddle);
      if(sampleLifetime[curPageID]!=0) {
        curSpatialRUD->smplAvgSpatialMiddle= (( curSpatialRUD->smplAvgSpatialMiddle * (inSampleLifetimeCnt[curPageID]-1))+((double)((curSpatialRUD->smplMiddle))/((double)sampleLifetime[curPageID])))/ (double) inSampleLifetimeCnt[curPageID];
      }
      curSpatialRUD->spatialAccessMid=0;
      curSpatialRUD->smplMiddle =0;
    }
  }
    
  if(printDebug) printf("Size of vector %ld\n", vecBlockInfo.size()); 
  if(!vecBlockInfo.empty()){
    for(i = 0; i < memarea.blockCount; i++){
      BlockInfo *curBlock = vecBlockInfo.at(i);
      //printf("BLOCK i is %d access is %ld distance %ld life %ld sampleRUD %lf\n", i,totalAccess[i], totalDistance[i] , sampleTotalLifetime[i], inSampleAvgRUD[i]); 
      curBlock->setAccessRUD(totalAccess[i], totalDistance[i] , sampleTotalLifetime[i], inSampleAvgRUD[i]);
      //curBlock->printBlockRUD();
    }
  }
  if (printDebug) {
    for (itrSpRUD = spatialRUD.begin(); itrSpRUD!=spatialRUD.end(); ++itrSpRUD) {
      curSpatialRUD = itrSpRUD->second;
      printf("Spatial hash i %d, j %d \n", (itrSpRUD->first)/numBlocks, (itrSpRUD->first)%numBlocks); 
      uint32_t curPageID = floor(itrSpRUD->first/numBlocks);
      uint32_t corrPageID = floor(itrSpRUD->first%numBlocks);
      printf(" curSampleId %d curPageID %d corrPageID %d sampleLifetime %d smplMiddle %d smplAvgSpatialMiddle %f \n", 
                        curSampleId, curPageID, corrPageID, sampleLifetime[curPageID], curSpatialRUD->smplMiddle, curSpatialRUD->smplAvgSpatialMiddle);
      }
  }

  for(i = 0; i < memarea.blockCount; i++){
    if(totalAccess[i]!=0) {  
      BlockInfo *curBlock = vecBlockInfo.at(i);
      for(j = 0; j < numBlocks; j++){
        itrSpRUD = spatialRUD.find((i*numBlocks)+j);
        if (itrSpRUD != spatialRUD.end()) {
           curBlock->setSpatialRUD(j, itrSpRUD->second);
           //printf("Spatial assign  i %d, j %d \n"  , i, j);
        }
      }
    }
  }

	//for(int i = 0; i<coreNumber; i++){
	//	printf("core %d, total access is %d\n", i, totalCAccess[i]);
	//}
	delete[] distance;
	delete[] sampleDistance ;
  delete[] sampleRefdistance;
  delete[] sampleStack;
	delete[] sampleTotalLifetime; 
	delete[] sampleLifetime ;
	delete[] inSampleLifetimeCnt; 
	delete[] totalDistance; 
	delete[] totalAccess; 
  delete[] lastAccess; 
  delete[] sampleLastAccess; 
	delete[] inSampleAvgRUD;  
  delete[] stack; 
	delete[] inSampleAccess; 
  delete[] inSampleTotalRUD; 
	delete[] inSampleRUDAvgCnt; 
  delete[] totalCAccess;
  return 0;
}
#endif
