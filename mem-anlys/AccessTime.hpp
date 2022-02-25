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
#ifndef ACCESSTIME_H
#define ACCESSTIME_H

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
//***************************************************************************
using namespace std;


//This class hold the access time
//v1: hold a pointer to each other classes
//v2: holds an array/map to CPUs
class AccessTime {
  public:
    unsigned long time;
    int sampleID;
    //v1
    Address *addr;
    CPU *cpu;
    Instruction *ip;

    AccessTime(unsigned long _time, Address *_addr = NULL, Instruction *_ip = NULL, CPU *_cpu = NULL ){
      cpu = _cpu;
      time = _time;
      ip = _ip;
      addr= _addr;
      sampleID = 0;
    }
    
    AccessTime(unsigned long _time, int _sampleID, Address *_addr = NULL, Instruction *_ip = NULL, CPU *_cpu = NULL ){
      cpu = _cpu;
      time = _time;
      ip = _ip;
      addr= _addr;
      sampleID = _sampleID;
    }
    void setTime(unsigned long _time){ time = _time;}
    void setAll(unsigned long _time, int _sampleID,  CPU *_cpu , Address *_addr , Instruction *_ip){
      cpu = _cpu;
      time = _time;
      ip = _ip;
      addr = _addr;
      sampleID =  _sampleID;
    }
    bool operator < (const AccessTime& input) const
    {
      return (time < input.time);
    }

    bool operator > (const AccessTime& input) const
    {
      return (time > input.time);
    }
    void setSampleID(int _sampleID){ sampleID = _sampleID;}
    int getSampleID(){return sampleID;}
};

#endif
