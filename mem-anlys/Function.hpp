// -*-Mode: C++;-*-

//*BeginPNNLCopyright********************************************************
//
// $HeadURL$
// $Id:
//
//**********************************************************EndPNNLCopyright*

//***************************************************************************
// $HeadURL$
//
// Ozgur Ozan Kilic, Nathan Tallent
//***************************************************************************

//***************************************************************************
#ifndef FUNCTION_H
#define FUNCTION_H


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
//***************************************************************************
#include "AccessTime.hpp"
using namespace std;

namespace memgaze {
class Function {
  public:
    int fp;
    double totalLoads;
    std::string name;
    unsigned long startIP;
    unsigned long endIP;
    std::vector<Function *> children;
    Function * parrent;
    std::vector<AccessTime *> timeVec;
    map <unsigned long, map <int,int>> fpMap; //<address < type, count> 
    void getdiagMap (map <int,int> *typeMap, map <int, double> *fpDiagMap);  
    void  getFPDiag(map <int, double> *diagMap); 
    int getFP();    
    void calcFP();
    Function  (std::string _name, unsigned long _s = 0,  unsigned long _e = 0);
    std::vector<AccessTime *> calculateFunctionFP();
    void  printFunctionTree();
    float getMultiplier(unsigned long  period, bool is_load = false);
  private:
    std::vector<AccessTime *> calculateFunctionFPRec(Function *root,  int level );
    void  printFunctionTreeRec(Function *root, int level );
};

} // namespace memgaze
#endif
