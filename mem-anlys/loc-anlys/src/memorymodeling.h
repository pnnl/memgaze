#ifndef MODELING_H
#define MODELING_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <bits/stdc++.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <unistd.h>
#include <sys/time.h>
#include "structure.h"
//#define HMC

#ifdef HMC
#include "hmc_sim.h"
#endif

#define FIFO 0 
#define LIFO 1
#define PAGEHIT 2

uint64_t addrThreshold = stoull("FFFFFFFF",0,16);   //Threshold for checking if the addr is a offsit of PTR write
uint64_t pageSize; 

int phyPage;
uint32_t mempin;

//configruation parameters
int queueNumber;
int queuePolicy;

int fastMemory;     // Fast Memory Latency
int slowMemory;    // Slow Memory Latency
int Maxlength;     // max Queue length

int l1CacheWide;
int l1CacheLatency;
int LLCacheWide;
int LLCacheLatency;
uint64_t l1CacheOffset; // how many L1 cache_line per block
uint64_t LLCacheOffset; // how many LL cache_line per block

#ifdef HMC
//hmc configuration and function
	/* vars */
	uint32_t num_devs	= 0;
	uint32_t num_links	= 0;
	uint32_t num_vaults	= 0;
	uint32_t queue_depth	= 0;
	uint32_t num_banks	= 0;
	uint32_t num_drams	= 0;
	uint32_t capacity	= 0;
	uint32_t xbar_depth	= 0;	
	uint32_t read_perct	= 50;
	uint32_t write_perct	= 50;	
	uint32_t seed		= 0;
	uint32_t bsize		= 128;
	/* ---- */
int getshiftamount( 	uint32_t num_links, 
				uint32_t capacity, 
				uint32_t bsize,
				uint32_t *shiftamt )
{

	if( num_links == 4 ){ 
		/* 
		 * 4 link devices
		 * 
		 */
		if( capacity == 2 ){ 
			/* 
			 * 2GB capacity 
	 		 *
			 */
			switch( bsize )	
			{
				case 32 : 
					*shiftamt = 5;
					break;
				case 64 : 
					*shiftamt = 6;
					break;
				case 128:
					*shiftamt = 7;
					break;
				default:
					return -1;
					break;
			}
		}else if( capacity == 4 ){
			/* 
			 * 4GB capacity 
	 		 *
			 */
			switch( bsize )	
			{
				case 32 : 
					*shiftamt = 5;
					break;
				case 64 : 
					*shiftamt = 6;
					break;
				case 128:
					*shiftamt = 7;
					break;
				default:
					return -1;
					break;
			}
		}else{ 
			return -1;
		}
	}else if( num_links == 8 ){
		/* 
		 * 8 link devices
		 * 
		 */
		if( capacity == 4 ){ 
			/* 
			 * 4GB capacity 
	 		 *
			 */
			switch( bsize )	
			{
				case 32 : 
					*shiftamt = 5;
					break;
				case 64 : 
					*shiftamt = 6;
					break;
				case 128:
					*shiftamt = 7;
					break;
				default:
					return -1;
					break;
			}
		}else if( capacity == 8 ){ 
			/* 
			 * 8GB capacity 
	 		 *
			 */
			switch( bsize )	
			{
				case 32 : 
					*shiftamt = 5;
					break;
				case 64 : 
					*shiftamt = 6;
					break;
				case 128:
					*shiftamt = 7;
					break;
				default:
					return -1;
					break;
			}
		}else{
			return -1;
		}
	}else{
		return -1;
	}	
	return 0;
}


/* ---------------------------------------------------- ZERO_PACKET */
/* 
 * ZERO_PACKET 
 * 
 */
static void zero_packet( uint64_t *packet )
{
	uint64_t i = 0x00ll;

	/* 
	 * zero the packet
	 * 
	 */
	for( i=0; i<HMC_MAX_UQ_PACKET; i++ ){
		packet[i] = 0x00ll;
	} 


	return ;
}

#endif

//queue structure
struct ChannelQueue {
	MemArea memarea;         // channel wide
	int length;			     // record dynamic q length
	int stall;
	int tag;
	int processinglength;	//Memory in latency, module by instruction count instead of cycle
	int transmitlength; // transmit latency from remote core
	int Qhead;				// record time of Qhead
	int * QpageID; // record the page ID
	int * Qtail;	//  record the Arrive time of the access	
	int * Qissue;  // record the issue time for the access
	int * Qkey;   //  record Q key value 
	uint64_t * Qaddr; // record Q addr
	int * Qsent;
};

//cache structure
struct Cache {
	MemArea memarea;  //cache address space (Not used right now)
	int wide;    //cache size
	int length;              //record dynamic cache fill rate
	int policy = 0;          //current only use LRU
	int latency;     
	int * cacheline; //record cached page ID
    int * cachekey;  //record cache key value: issue time, hit rate 

};


