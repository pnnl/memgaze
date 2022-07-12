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
void readTrace(string filename, int *intTotalTraceLine,  vector<TraceLine *>& vecInstAddr)
{
	// File pointer 
  fstream fin; 
  // Open an existing file 
  fin.open(filename, ios::in); 
  // read the state line
  string line,ip,addr,core, inittime, sampleIdStr;
  uint64_t insPtrAddr, loadAddr, instTime; 
  uint32_t  coreNum; 
  uint32_t  sampleId; 
	core = "0";
  TraceLine *ptrTraceLine;
  while (!fin.eof()){
  	getline(fin, line);
		std::stringstream s(line);
		if(getline(s,ip,' ')){
      (*intTotalTraceLine)++;
	  	//if(ip.compare(ip2) == 0){
		  // printf("find the instruction from input file\n");
   		getline(s,addr,' ');
      loadAddr = stoull(addr,0,16);
      // Remove for miniVite trace - no need to check, the trace is final
      // But had to add it in - because there were some addresses below this range
      // uint64_t addrThreshold = stoull("FFFFFFFF",0,16);
        uint64_t instThreshold = stoull("FFFFFFFFFF",0,16); // Added for invalid IP address checks 
        uint64_t addrHighThreshold = stoull("8fffffffffff", 0, 16); // Added for load addresses in range AXX.. and FFXX..
			if ((loadAddr > addrThreshold) && (loadAddr < addrHighThreshold)) {
        insPtrAddr = stoull(ip,0,16); 
        // Added because miniVite trace (possibly) has corrupted data for the last sample
        // It has heap addresses as values for instruction addresses
      		getline(s,core,' ');
          coreNum= stoi(core);
      		getline(s,inittime,' ');
          instTime= stoull(inittime);
      		getline(s,sampleIdStr,' ');
          sampleId= stoull(sampleIdStr);
          // USED in GAP verification of access counts - experimental
          //uint64_t GAP_pr_low_ip = stoull("30a32c", 0, 16);
          //uint64_t GAP_pr_high_ip= stoull("30a578", 0, 16);
          //uint64_t GAP_cc_low_ip = stoull("300ad0", 0, 16); // [0x300ad0-0x300be6)
          //uint64_t GAP_cc_high_ip= stoull("300be6", 0, 16);
          //uint64_t GAP_CSR_low = stoull("3008fe", 0, 16); // [0x300ad0-0x300be6)
          //uint64_t GAP_CSR_high = stoull("300a5b", 0, 16);
          //if((insPtrAddr >= GAP_pr_low_ip) && (insPtrAddr < GAP_pr_high_ip))
          //if((insPtrAddr >= GAP_cc_low_ip) && (insPtrAddr < GAP_cc_high_ip))
          //if((insPtrAddr >= GAP_CSR_low) && (insPtrAddr < GAP_CSR_high))
          //{
            ptrTraceLine=new TraceLine(insPtrAddr, loadAddr, coreNum, instTime, sampleId);
            vecInstAddr.push_back(ptrTraceLine);
          //}
      }
    }
  }
  /*for(int i=0; i<vecInstAddr.size(); i++){
    ptrTraceLine=vecInstAddr.at(i);
    ptrTraceLine->printTraceLine();    
  }
  */
}
void getInstInRange(vector<TraceLine *>& vecInstAddr,MemArea memarea)
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
    printf("%ld\t0x%lx \n", itr->first, itr->second);
  }
  //for (itr=sortInstr.begin(); itr!=sortInstr.end(); itr++)
   // printf("%lx\\\|", itr->second);
  //printf("\n");
  
}

