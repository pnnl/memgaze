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
#ifndef WINDOW_H
#define WINDOW_H

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
//OZGURCLEANUP class Address;
//OZGURCLEANUP class Instruction;
//OZGURCLEANUP class CPU;
//OZGURCLEANUP class AccessTime;
//class Trace;
//class Access;

#include "Trace.hpp"
//#include "AccessTime.hpp"

//***************************************************************************
using namespace std;

// This class holds a list of the memory acceses and
// a list of the child windows wiht a parent window pointer.
class Window {
  public:
    Window * left, *right, *parent;
    bool sampleHead;
    float multiplier;
    unsigned long constant_lds;
    bool in_sample;
    unsigned long stime;
    unsigned long mtime;
    unsigned long etime;
    unsigned long period;
    //OZGURCLEANUP std::string funcName; //this change to uint32_t
//OZGURCLEANUP DEPRICATE ??    uint32_t funcName;

//OZGURCLEANUP    Address* maxFP_addr; //TODO Why we need this??
    //OZGURCLEANUP vector <Address *> addresses; // I need trace here
    Trace *trace;
    pair<unsigned long, uint32_t> windowID;
    map <enum Metrics, double> fpMetrics;
    map <unsigned long, map <enum Metrics, uint32_t>> fpMap; //<address < type, count> 
    
    Window ();
    ~Window ();
//OZGURCLEANUP DEPRICATE ??    void setFuncName( std::string _name );
//OZGURCLEANUP DEPRICATE ??    void setFuncName();
    //TODO void getFunctionFP( std::string _name map<int,float>)
//OZGURCLEANUP DEPRICATE ??    std::string getFuncName( );
    int getSize();
    unsigned long getConstantLds(){ return constant_lds;}
    unsigned long calcConstantLds();
    void setConstantLds(unsigned long _lds){ constant_lds =  _lds;}
    void setStime( unsigned long _stime );
    void setEtime( unsigned long _etime );
    void setMtime( unsigned long _etime );
    void calcTime();
    void setID ( pair<unsigned long, uint32_t> _windowID ); 
//OZGURCLEANUP    void addAddress(Address *add);
    void addAccess(Access *access);
    void addWindow(Window * w);
    pair<unsigned long, uint32_t> getWindowID ();
    int getFP();
    void setParent ( Window *w);
    void setPeriod(unsigned long _period){period =  _period;}
    void rmFPMap();
    void createFPMap();
    void removeTrace ();
    void fillTrace();
    void fillTraceWalkDown(Window *w);
    void fillTraceWalkUp(Window *w);
    void addRightChild (Window *w);
    void addLeftChild (Window *w);
    //void getdiagMap (map <enum Metrics,uint32_t> *typeMap, map <enum Metrics, double> *fpDiagMap);  
    void getdiagMap (map <enum Metrics,uint32_t> *typeMap);  
    void  getFPDiag(map <enum Metrics, double> *diagMap);
    void  calcFPMetrics();
    float calcMultiplier();
    float getMultiplier();
    map<enum Metrics, double> getMetrics();
    map<enum Metrics, double> calculateMetrics();
    void removeFPMap();
};

#endif
