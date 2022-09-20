#include "structure.h"
#include "memoryanalysis.h"
#include "memorymodeling.h"
using std::list;
// Global variables for threshold values
int32_t lvlConstBlockSize = 2; // Level for setting constant blocksize in zoom
double zoomThreshold = 0.10; // Threshold for access count - to include in zoom
//double zoomThreshold = 0.01 ; // Minivite uses this - Cluster paper data used different threshold values - to match regions to paper use this value 
uint64_t heapAddrEnd ; // 
uint64_t cacheLineWidth; // Added for ZoomRUD analysis - option to set last level's block width
uint64_t zoomLastLvlPageWidth ; // Added for ZoomRUD analysis - option to set last Zoom level's page width
uint64_t OSPageSize = 4096;

void findHotPage( MemArea memarea, int zoomOption, vector<double> Rud, 
                  vector<uint32_t>& pageTotalAccess, uint32_t thresholdTotAccess, int *zoomin, Memblock curPage, 
                  std::list<Memblock >& zoomPageList)
{
  //printf("findHotPage zoomOption %d  memarea.min = %08lx memarea.max = %08lx memarea.blockSize = %ld \n",  zoomOption, memarea.min, memarea.max, memarea.blockSize);
  uint32_t totalAccessParent = 0;
  int zoominaccess = 0;
  uint32_t wide = 0; 
  string strNodeId; 
  uint32_t i =0;
  uint32_t j =0;
  uint64_t levelOneSize ;
  if(zoomLastLvlPageWidth == 16384 || zoomLastLvlPageWidth == 4096)
    levelOneSize = 4194304*16;
  else
    levelOneSize = 4194304*8;
    
  vector <uint32_t> sortPageAccess;
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
      uint32_t tmpCompare;
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
          //if((hotpage.min+((hotpage.max-hotpage.min)/2)) <= heapAddrEnd) {
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
    // After 16384 - join and look for 1024 or 256
      for(i = 0; i<memarea.blockCount; i++  ){
       zoominaccess = 0;
	     wide = 0; 
  	   for(j = i; j<memarea.blockCount; j++){
	       if(pageTotalAccess.at(j) != 0){
	    		  zoominaccess = zoominaccess + pageTotalAccess.at(j);
	  	  	  wide++;
	  		  } else {
	  			  break;
	  		  }
	    }
      //Added to keep parents separate at the 'root+1' level
      //Access in Thread heap gets obscured by the stack access - so do not aggregate pages to region
      if(curPage.level == (lvlConstBlockSize-1)) {
        wide=1;
        zoominaccess = pageTotalAccess.at(i);
      }
      // 0.001 multiplier at level 1 - used for including heap address range at 'root+1' level 
      //if(((curPage.level == (lvlConstBlockSize-1)) && ((double)zoominaccess>=(double)0.001*totalAccessParent)) ||
      // Using 1000 - to include all heap regions
      if(((curPage.level == (lvlConstBlockSize-1)) && ((double)zoominaccess>=1000)) ||
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
        //printf("Add to list zoom in  wide = %d memarea.min = %08lx memarea.max = %08lx memarea.blockSize = %ld hotpage.level =%d hotpage min = %08lx max %08lx hotpage.blockSize = %ld \n",  wide, memarea.min, memarea.max, memarea.blockSize, hotpage.level, hotpage.min,hotpage.max, hotpage.blockSize);
        if (hotpage.level <lvlConstBlockSize) {
  	  	    hotpage.blockSize = memarea.blockSize;
        } else if(hotpage.level >= lvlConstBlockSize) { 
            hotpage.blockSize = ((levelOneSize)/((int) pow((double)4,(hotpage.level-lvlConstBlockSize))));
        }
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
        //printf("Add to list zoom in  memarea.min = %08lx memarea.max = %08lx memarea.blockSize = %ld hotpage.level =%d hotpage min = %08lx max %08lx hotpage.blockSize = %ld \n",  memarea.min, memarea.max, memarea.blockSize, hotpage.level, hotpage.min,hotpage.max, hotpage.blockSize);
      			  zoomPageList.push_back(hotpage);
  	    		  (*zoomin)++;
              childCount++;
        } else {
         printf(" Not zooming in stack region min = %08lx max = %08lx blockSize = %ld level =%d \n",  hotpage.min,hotpage.max, hotpage.blockSize, hotpage.level);
        }

      }
      if(curPage.level != (lvlConstBlockSize-1)) {
		    i=i+wide;
      }
    }
  }
  if(zoomOption ==3)
  {
    int cntListAdd=0;
     for(i = 0; i<memarea.blockCount; i++  ){
      if(pageTotalAccess.at(i) !=0 )
      {
        uint32_t cmpAccess;
        if(sortPageAccess.size()>=3)
          cmpAccess = sortPageAccess.at(2);
        else if (sortPageAccess.size()>=2)
          cmpAccess = sortPageAccess.at(1);
        else
          cmpAccess = sortPageAccess.at(0);
          
        if((pageTotalAccess.at(i) >= cmpAccess) && (cntListAdd <3) ) 
            // 10% misses hot blocks in highly active object
            // ((double)pageTotalAccess.at(i)>=(double)zoomThreshold*totalAccessParent))
        {
          Memblock hotpage;
          strNodeId = curPage.strID+std::to_string(childCount);
          hotpage.strID = strNodeId; 
	  	    hotpage.min = memarea.min+memarea.blockSize*(i);
          hotpage.max = memarea.min+(memarea.blockSize*(i+1))-1;
	    	  hotpage.level = curPage.level+1;
  	    	hotpage.blockSize = cacheLineWidth; 
          if(hotpage.max <= heapAddrEnd) {
            //printf("Add to list zoom in  memarea.min = %08lx memarea.max = %08lx memarea.blockSize = %ld hotpage.level =%d hotpage min = %08lx max %08lx hotpage.blockSize = %ld \n",  memarea.min, memarea.max, memarea.blockSize, hotpage.level, hotpage.min,hotpage.max, hotpage.blockSize);
      		  zoomPageList.push_back(hotpage);
            childCount++;
            cntListAdd++;
        }
      }
    }
  }
  }
  sortPageAccess.clear();
}

