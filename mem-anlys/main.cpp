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

#include <cassert> // FIXME:tallent replace with hpctoolkit version

#include <fstream>
#include <regex>
#include <unordered_set>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <iterator>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <math.h>
#include <list>
//***************************************************************************
#include "CPU.hpp"
#include "AccessTime.hpp"
#include "Instruction.hpp"
#include "Window.hpp"
#include "Address.hpp"
#include "Function.hpp"
#include "metrics.hpp"

#ifdef DEVELOP
#include "MemgazeSource.hpp"
#endif
//***************************************************************************
std::ostream ofs(NULL);
using namespace std;

//Command Parser class
class CmdOptionParser{
    public:
        CmdOptionParser (int &argc, char **argv){
            for (int i=1; i < argc; ++i)
                this->tokens.push_back(std::string(argv[i]));
        }
        const std::string& getCmdOption(const std::string &option) const{
            std::vector<std::string>::const_iterator itr;
            itr =  std::find(this->tokens.begin(), this->tokens.end(), option);
            if (itr != this->tokens.end() && ++itr != this->tokens.end()){
                return *itr;
            }
            static const std::string empty_string("");
            return empty_string;
        }
        bool cmdOptionExists(const std::string &option) const{
            return std::find(this->tokens.begin(), this->tokens.end(), option)
                   != this->tokens.end();
        }
    private:
        std::vector <std::string> tokens;
};

//Set this one for debuging and 0 for normal run
bool debugging_enabled = 0;

#if debugging_enabled
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

//Split fucntion to read the input file
template <class Container>
void split(const std::string& str, Container& cont, char delim = ' ')
{
  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, delim)) {
    cont.push_back(token);
  }
}

// Following what will be in treeFPavgMap we can add new types and 
//map <int, <int, double>> *treeFPavgMap 
//map <level , <type , FP> 
//type 0 = constant, 1 = strided,  2 = insdirect,  -1 = unknown
//type 3 =  totalFP, 4 = growth rate, -2 number of nodes
//#define NUMBER_OF_NODES -2
//#define UNKNOWN -1
//#define CONSTANT 0
//#define STRIDED 1
//#define INDIRECT 2
//#define FP 3
//#define IN_SAMPLE -3
//#define WINDOW_SIZE -4

int getKey(int size){
  return (int)log2((double)size);
}


//Print Binary Tree with setting TreeWindowMap
void printTree (Window * root,  int period ,int lvl, map <int, map<int, double>> *treeFPavgMap, bool in_sample = false , bool is_load=false){
  map <int, double> diagMap; 
  float modifier = 1;
  if (root==NULL){
    return;
  } else {
    string dso = root->addresses[0]->ip->getDSOName();
//    cout << "Window: "<<root->windowID.first<<":"<<root->windowID.second<<" DSO: "<<dso<<endl;
    modifier =  root->calcMultiplier(period, is_load);
    float org_modifier = modifier;
    root->getFPDiag(&diagMap);
    unsigned long constant_lds = root->calcConstantLds();
    diagMap[CONSTANT] = 0;
    root->windowID.second = lvl;
    
    int size  = root->getSize();
    int key =  getKey(size);
    if (key < 3){
      key = 3;
    }
    if (size > pow(2, key)){
      key++;
    }
    if (in_sample){
      auto node =  treeFPavgMap->find(key);
      if (node != treeFPavgMap->end()){
        node->second[NUMBER_OF_NODES] += 1;
        node->second[FP] += root->getFP();
        node->second[UNKNOWN] += diagMap[UNKNOWN];
        node->second[CONSTANT] += constant_lds;
        node->second[STRIDED] += diagMap[STRIDED];
        node->second[INDIRECT] += diagMap[INDIRECT];
        node->second[WINDOW_SIZE] += root->getSize();
        node->second[IN_SAMPLE] = 1;
      } else {
        diagMap[CONSTANT]= constant_lds;
        diagMap.insert( {NUMBER_OF_NODES, 1});
        diagMap.insert( {IN_SAMPLE, 1});
        diagMap.insert( {WINDOW_SIZE, root->getSize()});
        diagMap.insert({FP, root->getFP()});
        treeFPavgMap->insert({key, diagMap});
      }
      
    } else {
      auto node =  treeFPavgMap->find(key);
      if (node != treeFPavgMap->end()){
        node->second[NUMBER_OF_NODES] += 1;
        node->second[FP] += root->getFP();
        node->second[UNKNOWN] += diagMap[UNKNOWN];
        node->second[CONSTANT] += constant_lds;
        node->second[STRIDED] += diagMap[STRIDED];
        node->second[INDIRECT] += diagMap[INDIRECT];
        node->second[WINDOW_SIZE] += root->getSize();
        node->second[IN_SAMPLE] = 0;
      } else {
        diagMap[CONSTANT]= constant_lds;
        diagMap.insert( {NUMBER_OF_NODES,1});
        diagMap.insert( {IN_SAMPLE, 0});
        diagMap.insert( {WINDOW_SIZE, root->getSize()});
        diagMap.insert({FP, root->getFP()});
        treeFPavgMap->insert({key, diagMap});
      }
    }
    lvl++;
    if(root->sampleHead || in_sample){
      printTree (root->left, period, lvl, treeFPavgMap, true, is_load);
      printTree (root->right, period, lvl, treeFPavgMap , true, is_load);
    } else {
      printTree (root->left, period, lvl, treeFPavgMap, false, is_load);
      printTree (root->right, period, lvl, treeFPavgMap, false, is_load);
    }
  }
}

//building the sample tree
Window * buildTree( vector <Window *> * windows){ 
  map <int, double> diagMap;
  map <int, double> diagMapTemp;

  if (windows->size() == 1){
    auto it = windows->begin();
    return (*it);
  }
  
  if (windows->size() < 1 || windows == NULL){
    return NULL;
  }
  vector <Window *> n_windows;
  Window * newWindow;
  pair<unsigned long, int> windowID;
  int lvl;
  
  if (windows->size() == 2){ 
    vector <Window *>::iterator  it = windows->begin();
    vector <Window *>::iterator sit = (it + 1);
    if (sit == windows->end() || it == windows->end()) {
      cout << "!!!ERROR at least one window is missing with only 2 left\n";
      return NULL;
    }
    lvl = (*it)->getWindowID().second;
    if (lvl < (*sit)->getWindowID().second){
      lvl = (*sit)->getWindowID().second;
    }

    newWindow = new Window();
    newWindow->addLeftChild((*it));
    newWindow->addRightChild((*sit));
    windowID = (*it)->getWindowID();
    windowID.second =  lvl+1;
    newWindow->setID(windowID);
    (*it)->setParent (newWindow);
    (*sit)->setParent (newWindow);
    newWindow->setFuncName();
    if (newWindow == NULL){                       // based on max FP or trace
      cout << "Returning a NULL window\n";
    }  
    return newWindow;
  } 
  for(auto it = windows->begin(); it != windows->end(); it++) {
    vector <Window *>::iterator sit = (it + 1);
    if (sit != windows->end()) {
      lvl = (*it)->getWindowID().second;
      if (lvl < (*sit)->getWindowID().second){
        lvl = (*sit)->getWindowID().second;
      }

      newWindow = new Window();
      newWindow->addLeftChild((*it));
      newWindow->addRightChild((*sit));

      windowID = (*it)->getWindowID();
      windowID.second = lvl+1;
      newWindow->setID(windowID);
      newWindow->setFuncName();

      (*it)->setParent (newWindow);
      (*sit)->setParent (newWindow);
      n_windows.push_back(newWindow);
      it++; 
    } else {
      //create a new window only change windowID lvl
      newWindow =  nullptr;
      newWindow = new Window();
      newWindow->addLeftChild((*it));
      windowID = (*it)->getWindowID();
      lvl = (*it)->getWindowID().second;
      windowID.second = lvl+1;
      newWindow->setID(windowID);
      newWindow->setFuncName();

      n_windows.push_back(newWindow);
    }
  }
  return buildTree(&n_windows);
}

