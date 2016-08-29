#!/bin/bash

# Copy this script and edit it as needed.
# First, define the location where the MIAMI framework is installed on your 
# system
MIAMI_ROOT=${HOME}/MIAMI

# You must specify the 'tag' for the profile files to use, and optionally a 
# thread index. By default, data for thread 0 is used.
case $# in
1) tag=$1; tnum=0;;
2) tag=$1; tnum=$2;;
*) echo "Usage: $0 <file_tag>"; exit;;
esac

ttext=`printf "%04d" ${tnum}`

# compute the instruction schedule and the memory reuse related metrics
time ${MIAMI_ROOT}/bin/miami -c ${tag}-00000-${ttext}.cfg \
   -mrd ${tag}-:64:-00000-${ttext}.mrdt -iwmix \
   -m ${MIAMI_ROOT}/share/machines/x86_SandyBridge_1.mdf \
   -o ${tag}_${ttext}_SB -no_pid > log_${tag}_${ttext}_miami_SB.txt 2>&1


# process the streaming data separately, so that the stream metrics are 
# saved in a separate XML file.
time ${MIAMI_ROOT}/bin/miami -c ${tag}-00000-${ttext}.cfg \
   -S ${tag}-L1-00000-${ttext}.srd -xml \
   -o ${tag}_${ttext}_L1 -no_pid > log_${tag}_${ttext}_miami_L1.txt 2>&1