int writeZoomFile(const MemArea memarea, const Memblock thisMemblock, const vector<BlockInfo *>& vecBlockInfo, std::ofstream& zoomFile_det, 
                  uint32_t* thresholdTotAccess, int zoominTimes, int writeOption) 
{
  double * w_Rud; 
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
    w_Rud = new double [memarea.blockCount];   // Whole trace RUD
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
      w_Rud[i]=curBlock->getAvgRUD();
      w_sampleRud[i]=curBlock->getSampleAvgRUD();
      if(w_pageTotalAccess[i]!=0){
          //printf("level %d Non-Zero access %d %ld \n", thisMemblock.level, i, pageTotalAccess[i]);
        if(w_pageTotalAccess[i]==1)
          cntSingleAccessblocks++;
        printTotAccess+=w_pageTotalAccess[i];
        w_printPageAccess.push_back(w_pageTotalAccess[i]); 
        if(w_Rud[i] != -1){ 
          w_printPageRUD.push_back(w_Rud[i]);
          printAvgRUD+= w_Rud[i];
          printAvgRUDdiv+=1;
        }
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
                    << " "<< std::dec<< w_pageTotalAccess[i] << "(" << w_Rud[i]<< ","<<w_sampleRud[i]<< "); ";
        else
			      zoomFile_det << std::dec << "p"<< i << ": " 
                    //<<hex << blockMinAddr <<"-" << hex<< blockMaxAddr 
                    << " "<< std::dec<< w_pageTotalAccess[i] << "(" << w_Rud[i]<< ","<<w_sampleRud[i]<< "); ";
		  } 
    }
		zoomFile_det << endl;
  }
  w_printPageAccess.clear();
  w_printPageRUD.clear();
  w_printPageSampleRUD.clear();
  delete[] w_Rud; 
  delete[] w_sampleRud; 
  delete[] w_pageTotalAccess; 
  return 0;
}

