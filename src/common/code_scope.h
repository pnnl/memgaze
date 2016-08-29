/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: code_scope.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Base class for building a program scope tree.
 */

#ifndef _CODE_SCOPE_H_
#define _CODE_SCOPE_H_

#include <map>
#include <string>
#include "miami_types.h"
#include "uipair.h"

namespace MIAMI
{

#define NO_LINE  0
#define IS_ABSTRACT_CLASS  virtual void is_abstract_class() = 0
#define IS_CONCRETE_CLASS  void is_abstract_class() {}

class ScopeImplementation;

/* Should it be a multimap instead?
 */
typedef addrtype CodeScopeKey;
typedef std::map<CodeScopeKey, ScopeImplementation*> UiCodeMap;
typedef enum {UNDEF_SCOPE, PROG_SCOPE, IMAGE_SCOPE, FILE_SCOPE, ROUTINE_SCOPE, 
        LOOP_SCOPE, GROUP_SCOPE} ScopeType;

class CodeScope : public UiCodeMap
{
public:
   IS_ABSTRACT_CLASS;
   
   CodeScope(ScopeImplementation *_parent, ScopeType _stype, 
              CodeScopeKey _key);
   
   void Link(ScopeImplementation *_parent, CodeScopeKey _key);
   void Unlink();
   
   ScopeImplementation *Ancestor(ScopeType stype);
   ScopeImplementation *Descendant(CodeScopeKey key);
   ScopeImplementation *FindScopeByName(const std::string &sname);
   
   virtual std::string ToString() = 0;
   virtual std::string XMLLabel() = 0;
   virtual std::string XMLScopeHeader () = 0;
   virtual std::string XMLScopeFooter () = 0;
   virtual std::string AlienXMLScopeHeader (const char* namePrx = "") = 0;
   virtual int Level () const = 0;
   
   virtual ~CodeScope();
   
   void addSourceFileInfo(unsigned int _findex, std::string& _func, 
             unsigned int _sLine, unsigned int _eLine);
   
   virtual void setStartLine(unsigned int _sLine, std::string& _func);
   virtual void setEndLine(unsigned int _eLine, std::string& _func);
   
   inline unsigned int getFileIndex() const { return (findex); }
   inline unsigned int getStartLine() const { return (sLine); }
   inline unsigned int getEndLine() const   { return (eLine); }
   
   inline void resetLineInfo() { sLine = eLine = NO_LINE; }
   inline ScopeImplementation* Parent() { return (parent); }
   
   ScopeType getScopeType() { return (scope_type); }
   
   int getAncestorDistance (CodeScope *o_scope);
   const MIAMIU::Ui2SetMap& GetLineMappings() const   { return (lineMapping); }
   void setLineMappings(MIAMIU::Ui2SetMap mapping){
      lineMapping = mapping;
   }
   
   inline CodeScopeKey getScopeKey() const { return (key); }
   
protected:
   unsigned int sLine, eLine;
   unsigned int findex;

private:
   CodeScopeKey key;
   ScopeImplementation *parent;
   ScopeType scope_type;
   // maintain a complete line mapping for each scope, not just start and 
   // end lines.
   // We use a map, where the key is a file index and the value is a set of 
   // line numbers from that file
   MIAMIU::Ui2SetMap lineMapping;
};

} /* namespace MIAMI */

#endif
