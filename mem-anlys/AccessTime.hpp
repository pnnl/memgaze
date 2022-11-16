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
//OZGURCLEANUP class Address;
//OZGURCLEANUP class Instruction;
//OZGURCLEANUP class CPU;
//OZGURCLEANUP class AccessTime;
//***************************************************************************
using namespace std;


//This class hold the access time
//v1: hold a pointer to each other classes
//v2: holds an array/map to CPUs
class AccessTime {
  public:
    unsigned long time;
    uint32_t sampleID;
    //v1
//OZGURCLEANUP    Address *addr;
//OZGURCLEANUP    CPU *cpu;
//OZGURCLEANUP    Instruction *ip;
  AccessTime(unsigned long _time, uint32_t _sampleID){
    time = _time;
    sampleID = _sampleID;
  }
  AccessTime(unsigned long _time){
    time = _time;
  }
  AccessTime (){
    time = 0;
  }

/*    AccessTime(unsigned long _time, Address *_addr = NULL, Instruction *_ip = NULL, CPU *_cpu = NULL ){
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
*/
    void setTime(unsigned long _time){ time = _time;}
/*    void setAll(unsigned long _time, int _sampleID,  CPU *_cpu , Address *_addr , Instruction *_ip){
      cpu = _cpu;
      time = _time;
      ip = _ip;
      addr = _addr;
      sampleID =  _sampleID;
    }
*/
    bool operator < (const AccessTime& input) const
    {
      return (time < input.time);
    }

    bool operator > (const AccessTime& input) const
    {
      return (time > input.time);
    }
    void setSampleID(uint32_t _sampleID){ sampleID = _sampleID;}
    uint32_t getSampleID(){return sampleID;}
};

#endif
