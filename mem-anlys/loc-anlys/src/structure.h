#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <stdio.h>
#include <bits/stdc++.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <list>
#include <vector>

using namespace std;
using std::cerr;
using std::dec;
using std::endl;
using std::hex;
using std::ofstream;
using std::setw;
using std::string;

uint64_t addrThreshold = stoull("FFFFFFFF",0,16);   //Threshold for checking if the addr is a offsit of PTR write
uint64_t pageSize; 

int phyPage;
uint32_t mempin;

//Define the structure of memory pages
struct MemArea {
	uint32_t blockCount;
	uint64_t blockSize;
	uint64_t max;
	uint64_t min;
};

//Define the structure of memory input 
//Used to find target instruction. Didn't use now
struct MemIns {
   string ip;
   MemArea memarea;
};

//Define the structure of memory input 
//Used to find target instruction. Didn't use now
struct Memblock{
  int level;
  string strID;
  string strParentID;
  int leftPid;
  int rightPid;
  double RUD;
  int access;
  uint64_t max;
  uint64_t min;
	uint32_t blockCount;
	uint64_t blockSize;
};

//Load target instrution IP from filename. Didn't use now
//Data stores as <IP\n>
int searchAddr(string* addrDetail, string filename) 
{ 
  
    // File pointer 
    fstream fin; 
  
    // Open an existing file 
    fin.open(filename, ios::in); 

    // read the state line
    
    string line,addr2, core;
	int linenumber = 0;
        

    while (!fin.eof()){ 
  
    getline(fin, line);
    std::stringstream s(line);
    getline(s,addr2,' '); 
	
	(*addrDetail).append("0x");
    (*addrDetail).append(addr2);
	(*addrDetail).append(";");
	//cout<< " read address form file" << addr2 << filename<< "\n";
	linenumber++;

	}

	cout<<"target instruction: "<< *addrDetail<<"\n";
    fin.close();    
    
	return linenumber;
}

//Use to check the max, min memory area for the memory trace and how many cores to determine page size
//Data stored as <IP addr core initialtime\n>
//Todo: Distributed memory space check
uint64_t areacheck(string filename, uint64_t * max, uint64_t * min, int *coreNumber){

	 fstream fin; 
   uint64_t totalAccess=0;
    // Open an existing file 
    fin.open(filename, ios::in); 
  if (fin.fail()) {
    cout << " File open failed on "<< filename<< endl;
    return 0;
  }

    // read the state line
    string line,ip2,addr, core;
	uint64_t previousAddr =0;  //used for offsit check

    while (!fin.eof()){ 
  
    getline(fin, line);
    std::stringstream s(line);
    if(getline(s,ip2,' ')){ 
//		if(ip.compare(ip2) == 0){
	   // printf("find the instruction from input file\n");
   			getline(s,addr,' ');
     // Remove for miniVite trace - no need to check, the trace is final 
			if (stoull(addr,0,16) > addrThreshold) 
			  previousAddr = stoull(addr,0,16);//check if the addr is offsit
			//else previousAddr = previousAddr + stoull(addr,0,16);
			else continue;
			//getline(s,core,' ');
			//if ((stoi)(core) > (*coreNumber)) (*coreNumber) = (stoi)(core); //check core ID

			if (previousAddr > (*max)) (*max) = previousAddr; //check max
			if (previousAddr < (*min)) (*min) = previousAddr; //check min
      totalAccess++;
			}
		
	}
    fin.close();

		(*coreNumber)++;
		
		printf("min address - %08lx  ", (*min));
		printf("max address - %08lx  ", (*max));
                printf("total core - %d\n", (*coreNumber));
	
return totalAccess;	
}

#endif