int main(int argc, char ** argv){
   printf("-------------------------------------------------------------------------------------------\n");
   int argi = 1;
   char *memoryfile;
   char *outputFileZoom;
   char *outputFileSpatial;
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
	int out = 0;
	int outZoom = 0;
	int outSpatial = 0;
	int autoZoom = 0;
	int zoomOption = 0;
  int bottomUp = 0;
  int getInsn = 0;
  uint64_t min = stoull("FFFFFF",0,16); // Added for invalid load address checks - range reduced because heap addresses have 0x1d49620 format
  uint64_t max = stoull("8F0000000000", 0, 16); // Omit load addresses beyond stack range - 12 hex digits with 7F..
  uint64_t user_max = 0;
	uint64_t user_min = 0;
	char *qpoint;
	//pageSize = stol("40",0,16);
	pageSize = 64; 
  int intTotalTraceLine = 0;
  vector<TraceLine *> vecInstAddr;
  vector<TraceLine *>::iterator itr_tr;
  TraceLine *ptrTraceLine;
  vector<BlockInfo *> vecBlockInfo;
  vector<BlockInfo *>::iterator itr_blk;
  // Changed to include stack also - threaded application's heap is mapped to 0x7...
  // Minivite - Single threaded - stack starts at 0x7f
  // Others - multi-threaded stack starts at 0x7ff
  heapAddrEnd = stol("7ff000000000",0,16); // stack region is getting included in mid-point 

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
  }
  
	if(model == 1){
		if(loadConfig()==-1) return -1;
	}
  //check the memory area for the first time
	uint32_t totalAccess = 0;
  std::cout << "Application: " << memoryfile << "\n";
	printf("Reading trace with load address in range min %lx max %lx \n", min, max);
	//totalAccess = areacheck(memoryfile, &max, &min, &coreNumber);
  //if(totalAccess ==0 ) 
  //{
   // printf("Trace file not read - error in areacheck\n");
   // return -1;
  // }
  uint32_t windowMin;
  uint32_t windowMax;
  double windowAvg;
  windowMin=0; 
  windowMax=0;
  windowAvg=0.0;
  int readReturn = readTrace(memoryfile, &intTotalTraceLine, vecInstAddr, &windowMin, &windowMax, &windowAvg, &max, &min);
  if(readReturn == -1) {
    printf("Error in readTrace \n");
    return -1;
  }
  totalAccess = intTotalTraceLine;
  printf( "Number of lines in trace %d number of lines with valid addresses %ld total access %d \n", 
          intTotalTraceLine, vecInstAddr.size(), totalAccess);
	printf("Data from Trace load address min %lx max %lx \n", min, max);
  printf("Sample sizes in trace min %d max %d average %f \n", windowMin, windowMax, windowAvg);
  printf("Using Zoom perrcent value %lf\n", zoomThreshold); 
	printf("Using heap address range to max of %lx \n", heapAddrEnd);
  printf("Using cache line width %ld Bytes\n ", cacheLineWidth ); 
  printf("Using last level block/page size %ld Bytes\n", zoomLastLvlPageWidth); 
  printf("***************************************\n");
  bool done = 0;
  std::list<Memblock > zoomPageList;
  std::list<Memblock > spatialRegionList;
  std::list<Memblock > spatialOSPageList;
  std::list<Memblock>::iterator itrRegion;
  std::list<Memblock>::iterator itrOSPage;
  vector<pair<uint64_t, uint64_t>> setRegionAddr;
  int zoomin = 0;
  int zoominTimes = 0;
  ofstream zoomInFile_det;
  if( autoZoom ==1 ) {
    if(outZoom==1) {
      zoomInFile_det.open(outputFileZoom, std::ofstream::out | std::ofstream::trunc);
    } else {
      zoomInFile_det.open("zoomIn.txt", std::ofstream::out | std::ofstream::trunc);
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
  }
  uint32_t i=0;
  Memblock thisMemblock;
	thisMemblock.level=1;
  thisMemblock.strID="R";
  thisMemblock.strParentID="-";
  thisMemblock.leftPid=0;
  thisMemblock.rightPid=0;
	MemArea memarea;
	if(memRange ==0){
			memarea.max = max;
			memarea.min = min;
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
  vector<double> Rud; 
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
                                              memarea.blockCount, 0); //spatialResult =0
          vecBlockInfo.push_back(newBlock);
        }
        printf("Size of vector BlockInfo %ld\n", vecBlockInfo.size());
	  	  analysisReturn=spatialAnalysis( vecInstAddr, memarea, coreNumber, 0, out, vecBlockInfo,false, setRegionAddr); //spatialResult =0
        if(analysisReturn ==-1)
          return -1;
        for(i = 0; i< memarea.blockCount; i++){
          BlockInfo *curBlock = vecBlockInfo.at(i);
          curBlock->printBlockRUD();
        }
      }
    }
    // END bottom-up - TRIAL 
    /************************************************/
    // START Analysis and top-down zoom functionality 
    // Correlational RUD analysis not done in this loop - costly space & time overhead
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
		    printf("start memory analysis for itr %d ", zoominTimes);
		    //printf("Memory address min %lx max %lx min %ld max %ld \n", memarea.min, memarea.max, memarea.min, memarea.max);
		    printf("Memory address min %lx max %lx ", memarea.min, memarea.max);
				printf(" page number = %d ", memarea.blockCount);
				printf(" page size =  %ld\n", memarea.blockSize);
        for (itr_blk = vecBlockInfo.begin(); itr_blk != vecBlockInfo.end(); ++itr_blk) {
          delete (*itr_blk);
        }
        vecBlockInfo.clear();
        for(i = 0; i< memarea.blockCount; i++){
          pair<unsigned int, unsigned int> blockID = make_pair(0, i);
          // Correlational RUD analysis not done in this loop - costly space & time overhead
          BlockInfo *newBlock = new BlockInfo(blockID, memarea.min+(i*memarea.blockSize), memarea.min+((i+1)*memarea.blockSize-1), 
                                              memarea.blockCount, 0); // spatialResult=0
          vecBlockInfo.push_back(newBlock);
        }
		    analysisReturn= spatialAnalysis( vecInstAddr, memarea, coreNumber, 0, out, vecBlockInfo,false, setRegionAddr); // spatialResult=0
        if(analysisReturn ==-1) {
          printf("No analysis done\n");
          return -1;
        }
        printTotAccess=0;
        // Do not move these out of the loop - memarea.blockCount is different for each run
	      pageTotalAccess.clear();
        Rud.clear();
        sampleRud.clear();
        for(i = 0; i< memarea.blockCount; i++){
          BlockInfo *curBlock = vecBlockInfo.at(i);
          curBlock->printBlockRUD();
          pageTotalAccess.push_back(curBlock->getTotalAccess());
          Rud.push_back(curBlock->getAvgRUD());
          sampleRud.push_back(curBlock->getSampleAvgRUD());
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
         if (thisMemblock.blockSize == cacheLineWidth || thisMemblock.blockSize <= (4*zoomLastLvlPageWidth))
      			  spatialRegionList.push_back(thisMemblock);
 				 zoomPageList.pop_front();
  			 memarea.max = thisMemblock.max;
	  		 memarea.min = thisMemblock.min;
         if(zoomOption >=1 && thisMemblock.level >=lvlConstBlockSize){
           memarea.blockSize = thisMemblock.blockSize;
   	       memarea.blockCount =  ceil((memarea.max - memarea.min)/(double)memarea.blockSize);
           blCustomSize = 1;
          //printf(" in zoom level %d size %ld count %ld memarea.min %08lx memarea.max %08lx \n", thisMemblock.level, memarea.blockSize, memarea.blockCount, memarea.min, memarea.max);
         }
       } else {
          done =1;
          printf("Zoomlist empty \n");
       }
		} else
				 done = 1;
	} // END WHILE
	zoomInFile_det.close(); 
  // START - correlational RUD analysis
  // Steps
  // 0. Find instructions to map regions to objects - using getInstInRange
  // 1. Calculate spatial correlational RUD at data object (region) level
  // 2. Zoom into objects to find OS page sized (4096 B) hot blocks 
  // 3. Calculate spatial correlational RUD at OS page level - using 64 B cache line 
  if(spatialResult == 1)
  {
    printf("Spatial Analysis START \n");
    uint64_t minRegionAddr, maxRegionAddr;
    setRegionAddr.clear();
    // STEP 0   Get instructions that for corresponsing load addresses in the region
    MemArea insnMemArea; 
    // Check if RUD analysis reaches cacheLine level - if not do at higher level
    uint64_t spatiallastlvlBlockSize = cacheLineWidth;
    if(!spatialRegionList.empty()) {
      itrRegion = std::prev(spatialRegionList.end());
      thisMemblock = *itrRegion;
      spatiallastlvlBlockSize = thisMemblock.blockSize; 
    }
    for (itrRegion=spatialRegionList.begin(); itrRegion != spatialRegionList.end(); ++itrRegion){
 	    thisMemblock = *itrRegion; 
      if( thisMemblock.blockSize == spatiallastlvlBlockSize) {
    		insnMemArea.max = thisMemblock.max;
	    	insnMemArea.min = thisMemblock.min;
        printf("--insn  : Find instructions for %s in memRange ", memoryfile);
        printf("%08lx-%08lx\n",thisMemblock.min,thisMemblock.max);
        //--insn  : Find instructions for ../MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1.trace.final in memRange 56122007b08a-56122007f089
        spatialOutInsnFile << " --insn  : Find instructions for " << memoryfile << " in memRange " << hex<< insnMemArea.min << "-" << insnMemArea.max << endl;
        getInstInRange(&spatialOutInsnFile, vecInstAddr,insnMemArea);
      }
    }
    // STEP 1 - Calculate spatial correlational RUD at data object (region) level
    for (itrRegion=spatialRegionList.begin(); itrRegion != spatialRegionList.end(); ++itrRegion){
 	    thisMemblock = *itrRegion; 
      if( thisMemblock.blockSize == spatiallastlvlBlockSize) {
    	  minRegionAddr = thisMemblock.min;
  	    maxRegionAddr = thisMemblock.max;
        setRegionAddr.push_back(make_pair(minRegionAddr, maxRegionAddr));
      }
    }
    sort(setRegionAddr.begin(), setRegionAddr.end());
    //for(uint32_t k=0; k<setRegionAddr.size(); k++)
    //  printf("main Spatial set min-max %d %lx-%lx \n", k, setRegionAddr[k].first, setRegionAddr[k].second);
    if(setRegionAddr.size() ==0) {
      printf("In Spatial Analysis Step 1 - Region address list is empty\n");
      return 0;
    }
    memarea.min = setRegionAddr[0].first;
    memarea.max = setRegionAddr[(setRegionAddr.size()-1)].second;
    memarea.blockCount = setRegionAddr.size();
    for (itr_blk = vecBlockInfo.begin(); itr_blk != vecBlockInfo.end(); ++itr_blk) {
        delete (*itr_blk);
    }
    vecBlockInfo.clear(); 
    for(i = 0; i< memarea.blockCount; i++){
        pair<unsigned int, unsigned int> blockID = make_pair(0, i);
        BlockInfo *newBlock = new BlockInfo(blockID, setRegionAddr[i].first, setRegionAddr[i].second, 
                                              memarea.blockCount, spatialResult); 
          vecBlockInfo.push_back(newBlock);
      }
        // Correlational RUD analysis done at region based range - to identify co-existence of objects
	   analysisReturn= spatialAnalysis( vecInstAddr, memarea, coreNumber, spatialResult, out, vecBlockInfo, true, setRegionAddr); 
     if(analysisReturn ==-1) {
      printf("Spatial Analysis at region level returned without results \n");
      return -1;
     }
     spatialOutFile<< "***** New region starts " << " MemoryArea " << hex<< memarea.min << "-" << memarea.max 
                  << " Block size " <<std::dec << zoomLastLvlPageWidth <<" Region count " << std::dec << memarea.blockCount << endl; 
     for(i = 0; i< memarea.blockCount; i++){
         BlockInfo *curBlock = vecBlockInfo.at(i);
         curBlock->printBlockSpatialDensity(spatialOutFile,zoomLastLvlPageWidth);
    }
    spatialOutFile << endl;
     for(i = 0; i< memarea.blockCount; i++){
         BlockInfo *curBlock = vecBlockInfo.at(i);
         curBlock->printBlockSpatialProb(spatialOutFile,zoomLastLvlPageWidth);
    }
    // END - STEP 1 
    /*
    // STEP 2 -  Zoom into objects to find OS page sized (4096 B) hot blocks
    for (itrRegion=spatialRegionList.begin(); itrRegion != spatialRegionList.end(); ++itrRegion){
 	    thisMemblock = *itrRegion; 
    	memarea.max = thisMemblock.max;
	    memarea.min = thisMemblock.min;
      memarea.blockSize = OSPageSize; 
     	memarea.blockCount =  ceil((memarea.max - memarea.min)/(double)memarea.blockSize);
      // FOR 100 line trace - calculate SD at root level for verification
	    //totalAccess = areacheck(memoryfile, &max, &min, &coreNumber);
    	//memarea.max = max;
	    //memarea.min = min;
     	//memarea.blockCount =  mempin; 
	    //memarea.blockSize = ceil((memarea.max - memarea.min)/(double)mempin);
      printf(" in spatial %d size %ld count %d memarea.min %08lx memarea.max %08lx \n", thisMemblock.level, memarea.blockSize, memarea.blockCount, memarea.min, memarea.max);
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
                                              memarea.blockCount, 0); 
          vecBlockInfo.push_back(newBlock);
      }
		  analysisReturn= spatialAnalysis( vecInstAddr, memarea, coreNumber, 0, out, vecBlockInfo,false, setRegionAddr); // spatialResult=0
      if(analysisReturn ==-1) {
          printf("Spatial Analysis Step 2 - Zoom into objects to find OS page sized (4096 B) hot blocks returned without results\n");
          return -1;
      }
	    pageTotalAccess.clear();
      Rud.clear();
      sampleRud.clear();
      for(i = 0; i< memarea.blockCount; i++){
          BlockInfo *curBlock = vecBlockInfo.at(i);
          curBlock->printBlockRUD();
          pageTotalAccess.push_back(curBlock->getTotalAccess());
          sampleRud.push_back(curBlock->getSampleAvgRUD());
      }
      zoomOption =3;
      findHotPage(memarea, zoomOption, sampleRud, pageTotalAccess,thresholdTotAccess, &zoomin, thisMemblock, zoomPageList);
      while(!zoomPageList.empty()) {
 				  thisMemblock = zoomPageList.front();
          printf(" in spatial zoom list %d thisMemblock.min %08lx thisMemblock.max %08lx \n", thisMemblock.level, thisMemblock.min, thisMemblock.max);
      		spatialOSPageList.push_back(thisMemblock);
 				  zoomPageList.pop_front();
      }
    }
      // END - STEP 2
      // STEP 3 - Calculate spatial correlational RUD at OS page level - using 64 B cache line 
      for (itrOSPage=spatialOSPageList.begin(); itrOSPage != spatialOSPageList.end(); ++itrOSPage){
 	    thisMemblock = *itrOSPage; 
    	memarea.max = thisMemblock.max;
	    memarea.min = thisMemblock.min;
      memarea.blockSize = cacheLineWidth; 
     	memarea.blockCount =  ceil((memarea.max - memarea.min)/(double)memarea.blockSize);
      printf(" in spatial last %d size %ld count %d memarea.min %08lx memarea.max %08lx \n", thisMemblock.level, memarea.blockSize, memarea.blockCount, memarea.min, memarea.max);
      for (itr_blk = vecBlockInfo.begin(); itr_blk != vecBlockInfo.end(); ++itr_blk) {
        delete (*itr_blk);
      }
      vecBlockInfo.clear();
      for(i = 0; i< memarea.blockCount; i++){
        pair<unsigned int, unsigned int> blockID = make_pair(0, i);
        BlockInfo *newBlock = new BlockInfo(blockID, memarea.min+(i*memarea.blockSize), memarea.min+((i+1)*memarea.blockSize-1), 
                                              memarea.blockCount, spatialResult); 
        vecBlockInfo.push_back(newBlock);
      }
      //printf(" in spatial last %d size %ld count %d memarea.min %08lx memarea.max %08lx \n", thisMemblock.level, memarea.blockSize, memarea.blockCount, memarea.min, memarea.max);
		  printf("Memory address min %lx max %lx ", memarea.min, memarea.max);
			printf(" page number = %d ", memarea.blockCount);
			printf(" page size =  %ld\n", memarea.blockSize);
		  analysisReturn= spatialAnalysis( vecInstAddr, memarea, coreNumber, spatialResult, out, vecBlockInfo, false, setRegionAddr);
      if(analysisReturn ==-1) {
        printf("Spatial Analysis Step 2 - Zoom into objects to find OS page sized (4096 B) hot blocks returned without results\n");
        return -1;
      }
      spatialOutFile << "***** New region starts " << " MemoryArea " << hex<< memarea.min << "-" << memarea.max
                   << " Block size " <<std::dec <<  memarea.blockSize << " Block count " << memarea.blockCount << endl; 
      for(i = 0; i< memarea.blockCount; i++){
          BlockInfo *curBlock = vecBlockInfo.at(i);
          curBlock->printBlockSpatial(spatialOutFile, cacheLineWidth);
      }
    }
    // END - STEP 3
    */
    spatialOutFile.close();
  }

  }// END Analysis and top-down zoom functionality	
  /******************************************************/
	//model start
   if(model == 1 ){
	   //initial memarea
	   if(memRange ==0){
			memarea.max = max;
			memarea.min = min;
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
  Rud.clear(); 
  sampleRud.clear(); 
  pageTotalAccess.clear(); 
  setRegionAddr.clear();
  return 0;
}
