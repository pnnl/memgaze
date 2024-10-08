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
#ifndef BLOCKINFO_H
#define BLOCKINFO_H

#include <fstream>
#include <regex>
#include <unordered_set>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <iterator>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "SpatialRUD.hpp"

using namespace std;

//This class holds the Block's results - information on RUD, access counts, spatial distance
class BlockInfo {
  public:
    std::pair<unsigned int, unsigned int> blockID;
    string strBlockId;
    uint64_t addrMin;
    uint64_t addrMax;
    unsigned int numBlocksInLevel;
    uint32_t totalAccess=0;
    int totalRUD=-1;
    double avgRUD=-1.0;
    double sampleAvgRUD=-1.0;
    uint32_t lifetime=0;
    vector <pair<uint32_t, class SpatialRUD*>> vecSpatialResult;
    uint32_t *totalAccessAllBlocks;
    uint16_t *spatialNext;
    uint16_t *spatialAccess;
    uint16_t *spatialMiddle;
    uint16_t *totalSpatialDist; 

  BlockInfo(pair< unsigned int, unsigned int> _blockID,  uint64_t _addrMin, uint64_t _addrMax, unsigned int _numBlocksInLevel, int spatialResult, string _strBlockId);
  ~BlockInfo();
  void setAccess(uint32_t _totalAccess); 
  void setAccessRUD(uint32_t _totalAccess, int _totalRUD, uint32_t lifetime, double _sampleAvgRUD);
  void setSpatialRUD(uint32_t blkID, SpatialRUD* curSpatialRUD);

  void setTotalAccessAllBlocks( uint32_t  * _totalAccessAllBlocks);
  void setSpatialNext( uint16_t  * _spatialNext);
  void setSpatialAccess( uint16_t  * _spatialAccess);
  void setSpatialMiddle( uint16_t * _spatialMiddle);
  void setTotalSpatialDist( uint16_t * _totalSpatialDist);
  
  uint32_t getTotalAccess(); 
  uint32_t getLifetime(); 
  int getTotalRUD(); 
  double getAvgRUD(); 
  double getSampleAvgRUD(); 
  int getMinRUD(); 
  int getMaxRUD(); 

  void printBlockInfo();
  void printBlockSpatialDensity(std::ofstream& outFile, uint64_t blockWidth, bool flagLastLevel);
  void printBlockSpatialProb(std::ofstream& outFile, uint64_t blockWidth, bool flagLastLevel);
  void printBlockSpatialInterval(std::ofstream& outFile, uint64_t blockWidth, bool flagLastLevel);
  void printBlockSpatialNext(std::ofstream& outFile, uint64_t blockWidth, bool flagLastLevel);
  void printBlockRUD();
  void printBlockAccess();

};
#endif
