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
class Address;
class Instruction;
class CPU;
class AccessTime;

#include "AccessTime.hpp"

//***************************************************************************
using namespace std;

// This class holds a list of the memory acceses and
// a list of the child windows wiht a parent window pointer.
class Window {
  public:
    Window * left, *right, *parent;
    bool sampleHead;
    float ws, zs;
    float multiplier;
    float multiplierAvg;
    unsigned long constant_lds;
    int number_of_samples;
    unsigned long stime;
    unsigned long mtime;
    unsigned long etime;
    std::string funcName;
    vector <Address *> addresses;
    pair<unsigned long, int> windowID;
    map <unsigned long, map <int,int>> fpMap; //<address < type, count> 
    //unsigned long total_lds;
//    map <int, map <string, map <int,int>>> fpFunctionMap; //<fp, <function name , <type,fp>>> //TODO NOTE:: fix after collecting data
    Window ();
    ~Window ();
    void setFuncName( std::string _name );
    void setFuncName();
    //TODO void getFunctionFP( std::string _name map<int,float>)
    std::string getFuncName( );
    int getSize();
    unsigned long getConstantLds(){ return constant_lds;}
    unsigned long calcConstantLds();
    //unsigned long getTotalLds(){ return total_lds;}
    void setConstantLds(unsigned long _lds){ constant_lds =  _lds;}
    //void setTotalLds(unsigned long _lds){ total_lds =  _lds;}
    int getNumberOfSamples() {return number_of_samples;}
    void setNumberOfSamples( int _nSample){ number_of_samples= _nSample;}
    void setStime( unsigned long _stime );
    void setEtime( unsigned long _etime );
    void setMtime( unsigned long _etime );
    void calcTime();
    void setID ( pair<unsigned long, int> _windowID ); 
    void addAddress(Address *add);
    void addWindow(Window * w);
    pair<unsigned long, int> getWindowID ();
    int getFP();
    void setParent ( Window *w);
    void addRightChild (Window *w);
    void addLeftChild (Window *w);
    void getdiagMap (map <int,int> *typeMap, map <int, double> *fpDiagMap);  
    void  getFPDiag(map <int, double> *diagMap);
    float calcMultiplier(unsigned long period, bool is_load = false);
    float getMultiplier();
    map<int, double> getMetrics(map<int, double> diagMap);
    //float getMultiplierAvg();
    //void setMultiplierAvg(float _multiplierAvg){multiplierAvg = _multiplierAvg;}
};

#endif
