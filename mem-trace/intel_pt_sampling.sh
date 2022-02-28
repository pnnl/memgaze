#!/bin/bash
if [ "$#" -ne 2 ]; then
   echo "run as"
   echo "./intel_pt_snapshot.sh <pid> <wait time between each sample(second)>"
   exit
fi
#$1 pid
pid=$1
#$2 frequency
freq=$2

while :
do 
   kill -USR2 $pid
   sleep $freq
done