//Use to do the dynamic analysis about the RUD, temp RUD and find the outline instruction
//Data stored as <IP addr core initialtime\n>
//void memoryAnalysis(string filename, MemArea memarea,  int coreNumber, int spatialResult, int out, double *Rud, int *pageAccess){
int memoryAnalysis(vector<TraceLine *>& vecInstAddr,  MemArea memarea,  int coreNumber, int spatialResult, 
                        int out, vector<BlockInfo *>& vecBlockInfo){
  TraceLine *ptrTraceLine;
	//initial temp value
	int * distance = new int [memarea.blockCount];   
	int * sampleDistance = new int [memarea.blockCount];   
  if(printProgress) printf("in memory analysis before array\n");
	int * refdistance = new int [memarea.blockCount]; // ref distance
	int * lifetime = new int [memarea.blockCount]; // lifetime of block
	int * minDistance = new int [memarea.blockCount]; //minimize RUD 
	int * maxDistance = new int [memarea.blockCount]; //maximum RUD
	uint32_t * totalDistance = new uint32_t [memarea.blockCount]; //Total RUD to calculate the average 
	uint32_t * totalAccess = new uint32_t [memarea.blockCount]; //Total memory access
	int * lastAccess = new int [memarea.blockCount]; //Record the temp access timing
	int * stack = new int [memarea.blockCount]; //stacked distance buffer
	int stacklength = 0;
	int * sampleStack = new int [memarea.blockCount]; //stacked distance buffer
	int sampleStackLength = 0;
  uint32_t i, j;

	int ** tempRUD = new int * [memarea.blockCount]; //Record the temp RUD for window analysis
	uint64_t ** tempIP = new uint64_t * [memarea.blockCount]; //Record the temp instruction IP for window analysis
	uint64_t ** tempAddr = new uint64_t * [memarea.blockCount]; //Record the temp Addr for window analysis
	int * tempAccess = new int [memarea.blockCount];    //Total memory access inside the window
	int * temptotalRUD = new int [memarea.blockCount];  //Total RUD inside the window
	int * inSampleAccess = new int [memarea.blockCount];    //Total memory access inside a sample 
	int * inSampleTotalRUD = new int [memarea.blockCount];  //Total RUD inside a sample   
	int * inSampleRUDAvgCnt = new int [memarea.blockCount];  //Average counter for inSampleAvgRUD calculations - increase counter if a sample has more than one access to a block and hence has valid RUD values
	double * inSampleAvgRUD = new double [memarea.blockCount];  //Average of RUD between samples
  uint32_t curSampleId, prevSampleId; 
  prevSampleId = 0;
	uint64_t * Outstand = new uint64_t [WINDOW];  //Record the outstand instruction
	uint64_t previousAddr =0;  //used for offset check
  	uint16_t **spatialDistance = NULL; 
  	uint16_t **spatialTotalDistance =  NULL;
	  int **spatialMinDistance = NULL; 
  	int **spatialMaxDistance = NULL; 
  	uint16_t **spatialAccess = NULL; 
  	uint16_t **spatialAccessMid = NULL; 
  	uint16_t **spatialAccessTotalMid = NULL; 
  	uint16_t **spatialNext = NULL; 
  if(spatialResult == 1) {
  	spatialDistance = new uint16_t * [memarea.blockCount]; // for each page, record the access distance with other pages
  	spatialTotalDistance = new uint16_t * [memarea.blockCount];
	  spatialMinDistance = new int * [memarea.blockCount];
  	spatialMaxDistance = new int * [memarea.blockCount];
  	spatialAccess = new uint16_t * [memarea.blockCount]; //for each page, record the access time with other pages
  	spatialAccessMid = new uint16_t * [memarea.blockCount];//record the access time within RUD
  	spatialAccessTotalMid = new uint16_t * [memarea.blockCount];
  	spatialNext = new uint16_t  * [memarea.blockCount]; // for each page, record the count of next
  }
	//int64_t * addrDiff = new int64_t [memarea.blockCount];   //addr distance
	//int64_t * addrTotalDiff = new int64_t [memarea.blockCount];   //addr distance
	//int64_t * addrMinDiff = new int64_t [memarea.blockCount]; //minimize addr distance
	//int64_t * addrMaxDiff = new int64_t [memarea.blockCount]; //maximum addr distance
	uint64_t * lastAddr = new uint64_t [memarea.blockCount]; //Record the previous access address
  int * totalCAccess = new int [coreNumber];
	
	for(unsigned int i = 0; i < WINDOW; i++) Outstand[i] = 0;
	for(int i = 0; i < coreNumber; i++) totalCAccess[i] = 0;

  if(printProgress) printf("in memory analysis before for loop\n");
	for(i = 0; i < memarea.blockCount; i++){
		stack[i] = 0;
		distance[i] = 0;
		sampleStack[i] = 0;
		sampleDistance[i] = 0;
		refdistance[i] = 0;
		lifetime[i] = 0;
	  minDistance[i] = -1;
	  maxDistance[i] = -1;
	  totalDistance[i] = 0;
	  totalAccess[i] = 0;
	  lastAccess[i] = 0;	
		tempRUD[i] = new int [WINDOW]; // set sample rate = 1000
		tempIP[i] = new uint64_t [WINDOW];
		tempAddr[i] = new uint64_t [WINDOW];
		tempAccess[i] = 0;
		temptotalRUD[i] = 0;
	  inSampleAccess[i] = 0; 
	  inSampleRUDAvgCnt[i] = 0; 
	  inSampleTotalRUD[i] = 0; 
	  inSampleAvgRUD[i] = -1; 
    if(spatialResult == 1) {
		  spatialDistance[i] = new uint16_t [memarea.blockCount]; 
  		spatialTotalDistance[i] = new uint16_t [memarea.blockCount]; 
	  	spatialMinDistance[i] = new int [memarea.blockCount];
  		spatialMaxDistance[i] = new int [memarea.blockCount];
  		spatialAccess[i] = new uint16_t  [memarea.blockCount];
  		spatialAccessMid[i] = new uint16_t  [memarea.blockCount];
  		spatialAccessTotalMid[i] = new uint16_t  [memarea.blockCount];
	  	spatialNext[i] = new  uint16_t  [memarea.blockCount];
  		for(j = 0; j < memarea.blockCount; j++){
	  		spatialDistance[i][j] = 0; 
		  	spatialTotalDistance[i][j] = 0; 
			  spatialMinDistance[i][j] = -1;
  			spatialMaxDistance[i][j] = -1;
	  		spatialAccess[i][j] = 0;
		  	spatialAccessMid[i][j] = 0;
			  spatialAccessTotalMid[i][j] = 0;
  			spatialNext[i][j] = 0; 
		  }
    }
		/*
    addrDiff[i] = 0;
		addrTotalDiff[i] = 0;
		addrMinDiff[i] = 0;
		addrMaxDiff[i] = 0;
    */
		lastAddr[i] = 0;
	}

  if(printProgress) printf("in memory analysis before vector procesing \n");
	int spatialDiff_O = 0;
	int spatialDiff_W = 0;
	int lastPage = 0;
  uint64_t totalinst = 0;
  int strideDiff = 0;
	int time = 0 ;
    //printf( "in memory analysis vector size %d \n", vecInstAddr.size());
  for (uint32_t itr=0; itr<vecInstAddr.size(); itr++){
      if(printProgress) {
        if(itr%1000000==0) printf("in memory analysis procesing inst %d\n", itr);
      }
      ptrTraceLine=vecInstAddr.at(itr);
      //ptrTraceLine->printTraceLine();    
      previousAddr = ptrTraceLine->getLoadAddr();
     	//if(printDebug) printf("previous Addr %08lx less than %d greater than %d \n", previousAddr, (previousAddr<=memarea.max), (previousAddr>=memarea.min));
    	if((previousAddr>=memarea.min)&&(previousAddr<=memarea.max)){
          totalinst++; 
    			totalCAccess[ptrTraceLine->getCoreNum()]++;
    			curSampleId = ptrTraceLine->getSampleId();
          if ( ( ((itr!=0) && (curSampleId != prevSampleId))) ) 
          {
            if(printDebug) printf(" ****************** prevSampleID %d curSampleId %d\n", prevSampleId, curSampleId);
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
            }
          }
    			uint64_t instAddr =  previousAddr;
    			//check the pageID
    			uint32_t pageID = 0 ;
    			pageID = floor((instAddr-memarea.min)/memarea.blockSize);
          // This should not happen - unsure why it was included in original code
          // Changing into error assetion
    		  if(pageID == memarea.blockCount) {
            return -1; 
            //pageID--;
          }
          //printf(" time %d curSampleId %d pageID %d\n", time , curSampleId, pageID);
    			//printf("%d: page ID %d, %08lx", totalinst, pageID , memarea.blockSize);
    			//printf("for instruction %s, area %x, total access is %d\n", ip2, (memarea.min+pageID*memarea.blockSize), totalinst);
    			totalAccess[pageID]++;
	        inSampleAccess[pageID]++;
    			//tempAccess[pageID]++;
    			//using the trace squence as timing
    			//time = stoi(initialtime);
    			time = totalinst; 

    			//RUD
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
    				//printf("pageID %d, last access %d stacklength %d pos %d distance %d\n", pageID, lastAccess[pageID], stacklength, pos, distance[pageID]  );
    				totalDistance[pageID] = totalDistance[pageID]+distance[pageID];
    				if ((distance[pageID]>maxDistance[pageID])||(maxDistance[pageID]==-1)) maxDistance[pageID] = distance[pageID];
    				if ((distance[pageID]<minDistance[pageID])||(minDistance[pageID]==-1)) minDistance[pageID] = distance[pageID];
    
    				//record the intermediate RUD for window analysis
    				//temptotalRUD[pageID] = temptotalRUD[pageID]+ distance[pageID];
    				//tempRUD[pageID][tempAccess[pageID]] = distance[pageID];
    				//tempIP[pageID][tempAccess[pageID]] = ptrTraceLine->getInsPtAddr();
    				//tempAccess[pageID]++;
    
    				//SpatialDistance within the pages
    				//TRIAL might be one of the trial and error metrics developed
    				//TRIAL MIN distance shows -10 for access address value of 160 followed by 150
    				//if(previousAddr>=lastAddr[pageID]) 
    				//addrDiff[pageID] = previousAddr - lastAddr[pageID];
    				//else addrDistance[pageID] = lastAddr[pageID] - previousAddr;
    				//printf("%08lx,%08lx,%08lx\n", previousAddr, lastAddr[pageID], addrDistance[pageID]);
    				/*
    				addrTotalDiff[pageID] += addrDiff[pageID];
    				if ((addrDiff[pageID]>addrMaxDiff[pageID])||(addrMaxDiff[pageID]==0)) addrMaxDiff[pageID] = addrDiff[pageID];
    				if ((addrDiff[pageID]<addrMinDiff[pageID])||(addrMinDiff[pageID]==0)) addrMinDiff[pageID] = addrDiff[pageID];
            */
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
          }

     			//SpatialDistance
          if(spatialResult == 1) {
       			//record the count of spatial next;
       			if(totalAccess[lastPage] >= 1) spatialNext[lastPage][pageID]++; 
            //printf("spatialNext[%d][%d] %d \n", lastPage, pageID, spatialNext[lastPage][pageID]);
    	  			
            //TRIAL might be one of the trial and error metrics developed
            //TRIAL prints negative values sometimes
       			if(time > 1){
       			  strideDiff = pageID - lastPage;
       			  spatialDiff_O += (pageID - lastPage);
     	  		  spatialDiff_W += (pageID - lastPage);
     		  	}
          }
     			  lastPage = pageID;
     				
          if(spatialResult == 1) {
     			//record the access in mid
     			if(printDebug) printf("******* %ld: page ID %d\n", totalinst, pageID);
     			for(j = 0; j < memarea.blockCount; j++){
     				spatialAccessTotalMid[pageID][j]+=spatialAccessMid[pageID][j];
     				spatialAccessMid[pageID][j]=0;
     			}

    			//record spatial distance after i
    			for(i = 0; i < memarea.blockCount; i++){
    				if(lastAccess[i]!=0){
    					spatialDistance[i][pageID] = time - lastAccess[i];
    					if(spatialAccessMid[i][pageID]==0){
    						spatialTotalDistance[i][pageID] += spatialDistance[i][pageID];
    						if ((spatialDistance[i][pageID]>spatialMaxDistance[i][pageID])||(spatialMaxDistance[i][pageID]==-1)) 
                    spatialMaxDistance[i][pageID] = spatialDistance[i][pageID];
    						if ((spatialDistance[i][pageID]<spatialMinDistance[i][pageID])||(spatialMinDistance[i][pageID]==-1)) 
                    spatialMinDistance[i][pageID] = spatialDistance[i][pageID];
    						spatialAccess[i][pageID]++;
    					}
    					spatialAccessMid[i][pageID]++;
              if(printDebug) printf("i %d, lastAcs[%d] %d, spatDist[%d][%d] %d spatTotalDist[%d][%d] %d spatAcs[%d][%d] %d spatAcsMid[%d][%d] %d "
                   ,i, i, lastAccess[i], i, pageID, spatialDistance[i][pageID], i, pageID, spatialTotalDistance[i][pageID], 
                  i, pageID, spatialAccess[i][pageID], i, pageID,spatialAccessMid[i][pageID]);
              if(printDebug) printf(" spatAcsTotalMid[%d][%d] %d spatNext[%d][%d] %d refdistance[%d] %d \n", i, pageID, spatialAccessTotalMid[i][pageID], 
                     i, pageID, spatialNext[i][pageID], pageID, refdistance[pageID]);
    				}
    			}
          }
    
    			lastAddr[pageID] = previousAddr;
    			lifetime[pageID] +=  refdistance[pageID];
          //if(printDebug) printf(" lifetime[%d] %d\n", pageID, lifetime[pageID]);
    			lastAccess[pageID] = time; //update the page access time
    			//if (out == 1){
    			//RecordMem(line, pageID, distance[pageID], strideDiff);
    			//}
			}	
    /*
		//Window analysis 
		if(((totalinst%WINDOW) == 0) || (totalinst==vecInstAddr.size())){
		//printf("\n=====Analysis %d inst size %d ====\n",totalinst+1, vecInstAddr.size());
      int windowRUD = 0;
      int analysisAccess = 0;
      int minPage = memarea.blockCount;
      int maxPage = 0;
      int instWindow = totalinst-WINDOW;
      if(totalinst == vecInstAddr.size())
        instWindow = ((totalinst/WINDOW)* WINDOW) +1;
      //printf(" totalinst %ld  WINDOW %ld DIV %ld instWindow %d\n", totalinst, WINDOW, (totalinst/WINDOW), instWindow);
			printf("\n=====Analysis from %d to %ld inst====\n",instWindow>0?instWindow:1, totalinst);
			printf("Pages Spatial Distance within this Windows is %d\n", spatialDiff_W);
			
			for(i = 0; i < memarea.blockCount; i++){
				if( tempAccess[i] != 0){
					printf("\nPage %d, area %08lx-%08lx, total access is %d, temp access is %d\n", i, (memarea.min+i*memarea.blockSize), (memarea.min+(i+1)*memarea.blockSize), totalAccess[i], tempAccess[i]);
        if(i <= minPage) minPage = i;
        if(i >= maxPage) maxPage = i;                 
     
					if (tempAccess[i] > 1 ){
						printf("========temp RUD in instructions from %d to %ld =======\n", instWindow>0?instWindow:1,totalinst);
						printf("average total RUD is %f\n", (double)totalDistance[i]/(totalAccess[i]-1));
						printf("average window RUD is %f\n", (double)temptotalRUD[i]/tempAccess[i]);
            windowRUD = windowRUD + temptotalRUD[i];
            analysisAccess = analysisAccess + tempAccess[i];
            
						double TotalError = 0;
						double RUDError = 0;
						for(int j =0; j < tempAccess[i]; j++){
							RUDError = ((double)tempRUD[i][j] - (double)temptotalRUD[i]/tempAccess[i]); //calculated bias
							TotalError += RUDError*RUDError; //record squred errors
							if(RUDError > RUDthreshold){ //outstanding instruction
								//Check if the instruction is already recorded
								//TRIAL works in fft_trace - might be one of the trial and error metric
								int check = 0;
								for (int k=0; k<OutstandLength ;k++){
									if (Outstand[k] == tempIP[i][j]) check =1;
								}
								if (check == 0){
									Outstand[OutstandLength] = tempIP[i][j];
									OutstandLength++;
								}
								
							}
						}
						printf("Sample Variance of RUD is %f\n", TotalError/tempAccess[i]); // MSE
						printf("========Spatial Analysis within the Page=======\n");
					  //printf("%s%x\n", x<0 ? "-" : "", x<0 ? -(unsigned)x : x);
						printf("min address difference within the page is %s%08lx\n", addrMinDiff[i] <0 ? "-" : " ", addrMinDiff[i] <0 ? -(unsigned)addrMinDiff[i] : addrMinDiff[i]);
						printf("max address difference within the page is %s%08lx\n", addrMaxDiff[i] <0 ? "-" : " ", addrMaxDiff[i] <0 ? -(unsigned)addrMaxDiff[i] : addrMaxDiff[i]);
						long long int x = addrTotalDiff[i]/(tempAccess[i]);
						printf("average addr difference within the page is %s%08lx\n", x<0 ? "-" : "", x<0 ? -(unsigned)x : x);
            
					}
				}
			}
			//TRIAL works in fft_trace - might be one of the trial and error metric
			printf("=========Outstanding instruction==========\n[");
			for (int k=0; k<OutstandLength ;k++){
				printf("%08lx;",Outstand[k]);
			}
			printf("]\n");
			OutstandLength=0;
      double centrePage=0;
      for(i = 0; i < memarea.blockCount; i++){
        centrePage = centrePage + (double)i*(double)tempAccess[i]/(double)analysisAccess;
        //addrMinDiff[i]=0;
				//addrMaxDiff[i]=0;
				//addrTotalDiff[i] = 0;
				temptotalRUD[i] = 0;
				tempAccess[i] = 0;
      }
      double winRUD = (double)windowRUD/(double)analysisAccess;
      printf("Window RUD is %f\n", winRUD );
      if(out==1) 
        RecordMem("Window", totalinst, winRUD, (double)spatialDiff_W, centrePage, minPage, maxPage);
      spatialDiff_W = 0;
		} 
    */
      prevSampleId = curSampleId;
  }
  // Add the last sample RUD to the averages
  for(i = 0; i < memarea.blockCount; i++){
    if (inSampleAccess[i] > 1)
    {
      if(inSampleAvgRUD[i] == -1)
        inSampleAvgRUD[i] = 0; 
      inSampleRUDAvgCnt[i]++;
      // RUD Average in a sample added to total - averaged over the count of samples with valid RUD values
      inSampleAvgRUD[i] = (inSampleAvgRUD[i]*(inSampleRUDAvgCnt[i]-1) + (double)inSampleTotalRUD[i]/(double)(inSampleAccess[i] -1)) / inSampleRUDAvgCnt[i];
    }
    //if(inSampleAvgRUD[i] != -1)
    //printf(" inSampleAccess[%d] %ld inSampleTotalRUD[%d] %ld inSampleAvgRUD[%d] %f \n", i, inSampleAccess[i],
     //                             i, inSampleTotalRUD[i], i, inSampleAvgRUD[i] );
  }
    
  if(printDebug) printf("Size of vector %ld\n", vecBlockInfo.size()); 
  if(!vecBlockInfo.empty()){
    for(i = 0; i < memarea.blockCount; i++){
      BlockInfo *curBlock = vecBlockInfo.at(i);
      //if(totalAccess[i]!=0) printf("BLOCK i is %d access is %ld\n", i,totalAccess[i]); 
      curBlock->setAccessRUD(totalAccess[i], totalDistance[i] , minDistance[i], maxDistance[i], lifetime[i], inSampleAvgRUD[i]);
          if(spatialResult == 1) {
      curBlock->setSpatialAccess(spatialAccess[i]);
      curBlock->setSpatialNext(spatialNext[i]);
      curBlock->setTotalAccessAllBlocks(totalAccess);
      curBlock->setSpatialMiddle(spatialAccessTotalMid[i]);
      curBlock->setTotalSpatialDist(spatialTotalDistance[i]);
      curBlock->setMinSpatialDist(spatialMinDistance[i]);
      curBlock->setMaxSpatialDist(spatialMaxDistance[i]);
        }
    }
  }

	//for(int i = 0; i<coreNumber; i++){
	//	printf("core %d, total access is %d\n", i, totalCAccess[i]);
	//}
	for(i = 0; i < memarea.blockCount; i++){
    if(spatialResult == 1) {
		delete(spatialDistance[i]);
		delete(spatialTotalDistance[i]);
		delete(spatialMinDistance[i]);
		delete(spatialMaxDistance[i]);
		delete(spatialAccess[i]);
		delete(spatialAccessMid[i]);
		delete(spatialAccessTotalMid[i]);
		delete(spatialNext[i]);
    }
		delete(tempRUD[i]);
		delete(tempIP[i]);
		delete(tempAddr[i]);
  }
  delete(refdistance);
  delete(lifetime );
  delete(minDistance );
  delete(maxDistance); 
	delete(totalDistance); 
	delete(totalAccess); 
  delete(lastAccess); 
  delete(stack); 

	delete(tempRUD); 
	delete(tempIP); 
	delete(tempAddr); 
	delete(tempAccess); 
	delete(temptotalRUD); 
	delete(inSampleAccess); 
  delete(inSampleTotalRUD); 
	delete(inSampleRUDAvgCnt); 
	delete(Outstand); 
    if(spatialResult == 1) {
	delete(spatialDistance); 
	delete(spatialTotalDistance);
	delete(spatialMinDistance);
	delete(spatialMaxDistance);
	delete(spatialAccess); 
    }
 return 0;
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
  uint32_t curSampleId, prevSampleId; 
  prevSampleId = 0;
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

  if(printProgress) printf("in memory analysis before vector procesing \n");
	int lastPage = 0;
  uint64_t totalinst = 0;
	int time = 0 ;
  //for(uint32_t k=0; k<setRegionAddr.size(); k++)
   //  printf("memcode - Spatial set min-max %d %lx-%lx \n", k, setRegionAddr[k].first, setRegionAddr[k].second);

  //printf( "in memory analysis vector size %d \n", vecInstAddr.size());
  for (uint32_t itr=0; itr<vecInstAddr.size(); itr++){
    if(printProgress) {
      if(itr%1000000==0) printf("in memory analysis procesing inst %d\n", itr);
    }
    ptrTraceLine=vecInstAddr.at(itr);
    //ptrTraceLine->printTraceLine();    
    previousAddr = ptrTraceLine->getLoadAddr();
    //if(printDebug) printf("previous Addr %08lx less than %d greater than %d \n", previousAddr, (previousAddr<=memarea.max), (previousAddr>=memarea.min));
    if(( (previousAddr>=memarea.min)&&(previousAddr<=memarea.max) ) ){
//        || ((flagSetRegion==1) && (previousAddr>=setRegionAddr[0].first)&&(previousAddr<=setRegionAddr[(setRegionAddr.size()-1)].second))){
      totalinst++; 
    	totalCAccess[ptrTraceLine->getCoreNum()]++;
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
        
        // This should not happen - unsure why it was included in original code
        // Changing into error assetion
    		if(pageID == memarea.blockCount) {
          return -1; 
          //pageID--;
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
  				//printf("pageID %d, last access %d stacklength %d pos %d distance %d\n", pageID, lastAccess[pageID], stacklength, pos, distance[pageID]  );
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
          if(printDebug) printf(" pageID %d, sampleRef[%d] %d samplelasAccess[%d] %d ", pageID, pageID, sampleRefdistance[pageID], pageID, sampleLastAccess[pageID]);
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
             /* printf("time %d, pageID %d, i %d, lastAcs[%d] %d, spatDist[%d][%d] %d spatTotDist[%d][%d] %d spatAcs[%d][%d] %d spatAcsMid[%d][%d] %d "
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
        if(printDebug) printf("time %d, pageID %d, sampleRef[%d] %d samplelife[%d] %d \n", time, pageID, pageID, sampleRefdistance[pageID], pageID, sampleTotalLifetime[pageID]);
        //if(printDebug) printf(" lifetime[%d] %d\n", pageID, lifetime[pageID]);
    		lastAccess[pageID] = time; //update the page access time
    		sampleLastAccess[pageID] = time; //update the page access time
			}	
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
    //if(inSampleAvgRUD[i] != -1)
    //printf(" inSampleAccess[%d] %ld inSampleTotalRUD[%d] %ld inSampleAvgRUD[%d] %f \n", i, inSampleAccess[i],
    //                                  i, inSampleTotalRUD[i], i, inSampleAvgRUD[i] );
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
      //printf("BLOCK i is %d access is %ld\n", i,totalAccess[i]); 
      curBlock->setAccessRUD(totalAccess[i], totalDistance[i] , minDistance[i], maxDistance[i], sampleTotalLifetime[i], inSampleAvgRUD[i]);
    }
  }
  
    /*for (itrSpRUD = spatialRUD.begin(); itrSpRUD!=spatialRUD.end(); ++itrSpRUD) {
           curSpatialRUD = itrSpRUD->second;
           printf(" i %d, j %d \n", (itrSpRUD->first)/memarea.blockCount, (itrSpRUD->first)%memarea.blockCount); 
    }*/
    //FILE *out_file = fopen("/files0/suri836/Spatial_RUD/100_trace/spatial_log", "a+");
    //FILE *out_file = fopen("/files0/suri836/Spatial_RUD/minivite_results/spatial_log", "a+");

    for(i = 0; i < memarea.blockCount; i++){
      if(totalAccess[i]!=0) {  
        BlockInfo *curBlock = vecBlockInfo.at(i);
        for(j = 0; j < memarea.blockCount; j++){
          itrSpRUD = spatialRUD.find((i*memarea.blockCount)+j);
              if (itrSpRUD != spatialRUD.end()) {
                curBlock->setSpatialRUD(j, itrSpRUD->second);
                
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
    /*
	for(i = 0; i < memarea.blockCount; i++){
    if(spatialResult == 1) {
		delete(spatialDistance[i]);
		delete(spatialTotalDistance[i]);
		delete(spatialMinDistance[i]);
		delete(spatialMaxDistance[i]);
		delete(spatialAccess[i]);
		delete(spatialAccessMid[i]);
		delete(spatialAccessTotalMid[i]);
		delete(spatialNext[i]);
    }
  }
    */
  delete(refdistance);
  delete(lifetime );
	delete(totalDistance); 
	delete(totalAccess); 
  delete(lastAccess); 
  delete(sampleLastAccess); 
  delete(stack); 

	delete(tempAccess); 
	delete(temptotalRUD); 
	delete(inSampleAccess); 
  delete(inSampleTotalRUD); 
	delete(inSampleRUDAvgCnt); 
    if(spatialResult == 1) {
	delete(spatialDistance); 
	delete(spatialTotalDistance);
	delete(spatialMinDistance);
	delete(spatialMaxDistance);
    }
 return 0;
}
#endif
