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
#include "Function.hpp"
#include "Address.hpp"
#include "Instruction.hpp"
#include "CPU.hpp"
//***************************************************************************
using namespace std;
using namespace memgaze;


// Function::Function  (std::string _name, unsigned long _s,  unsigned long _e) { 
//   startIP = _s;
//   endIP = _e;
//   name = _name;
//   totalLoads =  0;
// }

Function::Function  (std::string _name, uint16_t _load_module, unsigned long _s,  unsigned long _e, int _ncpus) { 
  startIP = _s;
  endIP = _e;
  name = _name;
  totalLoads =  0;
  ncpus = _ncpus;
  cpuFP.reserve(ncpus);
  cpuFPMap.reserve(ncpus);
  load_module = _load_module;
  for (int i=0; i<ncpus; i++) {
    map <unsigned long, map <enum Metrics,int>> tmp;
    cpuFPMap.push_back(tmp);
    cpuFP.push_back(-1);
  }
  trace = new Trace();
}


Function::Function  (std::string _name, unsigned long _s,  unsigned long _e, int _ncpus) { 
  startIP = _s;
  endIP = _e;
  name = _name;
  totalLoads =  0;
  ncpus = _ncpus;
  cpuFP.reserve(ncpus);
  cpuFPMap.reserve(ncpus);
  for (int i=0; i<ncpus; i++) {
    map <unsigned long, map <enum Metrics,int>> tmp;
    cpuFPMap.push_back(tmp);
    cpuFP.push_back(-1);
  }
  trace = new Trace();
}

int Function::getFP(){return fp;}

void Function::calcFP(){
  map <enum Metrics,int>typemap;
  map <unsigned long, map <enum Metrics, int>>::iterator typeMapIt;
//OZGURCLEANUP  for(auto it = timeVec.begin(); it != timeVec.end(); it++) {
  for(auto it = trace->trace.begin(); it != trace->trace.end(); it++) {
    typeMapIt = this->fpMap.find((*it)->addr->addr);
    if (typeMapIt != fpMap.end()){
      map <enum Metrics, int>::iterator tit = typeMapIt->second.find((*it)->ip->type);
      if (tit != typeMapIt->second.end()){
        tit->second++;
      } else {
        typeMapIt->second.insert({(*it)->ip->type, 1});
      }
    } else {
      typemap.insert({(*it)->ip->type,  1});
      this->fpMap.insert({(*it)->addr->addr, typemap});
      typemap.clear();
    }
  }
  fp = this->fpMap.size();
}

void Function::calcCPUFP(){
//   cout << "[NVDBG]: Begin CPU FP" << endl;
//   cout << "[NVDBG]: cpuFPMap size " << cpuFPMap.size() << endl;
//   cout << "[NVDBG]: timeVec size " << timeVec.size() << endl;
  map <enum Metrics,int>typemap;
  map <unsigned long, map <enum Metrics, int>>::iterator typeMapIt;
//OZGURCLEANUP  for(auto it = timeVec.begin(); it != timeVec.end(); it++) {
  for(auto it = trace->trace.begin(); it != trace->trace.end(); it++) {
    uint16_t cpuid = (*it)->cpu->cpu;
    typeMapIt = (this->cpuFPMap[cpuid]).find((*it)->addr->addr);
    // cout << "[NVDBG]: cpuid " << cpuid << " typeMapIt " << endl;

    if (typeMapIt != cpuFPMap[cpuid].end()){
      map <enum Metrics, int>::iterator tit = typeMapIt->second.find((*it)->ip->type);
      if (tit != typeMapIt->second.end()){
        tit->second++;
      } else {
        typeMapIt->second.insert({(*it)->ip->type, 1});
      }
    } else {
      typemap.insert({(*it)->ip->type,  1});
      this->cpuFPMap[cpuid].insert({(*it)->addr->addr, typemap});
      typemap.clear();
    }
  }
//   cout << "[NVDBG]: cpuFPMap sucessfully constructed" << endl;
  for (int cpuid=0; cpuid<this->ncpus; cpuid++)
    this->cpuFP[cpuid] =  this->cpuFPMap[cpuid].size();
}

