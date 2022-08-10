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
#ifndef INSTRUCTION_H
#define INSTRUCTION_H

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

//This class will hold ip
//v1 ip: holds instruction IP and there will be pointers to all other classes
//v2 holds an array/map to the memory addresss
class Instruction {
  public:
    unsigned long ip;
    AccessTime *time;
    CPU *cpu;
    //v1
    Address *addr; 
    int type; //This is a type selector
             // -1 is uknown
             // 0 is frame/constant
             // 1 strided
             // 2 is indirect
    int extra_frame_lds;
    string dso_name;

    Instruction(unsigned long _ip, Address *_addr = NULL, AccessTime *_time = NULL, CPU *_cpu = NULL ){
      cpu = _cpu;
      time = _time;
      ip = _ip;
      addr= _addr;
    }
    
    void setExtraFrameLds(int _lds){extra_frame_lds = _lds;}
    void setDSOName(string inName){dso_name = inName;}
    string getDSOName(){ return dso_name;}
    int getExtraFrameLds(){ return extra_frame_lds;}
    void setIp(unsigned long _ip){ ip = _ip;}
    void setType(unsigned long _type){ type = _type;}
    void setAll(unsigned long _ip, CPU *_cpu, AccessTime *_time, Address *_addr, int _type){
      cpu = _cpu;
      time = _time;
      ip = _ip;
      addr = _addr;
      type = _type;
    }
    bool operator < (const Instruction& input) const
    {
      return (ip < input.ip);
    }

    bool operator > (const Instruction& input) const
    {
      return (ip > input.ip);
    }
};

#endif
