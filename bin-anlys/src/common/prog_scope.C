/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: prog_scope.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Specialized classes for various program scope types.
 */

#include "prog_scope.h"
#include "xml.h"
#include "Assertion.h"
#include <iostream>
#include <sstream>

using namespace MIAMI;

std::string 
ProgScope::ToString()
{
   return (std::string("Program:") + exec_name);
}

std::string 
ProgScope::XMLLabel() 
{ 
   return std::string ("PGM"); 
}
   
std::string 
ProgScope::XMLScopeHeader()
{
   return (std::string ("<PGM n=\"") + xml::EscapeStr(exec_name.c_str()) + "\">");
}

std::string 
ProgScope::AlienXMLScopeHeader(const char* namePrx)
{
   return (std::string ("<A n=\"") + xml::EscapeStr (namePrx)
            + "PGM: " + xml::EscapeStr(exec_name.c_str()) + "\">");
}
   
std::string 
ProgScope::XMLScopeFooter ()
{
   return (std::string ("</PGM>"));
}


std::string 
ImageScope::ToString()
{
   return (std::string("Module:") + img_name);
}

std::string 
ImageScope::XMLLabel() 
{ 
   return std::string ("LM"); 
}
   
std::string 
ImageScope::XMLScopeHeader()
{
   return (std::string ("<LM n=\"") + xml::EscapeStr(img_name.c_str()) + "\">");
}

std::string 
ImageScope::AlienXMLScopeHeader(const char* namePrx)
{
   return (std::string ("<A n=\"") + xml::EscapeStr (namePrx)
            + "LM: " + xml::EscapeStr(img_name.c_str()) + "\">");
}
   
std::string 
ImageScope::XMLScopeFooter ()
{
   return (std::string ("</LM>"));
}


std::string 
FileScope::ToString()
{
   return ("File:" + file_name );
}

std::string 
FileScope::XMLLabel() 
{ 
   return std::string ("F"); 
}

std::string 
FileScope::XMLScopeHeader()
{
   return (std::string ("<F n=\"") + xml::EscapeStr(file_name.c_str()) + "\">");
}
   
std::string 
FileScope::AlienXMLScopeHeader(const char* namePrx)
{
   const char* fname = file_name.c_str();
   size_t lastSlash = file_name.rfind("/");
   if (lastSlash != std::string::npos)  // we found a '/'
      fname = fname + lastSlash + 1;
   return (std::string ("<A f=\"") + xml::EscapeStr(file_name.c_str()) + "\" n=\"" +
         xml::EscapeStr (namePrx) +
         xml::EscapeStr (fname) + "\">");
}
   
std::string 
FileScope::XMLScopeFooter()
{
   return (std::string ("</F>"));
}


std::string 
RoutineScope::ToString()
{
   FileScope *fscope = dynamic_cast<FileScope*>
               (Ancestor(FILE_SCOPE));
   const char *fname;
   if (fscope!=NULL)
      fname = fscope->FileName();
   else
      fname = "unknown-file";
   
   std::string file_name = fname;
   size_t lastSlash = file_name.rfind("/");
   if (lastSlash != std::string::npos)  // we found a '/'
      fname = fname + lastSlash + 1;
   std::ostringstream oss;
   oss << "P:" << routine_name << ";" << fname
       << "[" << getStartLine() << ";" 
       << getEndLine() << "]";
   return (oss.str());
}

std::string 
RoutineScope::XMLLabel() 
{ 
   return std::string ("P"); 
}

std::string 
RoutineScope::XMLScopeHeader()
{
   std::ostringstream oss;
   oss << "<P n=\"" << xml::EscapeStr(routine_name.c_str()) << "\" b=\"" 
       << getStartLine() << "\" e=\"" << getEndLine() << "\">";
   return (oss.str());
}

std::string 
RoutineScope::AlienXMLScopeHeader(const char* namePrx)
{
   FileScope *fscope = dynamic_cast<FileScope*>
               (Ancestor(FILE_SCOPE));
   const char *fname = 0;
   if (fscope!=NULL)
      fname = fscope->FileName();
   
   std::string result ("<A");
   if (fname)
      result = result + " f=\"" + xml::EscapeStr (fname)
               + "\"";
   std::ostringstream oss;
   oss << result << " n=\"" << xml::EscapeStr (namePrx) 
       << xml::EscapeStr(routine_name.c_str()) << "\" b=\"" << getStartLine() 
       << "\" e=\"" << getEndLine() << "\">";
   return (oss.str());
}
   
std::string 
RoutineScope::XMLScopeFooter()
{
   return (std::string ("</P>"));
}

void
RoutineScope::setStartLine(unsigned int _sLine, std::string& _func)
{  
   if (sLine==NO_LINE && _sLine!=NO_LINE)
      sLine = _sLine;
}


