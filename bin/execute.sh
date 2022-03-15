#!/bin/bash
set -x
INPUT=$3
LC=$2
HPCSTRUCT=$1
FUNC=$6
PERIOD=$4
LOAD=$5
OUTPUT=${LC%.lc_Fixed}
if [ "$#" -eq 5 ]
then
  ../fp_intel_pt.x -t $INPUT -l  $LC -s  $HPCSTRUCT -o ${OUTPUT}_graph.txt -m $LOAD -p $PERIOD 
fi
if [ "$#" -eq 6 ]
then
  #../fp_intel_pt.x $INPUT $LC $HPCSTRUCT ${OUTPUT}_graph.txt $LOAD  $PERIOD $FUNC
  ../fp_intel_pt.x -t $INPUT -l  $LC -s  $HPCSTRUCT -o ${OUTPUT}_graph.txt -m $LOAD -p $PERIOD  -f $FUNC
fi
if [ "$#" -eq 3 ]
then
  #../fp_intel_pt.x $INPUT $LC $HPCSTRUCT ${OUTPUT}_graph.txt 
  ../fp_intel_pt.x -t $INPUT -l  $LC -s  $HPCSTRUCT -o ${OUTPUT}_graph.txt 
fi

#mask 0xffffffffff00
