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
#ifndef ACCESS_H
#define ACCESS_H

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
//class Address;
//class Instruction;
//class CPU;
//class AccessTime;
#include "Address.hpp"
#include "Instruction.hpp"
#include "CPU.hpp"
#include "AccessTime.hpp"

//***************************************************************************
using namespace std;

class Access {
  public:
  Address *addr;
  CPU *cpu;
  Instruction *ip;
  AccessTime *time;




  Access(unsigned long _ip,  uint16_t _cpu, unsigned long _addr, unsigned long _time){
    addr =  new Address(_addr);
    cpu =  new CPU(_cpu);
    ip =  new Instruction(_ip);
    time =  new AccessTime(_time);
   
//    addr.setAddr(_addr);
//    cpu.setCPU(_cpu);
//    ip.setIp(_ip);
//    time.setTime(_time);
  }
  
  Access();
  ~Access(){
    delete addr;
    delete cpu;
    delete ip;
    delete time;
  }
  Instruction* getIP(){return ip;}
  Address * getAddr(){return addr;}
  CPU * getCPU(){return cpu;}
  AccessTime * getTime(){return time;}

};

#endif
