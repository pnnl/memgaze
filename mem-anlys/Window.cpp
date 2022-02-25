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
#include "Window.hpp"
#include "Address.hpp"
#include "Instruction.hpp"
#include "CPU.hpp"
//***************************************************************************
using namespace std;
    
    Window::~Window() {
      
    }
    
    Window::Window () {
      left = NULL;
      right = NULL;
      parent = NULL;
      
      stime = 0;
      etime = 0;
      funcName = "";
      sampleHead = false;
      multiplier =  1;
      multiplierAvg = 1;
      constant_lds = 0;
//      total_lds = 0;
    }
    
    void Window::setFuncName( std::string _name ) { funcName = _name;}
    void Window::setFuncName( ) { 
      map <string , map <unsigned long, int>> localFuncFPMap;
      string name = "";
      for (auto ait=this->addresses.begin(); ait != this->addresses.end(); ait++){
        name =  (*ait)->getFuncName();
//        cout << "IP: "<<hex<<(*ait)->addr<<dec<<" Func: "<<name<<endl;
        map <string , map <unsigned long, int>>::iterator funcIter = localFuncFPMap.find(name);
        if (funcIter != localFuncFPMap.end()){
          map <unsigned long, int>::iterator fpIter = funcIter->second.find((*ait)->addr);
          if (fpIter != funcIter->second.end()){
            fpIter->second++;
          } else {
            funcIter->second.insert({(*ait)->addr,1});
          }
        } else {
          map <unsigned long, int> localFPMap;
          localFPMap.insert({(*ait)->addr,1});
          localFuncFPMap.insert({name, localFPMap});
        }
      }
      int maxFP = 0;
      for(auto it=localFuncFPMap.begin() ;  it != localFuncFPMap.end(); it++){
        if (maxFP < (*it).second.size()){
          this->funcName = (*it).first;
          maxFP = (*it).second.size();
        }
      }
    }
    std::string Window::getFuncName( ) { return funcName;}
    void Window::setStime( unsigned long _stime ) { stime = _stime;}
    void Window::setEtime( unsigned long _etime ) { etime = _etime;}
    void Window::setID ( pair<unsigned long, int> _windowID ) { windowID = _windowID; } 
    unsigned long Window::calcConstantLds(){
      //unsigned long constant_lds = 0;
      for (auto it = addresses.begin(); it != addresses.end();  it++){
        this->constant_lds += (*it)->ip->getExtraFrameLds();
      }
      return this->constant_lds;
    }
    void Window::addAddress(Address *add){
      addresses.push_back(add);
//cout << "DEBUG:::" << __func__ << " : " << __LINE__ << endl;
      map <unsigned long, map <int, int>>::iterator it = fpMap.find(add->addr);
      if (it == fpMap.end()){
 //       cout << "This is a new address:0x"<<hex<<add->addr<<" with type "<<dec<<add->ip->type<<endl;
        map <int, int> addthis;
        addthis.insert({add->ip->type, 1});
        if (add->ip->getExtraFrameLds() >0){
          addthis.insert({0,add->ip->getExtraFrameLds()});
        }
        fpMap.insert({add->addr , addthis});
      } else {
        map <int, int>::iterator tit = it->second.find(add->ip->type);
        if (tit != it->second.end()){
//        cout << "I am adding to an existing entry with addr 0x"<<hex<<add->addr<<dec<<" type:"<<add->ip->type<<endl;
//" count:"<<tit->second.second<<endl;
          tit->second++;
        } else {
//        cout << "I am adding to an existing entry with addr 0x"<<hex<<add->addr<<dec<<" a new type:"<<add->ip->type<<endl;
          it->second.insert({add->ip->type, 1});
        }
        if (add->ip->getExtraFrameLds() >0){
          tit = it->second.find(0);
          if (tit != it->second.end()){
            tit->second+=add->ip->getExtraFrameLds();
          } else {
            it->second.insert({0,add->ip->getExtraFrameLds()});
          }
        }

//        cout << "I add addr 0x"<<hex<<add->addr<<dec<<" type:"<<add->ip->type<<endl;//" count:"<<tit->second.second<<endl;
      }
//      for (auto iter = this->fpMap.find(add->addr)->second.begin(); iter != this->fpMap.find(add->addr)->second.end(); iter++){
//        cout << "printing FP map's type Map index: "<<this->windowID.first<<":"<<this->windowID.second<<" type:"<<iter->first<<" count"<<iter->second<<endl;  
//      }
    }
    
    void Window::addWindow(Window * w){
//      for (auto ait=w->addresses.begin(); ait != w->addresses.end(); ait++){
//        addresses.push_back(*ait);
//      }
//cout <<__func__ << " "<< __LINE__<<endl;
      if (this->fpMap.empty()){
        for (auto ait=this->addresses.begin(); ait != this->addresses.end(); ait++){
          map <unsigned long, map <int, int>>::iterator it = this->fpMap.find((*ait)->addr);
          if (it == fpMap.end()){
 //           cout << "This is a new address:0x"<<hex<<add->addr<<" with type "<<dec<<add->ip->type<<endl;
            map <int, int> addthis;
            addthis.insert({(*ait)->ip->type, 1});
            fpMap.insert({(*ait)->addr , addthis});
          } else {
            map <int, int>::iterator tit = it->second.find((*ait)->ip->type);
            if (tit != it->second.end()){
//            cout << "I am adding to an existing entry with addr 0x"<<hex<<add->addr<<dec<<" type:"<<add->ip->type<<endl;//" count:"<<tit->second.second<<endl;
              tit->second++;
            } else {
//            cout << "I am adding to an existing entry with addr 0x"<<hex<<add->addr<<dec<<" a new type:"<<add->ip->type<<endl;
              it->second.insert({(*ait)->ip->type, 1});
            }

//            cout << "I add addr 0x"<<hex<<add->addr<<dec<<" type:"<<add->ip->type<<endl;//" count:"<<tit->second.second<<endl;
          }
//          this->addAddress(*ait);  
        }
        for (auto ait=w->addresses.begin(); ait != w->addresses.end(); ait++){
          map <unsigned long, map <int, int>>::iterator it = this->fpMap.find((*ait)->addr);
          if (it == fpMap.end()){
 //           cout << "This is a new address:0x"<<hex<<add->addr<<" with type "<<dec<<add->ip->type<<endl;
            map <int, int> addthis;
            addthis.insert({(*ait)->ip->type, 1});
            fpMap.insert({(*ait)->addr , addthis});
          } else {
            map <int, int>::iterator tit = it->second.find((*ait)->ip->type);
            if (tit != it->second.end()){
//            cout << "I am adding to an existing entry with addr 0x"<<hex<<add->addr<<dec<<" type:"<<add->ip->type<<endl;//" count:"<<tit->second.second<<endl;
              tit->second++;
            } else {
//            cout << "I am adding to an existing entry with addr 0x"<<hex<<add->addr<<dec<<" a new type:"<<add->ip->type<<endl;
              it->second.insert({(*ait)->ip->type, 1});
            }

//            cout << "I add addr 0x"<<hex<<add->addr<<dec<<" type:"<<add->ip->type<<endl;//" count:"<<tit->second.second<<endl;
          }
//          this->addAddress(*ait);
        }
      } else {
        //cout <<" window is not empty\n";
        map <unsigned long, map <int, int>>::iterator it = w->fpMap.begin();
      //map <unsigned long, map<int, int>::iterator it =  w->fpMap.begin();
        for (it ; it != w->fpMap.end(); it++){
//cout <<hex<< "it first:"<<it->first<<dec<<endl;
          map <unsigned long, map <int,int>>::iterator curr_it = this->fpMap.find(it->first);
          if (curr_it == this->fpMap.end()){
//            cout << "This is a new address:0x"<<hex<<it->first<<endl;//<<" with type "<<dec<<it->second.first<<endl;
            fpMap.insert({it->first, it->second});
          } else {
            map <int, int>::iterator tit = it->second.begin();
            for (tit; tit != it->second.end(); tit++){
              map <int, int>::iterator curr_tit = curr_it->second.find(tit->first);
              if (curr_tit != curr_it->second.end()){
//                cout << "I am adding to an existing entry with addr 0x"<<hex<<it->first<<dec<<" type:"<<tit->first <<" count:"<<tit->second<<endl;
                curr_tit->second+=tit->second;
              } else {
//                cout << "I am adding to an existing entry with addr 0x"<<hex<<it->first<<dec<<" a new type:"<<tit->first<<endl;
                curr_it->second.insert({tit->first, tit->second});
              }

//              cout << "I add addr 0x"<<hex<<curr_it->first<<dec<<" type:"<<curr_tit->first<<endl;//" count:"<<curr_tit->second.second<<endl;
            }
          }
        }
      } 
    }
    
    pair<unsigned long, int> Window::getWindowID () { return windowID; }
    int Window::getFP(){return fpMap.size();}


    void Window::getdiagMap (map <int,int> *typeMap, map <int, double> *fpDiagMap) {
      map <int, int>::iterator tit = typeMap->begin();
      map <int, double>::iterator did;
      double total = 0;
      double freq = 1.0; 
      for (tit;  tit != typeMap->end(); tit++){
//        cout << "Total :"<<total<< " new entry:"<<tit->second<<" type:"<<tit->first<<endl;
        total +=tit->second;
      }
//      cout << "Total input: "<<total<<endl;
      for (tit = typeMap->begin();  tit != typeMap->end(); tit++){
//        cout << " Before For type "<< tit->first <<" Count:"<<tit->second<< " freq "<<freq<<" Total:"<<total<<endl;
        freq =  ( (double)tit->second / (double)total);
//        cout << " After 1 For type "<< tit->first << " Count:"<<tit->second<< " freq "<<freq<<endl;
        did =  fpDiagMap->find(tit->first);
        if (did !=  fpDiagMap->end()){
//          cout << " 2 before fp:"<< did->second<< endl;
          did->second += freq;
//          cout << " 2 after  fp:" << did->second<<endl;
        } else { 
//          cout << "Else2  fp:"<<freq<<endl;
          fpDiagMap->insert({tit->first, freq});
        }
      }
//      cout<<endl;
    }
    
    void  Window::getFPDiag(map <int, double> *diagMap){ // this map holds fp per type
//      cout <<" getting FP diragram  FP :"<<this->fpMap.size()<<endl;
      map <unsigned long, map <int,int>>::iterator fp_it = this->fpMap.begin();
      for (fp_it; fp_it != this->fpMap.end() ;  fp_it++){
        if (fp_it->second.size() ==1){
//          cout << "I am adding fp directly since there is only one type\n";
          map <int, double >::iterator dmit =  diagMap->find(fp_it->second.begin()->first);
          if (dmit != diagMap->end()){
            dmit->second++;
          } else {
            diagMap->insert({fp_it->second.begin()->first, 1.0});
          }
        } else { 
          getdiagMap(&(fp_it->second), diagMap);    
        }
      } 
    }
    
    void Window::setParent ( Window *w){
      parent = w;
    }
    
    void Window::addRightChild (Window *w){
      right = w;
      addWindow(w);
      for (auto ait =  w->addresses.begin() ; ait != w->addresses.end(); ait++){
        addresses.push_back((*ait));  
      }
    }

    void Window::addLeftChild (Window *w){
      left = w;
      addWindow(w);
      for (auto ait =  w->addresses.begin() ; ait != w->addresses.end(); ait++){
        addresses.push_back((*ait));   
      }
    }

    int Window::getSize(){
      if (this->getConstantLds() == 0 ){
        this->calcConstantLds();
      }
      return addresses.size() + this->getConstantLds();      
    }

   float Window::getMultiplier(){ return this->multiplier; }
   //float Window::getMultiplierAvg(){ return this->multiplierAvg; }

