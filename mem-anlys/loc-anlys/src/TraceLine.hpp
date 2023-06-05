// -*-Mode: C++;-*-
//
//*BeginPNNLCopyright********************************************************
//
// $HeadURL$
// $Id:
//
//**********************************************************EndPNNLCopyright*

//***************************************************************************
// $HeadURL$
//
// Yasodha Suriyakumar 
//***************************************************************************

//***************************************************************************
#ifndef TRACELINE_H
#define TRACELINE_H

#include <fstream>
#include <regex>
#include <unordered_set>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <string>
#include <iterator>
#include <fstream>
#include <sstream>
#include <algorithm>
using namespace std;

//This class holds the trace information
//Data stored as <IP addr core initialtime\n> 
class TraceLine {
  public:
    uint64_t insPtrAddr;
    uint64_t loadAddr;
    uint8_t coreNum;
    uint32_t sampleId;
    uint32_t instTime;
    uint8_t regionId;

  TraceLine(uint64_t _insPtrAddr, uint64_t _loadAddr, uint8_t _coreNum, uint32_t _instTime, uint32_t _sampleId)
  {
    insPtrAddr=_insPtrAddr;
    loadAddr=_loadAddr;
    coreNum=_coreNum;
    instTime=_instTime;
    sampleId=_sampleId;
  }
  
  void setAll(uint64_t _insPtrAddr, uint64_t _loadAddr, uint8_t _coreNum, uint32_t _instTime, uint32_t _sampleId)
  {
    insPtrAddr=_insPtrAddr;
    loadAddr=_loadAddr;
    coreNum=_coreNum;
    instTime=_instTime;
    sampleId=_sampleId;
  }
  
  void setRegionId(uint8_t _regionId)  {
    regionId = _regionId;
  }

  uint64_t getInsPtAddr() { return insPtrAddr;}
  uint64_t getLoadAddr() { return  loadAddr;}
  uint8_t getCoreNum() { return  coreNum;}
  uint32_t getInstTime() { return  instTime;}
  uint32_t getSampleId() { return  sampleId;}
  uint32_t getRegionId() { return  regionId;}

  void printTraceLine(){
    printf("ip %08lx addr %08lx core %d insttime %d sampleId %d \n", insPtrAddr, loadAddr, coreNum, instTime, sampleId); 
  }
  void printTraceRegion(){
    printf("ip %08lx addr %08lx insttime %d sampleId %d region %d \n", insPtrAddr, loadAddr,  instTime, sampleId, regionId); 
  }

};
#endif
