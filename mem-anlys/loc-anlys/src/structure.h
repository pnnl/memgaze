#ifndef STRUCTURE_H_
#define STRUCTURE_H_

#include <stdio.h>
#include <bits/stdc++.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <list>
#include <vector>
#include <map>
#include <boost/icl/interval_set.hpp>

using namespace std;

//Define the structure of memory pages
struct MemArea {
	uint32_t blockCount;
	uint64_t blockSize;
	uint64_t max;
	uint64_t min;
};


//Define the structure of memory block 
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

struct TopAccessLine {
  uint8_t regionId;
  uint32_t pageId;
  uint32_t lineId;
  uint64_t lowAddr; // low addr of cache-line (64B)
  uint32_t accessCount;
};

// Unused - carries over from original code
// Define the structure of memory input 
struct MemIns {
   string ip;
   MemArea memarea;
};

// Unused functions carried over from original code
int searchAddr(string* addrDetail, string filename);
uint64_t areacheck(string filename, uint64_t * addr_max, uint64_t * addr_min, int *coreNumber);

#endif
