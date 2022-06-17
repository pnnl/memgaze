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
  BlockInfo::BlockInfo(pair<unsigned int, unsigned int> _blockID, uint64_t _addrMin, uint64_t _addrMax, unsigned int _numBlocksInLevel, int spatialResult)
  {
    blockID.first = _blockID.first;
    blockID.second = _blockID.second;
    addrMin = _addrMin;
    addrMax = _addrMax;
    numBlocksInLevel = _numBlocksInLevel;
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
        printf("sample_average RUD %f", sampleAvgRUD);
      } 
      printf("\n");
    }
  }
  void BlockInfo::printBlockSpatial(std::ofstream& outFile){
    unsigned int j=0;
    //std::map<uint32_t, class SpatialRUD*>::iterator itrSpRUD;
    //vector <pair<uint32_t, class SpatialRUD*>> vecSpatialResult;
    vector <pair<uint32_t, class SpatialRUD*>>::iterator itrSpRUD;
    if(totalAccess != 0 && lifetime !=0){
      printf("*** Page %d: area %08lx-%08lx Access %d Lifetime %d\n", blockID.second, addrMin, addrMax, totalAccess, lifetime );
      outFile << "*** Page "<< std::dec << blockID.second << " : area "<<hex<<addrMin<<"-"<<addrMax<<" Access " 
          << std::dec << totalAccess <<" Lifetime "<< lifetime << endl;
/*      outFile << "Page "<< std::dec << blockID.second << " spatial Next ";
      for(j=0; j<vecSpatialResult.size(); j++) { 
         if(((vecSpatialResult[j].second)->spatialNext!=0)) 
          outFile << std::dec << vecSpatialResult[j].first <<","<<(vecSpatialResult[j].second)->spatialNext<<" ";
      }
      outFile<<endl;
      outFile << "Page "<< std::dec << blockID.second << " spatial Distance ";
      for(j=0; j<vecSpatialResult.size(); j++) { 
         if(((vecSpatialResult[j].second)->spatialTotalDistance!=0)) 
          outFile << std::dec << vecSpatialResult[j].first <<","<<(vecSpatialResult[j].second)->spatialTotalDistance<<" ";
      }
      outFile<<endl;
      outFile << "Page "<< std::dec << blockID.second << " spatial Access ";
      for(j=0; j<vecSpatialResult.size(); j++) { 
         if(((vecSpatialResult[j].second)->spatialAccess!=0)) 
          outFile << std::dec << vecSpatialResult[j].first <<","<<(vecSpatialResult[j].second)->spatialAccess<<" ";
      }
      outFile<<endl;
      outFile << " spatial Middle ";
      for(j=0; j<vecSpatialResult.size(); j++) { 
         if(((vecSpatialResult[j].second)->spatialAccessTotalMid!=0)) 
          outFile << std::dec << vecSpatialResult[j].first <<","<<(vecSpatialResult[j].second)->spatialAccessTotalMid<<" ";
      }
      outFile<<endl;
*/
      vector<pair<double, uint32_t>> vecDensity;
      //outFile << " Spatial Density ";
      for(j=0; j<vecSpatialResult.size(); j++) { 
         if(((vecSpatialResult[j].second)->spatialAccessTotalMid!=0))  {
       //   outFile << std::fixed << std::setprecision(3)  << vecSpatialResult[j].first <<","<<(double)((vecSpatialResult[j].second)->spatialAccessTotalMid)/lifetime<<" ";
        vecDensity.push_back(make_pair(((double)((vecSpatialResult[j].second)->spatialAccessTotalMid)/lifetime),vecSpatialResult[j].first));
        }
      }
      //outFile<<endl;
      sort(vecDensity.begin(), vecDensity.end(), greater<>());
      outFile << " Spatial Density in order ";
      for(j=0; j<vecDensity.size(); j++) {
          if(vecDensity[j].first >= 0.1)
          outFile << std::fixed << std::setprecision(3)  << vecDensity[j].second <<","<<vecDensity[j].first<<" ";
      }
      outFile<<endl;
      vecDensity.clear();
      for(j=0; j<vecSpatialResult.size(); j++) { 
        vecDensity.push_back(make_pair((vecSpatialResult[j].second)->smplAvgSpatialMiddle,vecSpatialResult[j].first));
        }
      sort(vecDensity.begin(), vecDensity.end(), greater<>());
      outFile << " Spatial Density in order ";
      for(j=0; j<vecDensity.size(); j++) {
          if(vecDensity[j].first >= 0.005)
          outFile << std::fixed << std::setprecision(3)  << vecDensity[j].second <<","<<vecDensity[j].first<<" ";
      }
      outFile<<endl;
      
 /*     outFile << "Page "<< std::dec << blockID.second << " Spatial Probability ";
      for(j=0; j<vecSpatialResult.size(); j++) { 
         if(((vecSpatialResult[j].second)->spatialAccess!=0)) 
          outFile << std::fixed << std::setprecision(3)  << vecSpatialResult[j].first <<","<<(double)((vecSpatialResult[j].second)->spatialAccess)/totalAccess<<" ";
      }
      outFile<<endl;
      outFile << "Page "<< std::dec << blockID.second << " Spatial Ratio ";
      for(j=0; j<vecSpatialResult.size(); j++) { 
         if(((vecSpatialResult[j].second)->spatialAccess!=0)) 
          outFile << std::fixed << std::setprecision(3) << vecSpatialResult[j].first <<","<<(double)(((vecSpatialResult[j].second)->spatialTotalDistance)/(vecSpatialResult[j].second)->spatialAccess)/lifetime<<" ";
      }
      outFile<<endl;
      outFile << "Page "<< std::dec << blockID.second << " Spatial Next-Coexistence ";
      for(j=0; j<vecSpatialResult.size(); j++) { 
         if(((vecSpatialResult[j].second)->spatialNext!=0)) 
          outFile << std::fixed << std::setprecision(3) << vecSpatialResult[j].first <<","<<(double)((vecSpatialResult[j].second)->spatialNext)/totalAccess<<" ";
      }
      outFile<<endl;
  */

      int printDebug =1;
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
        //for(j = 0; j < numBlocksInLevel; j++)
         //if(*(spatialNext+j) !=0) printf(" %d,%d ",j, *(spatialNext+j)); 
        //for(j = 0; j < numBlocksInLevel; j++)
        // if(*(totalSpatialDist+j)!=0) printf("%d,%d ", j,*(totalSpatialDist+j)); 
        //for(j = 0; j < numBlocksInLevel; j++)
        // if(*(spatialAccess+j)!=0) printf("%d,%d ", j,*(spatialAccess+j)); 
        //for(j = 0; j < numBlocksInLevel; j++)
        // if(*(spatialMiddle+j)!=0) printf("%d,%d ", j,*(spatialMiddle+j)); 
      }
      /*
      printf("Spatial Probability - Access_Possibility_immediate for Page %d : [ ", blockID.second);
      for(j = 0; j < numBlocksInLevel; j++){
        if(*(spatialAccess+j)!=0) printf("%f ",  (double)(*(spatialNext+j))/(double)(totalAccess));
        else printf("N/A ");
      }
      printf("]\n");
      printf("Average spatial distance for Page %d : [ ", blockID.second);
      for(j = 0; j < numBlocksInLevel; j++){
        if(*(spatialAccess+j)!=0) printf("%f ",  (double)(*(totalSpatialDist+j))/(double)(*(spatialAccess+j)));
        else printf("N/A ");
      }
      printf("]\n");
      */
      /*
      printf("Min spatial distance for Page %d : [ ", blockID.second);
      for(j = 0; j < numBlocksInLevel; j++){
        if(*(spatialAccess+j)!=0) printf("%d ",  *(minSpatialDist+j));
        else printf("N/A ");
      }
      printf("]\n");
      printf("Max spatial distance for Page %d : [ ", blockID.second);
      for(j = 0; j < numBlocksInLevel; j++){
        if(*(spatialAccess+j)!=0) printf("%d ",  *(maxSpatialDist+j));
        else printf("N/A ");
      }
      printf("]\n");
      */
      /*
      printf("Spatial density - Access_Possibility_in_Middle for Page %d : [ ", blockID.second);
      for(j = 0; j < numBlocksInLevel; j++){
        // Original code
        //if ((totalAccess > 1 )  && (*(spatialMiddle+j)!=0 ))printf("(%d, %f) ", j, (double)(*(spatialMiddle+j))/(double)(lifetime));
        if ((totalAccess > 1 ) &&(*(spatialMiddle+j) !=0))printf("%f ", (double)(*(spatialMiddle+j))/(double)(totalAccess-1));
        else printf("N/A ");
      }
      printf("]\n");
      printf("Spatial ratio - Avergae_SD_over_lifetime for Page %d : [ ", blockID.second);
      for(j = 0; j < numBlocksInLevel; j++){
        if ((lifetime >= 1 && (*(spatialAccess+j)!=0)))printf("%f ",(double)(*(totalSpatialDist+j))/(double)((*(spatialAccess+j)*lifetime)));
        else printf("N/A ");
      }
      printf("]\n");

      vector<pair<double, int>> distanceVP;
      vector<pair<double, int>> possibilityVP;
      for(j = 0; j < numBlocksInLevel; j++){
        if (*(totalAccessAllBlocks+j) > 1 ){
          //if(printDebug) printf("Page ID i %d j %d spatTotalDist %d spatAcs %d d    istance value %f spatAcsTotMid %d lifetime %d possibility value %f\n", i, j,
          // totalSpatialDist[i][j],spatialAccess[i][j], (double)spatialTotal    Distance[i][j]/(double)(spatialAccess[i][j]),
            //spatialAccessTotalMid[i][j], lifetime[i], (double)spatialAccessTotal    Mid[i][j]/(double)(lifetime[i]) );
          if(*(totalSpatialDist+j)!=0)
            distanceVP.push_back(make_pair((double)(*(totalSpatialDist+j))/(double)(*(spatialAccess+j)), j));
          if(*(spatialMiddle+j) !=0)
            possibilityVP.push_back(make_pair((double)(*(spatialMiddle+j))/(double)(lifetime),j));
        }
      }
      sort(distanceVP.begin(), distanceVP.end());
      sort(possibilityVP.begin(), possibilityVP.end(),greater<>());
      printf("The closest three pages in Spatial Distance for Page %d : [", blockID.second);
      for (uint32_t n = 0; n < 3; n++){
        if(distanceVP.size()>n){
          //printf("%f %d ", distanceVP[n].first, distanceVP[n].second);
          printf("%d ", distanceVP[n].second);
        } else {
          printf("N/A ");
        }
      }
      printf("]\n");
      printf("The highest frequent three pages in Middle for Page %d : [", blockID.second);
      for (uint32_t n = 0; n < 3; n++){
        if(possibilityVP.size()>n){
          printf("%d ",possibilityVP[n].second);
          //printf(" %f %d, ",possibilityVP[n].first, possibilityVP[n].second);
        } else{
          printf("N/A ");
        }
      }
      printf("]\n\n");
      */
    }
  }
  
  void BlockInfo::printBlockInfo(){
    printf("*** Page ID %d Area %08lx-%08lx level %d block num %d  Access %d RUD avg %lf min %d max %d lifetime %d sampleAvgRUD %f \n "
               , blockID.second, addrMin, addrMax, blockID.first, blockID.second, totalAccess, avgRUD, minRUD, maxRUD, lifetime, sampleAvgRUD);
  }

