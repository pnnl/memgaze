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

#include "BlockInfo.hpp"
#include <stdio.h>
using namespace std;

//This class holds the Block's results - information on RUD, access counts, spatial distance
  BlockInfo::BlockInfo(pair<unsigned int, unsigned int> _blockID, uint64_t _addrMin, uint64_t _addrMax, unsigned int _numBlocksInLevel, int spatialResult, string _strBlockId)
  {
    blockID.first = _blockID.first;
    blockID.second = _blockID.second;
    addrMin = _addrMin;
    addrMax = _addrMax;
    numBlocksInLevel = _numBlocksInLevel;
    strBlockId = _strBlockId;
    totalAccess=0;
    totalRUD=-1;
    avgRUD=-1.0;
    sampleAvgRUD=-1.0;
    minRUD=-1;
    maxRUD=-1;
    lifetime=0;
    if(spatialResult == 1) {
    //vector <pair<uint32_t, class SpatialRUD*>> vecSpatialResult;
    /*
    spatialNext = new  uint16_t[numBlocksInLevel];
    totalAccessAllBlocks = new  uint32_t[numBlocksInLevel];
    spatialMiddle = new uint16_t[numBlocksInLevel];
    totalSpatialDist = new  uint16_t[numBlocksInLevel];
    spatialAccess = new  uint16_t[numBlocksInLevel];
    minSpatialDist = new  int[numBlocksInLevel];
    maxSpatialDist = new  int[numBlocksInLevel];
    */
    }
  }
  
  BlockInfo::~BlockInfo()
  {
 /*     delete spatialNext;
      delete totalAccessAllBlocks;
      delete spatialMiddle; 
      delete totalSpatialDist; 
      delete spatialAccess; 
      delete minSpatialDist; 
      delete maxSpatialDist; 
  */

  }
  
  void BlockInfo::setAccessRUD(uint32_t _totalAccess, int _totalRUD, int _minRUD, int _maxRUD, uint32_t _lifetime, double _sampleAvgRUD)
  {
    totalAccess = _totalAccess;
    totalRUD = _totalRUD;
    if(totalAccess > 1)
      avgRUD = (double)totalRUD/(double)(totalAccess-1);
    minRUD = _minRUD;
    maxRUD = _maxRUD;
    lifetime = _lifetime;
    sampleAvgRUD = _sampleAvgRUD;
  }
  //vector vecSpatialResult<pair<uint32_t, class SpatialRUD*>>;
  void BlockInfo::setSpatialRUD(uint32_t blkID, SpatialRUD* curSpatialRUD)
  {
     vecSpatialResult.push_back(make_pair(blkID,curSpatialRUD));
  }  
    //vector vecSpatialResult<pair<uint32_t, class SpatialRUD*>>;
  void BlockInfo::setTotalAccessAllBlocks( uint32_t * _totalAccessAllBlocks)
  {
    for (unsigned int i=0; i<numBlocksInLevel; i++)
      *(totalAccessAllBlocks+i) = *(_totalAccessAllBlocks+i);
  }
  void BlockInfo::setSpatialNext( uint16_t * _spatialNext)
  {
    for (unsigned int i=0; i<numBlocksInLevel; i++)
      *(spatialNext+i) = *(_spatialNext+i);
  }
  void BlockInfo::setSpatialAccess( uint16_t * _spatialAccess)
  {
    for (unsigned int i=0; i<numBlocksInLevel; i++)
      *(spatialAccess+i) = *(_spatialAccess+i);
  }
  void BlockInfo::setSpatialMiddle( uint16_t * _spatialMiddle)
  {
    for (unsigned int i=0; i<numBlocksInLevel; i++)
      *(spatialMiddle+i) = *(_spatialMiddle+i);
  }
  void BlockInfo::setTotalSpatialDist( uint16_t * _totalSpatialDist)
  {
    for (unsigned int i=0; i<numBlocksInLevel; i++)
      *(totalSpatialDist+i) = *(_totalSpatialDist+i);
  }
  void BlockInfo::setMinSpatialDist(int * _minSpatialDist)
  {
    for (unsigned int i=0; i<numBlocksInLevel; i++)
      *(minSpatialDist+i) = *(_minSpatialDist+i);
  }
  void BlockInfo::setMaxSpatialDist(int * _maxSpatialDist)
  {
    for (unsigned int i=0; i<numBlocksInLevel; i++)
      *(maxSpatialDist+i) = *(_maxSpatialDist+i);
  }
  
  uint32_t BlockInfo::getTotalAccess() { return totalAccess;}
  int BlockInfo::getTotalRUD() { return totalRUD;}
  double BlockInfo::getAvgRUD() { return avgRUD;}
  double BlockInfo::getSampleAvgRUD() { return sampleAvgRUD;}
  uint32_t BlockInfo::getLifetime() { return lifetime;}
  int BlockInfo::getMinRUD() { return minRUD;}
  int BlockInfo::getMaxRUD() { return maxRUD;}

  void BlockInfo::printBlockRUD(){
    if(totalAccess != 0){
      printf("Page %d: area %08lx-%08lx, total access %d ", blockID.second, addrMin, addrMax, totalAccess);
      if (totalAccess != 1 ){
        //printf("min RUD - %d ,", minRUD);
        //printf("max RUD - %d ,", maxRUD);
        //printf("average RUD - %f, ", avgRUD);
        printf("smpl-avg RUD %f", sampleAvgRUD);
      } 
      printf("\n");
    }
  }

  void BlockInfo::printBlockSpatialDensity(std::ofstream& outFile, uint64_t blockWidth, bool flagLastLevel) {
    unsigned int j=0;
    vector <pair<uint32_t, class SpatialRUD*>>::iterator itrSpRUD;
    if(totalAccess != 0 && lifetime !=0){
      printf("*** Page %d: area %08lx-%08lx  Lifetime %d Access %d \n", blockID.second, addrMin, addrMax, lifetime, totalAccess);
      outFile << "*** ID " << strBlockId<< " Page "<< std::dec << blockID.second << " : area "<<hex<<addrMin<<"-"<<addrMax<< std::dec <<" Lifetime "<< lifetime 
              <<" Access " << totalAccess << " Block_count " << std::dec<< ((addrMax-addrMin)+1)/blockWidth << " Spatial_Density " ;

      vector<pair<double, uint32_t>> vecDensity;
      vecDensity.clear();
      uint32_t vecIndex=0;
      // debug START 
      for(vecIndex = 0; vecIndex < vecSpatialResult.size(); vecIndex++) {
        //outFile << std::fixed << std::setprecision(2)  << vecSpatialResult[vecIndex].first <<","<<(vecSpatialResult[vecIndex].second)->smplAvgSpatialMiddle<<" ";  
        cout << std::fixed << std::setprecision(2)  << vecSpatialResult[vecIndex].first <<","<<(vecSpatialResult[vecIndex].second)->smplAvgSpatialMiddle<<" ";  
      }
      // debug END 
      if ( flagLastLevel == true) {
        vecIndex=0; 
        for(j = 0; j < numBlocksInLevel; j++) {
          if(vecIndex<vecSpatialResult.size()) {
            if(vecSpatialResult[vecIndex].first == j) {
              if ((vecSpatialResult[vecIndex].second)->smplAvgSpatialMiddle >= 0.01) {
                outFile << std::fixed << std::setprecision(2)  << vecSpatialResult[vecIndex].first <<","<<(vecSpatialResult[vecIndex].second)->smplAvgSpatialMiddle<<" ";
              }
              vecIndex++;
            } 
          }
        }
      } else {
        vecIndex=0; 
        for(j = 0; j < numBlocksInLevel; j++) {
          if(vecIndex<vecSpatialResult.size()) {
            if(vecSpatialResult[vecIndex].first == j) {
              outFile << std::fixed << std::setprecision(2)  << vecSpatialResult[vecIndex].first <<","<<(vecSpatialResult[vecIndex].second)->smplAvgSpatialMiddle<<" ";
              vecIndex++;
            } 
          }
        }
      }
      outFile << endl;
      /*
      // Print  Self and top-3 spatial density
      for(j=0; j<vecSpatialResult.size(); j++) {
        if(vecSpatialResult[j].first != blockID.second) 
          vecDensity.push_back(make_pair((vecSpatialResult[j].second)->smplAvgSpatialMiddle,vecSpatialResult[j].first));
        else
         selfSpatialDensity= (vecSpatialResult[j].second)->smplAvgSpatialMiddle; 
        }
      sort(vecDensity.begin(), vecDensity.end(), greater<>());
      outFile << " Spatial Density in order ";
      for(j=0; j<vecDensity.size(); j++) {
            outFile << std::fixed << std::setprecision(2)  << vecDensity[j].second <<","<<vecDensity[j].first<<" ";
      }
      outFile << " Self "<< std::fixed << std::setprecision(2)  << blockID.second <<","<< selfSpatialDensity ; 
      outFile<<endl;
    
      outFile << "Page "<< std::dec << blockID.second << " Spatial Ratio ";
      for(j=0; j<vecSpatialResult.size(); j++) { 
         if(((vecSpatialResult[j].second)->spatialAccess!=0)) 
          outFile << std::fixed << std::setprecision(3) << vecSpatialResult[j].first <<","<<(double)(((vecSpatialResult[j].second)->spatialTotalDistance)/(vecSpatialResult[j].second)->spatialAccess)/lifetime<<" ";
      }
      outFile<<endl;
      */

      int printDebug =0;
      if(printDebug){
        //printf("lifetime %d\n", lifetime); 
        printf("Page %d spatial Next ", blockID.second);
        for(j=0; j<vecSpatialResult.size(); j++) 
         if(((vecSpatialResult[j].second)->spatialNext!=0)) printf("%d,%d ",vecSpatialResult[j].first, (vecSpatialResult[j].second)->spatialNext); 
        printf("\nPage %d spatial Distance ", blockID.second);
        for(j=0; j<vecSpatialResult.size(); j++) 
         if(((vecSpatialResult[j].second)->spatialTotalDistance!=0)) printf("%d,%d ",vecSpatialResult[j].first, (vecSpatialResult[j].second)->spatialTotalDistance); 
        printf("\nPage %d spatial Access ", blockID.second);
        for(j=0; j<vecSpatialResult.size(); j++) 
         if(((vecSpatialResult[j].second)->spatialAccess!=0)) printf("%d,%d ",vecSpatialResult[j].first, (vecSpatialResult[j].second)->spatialAccess); 
        printf("\nPage %d spatial Middle ", blockID.second);
        for(j=0; j<vecSpatialResult.size(); j++) 
         if(((vecSpatialResult[j].second)->spatialAccessTotalMid!=0)) printf("%d,%d ",vecSpatialResult[j].first, (vecSpatialResult[j].second)->spatialAccessTotalMid); 
        printf("\n");
      }
  }
}

