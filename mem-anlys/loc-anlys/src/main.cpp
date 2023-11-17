#include "memoryanalysis.h"
#include "memorymodeling.h"
#include "hyperloglog.hpp"

using std::list;
// Global variables for threshold values
int32_t lvlConstBlockSize = 2; // Level for setting constant blocksize in zoom
double zoomThreshold = 0.10; // Threshold for access count - to include in zoom
//double zoomThreshold = 0.05 ; // Minivite uses this - Cluster paper data used different threshold values - to match regions to paper use this value 
uint64_t heapAddrEnd ; // 
uint64_t cacheLineWidth; // Added for ZoomRUD analysis - option to set last level's block width
uint64_t zoomLastLvlPageWidth ; // Added for ZoomRUD analysis - option to set last Zoom level's page width
uint64_t OSPageSize = 16384;
uint64_t levelOneSize ;

/********************************************************************************
Zoom only uses Access counts
Rud - not used, not set when getAccessCount is used, defaults to -1 for all
**********************************************************************************/
void findHotPage( MemArea memarea, int zoomOption, vector<double> Rud, 
                  vector<uint32_t>& pageTotalAccess, uint32_t thresholdTotAccess, int *zoomin, Memblock curPage, 
                  std::list<Memblock >& zoomPageList)
{
  printf("findHotPage zoomOption %d  for %s memarea.min = %08lx memarea.max = %08lx memarea.blockSize = %ld \n",  zoomOption, 
                    (curPage.strID).c_str(), memarea.min, memarea.max, memarea.blockSize);
  uint32_t totalAccessParent = 0;
  int zoominaccess = 0;
  uint32_t wide = 0; 
  string strNodeId; 
  uint32_t i =0;
  uint32_t j =0;
    
  vector <uint32_t> sortPageAccess;
  vector<pair<uint32_t, uint32_t>> vecAccessBlockId;
  vector<uint32_t> vecBlockId;
  sortPageAccess.clear();
  for(i = 0; i< memarea.blockCount; i++){
    totalAccessParent +=  pageTotalAccess.at(i);
    sortPageAccess.push_back(pageTotalAccess.at(i));
  }
  sort(sortPageAccess.begin(), sortPageAccess.end(), greater<>());
  
  //printf("sortPageAccess %d %d \n", sortPageAccess.at(0), sortPageAccess.at(1) );
  uint32_t childCount=0;
  // Contiguous hot pages
  if (zoomOption == 0)
  {
    for(i = 0; i<memarea.blockCount; i++  ){
     zoominaccess = 0;
	   wide = 0; 
	   //merge contiguous memory regions
	   for(j = i; j<memarea.blockCount; j++){
	     if(pageTotalAccess.at(j) != 0){
	  		  zoominaccess = zoominaccess + pageTotalAccess.at(j);
	  		  wide++;
	  		} else {
	  			  break;
	  		}
	   } 
     //printf(" in contiguous hot page i %d, wide %d access for area %d \n", i, wide, zoominaccess);
     //if(((double)zoominaccess>(double)totalAccessParent*0.4)&&((double)zoominaccess>(double)totalAccess*0.1)&&(wide!=memarea.blockCount))
     if(((double)zoominaccess>(double)(zoomThreshold*totalAccessParent))&&(wide!=memarea.blockCount))
     //if(((double)zoominaccess>6000)&&(wide!=memarea.blockCount))
     {
        Memblock hotpage;
        if(curPage.strID.compare("R")==0) 
           strNodeId ='A'+childCount; 
        else
           strNodeId = curPage.strID+std::to_string(childCount);
        hotpage.strID = strNodeId; 
	  	  hotpage.min = memarea.min+memarea.blockSize*(i);
        hotpage.max = memarea.min+(memarea.blockSize*(i+wide))-1;
	  	  hotpage.level = curPage.level+1;
        //printf("zoom in i=%d  wide = %d hotpage min = %08lx max %08lx nodeid = %s\n", i, wide, hotpage.min,hotpage.max,strNodeId);
        hotpage.leftPid = i;
        hotpage.rightPid = i+wide-1;
		    hotpage.strParentID = curPage.strID;
			  zoomPageList.push_back(hotpage);
			  (*zoomin)++;
        childCount++;
        //printf("zoomin value %d childCount %d\n", zoomin, childCount);
      }
		  i=i+wide;
	  }
  }
  if (zoomOption == 1)
  {
    for(i = 0; i<memarea.blockCount; i++  ){
      //if(((double)pageTotalAccess.at(i)>(double)totalAccessParent*0.1)&&(Rud.at(i)<3))
      //Zoom in block with 10percent access of parent 
      //if(((double)pageTotalAccess.at(i)>0.1*totalAccessParent))   
      bool flZoomBlock=false;
      //uint32_t tmpCompare;
      //Zoom logic
      // top three levels - three blocks
      // level FOUR and up - top block in each parent OR block with access count of 1000
      if(pageTotalAccess.at(i) >1) {
        if((double) pageTotalAccess.at(i) >= ((double)zoomThreshold*totalAccessParent))
          flZoomBlock=true;
        if (flZoomBlock)   
        {
          Memblock hotpage;
          if(curPage.strID.compare("R")==0) 
             strNodeId ='A'+childCount; 
          else
             strNodeId = curPage.strID+std::to_string(childCount);
          hotpage.strID = strNodeId; 
	  	    hotpage.level = curPage.level+1;
          if (hotpage.level <lvlConstBlockSize) {
	  	      hotpage.blockSize = memarea.blockSize;
	    	    hotpage.min = memarea.min+memarea.blockSize*(i);
            hotpage.max = memarea.min+(memarea.blockSize*(i+1))-1;
          //Set blockSize rather than using constant branching factor
          } else if(hotpage.level >= lvlConstBlockSize) { 
            hotpage.blockSize = ((levelOneSize)/((int) pow((double)4,(hotpage.level-lvlConstBlockSize))));
	    	    hotpage.min = memarea.min+memarea.blockSize*(i);
            hotpage.max = memarea.min+(memarea.blockSize*(i+1))-1;
          }
          //printf("zoom in i=%d  memarea.min = %08lx memarea.max = %08lx memarea.blockSize = %d hotpage.level =%d hotpage min = %08lx max %08lx \n", i, memarea.min, memarea.max, memarea.blockSize, hotpage.level, hotpage.min,hotpage.max);
          hotpage.leftPid = i;
          hotpage.rightPid = i;
  		    hotpage.strParentID = curPage.strID;
          //Zoom only in heap area
          //if((hotpage.min+((hotpage.max-hotpage.min)/2)) <= heapAddrEnd) 
          if(hotpage.max <= heapAddrEnd) {
    			  zoomPageList.push_back(hotpage);
  	  		  (*zoomin)++;
            childCount++;
          }
        //printf("zoomin value %d childCount %d\n", zoomin, childCount);
        }
      }
    }
  }
  if(zoomOption == 2) {
    // Zoom in blocks with high access counts upto block size 16384
    // Find contiguous blocks - form a region
    // After 16384 - join and look for 1024 or 256
    bool flCombineFirstLevel = false;
    for(i = 0; i<memarea.blockCount; i++  ){
      zoominaccess = 0;
	    wide = 0; 
  	  for(j = i; j<memarea.blockCount; j++){
	      if(pageTotalAccess.at(j) != 0 ){ 
	  		  zoominaccess = zoominaccess + pageTotalAccess.at(j);
	    	  wide++;
	  	  } else {
	  		  break;
	  	  }
	    }
      //if(zoominaccess!=0) printf(" i = %d, wide = %d, zoominaccess %d , totalAccessParent %d \n", i, wide, zoominaccess, totalAccessParent);
      //Added to keep parents separate at the 'root+1' level
      //Access in Thread heap gets obscured by the stack access - so do not aggregate pages to region
      // Issues with smaller region traces - example gemm tile vs reorder
      // SIZE - FIX
      if( (curPage.level == (lvlConstBlockSize-1))) {
          // Issues with smaller region traces - example gemm tile vs reorder
          // SIZE - FIX
          if( (memarea.blockSize) <= levelOneSize ) {
            flCombineFirstLevel = true;
            uint32_t levelSize = 1;
            while(levelSize < memarea.blockSize) {
              levelSize*=2;
            }
            levelOneSize =levelSize;
            //printf(" combine first level levelOneSize %ld\n", levelOneSize);
        } else {
          wide=1;
          zoominaccess = pageTotalAccess.at(i);
        }
      }  
      // 0.001 multiplier at level 1 - used for including heap address range at 'root+1' level 
      if(((curPage.level == (lvlConstBlockSize-1)) && ((double)zoominaccess>=(double)0.001*totalAccessParent)) ||
      // Using 1000 - to include all heap regions
      //if(((curPage.level == (lvlConstBlockSize-1)) && ((double)zoominaccess>=1000)) ||
       ((curPage.level >= lvlConstBlockSize) && ((double)zoominaccess>=(double)zoomThreshold*totalAccessParent)))
      {
        Memblock hotpage;
        if(curPage.strID.compare("R")==0) 
           strNodeId ='A'+childCount; 
        else
           strNodeId = curPage.strID+std::to_string(childCount);
        hotpage.strID = strNodeId; 
	  	  hotpage.min = memarea.min+memarea.blockSize*(i);
        hotpage.max = memarea.min+(memarea.blockSize*(i+wide))-1;
	  	  hotpage.level = curPage.level+1;
  	  	hotpage.blockSize = memarea.blockSize;
        if (hotpage.level <lvlConstBlockSize) {
  	  	    hotpage.blockSize = memarea.blockSize;
        } else if(hotpage.level >= lvlConstBlockSize) { 
            hotpage.blockSize = ((levelOneSize)/((int) pow((double)4,(hotpage.level-lvlConstBlockSize))));
        }
        printf("Before Add to zoomlist start=%d wide = %d memarea.min = %08lx memarea.max = %08lx memarea.blockSize = %ld hotpage.level =%d hotpage min = %08lx max %08lx hotpage.blockSize = %ld \n",  i, wide, memarea.min, memarea.max, memarea.blockSize, hotpage.level, hotpage.min,hotpage.max, hotpage.blockSize);
			//printf("Using block size for last level in zoomRUD %08lx \n", cacheLineWidth);
        //if(memarea.blockSize <= 16777216) 
        if(memarea.blockSize <= zoomLastLvlPageWidth) 
          hotpage.blockSize = cacheLineWidth;
        hotpage.leftPid = i;
        hotpage.rightPid = i+wide-1;
		    hotpage.strParentID = curPage.strID;
        //printf("zoomin value %d childCount %d\n", zoomin, childCount);
        if(((curPage.level == (lvlConstBlockSize-1)) ||  // Zoom in stack area also at Level 1 - below root
          ((curPage.level >= lvlConstBlockSize) && (hotpage.max <= heapAddrEnd ) && (memarea.blockSize !=cacheLineWidth)))) { //Zoom only in heap area, end at size 128
            printf("Add to zoomlist option = %d memarea.min = %08lx memarea.max = %08lx memarea.blockSize = %ld hotpage.level =%d hotpage min = %08lx max %08lx hotpage.blockSize = %ld iparent %s , ID %s \n",  zoomOption, memarea.min, memarea.max, memarea.blockSize, hotpage.level, hotpage.min,hotpage.max, hotpage.blockSize, (curPage.strID).c_str(), (hotpage.strID).c_str());
      			  zoomPageList.push_back(hotpage);
  	    		  (*zoomin)++;
              childCount++;
        } else {
         printf(" Not zooming in stack region min = %08lx max = %08lx blockSize = %ld level =%d \n",  hotpage.min,hotpage.max, hotpage.blockSize, hotpage.level);
        }
      }
      // SIZE - FIX
      if( flCombineFirstLevel == true)  {
        i = i+wide;
      }
      if (curPage.level != (lvlConstBlockSize-1)) {
		    i=i+wide;
      }
    }
    if( flCombineFirstLevel == true)  {
      printf(" WARNING - Small memarea trace - Level one combined to size levelOneSize %ld\n", levelOneSize); 
    }
  }
  /* 
   * Zoom functionality used in spatial affinity STEP 3 - identify hot blocks for inter-region zoom
   * Find ten highest access blocks in the region 
   */
  if(zoomOption ==3)
  {
    int cntListAdd=0;
    int numBlocksToZoom = 10;
    childCount=0;
    vector<pair<uint64_t, uint64_t>> vecAccessBlockId;
    vector<uint64_t> vecBlockId;
    // SORT - based on access counts
    for(i = 0; i<memarea.blockCount; i++  )
       vecAccessBlockId.push_back(make_pair(pageTotalAccess.at(i),i ));
    std::sort(vecAccessBlockId.begin(), vecAccessBlockId.end(), greater<>());
    if(((int)vecAccessBlockId.size()) < numBlocksToZoom)
      numBlocksToZoom = vecAccessBlockId.size();
    // COPY - highest access blocks
    for(int iter=0; iter< numBlocksToZoom; iter++) {
      vecBlockId.push_back(vecAccessBlockId[iter].second);
      //printf( " sorted vector blockid %ld access %ld \n", vecAccessBlockId[iter].second, vecAccessBlockId[iter].first);
    }
    // SORT - based on blockid 
    std::sort(vecBlockId.begin(), vecBlockId.end() );

    for (int iter =0; iter< ((int)vecBlockId.size()); iter++) {
      i = vecBlockId[iter];
      if (cntListAdd < numBlocksToZoom)  {
          Memblock hotpage;
          strNodeId = curPage.strID+std::to_string(childCount);
          hotpage.strID = strNodeId; 
	  	    hotpage.min = memarea.min+memarea.blockSize*(i);
          hotpage.max = memarea.min+(memarea.blockSize*(i+1))-1;
	    	  hotpage.level = curPage.level+1;
  	    	hotpage.blockSize = cacheLineWidth; 
		      hotpage.strParentID = curPage.strID;
          if(hotpage.max <= heapAddrEnd) {
            printf("Add to list zoom in  zoomOption = %d, memarea.min = %08lx memarea.max = %08lx memarea.blockSize = %ld hotpage.level =%d hotpage min = %08lx max %08lx hotpage.blockSize = %ld iparent %s , ID %s i val - %d \n",  zoomOption, memarea.min, memarea.max, memarea.blockSize, hotpage.level, hotpage.min,hotpage.max, hotpage.blockSize, (curPage.strID).c_str(), (hotpage.strID).c_str(), i);
      		  zoomPageList.push_back(hotpage);
            childCount++;
            cntListAdd++;
         }
      }
    }
    vecAccessBlockId.clear();
    vecBlockId.clear();
  }
  // HOT-INSN add area
  if(zoomOption == 4) {
    // Zoom in blocks with high access counts upto block size 16384
    // Find contiguous blocks - form a region
    for(i = 0; i<memarea.blockCount; i++  ){
      zoominaccess = 0;
	    wide = 0; 
  	  for(j = i; j<memarea.blockCount; j++){
	      if(pageTotalAccess.at(j) != 0 ){ 
	  		  zoominaccess = zoominaccess + pageTotalAccess.at(j);
	    	  wide++;
	  	  } else {
	  		  break;
	  	  }
	    }
       if ( (double)zoominaccess>=(double)zoomThreshold*totalAccessParent) {
        Memblock hotpage;
        strNodeId = curPage.strID+std::to_string(childCount);
        hotpage.strID = strNodeId; 
	  	  hotpage.min = memarea.min+memarea.blockSize*(i);
        hotpage.max = memarea.min+(memarea.blockSize*(i+wide))-1;
	  	  hotpage.level = curPage.level+1;
  	  	hotpage.blockSize = memarea.blockSize;
        printf("Add to list zoom in start=%d wide = %d \n",  i, wide);
        hotpage.leftPid = i;
        hotpage.rightPid = i+wide-1;
		    hotpage.strParentID = curPage.strID;
        printf("Add to zoomlist option= %d Memarea min = %08lx max = %08lx blockSize = %ld hotpage.level =%d hotpage min = %08lx max %08lx hotpage.blockSize = %ld iparent %s , ID %s \n",  zoomOption, memarea.min, memarea.max, memarea.blockSize, hotpage.level, hotpage.min,hotpage.max, hotpage.blockSize, (curPage.strID).c_str(), (hotpage.strID).c_str());
        zoomPageList.push_back(hotpage);
  	    (*zoomin)++;
        childCount++;
      }
		    i=i+wide;
    }
    if(childCount == 0 )  {
      printf("NON-STACK, No child added for %s Adding two CHILDREN with ZERO access pages\n", (curPage.strID).c_str());
      uint32_t numBlockGroup = 16;
      uint32_t numChildrenToAdd =2;
      uint32_t numPagesInGroup = 1;
      vector<pair<uint32_t, uint32_t>> vecAccessPages;
      if ( memarea.blockCount <= numBlockGroup ) {
        numBlockGroup =1 ;
      } else {
        while( (memarea.blockCount / numPagesInGroup ) > numBlockGroup) 
          numPagesInGroup*=2;
      }
      printf(" CHILD memarea.blockCount = %d numBlockGroup = %d numPagesInGroup = %d \n", memarea.blockCount ,numBlockGroup , numPagesInGroup);  
      for(i = 0; i<numBlockGroup; i++  ){
          zoominaccess = 0;
        for(j = 0; j<numPagesInGroup; j++ ){
          if (((i*numPagesInGroup)+j) < memarea.blockCount) {
            zoominaccess = zoominaccess + pageTotalAccess.at((i*numPagesInGroup)+j);
          }
        } 
        //printf(" Child access i = %d j = %d access = %d \n", i, j, zoominaccess);  
        vecAccessPages.push_back(make_pair(zoominaccess, i*numPagesInGroup));
      }
      sort(vecAccessPages.begin(), vecAccessPages.end(), greater<>());
      
      /* 
      // Previous trial to add based on threshold with holes - still results in significant region loss
      for(i = 0; i<memarea.blockCount; i++  ){
        zoominaccess = 0;
  	    wide = 0; 
    	  for(j = i; j<memarea.blockCount; j++){
	        if((pageTotalAccess.at(j) != 0) || (((j+2) < memarea.blockCount) && ((pageTotalAccess.at(j+1) != 0)|| (pageTotalAccess.at(j+2) != 0)))){ 
	  	  	  zoominaccess = zoominaccess + pageTotalAccess.at(j);
	    	    wide++;
	  	    } else {
	  		    break;
	  	    }
          if(zoominaccess > child_zoominaccess) {
            child_i = i; 
            child_wide = wide;
            child_zoominaccess = zoominaccess;
          }
        }
        //printf(" Child access i = %d wide = %d access = %d \n", i, wide, zoominaccess);  
        i=i+wide;
      } */
      
      for ( uint32_t vecItr=0; vecItr<numChildrenToAdd; vecItr++) {
        printf(" top access - %d , i %d \n", vecAccessPages[vecItr].first, vecAccessPages[vecItr].second);
        printf(" Child with largest access start = %d wide = %d access = %d \n", vecAccessPages[vecItr].second, numPagesInGroup, vecAccessPages[vecItr].first);  
        i = vecAccessPages[vecItr].second;
        wide = numPagesInGroup ;
        childCount++;
        Memblock hotpage;
        strNodeId = curPage.strID+std::to_string(childCount);
        hotpage.strID = strNodeId; 
	  	  hotpage.min = memarea.min+memarea.blockSize*(i);
        hotpage.max = memarea.min+(memarea.blockSize*(i+wide))-1;
	  	  hotpage.level = curPage.level+1;
  	  	hotpage.blockSize = memarea.blockSize;
        printf("Add to list ATLEAST TWO CHILDREN zoom in start=%d wide = %d \n",  i, wide);
        hotpage.leftPid = i;
        hotpage.rightPid = i+wide-1;
		    hotpage.strParentID = curPage.strID;
        printf("Add to list ATLEAST TWO CHILDREN zoom in option = %d memarea.min = %08lx memarea.max = %08lx memarea.blockSize = %ld hotpage.level =%d hotpage min = %08lx max %08lx hotpage.blockSize = %ld iparent %s , ID %s \n",  zoomOption, 
         memarea.min, memarea.max, memarea.blockSize, hotpage.level, hotpage.min,hotpage.max, hotpage.blockSize, (curPage.strID).c_str(), (hotpage.strID).c_str());
      	 zoomPageList.push_back(hotpage);
  	     (*zoomin)++;
      }
    }
  }
  sortPageAccess.clear();
}

