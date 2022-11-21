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
#ifndef TRACE_H
#define TRACE_H

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
#include "metrics.hpp"
//***************************************************************************
//class Access;
#include "Access.hpp"
//***************************************************************************
using namespace std;

//This class hold the access time
//v1: hold a pointer to each other classes
//v2: holds an array/map to CPUs
class Trace {
  public:
  vector<Access *> trace;
  
  ~Trace(){emptyTrace();}
  
  vector<Access*> * getTrace(){return &trace;}

  int getSize(){return trace.size();}

  void addAccess(Access * access){
    trace.push_back(access);
  }
  
  void emptyTrace(){
    this->trace.clear();
  }

  void removeAfter(int index){
    trace.erase(trace.begin()+index+1, trace.end());
  }
  void removeBefore( int index){
    trace.erase(trace.begin(), trace.begin()+index);
  }
  
  void addTrace(Trace * inTrace){
    for (auto  it = inTrace->trace.begin(); it != inTrace->trace.end(); it++){
      this->addAccess(*it);
    }
  }

};

#endif
