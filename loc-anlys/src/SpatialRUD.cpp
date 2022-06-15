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

#include "SpatialRUD.hpp"
using namespace std;

//This class holds the Block's spatial results - spatial distance metrics
  SpatialRUD::SpatialRUD(uint32_t pageBlkID)
  {
    totalAccess=0;
    spatialDistance=0;
    spatialTotalDistance=0;
    spatialMinDistance=-1;
    spatialMaxDistance=-1; 
    spatialAccess=0; 
    spatialAccessMid=0; 
    spatialAccessTotalMid=0; 
    spatialNext=0; 
  }
  
  