void BlockInfo::printBlockSpatialProb(std::ofstream& outFile, uint64_t blockWidth, bool flagLastLevel){
    unsigned int j=0;
    vector <pair<uint32_t, class SpatialRUD*>>::iterator itrSpRUD;
    if(totalAccess != 0 && lifetime !=0){
      printf("=== Page %d: area %08lx-%08lx  Lifetime %d Access %d \n", blockID.second, addrMin, addrMax, lifetime, totalAccess);
      outFile << "=== ID " << strBlockId << " Page "<< std::dec << blockID.second << " : area "<<hex<<addrMin<<"-"<<addrMax<< std::dec <<" Lifetime "<< lifetime 
              <<" Access " << totalAccess << " Block_count " << std::dec<< ((addrMax-addrMin)+1)/blockWidth << " Spatial_Prob " ;
      
      uint32_t vecIndex=0;
      // debug START 
      for(vecIndex = 0; vecIndex < vecSpatialResult.size(); vecIndex++) {
        cout << std::fixed << std::setprecision(2)  << vecSpatialResult[vecIndex].first <<","<<(((double)(vecSpatialResult[vecIndex].second)->spatialNext))/(double)totalAccess<<" ";  
      }
      // debug END
      if ( flagLastLevel == true) {
        vecIndex=0; 
        for(j = 0; j < numBlocksInLevel; j++) {
          if(vecIndex<vecSpatialResult.size()) {
            double printProbValue = ((double)(vecSpatialResult[vecIndex].second)->spatialNext)/(double)totalAccess;
            if(vecSpatialResult[vecIndex].first == j) {
              if((printProbValue>=0.01)) {
                outFile << std::fixed << std::setprecision(2)  << vecSpatialResult[vecIndex].first <<","<<printProbValue<<" ";
              }
              vecIndex++;
            } 
        }
      }
    } else {
        vecIndex=0; 
        for(j = 0; j < numBlocksInLevel; j++) {
          if(vecIndex<vecSpatialResult.size()) {
            if(vecSpatialResult[vecIndex].first == j) {
              outFile << std::fixed << std::setprecision(2)  << vecSpatialResult[vecIndex].first <<","<<((double)(vecSpatialResult[vecIndex].second)->spatialNext)/(double)totalAccess<<" ";
              vecIndex++;
            } 
        }
      }
    }
      outFile << endl;
  }
}
  
  void BlockInfo::printBlockInfo(){
    printf("*** Page ID %d Area %08lx-%08lx level %d block num %d  Access %d RUD avg %lf min %d max %d lifetime %d sampleAvgRUD %f \n "
               , blockID.second, addrMin, addrMax, blockID.first, blockID.second, totalAccess, avgRUD, minRUD, maxRUD, lifetime, sampleAvgRUD);
  }

