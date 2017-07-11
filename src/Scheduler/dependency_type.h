/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: dependency_type.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Define dependence type constants and a few helper functions to operate
 * on them.
 */

#ifndef _DEPENDENCY_TYPE_H
#define _DEPENDENCY_TYPE_H


enum DependencyDirection {
  ANTI_DEPENDENCY = -1,
  OUTPUT_DEPENDENCY, 
  TRUE_DEPENDENCY,
  CHANGING_DEPENDENCY };

/* DTYPE_TOP_MARKER is a special name that must appear always as the last
 * dependency type in the enumeration. It is necessary to enable special
 * processing based on the dependency type.
 */
enum DependencyType {
  GP_REGISTER_TYPE = 0,
  ADDR_REGISTER_TYPE,
  MEMORY_TYPE,
  CONTROL_TYPE,
  STRUCTURE_TYPE,
  SUPER_EDGE_TYPE,
  DTYPE_TOP_MARKER };

enum DependencyDistance { ONE_ITERATION = -2, VARYING_DISTANCE, 
       LOOP_INDEPENDENT }; // values > 0 represent the depend distance

#define  ANY_DEPENDENCE_DIRECTION  (-101)
#define  ANY_DEPENDENCE_TYPE  (-101)
#define  ANY_DEPENDENCE_DISTANCE  (-101)
#define  ANY_DEPENDENCE_LEVEL  (-101)
#define  ANY_GRAPH_NODE  ((void*)(-1))

extern const char* dependencyDirectionToString(int dd);
extern const char* dependencyTypeToString(int dt);
extern const char* dependencyDistanceToString(int dd);

extern const char* dependencyDirectionColor(int dd, int dist);
extern const char* dependencyTypeStyle(int dt);

#endif