int writeZoomFile(const MemArea memarea, const Memblock thisMemblock, const vector<BlockInfo *>& vecBlockInfo, std::ofstream& zoomFile_det, 
                  uint32_t* thresholdTotAccess, int zoominTimes, int writeOption) 
{
  double * w_sampleRud; 
  uint32_t * w_pageTotalAccess; 
  uint32_t printTotAccess =0;
  uint32_t cntSingleAccessblocks =0;
  uint32_t cntZeroAccessblocks =0;
  vector <uint32_t> w_printPageAccess;  
  vector <double> w_printPageRUD;  
  vector <double> w_printPageSampleRUD;  
  double printAvgRUD=0.0;
  double printSampleAvgRUD=0.0;
  uint32_t printAvgRUDdiv=0;
  uint32_t printSampleAvgRUDdiv=0;
  uint32_t i=0;
  if(writeOption == 0) {
			  zoomFile_det << std::dec <<zoominTimes << " Level " << thisMemblock.level << " ID " << thisMemblock.strID 
                  << " Parent " <<  thisMemblock.strParentID << " - (" << thisMemblock.leftPid 
                  << "," << thisMemblock.rightPid << ") MemoryArea " << hex<< memarea.min << "-" << hex<< memarea.max 
                  << " Block size " <<std::dec << memarea.blockSize << " skip this due to small memarea" << endl;
      return 0;
  } else if(writeOption == 1) {
			  zoomFile_det << std::dec <<zoominTimes << " Level " << thisMemblock.level << " ID " << thisMemblock.strID 
                  << " Parent " <<  thisMemblock.strParentID << " - (" << thisMemblock.leftPid 
                  << "," << thisMemblock.rightPid << ") MemoryArea " << hex<< memarea.min << "-" << hex<< memarea.max 
                  << " Block size " <<std::dec << memarea.blockSize << " skip this due to small block size" << endl;
      return 0;
  } else {
    w_pageTotalAccess = new uint32_t [memarea.blockCount];   
    w_sampleRud = new double [memarea.blockCount];   //Intra-sample RUD
    w_printPageAccess.clear();
    w_printPageRUD.clear();
    w_printPageSampleRUD.clear();
    printTotAccess=0;
    printAvgRUD=0.0;
    printAvgRUDdiv=0;
    printSampleAvgRUD=0.0;
    printSampleAvgRUDdiv=0;
    cntSingleAccessblocks = 0;
    cntZeroAccessblocks = 0;
    //printf("memarea.blockCount %ld\n", memarea.blockCount);
    for(i = 0; i< memarea.blockCount; i++){
      BlockInfo *curBlock = vecBlockInfo.at(i);
      //curBlock->printBlockRUD();
      w_pageTotalAccess[i]=curBlock->getTotalAccess();
      //w_Rud[i]=curBlock->getAvgRUD();
      w_sampleRud[i]=curBlock->getSampleAvgRUD();
      if(w_pageTotalAccess[i]!=0){
          //printf("level %d Non-Zero access %d %ld \n", thisMemblock.level, i, pageTotalAccess[i]);
        if(w_pageTotalAccess[i]==1)
          cntSingleAccessblocks++;
        printTotAccess+=w_pageTotalAccess[i];
        w_printPageAccess.push_back(w_pageTotalAccess[i]); 
        if(w_sampleRud[i] != -1){ 
          w_printPageSampleRUD.push_back(w_sampleRud[i]);
          printSampleAvgRUD+= w_sampleRud[i];
          printSampleAvgRUDdiv+=1;
        }
      } else {
          //printf("level %d Zero access %d %ld \n" , thisMemblock.level , i, pageTotalAccess[i]);
        cntZeroAccessblocks++;
      }
    }
    if(printTotAccess ==0) {
      printf("All values are zero - No analysis done\n");
      return -1;
    }
    sort(w_printPageAccess.begin(), w_printPageAccess.end(), greater<>());
    sort(w_printPageRUD.begin(), w_printPageRUD.end(), greater<>());
    sort(w_printPageSampleRUD.begin(), w_printPageSampleRUD.end(), greater<>());
		zoomFile_det << std::dec <<zoominTimes << " Level " << thisMemblock.level << " ID " << thisMemblock.strID 
                  << " Parent " <<  thisMemblock.strParentID << " - (" << thisMemblock.leftPid 
                  << "," << thisMemblock.rightPid << ") MemoryArea " << hex<< memarea.min << "-" << memarea.max 
                  << " Block size " <<std::dec <<  memarea.blockSize << " " ;
    if(memarea.blockSize == cacheLineWidth) {
      if(w_printPageRUD.size() !=0 && w_printPageSampleRUD.size() !=0) {
		    zoomFile_det << "blockCount " << std::dec << memarea.blockCount <<" Access "<< printTotAccess 
            << " RUD (" << w_printPageRUD.at(w_printPageRUD.size()-1) << ", " << w_printPageRUD.at(0) << ", "<< (printAvgRUD/printAvgRUDdiv)<<") "
            << " SampleRUD (" << w_printPageSampleRUD.at(w_printPageSampleRUD.size()-1) << ", " << w_printPageSampleRUD.at(0) << ", "
            << (printSampleAvgRUD/printSampleAvgRUDdiv)<<") "<< "single_acs_blk "<< cntSingleAccessblocks<<" zero_acs_blk "<< cntZeroAccessblocks<<" ";  
      }
      else if(w_printPageRUD.size() ==0 && w_printPageSampleRUD.size() !=0) {
		    zoomFile_det << "blockCount " << std::dec << memarea.blockCount <<" Access "<< printTotAccess 
            << " RUD (-1, -1, -1) " 
            << " SampleRUD (" << w_printPageSampleRUD.at(w_printPageSampleRUD.size()-1) << ", " << w_printPageSampleRUD.at(0) << ", "
            << (printSampleAvgRUD/printSampleAvgRUDdiv)<<") "<< "single_acs_blk "<< cntSingleAccessblocks<<" zero_acs_blk "<< cntZeroAccessblocks<<" " ;  
      }
      else if(w_printPageRUD.size() !=0 && w_printPageSampleRUD.size() ==0) {
		    zoomFile_det << "blockCount " << std::dec << memarea.blockCount <<" Access "<< printTotAccess 
            << " RUD (" << w_printPageRUD.at(w_printPageRUD.size()-1) << ", " << w_printPageRUD.at(0) << ", "<< (printAvgRUD/printAvgRUDdiv)<<") "
            << " SampleRUD (-1, -1, -1) " << "single_acs_blk "<< cntSingleAccessblocks<<" zero_acs_blk "<< cntZeroAccessblocks<<" ";  
      }
      else { 
		    zoomFile_det << "blockCount " << std::dec << memarea.blockCount <<" Access "<< printTotAccess 
            << " RUD (-1, -1, -1) " 
            << " SampleRUD (-1, -1, -1) " << "single_acs_blk "<< cntSingleAccessblocks<<" zero_acs_blk "<< cntZeroAccessblocks<<" ";  
          }
        }
   
	for(i = 0; i< memarea.blockCount; i++){
      if(w_pageTotalAccess[i]!=0) {
        uint64_t blockMinAddr = memarea.min+memarea.blockSize*(i);
        uint64_t blockMaxAddr = memarea.min+(memarea.blockSize*(i+1))-1;
        // SET access threshold to zoom in heap area - access count 
        if((thisMemblock.level ==1 )&& (blockMaxAddr <= heapAddrEnd))
            (*thresholdTotAccess) += w_pageTotalAccess[i];
        if(thisMemblock.level ==1) 
	         zoomFile_det << std::dec << "p"<< i << ": " 
                    <<hex << blockMinAddr <<"-" << hex<< blockMaxAddr 
                    //<< " "<< std::dec<< w_pageTotalAccess[i] << "(" << w_sampleRud[i]<< ");";
                    << " " << std::dec<< w_pageTotalAccess[i] << " ";
        else if(memarea.blockSize == cacheLineWidth) 
			      zoomFile_det << std::dec << "p"<< i << ":" 
                    //<<hex << blockMinAddr <<"-" << hex<< blockMaxAddr 
                    //<< " "<< std::dec<< w_pageTotalAccess[i] << "(" <<w_sampleRud[i]<< "); ";
                    << std::dec<< w_pageTotalAccess[i] << "(" << w_sampleRud[i]<< "); ";
        else
			      zoomFile_det << std::dec << "p"<< i << ":" 
                    //<<hex << blockMinAddr <<"-" << hex<< blockMaxAddr 
                    << std::dec<< w_pageTotalAccess[i] << " " ;
		  } 
    }
		zoomFile_det << endl;
  }
  w_printPageAccess.clear();
  w_printPageRUD.clear();
  w_printPageSampleRUD.clear();
  delete[] w_sampleRud; 
  delete[] w_pageTotalAccess; 
  return 0;
}

