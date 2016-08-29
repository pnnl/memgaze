/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: position.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data structure that holds information about the position in a file. Used
 * during the parsing of machine description files.
 */

#ifndef _POSITION_H
#define _POSITION_H

namespace MIAMI
{

class Position
{
public:
   void initPosition(const char* _fname, int _line, int _col)
   {
      file_name = _fname; line = _line; column = _col;
   }
   
   inline const char* FileName() const { return (file_name); }
   inline int Line() const { return (line); }
   inline int Column() const { return (column); }

   // increment column
   Position operator+ (int cols) 
   { 
      Position temp;
      temp.file_name = file_name;
      temp.line = line;
      temp.column = column + cols;
      return (temp);
   }
   Position& operator+= (int cols)
   {
      column += cols; return (*this);
   }
   // decrement column
   Position operator- (int cols) 
   { 
      Position temp;
      temp.file_name = file_name;
      temp.line = line;
      temp.column = column - cols;
      return (temp);
   }
   Position& operator-= (int cols)
   {
      column -= cols; return (*this);
   }
   
   // go to start of next line
   Position& operator++ () { line+=1; column=1; return (*this); }
   Position& operator++ (int) { line+=1; column=1; return (*this); }
   // go to start of previous line
   Position& operator-- () { line-=1; column=1; return (*this); }
   Position& operator-- (int) { line-=1; column=1; return (*this); }
   
   // must allow TABS
   Position& tab(int tabSize, int count) 
   { 
      column = tabSize*((column-1)/tabSize) + 1 + count*tabSize;
      return (*this);
   }
   
private:
   const char* file_name;
   int line;
   int column;
};

} /* namespace MIAMI */

#endif