//////

int main(int argc, char* argv[], const char* envp[]) {
  // -------------------------------------------------------
  // Parse command line
  // -------------------------------------------------------

  //Setting Option Parser
  CmdOptionParser opps(argc, argv);
  if (opps.cmdOptionExists("-h")){
    cout << "-t Trace File\n-l Load Classification File\n-s hpcstruct File\n-o Graph Output File\n-m Mode o for time based and 1 for load based\n-p Period\n-f Focus Function Name\n-b block size mask def 0xffffffffffff\n-c CallPath File\n-d detailed function view\n-h Help"<<endl;
    return 1;
  }

  //declaring required data structures
  Address *addr;
  CPU *cpu;
  Instruction *ip;
  AccessTime *time;
  Window * sampleWindow;
  int type = -1;
  std::string maskStr(opps.getCmdOption("-b"));
  unsigned long mask = strtoll(opps.getCmdOption("-b").c_str(), NULL, 16);
  if (!opps.cmdOptionExists("-b")){
    mask = 0; //shift bits
  }
  bool do_reuse =  false;
  bool do_focus =  false;
  unsigned long period = 3000000000;
  vector <unsigned long> Zs;
  vector <unsigned long> Zt;
  vector <unsigned long> wSize;
  vector <unsigned long> wTime;
  vector <double> wMultipliers;

  //V1:createing the vectors/Maps
  vector<AccessTime *> timeVec;
  vector<Window *> sampleVec;
  vector<Window *> funcSampleVec;
  vector<AccessTime *> funcVec;//Will hold important function's trace
  //map<string, vector<AccessTime *> funcVec;//Will hold all important functions' trace
  map<unsigned long ,  int> ipTypeMap;
  map <int,  map <int, double>> treeFPavgMap;
  map <int,  map <int, double>> treeFPavgMap2;
  
  int sampleID = 0,  in_sampleID = 0, prev_sampleID = 0;
  int in_dso_id = -1;
  string dso_name = "";
//New Mode to read input comands  
  string  inputFile = opps.getCmdOption("-t");
  string  callGraphFileName = opps.getCmdOption("-c");
//  string callGraphFileName = inputFile+std::string("_callGraph");
  string classificationInputFile = opps.getCmdOption("-l");
  string hpcStructInputFile = opps.getCmdOption("-s");
  string outputFile = opps.getCmdOption("-o");
  int is_detailed = strtol(opps.getCmdOption("-d").c_str(),NULL,10);
  //int is_load = strtol(opps.getCmdOption("-m").c_str(),NULL,10);
  int is_load = 0;
  int is_LDLAT = 0;
  string mode = opps.getCmdOption("-m");
  if (mode == "pt-load"){
    is_load = 1;
  } else if (mode == "ldlat"){
    is_LDLAT = 1;
    is_load = 0;
  }
  unsigned long startIP =0 ;
  vector <unsigned long> endIPs ;
  period  =  strtol(opps.getCmdOption("-p").c_str(), NULL, 10);
  float multiplier = (float)period;
  do_reuse = true;
  string functionName = opps.getCmdOption("-f");
  if (opps.cmdOptionExists("-f")){
    do_focus = true;
  }
  bool do_lc = false;
  if (opps.cmdOptionExists("-l")){
    do_lc = true;
  }
  bool do_hpsctruct = false;
  if (opps.cmdOptionExists("-s")){
    do_hpsctruct =  true;
  }
  bool do_output = false;
  if (opps.cmdOptionExists("-o")){
    do_output =  true; 
  }



  printf( "Trace InputFile: %s Classication inputFile: %s outputFile: %s\n" ,inputFile.c_str(), classificationInputFile.c_str(), outputFile.c_str());
  
  fstream outFile, inFile, classInFile, structInFile, cgFile;
  inFile.open(inputFile, ios::in);
  cgFile.open(callGraphFileName, ios::in);
  classInFile.open(classificationInputFile, ios::in);
  structInFile.open(hpcStructInputFile, ios::in);
  outFile.open(outputFile, ios::out);
  string line;


//NATHAN_B
  //Creating Call graph map
  //Call graph map  < key, Value>
  //               < sample id , list<functions>
  // list of function ordered based on call order 
  // Example   if f1 -> f2 -> f3 is the call order
  //       list {f1, f2, f3}
  string cg_delim = " :*: ";
  size_t delim_pos = 0;
  string cg_token;

  list<string> callPath;
  list<string>::iterator callPathIter;

  string in_cg_name = "";
  int in_cg_sample_id = 0, cg_sample_id_last= -1;

  // ------------------------------------------------------------
  // Convert call-path data into Calling Context Tree
  // ------------------------------------------------------------

  if (cgFile.is_open()) {

    while (getline(cgFile, line)) {

      // -------------------------------------------------------
      // Parse single line (Call path is multiple lines)
      // -------------------------------------------------------
      int token_cnt = 1;
      while ((delim_pos = line.find(cg_delim)) != std::string::npos) {
        cg_token = line.substr(0, delim_pos);
        if (token_cnt == 2) {
          in_cg_name = cg_token;
//          cout << "1=TOKEN::"<<cg_token<<endl;
        } 
        line.erase(0, delim_pos + cg_delim.length());
        token_cnt ++;
      }

      assert(token_cnt == 3);
      
      stringstream cg_id_ss(line);
      cg_id_ss >> in_cg_sample_id;
//      cout << "2.2TOKEN::"<<line<<endl;


      // -------------------------------------------------------
      // Insert into 'callPath'
      // -------------------------------------------------------

      cout << "LAST id: "<<cg_sample_id_last << " CURRENT: "<<in_cg_sample_id<< endl;

      // -------------------------------
      // case 1: new call path
      // -------------------------------
      if (cg_sample_id_last != in_cg_sample_id) {

        // Prior call path is now complete
        if (!callPath.empty()){
          cout << "Call path for sample "<<in_cg_sample_id<<endl;
          for (auto it = callPath.begin(); it != callPath.end(); it++){
            cout << "\t" <<*it<<endl; 
          }
        }

        // Begin new call path
        cg_sample_id_last = in_cg_sample_id;
        callPath.clear();
        callPath.push_front(in_cg_name);
      }
      // -------------------------------
      // case 2: new frame for continuing current call path
      // -------------------------------
      else {
        callPath.push_front(in_cg_name);
      }
    }
    // -------------------------------------------------------
    // FIXED: print last path
    // -------------------------------------------------------
    cout << "Call path for last sample "<<in_cg_sample_id<<endl;
    for (auto it = callPath.begin(); it != callPath.end(); it++){
      cout << "\t" <<*it<<endl; 
    }    
  }
//End of Call graph reader
//NATHAN_E


  // Here we are reading hpcstruct file  to create function bounds
  unsigned long in_ip, in_addr, in_time;
  unsigned long in_cpu;
  map <unsigned long , memgaze::Function *> funcMAP; //This will hold start IP to function map
                                            //This map is only to know the bounds 
  unsigned long func_start = 0, func_end = 0;
  string func_name;
  memgaze::Function *func;
  bool in_section = false;
  std::string name_deli_s = "n=\"";
  std::string addr_deli_s = "v=\"{[";
  std::string addr_deli_e = "-0x";
  std::string name_deli_e = "\" ln=\"";
  size_t spos = 0, epos = 0;

//XML READER
  if (do_hpsctruct){
    if (structInFile.is_open()){
      while(getline(structInFile, line)) {
        if (line.find("<HPCToolkitStructure") != std::string::npos){
          in_section=true;
          continue;
        }
        if (line.find("</HPCToolkitStructure") != std::string::npos){
          in_section=false;
          continue;
        }
        if (in_section){
          if (line.find("<P") != std::string::npos){
            std::string name_token = line.substr(line.find(name_deli_s) +3, line.find(name_deli_e)-(line.find(name_deli_s) +3));
            std::string addres_token = line.substr(line.find(addr_deli_s) +5, line.find(addr_deli_e)-(line.find(addr_deli_s) +5));
            std::istringstream ss(addres_token);
            ss >> hex >> func_start;
            if(!funcMAP.empty()){
              map <unsigned long , memgaze::Function *>::iterator zz = funcMAP.end();
              zz --;
              zz->second->endIP = func_start - 1;
            }
            func = new memgaze::Function(name_token ,  func_start);
            funcMAP.insert({func_start, func});
          }
        }
      }
    }
  }
  

  map <unsigned long , memgaze::Function *>::iterator mapEndit = funcMAP.end();
  if (do_hpsctruct){
    mapEndit --;
    mapEndit->second->endIP = 0x7FFFFFFFFFFF;
  }
  map <unsigned long, int> frameLdsMap;
  int number_of_lds = 0;
//Reading load classification file  to build ipTypeMap
  if (do_lc){
    if (classInFile.is_open()) {  
      while(getline(classInFile, line)) { 
        std::vector<std::string> celements;
        split(line, celements);
        std::istringstream ss_cip(celements[0]);
        ss_cip >> std::hex >> in_ip;
        std::istringstream ss_type(celements[1]);
        ss_type >> std::hex >> type;
        ipTypeMap.insert({in_ip , type});
        std::istringstream ss_frame_lds(celements[4]);
        ss_frame_lds >> std::hex >> number_of_lds;
        frameLdsMap.insert({in_ip , number_of_lds});
      }
    }
  } 
  line="";
 
  map<unsigned long , pair <unsigned long, unsigned long>> reuseMap;
  map<unsigned long , pair <unsigned long, unsigned long>>::iterator reuseMapIter;
  unsigned long prevTime = 0 , index = 0, reuse = -1, prevIndex = 0;
  int func_last_index = 0, func_index=0;
  bool is_in_func = false;
  int totalWindow = 0, importantWindows=0;
  int missing_frame_loads = 0;
  int trace_size = 0;
  int func_found = 0 , func_not_found= 0 ;
  int loads_from_removed_samples = 0;
  Instruction * ip_to_add = NULL;
  int control = 0;
  map <int, string> dsoMap;
  bool isDSO = false;
  bool isTrace = false;

  if(inFile.is_open()){
    while(getline(inFile, line)){
      if (line.find("DSO:") != std::string::npos){
        isDSO = true;
        isTrace = false;
        continue;
      } else if (line.find("TRACE:") != std::string::npos){
        isDSO = false;
        isTrace = true;
        continue;
      }
      if (isDSO){
        std::vector<std::string> dso_elems;
        split(line, dso_elems);
        int dsoID1 = stoi(dso_elems[1]);
        string dsoName1 = dso_elems[0];
        dsoMap.insert({dsoID1, dsoName1});
      }

      if (isTrace){ 
        trace_size++;
        reuse = -1;
        std::vector<std::string> elements;
        split(line, elements);
        std::istringstream ss_ip(elements[0]);
        ss_ip >> std::hex >> in_ip;
        map<unsigned long ,  int>::iterator tit = ipTypeMap.find(in_ip);
        if(tit != ipTypeMap.end()){
          type = tit->second;
        } else {
  //FIXME OPEN THIS ERROR        cout << ">>>>>>>> IP for unknown load type is "<< hex<<in_ip<<dec << endl;
          type = -1;
        }
        std::istringstream ss_addr(elements[1]);
        ss_addr >> hex >> in_addr;
       
        in_time = stold(elements[3]) * 1000000000;
        in_cpu = stold(elements[2]);
        if (elements.size()>4){
          in_sampleID = stoi(elements[4]);
          in_dso_id = stoi(elements[5]);
        } else {
          in_sampleID =  0 ;
        }
        dso_name = dsoMap[in_dso_id];

  //TODO exclude the frame loads and move their frm load exxtras to the next entry
        map<unsigned long, int>::iterator frameMapIter = frameLdsMap.find(in_ip);
        int xtr_lds = 0;
        if (frameMapIter != frameLdsMap.end()){
          xtr_lds = frameMapIter->second;
        } else {
          xtr_lds = 0;
        }

        if (type == 0){
  //        cout << "TYPE 0 before Missing frame loads: "<<missing_frame_loads<< " Extra:"<<xtr_lds<<endl;
          if( prev_sampleID != 0 && prev_sampleID != in_sampleID){
            if (ip_to_add != NULL){
              ip_to_add->setExtraFrameLds(ip_to_add->getExtraFrameLds()+missing_frame_loads);
              //TODO add missing loads to ip_to_add;
              ip_to_add = NULL;
            }
            loads_from_removed_samples += missing_frame_loads;
            missing_frame_loads =  xtr_lds + 1;
          }else{
            missing_frame_loads = missing_frame_loads + xtr_lds + 1;
          }
  //        cout << "TYPE 0 after Missing frame loads: "<<missing_frame_loads<< " Extra:"<<xtr_lds<<endl;
        } else {
  //        cout << "TYPE 1/2 Missing frame loads: "<<missing_frame_loads<< " Extra:"<<xtr_lds<<endl;
          if (prev_sampleID == 0) {
            prev_sampleID = in_sampleID;
          } else  if (prev_sampleID != in_sampleID ){
            if (is_in_func){
              importantWindows++;
            }
            totalWindow++;
            sampleID ++;
            prev_sampleID = in_sampleID;
          }


          elements.erase(elements.begin(), elements.end());

         
          in_addr = in_addr >> mask;//shiftin to right
          in_addr = in_addr << mask;//shiftin back to left


          //Creating class elements and adding them to the timeMap
          addr = new Address(in_addr);
          cpu = new CPU(in_cpu);
          time = new AccessTime(in_time, sampleID);
          ip = new Instruction(in_ip);
          ip_to_add = ip;
         
          //setting class pointers for each class
          addr->setAll(in_addr, cpu, time, ip, reuse);
          cpu->setAll(in_cpu, addr, time, ip);
          time->setAll(in_time, sampleID, cpu, addr, ip);
          ip->setAll(in_ip, cpu, time, addr, type);

          ip->setExtraFrameLds(xtr_lds +  missing_frame_loads);
          ip->setDSOName(dso_name);
          missing_frame_loads = 0;
          xtr_lds = 0;

          //Add the appropriate pointer to appropriate vector
          timeVec.push_back(time);
          prevTime = in_time;
          
          //Adding check if do_focus
          if(do_focus){
            //Check funtion
            // Here we are getting function info for each entry
            map <unsigned long, memgaze::Function*>::iterator funcIter;
            funcIter = funcMAP.upper_bound(in_ip);
            if (funcIter == funcMAP.end()){
              func_not_found++;
              //TODO  OPEN OR DO SMTH      cout << ">>>>>ERROR<<<<< func not found with IP:" <<hex<<currIP<<dec<<endl;
              continue;
            } else {
              func_found++;
              funcIter--;
              string currFuncName = funcIter->second->name;
              if (currFuncName.find(functionName) != std::string::npos ){
                is_in_func = true;
                func_last_index = func_index; 
              }
            }
            if (is_in_func){
              funcVec.push_back(time); 
              func_index++;
            }
          }
        }
      }
    }
  }
  
  cout <<"Total Loads:"<<timeVec.size()<<" Trace Size:"<<trace_size<<endl;
  cout <<" before Total Loads in Function:"<<funcVec.size()<<" Func found:"<<func_found<<" Func not Found: "<<func_not_found<<endl;
  int fsize = funcVec.size();
  funcVec.erase(funcVec.begin()+func_last_index+1, funcVec.end());
  cout <<"after Total Loads in Function:"<<funcVec.size()<< " last index: "<<func_last_index << " index"<<func_index<<endl;
  cout << "TOTAL Windows: "<< totalWindow <<" Important: "<<importantWindows << " Problem: " <<totalWindow-importantWindows <<endl;

//Adding do_focus check
 if(do_focus){
  timeVec.clear();
  copy(funcVec.begin(), funcVec.end(), back_inserter(timeVec));
//  timeVec =  funcVec;
 }
  cout <<"Total Loads:"<<timeVec.size()<<endl;


// Trace window based FP Analysis. 
// This will approach Trace as windows per each sample
// We will create a binary tree for each window
// Example a sample of the trace: F1 F1 F1 F2 F2 F2 F2 F3 F3 F1 F1
// Root (Level 2)               N7(N5,N6)
// Level 1           N5(N1,N2)                   N6(N3,N4)
// Level 0     N1 (F1 F1 F1) N2(F2 F2 F2 F2) N3(F3 F3) N4(F1 F1)

//After building tree for each sample we will build a forrest from all of them
//Example     If we have 3 samples as S1 S2 and S3
//                    Root(S1,S2,S3)
//                 C1(S1,S2)     C2(S3)
//              C3(S1)  C4(S2)

  vector <Window *> forest; // This will hold the root of each tree (each sampled period)
  vector <Window *> windows; 
  Window * window =  NULL;
  int lvl=0; 
  pair<unsigned long, int> windowID; // This holds an ID for each window  <Start time(local), level>
  unsigned long startAddr =0  ,  endAddr= 0; 
  unsigned long l_time = 0;
  prevTime = 0 ;
  //TODO NOTE:: probably not need this since I am using madian now  but keep for now. 
  unsigned long window_size = 0 , skip_time=0, window_time=0 , window_first_time =0; //I use these to hold some info
  unsigned long current_ws = 0 , number_of_windows=0, current_ztime = 0, curr_ws_wo_frames = 0;
  //Function related variables. 
  memgaze::Function * tempFunc = NULL;
  std::string currFuncName = "";
  std::string prevFuncName = "";
  map <unsigned long, memgaze::Function*>::iterator funcIter;

  unsigned long currIP =  0;
  bool isWindowAdded = false; 
  int lostInstructions= 0; //TODO NOTE:: I am keeping this but not using in anywhere
  window_first_time = (*timeVec.begin())->time;
  int prevSampleID = (*timeVec.begin())->getSampleID();
  int min_window_size = 7;
  int in_sample_w_size = 0;
  int total_loads_in_trace = 0;
  int total_loads_in_window = 0;
  int ws_wo_frames= 0;
//  bool skip_frame_lds = false;
  cout << "OZGURDBGFRAMELDS total loads before frame loads is "<<timeVec.size()<<endl;
//  cout << "DEBUG:: Line: " << __LINE__ << endl;
  for(auto it = timeVec.begin(); it != timeVec.end(); it++) {
//    skip_frame_lds = false;
    total_loads_in_trace = total_loads_in_trace + 1 + (*it)->ip->getExtraFrameLds();
    total_loads_in_window = total_loads_in_window + 1 + (*it)->ip->getExtraFrameLds();

//    cout << "Current Total: "<<total_loads_in_trace<<" extra frame loads: "<<(*it)->ip->getExtraFrameLds()<<endl;;
//    cout << "OZGUR::Reuse  Address:"<<hex<<(*it)->addr->addr<<dec<< " Reuse: "<<(*it)->addr->rDist;
    current_ws++; // keep current window size 
//    if ((*it)->ip->type == 0){//TODO FIXME I am trying to skip extra frame loads
                              //And delete that entry from the app to save memory
//      delete (*it)->ip;  
//      delete (*it)->addr;  
//      delete (*it)->cpu;  
//      delete (*it); 
//      timeVec.erase(it);
//    } else {
    curr_ws_wo_frames++;
    currIP = (*it)->ip->ip; // instuction IP of current entry
//    cout << " CurrIP: "<<hex<< currIP <<dec<<" time "<<(*it)->time<< " PrevTime "<<prevTime<<" diff "<< (*it)->time - prevTime<<endl;
    
    // Here we are getting function info for each entry
    funcIter = funcMAP.upper_bound(currIP);
    if (funcIter == funcMAP.end()){
      lostInstructions++;
      //TODO NOTE::  Find the issue in here
//TODO  OPEN OR DO SMTH      cout << ">>>>>ERROR<<<<< func not found with IP:" <<hex<<currIP<<dec<<endl;
//      continue; //TODO I remove the continue here since leaf is just each 8 entry but I need to make sure FIXME 
    } else {
      funcIter--;
      tempFunc = funcIter->second;
      currFuncName = tempFunc->name;
//      cout << currFuncName << " size:"<<funcIter->second->timeVec.size() << " adding 0x"<<hex<<(*it)->addr->addr<<dec;
      funcIter->second->timeVec.push_back(*it);
      (*it)->addr->setFuncName(currFuncName);
    }

//Check sample bound
    bool isNewSample =  false;
      if(prevSampleID != (*it)->getSampleID()){
        isNewSample =  true;
      } else {
        isNewSample =  false;
      }

//Dividing trace regarding the sampling frequency/period
    if (isNewSample){
//calculating window size W and skip time Z
      number_of_windows++;
      window_size+=current_ws;
      ws_wo_frames+=curr_ws_wo_frames;
//      cout << "Current w:"<<current_ws << " Tot w:"<<window_size << " #w:"<< number_of_windows;
      current_ztime = ((*it)->time-prevTime);
      skip_time+=current_ztime;
//      cout << " Prev T:"<<prevTime<<" firstT:"<<window_first_time<< " wT:"<<(prevTime - window_first_time);
      wTime.push_back(prevTime - window_first_time);
      unsigned long z_curr;
      if (prevTime - window_first_time){
        z_curr = (current_ws*current_ztime)/(prevTime - window_first_time);
      } else {
        z_curr = 0;
      }
      window_time += prevTime - window_first_time;
      window_first_time =  (*it)->time;
//      cout <<" Current Z:"<<z_curr<<" Zt:"<<current_ztime<< " zt:"<<skip_time<<endl;
      Zs.push_back(z_curr);
      Zt.push_back(current_ztime);
      wSize.push_back(current_ws);
//      cout <<"MULCHECK " <<windowID.first<<":"<<windowID.second << " SampleID:"<< prevSampleID<<" Period::"<<period<<" total_loads_in_window::"<<total_loads_in_window<<" multiplier::"<<(double)period/(double)total_loads_in_window<<endl;
      wMultipliers.push_back((double)period/(double)total_loads_in_window);
      total_loads_in_window = 0;
      curr_ws_wo_frames=0;
      current_ws=0;

// Check if I already add this window to the windows vector
      isWindowAdded = false;
      if (windows.size() >  0 && window!=NULL){ //first check if windows has any element 
                                                // if not definitly add last window to the windows
        if(windows.back()->windowID.first == window->windowID.first && 
            windows.back()->windowID.second == window->windowID.second) {
          isWindowAdded = true;
          cout<<"This window is already in the windows\n";    
        } else {
          isWindowAdded = false;
        }
      } else {
        isWindowAdded =false;
      }

//add this window to windows
      if (window != NULL && !isWindowAdded){
        window->setEtime(prevTime);
        window->setFuncName();
        windows.push_back(window);
      }

// creating tree for current windows and adding them to the forest
      Window * node;
      node = buildTree(&windows);

      if (node == NULL)
        cout << "OZGUR::ERROR NOde IS NULL\n";

      node->sampleHead = true;
      node->ws = current_ws;
      forest.push_back(node);
      in_sample_w_size = 0;

// Cleaning windows and creating new window for current entry      
      windows.clear();
      window = nullptr;
      window =  new Window();
      window->setStime((*it)->time);
      window->setFuncName(currFuncName);
      window->addAddress((*it)->addr);
      windowID = {l_time, lvl};
      window->setID(windowID);

    } else { // now add each entry to their respective windows based on Function bounries
             // acually we moved from function boundries to min window size 8
      if (window == NULL){
        //This is the first window
        window  = new Window();
        window->setStime((*it)->time);
        window->setFuncName(currFuncName);
        window->addAddress((*it)->addr); 
        windowID = {l_time, lvl};
        window->setID(windowID);
      } else {
//Divide sample trace in 2's compenent with min size = 8
        if (in_sample_w_size == min_window_size){
          windows.push_back(window);
          window = nullptr;
          window  = new Window();
          window->setStime((*it)->time);
          window->addAddress((*it)->addr);
          window->setFuncName(currFuncName);
          windowID = {l_time, lvl};
          window->setID(windowID);
          in_sample_w_size = 0;
        } else { 
          window->setEtime((*it)->time);
          window->addAddress((*it)->addr);
          in_sample_w_size++;
        }

////This version divides Sample trace per function
//        if (currFuncName != window->getFuncName() ){ //Checks the function name to build new window on the leaf
//                                          //If all instanes is the same leaf there wont be a second window.
////cout << "DEBUG:: Line: " << __LINE__ << " New Function is "<<currFuncName<<" old Function was "<<window->getFuncName()<<endl;
//          windows.push_back(window);
//          window  = new Window();
//          window->setStime((*it)->time);
//          window->addAddress((*it)->addr);
//          window->setFuncName(currFuncName);
//          windowID = {l_time, lvl};
//          window->setID(windowID);
//        } else {
//        cout << "ELSE  l_time "<<l_time<<" windowID: " <<window->getWindowID().first << " " << window->getWindowID().second << endl ;
//          window->setFuncName(currFuncName); //FIXME fix this add function name 
//          window->setEtime((*it)->time);
//          window->addAddress((*it)->addr);
//        } 
      }
    }
    l_time++;
    prevTime = (*it)->time;
    prevFuncName =  currFuncName ;
    prevSampleID = (*it)->getSampleID();
//    cout << "PrevTime: " << prevTime << " CurrTime: "<<(*it)->time << " Period: " << period << endl;
//  }
  }
  cout << "OZGURDBGFRAMELDS after timeVec  size  "<<timeVec.size()<<endl;

  cout << "OZGURDBGFRAMELDS total loads after frame loads is "<<total_loads_in_trace<<endl;
//ADD last window crated
  if (window != NULL){
    if (windows.size() >  0){ //first check if windows has any element 
                                    // if not definitly add last window to the windows
      if(windows.back()->windowID.first == window->windowID.first && 
          windows.back()->windowID.second == window->windowID.second) {
        isWindowAdded = true;
        cout<<"This window is already in the windows\n";    
      } else {
        isWindowAdded = false;
      }
    } else {
      cout <<"There are no windows added\n";
      isWindowAdded =false;
    }

    if (window != NULL && !isWindowAdded){
      window->setFuncName();
      windows.push_back(window);
    }
    Window * node;

    node = buildTree(&windows);
    if (node == NULL)
      cout << "OZGUR::ERROR NOde IS NULL\n";
    node->sampleHead = true;
    node->ws = current_ws;
    forest.push_back(node);
    windows.clear();
    window = nullptr;
  }

//  //DEBUG the forest first 
//  cout << "Debugging forest with "<<forest.size()<<" window\n";
//  for(auto it = forest.begin() ; it != forest.end(); it++){
//    cout << "windowId "<< (*it)->getWindowID().first<<":"<<(*it)->getWindowID().second<<" size:"<<(*it)->getSize()<<endl; 
//   // printTree((*it) , period,  (*it)->windowID.second, false , is_load); 
//  }
 
  float total_ld_multiplier = 1; 
  if (window_size && number_of_windows > 0){
    float single_w = (float)window_size/(float)number_of_windows;
    total_ld_multiplier = multiplier/single_w;
  }
  if (number_of_windows){
  cout <<"PERIOD: "<<multiplier<<" WS:"<<window_size/number_of_windows<<endl;
  }
  
  unsigned long skip_size, total_window_time = window_time, total_window_size=window_size;
  unsigned long total_skip_time = skip_time;
//  cout << "Before WS:" <<window_size<< " Zt:"<<skip_time << " Wt:"<<window_time<<" ZS:"<<skip_size<<endl;
  float wSize1=0, wTime1=0, zSize1=0, zTime1=0;
  float skip_size2 =0, skip_size1=0;

  //calculating LoadBased Multiplier versions:
  double multiplier_ld_median =0;
  double multiplier_ld_mean =0;
  double multiplier_ld_from_totals =0;
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
    double total_of_multipliers = 0;
    for (int index=0; index< size_wMultiplier; index++){
//      cout << " Sample Index="<< index<<" multipler="<<wMultipliers[index]<<endl;
      total_of_multipliers+=wMultipliers[index];
    }
    multiplier_ld_mean = total_of_multipliers/number_of_windows;
    multiplier_ld_from_totals = ((double)number_of_windows*(double)period) / (double)total_loads_in_trace;
  }


  //TODO NOTE:: When you are done remove un used ones to clean up
  //Calculating  w and z 
  if (number_of_windows >0 && window_time >0){
    std::sort(Zs.begin(),Zs.end());
    std::sort(Zt.begin(),Zt.end());
    std::sort(wTime.begin(),wTime.end());
    std::sort(wSize.begin(),wSize.end());
    int size_Zs =  Zs.size();
    int size_Zt =  Zt.size();
    int size_wSize =  wSize.size();
    int size_wTime =  wTime.size();
//    cout << "CONTROL:: size of  Zs:"<<size_Zs<<" Zt:"<<size_Zt<<" Ws:"<<size_wSize<<" Wt"<<size_wTime<<endl;
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
    cout<< "CONTROL Median Zs:"<<skip_size<<" Zt:"<<zTime1<<" Ws:"<<wSize1<<" Wt:"<<wTime1<<" Stime: "<<skip_time<<endl;
    cout << "Window Size "<<window_size << " Wtime: "<<window_time << " #Win: "<<number_of_windows<< endl;
    if (window_time)
      skip_size1 = (skip_time*window_size)/window_time;
    if (number_of_windows)
      window_size=window_size/number_of_windows;
    if (number_of_windows-1)
      skip_time=skip_time/(number_of_windows-1);
    if (number_of_windows)
      window_time = window_time/number_of_windows;
    if (window_time)
      skip_size2 = (skip_time*window_size)/window_time;
    cout << "alternative Z1:"<<skip_size1 << " ZS:"<<skip_size2<<" Z3:"<<zSize1<<endl;
  }
  if (do_reuse){
    cout << "Do_reuse WS:" <<window_size<< " Zt:"<<skip_time << " Wt:"<<window_time<<" ZS:"<<skip_size<<endl;
  } else {
    skip_size = 0;
    cout << "FULL WS:" <<window_size<< " Zt:"<<skip_time << " Wt:"<<window_time<<" ZS:"<<skip_size<<endl;
  }
  
  cout << "Size of forest is "<<forest.size()<<endl;

  // Calculate footprint with new formulate by using the forest. 
  multiplier = ( ((float)window_size+(float)skip_size)/window_size ); 
  cout << "MULTIPLIERS: xx="<<multiplier;

  Window * fullT;
  cout << "Building tree Forest size  "<<forest.size()<<endl;
  fullT = buildTree(&forest );
  //TODO FUNCVIEW create forest for each function's trace and sent build tree similart to this.
