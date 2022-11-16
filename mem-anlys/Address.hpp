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
#ifndef ADDRESS_H
#define ADDRESS_H

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


//This class will hold the memory addresses that has been accessed
//addr it the virtual address that has been accessed
//cpu holds a pointer to the cpu object
//time holds a pointer to the time object
//ip holds a pointer to the instruction pointer.
class Address {
  public:
    unsigned long addr;
//OZGURCLEANUP    string fName; TODO  needs to move to Instruction as  a map
//OZGURCLEANUP    CPU *cpu;
//OZGURCLEANUP    AccessTime *time;
//OZGURCLEANUP    Instruction *ip;
//OZGURCLEANUP    int rDist

    Address(unsigned long _addr){ addr = _addr;} 

//OZGURCLEANUP
/*    Address(unsigned long _addr, 
        int _rDist = -1, CPU *_cpu = NULL, AccessTime *_time = NULL, Instruction *_ip = NULL ){
//      rDist = _rDist;//REMOVING_XTRA
      cpu = _cpu;
      time = _time;
      ip = _ip;
      addr = _addr;
      fName = "";
    }
    void setFuncName(string _name){fName= _name; }
    string getFuncName(){return fName;}
//    void setReuseDistance(int _rDist) {rDist = _rDist;} //REMOVING_XTRA
*/
    void setAddr(unsigned long _addr){ addr = _addr;}
/*    void setAll(unsigned long _addr,  CPU *_cpu , AccessTime *_time , Instruction *_ip, int _rDist = -1){
//      rDist =  _rDist;//REMOVING_XTRA
      cpu = _cpu;
      time = _time;
      ip = _ip;
      addr = _addr;
    }
*/
    bool operator < (const Address& input) const
    { 
      cout << "Comparing this: " << hex << this->addr << " with " <<input.addr<<endl; 
      return (this->addr < input.addr);
    }

    bool operator > (const Address& input) const
    {
      return (this->addr > input.addr);
    }
};

#endif