int main(int argc, char ** argv){
   printf("-------------------------------------------------------------------------------------------\n");
   int argi = 1;
   char *memoryfile;
   char *outputFileZoom = (char *) malloc(500*sizeof(char));
   char *outputFileSpatial = (char *) malloc(500*sizeof(char));
   char *outputFileSpatialInsn=(char *) malloc(500*sizeof(char));
   if(argc > argi)
   {
        memoryfile = argv[argi];
        argi++;
		  if (strcmp(memoryfile, "--help") == 0 || (strcmp(memoryfile, "-h") == 0)){
			  printf("Using ./memgaze-analyze-loc (memorytrace) [--analysis] [--memRange min-max] [--pnum #page] [--phyPage (size)] \n\t[--zoomRUD] [--outputZoom zoomOutputFile] [--spatial] [--outputSpatial spatialOutputFile] \n\t[--blockWidth (size)] [--zoomStopPageWidth (size)] [--insn] [--heapAddrEnd address] \n");
			  printf("Other possible options [--autoZoom] [--zoomAccess] [--bottomUp] [--model]\n");
			  printf("--analysis\t: set to enable analysis\n");
			  printf("--memRange min-max\t: zoom into memory address range, ignore outage memory\n");
			  printf("--pnum\t: Change the number of pages within the memory trace address range - DEFAULT 16\n");
			  printf("--phyPage [size]\t: Using physical page size instead of page count, [setup page size in word: example 64 for 64B]\n"); 
			  printf("--spatial\t: display Spatial Analysis results - Analysis performed at zoom Stop page level\n");
			  //printf("--model\t: set if enable modeling\n"); //configuration from model.config
			  //printf("--autoZoom\t: enable automatic zoomin for contiguous hot pages\n");
			  //printf("--zoomAccess\t: enable automatic zoomin for pages with access count above threshold value of %f \n", zoomThreshold);
			  printf("--zoomRUD\t: enable automatic zoomin for RUD \n");
			  printf("--blockWidth\t: Set block size in words (ex. 64 for 64 Bytes) for last level in zoomRUD analysis \n");
			  printf("--zoomStopPageWidth\t: Set zoom Stop page width in words (ex. 16384 for 16384 Bytes) for last level in zoomRUD analysis \n");
			  //stack changes between 0x7f.. in single threaded to 0x7ff.. in multi-threaded application
			  printf("--heapAddrEnd\t: Set heap address max value - spcify end (length of address 12), located in memgaze.config file \n"); 
			  printf("--insn\t: Find instructions in memRange - use with memRange\n");
			  printf("--count\t: Find cardinality in trace\n");
			  //printf("--bottomUp\t: enable bottom-up analysis - doesnt implement feature yet\n");
			  return -1;
		  }
   } else {
    printf("Specify input file and arguments\n");
    return -1;
   }
        
  mempin = 16;
	int model = 0;
	int analysis = 0;
	int coreNumber = 0;
	int memRange = 0;
	phyPage = 0;
  //cacheLineWidth = stol("80",0,16); // Added for ZoomRUD analysis - option to set last level's block width
  cacheLineWidth = 64 ; // Added for ZoomRUD analysis - option to set last level's block width
  zoomLastLvlPageWidth = 16384;
	int spatialResult = 0;
	int outZoom = 0;
	int outSpatial = 0;
	int autoZoom = 0;
	int zoomOption = 0;
  int bottomUp = 0;
  int getInsn = 0;
  int countCardinality=0;
  uint64_t traceMin = stoull("FFFFFF",0,16); // Added for invalid load address checks - range corrected - load address with 0x1d49620 format refers to offset in double ptwrite loads, and perf drops some records resulting in offset loads being reported
  uint64_t traceMax = stoull("8F0000000000", 0, 16); // Omit load addresses beyond stack range - 12 hex digits with 7F..
  uint64_t user_max = 0;
	uint64_t user_min = 0;
	char *qpoint;
	pageSize = 64; 
  int intTotalTraceLine = 0;
  vector<TraceLine *> vecInstAddr;
  vector<TraceLine *>::iterator itr_tr;
  //TraceLine *ptrTraceLine;
  vector<BlockInfo *> vecBlockInfo;
  vector<BlockInfo *>::iterator itr_blk;
  // Changed to include stack also - threaded application's heap is mapped to 0x7...
  // Minivite - Single threaded - stack starts at 0x7f
  // Others - multi-threaded stack starts at 0x7ff
  heapAddrEnd = stol("7f0000000000",0,16); // stack region is getting included in mid-point 


	//get parameters
  while(argc > argi){
      qpoint = argv[argi];
      argi++;
      if (strcmp(qpoint, "--pnum") == 0){
    		printf("configuration mempin = ");
	      mempin = atoi(argv[argi]);
        printf("%d\n", mempin);
	      argi++;		
	    }

		/*if (strcmp(qpoint, "--queue") == 0){
		  printf("configuration queue policy = ");
		  policy = atoi(argv[argi]);
		  switch(policy){ 
		    case 0:	printf("FIFO\n"); break;
		    case 1:	printf("LIFO\n"); break;
		    case 2:	printf("PAGEHIT\n"); break;
		    default: printf("not configure, used FIFO\n"); break;
		  }
		  argi++;		
		 } */

		if (strcmp(qpoint, "--memRange") == 0){
			printf("configuration mem address range:");
  		memRange = 1;
	  	char* Range;
		  Range = argv[argi];
		  char* temp;
  		temp = strtok(Range,"-");
      if(temp == NULL) {
        printf("Enter mem address range in format min-max\n"); 
        return -1;
      }
	  	user_min=stol(temp,0,16);
  		temp = strtok(NULL,"-");
  		user_max=stol(temp,0,16);
  		argi++;		
			printf("%08lx-%08lx\n",user_min,user_max);
		}
		if (strcmp(qpoint, "--phyPage") == 0){
			printf("Using Physical Page Size\n");
	  	phyPage = 1;
			pageSize=stol(argv[argi],0);		
		  argi++;
		}
		if (strcmp(qpoint, "--model") == 0){
			printf("enabling memory modeling\n");
		  model = 1;		
		}
		if (strcmp(qpoint, "--analysis") == 0){
			printf("enabling memory analysis\n");
		  analysis = 1;	
		}
		if (strcmp(qpoint, "--bottomUp") == 0){
			printf("Bottom-up memory analysis\n");
		  analysis = 1;	
      bottomUp =1; 
		}
		if (strcmp(qpoint, "--spatial") == 0){
			spatialResult=1;
			printf("configuration for Spatial Result display = %d\n", spatialResult);	
		}
		if (strcmp(qpoint, "--autoZoom") == 0){
			printf("enable automatic zoom in for contiguous hot pages\n");
		  autoZoom  = 1;
		  zoomOption  = 0;
      bottomUp =0; 
		}
		if (strcmp(qpoint, "--zoomAccess") == 0){
			printf("--zoomAccess  : enable automatic zoomin for pages with access count above threshold value of %f \n", zoomThreshold);
		  autoZoom  = 1;
		  zoomOption  = 1;
      bottomUp =0; 
		}
		if (strcmp(qpoint, "--zoomRUD") == 0){
			printf("--zoomRUD  : enable automatic zoomin for RUD \n");
		  autoZoom  = 1;
		  zoomOption  = 2;
      bottomUp =0; 
		}
		if (strcmp(qpoint, "--outputZoom") == 0){
		  outZoom = 1;
		  outputFileZoom = argv[argi];
			printf("Zoom output redirected to %s \n", outputFileZoom);
		  argi++;			
		}
		if (strcmp(qpoint, "--outputSpatial") == 0){
		  outSpatial = 1;
		  outputFileSpatial = argv[argi];
			printf("Spatial output redirected to %s \n", outputFileSpatial);
		  argi++;			
		}
		if (strcmp(qpoint, "--blockWidth") == 0){
			cacheLineWidth=stol(argv[argi],0);		
			printf("Using block size for last level in zoomRUD %ld \n", cacheLineWidth);
		  argi++;
		}
		if (strcmp(qpoint, "--zoomStopPageWidth") == 0){
			zoomLastLvlPageWidth = stol(argv[argi],0);		
			printf("Using Page size for zoom stop level in zoomRUD %ld \n", zoomLastLvlPageWidth);
		  argi++;
		}
		if (strcmp(qpoint, "--heapAddrEnd") == 0){
      heapAddrEnd = stol(argv[argi],0,16); // stack region is getting included in mid-point 
		  argi++;
		}
		if (strcmp(qpoint, "--insn") == 0){
			printf("--insn : Find instructions for %s in memRange ", memoryfile);
			printf("%08lx-%08lx\n",user_min,user_max);
      getInsn = 1;
      if(memRange !=1)
      {
			  printf("--insn : Specify memRange to find instructions in memRange \n");
        return -1;
      }
		}
		if (strcmp(qpoint, "--count") == 0){
			printf("--count : Find cardinality for trace %s\n", memoryfile);
      countCardinality = 1;
    }
  }
  if(zoomLastLvlPageWidth == 16384 || zoomLastLvlPageWidth == 4096)
    levelOneSize = 4194304*16;
  else
    levelOneSize = 4194304*8;
  
	if(model == 1){
		if(loadConfig()==-1) return -1;
	}
  //check the memory area for the first time
	uint32_t totalAccess = 0;
	uint32_t totalSamples = 0;
  std::cout << "Application: " << memoryfile << "\n";
	printf("Reading trace with load address in range min %lx max %lx \n", traceMin, traceMax);
  uint32_t windowMin;
  uint32_t windowMax;
  double windowAvg;
  windowMin=0; 
  windowMax=0;
  windowAvg=0.0;
  int readReturn = readTrace(memoryfile, &intTotalTraceLine, vecInstAddr, &windowMin, &windowMax, &windowAvg, &traceMax, &traceMin, &totalSamples);
  if(readReturn == -1) {
    printf("Error in readTrace \n");
    return -1;
  }
  if(countCardinality ==1) {
     TraceLine *ptrTraceLine;
     hll::HyperLogLog hll(4);
    for (uint32_t itr=0; itr<vecInstAddr.size(); itr++){
      ptrTraceLine=vecInstAddr.at(itr);
      std::string strLoadAddr = std::to_string(ptrTraceLine->getLoadAddr());
      hll.add(strLoadAddr.c_str(), strLoadAddr.size());
      }
    double cardinality = hll.estimate();
    std::cout << "Cardinality:" << cardinality << std::endl;
  }
    
  totalAccess = intTotalTraceLine;
  printf( "Number of lines in trace %d number of lines with valid addresses %ld total access %d \n", 
          intTotalTraceLine, vecInstAddr.size(), totalAccess);
	printf("Data from Trace load address min %lx max %lx \n", traceMin, traceMax);
  printf("number of samples in trace %d \n", totalSamples);
  printf("Sample sizes in trace min %d max %d average %f \n", windowMin, windowMax, windowAvg);
  printf("Using Zoom perrcent value %lf\n", zoomThreshold); 
	printf("Using heap address range to max of %lx \n", heapAddrEnd);
  printf("Using cache line width %ld Bytes\n ", cacheLineWidth ); 
  printf("Using last level block/page size %ld Bytes\n", zoomLastLvlPageWidth); 
  printf("***************************************\n");
  bool done = 0;
  std::list<Memblock > zoomPageList;
  std::list<Memblock > spatialRegionList;
  std::list<Memblock > finalRegionList;
  std::list<Memblock > spatialOSPageList;
  std::list<Memblock>::iterator itrRegion;
  std::list<Memblock>::iterator itrOSPage;
  vector<pair<uint64_t, uint64_t>> setRegionAddr;
  std::vector<pair<uint64_t, uint64_t>> vecParentFamily ;
  std::vector<TopAccessLine *> vecLineInfo;
  std::vector<uint64_t> vecTopAccessLineAddr;
  TopAccessLine ptrTopAccessLine;
  int zoomin = 0;
  int zoominTimes = 0;
  ofstream zoomInFile_det;
  if( autoZoom ==1 ) {
    if(outZoom==1) {
      zoomInFile_det.open(outputFileZoom, std::ofstream::out | std::ofstream::trunc);
    } else {
      zoomInFile_det.open("zoomIn.txt", std::ofstream::out | std::ofstream::trunc);
    }
    if (zoomInFile_det.is_open() ) {
			printf("Zoom output redirected to %s \n", outputFileZoom);
    } else {
			printf("Zoom output file open failed in %s \n", outputFileZoom);
      return -1;
    }
  }
  ofstream spatialOutFile;
  ofstream spatialOutInsnFile;
  if (spatialResult ==1 ) {
    if(outSpatial==1){
      strcpy(outputFileSpatialInsn ,outputFileSpatial);
      strcat(outputFileSpatialInsn, "_insn");
      spatialOutFile.open(outputFileSpatial, std::ofstream::out | std::ofstream::trunc);
      spatialOutInsnFile.open(outputFileSpatialInsn, std::ofstream::out | std::ofstream::trunc);
    } else
      spatialOutFile.open("spatialRUD.txt", std::ofstream::out | std::ofstream::trunc);
    if (spatialOutFile.is_open() && spatialOutInsnFile.is_open()) {
			printf("Spatial output redirected to %s \n", outputFileSpatial);
			printf("Spatial instruction output redirected to %s \n", outputFileSpatialInsn);
    }
    else {
			printf("Spatial output file open failed in %s and %s \n", outputFileSpatial, outputFileSpatialInsn);
      return -1;
    }
  }
  uint32_t i=0;
  Memblock thisMemblock;
	thisMemblock.level=1;
  thisMemblock.strID="R";
  thisMemblock.strParentID="-";
  thisMemblock.leftPid=0;
  thisMemblock.rightPid=0;
	MemArea memarea;
	MemArea memIncludePages;
  uint64_t cntAddPages=1;
	if(memRange ==0){
			memarea.max = traceMax;
			memarea.min = traceMin;
	}else{
			memarea.max = user_max;
			memarea.min = user_min;
	}
	if (phyPage == 0){
	    memarea.blockCount = mempin;
	    memarea.blockSize = ceil((memarea.max - memarea.min)/(double)mempin);
  } else{
		  memarea.blockCount =  ceil((memarea.max - memarea.min)/(double)pageSize);
		  memarea.blockSize = pageSize;
  }
  vector<double> sampleRud; 
  vector<uint32_t> pageTotalAccess; 
  uint32_t thresholdTotAccess =0;
  uint32_t printTotAccess =0;
  bool blCustomSize = 0;
  int writeReturn=0;
  int analysisReturn=0;
  if ((getInsn == 1) && (memRange==1))
  {
    getInstInRange(nullptr, vecInstAddr, memarea);
  }
	//start analysis
	if(analysis == 1){
    // START bottom-up - TRIAL 
    if (bottomUp ==1) {
	  	printf(" BU memory page number = %d\n", memarea.blockCount);
		  printf(" BU memory page size = %lx\n", memarea.blockSize);
      printf("before FOR value ");
      for (int k=16; k>=1; k=k/2) {
        printf("k value %d\n", k);
			  memarea.blockCount = k;
  			memarea.blockSize = ceil((memarea.max - memarea.min)/(double)memarea.blockCount);
        for (itr_blk = vecBlockInfo.begin(); itr_blk != vecBlockInfo.end(); ++itr_blk) {
          delete (*itr_blk);
        }
        vecBlockInfo.clear();
        for(i = 0; i< memarea.blockCount; i++){
          pair<unsigned int, unsigned int> blockID = make_pair(k, i);
          BlockInfo *newBlock = new BlockInfo(blockID, memarea.min+(i*memarea.blockSize), memarea.min+((i+1)*memarea.blockSize)-1, 
                                              memarea.blockCount, 0, "bu"); //spatialResult =0
          vecBlockInfo.push_back(newBlock);
        }
        printf("Size of vector BlockInfo %ld\n", vecBlockInfo.size());
	  	  analysisReturn=spatialAnalysis( vecInstAddr, memarea, coreNumber, 0, vecBlockInfo, setRegionAddr, memIncludePages,
                                  vecParentFamily, vecTopAccessLineAddr,1); //spatialResult =0
        if(analysisReturn ==-1)
          return -1;
        for(i = 0; i< memarea.blockCount; i++){
          BlockInfo *curBlock = vecBlockInfo.at(i);
          curBlock->printBlockRUD();
        }
      }
    } // END bottom-up - TRIAL 

    /*-------------------------------------------------------------------------------*/
    // START Analysis and top-down zoom functionality 
    // Spatial Affinity analysis not done in this loop - costs space & time overhead
    while((bottomUp==0) && (done != 1)){ //loop in zoom itr
      //printf(" in while size %ld count %ld \n", memarea.blockSize, memarea.blockCount);
      // Start analysis on root level with user defined page size
      if(blCustomSize == 0) {
  	    if ((phyPage ==0)  ){
	        memarea.blockSize = ceil((memarea.max - memarea.min)/(double)mempin);
        } else{
	  	    memarea.blockCount =  ceil((memarea.max - memarea.min)/(double)memarea.blockSize);
        }
      }
      //printf(" in while 2 size %ld count %ld %ld \n", memarea.blockSize, memarea.blockCount, pageSize);
      if((memarea.max-memarea.min)<=pageSize){
		    printf("skip this due to small memarea\n");
        writeReturn=writeZoomFile( memarea, thisMemblock, vecBlockInfo, zoomInFile_det, &thresholdTotAccess, zoominTimes,0 );
        if(writeReturn ==-1)
          return -1;
		  } else if(memarea.blockSize < 32) {
		    printf("skip this due to small block size %ld \n", memarea.blockSize);
        writeReturn=writeZoomFile( memarea, thisMemblock, vecBlockInfo, zoomInFile_det, &thresholdTotAccess, zoominTimes,1 );
        if(writeReturn ==-1)
          return -1;
      } else {
		    printf("start memory analysis for itr %d ID %s ", zoominTimes, thisMemblock.strID.c_str());
		    //printf("Memory address min %lx max %lx min %ld max %ld \n", memarea.min, memarea.max, memarea.min, memarea.max);
		    printf("Memory address min %lx max %lx ", memarea.min, memarea.max);
				printf(" page number = %d ", memarea.blockCount);
				printf(" page size =  %ld\n", memarea.blockSize);
        for (itr_blk = vecBlockInfo.begin(); itr_blk != vecBlockInfo.end(); ++itr_blk) {
          delete (*itr_blk);
        }
        cntAddPages=1;
        memIncludePages.min = traceMin; 
        memIncludePages.max = traceMax; 
        memIncludePages.blockSize = OSPageSize;
        memIncludePages.blockCount = cntAddPages;
        vecBlockInfo.clear();
        for(i = 0; i< memarea.blockCount; i++){
          pair<unsigned int, unsigned int> blockID = make_pair(0, i);
          BlockInfo *newBlock = new BlockInfo(blockID, memarea.min+(i*memarea.blockSize), memarea.min+((i+1)*memarea.blockSize-1), 
                                              memarea.blockCount+cntAddPages, 0, thisMemblock.strID); // spatialResult=0
          vecBlockInfo.push_back(newBlock);
        }
        if (memarea.blockSize == cacheLineWidth) { 
          // RUD analyisis only - Affinity analysis not done in this loop - costly space & time overhead
	  	    analysisReturn=spatialAnalysis( vecInstAddr, memarea, coreNumber, 0, vecBlockInfo, setRegionAddr, memIncludePages,
                                  vecParentFamily, vecTopAccessLineAddr,1); //spatialResult =0
        } else {
          analysisReturn= getAccessCount(vecInstAddr,   memarea,  coreNumber , vecBlockInfo );
        }
        if(analysisReturn ==-1) {
          printf("No analysis done\n");
          return -1;
        }
        printTotAccess=0;
        // Do not move these out of the loop - memarea.blockCount is different for each run
	      pageTotalAccess.clear();
        sampleRud.clear();
        for(i = 0; i< memarea.blockCount; i++){
          BlockInfo *curBlock = vecBlockInfo.at(i);
          if (memarea.blockSize == cacheLineWidth) { 
            curBlock->printBlockRUD();
          } else
            curBlock->printBlockAccess();
          pageTotalAccess.push_back(curBlock->getTotalAccess());
          //printf(" sampleRud %lf\n", curBlock->getSampleAvgRUD());
          if(pageTotalAccess.at(i)!=0){
              //printf("level %d Non-Zero access %d %ld \n", thisMemblock.level, i, pageTotalAccess.at(i));
            printTotAccess+=pageTotalAccess.at(i);
          }
        }
        if(printTotAccess ==0) {
          printf("All values are zero - No analysis done\n");
          return -1;
        }
        // Write Zoom output file
        writeReturn=writeZoomFile( memarea, thisMemblock, vecBlockInfo, zoomInFile_det, &thresholdTotAccess, zoominTimes , 2);
        if(writeReturn ==-1)
          return -1;
		    printf("zoominTimes %d done!\n", zoominTimes);
        printf("thresholdTotAccess %d\n", thresholdTotAccess);
        findHotPage(memarea, zoomOption, sampleRud, pageTotalAccess,thresholdTotAccess, &zoomin, thisMemblock, zoomPageList);
			}
			if ((zoomin != 0)&&(autoZoom == 1)){
        //printf("zoomin value %d\n", zoomin);
			 done = 0;
			 zoomin--;
			 zoominTimes++;
			 memRange = 1;
       if(!zoomPageList.empty()) {
 				 thisMemblock = zoomPageList.front();
        // RUD zoom analysis does not reaches cacheLine level - if not do at higher level
         if (thisMemblock.blockSize == cacheLineWidth || thisMemblock.blockSize <= (4*zoomLastLvlPageWidth)) {
      			  spatialRegionList.push_back(thisMemblock); // Final-Regions-of-Interest
          }
 				 zoomPageList.pop_front();
  			 memarea.max = thisMemblock.max;
	  		 memarea.min = thisMemblock.min;
         if(zoomOption >=1 && thisMemblock.level >=lvlConstBlockSize){
           memarea.blockSize = thisMemblock.blockSize;
   	       memarea.blockCount =  ceil((memarea.max - memarea.min)/(double)memarea.blockSize);
           blCustomSize = 1;
          //printf("zoom level %d size %ld count %ld memarea.min %08lx memarea.max %08lx \n", thisMemblock.level, memarea.blockSize, memarea.blockCount, memarea.min, memarea.max);
         } 
       } else {
          done =1;
          printf("Zoomlist empty \n");
       }
		} else
				 done = 1;
	} // END WHILE
	zoomInFile_det.close(); 

  // START - affinity analysis
  // Steps
  // 0-a. Find hot instructions - handle sparse accesses with HOT instructions
  //     Add region of HOT instruction with large, sparse footprint to Final-Regions-of-Interest 
  // 0-b. Find instructions to map regions to objects - using getInstInRange
  // 1. Calculate spatial affinity at data object (region) level
  // 2. Zoom into objects to find OS page sized (default 16K) hot blocks 
  // 3. Calculate spatial affinity at OS page level - using 64 B cache line 
  if(spatialResult == 1)
  {
    printf("Spatial Analysis START \n");
    /*---------------------------- HOT-INSN addition START ----------------------------------*/
    // STEP 0-a add rgions if the accesses are sparse, but the instruction is HOT 
    vector<std::pair<uint64_t,uint32_t>> vecInstAccessCount;
    vector<std::pair<uint64_t,uint64_t>> vecInstRegion;
    printf(" STEP 0-a get HOT Insn\n");
    getTopInst(vecInstAddr,vecInstAccessCount);
    size_t numHotInsn = vecInstAccessCount.size(); 
    for (size_t cntHotInsn=0; cntHotInsn < numHotInsn; cntHotInsn++)  {
      printf("HOT INSN %08lx count %d\n", vecInstAccessCount.at(cntHotInsn).first, vecInstAccessCount.at(cntHotInsn).second);
      getRegionforInst(&spatialOutInsnFile, vecInstAddr,vecInstAccessCount.at(cntHotInsn).first, vecInstRegion);
    }

    //XSBench get region for 1011be
    //getRegionforInst(&spatialOutInsnFile, vecInstAddr,1053118, vecInstRegion);

    sort(vecInstRegion.begin(), vecInstRegion.end());
    for (size_t cntHotInsn=0; cntHotInsn < vecInstRegion.size(); cntHotInsn++)  {
      printf(" HOT INSN region  %08lx - %08lx \n", vecInstRegion.at(cntHotInsn).first, vecInstRegion.at(cntHotInsn).second);
    }
    // Coalesce regions if they cover overlapping address range
    // Example  HOT INSN region  7fce42335d60 - 7fce432027bc
    //          HOT INSN region  7fce42335d64 - 7fce432027c0 
    uint64_t minRegionAddr, maxRegionAddr;
    typedef boost::icl::interval_set<uint64_t> set_t;
    typedef set_t::interval_type ival;
    set_t insnRegSet;
    for (size_t cntVecInsn=0; cntVecInsn < vecInstRegion.size(); cntVecInsn++)  {
      minRegionAddr = vecInstRegion[cntVecInsn].first;
      maxRegionAddr = vecInstRegion[cntVecInsn].second;     
      insnRegSet.insert(ival::open(minRegionAddr, maxRegionAddr)); 
    }
    vecInstRegion.clear();
    for(set_t::iterator it = insnRegSet.begin(); it != insnRegSet.end(); it++){
      std::cout <<" Interval Set "<< std::hex<< it->lower() << " - " << std::hex << it->upper() << "\n";
      vecInstRegion.push_back(make_pair(it->lower(),it->upper())); 
    }
    sort(vecInstRegion.begin(), vecInstRegion.end());
    for (size_t cntHotInsn=0; cntHotInsn < vecInstRegion.size(); cntHotInsn++)  {
      printf(" First HOT-INSN Interval region  %08lx - %08lx \n", vecInstRegion.at(cntHotInsn).first, vecInstRegion.at(cntHotInsn).second);
    }
    //getRegionforInst(&spatialOutInsnFile, vecInstAddr,2106708); // Alpaca trials
    /*---------------------------- HOT-INSN addition END ----------------------------------*/

    setRegionAddr.clear();
    MemArea insnMemArea; 
    // Check if RUD analysis reaches cacheLine level - if not do at higher level
    uint64_t spatiallastlvlBlockSize = cacheLineWidth;
    if(!spatialRegionList.empty()) {
      itrRegion = std::prev(spatialRegionList.end());
      thisMemblock = *itrRegion;
      spatiallastlvlBlockSize = thisMemblock.blockSize; 
    }

    // Copy final sorted region list to a new one  
    std::unordered_map<uint64_t, std::string> mapMinAddrToID;
    for (itrRegion=spatialRegionList.begin(); itrRegion != spatialRegionList.end(); ++itrRegion){
 	    thisMemblock = *itrRegion; 
      if( thisMemblock.blockSize == spatiallastlvlBlockSize) {
    	  minRegionAddr = thisMemblock.min;
    	  maxRegionAddr = thisMemblock.max;
        //printf("add Spatial set min-max %08lx-%08lx \n", minRegionAddr, maxRegionAddr);
        setRegionAddr.push_back(make_pair(minRegionAddr, maxRegionAddr));
        mapMinAddrToID[minRegionAddr] = thisMemblock.strID;
      }
    } 

    // SORT region by address - regionId updated to trace (in STEP 1.5)
    sort(setRegionAddr.begin(), setRegionAddr.end());
    for(uint32_t k=0; k<setRegionAddr.size(); k++) {
      printf("main Spatial set min-max %d %08lx-%08lx \n", k, setRegionAddr[k].first, setRegionAddr[k].second);
    }

    //XSBEnch trial event k-0 - START
    //vecInstRegion.clear();
    //setRegionAddr.clear();
    //spatialRegionList.clear();
    //HOT-INSN add closed REGION set min-max 7f01775255eb-7f01796d95ea

    //vecInstRegion.push_back(make_pair(stoull("556bed66d704",0,16), stoull("556bed67d703",0,16)));
    //vecInstRegion.push_back(make_pair(stoull("7f017249d5eb",0,16), stoull("7f0174ca55ea",0,16)));
    //vecInstRegion.push_back(make_pair(stoull("7f01754a7010",0,16), stoull("7f017f64e0e6",0,16)));
    //setRegionAddr.push_back(make_pair(stoull("556bed66d704",0,16), stoull("556bed67d703",0,16)));
    //setRegionAddr.push_back(make_pair(stoull("7f017249d5eb",0,16), stoull("7f0174ca55ea",0,16)));
    //setRegionAddr.push_back(make_pair(stoull("7f01754a7010",0,16), stoull("7f017f64e0e6",0,16)));

    //XSBEnch trial - END


    /*---------------------------- HOT-INSN addition START ----------------------------------*/
    // START - for-DEBUG
    insnRegSet.clear();
    for (size_t cntVecInsn=0; cntVecInsn < vecInstRegion.size(); cntVecInsn++)  {
      minRegionAddr = vecInstRegion[cntVecInsn].first;
      maxRegionAddr = vecInstRegion[cntVecInsn].second;     
      insnRegSet.insert(ival::closed(minRegionAddr, maxRegionAddr)); 
      printf("HOT-INSN add closed INSN set min-max %08lx-%08lx \n",  minRegionAddr, maxRegionAddr);
    }
    for(uint32_t k=0; k<setRegionAddr.size(); k++) {
      minRegionAddr =setRegionAddr[k].first;
      maxRegionAddr = setRegionAddr[k].second;
      insnRegSet.insert(ival::closed(minRegionAddr, maxRegionAddr)); 
      printf("HOT-INSN add closed REGION set min-max %08lx-%08lx \n",  minRegionAddr, maxRegionAddr);
    }
    for(set_t::iterator it = insnRegSet.begin(); it != insnRegSet.end(); it++){
      std::cout <<" Closed HOT-INSN Interval Set "<< std::hex<< it->lower() << " - " << std::hex << it->upper() << "\n";
    }
    // END - for-DEBUG

    insnRegSet.clear();
    for(uint32_t k=0; k<setRegionAddr.size(); k++) {
      minRegionAddr =setRegionAddr[k].first;
      maxRegionAddr = setRegionAddr[k].second;
      insnRegSet.insert(ival::closed(minRegionAddr, maxRegionAddr)); 
      printf("HOT-INSN add before region check REGION set min-max %08lx-%08lx \n",  minRegionAddr, maxRegionAddr);
    }
    // Add regions from STEP 0-a if the region is not covered in the Final-Regions-of-Interest
    for(uint32_t cntVecInsn=0; cntVecInsn<vecInstRegion.size(); cntVecInsn++) {
      bool flInsnRegInList=false;
      uint32_t cntIntervals = insnRegSet.iterative_size();
      minRegionAddr = vecInstRegion[cntVecInsn].first;
      maxRegionAddr = vecInstRegion[cntVecInsn].second;
      if (maxRegionAddr < heapAddrEnd) {
        insnRegSet.insert(ival::closed(minRegionAddr, maxRegionAddr)); 
          printf(" HOT-INSN cntIntervals %d SIZE %ld region not in list %08lx-%08lx \n", cntIntervals, insnRegSet.iterative_size(), minRegionAddr, maxRegionAddr);
        if( insnRegSet.iterative_size() <= cntIntervals) {
          printf(" ****** NOT HOT-INSN cntIntervals %d SIZE %ld region not in list %08lx-%08lx \n", cntIntervals, insnRegSet.iterative_size(), minRegionAddr, maxRegionAddr);
            flInsnRegInList = true;
        }
        if(flInsnRegInList ==false) {
          printf(" HOT-INSN region not in list %08lx-%08lx \n", minRegionAddr, maxRegionAddr);
          Memblock hotpage;
          string strNodeId; 
          strNodeId = "HotIns-"+std::to_string(cntVecInsn);
          hotpage.strID = strNodeId; 
	  	    hotpage.min = minRegionAddr; 
          hotpage.max =  maxRegionAddr ;
	    	  hotpage.level = 200; // random number - there's no level correlation
  	    	hotpage.blockSize = OSPageSize; 
		      hotpage.strParentID = "HotIns";
          memarea.max = maxRegionAddr;
          memarea.min = minRegionAddr;
          memarea.blockSize = OSPageSize;
          memarea.blockCount =  ceil((memarea.max - memarea.min)/(double)memarea.blockSize);
          vecBlockInfo.clear();
          for(i = 0; i< memarea.blockCount; i++){
            pair<unsigned int, unsigned int> blockID = make_pair(0, i);
            BlockInfo *newBlock = new BlockInfo(blockID, memarea.min+(i*memarea.blockSize), memarea.min+((i+1)*memarea.blockSize-1), 
                                              memarea.blockCount+cntAddPages, 0, strNodeId); // spatialResult=0
            vecBlockInfo.push_back(newBlock);
          }
          analysisReturn= getAccessCount(vecInstAddr,   memarea,  coreNumber , vecBlockInfo );
          if(analysisReturn ==-1) {
            printf("No analysis done\n");
            return -1;
          }
          pageTotalAccess.clear(); 
          for(i = 0; i< memarea.blockCount; i++){
            BlockInfo *curBlock = vecBlockInfo.at(i);
            curBlock->printBlockAccess(); 
            pageTotalAccess.push_back(curBlock->getTotalAccess());
          }
          // HOT-INSN zoom - if there's no hot-contiguous CHILD, get 2 top access CHILDREN with NON-HOT holes
          findHotPage(memarea, 4, sampleRud, pageTotalAccess,thresholdTotAccess, &zoomin, hotpage, zoomPageList);
          while(!zoomPageList.empty()) {
     				thisMemblock = zoomPageList.front();
 				    zoomPageList.pop_front();
      			spatialRegionList.push_back(thisMemblock); // Final-Regions-of-Interest
    	      minRegionAddr = thisMemblock.min;
         	  maxRegionAddr = thisMemblock.max;
            printf("add Spatial set min-max %08lx-%08lx \n", minRegionAddr, maxRegionAddr);
            setRegionAddr.push_back(make_pair(minRegionAddr, maxRegionAddr));
            mapMinAddrToID[minRegionAddr] = thisMemblock.strID;
          }
        }
      }
    }
    /*---------------------------- HOT-INSN addition END ----------------------------------*/
    
    sort(setRegionAddr.begin(), setRegionAddr.end());
    for(uint32_t k=0; k<setRegionAddr.size(); k++) {
      //printf("main Spatial set min-max %d %08lx-%08lx \n", k, setRegionAddr[k].first, setRegionAddr[k].second);
      for (itrRegion=spatialRegionList.begin(); itrRegion != spatialRegionList.end(); ++itrRegion){
 	      thisMemblock = *itrRegion; 
        if( (thisMemblock.blockSize == spatiallastlvlBlockSize) 
              || (thisMemblock.strID.find("HotIns") != std::string::npos))  // HOT-INSN addition
          {
            //printf(" Spatial set min-max  %08lx-%08lx \n", thisMemblock.min, thisMemblock.max);
          if (thisMemblock.min == setRegionAddr[k].first && thisMemblock.max == setRegionAddr[k].second) {
        	  finalRegionList.push_back(thisMemblock);
            //printf("final Spatial set min-max %d %08lx-%08lx \n", k, thisMemblock.min, thisMemblock.max);
          }
        }
      }
    }
    spatialRegionList.clear();
    // Final-Regions-of-Interest
    // STEP 0-b Get instructions that for corresponsing load addresses in the region
    for (itrRegion=finalRegionList.begin(); itrRegion != finalRegionList.end(); ++itrRegion){
 	    thisMemblock = *itrRegion; 
      insnMemArea.max = thisMemblock.max;
	   	insnMemArea.min = thisMemblock.min;
       printf("--insn  : Find instructions for %s in memRange ", memoryfile);
       printf(" %08lx-%08lx %s \n",thisMemblock.min,thisMemblock.max, (thisMemblock.strID).c_str());
       //--insn  : Find instructions for ../MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1.trace.final in memRange 56122007b08a-56122007f089
       spatialOutInsnFile << " --insn  : Find instructions in " << memoryfile << " for memRange " << hex<< insnMemArea.min << "-" 
                          << insnMemArea.max << " ID " << thisMemblock.strID << endl;
        getInstInRange(&spatialOutInsnFile, vecInstAddr,insnMemArea);
    }
    // STEP 1 - Calculate spatial affinity at data object (inter-region) level
    if(setRegionAddr.size() ==0) {
      printf("In Spatial Analysis Step 1 - Region address list is empty\n");
      return 0;
    }
    // STEP 1.5 - Update Trace with region Ids for addresses
    int updateReturn = updateTraceRegion(vecInstAddr ,setRegionAddr, heapAddrEnd);
    if(updateReturn !=0) {
      printf("Error in updateTraceRegion\n");
      return -1;
    }
    printf("Trace updated with updateTraceRegion \n");
    
    memarea.min = setRegionAddr[0].first;
    memarea.max = setRegionAddr[(setRegionAddr.size()-1)].second;
    memarea.blockCount = setRegionAddr.size();
    printf(" STEP1 in before zoom spatial %d size %ld count %d memarea.min %08lx memarea.max %08lx ID %s \n", thisMemblock.level, memarea.blockSize, 
                              memarea.blockCount, memarea.min, memarea.max, (thisMemblock.strID).c_str());
    for (itr_blk = vecBlockInfo.begin(); itr_blk != vecBlockInfo.end(); ++itr_blk) {
        delete (*itr_blk);
    }
    cntAddPages=1;
    memIncludePages.min = traceMin; 
    memIncludePages.max = traceMax; 
    memIncludePages.blockSize = OSPageSize;
    memIncludePages.blockCount = cntAddPages;
    vecBlockInfo.clear(); 
    for(i = 0; i< memarea.blockCount; i++){
        pair<unsigned int, unsigned int> blockID = make_pair(0, i);
        BlockInfo *newBlock = new BlockInfo(blockID, setRegionAddr[i].first, setRegionAddr[i].second, 
                                              memarea.blockCount+cntAddPages, spatialResult, mapMinAddrToID[setRegionAddr[i].first]); 
          vecBlockInfo.push_back(newBlock);
    }
    // Affinity analysis done for inter-region - region based range - to identify co-existence of objects
	  analysisReturn= spatialAnalysis( vecInstAddr, memarea, coreNumber, spatialResult, vecBlockInfo, setRegionAddr,
                                                memIncludePages,vecParentFamily, vecTopAccessLineAddr,0); 
    if(analysisReturn ==-1) {
      printf("Spatial Analysis at region level returned without results \n");
      return -1;
    }
    spatialOutFile << endl;
    uint64_t regionTotalAccess=0;
    for(i = 0; i< memarea.blockCount; i++){
         BlockInfo *curBlock = vecBlockInfo.at(i);
         regionTotalAccess += curBlock->getTotalAccess();
    }
    spatialOutFile<< ">---- New inter-region starts " << " MemoryArea " << hex<< memarea.min << "-" << memarea.max 
                  << " Total-access "<<std::dec << regionTotalAccess 
                  << " Block size " <<std::dec << zoomLastLvlPageWidth <<" Region count " << std::dec << memarea.blockCount << " -----" << endl; 
    for(i = 0; i< memarea.blockCount; i++){
         BlockInfo *curBlock = vecBlockInfo.at(i);
         curBlock->printBlockSpatialDensity(spatialOutFile,zoomLastLvlPageWidth, false);
    }
    spatialOutFile << endl;
     for(i = 0; i< memarea.blockCount; i++){
         BlockInfo *curBlock = vecBlockInfo.at(i);
         curBlock->printBlockSpatialProb(spatialOutFile,zoomLastLvlPageWidth, false);
    }
    spatialOutFile << endl;
     for(i = 0; i< memarea.blockCount; i++){
         BlockInfo *curBlock = vecBlockInfo.at(i);
         curBlock->printBlockSpatialInterval(spatialOutFile,zoomLastLvlPageWidth, false);
    }
    spatialOutFile << endl;

    // STEP 1 - Calculate spatial affinity at data object (inter-region) level
    // END - STEP 1 
    // STEP 2 -  Intra-region spatial affinity - affinity at cache-line level
    // STEP 2 -  Zoom into objects to find OS page sized hot blocks
    mapMinAddrToID.clear(); 
    for (itrRegion=finalRegionList.begin(); itrRegion != finalRegionList.end(); ++itrRegion){
 	    thisMemblock = *itrRegion; 
    	  memarea.max = thisMemblock.max;
  	    memarea.min = thisMemblock.min;
        mapMinAddrToID[memarea.min] = thisMemblock.strID;
        memarea.blockSize = OSPageSize; 
       	memarea.blockCount =  ceil((memarea.max - memarea.min)/(double)memarea.blockSize);
        printf(" STEP2 in before zoom spatial %d size %ld count %d memarea.min %08lx memarea.max %08lx ID %s \n", thisMemblock.level, 
                            memarea.blockSize, memarea.blockCount, memarea.min, memarea.max, (thisMemblock.strID).c_str());
	  	  printf("Memory address min %lx max %lx ", memarea.min, memarea.max);
  			printf(" page number = %d ", memarea.blockCount);
  			printf(" page size =  %ld\n", memarea.blockSize);
        for (itr_blk = vecBlockInfo.begin(); itr_blk != vecBlockInfo.end(); ++itr_blk) {
          delete (*itr_blk);
        }
        vecBlockInfo.clear();
        // Do not move these out of the loop - memarea.blockCount is different for each run
        for(i = 0; i< memarea.blockCount; i++){
          pair<unsigned int, unsigned int> blockID = make_pair(0, i);
          BlockInfo *newBlock = new BlockInfo(blockID, memarea.min+(i*memarea.blockSize), memarea.min+((i+1)*memarea.blockSize-1), 
                                              memarea.blockCount, 0, mapMinAddrToID[memarea.min]); 
          vecBlockInfo.push_back(newBlock);
        }
        analysisReturn= getAccessCount(vecInstAddr,   memarea,  coreNumber , vecBlockInfo );
        if(analysisReturn ==-1) {
          printf("Spatial Analysis Step 2 - Zoom into objects to find OS page sized %ld B hot blocks returned without results\n", OSPageSize);
          return -1;
        }
  	    pageTotalAccess.clear();
        for(i = 0; i< memarea.blockCount; i++){
          BlockInfo *curBlock = vecBlockInfo.at(i);
          curBlock->printBlockAccess();
          pageTotalAccess.push_back(curBlock->getTotalAccess());
        }
        zoomOption =3;
        findHotPage(memarea, zoomOption, sampleRud, pageTotalAccess,thresholdTotAccess, &zoomin, thisMemblock, zoomPageList);
        while(!zoomPageList.empty()) {
 	  			  thisMemblock = zoomPageList.front();
            //printf(" STEP 2 after zoom spatial list %d thisMemblock.min %08lx thisMemblock.max %08lx ID %s \n", thisMemblock.level, thisMemblock.min, 
            //            thisMemblock.max, thisMemblock.strID.c_str());
        		spatialOSPageList.push_back(thisMemblock);
   				  zoomPageList.pop_front();
        }
    }
    // END - STEP 2
    // STEP 2.5 - Create a map of parent-children regions
    std::vector<pair<uint64_t, uint64_t>> vecParentChild[finalRegionList.size()];
    std::unordered_map<std::string, uint8_t> mapParentIndex;
    int regionCnt=0;
    uint8_t parentIndex=0;
    for (itrRegion=finalRegionList.begin(); itrRegion != finalRegionList.end(); ++itrRegion){
      thisMemblock = *itrRegion;
      printf(" in spatial last %d size %ld count %d memarea.min %08lx memarea.max %08lx parent %s Id %s \n", thisMemblock.level, thisMemblock.blockSize, thisMemblock.blockCount, thisMemblock.min, thisMemblock.max, thisMemblock.strParentID.c_str(), thisMemblock.strID.c_str());
        vecParentChild[regionCnt].push_back(make_pair(thisMemblock.min, thisMemblock.max));
        mapParentIndex[(thisMemblock.strID.c_str())] = regionCnt;
        regionCnt++;
    }
    for (itrOSPage=spatialOSPageList.begin(); itrOSPage != spatialOSPageList.end(); ++itrOSPage){
 	    thisMemblock = *itrOSPage;
      //printf(" in spatial last %d size %ld count %d memarea.min %08lx memarea.max %08lx parent %s Id %s \n", thisMemblock.level, thisMemblock.blockSize, thisMemblock.blockCount, thisMemblock.min, thisMemblock.max, thisMemblock.strParentID.c_str(), thisMemblock.strID.c_str());
      parentIndex = mapParentIndex.find(thisMemblock.strParentID.c_str())->second;
      vecParentChild[parentIndex].push_back(make_pair(thisMemblock.min, thisMemblock.max));
    }
    // Debug for parentFamily
    /* 
    for( uint32_t dbg_i=0; dbg_i<finalRegionList.size(); dbg_i++)
    {
      vecParentFamily = vecParentChild[dbg_i];
      for (uint32_t dbg_j=0; dbg_j< vecParentFamily.size(); dbg_j++)
      {
 	     printf(" in spatial 2.5 memarea.min %08lx memarea.max %08lx \n",  vecParentFamily[dbg_j].first, vecParentFamily.at(dbg_j).second);
      }
    }
    */
    // STEP 2.6 - Get top-10 hot cache-lines in the finalRegionList
    uint8_t cntRegion=0;
    for (itrRegion=finalRegionList.begin(); itrRegion != finalRegionList.end(); ++itrRegion){
      thisMemblock = *itrRegion;
      parentIndex = mapParentIndex.find(thisMemblock.strID.c_str())->second;
      if(( thisMemblock.blockSize == spatiallastlvlBlockSize) || (thisMemblock.strID.find("HotIns") != std::string::npos)) {
        parentIndex = mapParentIndex.find(thisMemblock.strID.c_str())->second;
        vecParentFamily = vecParentChild[parentIndex];
        printf(" in spatial STEP2.6 last %d size %ld count %d memarea.min %08lx memarea.max %08lx parent %s Id %s \n", thisMemblock.level, 
                thisMemblock.blockSize, thisMemblock.blockCount, thisMemblock.min, thisMemblock.max, thisMemblock.strParentID.c_str(), thisMemblock.strID.c_str());
        int accessReturn = getTopAccessCountLines(vecInstAddr, thisMemblock, vecParentFamily, vecLineInfo , OSPageSize, cacheLineWidth,cntRegion);
        if(accessReturn !=0) {
          printf("Error - failed in getTopAccessCountLines\n");
          return 0;
        }
        cntRegion++;
      }
    }

    // STEP 2.6a - sort and get top 10
    std::vector <pair<uint32_t, uint64_t>> vecAccessCount;
    std::map<uint64_t, TopAccessLine> mapAddrHotLine;
    for(uint32_t dbg_j=0; dbg_j< vecLineInfo.size(); dbg_j++) {
      ptrTopAccessLine = *(vecLineInfo.at(dbg_j)); 
      //printf(" after spatial 2.6 regionId %d pageId %d lineId %d addrd %08lx access %d\n", ptrTopAccessLine.regionId, ptrTopAccessLine.pageId,  ptrTopAccessLine.lineId, ptrTopAccessLine.lowAddr, ptrTopAccessLine.accessCount);
      vecAccessCount.push_back(make_pair(ptrTopAccessLine.accessCount, ptrTopAccessLine.lowAddr));
      mapAddrHotLine[ptrTopAccessLine.lowAddr] = ptrTopAccessLine;
    }
    std::sort(vecAccessCount.begin(),vecAccessCount.end(),greater<>());
    uint8_t cntTopHotLines=10;
    if (vecAccessCount.size()<10) 
      cntTopHotLines = vecAccessCount.size();
    for(uint8_t cntVecAccess=0; cntVecAccess< cntTopHotLines; cntVecAccess++) {
      //printf(" after spatial 2.6a %d addr %08lx \n", vecAccessCount.at(cntVecAccess).first, vecAccessCount.at(cntVecAccess).second); 
      vecTopAccessLineAddr.push_back(vecAccessCount.at(cntVecAccess).second);
      ptrTopAccessLine = (mapAddrHotLine[vecAccessCount.at(cntVecAccess).second]);
      //printf(" after spatial 2.6 regionId %d pageId %d lineId %d addrd %08lx access %d\n", ptrTopAccessLine.regionId, ptrTopAccessLine.pageId, 
      //                                                 ptrTopAccessLine.lineId, ptrTopAccessLine.lowAddr, ptrTopAccessLine.accessCount);
      spatialOutFile<< "#---- Top Access line aff_blockid " << (OSPageSize/cacheLineWidth)+ cntVecAccess<< " Address "<< std::hex << ptrTopAccessLine.lowAddr 
                    << " Access " << std::dec<< ptrTopAccessLine.accessCount << " RegionID " << std::dec<< unsigned(ptrTopAccessLine.regionId) 
                    << " pageID "<<std::dec<< ptrTopAccessLine.pageId << " lineID " << std::dec<< ptrTopAccessLine.lineId <<endl; 
    }
    uint16_t affRegionId =0;
    for (itrRegion=finalRegionList.begin(); itrRegion != finalRegionList.end(); ++itrRegion){
      thisMemblock = *itrRegion;
      spatialOutFile<< "#---- Region Address Range aff_blockid " << (OSPageSize/cacheLineWidth)+ cntTopHotLines+affRegionId<< " Address "<< std::hex << thisMemblock.min  
                    << "-" << thisMemblock.max << " RegionID " << std::dec<< unsigned(affRegionId) << endl; 
      affRegionId++;
    }
      spatialOutFile<< "#---- Region Address Range aff_blockid " << (OSPageSize/cacheLineWidth)+ cntTopHotLines+affRegionId<< " Non-hot " << endl; 
      spatialOutFile<< "#---- Region Address Range aff_blockid " << (OSPageSize/cacheLineWidth)+ cntTopHotLines+affRegionId+1<< " Stack " << endl; 
    
    // HOTLINE info - Add hot lines instruction mapping to spatial_insn
    for(uint8_t cntVecAccess=0; cntVecAccess< cntTopHotLines; cntVecAccess++) {
      ptrTopAccessLine = (mapAddrHotLine[vecAccessCount.at(cntVecAccess).second]);
      insnMemArea.max = ptrTopAccessLine.lowAddr+cacheLineWidth;
	   	insnMemArea.min = ptrTopAccessLine.lowAddr;
      spatialOutInsnFile << " --insn  : Find instructions in " << memoryfile << " for memRange " << hex<< insnMemArea.min << "-" << insnMemArea.max << " ID hotline" << endl;
        getInstInRange(&spatialOutInsnFile, vecInstAddr,insnMemArea);
    }

    mapAddrHotLine.clear();

    // STEP 3 - Calculate spatial affinity at OS page level - using 64 B cache line 
    mapMinAddrToID.clear(); 
    for (itrOSPage=spatialOSPageList.begin(); itrOSPage != spatialOSPageList.end(); ++itrOSPage){
 	    thisMemblock = *itrOSPage; 
    	memarea.max = thisMemblock.max;
	    memarea.min = thisMemblock.min;
      mapMinAddrToID[memarea.min] = thisMemblock.strID;
      parentIndex = mapParentIndex.find(thisMemblock.strParentID.c_str())->second;
      vecParentFamily = vecParentChild[parentIndex];
      memarea.blockSize = cacheLineWidth; 
     	memarea.blockCount =  ceil((memarea.max - memarea.min)/(double)memarea.blockSize);
      //printf(" in spatial last %d size %ld count %d memarea.min %08lx memarea.max %08lx \n", thisMemblock.level, memarea.blockSize, memarea.blockCount, memarea.min, memarea.max);
      for (itr_blk = vecBlockInfo.begin(); itr_blk != vecBlockInfo.end(); ++itr_blk) {
        delete (*itr_blk);
      }
      vecBlockInfo.clear();
      printf(" STEP3 in spatial %d size %ld count %d memarea.min %08lx memarea.max %08lx ID %s \n", thisMemblock.level, memarea.blockSize, 
                    memarea.blockCount, memarea.min, memarea.max, (thisMemblock.strID).c_str());
		  printf("Memory address min %lx max %lx ", memarea.min, memarea.max);
			printf(" page number = %d ", memarea.blockCount);
			printf(" page size =  %ld\n", memarea.blockSize);
      // Exponential page segments
      /* 
      cntAddPages = 64;
      memIncludePages.min = traceMin; 
      memIncludePages.max = traceMax; 
      memIncludePages.blockSize = OSPageSize;
      memIncludePages.blockCount = cntAddPages;
      for(i = 0; i< memarea.blockCount; i++){
        pair<unsigned int, unsigned int> blockID = make_pair(0, i);
        BlockInfo *newBlock = new BlockInfo(blockID, memarea.min+(i*memarea.blockSize), memarea.min+((i+1)*memarea.blockSize-1), 
                                              memarea.blockCount+cntAddPages, spatialResult,mapMinAddrToID[memarea.min]); 
        vecBlockInfo.push_back(newBlock);
      }
      vecParentFamily.clear();
      vecTopAccessLineAddr.clear();
		  analysisReturn= spatialAnalysis( vecInstAddr, memarea, coreNumber, spatialResult, vecBlockInfo,  setRegionAddr, 
                                                   memIncludePages,vecParentFamily,  vecTopAccessLineAddr, 2);
      */

      // Hot Pages 
      /*
      vecTopAccessLineAddr.clear();
      for(i = 0; i< memarea.blockCount; i++){
        pair<unsigned int, unsigned int> blockID = make_pair(0, i);
        BlockInfo *newBlock = new BlockInfo(blockID, memarea.min+(i*memarea.blockSize), memarea.min+((i+1)*memarea.blockSize-1), 
                                              (memarea.blockCount+vecParentFamily.size()-1+setRegionAddr.size()+2),spatialResult, 
                                              mapMinAddrToID[memarea.min]); 
        vecBlockInfo.push_back(newBlock);
      }
		  analysisReturn= spatialAnalysis( vecInstAddr, memarea, coreNumber, spatialResult, vecBlockInfo,  setRegionAddr, 
                                             memIncludePages,vecParentFamily,  vecTopAccessLineAddr,3);
      */

      // Hot lines
      vecParentFamily.clear();
      for(i = 0; i< memarea.blockCount; i++){
        pair<unsigned int, unsigned int> blockID = make_pair(0, i);
        BlockInfo *newBlock = new BlockInfo(blockID, memarea.min+(i*memarea.blockSize), memarea.min+((i+1)*memarea.blockSize-1), 
                                              (memarea.blockCount+vecTopAccessLineAddr.size()+setRegionAddr.size()+2),spatialResult, 
                                              mapMinAddrToID[memarea.min]); 
        vecBlockInfo.push_back(newBlock);
      }
		  analysisReturn= spatialAnalysis(vecInstAddr, memarea, coreNumber, spatialResult, vecBlockInfo,setRegionAddr, 
                                             memIncludePages,vecParentFamily,  vecTopAccessLineAddr,4);
      if(analysisReturn ==-1) {
        printf("Spatial Analysis Step 3 - returned without results\n");
        return -1;
      }
      spatialOutFile << endl;
      regionTotalAccess = 0; 
      for(i = 0; i< memarea.blockCount; i++){
         BlockInfo *curBlock = vecBlockInfo.at(i);
         regionTotalAccess += curBlock->getTotalAccess();
      }
      spatialOutFile << "<---- New intra-region starts " << " MemoryArea " << hex<< memarea.min << "-" << memarea.max
                  << " Total-access "<<std::dec << regionTotalAccess 
                   << " Block size " <<std::dec <<  memarea.blockSize << " Block count " << memarea.blockCount <<" -----" << endl; 
      for(i = 0; i< memarea.blockCount; i++){
          BlockInfo *curBlock = vecBlockInfo.at(i);
          curBlock->printBlockSpatialDensity(spatialOutFile, cacheLineWidth, true);
      }
      spatialOutFile << endl;
      for(i = 0; i< memarea.blockCount; i++){
         BlockInfo *curBlock = vecBlockInfo.at(i);
         curBlock->printBlockSpatialProb(spatialOutFile,cacheLineWidth, true);
      }
      spatialOutFile << endl;
      for(i = 0; i< memarea.blockCount; i++){
         BlockInfo *curBlock = vecBlockInfo.at(i);
         curBlock->printBlockSpatialInterval(spatialOutFile,cacheLineWidth, true);
      }
    }
    // END - STEP 3
    spatialOutFile.close();
  }

  }// END Analysis and top-down zoom functionality	
  /******************************************************/
	//model start
   if(model == 1 ){
	   //initial memarea
	   if(memRange ==0){
			memarea.max = traceMax;
			memarea.min = traceMin;
		}else{
			memarea.max = user_max;
			memarea.min = user_min;
		}

	   if (phyPage == 0){
			memarea.blockCount = mempin;
			memarea.blockSize = ceil((memarea.max - memarea.min)/(double)mempin);
			if (memarea.blockSize < LLCacheOffset){
				printf("too small page size (<l1CacheOffset), use l1CacheOffset instead\n");
				memarea.blockCount =  ceil((memarea.max - memarea.min)/(double)l1CacheOffset);
				memarea.blockSize = l1CacheOffset;
			}
			printf("memory page number = %d\n", memarea.blockCount);
			printf("memory page size = %lx\n", memarea.blockSize);

		}else{
			memarea.blockCount =  ceil((memarea.max - memarea.min)/(double)pageSize);
			memarea.blockSize = pageSize;
			printf("memory page number = %d\n", memarea.blockCount);
			printf("memory page size = %lx\n", memarea.blockSize);
		}
		struct timeval t1, t2;
		gettimeofday(&t1, NULL);
		memoryModeling( memoryfile, memarea, coreNumber);
		gettimeofday(&t2, NULL);
		double time = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
		printf("done with %4.2f ms\n", time);
	}

  for (itr_tr = vecInstAddr.begin(); itr_tr != vecInstAddr.end(); ++itr_tr) {
        delete (*itr_tr);
  }
  vecInstAddr.clear();   
  for (itr_blk = vecBlockInfo.begin(); itr_blk != vecBlockInfo.end(); ++itr_blk) {
        delete (*itr_blk);
  }
  vecBlockInfo.clear();   
  sampleRud.clear(); 
  pageTotalAccess.clear(); 
  setRegionAddr.clear();
  return 0;
}