//      cout << "Size of head node is "<<fullT->getSize()<<endl;
  if (fullT == NULL)
    cout << "ERROR ROOT IS NULL\n";
  cout << "PRINTING TREE of FP\n";

  cout << "General MULTIPLIER="<<multiplier<<endl; //TODO NOTE:: maybe get rid of general completely
  cout << "#windows: "<<number_of_windows<<" Period:"<<period<<" Total Lds:"<< total_loads_in_trace<<endl;
  double multiplier2 = (((double)number_of_windows*(double)period) - (double)total_loads_in_trace) / (double)total_loads_in_trace;
  double imp_to_all_ratio = (double)total_loads_in_trace / (double)timeVec.size();
  cout << "General MULTIPLIER with frame loads="<<multiplier2<<endl; //TODO NOTE:: maybe get rid of general completely
  cout << "Recorded: "<<timeVec.size()<<" supposed to reccord(w/ frame)"<<total_loads_in_trace<<" ratio:"<<imp_to_all_ratio<<endl; //TODO NOTE:: maybe get rid of general completely
  printTree(fullT , period, 0 , &treeFPavgMap2 , false , is_load); 
//TODO open  printTree(fullT , period,  fullT->windowID.second, false , is_load); 
  //printTree(fullT , &treeFPavgMap, period,  fullT->windowID.second, false , is_load); 

