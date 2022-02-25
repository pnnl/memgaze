#!/bin/bash
set -x
INPUT=$3
LC=$2
HPCSTRUCT=$1
MASK=$4
PERIOD=$5
if [ "$#" -eq 5 ]
then
  ../../fp_intel_pt.x $INPUT $LC $HPCSTRUCT output.txt $MASK $PERIOD
fi
if [ "$#" -eq 3 ]
then
../../fp_intel_pt.x $INPUT $LC $HPCSTRUCT output.txt 
fi

#mask 0xffffffffff00
