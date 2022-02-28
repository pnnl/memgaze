/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: dependency_type.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements helper functions to handle dependence types.
 */

#include <stdio.h>
#include <assert.h>
#include <dependency_type.h>


const char* 
dependencyDirectionToString(int dd)
{
   switch(dd)
   {
      case ANTI_DEPENDENCY:
         return "ANTI_DEPENDENCY"; break;
      case OUTPUT_DEPENDENCY:
         return "OUTPUT_DEPENDENCY"; break;
      case TRUE_DEPENDENCY:
         return "TRUE_DEPENDENCY"; break;
      case CHANGING_DEPENDENCY:
         return "CHANGING_DEPENDENCY"; break;
      default:
         assert(! "Invalid dependency direction.");
   }
   return (NULL);
}

const char* 
dependencyTypeToString(int dt) 
{
   switch(dt)
   {
      case GP_REGISTER_TYPE:
         return "GP_REGISTER_TYPE"; break;
      case ADDR_REGISTER_TYPE:
         return "ADDR_REGISTER_TYPE"; break;
      case MEMORY_TYPE:
         return "MEMORY_TYPE"; break;
      case CONTROL_TYPE:
         return "CONTROL_TYPE"; break;
      case STRUCTURE_TYPE:
         return "STRUCTURE_TYPE"; break;
      case SUPER_EDGE_TYPE:
         return "SUPER_EDGE_TYPE"; break;
      default:
         assert(! "Invalid dependency type.");
   }
   return (NULL);
}

const char* 
dependencyDistanceToString(int dd) 
{
   static char buf[64];
   switch(dd)
   {
      case ONE_ITERATION:
         return "ONE_ITERATION(see level)"; break;
      case VARYING_DISTANCE:
         return "VARYING_DISTANCE"; break;
      case LOOP_INDEPENDENT:
         return "LOOP_INDEPENDENT"; break;
      default:
         if (dd>0)  // valid distance
         {
            sprintf(buf, "LOOP_CARRIED(dist=%d)", dd);
            return (buf);
         } else
            assert(! "Invalid dependency distance.");
   }
   return (NULL);
}

const char* 
dependencyDirectionColor(int dd, int dist)
{
   switch(dd)
   {
      case ANTI_DEPENDENCY:
         if (dist==0)
            return "red"; 
         else
            return "salmon";
         break;
      case OUTPUT_DEPENDENCY:
         if (dist==0)
            return "darkgreen";
         else
            return "green"; 
         break;
      case TRUE_DEPENDENCY:
         if (dist==0)
            return "blue"; 
         else 
            return "deepskyblue";
         break;
      case CHANGING_DEPENDENCY:
         if (dist==0)
            return "darkorchid"; 
         else
            return "magenta";
         break;
      default:
         assert(! "Invalid dependency direction.");
   }
   return (NULL);
}

const char* 
dependencyTypeStyle(int dt)
{
   switch(dt)
   {
      case GP_REGISTER_TYPE:
      case ADDR_REGISTER_TYPE:
         return "\"setlinewidth(2),solid\""; break;
      case MEMORY_TYPE:
         return "\"setlinewidth(2),dashed\""; break;
      case CONTROL_TYPE:
         return "\"setlinewidth(2),dotted\""; break;
      case STRUCTURE_TYPE:
         return "\"invis\""; break;
      case SUPER_EDGE_TYPE:
         return "\"setlinewidth(4),dashed\""; break;
      default:
         assert(! "Invalid dependency type.");
   }
   return (NULL);
}