//Here we will calculate the FP for each function seperately 
//  if (do_reuse){
//    multiplier = ((float)window_size + (float)skip_size)/(float)window_size; //MEAN & MEDIAN        3
//    cout << "MULTIPLIERS: 3="<<multiplier;
//    multiplier = ((float)window_size + (float)skip_size2)/(float)window_size; //MEAN & from MEAN    4
//    cout << " 4="<<multiplier;
//    multiplier = ((float)window_size + (float)zSize1)/(float)window_size; //MEAN & from MEdian      5
//    cout << " 5="<<multiplier;
//    multiplier = ((float)wSize1 + (float)skip_size)/(float)wSize1; //MEDIAN & MEDIAN              *  2
//    cout << " 2="<<multiplier;
//    multiplier = ((float)wSize1 + (float)zSize1)/(float)wSize1; //MEDIAN & From MEDIANs               1
//    cout << " 1="<<multiplier<< endl;
//  } else {
//    multiplier = 1; 
//  } 
  if (is_LDLAT || !do_reuse){
    multiplier = 1; 
  }
  
  cout << "Function based Flat FP with mutiplier:"<<multiplier<<endl;
  float local_multiplier= 0;
  for (auto it= funcMAP.begin();it !=funcMAP.end();it++){//TODO actually here first buila a tree for
                                                         //each function then print tree
    if (it->second->timeVec.size() > 0){
      (*it).second->calcFP();
  //    cout << "getting Local Multiplier for function "<<(*it).second->name<<endl;
      local_multiplier =  (*it).second->getMultiplier(period, is_load);
      map <int, double> diagMap;
      (*it).second->getFPDiag(&diagMap);
  //    cout << "General Multipliler = "<<multiplier<<endl;
  //    cout << (*it).second->name << " StartIP:"<<hex<<(*it).second->startIP<<dec <<" Size:"<< it->second->timeVec.size() << " FP:"<<(*it).second->fp*multiplier << " lds:"<<(*it).second->totalLoads ;
  //    cout<<" Strided:"<<diagMap[1]*multiplier <<" Indirect:"<<diagMap[2]*multiplier <<" Constant:"<<diagMap[0]*multiplier <<" Unknown:"<<diagMap[-1]*multiplier<<endl;
//      cout << (*it).second->name << " StartIP: "<<hex<<(*it).second->startIP<<dec <<" Size: "<< it->second->timeVec.size() << " FP: "<<(*it).second->fp*local_multiplier << " lds: "<<(*it).second->totalLoads ;
//      cout<<" Strided: "<<diagMap[1]*local_multiplier <<" Indirect: "<<diagMap[2]*local_multiplier<<" Constant: "<<diagMap[0]*local_multiplier <<" Unknown: "<<diagMap[-1]*local_multiplier;
//      cout << " Local multiplier = "<<local_multiplier<<endl;
//      local_multiplier = multiplier2;
//      cout << (*it).second->name << " M2 StartIP: "<<hex<<(*it).second->startIP<<dec <<" Size: "<< it->second->timeVec.size() << " FP: "<<(*it).second->fp*local_multiplier << " lds: "<<(*it).second->totalLoads ;
//      cout<<" Strided: "<<diagMap[1]*local_multiplier <<" Indirect: "<<diagMap[2]*local_multiplier<<" Constant: "<<diagMap[0]*local_multiplier <<" Unknown: "<<diagMap[-1]*local_multiplier;
//      cout << " Local multiplier = "<<local_multiplier<<endl;
//     local_multiplier = multiplier_ld_median;
//      cout << (*it).second->name << " Med StartIP: "<<hex<<(*it).second->startIP<<dec <<" Size: "<< it->second->timeVec.size() << " FP: "<<(*it).second->fp*local_multiplier << " lds: "<<(*it).second->totalLoads ;
//      cout<<" Strided: "<<diagMap[1]*local_multiplier <<" Indirect: "<<diagMap[2]*local_multiplier<<" Constant: "<<diagMap[0]*local_multiplier <<" Unknown: "<<diagMap[-1]*local_multiplier;
//      cout << " Local multiplier = "<<local_multiplier<<endl;
//     local_multiplier = multiplier_ld_mean;
//      cout << (*it).second->name << " Mean StartIP: "<<hex<<(*it).second->startIP<<dec <<" Size: "<< it->second->timeVec.size() << " FP: "<<(*it).second->fp*local_multiplier << " lds: "<<(*it).second->totalLoads ;
//      cout<<" Strided: "<<diagMap[1]*local_multiplier <<" Indirect: "<<diagMap[2]*local_multiplier<<" Constant: "<<diagMap[0]*local_multiplier <<" Unknown: "<<diagMap[-1]*local_multiplier;
//      cout << " Local multiplier = "<<local_multiplier<<endl;
     //local_multiplier = multiplier_ld_from_totals;
     local_multiplier = multiplier2;
      cout << (*it).second->name << " tot StartIP: "<<hex<<(*it).second->startIP<<" EndIP: "<< (*it).second->endIP<<dec <<" Size: "<< it->second->timeVec.size() << " FP: "<<(*it).second->fp*local_multiplier << " lds: "<<(*it).second->timeVec.size()*imp_to_all_ratio*local_multiplier<<" collected/total: "<<imp_to_all_ratio;
      cout<<" Strided: "<<diagMap[1]*local_multiplier <<" Indirect: "<<diagMap[2]*local_multiplier<<" Constant: "<<diagMap[0]*local_multiplier <<" Unknown: "<<diagMap[-1]*local_multiplier;
      cout << " Multiplier = "<<local_multiplier<<" Growth Rate: "<<((*it).second->fp*local_multiplier)/((*it).second->timeVec.size()*imp_to_all_ratio*local_multiplier)<<endl;


    }
  }

