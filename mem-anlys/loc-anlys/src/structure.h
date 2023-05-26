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

using namespace std;

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

int searchAddr(string* addrDetail, string filename);
uint64_t areacheck(string filename, uint64_t * addr_max, uint64_t * addr_min, int *coreNumber);

#endif