int loadConfig() 
{ 


	queueNumber =1;
	queuePolicy = 0;

	//queue default configuration
	
	fastMemory = 100;     // Fast Memory Latency
	slowMemory = 100;    // Slow Memory Latency
	Maxlength = 2048;     // max Queue length
	//cache default configuration
	l1CacheWide = 800;
	l1CacheLatency = 5;
	LLCacheWide = 4000;
	LLCacheLatency = 20;
	l1CacheOffset = stol("FFFF",0, 16);
	LLCacheOffset = stol("FF",0,16);

 // File pointer 
    fstream fin0; 

	string filename = "model.config";
  
    // Open an existing file 
   fin0.open(filename, ios::in);

    // read the state line

	string opt;
    string line, value;
     
	//printf("Modeling configuration:\n");

    while (!fin0.eof()){ 
  
		getline(fin0, line);
		if(line[0] == '-'){
			std::stringstream s(line);
			getline(s,opt,' '); 
			getline(s,value,' ');
	
			if (opt.compare("--queueNumber") == 0){
			printf("configuration queueNumber = ");
			queueNumber = stoi(value);
 			printf("%d\n", queueNumber);
			}

			else if (opt.compare("--queuePolicy") == 0){
			printf("configuration queuePolicy = ");
			queuePolicy = stoi(value);
 			printf("%d\n", queuePolicy);
			}

			else if (opt.compare("--fastMemory") == 0){
			printf("configuration fastMemory = ");
			fastMemory = stoi(value);
 			printf("%d\n", fastMemory);
			}

			else if (opt.compare("--slowMemory") == 0){
			printf("configuration slowMemory = ");
			slowMemory = stoi(value);
 			printf("%d\n", slowMemory);
			}

			else if (opt.compare("--Maxlength") == 0){
			printf("configuration Queue Maxlength = ");
			Maxlength = stoi(value);
 			printf("%d\n", Maxlength);
			}

			else if ((opt.compare("--pageSize") == 0)&&(phyPage == 1)){
			printf("configuration physical pageSize = ");
			pageSize = stol(value,0,16);
 			printf("%08lx\n", pageSize);
			}

			else if (opt.compare("--l1CacheWide") == 0){
			printf("configuration l1CacheWide = ");
			l1CacheWide = stoi(value);
 			printf("%d\n", l1CacheWide);
			}

			else if (opt.compare("--l1CacheLatency") == 0){
			printf("configuration l1CacheLatency = ");
			l1CacheLatency = stoi(value);
 			printf("%d\n", l1CacheLatency);
			}

			else if (opt.compare("--LLCacheWide") == 0){
			printf("configuration LLCacheWide = ");
			LLCacheWide = stoi(value);
 			printf("%d\n", LLCacheWide);
			}

			else if (opt.compare("--LLCacheLatency") == 0){
			printf("configuration LLCacheLatency = ");
			LLCacheLatency = stoi(value);
 			printf("%d\n", LLCacheLatency);
			}

			else if (opt.compare("--l1CacheOffset") == 0){
			printf("configuration l1CacheOffset = ");
			l1CacheOffset = stol(value,0,16);
 			printf("%lx\n", l1CacheOffset);
			}

			else if (opt.compare("--LLCacheOffset") == 0){
			printf("configuration LLCacheOffset = ");
			LLCacheOffset = stol(value,0,16);
 			printf("%lx\n", LLCacheOffset);
			}
		}

	}
    fin0.close();    
    
	printf("\n");




#ifdef HMC
    // File pointer 
    fstream fin; 

	string HMCfile = "hmc.config";
  
    // Open an existing file 
    fin.open(HMCfile, ios::in); 

    // read the state line
     
	printf("HMC configuration:");

    while (!fin.eof()){ 
  
		getline(fin, line);
		std::stringstream s(line);
		getline(s,opt,' '); 
		getline(s,value,' ');
	
		switch( opt[0] )
		{
			case 'b': 
				num_banks = (uint32_t)(stoi(value));
				cout << "BANK:" << num_banks << ";";
				break;
			case 'c':
				capacity = (uint32_t)(stoi(value));
				cout << "CAPACITY:" << capacity << ";";
				break;
			case 'd': 
				num_drams = (uint32_t)(stoi(value));
				cout << "DRAMS:" << num_drams << ";";
				break;
			case 'l':
				num_links = (uint32_t)(stoi(value));
				cout << "LINKS:" << num_links << ";";
				break;
			case 'm':
				bsize 	= (uint32_t)(stoi(value));
				cout << "BSIZE:" << bsize << ";";
				break;
			case 'n':
				num_devs = (uint32_t)(stoi(value));
				cout << "DEVS:" << num_devs << ";";
				break;
			case 'q':
				queue_depth = (uint32_t)(stoi(value));
				cout << "QDEPTH:" << queue_depth << ";";
				break;
			case 'v': 
				num_vaults = (uint32_t)(stoi(value));
				cout << "VAULTS:" << num_vaults << ";";
				break;
			case 'x': 
				xbar_depth = (uint32_t)(stoi(value));
				cout << "XDEPTH:" << xbar_depth << ";";
				break;
			case 'R': 
				read_perct = (uint32_t)(stoi(value));
				cout << "READ:" << read_perct << ";";
				break;
			case 'S':
				seed = (uint32_t)(stoi(value));
				cout << "SEED:" << seed << ";";
				break;
			case 'W':
				write_perct = (uint32_t)(stoi(value));
				cout << "WRITE:" << write_perct << ";";
				break;
			default:
				break;
		}

	}
    fin.close();    
    
	printf("\n");

	/* 
	 * sanity check the runtime args 
	 * 
	 */
	if( (read_perct+write_perct)!=100 ){ 
		printf( "FAILED TO VALIDATE ARGS: READ+WRITE PERCENTAGE MUST == 100\n" );
		return -1;
	}

	if( (num_devs < 1 ) || ( num_devs > 8 ) ){
		printf( "FAILED TO VALIDATE ARGS: NUM DEVS OUT OF BOUNDS\n" );
		return -1;
	}

	if( (num_links != 4) && (num_links != 8) ){
		printf( "FAILED TO VALIDATE ARGS: NUM LINKS OUT OF BOUNDS\n" );
		return -1;
	}

#endif

	return 1;
}


bool cacheAccess(Cache * cache, int lineID, int totalinst, uint64_t time){
	bool miss = true;
	int hitID = 0;
	for(int i = 0; i< cache->length; i++){
		if( lineID == cache->cacheline[i]) {
			miss = false;
			hitID = i;
		}
	}

	if (miss == false){ //Hit, update the key value
		switch(cache->policy){
			case FIFO:{
				cache->cachekey[hitID] = time; //most recent one
			}
			case LIFO:{
			}
			case PAGEHIT:{
				cache->cachekey[hitID] ++; //new
			}
		}
	}
	else if (miss == true){ //Miss, resert a cacheline and send to queue
		//cacheMiss[cacheID]++;
		if(cache->length==cache->wide){ //check if cache full, replace one inside the cache
			int target = 0;
			switch(cache->policy){ //find target
				case FIFO:{      //LRU
					int oldest = time;
					for (int i = 0; i < cache->length; i++){   // search the oldest in the cache
						if (cache->cachekey[i]<oldest){
							oldest = cache->cachekey[i];
							target = i;
						}
					}
					cache->cacheline[target] = lineID;
					cache->cachekey[target] = time; //most recent one
				}
				case LIFO:{
				}
				case PAGEHIT:{
					int hitcount = totalinst;
					for (int i = 0; i < cache->length; i++){   // search the oldest in the cache
						if (cache->cachekey[i]<hitcount){
							hitcount = cache->cachekey[i];
							target = i;
						}
					}
					cache->cacheline[target] = lineID;
					cache->cachekey[target] = 0; //new
				}
			}
		}
		else{ //cache not full, insert to the end
			cache->cacheline[cache->length] = lineID;
			switch(cache->policy){
				case FIFO:{
					cache->cachekey[cache->length] = time; //most recent one
				}
				case LIFO:{
				}
				case PAGEHIT:{
					cache->cachekey[cache->length] = 0; //new
				}
			}
			cache->length++;
		}
	}
	return miss;
}