//PRINT TREE Averages:
  cout << "FULL TRACE FP: "<<fullT->getFP()* multiplier<<endl;
  cout<<endl << "Printing Level Averages"<<endl;
  const char filler = ' ';
  const int cellsize = 16;
  //multiplier2 = multiplier_ld_mean;
  //multiplier2 = multiplier_ld_median;
  multiplier2 = multiplier_ld_from_totals;
  if (is_LDLAT){
    multiplier2 =1;
  } else if (multiplier2 < 1 ){
    multiplier2 =1;
  } 
  long int lds = timeVec.size()*total_ld_multiplier;
  if (do_output){
    outFile << left <<setw(cellsize) << setfill(filler)<<"LVL"
            << left <<setw(cellsize) << setfill(filler)<<"Number_of_Nodes"
            << left <<setw(cellsize) << setfill(filler)<<"Multiplier"
            << left <<setw(cellsize) << setfill(filler)<<"Window_Size"
            << left <<setw(cellsize) << setfill(filler)<<"FP"
            << left <<setw(cellsize) << setfill(filler)<<"Strided_FP"
            << left <<setw(cellsize) << setfill(filler)<<"Indirect_FP"
            << left <<setw(cellsize) << setfill(filler)<<"Constant_Loads"
            << left <<setw(cellsize) << setfill(filler)<<"Unknown"
            << left <<setw(cellsize) << setfill(filler)<<"Total_Loads"
            << left <<setw(cellsize) << setfill(filler)<<"Cont2Load_ratio"
            << left <<setw(cellsize) << setfill(filler)<<"NPF_Rate"
            << left <<setw(cellsize) << setfill(filler)<<"NPF_Growth_Rate"
            << left <<setw(cellsize) << setfill(filler)<<"Growth_Rate"
            << endl;
  }
  for (auto it = treeFPavgMap2.begin(); it != treeFPavgMap2.end(); it ++){
    int treeLVL = it->first;
    int n_nodes = treeFPavgMap2[treeLVL][NUMBER_OF_NODES];
//    if (treeFPavgMap2[treeLVL][IN_SAMPLE] == 0){
    if (do_output){
      outFile << left <<setw(cellsize) << setfill(filler) << treeLVL   
              << left <<setw(cellsize) << setfill(filler) << n_nodes; 
//              << left <<setw(cellsize) << setfill(filler) << multiplier2;
      if (treeFPavgMap2[treeLVL][IN_SAMPLE] == 0){
        outFile << left <<setw(cellsize) << setfill(filler) << multiplier2
                << left <<setw(cellsize) << setfill(filler) << (treeFPavgMap2[treeLVL][WINDOW_SIZE] /n_nodes) * multiplier2
                << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][FP] / n_nodes) * multiplier2 
                << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][STRIDED] / n_nodes) * multiplier2
                << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][INDIRECT] / n_nodes) * multiplier2 
                << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][CONSTANT] / n_nodes) * multiplier2 
                << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][UNKNOWN] / n_nodes) * multiplier2 
                << left <<setw(cellsize) << setfill(filler) << ((treeFPavgMap2[treeLVL][WINDOW_SIZE] + treeFPavgMap2[treeLVL][CONSTANT]) /n_nodes) * multiplier2 ;
      } else {
        outFile << left <<setw(cellsize) << setfill(filler) << 1
                << left <<setw(cellsize) << setfill(filler) << (treeFPavgMap2[treeLVL][WINDOW_SIZE] /n_nodes) 
                << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][FP] / n_nodes) 
                << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][STRIDED] / n_nodes)
                << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][INDIRECT] / n_nodes) 
                << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][CONSTANT] / n_nodes) 
                << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][UNKNOWN] / n_nodes) 
                << left <<setw(cellsize) << setfill(filler) << ((treeFPavgMap2[treeLVL][WINDOW_SIZE] + treeFPavgMap2[treeLVL][CONSTANT]) /n_nodes);
      }  
      outFile   << left <<setw(cellsize) << setfill(filler) << (( treeFPavgMap2[treeLVL][CONSTANT] / n_nodes) * multiplier2) / (((treeFPavgMap2[treeLVL][WINDOW_SIZE] + treeFPavgMap2[treeLVL][CONSTANT]) /n_nodes) * multiplier2 )
                << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][INDIRECT] / treeFPavgMap2[treeLVL][FP] )
                << left <<setw(cellsize) << setfill(filler) << (( treeFPavgMap2[treeLVL][INDIRECT] / n_nodes) * multiplier2) / ((treeFPavgMap2[treeLVL][WINDOW_SIZE] /n_nodes) * multiplier2 )
                << left <<setw(cellsize) << setfill(filler) << (( treeFPavgMap2[treeLVL][FP] / n_nodes) * multiplier2) / ((treeFPavgMap2[treeLVL][WINDOW_SIZE] /n_nodes) * multiplier2)
                << endl;
      }
      //Checking total loads in that window 
      cout << "LVL: " << treeLVL << " Number_of_Nodes: " << n_nodes << " multiplier2: " << multiplier2 
           << " Window_Size: " << (treeFPavgMap2[treeLVL][WINDOW_SIZE] /n_nodes) *multiplier2
           << " FP: " << ( treeFPavgMap2[treeLVL][FP] / n_nodes) * multiplier2
           << " Strided: " << ( treeFPavgMap2[treeLVL][STRIDED] / n_nodes) * multiplier2
           << " Indirect: " << ( treeFPavgMap2[treeLVL][INDIRECT] / n_nodes) * multiplier2
           << " Constant: " << ( treeFPavgMap2[treeLVL][CONSTANT] / n_nodes) * multiplier2
           << " Unknown: " << ( treeFPavgMap2[treeLVL][UNKNOWN] / n_nodes) * multiplier2
           << " Acns: "<< (( treeFPavgMap2[treeLVL][CONSTANT] / n_nodes) ) / (((treeFPavgMap2[treeLVL][WINDOW_SIZE] + treeFPavgMap2[treeLVL][CONSTANT]) /n_nodes))
           << " Total_Loads: " << ((treeFPavgMap2[treeLVL][WINDOW_SIZE] + treeFPavgMap2[treeLVL][CONSTANT]) /n_nodes)   *multiplier2 <<endl;
