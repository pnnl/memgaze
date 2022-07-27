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
#ifndef METRICS_H
#define METRICS_H

// Following what will be in treeFPavgMap we can add new types and
//map <int, <int, double>> *treeFPavgMap
//map <level , <type , FP>
//type 0 = constant, 1 = strided,  2 = insdirect,  -1 = unknown
//type 3 =  totalFP, 4 = growth rate, -2 number of nodes
#define NUMBER_OF_NODES -2
#define UNKNOWN -1
#define CONSTANT 0
#define STRIDED 1
#define INDIRECT 2
#define FP 3
#define IN_SAMPLE -3
#define WINDOW_SIZE -4
#define MULTIPLIER 5
#define TOTAL_LOADS -5
#define CONSTANT2LOAD_RATIO 6
#define NPF_RATE -6
#define NPF_GROWTH_RATE 7
#define GROWTH_RATE -7


#endif
