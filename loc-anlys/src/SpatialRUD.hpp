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
 uint16_t spatialDistance;
  uint16_t spatialTotalDistance;
  int      spatialMinDistance;
  int      spatialMaxDistance;
  uint16_t spatialAccess;
  uint16_t spatialAccessMid;
  uint16_t spatialAccessTotalMid;
  uint16_t spatialNext;
 
//This class holds the Block's spatial results - spatial distance metrics
 SpatialRUD(uint32_t pageBlkID);
} ; 
  
