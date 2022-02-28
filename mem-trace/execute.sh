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
  ../fp_intel_pt.x -t $INPUT -l  $LC -s  $HPCSTRUCT -o ${OUTPUT}_graph_full.txt -m $LOAD -p $PERIOD 
fi
if [ "$#" -eq 6 ]
then
  #../fp_intel_pt.x $INPUT $LC $HPCSTRUCT ${OUTPUT}_graph.txt $LOAD  $PERIOD $FUNC
  echo "calculate FP for focused view"
  ../fp_intel_pt.x -t $INPUT -l  $LC -s  $HPCSTRUCT -o ${OUTPUT}_graph_${FUNC}_nMull.txt -m $LOAD -p $PERIOD  -f $FUNC
  echo "calculate FP for FULL APP"
  ../fp_intel_pt.x -t $INPUT -l  $LC -s  $HPCSTRUCT -o ${OUTPUT}_graph_wholeApp_nMull.txt -m $LOAD -p $PERIOD 
fi
if [ "$#" -eq 3 ]
then
  #../fp_intel_pt.x $INPUT $LC $HPCSTRUCT ${OUTPUT}_graph.txt 
echo  "../fp_intel_pt.x -t $INPUT -l  $LC -s  $HPCSTRUCT -o ${OUTPUT}_graph_FullTrace.txt "
fi

#mask 0xffffffffff00
