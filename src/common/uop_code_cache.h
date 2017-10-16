/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: uop_code_cache.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Decode the native instructions of a basic block into their
 * corresponding micro-operations, and provide direct, linear access to all 
 * the micro-ops in a block.
 */

#ifndef _UOP_CODE_CACHE_H
#define _UOP_CODE_CACHE_H

#include <vector>
#include <list>

#include "miami_types.h"
#include "InstructionDecoder.hpp"
#include "register_class.h"

namespace MIAMI
{

class uOp_t
{
public:
   uOp_t(const instruction_info *_ii, const DecodedInstruction *_di, int _idx = 0) :
      iinfo(_ii), dinst(_di), idx(_idx)
   { }
   
   // a word of caution. Micro-ops are stored in lists, inside objects for
   // decoded instructions. Pointers to micro-op objects are valid as long
   // as the decoded instruction objects are not deallocated. Being stored in a list,
   // I do not have to worry about the list growing and replacement of the objects
   // during reallocation. Elemnts in lists do not have to be contiguous. 
   // However, in my case, the lists are not growing either.
   const instruction_info *iinfo;   // pointer to decoded miro-op
   const DecodedInstruction *dinst; // pointer to the decoded instruction
//   addrtype pc;   // instruction's PC address
   int idx;       // micro-op's index inside the instruction
   // should I store the lists of source and destination registers here,
   // or should I work with operands??
   bool operator== (const uOp_t& op) const
   {
      return (iinfo==op.iinfo && dinst==op.dinst && idx==op.idx);
   }
   bool operator!= (const uOp_t& op) const
   {
      return (iinfo!=op.iinfo || dinst!=op.dinst || idx!=op.idx);
   }
   
   /* check if this micro-op writes the given register.
    * @full specifies if the register's entire bit range must be written
    * @ari  return the actual register (bit range) written by the instruction
    */
   bool Writes(const register_info& ri, bool full, register_info& ari) const;
   
   /* check if this micro-op reads the given register.
    * @full specifies if the read bit range is fully included in the register.
    * @ari  return the actual register (bit range) read by the instruction
    */
   bool Reads(const register_info& ri, bool full, register_info& ari) const;
   
   /* Return the index of the memory operand used by this micro-op.
    * If the uop is not a memory operation, return -1
    */
   int getMemoryOperandIndex() const;
};

// decode and store the uops for one basic block
class UopCodeCache
{
public:
   typedef std::vector<uOp_t> UopArray;
   typedef std::list<DecodedInstruction*> DInstList;
   
   typedef UopArray::iterator  iterator;
   typedef UopArray::const_iterator  const_iterator;
   
   UopCodeCache(addrtype _start, addrtype _end, addrtype _reloc, bool has_call = false);
   UopCodeCache();   // just initialize, no decodig
   ~UopCodeCache();  // deallocate the decoded instructions
   
   int DecodeAtPC(addrtype _pc, int _len);
   int NumberOfUops() const  { return (uops.size()); }
   int NumberOfInstructions() const  { return (dinsts.size()); }
   const uOp_t& UopAtIndex(int i) const
   {
      if (i<0 || i>=(int)uops.size())
         return (invalid_uOp);
      else
         return (uops[i]);
   }

   inline iterator begin()  { return (uops.begin()); }
   inline iterator end()    { return (uops.end()); }
   inline const_iterator begin() const { return (uops.begin()); }
   inline const_iterator end() const   { return (uops.end()); }
   
private:
   static uOp_t invalid_uOp;
   addrtype start_pc, end_pc;  // the start and end pcs of the instruction sequence whose
                               // micro ops are stored in this code cache object
   addrtype reloc;   // a relocation offset used to compute the actual addresses of 
                     // the code in memory
   UopArray uops;    // linear store for the micro-ops of all instructions in this block
   
   // I need also a place to store pointers to the unique decoded instruction objects
   DInstList dinsts;
};

}  /* namespace MIAMI */

#endif
