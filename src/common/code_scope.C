/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: code_scope.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Base class for building a program scope tree.
 */

#include <iostream>
#include "code_scope.h"
#include "Assertion.h"

using namespace MIAMI;

CodeScope::CodeScope(ScopeImplementation *_parent, ScopeType _stype, 
       CodeScopeKey _key) : UiCodeMap(), scope_type(_stype)
{
   sLine = eLine = NO_LINE;
   findex = 0;
   parent = NULL;
   if (_parent != NULL)
      Link(_parent, _key);
}
   
void 
CodeScope::Link(ScopeImplementation *_parent, CodeScopeKey _key)
{
   // First check if this scope is already linked
   if (parent!=NULL)
      Unlink();
   parent = _parent;
   ((CodeScope*)parent)->insert(CodeScope::value_type(_key, 
                (ScopeImplementation*)this));
   key = _key;
}
   
void 
CodeScope::Unlink()
{
   BriefAssertion(parent);
   CodeScope::iterator it = ((CodeScope*)parent)->find(key);
   BriefAssertion(it!=((CodeScope*)parent)->end() && 
                  ((CodeScope*)it->second)==this);
   ((CodeScope*)parent)->erase(it);
   parent = NULL;
}

int 
CodeScope::getAncestorDistance (CodeScope *o_scope)
{
   int distance = 0;
   CodeScope *s_ptr = this;
   while (s_ptr && o_scope!=s_ptr)
   {
      ++ distance;
      s_ptr = (CodeScope*)s_ptr->parent;
   }
   if (o_scope == s_ptr)
      return (distance);
   else
      return (-1);
}

ScopeImplementation*
CodeScope::Ancestor(ScopeType stype)
{
   CodeScope *iter = (CodeScope*)parent;
   while(iter && iter->scope_type!=stype)
      iter = (CodeScope*)iter->parent;
   return ((ScopeImplementation*)iter);
}

ScopeImplementation*
CodeScope::Descendant(CodeScopeKey skey)
{
   CodeScope::iterator csit = find(skey);
   if (csit != end())
      return (csit->second);
   else  // not a direct child of this scope, I have to test recursively for each child
   {
      ScopeImplementation *child = 0;
      for (csit=begin() ; csit!=end() ; ++csit)
         if ((child=((CodeScope*)csit->second)->Descendant(skey)) != 0)
            return (child);
      // not an indirect descendant of this scope, return 0
      return (0);
   }
}

// Find a descendant scope with the key name
// Return NULL if not found
ScopeImplementation*
CodeScope::FindScopeByName(const std::string &sname)
{
   std::string name = ToString();
   if (name == sname)
   {
      return ((ScopeImplementation*)this);
   }
   else
   {
      CodeScope::iterator csit;
      ScopeImplementation *child = 0;
      for (csit=begin() ; csit!=end() ; ++csit)
         if ((child=((CodeScope*)csit->second)->FindScopeByName(sname)) != 0)
            return (child);
      // not an indirect descendant of this scope, return 0
      return (0);
   }
}

void 
CodeScope::addSourceFileInfo(unsigned int _findex, std::string& _func, 
              unsigned int _sLine, unsigned int _eLine)
{
   if (_sLine==NO_LINE && _eLine==NO_LINE)
      return;
   
   // adjust the start and end lines for this scope as needed
   if (!findex || findex==_findex)
   {
      if (! findex)   // first time we call this method, use this file index
         findex = _findex;
      setStartLine(_sLine, _func);
      setEndLine(_eLine, _func);
   }
   
   // add these numbers to our local line mapping
   MIAMIU::Ui2SetMap::iterator uit = lineMapping.find(_findex);
   if (uit == lineMapping.end())
      uit = lineMapping.insert(lineMapping.begin(), 
             MIAMIU::Ui2SetMap::value_type(_findex, MIAMIU::UISet()));
   if (_sLine != NO_LINE)
      uit->second.insert(_sLine);
   if (_eLine!=NO_LINE && _eLine!=_sLine)
      uit->second.insert(_eLine);
}

/* the Sun compiler generates *bad* debug info, by assigning
 * the routine start line number to instructions that deal with
 * loading input parameters from the stack, which can sometime be
 * inside loops
 */
void
CodeScope::setStartLine(unsigned int _sLine, std::string& _func)
{  
   if (sLine==NO_LINE || (_sLine!=NO_LINE && _sLine<sLine)) 
      sLine = _sLine;
}

/* also, for inlined code it generates debug info with the line number
 * of the inlined code. Try to fix it by makeing sure the line number is
 * within the bounds the routne.
 */
void 
CodeScope::setEndLine(unsigned int _eLine, std::string& _func)
{  
   if (eLine==NO_LINE || (_eLine!=NO_LINE && _eLine>eLine)) 
      eLine = _eLine;
}
   
CodeScope::~CodeScope()
{
   CodeScope::iterator it;
   for(it=begin() ; it!=end() ; ++it)
   {
      delete ((CodeScope*)it->second);
   }
   clear();
}
