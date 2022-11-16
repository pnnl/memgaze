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
//#include "AccessTime.hpp"
#include "Trace.hpp"
#include "metrics.hpp"
using namespace std;


namespace memgaze {
class Function {
  public:
    int fp;
    uint16_t ncpus;
    std::vector<int> cpuFP;
    double totalLoads;
    std::string name;
    uint32_t nameID;
    uint16_t load_module;
    unsigned long startIP;
    unsigned long endIP;
    std::vector<Function *> children;
    Function * parrent;
 //OZGURCLEANUP    std::vector<AccessTime *> timeVec;
    Trace * trace;
    map <unsigned long, map <enum Metrics,int>> fpMap; //<address < type, count>
    std::vector <map <unsigned long, map <enum Metrics,int>>> cpuFPMap; //<address < type, count>
    void getdiagMap (map <enum Metrics,int> *typeMap, map <enum Metrics, double> *fpDiagMap);  
    void getFPDiag(map <enum Metrics, double> *diagMap); 
    void getCPUFPDiag(map <enum Metrics, double> *cpuDiagMap, uint16_t cpuid);
    int getFP();    
    void calcFP();
    void calcCPUFP();
    Function  (std::string _name, unsigned long _s = 0,  unsigned long _e = 0, int _ncpus = 24);
    Function  (std::string _name, uint16_t _load_module, unsigned long _s = 0,  unsigned long _e = 0, int _ncpus = 24);

//OZGURCLEANUP  DEPRICATE  ??    std::vector<AccessTime *> calculateFunctionFP(); // depricate TODO: remove/revisit
//OZGURCLEANUP DEPRICATE ??    std::vector<AccessTime *> calculateFunctionCPUFP(); // depricated TODO: remove/revisit
    void  printFunctionTree();
    float getMultiplier(unsigned long  period, bool is_load = false);
  private:
    Trace * calculateFunctionFPRec(Function *root,  int level );
//OZGURCLEANUP    std::vector<Access *> calculateFunctionFPRec(Function *root,  int level );
    Trace * calculateFunctionCPUFPRec(Function *root,  int level );
//OZGURCLEANUP    std::vector<Access *> calculateFunctionCPUFPRec(Function *root,  int level );
    void  printFunctionTreeRec(Function *root, int level );
};

} // namespace memgaze
#endif
