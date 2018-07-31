/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: prog_scope.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Specialized classes for various program scope types.
 */

#ifndef _PROG_SCOPE_H_
#define _PROG_SCOPE_H_

#include "scope_implementation.h"
#include <string>
//#include "routine.h"

namespace MIAMI
{

class ProgScope : public ScopeImplementation
{
public:
   /* this one has no parent */
   ProgScope(std::string name, CodeScopeKey _key) : 
             ScopeImplementation(NULL, PROG_SCOPE, _key)
   {
      exec_name = name;
   }
   
   virtual ~ProgScope()  {}
   
   std::string ToString();
   std::string XMLLabel();
   std::string XMLScopeHeader();
   std::string AlienXMLScopeHeader(const char* namePrx = "");
   std::string XMLScopeFooter();

   const char* Name() { return exec_name.c_str(); }
   int Level() const { return (-3); }

private:
   std::string exec_name;
};

class ImageScope : public ScopeImplementation
{
public:
   /* child of ProgScope */
   ImageScope(ScopeImplementation *_parent, std::string name, 
                  CodeScopeKey _key) : 
             ScopeImplementation(_parent, IMAGE_SCOPE, _key)
   {
      img_name = name;
   }
   
   virtual ~ImageScope()  {}
   
   std::string ToString();
   std::string XMLLabel();
   std::string XMLScopeHeader();
   std::string AlienXMLScopeHeader(const char* namePrx = "");
   std::string XMLScopeFooter();

   const char* Name() { return img_name.c_str(); }
   int Level() const { return (-2); }

private:
   std::string img_name;
};

class FileScope : public ScopeImplementation
{
public:
   FileScope(ScopeImplementation *_parent, std::string fname,
                 CodeScopeKey _key) : 
            ScopeImplementation(_parent, FILE_SCOPE, _key)
   {
      file_name = fname;
   }
   
   virtual ~FileScope()   {}
   
   std::string ToString();
   std::string XMLLabel();
   std::string XMLScopeHeader();
   std::string AlienXMLScopeHeader(const char* namePrx = "");
   std::string XMLScopeFooter();
   
   inline const char* FileName() { return file_name.c_str(); }
   inline int Level() const     { return (-1); }

private:
   std::string file_name;
};


class RoutineScope : public ScopeImplementation
{
public:
   RoutineScope(ScopeImplementation *_parent, std::string rname, /*std::string fname, */
                 CodeScopeKey _key) : 
            ScopeImplementation(_parent, ROUTINE_SCOPE, _key)
   {
      routine_name = rname;
   }
   
   virtual ~RoutineScope()   {}
   
   std::string ToString();
   std::string XMLLabel();
   std::string XMLScopeHeader();
   std::string AlienXMLScopeHeader(const char* namePrx = "");
   std::string XMLScopeFooter();
   
   inline const char* Name() { return routine_name.c_str(); }
   inline std::string& RoutineName() { return routine_name; }
   inline int Level() const { return (0); }

   virtual void setStartLine(unsigned int _eLine, std::string& _func);

private:
   std::string routine_name;
};


class LoopScope : public ScopeImplementation
{
public:
   LoopScope(ScopeImplementation *_parent, int _level, int _index, 
                  CodeScopeKey _key)
         : ScopeImplementation(_parent, LOOP_SCOPE, _key)
   {
      level = _level; index = _index;
   }
   
//   LoopScope(ScopeImplementation *_parent, int _level, int _index, 
//                  CodeScopeKey _key, MIAMI::LoopFP _loopFP)
//         : ScopeImplementation(_parent, LOOP_SCOPE, _key)
//   {
//      level = _level; index = _index; loopFP = _loopFP;
//   }
   
   virtual ~LoopScope()  {}

   std::string ToString();
   std::string XMLLabel();
   std::string XMLScopeHeader();
   std::string XMLScopeFooter();
   std::string AlienXMLScopeHeader(const char* namePrx = "");
   
   inline int Index() const  { return (index); }
   inline int Level() const { return (level); }

   virtual void setStartLine(unsigned int _eLine, std::string& _func);
   virtual void setEndLine(unsigned int _eLine, std::string& _func);
   
//   void addLoopFP( LoopFP _loopFP){ loopFP = _loopFP;}
//   MIAMI::LoopFP * getLoopFP(){retrun loopFP;}

private:
   int level, index;
//   MIAMI::LoopFP loopFP;
};

class GroupScope : public ScopeImplementation
{
public:
   GroupScope(ScopeImplementation *_parent, std::string gname,
                 CodeScopeKey _key) : 
            ScopeImplementation(_parent, GROUP_SCOPE, _key)
   {
      group_name = gname;
   }
   
   virtual ~GroupScope()   {}
   
   std::string ToString();
   std::string XMLLabel();
   std::string XMLScopeHeader();
   std::string AlienXMLScopeHeader(const char* namePrx = "");
   std::string XMLScopeFooter();
   
   const char* Name() { return group_name.c_str(); }
   int Level() const { return (0); }

private:
   std::string group_name;
};

////this vector will keep the dependencies of each indiriect
////or strided load but not the frame loads
//typedef std::vector <FPdependencies* > FPDepVector;
//
//struct FPdependencies{
//   coeff_t offset;
//   int level;
//};
//
//class FP{
//public:
//   FP(double _fp=-1){
//      fp = _fp;
//   }
//   
//   virtual ~FP(){}
//
//   double getFP(){return fp;}
//   void setFP(double _fp){fp = _fp;}
//   //This fuction will add a dependency to FP
//   //it will also update the fp based on dependencies if it requires
//   void addFPDependency(FPdependencies *_fpDep);
//   FPDepMap * getFPDependencies(){return &fpDep;}
//private:
//   double fp;
//   FPDepVector fpDep;
//};
//
//class LoopFP {
//public:
//   LoopFP(FP *_fp, addrtype _loopId, addrtype _startAddress, 
//            int _level, int _index, long int _loopCount, LoopFP *_innerLoop = NULL){
//      fp = _fp;
//      loopId = _loopId;
//      startAddress = _startAddress;
//      level = _level;
//      index = _index;
//      loopCount = _loopCount;
//      innerLoop = _innerLoop;
//   }
//
//   virtual ~LoopFP() {}
//
//   FP * getLoopFP(){return fp;}
//   void setLoopFP(FP *_fp){fp = _fp;}
//
//   addrtype getLoopId(){return loopId;}
//   addrtype getStartAddress(){return startAddress;}
//   int getLevel () {return level;}
//   int getIndex () {return index;}
//   long int getLoopCount(){return loopCount;}
//
//   LoopFP * getInnerLoop(){return innerLoop;}
//   void setInnerLoop(LoopFP * _innerLoop){innerLoop = _innerLoop;}
//
//private:
//   FP *fp;
//   addrtype loopId;
//   addrtype startAddress;
//   int level;
//   int index;
//   long int loopCount;
//   LoopFP *innerLoop;
//};
//

} /* namespace MIAMI */

#endif
