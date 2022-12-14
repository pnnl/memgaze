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
class SpatialRUD {
public:
  uint32_t totalAccess;
  uint32_t spatialDistance;
  uint32_t spatialTotalDistance;
  int      spatialMinDistance;
  int      spatialMaxDistance;
  uint32_t spatialAccess;
  uint32_t spatialAccessMid;
  uint32_t spatialAccessTotalMid;
  uint32_t spatialNext;
  uint32_t smplMiddle ;
  double   smplAvgSpatialMiddle;
 
//This class holds the Block's spatial results - spatial distance metrics
 SpatialRUD(uint32_t pageBlkID);
} ; 
  
