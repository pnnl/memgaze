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
#ifndef CPU_H
#define CPU_H


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



//This class is for each CPU
//v1: Holds pointer to al other classes
//v2: Holds a list of ponters of  IP Addresses and pointer to the time.
class CPU {
  public:
    uint16_t cpu;
//OZGURCLEANUP    AccessTime *time;
    //For v1
//OZGURCLEANUP    Instruction *ip;
//OZGURCLEANUP    Address *addr;

    CPU(uint16_t _cpu){ cpu = _cpu;}
//OZGURCLEANUP
/*    CPU(int _cpu, Address *_addr = NULL, AccessTime *_time = NULL, Instruction *_ip = NULL ){
      cpu = _cpu;
      time = _time;
      ip = _ip;
      addr= _addr;
    }
*/
    void setCPU(uint16_t _cpu){ cpu = _cpu;}
    uint16_t getCPU(){return cpu;}
/*    void setAll(int _cpu,  Address *_addr , AccessTime *_time , Instruction *_ip){
      cpu = _cpu;
      time = _time;
      ip = _ip;
      addr = _addr;
    }
*/
    bool operator < (const CPU& input) const
    {
      return (cpu < input.cpu);
    }

    bool operator > (const CPU& input) const
    {
      return (cpu > input.cpu);
    }
};

#endif
