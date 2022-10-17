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

ofstream outFile;

//#define WINDOW 2000000

uint32_t WINDOW = 20;
int RUDthreshold = 3;
int printDebug =0;
int printProgress =1;

// Print a memory record with block ID, RUD and stride
void RecordMem(string input, int ID, double rud, double stride, double centrePage, int minPage, int maxPage)
//void RecordMem(string input, int ID, double rud, double stride) 
{
    outFile << input << ":" << ID << " " << rud << " " << stride << " " << centrePage << " " << minPage <<" " << maxPage << endl;
    //outFile << input << ": " << ID << " " << rud << " " << stride << endl;
}

//Data stored as <IP addr core initialtime\n>
int readTrace(string filename, int *intTotalTraceLine,  vector<TraceLine *>& vecInstAddr, uint32_t *windowMin, uint32_t *windowMax, double *windowAvg, uint64_t * max, uint64_t * min)
{
	// File pointer 
  fstream fin; 
  // Open an existing file 
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

  //while (!fin.eof()){
  //getline(fin, line);
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
    printf("%lx\\\|", itr->second);
  printf("\n");
  
}

int spatialAnalysis(vector<TraceLine *>& vecInstAddr,  MemArea memarea,  int coreNumber, int spatialResult, 
                        int out, vector<BlockInfo *>& vecBlockInfo, bool flagSetRegion, vector<pair<uint64_t, uint64_t>> setRegionAddr){
  TraceLine *ptrTraceLine;
	//initial temp value
	int * distance = new int [memarea.blockCount];   
	int * sampleDistance = new int [memarea.blockCount];   
  if(printProgress) printf("in memory analysis before array\n");
	uint32_t * refdistance = new uint32_t [memarea.blockCount]; // ref distance
	uint32_t * sampleRefdistance = new uint32_t [memarea.blockCount]; // ref distance
	uint32_t * lifetime = new uint32_t [memarea.blockCount]; // lifetime of block
	uint32_t * sampleTotalLifetime = new uint32_t [memarea.blockCount]; // total intra-sample lifetime of block
	uint32_t * sampleLifetime = new uint32_t [memarea.blockCount]; // intra-sample lifetime of block
	uint32_t * inSampleLifetimeCnt = new uint32_t [memarea.blockCount];  //Counter for intra-sample lifetime & spatial metric calculations - increase counter if a sample has valid lifetime (non-zero) for a block 
	int * minDistance = new int [memarea.blockCount]; //minimize RUD 
	int * maxDistance = new int [memarea.blockCount]; //maximum RUD
	uint32_t * totalDistance = new uint32_t [memarea.blockCount]; //Total RUD to calculate the average 
	uint32_t * totalAccess = new uint32_t [memarea.blockCount]; //Total memory access
	uint32_t * lastAccess = new uint32_t [memarea.blockCount]; //Record the temp access timing
	uint32_t * sampleLastAccess = new uint32_t [memarea.blockCount]; //Record the temp access timing
	uint32_t * stack = new uint32_t [memarea.blockCount]; //stacked distance buffer
	uint32_t stacklength = 0;
	uint32_t * sampleStack = new uint32_t [memarea.blockCount]; //stacked distance buffer
	uint32_t sampleStackLength = 0;
  uint32_t i, j;

	uint32_t * tempAccess = new uint32_t [memarea.blockCount];    //Total memory access inside the window
	uint32_t * temptotalRUD = new uint32_t [memarea.blockCount];  //Total RUD inside the window
	uint32_t * inSampleAccess = new uint32_t [memarea.blockCount];    //Total memory access inside a sample 
	uint32_t * inSampleTotalRUD = new uint32_t [memarea.blockCount];  //Total RUD inside a sample   
	uint32_t * inSampleRUDAvgCnt = new uint32_t [memarea.blockCount];  //Average counter for inSampleAvgRUD calculations - increase counter if a sample has more than one access to a block and hence has valid RUD values
	double * inSampleAvgRUD = new double [memarea.blockCount];  //Average of RUD between samples
  int curSampleId, prevSampleId; 
  prevSampleId = -1;
  curSampleId = -1;
	uint64_t previousAddr =0;  //used for offset check
  uint16_t **spatialDistance = NULL; 
  uint16_t **spatialTotalDistance =  NULL;
	int **spatialMinDistance = NULL; 
  int **spatialMaxDistance = NULL; 
  std::map<uint32_t, class SpatialRUD*> spatialRUD;
  std::map<uint32_t, class SpatialRUD*>::iterator itrSpRUD;
  SpatialRUD *curSpatialRUD; 
	uint64_t * lastAddr = new uint64_t [memarea.blockCount]; //Record the previous access address
  uint32_t * totalCAccess = new uint32_t [coreNumber];
  bool blNewSample=1;
	
	for(int i = 0; i < coreNumber; i++) totalCAccess[i] = 0;

  if(printProgress) printf("in memory analysis before for loop\n");
	for(i = 0; i < memarea.blockCount; i++){
		stack[i] = 0;
		distance[i] = 0;
		sampleStack[i] = 0;
		sampleDistance[i] = 0;
		refdistance[i] = 0;
		sampleRefdistance[i] = 0;
		lifetime[i] = 0;
		sampleTotalLifetime[i] = 0;
		sampleLifetime[i] = 0;
	  minDistance[i] = -1;
	  maxDistance[i] = -1;
	  totalDistance[i] = 0;
	  totalAccess[i] = 0;
	  lastAccess[i] = 0;	
	  sampleLastAccess[i] = 0;	
		tempAccess[i] = 0;
		temptotalRUD[i] = 0;
	  inSampleAccess[i] = 0; 
	  inSampleRUDAvgCnt[i] = 0; 
	  inSampleLifetimeCnt[i] = 0; 
	  inSampleTotalRUD[i] = 0; 
	  inSampleAvgRUD[i] = -1; 
		lastAddr[i] = 0;
	}
  printf("Spatial analysis before vector procesing address range %08lx - %08lx \n", memarea.min, memarea.max);
	int lastPage = 0;
  uint64_t totalinst = 0;
	int time = 0 ;
  if(printDebug) printf("Size of vector %ld\n", vecBlockInfo.size()); 
  //for(uint32_t k=0; k<setRegionAddr.size(); k++)
   //  printf("memcode - Spatial set min-max %d %lx-%lx \n", k, setRegionAddr[k].first, setRegionAddr[k].second);

  //printf( "in memory analysis vector size %d \n", vecInstAddr.size());
  for (uint32_t itr=0; itr<vecInstAddr.size(); itr++){
    if(printProgress) {
      if(itr%1000000==0) printf("in spatial analysis procesing trace line %d\n", itr);
    }
    ptrTraceLine=vecInstAddr.at(itr);
    //ptrTraceLine->printTraceLine();    
    previousAddr = ptrTraceLine->getLoadAddr();
    //if(printDebug) printf("previous Addr %08lx less than %d greater than %d \n", previousAddr, (previousAddr<=memarea.max), (previousAddr>=memarea.min));
    if(( (previousAddr>=memarea.min)&&(previousAddr<=memarea.max) ) ){
//        || ((flagSetRegion==1) && (previousAddr>=setRegionAddr[0].first)&&(previousAddr<=setRegionAddr[(setRegionAddr.size()-1)].second)))
      totalinst++; 
    	//totalCAccess[ptrTraceLine->getCoreNum()]++;
    	curSampleId = ptrTraceLine->getSampleId();
      if ((((itr!=0) && (curSampleId != prevSampleId))) ) 
      {
         if(printDebug) printf(" prevSampleID %d curSampleId %d\n", prevSampleId, curSampleId);
          for(i = 0; i < memarea.blockCount; i++){
            if(sampleLifetime[i]!=0)
            {
              sampleLifetime[i]++; // Lifetime = total distance between first and last access +1
              inSampleLifetimeCnt[i]++;
            }
          }
          for( itrSpRUD = spatialRUD.begin(); itrSpRUD!= spatialRUD.end(); ++itrSpRUD){
            curSpatialRUD = itrSpRUD->second;
            uint32_t curPageID = floor(itrSpRUD->first/memarea.blockCount);
            uint32_t corrPageID = floor(itrSpRUD->first%memarea.blockCount);
            if(sampleLifetime[curPageID]!=0)
            {
              curSpatialRUD->smplAvgSpatialMiddle= (( curSpatialRUD->smplAvgSpatialMiddle * (inSampleLifetimeCnt[curPageID]-1))+((double)(curSpatialRUD->smplMiddle)/((double)sampleLifetime[curPageID])))/ (double) inSampleLifetimeCnt[curPageID];
            }
            if(printDebug) printf(" curSampleId %d curPageID %d corrPageID %d inSampleLifetimeCnt[%d] %d sampleLifetime %d smplMiddle %d smplAvgSpatialMiddle %f \n", curSampleId, curPageID, corrPageID, curPageID, inSampleLifetimeCnt[curPageID], sampleLifetime[curPageID], curSpatialRUD->smplMiddle, curSpatialRUD->smplAvgSpatialMiddle);
              
            curSpatialRUD->spatialAccessMid=0;
            curSpatialRUD->smplMiddle =0;
          }
    
          for(i = 0; i < memarea.blockCount; i++){
            if (inSampleAccess[i] > 1) {
              if(inSampleAvgRUD[i] == -1)
                  inSampleAvgRUD[i] = 0; 
              inSampleRUDAvgCnt[i]++;
              // RUD Average in a sample added to total - averaged over the count of samples with valid RUD values
              inSampleAvgRUD[i] = ((inSampleAvgRUD[i]*(inSampleRUDAvgCnt[i]-1)) + (double)inSampleTotalRUD[i]/(double)(inSampleAccess[i] -1)) / inSampleRUDAvgCnt[i];
              if(printDebug) printf(" curSampleId %d inSmplAcs[%d] %d inSmplTotalRUD[%d] %d inSmplAvgRUD[%d] %f totalAccess[%d] %d totalDistance[%d] %d \n", curSampleId, i, inSampleAccess[i], i, inSampleTotalRUD[i], i, inSampleAvgRUD[i], i, totalAccess[i], i, totalDistance[i] );
            }
            inSampleTotalRUD[i] =0;
            inSampleAccess[i] = 0;
            sampleStackLength=0;
            sampleStack[i]=0;
            sampleLastAccess[i]=0;
            sampleRefdistance[i]=0;
            sampleLifetime[i]=0;
            blNewSample=1;
          }
        }
    		uint64_t instAddr =  previousAddr;
    		uint32_t pageID = 0 ;
        bool addrInRange = false;
        if(flagSetRegion==0) { 
      		pageID = floor((instAddr-memarea.min)/memarea.blockSize);
        } else {
          for (uint32_t k=0; k<setRegionAddr.size(); k++) 
          {
           if((instAddr>=setRegionAddr[k].first)&&(instAddr<=setRegionAddr[k].second))
            {
              pageID = k;
              addrInRange = true;
              continue;
            } 
          }
          if(addrInRange == false)
            continue;
        }
        
        // Number of blocks does not evenly divide address space - so the last one includes spill-over address range
    		if(pageID == memarea.blockCount) {
        //printf(" instAddr %ld memarea.min %ld blockSize %ld  pageID %d memarea.blockCount %d pageID %d\n", instAddr, memarea.min, memarea.blockSize, pageID , memarea.blockCount, pageID);
          pageID--;
        }
        //printf(" time %d curSampleId %d pageID %d\n", time , curSampleId, pageID);
  			//printf("%d: page ID %d, %08lx", totalinst, pageID , memarea.blockSize);
  			//printf("for instruction %s, area %x, total access is %d\n", ip2, (memarea.min+pageID*memarea.blockSize), totalinst);
    		totalAccess[pageID]++;
        inSampleAccess[pageID]++;
  			time = totalinst; 

  			//put the page in stack if first access
  			if (totalAccess[pageID] == 1){
  				stack[stacklength] = pageID;
  				stacklength++;
  				distance[pageID] = -1;
  			}				
  			else if(totalAccess[pageID] > 1){
  				//search the page in stack	
  				int pos = 0;
  				for(int j = 0; j< stacklength; ++j){
  					if(stack[j] == pageID) {
  						pos = j;
  						break;
  					}
  				}
  				refdistance[pageID] = time - lastAccess[pageID];
  				distance[pageID] = stacklength - pos-1;
  				//update the page in stack
  				for(int j = pos; j< stacklength-1; ++j){
    					stack[j]=stack[j+1];
  				}
  				stack[stacklength-1]= pageID;
  				//printf("time %d pageID %d, last access %d stacklength %d pos %d distance %d\n", time, pageID, lastAccess[pageID], stacklength, pos, distance[pageID]  );
  				totalDistance[pageID] = totalDistance[pageID]+distance[pageID];
  				if ((distance[pageID]>maxDistance[pageID])||(maxDistance[pageID]==-1)) maxDistance[pageID] = distance[pageID];
  				if ((distance[pageID]<minDistance[pageID])||(minDistance[pageID]==-1)) minDistance[pageID] = distance[pageID];
  			}
    		if (inSampleAccess[pageID] == 1){
    			sampleStack[sampleStackLength] = pageID;
    			sampleStackLength++;
    			sampleDistance[pageID] = -1;
    		}				
    		else if(inSampleAccess[pageID] > 1){
    			//search the page in stack	
    			int samplePos = 0;
    			for(int j = 0; j< sampleStackLength; ++j){
    				if(sampleStack[j] == pageID) {
    					samplePos = j;
    					break;
    				}
    			}
    			sampleDistance[pageID] = sampleStackLength - samplePos-1;
    			//update the page in stack
    			for(int j = samplePos; j< sampleStackLength-1; ++j){
    				sampleStack[j]=sampleStack[j+1];
    			}
    			sampleStack[sampleStackLength-1]= pageID;
          inSampleTotalRUD[pageID] = inSampleTotalRUD[pageID] + sampleDistance[pageID];  
          //if(printDebug) printf(" pageID %d, sampleRef[%d] %d samplelasAccess[%d] %d ", pageID, pageID, sampleRefdistance[pageID], pageID, sampleLastAccess[pageID]);
    			sampleRefdistance[pageID] = time - sampleLastAccess[pageID];
        }
        //SpatialDistance
        if((spatialResult == 1) && (blNewSample==0)) {
       		if(totalAccess[lastPage] >= 1) {
             //spatialNext[lastPage][pageID]++; 
             itrSpRUD = spatialRUD.find((lastPage*memarea.blockCount)+pageID);
             if (itrSpRUD != spatialRUD.end()) {
               curSpatialRUD = itrSpRUD->second;
               (curSpatialRUD->spatialNext)++;
             } else {
               curSpatialRUD = new SpatialRUD((lastPage*memarea.blockCount)+pageID);
               spatialRUD[(lastPage*memarea.blockCount)+pageID] = curSpatialRUD;
               (curSpatialRUD->spatialNext)++;
             } 
            //printf("spatialNext[%d][%d] %d \n", lastPage, pageID, spatialNext[lastPage][pageID]);
    	 		}
        }
     		lastPage = pageID;
     				
        if((spatialResult == 1) && (blNewSample==0)) {
     	  	//record the access in mid
     		  if(printDebug) printf("******* %ld: page ID %d\n", totalinst, pageID);
     			for(j = 0; j < memarea.blockCount; j++){
     				//spatialAccessTotalMid[pageID][j]+=spatialAccessMid[pageID][j];
     				//spatialAccessMid[pageID][j]=0;
            if(totalAccess[j] !=0) {
              itrSpRUD = spatialRUD.find((pageID*memarea.blockCount)+j);
              if (itrSpRUD != spatialRUD.end()) {
                curSpatialRUD = itrSpRUD->second;
              } else {
                curSpatialRUD = new SpatialRUD((pageID*memarea.blockCount)+j);
                spatialRUD[(pageID*memarea.blockCount)+j] = curSpatialRUD;
                (curSpatialRUD->spatialAccessMid) =0;
              }
              (curSpatialRUD->spatialAccessTotalMid)+=(curSpatialRUD->spatialAccessMid);
              (curSpatialRUD->smplMiddle)+=(curSpatialRUD->spatialAccessMid);
              (curSpatialRUD->spatialAccessMid) =0;
            }
     			}
    			//record spatial distance after i
    			for(i = 0; i < memarea.blockCount; i++){
    				if(sampleLastAccess[i]!=0){
    				/*
              spatialDistance[i][pageID] = time - sampleLastAccess[i];
    					if(spatialAccessMid[i][pageID]==0){
    						spatialTotalDistance[i][pageID] += spatialDistance[i][pageID];
    						if ((spatialDistance[i][pageID]>spatialMaxDistance[i][pageID])||(spatialMaxDistance[i][pageID]==-1)) 
                    spatialMaxDistance[i][pageID] = spatialDistance[i][pageID];
    						if ((spatialDistance[i][pageID]<spatialMinDistance[i][pageID])||(spatialMinDistance[i][pageID]==-1)) 
                    spatialMinDistance[i][pageID] = spatialDistance[i][pageID];
    						spatialAccess[i][pageID]++;
    					}
    					spatialAccessMid[i][pageID]++;
              */
              if (totalAccess[i] !=0) {
                itrSpRUD = spatialRUD.find((i*memarea.blockCount)+pageID);
                if (itrSpRUD != spatialRUD.end()) {
                  curSpatialRUD = itrSpRUD->second;
                } else {
                  curSpatialRUD = new SpatialRUD((i*memarea.blockCount)+pageID);
                  spatialRUD[(i*memarea.blockCount)+pageID] = curSpatialRUD;
                }
        				curSpatialRUD->spatialDistance= time - sampleLastAccess[i];
      	  			if(curSpatialRUD->spatialAccessMid==0){
    	  	  			curSpatialRUD->spatialTotalDistance += curSpatialRUD->spatialDistance;
    				  		if ((curSpatialRUD->spatialDistance > curSpatialRUD->spatialMaxDistance)||(curSpatialRUD->spatialMaxDistance==-1)) 
                      curSpatialRUD->spatialMaxDistance = curSpatialRUD->spatialDistance;
      						if ((curSpatialRUD->spatialDistance<curSpatialRUD->spatialMinDistance)||(curSpatialRUD->spatialMinDistance==-1)) 
                     curSpatialRUD->spatialMinDistance = curSpatialRUD->spatialDistance;
      						curSpatialRUD->spatialAccess++;
                }
    					  curSpatialRUD->spatialAccessMid++;
              }
              /*printf("time %d, pageID %d, i %d, lastAcs[%d] %d, spatDist[%d][%d] %d spatTotDist[%d][%d] %d spatAcs[%d][%d] %d spatAcsMid[%d][%d] %d "
                   ,time, pageID, i, i, sampleLastAccess[i], i, pageID, spatialDistance[i][pageID], i, pageID, spatialTotalDistance[i][pageID], 
                  i, pageID, spatialAccess[i][pageID], i, pageID,spatialAccessMid[i][pageID]);
               printf(" spatAcsTotMid[%d][%d] %d spatNext[%d][%d] %d refdistance[%d] %d \n", i, pageID, spatialAccessTotalMid[i][pageID], 
                     i, pageID, spatialNext[i][pageID], pageID, refdistance[pageID] );
             */ 
    				}
    			}
        }
    		lastAddr[pageID] = previousAddr;
    		lifetime[pageID] +=  refdistance[pageID];
    		sampleTotalLifetime[pageID] +=  sampleRefdistance[pageID];
    		sampleLifetime[pageID] +=  sampleRefdistance[pageID];
        //if(printDebug) printf("time %d, pageID %d, sampleRef[%d] %d samplelife[%d] %d \n", time, pageID, pageID, sampleRefdistance[pageID], pageID, sampleTotalLifetime[pageID]);
        //if(printDebug) printf(" lifetime[%d] %d\n", pageID, lifetime[pageID]);
    		lastAccess[pageID] = time; //update the page access time
    		sampleLastAccess[pageID] = time; //update the page access time
			} 
      /* FOR DEBUG purpose 
      else {
         printf(" prevSampleID %d curSampleId %d\n", prevSampleId, curSampleId);
         printf("previous Addr %08lx less than  %08lx - %d greater than  %08lx - %d \n", previousAddr, memarea.max, (previousAddr<=memarea.max), memarea.min, (previousAddr>=memarea.min));
      }*/
	
      prevSampleId = curSampleId;
      blNewSample = 0;
  } // Vector FOR loop done
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
        if(sampleLifetime[i]!=0)
        {
          sampleLifetime[i]++; // Lifetime = total distance between first and last access +1
          inSampleLifetimeCnt[i]++;
        }
      }
    for( itrSpRUD = spatialRUD.begin(); itrSpRUD!= spatialRUD.end(); ++itrSpRUD){
      curSpatialRUD = itrSpRUD->second;
      uint32_t curPageID = floor(itrSpRUD->first/memarea.blockCount);
      uint32_t corrPageID = floor(itrSpRUD->first%memarea.blockCount);
      if(printDebug) printf(" curSampleId %d curPageID %d corrPageID %d sampleLifetime %d smplMiddle %d smplAvgSpatialMiddle %f \n", curSampleId, curPageID, corrPageID, sampleLifetime[curPageID], curSpatialRUD->smplMiddle, curSpatialRUD->smplAvgSpatialMiddle);
      if(sampleLifetime[curPageID]!=0)
      {
        curSpatialRUD->smplAvgSpatialMiddle= (( curSpatialRUD->smplAvgSpatialMiddle * (inSampleLifetimeCnt[curPageID]-1))+((double)(curSpatialRUD->smplMiddle)/((double)sampleLifetime[curPageID])))/ (double) inSampleLifetimeCnt[curPageID];
      }
      if(printDebug) printf(" curSampleId %d curPageID %d corrPageID %d inSampleLifetimeCnt[%d] %d sampleLifetime %d smplMiddle %d smplAvgSpatialMiddle %f \n", curSampleId, curPageID, corrPageID, curPageID, inSampleLifetimeCnt[curPageID], sampleLifetime[curPageID], curSpatialRUD->smplMiddle, curSpatialRUD->smplAvgSpatialMiddle);
      curSpatialRUD->spatialAccessMid=0;
      curSpatialRUD->smplMiddle =0;
    }
  }
    
  if(printDebug) printf("Size of vector %ld\n", vecBlockInfo.size()); 
  if(!vecBlockInfo.empty()){
    for(i = 0; i < memarea.blockCount; i++){
      BlockInfo *curBlock = vecBlockInfo.at(i);
      //printf("BLOCK i is %d access is %ld distance %ld min %ld max %ld life %ld sampleRUD %lf\n", i,totalAccess[i], totalDistance[i] , minDistance[i], maxDistance[i], sampleTotalLifetime[i], inSampleAvgRUD[i]); 
      curBlock->setAccessRUD(totalAccess[i], totalDistance[i] , minDistance[i], maxDistance[i], sampleTotalLifetime[i], inSampleAvgRUD[i]);
      //curBlock->printBlockRUD();
    }
  }
    /* 
    for (itrSpRUD = spatialRUD.begin(); itrSpRUD!=spatialRUD.end(); ++itrSpRUD) {
           curSpatialRUD = itrSpRUD->second;
           if(printDebug) printf("Spatial hash i %d, j %d \n", (itrSpRUD->first)/memarea.blockCount, (itrSpRUD->first)%memarea.blockCount); 
      uint32_t curPageID = floor(itrSpRUD->first/memarea.blockCount);
      uint32_t corrPageID = floor(itrSpRUD->first%memarea.blockCount);
      if(printDebug) printf(" curSampleId %d curPageID %d corrPageID %d sampleLifetime %d smplMiddle %d smplAvgSpatialMiddle %f \n", curSampleId, curPageID, corrPageID, sampleLifetime[curPageID], curSpatialRUD->smplMiddle, curSpatialRUD->smplAvgSpatialMiddle);
    }
    */
    //FILE *out_file = fopen("./Spatial_RUD/100_trace/spatial_log", "a+");
    //FILE *out_file = fopen("./Spatial_RUD/minivite_results/spatial_log", "a+");

    for(i = 0; i < memarea.blockCount; i++){
      if(totalAccess[i]!=0) {  
        BlockInfo *curBlock = vecBlockInfo.at(i);
        for(j = 0; j < memarea.blockCount; j++){
          itrSpRUD = spatialRUD.find((i*memarea.blockCount)+j);
              if (itrSpRUD != spatialRUD.end()) {
                curBlock->setSpatialRUD(j, itrSpRUD->second);
                //printf("Spatial assign  i %d, j %d \n"  , i, j);
              }
        }
        //fprintf(out_file, "Page %d\n",i);
        //fprintf(out_file, "lifetime %d intra-sample lifetime %d \n", lifetime[i], sampleTotalLifetime[i]);
        /*fprintf(out_file, "Page %d spatial Next ", i);
        for(j = 0; j < memarea.blockCount; j++){
          itrSpRUD = spatialRUD.find((i*memarea.blockCount)+j);
              if (itrSpRUD != spatialRUD.end()) {
                //printf("in if loop  ");
                curSpatialRUD = itrSpRUD->second;
                 if(curSpatialRUD->spatialNext !=0 ) fprintf(out_file, "%d,%d ",j, curSpatialRUD->spatialNext);
            }
        }
        fprintf(out_file, "\nspatial Distance ");
        for(j = 0; j < memarea.blockCount; j++) {
          itrSpRUD = spatialRUD.find((i*memarea.blockCount)+j);
              if (itrSpRUD != spatialRUD.end()) {
                curSpatialRUD = itrSpRUD->second;
               if(curSpatialRUD->spatialTotalDistance!=0) fprintf(out_file, "%d,%d ", j,curSpatialRUD->spatialTotalDistance);
          }
        }
        fprintf(out_file, "\nspatial Access ");
        for(j = 0; j < memarea.blockCount; j++) {
          itrSpRUD = spatialRUD.find((i*memarea.blockCount)+j);
              if (itrSpRUD != spatialRUD.end()) {
                curSpatialRUD = itrSpRUD->second;
                if(curSpatialRUD->spatialAccess!=0) fprintf(out_file, "%d,%d ", j,curSpatialRUD->spatialAccess);
          }
        }
        fprintf(out_file, "\nspatial middle ");
        for(j = 0; j < memarea.blockCount; j++) {
          itrSpRUD = spatialRUD.find((i*memarea.blockCount)+j);
              if (itrSpRUD != spatialRUD.end()) {
                curSpatialRUD = itrSpRUD->second;
                if(curSpatialRUD->spatialAccessTotalMid !=0) fprintf(out_file, "%d,%d ", j,curSpatialRUD->spatialAccessTotalMid); 
          }
        }
        fprintf(out_file, "\n");
        */
       
    }
  }
  //fclose(out_file); 

	//for(int i = 0; i<coreNumber; i++){
	//	printf("core %d, total access is %d\n", i, totalCAccess[i]);
	//}
	delete[] distance;
	delete[] sampleDistance ;
  delete[] refdistance;
  delete[] sampleRefdistance;
  delete[] sampleStack;
	delete[] sampleTotalLifetime; 
	delete[] sampleLifetime ;
	delete[] inSampleLifetimeCnt; 
	delete[] minDistance;
	delete[] maxDistance;
  delete[] lifetime ;
	delete[] totalDistance; 
	delete[] totalAccess; 
  delete[] lastAccess; 
  delete[] sampleLastAccess; 
	delete[] inSampleAvgRUD;  
  delete[] stack; 
	delete[] tempAccess; 
	delete[] temptotalRUD; 
	delete[] inSampleAccess; 
  delete[] inSampleTotalRUD; 
	delete[] inSampleRUDAvgCnt; 
  delete[] totalCAccess;
  delete[] lastAddr; 
    if(spatialResult == 1) {
	delete[] spatialDistance; 
	delete[] spatialTotalDistance;
	delete[] spatialMinDistance;
	delete[] spatialMaxDistance;
    }
 return 0;
}
#endif