//Use to model the memory performance
//Data stored as <IP addr core initialtime\n>
int memoryModeling(string filename, MemArea memarea, int coreNumber){

	printf("========start memory modeling==========\n");
	printf("Core Number: %d\n", coreNumber);
	printf("Memory Hierachy: 3 level\n");
	printf("Cache Size: %d\n", l1CacheWide);
	printf("Cache Latency: %d\n", l1CacheLatency);
	printf("Cache Policy: LRU\n");
	printf("Last Level Cache Size: %d\n", LLCacheWide);
	printf("Last Level Cache Latency: %d\n", LLCacheLatency);
	printf("Last Level Cache Policy: LRU\n");
	printf("Memory Organize: %d queues\n", queueNumber);
	printf("Queue Latency: %d\n", slowMemory);
	printf("Queue policy: %d\n", queuePolicy);
	switch(queuePolicy){
		case FIFO:{
			printf("FIFO\n");
			break;
				  }
		case LIFO:{
			printf("LIFO\n");
			break;
				  }
		case PAGEHIT:{
			printf("PageHit\n");
			break;
					 }
	}

 // File pointer 
    fstream fin; 
  
    // Open an existing file 
    fin.open(filename, ios::in); 

    // read the state line
    
    string line,ip2,addr,core, inittime;
	core = "0";
	//initial the memory queue
	//int queueNumber = 2;
	ChannelQueue *cq = new ChannelQueue [queueNumber];   //queue 
	uint64_t *totalQlength = new uint64_t [queueNumber];			//record queue length for contension
	int *maxQlength = new int [queueNumber];            //max queue length
	int *totalinstQ = new int [queueNumber];            //#queue access
	int *processedinstQ = new int [queueNumber];        //#finished access
	uint64_t *totalQlatency = new uint64_t [queueNumber];         //record latency


	//initial the core
    //int corenumber = 2;
    int ** totalCAccess = new int * [coreNumber];     //core access

	//initial cache, each core has one cache
	//int cachenumber = corenumber
	Cache *cache = new Cache [coreNumber];    //cache
	int *cacheMiss = new int [coreNumber];            //record #miss
	//int ** totalAccess_C = new int  * [coreNumber];   //record page access of the cache
	//int ** lastAccess_C = new int  * [coreNumber];    //record page access time of the core
	//int ** distance_C = new int  * [coreNumber];      //record page RUD of the core
	int * time_C = new int [coreNumber];              //inst from core

	//initial LLC, each queue has one LLC
	Cache *LLCache = new Cache [queueNumber]; // LLC
	int *LLCMiss = new int [queueNumber];            //record #miss
	//int ** totalAccess_LLC = new int  * [queueNumber];   //record page access of the cache
	//int ** lastAccess_LLC = new int  * [queueNumber];    //record page access time of the core
	//int ** distance_LLC = new int  * [queueNumber];      //record page RUD of the core
	int * time_LLC = new int [queueNumber];              //inst from queue

	//core,cache parameter
	for(int i = 0; i < coreNumber; i++) {
		 totalCAccess[i] = new int [queueNumber];
		 //distance_C[i] = new int  [memarea.blockCount];
		 //totalAccess_C[i] = new int [memarea.blockCount];
		 //lastAccess_C[i] = new int [memarea.blockCount];
		 cache[i].length=0;
		 cache[i].wide=l1CacheWide;
		 cache[i].latency=l1CacheLatency;
		 cache[i].cacheline = new int [l1CacheWide];
		 cache[i].cachekey = new int [l1CacheWide];

		 for(int j = 0; j<cache[i].wide; j++){
			 cache[i].cacheline[j]=0;
			 cache[i].cachekey[j]=0;
		 }
		 cacheMiss[i] = 0;
		 time_C[i] = 0;


	}

	for(int i = 0; i < queueNumber; i++) {
		 //totalCAccess[i] = new int [queueNumber];
		 //distance_C[i] = new int  [memarea.blockCount];
		 //totalAccess_C[i] = new int [memarea.blockCount];
		 //lastAccess_C[i] = new int [memarea.blockCount];
		 LLCache[i].length=0;
		 LLCache[i].wide= LLCacheWide;
		 LLCache[i].latency=LLCacheLatency;
		 LLCache[i].cacheline = new int [LLCacheWide];
		 LLCache[i].cachekey = new int [LLCacheWide];
		 for(int j = 0; j<LLCache[i].wide; j++){
			 LLCache[i].cacheline[j]=0;
			 LLCache[i].cachekey[j]=0;
		 }
		 LLCMiss[i] = 0;
		 time_LLC[i] = 0;
	}
/*
	for(int i = 0; i < memarea.blockCount; i++){
		for(int j = 0; j < coreNumber; j++) {
			//distance_C[j][i] = 0;
			//totalAccess_C[j][i] = 0;
			//lastAccess_C[j][i] = 0;
		}
	}
*/
	//queue parameter
	int widesize = ceil((double)memarea.blockCount/(double)queueNumber);
	for(int i = 0; i<queueNumber; i++){
		cq[i].memarea.min = memarea.min+i*(widesize)*memarea.blockSize;
		cq[i].memarea.max = memarea.min+(i+1)*(widesize)*memarea.blockSize;	
		cq[i].length = 0;
		cq[i].Qhead = 0;
		cq[i].stall = 0;
		cq[i].tag = 0;
		cq[i].processinglength = fastMemory;
		cq[i].transmitlength = slowMemory;
		cq[i].QpageID = new int [Maxlength]; // record the page ID
		cq[i].Qtail = new int [Maxlength];	//  record the Arrive time of the access	
		cq[i].Qissue = new int [Maxlength];  // record the issue time for the access
		cq[i].Qkey = new int [Maxlength];   //  record Q key value 
		cq[i].Qaddr  = new uint64_t [Maxlength]; // record Q addr
		cq[i].Qsent = new int [Maxlength];

		for(int j = 0; j < coreNumber; j++) {
		 totalCAccess[j][i] = 0;

		}
	
		for (int j = 0; j < Maxlength; j++){
			cq[i].QpageID[j] = -1;
			cq[i].Qtail[j] = 0;
			cq[i].Qissue[j] = 0;
			cq[i].Qkey[j] = 0;
			cq[i].Qsent[j] = 0;
			cq[i].Qaddr[j] = 0;
		}
        
		//slow memory and fast memory identify
        //if (i< queueNumber/2) cq[i].processinglength = fastMemory;
		//else cq[i].processinglength = slowMemory;

		totalQlength[i] = 0;
		maxQlength[i]=0;
		totalinstQ[i]=0;
		totalQlatency[i]=0;
		processedinstQ[i]=0;
	}

#ifdef HMC
	//init hmc
	long num_req= 0;
	uint32_t shiftamt= 0;
    hmcsim_t hmc;

		/* vars */
	uint32_t z		= 0x00;
	long iter		= 0x00l;
	uint64_t head		= 0x00ll;
	uint64_t tail		= 0x00ll;
	uint64_t payload[8]	= {0x00ll,0x00ll,0x00ll,0x00ll,
				   0x00ll,0x00ll,0x00ll,0x00ll};

	uint64_t packet[HMC_MAX_UQ_PACKET];
	uint8_t	cub		= 0;
	uint16_t tag		= 0;
	uint8_t link		= 0;
	int ret			= HMC_OK;
	int stall_sig		= 0;
	
	FILE *ofile = NULL;
	int *rtns = NULL;
	long all_sent		= 0;
	long all_recv		= 0;
	int done		= 0;

		uint64_t d_response_head;
		uint64_t d_response_tail;
		hmc_response_t d_type;
		uint8_t d_length;
		uint16_t d_tag;
		uint8_t d_rtn_tag;
		uint8_t d_src_link;
		uint8_t d_rrp;
		uint8_t d_frp;
		uint8_t d_seq;
		uint8_t d_dinv;
		uint8_t d_errstat;
		uint8_t d_rtc;
		uint32_t d_crc;

	
	/* 
	 * get the address shift amount based upon 
	 * the max block size, device size and link count
	 *
	 */
	if( getshiftamount( num_links, capacity, bsize, &shiftamt ) != 0 ){ 
		printf( "FAILED TO RETRIEVE SHIFT AMOUNT\n" );
		hmcsim_free( &hmc );
		return -1;
	}	

		/* 
	 * init the library 
	 * 
	 */

	ret = hmcsim_init(	&hmc, 
				num_devs, 
				num_links, 
				num_vaults, 
				queue_depth, 
				num_banks, 
				num_drams, 
				capacity, 
				xbar_depth );
	if( ret != 0 ){ 
		printf( "FAILED TO INIT HMCSIM : return code=%d\n", ret );	
		return -1;
	}else {
		printf( "SUCCESS : INITALIZED HMCSIM\n" );
	}


	/* 
	 * setup the device topology
	 * 
	 */
	if( num_devs > 1 ){ 

		/* -- TODO */			

	}else{ 
		/* 
	 	 * single device, connect everyone 
		 *
		 */
		for(int i=0; i<num_links; i++ ){ 
	
			ret = hmcsim_link_config( &hmc, 
						(num_devs+1), 
						0, 
						i, 
						i,
						HMC_LINK_HOST_DEV );
	
			if( ret != 0 ){ 
				printf( "FAILED TO INIT LINK %d\n", i );
				hmcsim_free( &hmc );
				return -1;
			}else{ 
				printf( "SUCCESS : INITIALIZED LINK %d\n", i );
			}
		}
	}

	/* 
	 * init the max block size 
	 * 
 	 */
	ret = hmcsim_util_set_all_max_blocksize( &hmc, bsize );
	
	if( ret != 0 ){ 
		printf( "FAILED TO INIT MAX BLOCK SIZE\n" );
		hmcsim_free( &hmc );
		return -1;
	}else {
		printf( "SUCCESS : INITALIZED MAX BLOCK SIZE\n" );
	}

		/* ---- */

	rtns = (int *) malloc( sizeof( int ) * hmc.num_links );
	memset( rtns, 0, sizeof( int ) * hmc.num_links );

	/* 
	 * Setup the tracing mechanisms
	 * 
	 */
	ofile = fopen( "hmctrace.out", "w" );	
	if( ofile == NULL ){ 
		printf( "FAILED : COULD NOT OPEN OUTPUT FILE\n" );
		return -1;
	}	
	
	hmcsim_trace_handle( &hmc, ofile );
	hmcsim_trace_level( &hmc, (HMC_TRACE_BANK|
				HMC_TRACE_QUEUE|
				HMC_TRACE_CMD|
				HMC_TRACE_STALL|
				HMC_TRACE_LATENCY) );

	printf( "SUCCESS : INITIALIZED TRACE HANDLERS\n" );				
	

	/* 
	 * zero the packet
	 * 
	 */
	zero_packet( &(packet[0]) );

	printf( "SUCCESS : ZERO'D PACKETS\n" );
	printf( "SUCCESS : BEGINNING TEST EXECUTION\n" );
#endif

    int totalinst = 0;

	uint64_t time = 0;

	int Qfinish = 0;
	uint64_t previousAddr = 0;
	bool traceStall = 0;
	//start fast simulation
    while(Qfinish != queueNumber){    
 
		time++; //cycles

#ifdef HMC
		hmcsim_clock( &hmc );

		//step 0: receive from HMC, remove from queue
		/* 
		 * reset the return code for receives
		 * 
		 */
		ret = HMC_OK;

		/* 
		 * We hit a stall or an error
		 * 
		 * Try to drain the responses off all the links
		 * 
		 */
		//printf( "...reading responses\n" );
		while( ret != HMC_STALL ){ 

			for( z=0; z<hmc.num_links; z++){ 
				
				rtns[z] = hmcsim_recv( &hmc, cub, z, &(packet[0]) );

				if( rtns[z] == HMC_STALL ){ 
					stall_sig++;
				}else{ 
					/* successfully received a packet */
					printf( "SUCCESS : RECEIVED A SUCCESSFUL PACKET RESPONSE\n" );	
                    hmcsim_decode_memresponse( &hmc,
                                                &(packet[0]), 
                                                &d_response_head,
                                                &d_response_tail,
                                                &d_type,
                                                &d_length,
                                                &d_tag,
                                                &d_rtn_tag,
                                                &d_src_link,
                                                &d_rrp,
                                                &d_frp,
                                                &d_seq,
                                                &d_dinv,
                                                &d_errstat,
                                                &d_rtc,
                                                &d_crc );
                    printf( "RECV tag=%d; rtn_tag=%d\n", d_tag, d_rtn_tag );
					all_recv++;
					//after receive update Queue
					int Qid = d_tag/(1024/queueNumber) ;
					//printf("QID:%d ", Qid);
					//int target = d_tag%Maxlength;
					//printf("line:%d ", target);
					//remove queue line from target queue 

					int target = -1;

					for (int i = 0; i < cq[Qid].length; i++){ 
						//printf("%lld vs %lld", cq[Qid].Qaddr[i], d_tag);
						if(cq[Qid].Qaddr[i] == d_tag) {target = i; break;}
					}

					if (target == -1) {
						printf("ERROR: cannot find return target\n");
						return -1;
					}else{

						totalQlatency[Qid] = time - cq[Qid].Qissue[target] + totalQlatency[Qid];
						cq[Qid].Qsent[target] = 0;
						//printf("Q %d: totalQlatency %d, time %d Qtail %d Qhead %d Q.issue %d Qaddr %08x\n", Qid,totalQlatency[Qid], time, cq[Qid].Qtail[target], cq[Qid].Qhead,cq[Qid].Qissue[target], cq[Qid].Qaddr[target]);
						for (int i = target; i < cq[Qid].length; i++){   // darin the target
							cq[Qid].QpageID[i] = cq[Qid].QpageID[i+1];
							cq[Qid].Qtail[i] = cq[Qid].Qtail[i+1];
							cq[Qid].Qkey[i] = cq[Qid].Qkey[i+1];
							cq[Qid].Qissue[i] = cq[Qid].Qissue[i+1];
							cq[Qid].Qaddr[i] = cq[Qid].Qaddr[i+1];
							cq[Qid].Qsent[i] = cq[Qid].Qsent[i+1];
						}

						//update queue status
					//	printf("update Q status, Q length %d",cq[Qid].length);
						cq[Qid].Qhead = time;   
						cq[Qid].length--;
						processedinstQ[Qid]++;
					}		
					
				}

				/* 
				 * zero the packet 
				 * 
				 */
				zero_packet( &(packet[0]) );
			}

			/* count the number of stall signals received */
			if( stall_sig == hmc.num_links ){ 
				/* 
				 * if all links returned stalls, 
				 * then we're done receiving packets
				 *
				 */
				
				//printf( "STALLED : STALLED IN RECEIVING\n" );
				ret = HMC_STALL;
			}

			stall_sig = 0;
			for( z=0; z<hmc.num_links; z++){
				rtns[z] = HMC_OK;
			}
		}
			ret = HMC_OK;
		//step 1: drain each queue (send to HMC)
		
		if ( ret != HMC_STALL ){

			for(int q = 0; q< queueNumber; q++){
		//state drain
				switch(queuePolicy){
				case FIFO: //queue key is the arrive time, since we do not model the network, the first one in the queue may not the first arrive
				{
					int target = 0; //queue position
					bool send = 0; //if drain this time
					int first = time;
					  //queue drain on cq[QID]
					//printf("preparing for sending: stall %d length %d", cq[q].stall, cq[q].length  );
					if ((cq[q].stall == 0)&&(cq[q].length != 0)){	
						for (int i = 0; i < cq[q].length; i++){   // search the first in the queue
							if ((first >= cq[q].Qkey[i])&&(cq[q].Qtail[i]+cq[q].processinglength <= time)&&(cq[q].Qsent[i] == 0)){
								first = cq[q].Qkey[i];
								target = i;
								send = 1;
								//printf("Q %d: time %d Qkey %d Qhead %d\n", q, time, cq[q].Qtail[target], cq[q].Qhead);
							}
						}

						//drain the queue
						if (send == 1){

							tag = q* 1024/queueNumber + cq[q].tag; // calculate the tag for queue space
						    uint64_t tempaddr = cq[q].Qaddr[target];
							cq[q].Qaddr[target] = tag;
							//generate the package
							int r_w = rand() % 99;
							if (r_w < read_perct){
									printf( "...building read request for device : %d\n", cub );
									hmcsim_build_memrequest( &hmc, 
												cub, 
												tempaddr,
												tag, 
												RD64, 
												link, 
												&(payload[0]), 
												&head, 
												&tail );
									/* 
									 * read packets have: 
									 * head + 
									 * tail
									 * 
									 */
									packet[0] = head;
									packet[1] = tail;
							}else{
										printf( "...building write request for device : %d\n", cub );
										hmcsim_build_memrequest( &hmc, 
													cub, 
													 tempaddr, 
													tag, 
													WR64, 
													link, 
													&(payload[0]), 
													&head, 
													&tail );
										/* 
										 * write packets have: 
										 * head + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * tail
										 * 
										 */
										packet[0] = head;
										packet[1] = 0x05ll;
										packet[2] = 0x06ll;
										packet[3] = 0x07ll;
										packet[4] = 0x08l;
										packet[5] = 0x09ll;
										packet[6] = 0x0All;
										packet[7] = 0x0Bll;
										packet[8] = 0x0Cll;
										packet[9] = tail;
							}

						    //sent 
							printf( "...sending packet %d from queue %d line %d: base addr=0x%016llx\n", tag, q, target,  tempaddr );

							ret = hmcsim_send( &hmc, &(packet[0]) );

							switch( ret ){ 
								case 0: 
									printf( "SUCCESS : PACKET WAS SUCCESSFULLY SENT\n" );
									cq[q].Qsent[target] = 1;
									all_sent++;
									break;
								case HMC_STALL:
									printf( "STALLED : PACKET WAS STALLED IN SENDING\n" );
									break;
								case -1:
								default:
									printf( "FAILED : PACKET SEND FAILED\n" );
									break;
							}

							/* 
							 * zero the packet 
							 * 
							 */
							zero_packet( &(packet[0]) );

							cq[q].tag++;
							if( cq[q].tag == 1024/queueNumber ){
								cq[q].tag = 0;
							}	

							link++;
							if( link == hmc.num_links ){
								/* -- TODO : look at the number of connected links
								 * to the host processor
								 */
								link = 0;
							}

						}
					
					}
					break;
				}

				case LIFO: //queue key is the arrive time
				{
					int target = 0;
					bool send = 0;
					int last = 0;
				  //queue drain on cq[QID]
					if ((cq[q].stall == 0)&&(cq[q].length != 0)){	
						for (int i = 0; i < cq[q].length; i++){   // search the last in the queue
							if ((last <= cq[q].Qkey[i])&&(cq[q].Qtail[i]+cq[q].processinglength <= time)&&(cq[q].Qsent[i] == 0)){
								last = cq[q].Qkey[i];
								target = i;
								send = 1;
							}
						}
						if (send == 1){
							tag = q* 1024/queueNumber + cq[q].tag; // calculate the tag for queue space
							uint64_t tempaddr = cq[q].Qaddr[target];
							cq[q].Qaddr[target] = tag;
							//generate the package
							int r_w = rand() % 99;
							if (r_w < read_perct){
									printf( "...building read request for device : %d\n", cub );
									hmcsim_build_memrequest( &hmc, 
												cub, 
												 tempaddr,
												tag, 
												RD64, 
												link, 
												&(payload[0]), 
												&head, 
												&tail );
									/* 
									 * read packets have: 
									 * head + 
									 * tail
									 * 
									 */
									packet[0] = head;
									packet[1] = tail;
							}else{
										printf( "...building write request for device : %d\n", cub );
										hmcsim_build_memrequest( &hmc, 
													cub, 
													 tempaddr, 
													tag, 
													WR64, 
													link, 
													&(payload[0]), 
													&head, 
													&tail );
										/* 
										 * write packets have: 
										 * head + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * tail
										 * 
										 */
										packet[0] = head;
										packet[1] = 0x05ll;
										packet[2] = 0x06ll;
										packet[3] = 0x07ll;
										packet[4] = 0x08l;
										packet[5] = 0x09ll;
										packet[6] = 0x0All;
										packet[7] = 0x0Bll;
										packet[8] = 0x0Cll;
										packet[9] = tail;
							}

						    //sent 
							printf( "...sending packet from queue %d line %d: base addr=0x%016llx\n", q, target,  tempaddr );

							ret = hmcsim_send( &hmc, &(packet[0]) );

							switch( ret ){ 
								case 0: 
									printf( "SUCCESS : PACKET WAS SUCCESSFULLY SENT\n" );
									cq[q].Qsent[target] = 1;
									break;
								case HMC_STALL:
									printf( "STALLED : PACKET WAS STALLED IN SENDING\n" );
									break;
								case -1:
								default:
									printf( "FAILED : PACKET SEND FAILED\n" );
									break;
							}

							/* 
							 * zero the packet 
							 * 
							 */
							zero_packet( &(packet[0]) );

							cq[q].tag++;
							if( cq[q].tag == 1024/queueNumber ){
								cq[q].tag = 0;
							}	


							link++;
							if( link == hmc.num_links ){
								/* -- TODO : look at the number of connected links
								 * to the host processor
								 */
								link = 0;
							}

						}	
					}
				   break;
				 }

				case PAGEHIT: //queue key is the MSHR hit rate
				{
					int target = 0;
					int hash = 0;
					bool sent = 0;
				  //queue drain on cq[QID]
					if ((cq[q].stall == 0)&&(cq[q].length != 0)){
						for (int i = 0; i < cq[q].length; i++){   // search the last in the queue
							if ((hash <= cq[q].Qkey[i])&&(cq[q].Qtail[i]+cq[q].processinglength <= time)&&(cq[q].Qsent[i] == 0)){
								hash = cq[q].Qkey[i];
								target = i;
								sent = 1;
							}
						}

						if (sent == 1){

							tag = q* 1024/queueNumber + cq[q].tag; // calculate the tag for queue space
							uint64_t tempaddr = cq[q].Qaddr[target];
							cq[q].Qaddr[target] = tag;
							//generate the package
							int r_w = rand() % 99;
							if (r_w < read_perct){
									printf( "...building read request for device : %d\n", cub );
									hmcsim_build_memrequest( &hmc, 
												cub, 
												 tempaddr,
												tag, 
												RD64, 
												link, 
												&(payload[0]), 
												&head, 
												&tail );
									/* 
									 * read packets have: 
									 * head + 
									 * tail
									 * 
									 */
									packet[0] = head;
									packet[1] = tail;
							}else{
										printf( "...building write request for device : %d\n", cub );
										hmcsim_build_memrequest( &hmc, 
													cub, 
													 tempaddr, 
													tag, 
													WR64, 
													link, 
													&(payload[0]), 
													&head, 
													&tail );
										/* 
										 * write packets have: 
										 * head + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * data + 
										 * tail
										 * 
										 */
										packet[0] = head;
										packet[1] = 0x05ll;
										packet[2] = 0x06ll;
										packet[3] = 0x07ll;
										packet[4] = 0x08l;
										packet[5] = 0x09ll;
										packet[6] = 0x0All;
										packet[7] = 0x0Bll;
										packet[8] = 0x0Cll;
										packet[9] = tail;
							}

						    //sent 
							printf( "...sending packet from queue %d line %d: base addr=0x%016llx\n", q, target,  tempaddr );

							ret = hmcsim_send( &hmc, &(packet[0]) );

							switch( ret ){ 
								case 0: 
									printf( "SUCCESS : PACKET WAS SUCCESSFULLY SENT\n" );
									cq[q].Qsent[target] = 1;
									break;
								case HMC_STALL:
									printf( "STALLED : PACKET WAS STALLED IN SENDING\n" );
									break;
								case -1:
								default:
									printf( "FAILED : PACKET SEND FAILED\n" );
									break;
							}

							/* 
							 * zero the packet 
							 * 
							 */
							zero_packet( &(packet[0]) );

							cq[q].tag++;
							if( cq[q].tag == 1024/queueNumber ){
								cq[q].tag = 0;
							}	


							link++;
							if( link == hmc.num_links ){
								/* -- TODO : look at the number of connected links
								 * to the host processor
								 */
								link = 0;
							}
						}
					}		
				   break;
				  }
			  }
			//update current queue status
			totalQlength[q] += cq[q].length;
			if (cq[q].length > maxQlength[q]) maxQlength[q] = cq[q].length;
			}
		}

#endif

#ifndef HMC

		//step 1: drain each queue
		for(int q = 0; q< queueNumber; q++){

		//state drain
			switch(queuePolicy){
			case FIFO: //queue key is the arrive time, since we do not model the network, the first one in the queue may not the first arrive
			{
				int target = 0; //queue position
				bool drain = 0; //if drain this time
				int first = cq[q].Qkey[0];
				  //queue drain on cq[QID]
				if ((((int)time) > (cq[q].Qhead+cq[q].processinglength))&&(cq[q].length != 0)){	
					for (int i = 0; i < cq[q].length; i++){   // search the first in the queue
						if ((first >= cq[q].Qkey[i])&&(cq[q].Qtail[i]+cq[q].processinglength <= ((int)time)) ){
							first = cq[q].Qkey[i];
							target = i;
							drain = 1;
							//printf("Q %d: time %d Qkey %d Qhead %d\n", q,totalQlatency[q], time, cq[q].Qtail[target], cq[q].Qhead);
						}
					}

					//drain the queue
					if (drain == 1){

						totalQlatency[q] = time - cq[q].Qissue[target] + totalQlatency[q];

						//printf("Q %d: totalQlatency %d, time %d Qtail %d Qhead %d Q.issue%d\n", q,totalQlatency[q], time, cq[q].Qtail[target], cq[q].Qhead,cq[q].Qissue[target]);

						for (int i = target; i < cq[q].length; i++){   // darin the target
							cq[q].QpageID[i] = cq[q].QpageID[i+1];
							cq[q].Qtail[i] = cq[q].Qtail[i+1];
							cq[q].Qkey[i] = cq[q].Qkey[i+1];
							cq[q].Qissue[i] = cq[q].Qissue[i+1];
						}

						//update queue status
						cq[q].Qhead =  time;   
						cq[q].length--;
						processedinstQ[q]++;
					}
					
				}
				break;
			}

			case LIFO: //queue key is the arrive time
			{
				int target = 0;
				bool drain = 0;
				int last = cq[q].Qkey[0];
			  //queue drain on cq[QID]
				if ((((int)time) > (cq[q].Qhead+cq[q].processinglength))&&(cq[q].length != 0)){	
					for (int i = 0; i < cq[q].length; i++){   // search the last in the queue
						if ((last <= cq[q].Qkey[i])&&(cq[q].Qtail[i]+cq[q].processinglength <= ((int)time))){
							last = cq[q].Qkey[i];
							target = i;
							drain = 1;
						}
					}
					if (drain == 1){

						totalQlatency[q] = time-cq[q].Qissue[target] + totalQlatency[q];

					//	printf("Q %d: totalQlatency %d, time %d Qtail %d Qhead %d Q.issue%d\n", q,totalQlatency[q], time, cq[q].Qtail[target], cq[q].Qhead,cq[q].Qissue[target]);

						
						for (int i = target; i < cq[q].length; i++){   // darin the target
							cq[q].QpageID[i] = cq[q].QpageID[i+1];
							cq[q].Qtail[i] = cq[q].Qtail[i+1];
							cq[q].Qkey[i] = cq[q].Qkey[i+1];
							cq[q].Qissue[i] = cq[q].Qissue[i+1];
						}
						cq[q].Qhead =  time;    // start processsing next
						cq[q].length--;
						processedinstQ[q]++;
					}	
				}
			   break;
			 }

			case PAGEHIT: //queue key is the MSHR hit rate
			{
				int target = 0;
				int hash = cq[q].Qkey[0];
				bool drain = 0;
			  //queue drain on cq[QID]
				if ((((int)time) > (cq[q].Qhead+cq[q].processinglength))&&(cq[q].length != 0)){
					for (int i = 0; i < cq[q].length; i++){   // search the last in the queue
						if ((hash <= cq[q].Qkey[i])&&(cq[q].Qtail[i]+cq[q].processinglength <= ((int)time))){
							hash = cq[q].Qkey[i];
							target = i;
							drain = 1;
						}
					}

					if (drain == 1){
						totalQlatency[q] = time-cq[q].Qissue[target] + totalQlatency[q];

						//printf("Q %d: totalQlatency %d, time %d Qtail %d Qhead %d\n", q,totalQlatency[q], time, cq[q].Qtail[target], cq[q].Qhead);

						for (int i = target; i < cq[q].length; i++){   // darin the target
							cq[q].QpageID[i] = cq[q].QpageID[i+1];
							cq[q].Qtail[i] = cq[q].Qtail[i+1];
							cq[q].Qkey[i] = cq[q].Qkey[i+1];
							cq[q].Qissue[i] = cq[q].Qissue[i+1];
						}
						cq[q].Qhead =  time;  // start processsing next
						cq[q].length--;
						processedinstQ[q]++;
					}
				}		
			   break;
			  }
		  }
			//update current queue status
			totalQlength[q] += cq[q].length;
			if (cq[q].length > maxQlength[q]) maxQlength[q] = cq[q].length;
		}
#endif

		//step2: insert queue if there is new memory access, one for each cycle
		//check stall
		traceStall = 0;
		for(int q = 0; q< queueNumber; q++){
			if(cq[q].length == Maxlength-1) {
				traceStall=1;
			}
		}

		if((!fin.eof())&&(traceStall==0)){

			getline(fin, line);
			std::stringstream s(line);
			
			getline(s,ip2,' ');		//get inst_id
			if(ip2.length() > 0){
   				getline(s,addr,' ');	//get addr
				if (stoull(addr,0,16) > addrThreshold) previousAddr = stoull(addr,0,16); //addr check
				//else previousAddr = previousAddr + stol(addr,0,16);
				else continue;
				if((previousAddr>=memarea.min)&&(previousAddr<=memarea.max)){


					//getline(s,core,' ');	//get core_id
					getline(s,inittime,' ');//get global time (not used now)
                        
					totalinst++;
					uint64_t instAddr =  previousAddr;
					// find page
					int pageID = 0 ;  // get page ID
					int cacheline = 0;
					int LLCline = 0;

					/*
					for(int i = 0; i < memarea.blockCount; i++){
						if(( instAddr >= (memarea.min+i*memarea.blockSize)) && ( instAddr < (memarea.min+(i+1)*memarea.blockSize))){ 
							pageID = i; 
							cacheline = (int)((instAddr - (memarea.min+i*memarea.blockSize))>>8); //8
							LLCline = (int)((instAddr - (memarea.min+i*memarea.blockSize))>>16); //16
							//printf("PageID %d, value 0x%016llx cacheline 0x%016llx\n",pageID, (instAddr - (memarea.min+i*memarea.blockSize)), cacheline);
							break;
						}
					}	
					*/
					
					pageID = floor((instAddr-memarea.min)/(memarea.blockSize));
                    if(pageID == ((int)memarea.blockCount)) pageID--;
					cacheline = floor((instAddr-(memarea.min+pageID*memarea.blockSize))/(memarea.blockSize/(l1CacheOffset)));
					LLCline = floor((instAddr-(memarea.min+pageID*memarea.blockSize))/(memarea.blockSize/(LLCacheOffset)));
					//printf("PageID %d, value 0x%016llx cacheline %d LLCline %d\n",pageID, (instAddr - (memarea.min+pageID*memarea.blockSize)), cacheline, LLCline);
					
					// find queue 
					int QID = 0;   //get queue ID

					/*
					for(int i = 0; i < queueNumber; i++){
						if(( instAddr >= cq[i].memarea.min) && ( instAddr < cq[i].memarea.max)){ 
							QID = i; 
							break;
						}
					}
					*/

					QID = (instAddr-memarea.min)/(widesize*memarea.blockSize);
					//printf("%d", QID);
				
					totalCAccess[stoi(core)][QID]++;    //memory accesss from core
				

					//step 2.1: cache operation
					//core L1cache 
					int cacheID = stoi(core);
					bool miss = true;
					//totalAccess_C[cacheID][pageID]++;
					time_C[cacheID]++;
					// cache activity
					
					 //RUD model, compare RUD with wide
					/*
					if (totalAccess_C[cacheID][pageID] != 1){
						distance_C[cacheID][pageID] = time_C[cacheID] - lastAccess_C[cacheID][pageID];	
						if (distance_C[cacheID][pageID] <= cache[cacheID].wide) miss = false;
					}
					else{
						miss = true;
					}
					lastAccess_C[cacheID][pageID] = time_C[cacheID];
					*/

					//physical model, insert cache addr to cache model
					//check miss or hit
					miss = cacheAccess(&cache[cacheID], (pageID*(l1CacheOffset+1)+cacheline), totalinst, time);

					/*
					int hitID = 0;
					for(int i = 0; i< cache[cacheID].length; i++){
						if( pageID == cache[cacheID].cacheline[i]) {
							miss = false;
							hitID = i;
						}
					}

					if (miss == false){ //Hit, update the key value
						switch(cache[cacheID].policy){
							case FIFO:{
								cache[cacheID].cachekey[hitID] = time; //most recent one
							}
							case LIFO:{
							}
							case PAGEHIT:{
								cache[cacheID].cachekey[hitID] ++; //new
							}
						}
					}
					else if (miss == true){ //Miss, resert a cacheline and send to queue
						//cacheMiss[cacheID]++;
						if(cache[cacheID].length==cache[cacheID].wide){ //check if cache full, replace one inside the cache
							int target = 0;
							switch(cache[cacheID].policy){ //find target
								case FIFO:{      //LRU
									int oldest = time;
									for (int i = 0; i < cache[cacheID].length; i++){   // search the oldest in the cache
										if (cache[cacheID].cachekey[i]<oldest){
											oldest = cache[cacheID].cachekey[i];
											target = i;
										}
									}
									cache[cacheID].cacheline[target] = pageID;
									cache[cacheID].cachekey[target] = time; //most recent one
								}
								case LIFO:{
								}
								case PAGEHIT:{
									int hitcount = totalinst;
									for (int i = 0; i < cache[cacheID].length; i++){   // search the oldest in the cache
										if (cache[cacheID].cachekey[i]<hitcount){
											hitcount = cache[cacheID].cachekey[i];
											target = i;
										}
									}
									cache[cacheID].cacheline[target] = pageID;
									cache[cacheID].cachekey[target] = 0; //new
								}
							}
						}
						else{ //cache not full, insert to the end
							cache[cacheID].cacheline[cache[cacheID].length] = pageID;
							switch(cache[cacheID].policy){
								case FIFO:{
									cache[cacheID].cachekey[cache[cacheID].length] = time; //most recent one
								}
								case LIFO:{
								}
								case PAGEHIT:{
									cache[cacheID].cachekey[cache[cacheID].length] = 0; //new
								}
							}
							cache[cacheID].length++;
						}
					}
				 */
					//step 2.2: queue insert operation
				
					if (miss == false){ //hit, update latency and memory status
						totalinstQ[QID]++;
						processedinstQ[QID]++;
						//totalQlatency[QID] = totalQlatency[QID] + cache[cacheID].latency;

					}
					else if (miss == true){ //miss, insert to LLCcache
						cacheMiss[cacheID]++;
						time_LLC[QID]++;
						bool LLCmiss = true;

					//	LLCmiss = cacheAccess(&LLCache[QID], (pageID*64+LLCline), totalinst, time);
						LLCmiss = cacheAccess(&LLCache[QID], (pageID*(LLCacheOffset+1)+LLCline), totalinst, time);


						if (LLCmiss == false){ //hit, update latency and memory status
							totalinstQ[QID]++;
							processedinstQ[QID]++;
							//totalQlatency[QID] = totalQlatency[QID] + cache[cacheID].latency+LLCache[QID].latency;
						}
						else if (LLCmiss == true){ //miss, insert to queue
							LLCMiss[QID]++;
							totalinstQ[QID]++;

							switch(queuePolicy){
								case FIFO:{
								//queue insert on cq[QID]
									if (cq[QID].length < Maxlength-1){	
										bool check = 0; // check if the page is in the queue, model MSHR
										for(int i = 0; i< cq[QID].length; i++){
											if (pageID == cq[QID].QpageID[i]) check = 1;   //if the page still in the queue, merge
										}
										if (check == 0) { // new request insert to queue end
											cq[QID].QpageID[cq[QID].length] = pageID;
											cq[QID].Qtail[cq[QID].length] = time;   //record the arrive time
											cq[QID].Qissue[cq[QID].length] = time;  //record the issue time
											if (cq[QID].length == 0) cq[QID].Qhead =  time; //if the first in the queue
											if (floor(QID/coreNumber) != stoi(core)) cq[QID].Qtail[cq[QID].length] = cq[QID].Qtail[cq[QID].length] + cq[QID].transmitlength;  //since I do not model network, add latency from remote memory
											cq[QID].Qkey[cq[QID].length] = cq[QID].Qtail[cq[QID].length];   // in FIFO the key is arraving time
											cq[QID].Qaddr[cq[QID].length] = instAddr; // record the addr
										//	printf("Q %d: length %d Qtail %d Qhead %d\n", QID, cq[QID].length, cq[QID].Qtail[cq[QID].length], cq[QID].Qhead);
											cq[QID].length++;	
										}else{   // merge the request do noting for fifo
											processedinstQ[QID]++;
											totalQlatency[QID] = totalQlatency[QID] + fastMemory;
										}
									}
									else {
										cq[QID].stall++;
										//traceStall=1;
										}  //exit the max length
									break;
								}
								case LIFO:
								{
								//queue insert on cq[QID]
									if (cq[QID].length < Maxlength-1){
										bool check = 0; // check if the page is in the queue
										for(int i = 0; i< cq[QID].length; i++){
											if (pageID == cq[QID].QpageID[i]) check = 1;
										}
										if (check == 0) { // new request insert to queue end
											cq[QID].QpageID[cq[QID].length] = pageID;
											cq[QID].Qtail[cq[QID].length] = time;
											cq[QID].Qissue[cq[QID].length] = time;
											if (floor(QID/coreNumber) != stoi(core)) cq[QID].Qtail[cq[QID].length] = cq[QID].Qtail[cq[QID].length] + cq[QID].transmitlength;  // add latency from remote memory
											cq[QID].Qkey[cq[QID].length] = cq[QID].Qtail[cq[QID].length];   // in FIFO the key is avaing time		
											cq[QID].Qaddr[cq[QID].length] = instAddr;
											//printf("Q %d: length %d Qtail %d Qhead %d\n", QID, cq[QID].length, cq[QID].Qtail[cq[QID].length], cq[QID].Qhead);
											if (cq[QID].length == 0) cq[QID].Qhead =  time;
											cq[QID].length++;	
										}else{   // merge the request
											processedinstQ[QID]++;
											totalQlatency[QID] = totalQlatency[QID] + fastMemory;
										}
									}
									else {
										cq[QID].stall++;
										//traceStall=1;
										}
									break;
								}
								case PAGEHIT:
								{
								//queue insert on cq[QID]
									int merge = 0;
									if (cq[QID].length < Maxlength-1){
										bool check = 0; // check if the page is in the queue
										for(int i = 0; i< cq[QID].length; i++){
											if (pageID == cq[QID].QpageID[i]) {
												check = 1;
												merge = i;
											}
										}
										if (check == 0) { // new request insert to queue end
											cq[QID].QpageID[cq[QID].length] = pageID;
											cq[QID].Qkey[cq[QID].length] = 1;
											cq[QID].Qtail[cq[QID].length] = time;
											cq[QID].Qissue[cq[QID].length] = time;
											if (floor(QID/coreNumber) != stoi(core)) cq[QID].Qtail[cq[QID].length] = cq[QID].Qtail[cq[QID].length] + cq[QID].transmitlength; 													
											//printf("Q %d: length %d Qtail %d Qhead %d\n", QID, cq[QID].length, cq[QID].Qtail[cq[QID].length], cq[QID].Qhead);
											if (cq[QID].length == 0) cq[QID].Qhead =  time;
											cq[QID].Qaddr[cq[QID].length] = instAddr;
											cq[QID].length++;		
										}else{   // merge the request
											cq[QID].Qkey[merge]++;
											processedinstQ[QID]++;
											totalQlatency[QID] = totalQlatency[QID] + fastMemory;
										}
									}
										else {
											//traceStall=1;
											cq[QID].stall++;
										}
									break;
								}
							}
						}
					}
				} 
			}// insert end
		}


		//step 3: check all memory acesses have been proccessed
		Qfinish = 0;
		for(int q = 0; q< queueNumber; q++){
		//	printf("Q %d total inst %d processed %d\n", q ,totalinstQ[q],processedinstQ[q]);
			if((fin.eof())&&(processedinstQ[q] == totalinstQ[q])){
			//if((fin.eof())&&(all_recv == all_sent)){
				//printf("Q %d finished %d",q, Qfinish);
				Qfinish++;
			}
		}
	}
    fin.close();

	//print moduling paramters
	printf("=============queue moduling ending at %ld cycle =========== \n", time);

	for(int i = 0; i<coreNumber; i++){
		if(time_C[i]!= 0){
			printf("core ID %d, total access is %d\n", i, time_C[i]);
			printf("L1cache miss is %d, miss rate is %f\n", cacheMiss[i], ((double)cacheMiss[i])/((double)time_C[i]));
		}
	}

	for(int i = 0; i<queueNumber; i++){
		if(time_LLC[i]!= 0){
			printf("\nLLC ID %d, total access is %d\n", i, time_LLC[i]);
			printf("LLcache miss is %d, miss rate is %f\n", LLCMiss[i], ((double)LLCMiss[i])/((double)time_LLC[i]));
		}
	}
	
	for(int i = 0; i<queueNumber; i++){
		printf("\nqueue ID %d, wide %08lx-%08lx, total access is %d\n", i, cq[i].memarea.min, cq[i].memarea.max,  LLCMiss[i]);
		if (totalinstQ[i] > 0 ){
			printf("average queue size is %ld\n", totalQlength[i]/time);
			printf("max queue length is %d\n", maxQlength[i]);
			//printf("average memory access latency is %lld\n", (totalQlatency[i])/(totalinstQ[i]));
			printf("average memory access latency is %ld\n", (totalQlatency[i])/(LLCMiss[i]));
		}
	//	printf("total queue length %d, latency%d, time%d\n", totalQlength[i], totalQlatency[i], time);
		printf("\n");
	}

return 0;
}

#endif