//
//     
//
//      //Version 1 median load based multiplier:
//      cout << "LVL: " << treeLVL << " Number_of_Nodes: " << n_nodes << " multiplier_ld_median: " << multiplier_ld_median 
//           << " Window_Size: " << (treeFPavgMap2[treeLVL][WINDOW_SIZE] /n_nodes) *multiplier_ld_median
//           << " FP: " << ( treeFPavgMap2[treeLVL][FP] / n_nodes) * multiplier_ld_median 
//           << " Strided: " << ( treeFPavgMap2[treeLVL][STRIDED] / n_nodes) * multiplier_ld_median 
//           << " Indirect: " << ( treeFPavgMap2[treeLVL][INDIRECT] / n_nodes) * multiplier_ld_median 
//           << " Constant: " << ( treeFPavgMap2[treeLVL][CONSTANT] / n_nodes) * multiplier_ld_median 
//           << " Unknown: " << ( treeFPavgMap2[treeLVL][UNKNOWN] / n_nodes) * multiplier_ld_median << endl;
//      //Version 1 means load based multiplier:
//
//      cout << "LVL: " << treeLVL << " Number_of_Nodes: " << n_nodes << " multiplier_ld_mean: " << multiplier_ld_mean
//           << " Window_Size: " << (treeFPavgMap2[treeLVL][WINDOW_SIZE] /n_nodes) *multiplier_ld_mean
//           << " FP: " << ( treeFPavgMap2[treeLVL][FP] / n_nodes) * multiplier_ld_mean 
//           << " Strided: " << ( treeFPavgMap2[treeLVL][STRIDED] / n_nodes) * multiplier_ld_mean 
//           << " Indirect: " << ( treeFPavgMap2[treeLVL][INDIRECT] / n_nodes) * multiplier_ld_mean 
//           << " Constant: " << ( treeFPavgMap2[treeLVL][CONSTANT] / n_nodes) * multiplier_ld_mean 
//           << " Unknown: " << ( treeFPavgMap2[treeLVL][UNKNOWN] / n_nodes) * multiplier_ld_mean << endl;
//
//      //Version 2 from totals  load based multiplier:
//
//      cout << "LVL: " << treeLVL << " Number_of_Nodes: " << n_nodes << " multiplier_ld_from_totals: " << multiplier_ld_from_totals
//           << " Window_Size: " << (treeFPavgMap2[treeLVL][WINDOW_SIZE] /n_nodes) *multiplier_ld_from_totals
//           << " FP: " << ( treeFPavgMap2[treeLVL][FP] / n_nodes) * multiplier_ld_from_totals 
//           << " Strided: " << ( treeFPavgMap2[treeLVL][STRIDED] / n_nodes) * multiplier_ld_from_totals 
//           << " Indirect: " << ( treeFPavgMap2[treeLVL][INDIRECT] / n_nodes) * multiplier_ld_from_totals 
//           << " Constant: " << ( treeFPavgMap2[treeLVL][CONSTANT] / n_nodes) * multiplier_ld_from_totals 
//           << " Unknown: " << ( treeFPavgMap2[treeLVL][UNKNOWN] / n_nodes) * multiplier_ld_from_totals << endl;
//           cout<<endl;
//    } else {
//      outFile <<left <<setw(cellsize) << setfill(filler) << treeLVL 
//           << left <<setw(cellsize) << setfill(filler) << n_nodes 
//           << left <<setw(cellsize) << setfill(filler) << "1"  
//           << left <<setw(cellsize) << setfill(filler) << treeFPavgMap2[treeLVL][WINDOW_SIZE] / n_nodes
//           << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][FP] / n_nodes)  
//           << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][STRIDED] / n_nodes) 
//           << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][INDIRECT] / n_nodes) 
//           << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][CONSTANT] / n_nodes)  
//           << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][UNKNOWN] / n_nodes)  
//           << left <<setw(cellsize) << setfill(filler) << ( treeFPavgMap2[treeLVL][INDIRECT] / treeFPavgMap2[treeLVL][FP])  
//           << left <<setw(cellsize) << setfill(filler) << (( treeFPavgMap2[treeLVL][INDIRECT] / n_nodes) ) / lds
//           << left <<setw(cellsize) << setfill(filler) << (( treeFPavgMap2[treeLVL][FP] / n_nodes) ) /lds
//           << endl;
//
//
//        cout << "LVL: " << treeLVL << " Number_of_Nodes: " << n_nodes << " Multiplier: 1"  
//           << " Window_Size: " << treeFPavgMap2[treeLVL][WINDOW_SIZE] / n_nodes
//           << " FP: " << ( treeFPavgMap2[treeLVL][FP] / n_nodes)  
//           << " Strided: " << ( treeFPavgMap2[treeLVL][STRIDED] / n_nodes) 
//           << " Indirect: " << ( treeFPavgMap2[treeLVL][INDIRECT] / n_nodes) 
//           << " Constant: " << ( treeFPavgMap2[treeLVL][CONSTANT] / n_nodes)  
//           << " Unknown: " << ( treeFPavgMap2[treeLVL][UNKNOWN] / n_nodes)  << endl;
//    }
  }
    
    cout <<"Total Trace size: "<<timeVec.size()<<" Important trace size: "<<funcVec.size()<<" Multiplier "<< total_ld_multiplier<<endl;
    cout <<"Total loads: "<<timeVec.size()*total_ld_multiplier<<" Important loads: "<<funcVec.size()*total_ld_multiplier<<endl;

  cout << "Tree root multiplier: "<<endl;
  fullT->calcMultiplier(period, is_load);
  if(do_focus){
    //PRINT IMPORTANT FUNCTION
    memgaze::Function *imp_func = new memgaze::Function(functionName ,  0);
    cout<<endl << "Printing important function: "<<functionName<<endl;
    unsigned long sTime = 0 , eTime=0;
    for (auto  it =funcVec.begin(); it != funcVec.end(); it++){
      imp_func->timeVec.push_back(*it);
      if (sTime == 0 ){
        sTime = (*it)->time;
      }
      eTime= (*it)->time;
    }
    map <int, double> diagMap;
    imp_func->calcFP();
    cout << " Start time: "<< sTime <<" End time: "<<eTime<<endl;
    local_multiplier =  imp_func->getMultiplier(period, is_load);
    if (is_LDLAT){
      local_multiplier = 1;
    }
    imp_func->getFPDiag(&diagMap);
    cout << imp_func->name << " StartIP: "<<hex<<imp_func->startIP<<dec <<" Size: "<<imp_func->timeVec.size() << " FP: "<<imp_func->fp*local_multiplier << " lds: "<<imp_func->totalLoads ;

    cout<<" Strided: "<<diagMap[1]*local_multiplier <<" Indirect: "<<diagMap[2]*local_multiplier<<" Constant: "<<diagMap[0]*local_multiplier <<" Unknown: "<<diagMap[-1]*local_multiplier;
    cout << " Local multiplier = "<<local_multiplier<<" time(s):"<<(eTime-sTime)/1000000000.0<<endl;

  }
