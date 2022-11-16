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
#include "metrics.hpp"
//***************************************************************************
//OZGURCLEANUP class Address;
//OZGURCLEANUP class Instruction;
//OZGURCLEANUP class CPU;
//OZGURCLEANUP class AccessTime;
//***************************************************************************

using namespace std;

//This class will hold ip
//v1 ip: holds instruction IP and there will be pointers to all other classes
//v2 holds an array/map to the memory addresss
class Instruction {
  public:
    unsigned long ip;
    enum Metrics type;
//OZGURCLEANUP    AccessTime *time;
//OZGURCLEANUP    CPU *cpu;
    //v1
//OZGURCLEANUP    Address *addr; 
//OZGURCLEANUP   int type; //This is a type selector //OZGURCLEANUP TODO this should be enum
             // -1 is uknown
             // 0 is frame/constant
             // 1 strided
             // 2 is indirect
    uint16_t extra_frame_lds;
    uint16_t load_module;
    uint32_t func_name;
//OZGURCLEANUP    string dso_name; //OZGURCLEANUP TODO some type of map
//OZGURCLEANUP    string funcName; //OZGURCLEANUP TODO  some type of map

    Instruction(unsigned long _ip){ ip=_ip;}

//OZGURCLEANUP
/*    Instruction(unsigned long _ip, Address *_addr = NULL, AccessTime *_time = NULL, CPU *_cpu = NULL ){
      cpu = _cpu;
      time = _time;
      ip = _ip;
      addr= _addr;
    }
*/    
    void setLoadModule(uint16_t inName){load_module = inName;}
    string getLoadModule(map <uint16_t, string> lm_map){ return lm_map.find(load_module)->second;}
    void setFuncName(uint32_t inName){func_name = inName;}
    string getFuncName(map <uint32_t, string> func_map){ return func_map.find(load_module)->second;}


    void setExtraFrameLds(uint16_t _lds){extra_frame_lds = _lds;}
//OZGURCLEANUP    void setDSOName(string inName){dso_name = inName;}
//OZGURCLEANUP    string getDSOName(){ return dso_name;}
//OZGURCLEANUP    void setFuncName(string inName){funcName = inName;}
//OZGURCLEANUP    string getFuncName(){ return dso_name;}
    uint16_t getExtraFrameLds(){ return extra_frame_lds;}
    void setIp(unsigned long _ip){ ip = _ip;}
    unsigned long getIp(){ return ip;}
    void setType(enum Metrics _type){ type = _type;}
    enum Metrics setType(){ return type;}
//OZGURCLEANUP
/*    void setAll(unsigned long _ip, CPU *_cpu, AccessTime *_time, Address *_addr, int _type){
      cpu = _cpu;
      time = _time;
      ip = _ip;
      addr = _addr;
      type = _type;
    }
*/
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