std::string 
LoopScope::ToString()
{
   RoutineScope *rscope = dynamic_cast<RoutineScope*>
               (Ancestor(ROUTINE_SCOPE));
   FileScope *fscope = dynamic_cast<FileScope*>
               (Ancestor(FILE_SCOPE));
   const char *rname, *fname;
   if (rscope!=NULL)
      rname = rscope->Name();
   else
      rname = "unknown-routine";
   
   if (fscope!=NULL)
      fname = fscope->FileName();
   else
      fname = "unknown-file";
   
   std::string file_name = fname;
   size_t lastSlash = file_name.rfind("/");
   if (lastSlash != std::string::npos)  // we found a '/'
      fname = fname + lastSlash + 1;
   std::ostringstream oss;
   oss << "L;Lev " << level << ";P:" << rname
       << ";" << fname << "[" << getStartLine() << ";" 
       << getEndLine() << "]";
   return (oss.str());
}

std::string 
LoopScope::XMLLabel() 
{ 
   return std::string ("L"); 
}

std::string 
LoopScope::XMLScopeHeader()
{
   std::ostringstream oss;
   oss << "<L b=\"" << getStartLine() << "\" e=\"" 
       << getEndLine() << "\">";
   return (oss.str());
}

std::string 
LoopScope::XMLScopeFooter()
{
   return (std::string ("</L>"));
}

std::string 
LoopScope::AlienXMLScopeHeader(const char* namePrx)
{
   FileScope *fscope = dynamic_cast<FileScope*>
               (Ancestor(FILE_SCOPE));
   const char *fname = 0;
   if (fscope!=NULL)
      fname = fscope->FileName();
   
   std::string result ("<A");
   if (fname)
      result = result + " f=\"" + xml::EscapeStr (fname)
               + "\"";
   std::ostringstream oss;
   oss << result << " n=\"" << xml::EscapeStr (namePrx) 
       << xml::EscapeStr(ToString().c_str()) << "\" b=\"" << getStartLine() 
       << "\" e=\"" << getEndLine() << "\">";
   return (oss.str());
}

/* the Sun compiler generates *bad* debug info, by assigning
 * the routine start line number to instructions that deal with
 * loading input parameters from the stack, which can sometime be
 * inside loops
 */
void
LoopScope::setStartLine(unsigned int _sLine, std::string& _func)
{  
   if (sLine==NO_LINE || (_sLine!=NO_LINE && _sLine<sLine)) 
   {
      RoutineScope *rscope = (RoutineScope*)Ancestor(ROUTINE_SCOPE);
      BriefAssertion(rscope!=NULL && "This LOOP has no ROUTINE ancestor");
      if (findex!=rscope->getFileIndex() || _func.compare(rscope->RoutineName()) || 
                (_sLine>=rscope->getStartLine() && _sLine<=rscope->getEndLine()))
         sLine = _sLine;
   }
}

/* also, for inlined code it generates debug info with the line number
 * of the inlined code. Try to fix it by makeing sure the line number is
 * within the bounds the routne.
 */
void 
LoopScope::setEndLine(unsigned int _eLine, std::string& _func)
{  
   if (eLine==NO_LINE || (_eLine!=NO_LINE && _eLine>eLine)) 
   {
      RoutineScope *rscope = (RoutineScope*)Ancestor(ROUTINE_SCOPE);
      BriefAssertion(rscope!=NULL && "This LOOP has no ROUTINE ancestor");
      if (findex!=rscope->getFileIndex() || _func.compare(rscope->RoutineName()) || 
                (_eLine>=rscope->getStartLine() && _eLine<=rscope->getEndLine()))
         eLine = _eLine;
   }
}


std::string 
GroupScope::ToString()
{
   std::ostringstream oss;
   oss << "G:" << group_name;
   return (oss.str());
}

std::string 
GroupScope::XMLLabel() 
{ 
   return std::string ("G"); 
}

std::string 
GroupScope::XMLScopeHeader()
{
   std::ostringstream oss;
   oss << "<G n=\"" << xml::EscapeStr(group_name.c_str()) << "\">";
   return (oss.str());
}

std::string 
GroupScope::AlienXMLScopeHeader(const char* namePrx)
{
   FileScope *fscope = dynamic_cast<FileScope*>
               (Ancestor(FILE_SCOPE));
   const char *fname = 0;
   if (fscope!=NULL)
      fname = fscope->FileName();
   else
      fname = "undefined-file";
   
   std::string result ("<A");
   if (fname)
      result = result + " f=\"" + xml::EscapeStr (fname)
               + "\"";
   std::ostringstream oss;
   oss << result << " n=\"" << xml::EscapeStr (namePrx) 
       << xml::EscapeStr(ToString().c_str()) << "\" b=\"" << getStartLine() 
       << "\" e=\"" << getEndLine() << "\">";
   return (oss.str());
}
   
std::string 
GroupScope::XMLScopeFooter()
{
   return (std::string ("</G>"));
}
   