void Function::getdiagMap (map <enum Metrics,int> *typeMap, map <enum Metrics, double> *fpDiagMap) {
  map <enum Metrics, int>::iterator tit = typeMap->begin();
  map <enum Metrics, double>::iterator did;
  double total = 0;
  double freq = 1.0; 
  for (;tit != typeMap->end(); tit++){
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

void  Function::getFPDiag(map <enum Metrics, double> *diagMap){ // this map holds fp per type
  map <unsigned long, map <enum Metrics,int>>::iterator fp_it = this->fpMap.begin();
  for (; fp_it != this->fpMap.end() ;  fp_it++){
    if (fp_it->second.size() ==1){
      map <enum Metrics, double >::iterator dmit =  diagMap->find(fp_it->second.begin()->first);
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

void Function::getCPUFPDiag(map <enum Metrics, double> *cpuDiagMap, uint16_t cpuid) {
    map <unsigned long, map <enum Metrics,int>>::iterator fp_it = this->cpuFPMap[cpuid].begin();
    for (; fp_it != this->cpuFPMap[cpuid].end() ;  fp_it++){
        if (fp_it->second.size() ==1){
        map <enum Metrics, double >::iterator dmit = (cpuDiagMap)->find(fp_it->second.begin()->first);
        if (dmit != (cpuDiagMap)->end()){
            dmit->second++;
        } else {
            (cpuDiagMap)->insert({fp_it->second.begin()->first, 1.0});
        }
        } else { 
            getdiagMap(&(fp_it->second), (cpuDiagMap));    
        }
    }
}


//Trace * Function::calculateFunctionFP(){
//  return calculateFunctionFPRec(this, 0);
//}

Trace * Function::calculateFunctionFPRec(Function *root,  int level ){
    Trace * childTimeVec = new Trace();
    std::vector<Trace * > tempVec;
    if (root == NULL){
      return childTimeVec;
    } 
    for (auto it = root->children.begin(); it != root->children.end(); it++){
      Trace * t1 = calculateFunctionFPRec(*it , level+1);
      if (t1->getSize() == 0){
        cout << "ERROR child vector is empty. " << root->name << " level: " <<level<<endl;
      } else {
        tempVec.push_back(t1);
      }
    }
    map <unsigned long, int> functionFPMap; // will hold every new access 
    map <enum Metrics,int>typemap;
    map <unsigned long, map <enum Metrics, int>>::iterator typeMapIt;
    if (tempVec.empty()){
      for(auto it = root->trace->trace.begin(); it != root->trace->trace.end(); it++) {
        root->totalLoads++;
        map <unsigned long, int>::iterator fmapIter = functionFPMap.find((*it)->addr->addr);
        if (fmapIter != functionFPMap.end()){
          fmapIter->second++;
        } else {
          functionFPMap.insert({(*it)->addr->addr,1});
          childTimeVec->addAccess(*it);
        } 
        typeMapIt = root->fpMap.find((*it)->addr->addr);
        if (typeMapIt != root->fpMap.end()){
          map <enum Metrics, int>::iterator tit = typeMapIt->second.find((*it)->ip->type);
          if (tit != typeMapIt->second.end()){
            tit->second++;
          } else {
            typeMapIt->second.insert({(*it)->ip->type, 1});
          }
        } else {
          typemap.insert({(*it)->ip->type,  1});
          root->fpMap.insert({(*it)->addr->addr, typemap});
          typemap.clear();
        }
      }
    } else {
      for (auto tit = tempVec.begin(); tit  != tempVec.end(); tit++){
        for(auto it = (*tit)->trace.begin(); it != (*tit)->trace.end(); it++) {
          map <unsigned long, int>::iterator fmapIter = functionFPMap.find((*it)->addr->addr);
          if (fmapIter != functionFPMap.end()){
            fmapIter->second++;
          } else {
            functionFPMap.insert({(*it)->addr->addr,1});
            childTimeVec->addAccess(*it);
          }
          typeMapIt = root->fpMap.find((*it)->addr->addr);
          if (typeMapIt != root->fpMap.end()){
            map <enum Metrics, int>::iterator tit = typeMapIt->second.find((*it)->ip->type);
            if (tit != typeMapIt->second.end()){
              tit->second++;
            } else {
              typeMapIt->second.insert({(*it)->ip->type, 1});
            }
          } else {
            typemap.insert({(*it)->ip->type,  1});
            root->fpMap.insert({(*it)->addr->addr, typemap});
            typemap.clear();
          }
        }
      }
      for(auto it = root->trace->trace.begin(); it != root->trace->trace.end(); it++) {
        root->totalLoads++;
        map <unsigned long, int>::iterator fmapIter = functionFPMap.find((*it)->addr->addr);
        if (fmapIter != functionFPMap.end()){
          functionFPMap.insert({(*it)->addr->addr,fmapIter->second +1});
        } else {
          functionFPMap.insert({(*it)->addr->addr,1});
          childTimeVec->addAccess(*it);
        }
        typeMapIt = root->fpMap.find((*it)->addr->addr);
        if (typeMapIt != root->fpMap.end()){
          map <enum Metrics, int>::iterator tit = typeMapIt->second.find((*it)->ip->type);
          if (tit != typeMapIt->second.end()){
            tit->second++;
          } else {
            typeMapIt->second.insert({(*it)->ip->type, 1});
          }
        } else {
          typemap.insert({(*it)->ip->type,  1});
          root->fpMap.insert({(*it)->addr->addr, typemap});
          typemap.clear();
        }
      }
      for (auto it = root->children.begin(); it != root->children.end(); it++){
        root->totalLoads += (*it)->totalLoads;
      }
    }
    root->fp =  functionFPMap.size();
    map <enum Metrics, double> diagMap;
    root->getFPDiag(&diagMap);

    return childTimeVec;
}

//std::vector<AccessTime *> Function::calculateFunctionCPUFP(){
//  return calculateFunctionCPUFPRec(this, 0);
//}

Trace * Function::calculateFunctionCPUFPRec(Function *root,  int level ){
    Trace * childTimeVec =  new Trace();
    std::vector< Trace * > tempVec;
    if (root == NULL){
      return childTimeVec;
    } 
    for (auto it = root->children.begin(); it != root->children.end(); it++){
      Trace *t1 = calculateFunctionCPUFPRec(*it , level+1);
      if (t1->getSize()==0){
        cout << "ERROR child vector is empty. " << root->name << " level: " <<level<<endl;
      } else {
        tempVec.push_back(t1);
      }
    }
    // map <unsigned long, int> functionFPMap; // will hold every new access 
    map <unsigned long, int> funcCPUFPMap [root->ncpus]; // will hold every new access

    map <enum Metrics,int>typemap;
    map <unsigned long, map <enum Metrics, int>>::iterator typeMapIt;
    if (tempVec.empty()){
      for(auto it = root->trace->trace.begin(); it != root->trace->trace.end(); it++) {
        int cpuid = (*it)->cpu->cpu;
        root->totalLoads++;
        map <unsigned long, int>::iterator fmapIter = funcCPUFPMap[cpuid].find((*it)->addr->addr);
        if (fmapIter != funcCPUFPMap[cpuid].end()){
          fmapIter->second++;
        } else {
          funcCPUFPMap[cpuid].insert({(*it)->addr->addr,1});
          childTimeVec->addAccess(*it);
        } 
        typeMapIt = root->cpuFPMap[cpuid].find((*it)->addr->addr);
        if (typeMapIt != root->cpuFPMap[cpuid].end()){
          map <enum Metrics, int>::iterator tit = typeMapIt->second.find((*it)->ip->type);
          if (tit != typeMapIt->second.end()){
            tit->second++;
          } else {
            typeMapIt->second.insert({(*it)->ip->type, 1});
          }
        } else {
          typemap.insert({(*it)->ip->type,  1});
          root->cpuFPMap[cpuid].insert({(*it)->addr->addr, typemap});
          typemap.clear();
        }
      }
    } else {
      for (auto tit = tempVec.begin(); tit  != tempVec.end(); tit++){
        for(auto it = (*tit)->trace.begin(); it != (*tit)->trace.end(); it++) {
          int cpuid = (*it)->cpu->cpu;
          map <unsigned long, int>::iterator fmapIter = funcCPUFPMap[cpuid].find((*it)->addr->addr);
          if (fmapIter != funcCPUFPMap[cpuid].end()){
            fmapIter->second++;
          } else {
            funcCPUFPMap[cpuid].insert({(*it)->addr->addr,1});
            childTimeVec->addAccess(*it);
          }
          typeMapIt = root->cpuFPMap[cpuid].find((*it)->addr->addr);
          if (typeMapIt != root->cpuFPMap[cpuid].end()){
            map <enum Metrics, int>::iterator tit = typeMapIt->second.find((*it)->ip->type);
            if (tit != typeMapIt->second.end()){
              tit->second++;
            } else {
              typeMapIt->second.insert({(*it)->ip->type, 1});
            }
          } else {
            typemap.insert({(*it)->ip->type,  1});
            root->cpuFPMap[cpuid].insert({(*it)->addr->addr, typemap});
            typemap.clear();
          }
        }
      }
      for(auto it = root->trace->trace.begin(); it != root->trace->trace.end(); it++) {
        int cpuid = (*it)->cpu->cpu;
        root->totalLoads++;
        map <unsigned long, int>::iterator fmapIter = funcCPUFPMap[cpuid].find((*it)->addr->addr);
        if (fmapIter != funcCPUFPMap[cpuid].end()){
          funcCPUFPMap[cpuid].insert({(*it)->addr->addr,fmapIter->second +1});
        } else {
          funcCPUFPMap[cpuid].insert({(*it)->addr->addr,1});
          childTimeVec->addAccess(*it);
        }
        typeMapIt = root->cpuFPMap[cpuid].find((*it)->addr->addr);
        if (typeMapIt != root->cpuFPMap[cpuid].end()){
          map <enum Metrics, int>::iterator tit = typeMapIt->second.find((*it)->ip->type);
          if (tit != typeMapIt->second.end()){
            tit->second++;
          } else {
            typeMapIt->second.insert({(*it)->ip->type, 1});
          }
        } else {
          typemap.insert({(*it)->ip->type,  1});
          root->cpuFPMap[cpuid].insert({(*it)->addr->addr, typemap});
          typemap.clear();
        }
      }
      for (auto it = root->children.begin(); it != root->children.end(); it++){
        root->totalLoads += (*it)->totalLoads;
      }
    }
    // for (auto it= funcCPUFPMap->begin(); it != funcCPUFPMap->end(); it++) {
    //     int cpuid = (*it)->cpu->cpu;
    //     root->cpuFP[cpuid] =  funcCPUFPMap[cpuid].size();
    // }
    // map <int, double> diagMap;
    // root->getFPDiag(&diagMap);

    return childTimeVec;
}

void  Function::printFunctionTree(){
   printFunctionTreeRec(this, 0);
}
void  Function::printFunctionTreeRec(Function *root, int level ){
    if (root == NULL){
      return;
    }
    for (int i =0; i < level ; i++){
      cout << "\t" ;
    }
    cout << root->name << " StartIP:"<<hex<<root->startIP<<dec << " FP: "<<root->fp << " lds: "<<root->totalLoads ;
    map <enum Metrics, double> diagMap;
    root->getFPDiag(&diagMap);
    cout<<" Strided: "<<diagMap[STRIDED]<<" Indirect: "<<diagMap[INDIRECT]<<" Constant: "<<diagMap[CONSTANT]<<" Unknown: "<<diagMap[UNKNOWN]<<endl;
    for (int i =0; i < level ; i++){
      cout << "\t" ;
    }
    cout<<"  GrowtRate for Strided: "<<diagMap[STRIDED]/root->totalLoads<<" Indirect: "<<diagMap[INDIRECT]/root->totalLoads<<" Constant: "<<diagMap[CONSTANT]/root->totalLoads<<" Unknown: "<<diagMap[UNKNOWN]/root->totalLoads<<endl;
    for (auto it = root->children.begin(); it != root->children.end(); it++){
      printFunctionTreeRec(*it , level+1);
    }
}

//TODO try to get multiplier as we were doing in the beginin but per window. This can be used locally so we may have a better estimate when we are doing window view. 
//TODO Probably we can do per funtion too
    float Function::getMultiplier(unsigned long  period, bool  is_load){
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
      vector <double> wMultipliers;
      bool new_sample =  false;
      uint32_t prev_sampleID = 0; 
      int total_loads_in_trace = 0;
      int total_loads_in_window = 0;


      for(auto it = trace->trace.begin(); it != trace->trace.end(); it++) {
        total_loads_in_trace = total_loads_in_trace + 1 + (*it)->ip->getExtraFrameLds();
        total_loads_in_window = total_loads_in_window + 1 + (*it)->ip->getExtraFrameLds();
        current_ws++;
        if (is_first){
          is_first = false;
          window_first_time = (*it)->time->time;
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
           skip_time+=((*it)->time->time-prevTime);
           Zt.push_back((*it)->time->time-prevTime);
           current_ztime = ((*it)->time->time-prevTime);
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
           wMultipliers.push_back((double)period/(double)total_loads_in_window);
           total_loads_in_window = 0;

           current_ws = 0;
          }
        prevTime = (*it)->time->time;
        prev_sampleID =  (*it)->time->getSampleID();
      }

      //calculating LoadBased Multiplier versions:
      //double multiplier_ld_median =0;
      //double multiplier_ld_mean =0;
      double multiplier_ld_from_totals =0;
      if (number_of_windows > 0){
        std::sort(wMultipliers.begin(),wMultipliers.end());
        int size_wMultiplier  = wMultipliers.size();
        double total_of_multipliers = 0;
        for (int index=0; index< size_wMultiplier; index++){
          total_of_multipliers+=wMultipliers[index];
        }
        multiplier_ld_from_totals = ((double)number_of_windows*(double)period) / (double)total_loads_in_trace;
      }
      

      std::sort(Zs.begin(),Zs.end());
      std::sort(Zt.begin(),Zt.end());
      std::sort(wTime.begin(),wTime.end());
      std::sort(wSize.begin(),wSize.end());
      int size_Zs =  Zs.size();
      int size_Zt =  Zt.size();
      int size_wSize =  wSize.size();
      int size_wTime =  wTime.size();
      int wSize1 = 0;
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
        float multiplier = (float)(wSize1+skip_size)/(float)wSize1;

        return multiplier_ld_from_totals;
      } else {
        if (multiplier_ld_from_totals)
          return multiplier_ld_from_totals;
        return 1.0;
      }
  }