// Here We are trying to calculate function total fp by checking each window
// This is work in porgress not finalized
// TODO work on this
//begin
//  diagMap.clear();
//  Function *f1 = new Function(functionName, 0);
//  cout <<endl << " Total windows in trace is "<<sampleVec.size();
//  for (auto it = sampleVec.begin(); it != sampleVec.end(); it++){
//    (*it)->setFuncName();
//    string fname = (*it)->getFuncName();
//    if (fname.find(functionName) != std::string::npos ){
//      funcSampleVec.push_back(*it);
//      for (auto add = (*it)->addresses.begin(); add != (*it)->addresses.end(); add++){
//        f1->timeVec.push_back((*add)->time);
//      }
//    }
//  }
//  cout <<endl<< " Total windows that includes function "<<functionName<<" is "<<funcSampleVec.size()<<endl; 
//  f1->calcFP();
//  local_multiplier =  f1->getMultiplier(period, is_load);
//  f1->getFPDiag(&diagMap);
//  cout << f1->name << " StartIP: "<<hex<<f1->startIP<<dec <<" Size: "<<f1->timeVec.size() << " FP: "<<f1->fp*local_multiplier << " lds: "<<f1->totalLoads ;
//
//  cout<<" Strided: "<<diagMap[1]*local_multiplier <<" Indirect: "<<diagMap[2]*local_multiplier<<" Constant: "<<diagMap[0]*local_multiplier <<" Unknown: "<<diagMap[-1]*local_multiplier;
//  cout << " Local multiplier = "<<local_multiplier<<" time(s):"<<(eTime-sTime)/1000000000.0<<endl;
//end

// Onur: test MemgazeSource
// #ifdef DEVELOP
//   hpctk_main(argc, argv, hpcStructInputFile, fullT);
// #endif


  //Here we free anyhing we created
  for (auto i = timeVec.begin(); i != timeVec.end(); ++i) {
    delete *i;
  }
  timeVec.clear();
  funcVec.clear();
 
  outFile.close();
  inFile.close(); 

  std::cout << "finished with Footprint Analysis" << std::endl;
  return 0;
}
