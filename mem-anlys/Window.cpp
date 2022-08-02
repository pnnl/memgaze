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
#include "metrics.hpp"
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
    }
    
    void Window::setFuncName( std::string _name ) { funcName = _name;}
    void Window::setFuncName( ) { 
      map <string , map <unsigned long, int>> localFuncFPMap;
      string name = "";
      for (auto ait=this->addresses.begin(); ait != this->addresses.end(); ait++){
        name =  (*ait)->getFuncName();
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
      unsigned int maxFP = 0;
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
    void Window::setMtime( unsigned long _mtime ) { mtime = _mtime;}
    void Window::calcTime(){
    	int size = addresses.size();
	int mid = size/2;
	this->setStime(addresses[0]->time->time);
	this->setMtime(addresses[mid]->time->time);
	this->setEtime(addresses[size-1]->time->time);
    }
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
      map <unsigned long, map <int, int>>::iterator it = fpMap.find(add->addr);
      if (it == fpMap.end()){
        map <int, int> addthis;
        addthis.insert({add->ip->type, 1});
        if (add->ip->getExtraFrameLds() >0){
          addthis.insert({0,add->ip->getExtraFrameLds()});
        }
        fpMap.insert({add->addr , addthis});
      } else {
        map <int, int>::iterator tit = it->second.find(add->ip->type);
        if (tit != it->second.end()){
          tit->second++;
        } else {
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

      }
    }
    
    void Window::addWindow(Window * w){
      if (this->fpMap.empty()){
        for (auto ait=this->addresses.begin(); ait != this->addresses.end(); ait++){
          map <unsigned long, map <int, int>>::iterator it = this->fpMap.find((*ait)->addr);
          if (it == fpMap.end()){
            map <int, int> addthis;
            addthis.insert({(*ait)->ip->type, 1});
            fpMap.insert({(*ait)->addr , addthis});
          } else {
            map <int, int>::iterator tit = it->second.find((*ait)->ip->type);
            if (tit != it->second.end()){
              tit->second++;
            } else {
              it->second.insert({(*ait)->ip->type, 1});
            }

          }
        }
        for (auto ait=w->addresses.begin(); ait != w->addresses.end(); ait++){
          map <unsigned long, map <int, int>>::iterator it = this->fpMap.find((*ait)->addr);
          if (it == fpMap.end()){
            map <int, int> addthis;
            addthis.insert({(*ait)->ip->type, 1});
            fpMap.insert({(*ait)->addr , addthis});
          } else {
            map <int, int>::iterator tit = it->second.find((*ait)->ip->type);
            if (tit != it->second.end()){
              tit->second++;
            } else {
              it->second.insert({(*ait)->ip->type, 1});
            }

          }
        }
      } else {
        map <unsigned long, map <int, int>>::iterator it = w->fpMap.begin();
        for ( ; it != w->fpMap.end(); it++){
          map <unsigned long, map <int,int>>::iterator curr_it = this->fpMap.find(it->first);
          if (curr_it == this->fpMap.end()){
            fpMap.insert({it->first, it->second});
          } else {
            map <int, int>::iterator tit = it->second.begin();
            for (; tit != it->second.end(); tit++){
              map <int, int>::iterator curr_tit = curr_it->second.find(tit->first);
              if (curr_tit != curr_it->second.end()){
                curr_tit->second+=tit->second;
              } else {
                curr_it->second.insert({tit->first, tit->second});
              }
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
      for (;  tit != typeMap->end(); tit++){
        total +=tit->second;
      }
      for (tit = typeMap->begin();  tit != typeMap->end(); tit++){
        freq =  ( (double)tit->second / (double)total);
        did =  fpDiagMap->find(tit->first);
        if (did !=  fpDiagMap->end()){
          did->second += freq;
        } else { 
          fpDiagMap->insert({tit->first, freq});
        }
      }
    }
    
    void  Window::getFPDiag(map <int, double> *diagMap){ // this map holds fp per type
      map <unsigned long, map <int,int>>::iterator fp_it = this->fpMap.begin();
      for (; fp_it != this->fpMap.end() ;  fp_it++){
        if (fp_it->second.size() ==1){
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


      for(auto it = addresses.begin(); it != addresses.end(); it++) {
        total_loads_in_window = total_loads_in_window + 1 + (*it)->ip->getExtraFrameLds();
        total_loads_in_trace = total_loads_in_trace + 1 + (*it)->ip->getExtraFrameLds();
        current_ws++;
        if (is_first){
          is_first = false;
          window_first_time = (*it)->time->time;
          prevTime = (*it)->time->time;
          prev_sampleID = (*it)->time->getSampleID();
        }
//Dividing trace regarding the sampling frequency/period
          if (prev_sampleID != (*it)->time->getSampleID()){
            new_sample = true;
          } else {
            new_sample = false;
          }
        
        if (new_sample){
        //calculating window size W and skip time Z
          number_of_windows++;
          window_size+=current_ws;
          wSize.push_back(current_ws);
          skip_time+=((*it)->time->time - prevTime);
          Zt.push_back((*it)->time->time - prevTime);
          current_ztime = ((*it)->time->time - prevTime);
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
          current_ws = 1;
        }
        prevTime = (*it)->time->time;
        prev_sampleID = (*it)->time->getSampleID();
      }

      //calculating LoadBased Multiplier versions:
      float multiplier_ld_from_totals =0;
      if (number_of_windows > 0){
        std::sort(wMultipliers.begin(),wMultipliers.end());
        int size_wMultiplier  = wMultipliers.size();
        float total_of_multipliers = 0;
        for (int index=0; index< size_wMultiplier; index++){
          total_of_multipliers+=wMultipliers[index];
        }
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
      int  wSize1 = 0;
      if (size_Zs){
        if (size_Zs%2){
          skip_size = Zs[size_Zs/2];
        } else {
          skip_size = (Zs[size_Zs/2] + Zs[size_Zs/2 -1])/2;
        }
      }
      if (size_wSize){
        if (size_wSize%2){
          wSize1 = wSize[size_wSize/2];
        } else {
          wSize1 = (wSize[size_wSize/2] + wSize[size_wSize/2 -1])/2;
        }
      }
      if (number_of_windows)
        window_size=window_size/number_of_windows;
      if (number_of_windows-1)
        skip_time=skip_time/(number_of_windows-1);
      if (number_of_windows)
        window_time = window_time/number_of_windows;

      if (wSize1){  
        this->multiplier = (float)(wSize1+skip_size)/(float)wSize1;
        this->multiplier = multiplier_ld_from_totals;
        return this->multiplier;
      } else {
        this->multiplier = 1.0;
        if (multiplier_ld_from_totals){
          this->multiplier = multiplier_ld_from_totals;
        }

        return this->multiplier;
      }
    }

    map<int, double> Window::getMetrics(map<int, double> diagMap) {
      map<int, double> metrics;
      float multiplier = this->getMultiplier();
      if (diagMap.empty()) {
        this->getFPDiag(&diagMap);
      }

      diagMap[FP] = this->getFP();
      diagMap[WINDOW_SIZE] = this->getSize();
      metrics[WINDOW_SIZE] = diagMap[WINDOW_SIZE];

      if (diagMap[IN_SAMPLE] == 0) {
        metrics[FP] = diagMap[FP] * multiplier;
        metrics[STRIDED] = diagMap[STRIDED] * multiplier;
        metrics[INDIRECT] = diagMap[INDIRECT] * multiplier;
        metrics[CONSTANT] = diagMap[CONSTANT] * multiplier;
        //metrics[UNKNOWN] = diagMap[UNKNOWN] * multiplier;
        metrics[TOTAL_LOADS] = diagMap[WINDOW_SIZE] * multiplier; 
      }
      else {
        metrics[FP] = diagMap[FP];
        metrics[STRIDED] = diagMap[STRIDED];
        metrics[INDIRECT] = diagMap[INDIRECT];
        metrics[CONSTANT] = diagMap[CONSTANT];
        //metrics[UNKNOWN] = diagMap[UNKNOWN];
        metrics[TOTAL_LOADS] = diagMap[WINDOW_SIZE];
      }
      metrics[CONSTANT2LOAD_RATIO] = (diagMap[CONSTANT] * multiplier) / ((diagMap[WINDOW_SIZE] + diagMap[CONSTANT]) * multiplier);
      metrics[NPF_RATE] = diagMap[INDIRECT] / diagMap[FP];
      metrics[NPF_GROWTH_RATE] = (diagMap[INDIRECT] * multiplier) / (diagMap[WINDOW_SIZE] * multiplier);
      metrics[GROWTH_RATE] = (diagMap[FP] * multiplier) / (diagMap[WINDOW_SIZE] * multiplier);
      return metrics;
    }