//TODO NOTE:: change this calculate multiplier and add multiplier as a float and add get multiplier
//TODO try to get multiplier as we were doing in the beginin but per window. This can be used locally so we may have a better estimate when we are doing window view. 
//TODO Probably we can do per funtion too
    float Window::calcMultiplier(unsigned long  period, bool is_load){
      unsigned long prevTime = 0;
      unsigned long window_first_time = 0;
      int number_of_windows = 0;
      bool is_first = true;
      int current_ws =0;
      int window_size = 0;
      unsigned long window_time = 0;
      unsigned long skip_time = 0;
      unsigned long skip_size = 0;
      unsigned long current_ztime = 0;
      vector <unsigned long> Zs;
      vector <unsigned long> Zt;
      vector <unsigned long> wSize;
      vector <unsigned long> wTime;
      vector <float> wMultipliers;
      bool new_sample =  false;
      int prev_sampleID = 0;
      int total_loads_in_trace = 0;
      int total_loads_in_window = 0;


//cout << "DEBUG:: Line: " << __LINE__ << " Period: "<<period <<endl;
      for(auto it = addresses.begin(); it != addresses.end(); it++) {
        total_loads_in_window = total_loads_in_window + 1 + (*it)->ip->getExtraFrameLds();
        total_loads_in_trace = total_loads_in_trace + 1 + (*it)->ip->getExtraFrameLds();
//cout << "DEBUG:: Line: " << __LINE__ << endl;
        current_ws++;
        if (is_first){
          is_first = false;
          window_first_time = (*it)->time->time;
          prevTime = (*it)->time->time;
          prev_sampleID = (*it)->time->getSampleID();
        }
//Dividing trace regarding the sampling frequency/period
//        if (is_load){ no need for is load anymore
          if (prev_sampleID != (*it)->time->getSampleID()){
            new_sample = true;
          } else {
            new_sample = false;
          }
//        } else {
//          if (prevTime != 0 && (*it)->time->time-prevTime >= (period)){
//            new_sample =  true;
//          } else {
//            new_sample = false;
//          }
//        }
      
        
        if (new_sample){
//if (prevTime != 0 && (*it)->time->time-prevTime >= (period)){
//cout << "DEBUG:: Line: " << __LINE__ << endl;
        //calculating window size W and skip time Z
          number_of_windows++;
          window_size+=current_ws;
          wSize.push_back(current_ws);
//          cout << "Current w:"<<current_ws << " Tot w:"<<window_size << " #w:"<< number_of_windows;
          skip_time+=((*it)->time->time - prevTime);
          Zt.push_back((*it)->time->time - prevTime);
          current_ztime = ((*it)->time->time - prevTime);
//          cout << " Prev T:"<<prevTime<<" firstT:"<<window_first_time<< " wT:"<<(prevTime - window_first_time);
          unsigned long z_curr;
          if (prevTime - window_first_time){
            z_curr = (current_ws*current_ztime)/(prevTime - window_first_time);
          } else {
            z_curr = 0;
          }
          Zs.push_back(z_curr);
          window_time += prevTime - window_first_time;
          wTime.push_back(prevTime - window_first_time);
          window_first_time =  (*it)->time->time;
          wMultipliers.push_back((float)period/(float)total_loads_in_window);
//          cout <<" Current Z:"<<z_curr<<" Zt:"<<current_ztime<< " zt:"<<skip_time<<endl;
          current_ws = 1;
        }
        prevTime = (*it)->time->time;
        prev_sampleID = (*it)->time->getSampleID();
      }
//      cout << "Ws: "<<window_size<<" Tz: "<<skip_time<<" Tw: "<<window_time<<" size = "<<addresses.size()<<" #windows:"<<number_of_windows<<endl;
//     multiplier = ((double)period * (double)number_of_windows)/ (double)total_loads_in_trace;      
//     return multiplier; 


      //calculating LoadBased Multiplier versions:
      float multiplier_ld_median =0;
      float multiplier_ld_mean =0;
      float multiplier_ld_from_totals =0;
      if (number_of_windows > 0){
        std::sort(wMultipliers.begin(),wMultipliers.end());
        int size_wMultiplier  = wMultipliers.size();
        if (size_wMultiplier){
          if (size_wMultiplier%2){
            multiplier_ld_median = wMultipliers[size_wMultiplier/2];
          } else {
            multiplier_ld_median = (wMultipliers[size_wMultiplier/2] + wMultipliers[size_wMultiplier/2 -1])/2;
          }
        }
        float total_of_multipliers = 0;
        for (int index=0; index< size_wMultiplier; index++){
//          cout << " Sample Index="<< index<<" multipler="<<wMultipliers[index]<<endl;
          total_of_multipliers+=wMultipliers[index];
        }
        multiplier_ld_mean = total_of_multipliers/number_of_windows;
        multiplier_ld_from_totals = ((float)number_of_windows*(float)period) / (float)total_loads_in_trace;
      }

//TODO FIXME MAKE SURE THAT WE DONT NEED REST IF WE WANT TO USE MEAN FOR NOW I WILL BE USING AVRG
      std::sort(Zs.begin(),Zs.end());
      std::sort(Zt.begin(),Zt.end());
      std::sort(wTime.begin(),wTime.end());
      std::sort(wSize.begin(),wSize.end());
      int size_Zs =  Zs.size();
      int size_Zt =  Zt.size();
      int size_wSize =  wSize.size();
      int size_wTime =  wTime.size();
      int zTime1 = 0 , wSize1 = 0, wTime1 = 0, zSize1 = 0;
      int skip_size1 = 0;
      int skip_size2 = 0;
//      cout << "CONTROL:: size of  Zs:"<<size_Zs<<" Zt:"<<size_Zt<<" Ws:"<<size_wSize<<" Wt"<<size_wTime<<endl;
      if (size_Zs){
        if (size_Zs%2){
          skip_size = Zs[size_Zs/2];
        } else {
          skip_size = (Zs[size_Zs/2] + Zs[size_Zs/2 -1])/2;
        }
      }
      if (size_Zt){
        if (size_Zt%2){
          zTime1 = Zt[size_Zt/2];
        } else {
          zTime1 = (Zt[size_Zt/2] + Zt[size_Zt/2 -1])/2;
        }
      }
      if (size_wSize){
        if (size_wSize%2){
          wSize1 = wSize[size_wSize/2];
        } else {
          wSize1 = (wSize[size_wSize/2] + wSize[size_wSize/2 -1])/2;
        }
      }
      if (size_wTime){
        if (size_wTime%2){
          wTime1 = wTime[size_wTime/2];
        } else {
          wTime1 = (wTime[size_wTime/2] + wTime[size_wTime/2 -1])/2;
        }
      }
      if (wTime1){
        zSize1 =  (zTime1*wSize1)/wTime1;
      }
//     cout<< "CONTROL Median Zs:"<<skip_size<<" Zt:"<<zTime1<<" Ws:"<<wSize1<<" Wt:"<<wTime1<<endl;
  //    skip_size = (skip_time*window_size)/window_time;
      if (window_time && number_of_windows)
        skip_size1 = (skip_time*window_size)/window_time/number_of_windows;
      if (number_of_windows)
        window_size=window_size/number_of_windows;
      if (number_of_windows-1)
        skip_time=skip_time/(number_of_windows-1);
      if (number_of_windows)
        window_time = window_time/number_of_windows;
      if (window_time)
        skip_size2 = (skip_time*window_size)/window_time;
//      cout << "alternative Z1:"<<skip_size1 << " ZS:"<<skip_size2<<" Z3:"<<zSize1<<" Z4*:"<<skip_size<<endl;
//      if (total_window_time)
//        total_skip_size = (total_skip_time*timeVec.size())/total_window_time;

//      cout << "Do_reuse WS:" <<window_size<< " Zt:"<<skip_time << " Wt:"<<window_time<<" ZS:"<<skip_size<<endl;

//      if (window_time){
//        float z =  (float)((float)window_size * (float)skip_time)/(float)window_time;
//        window_size =  window_size/number_of_windows;
//        skip_time =  skip_time/number_of_windows;
//        window_time = window_time/number_of_windows;
//      cout << "Ws: "<<window_size<<" Tz: "<<skip_time<<" Tw: "<<window_time<<" size = "<<timeVec.size()<<" #windows:"<<number_of_windows<<" z:"<<z<<endl;
//        float multiplier = (float)(window_size+z)/(float)window_size;
//        return multiplier;
//      }
      if (wSize1){  
        this->multiplier = (float)(wSize1+skip_size)/(float)wSize1;
//        cout << "For Focused Func Local multipliers:\nold: "<<this->multiplier
//            << " MEAN: "<<multiplier_ld_mean
//            << " Median: "<<multiplier_ld_median
//            << " AVG: "<<multiplier_ld_from_totals<<endl;
        this->multiplier = multiplier_ld_from_totals;
        return this->multiplier;
      } else {
        this->multiplier = 1.0;
//        cout << "For Focused Func Local multipliers:\nold: "<<this->multiplier
//            << " MEAN: "<<multiplier_ld_mean
//            << " Median: "<<multiplier_ld_median
//            << " AVG: "<<multiplier_ld_from_totals<<endl;
        if (multiplier_ld_from_totals){
          this->multiplier = multiplier_ld_from_totals;
        }

        return this->multiplier;
      }

//      if (window_size){
//        float multiplier = (float)(window_size+skip_size)/(float)window_size;
//        return multiplier;
//      } else {
//        return 1.0;
//      }
    }

