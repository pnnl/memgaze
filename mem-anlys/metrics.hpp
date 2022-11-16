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
//#define NUMBER_OF_NODES -2
//#define UNKNOWN -1
//#define CONSTANT 0
//#define STRIDED 1
//#define INDIRECT 2
//#define FP 3
//#define IN_SAMPLE -3
//#define WINDOW_SIZE -4
//#define MULTIPLIER 5
//#define TOTAL_LOADS -5
//#define CONSTANT2LOAD_RATIO 6
//#define NPF_RATE -6
//#define NPF_GROWTH_RATE 7
//#define GROWTH_RATE -7
//#define STORE 90

enum Metrics: int16_t{
    NUMBER_OF_NODES    = -2,
    UNKNOWN            = -1,
    CONSTANT           =  0,
    STRIDED            =  1,
    INDIRECT           =  2,
    FP                 =  3,
    IN_SAMPLE          = -3,
    WINDOW_SIZE        = -4,
    MULTIPLIER         =  5,
    TOTAL_LOADS        = -5,
    CONSTANT2LOAD_RATIO=  6,
    NPF_RATE           = -6,
    NPF_GROWTH_RATE    =  7,
    GROWTH_RATE        = -7,
    STORE              = 90,
};


//enum Metrics : uint16_t {
//  STORE,                //0
//  UNKNOWN,              //1
//  CONSTANT,             //2
//  STRIDED,              //3
//  INDIRECT,             //4
//  NUMBER_OF_NODES,      //5
//  FP,                   //6
//  IN_SAMPLE,            //7
//  WINDOW_SIZE,          //8
//  MULTIPLIER,           //9
//  TOTAL_LOADS,          //10
//  CONSTANT2LOAD_RATIO,  //11
//  NPF_RATE,             //12
//  NPF_GROWTH_RATE,      //13
//  GROWTH_RATE,          //14
//};
//

#endif
