#!/bin/bash
set -x
bin=$1
args=$2
SIZE=$3
PERIOD=$4 #default=37000000 = 30ms
full=$5
zero=0

echo "collecting data for $bin with $args"
if [ ${full} -eq ${zero} ]
then
  perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e ref-cycles/period=${PERIOD},aux-sample-size=${SIZE},call-graph=lbr/u -o ${bin}.data -- ./${bin}_PTW $args
  #/home/kili337/src/linux-5.5.9/tools/perf/perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e ref-cycles/period=${PERIOD},aux-sample-size=${SIZE},call-graph=lbr/u -o ${bin}.data -- ./${bin}_PTW $args
   #/home/kili337/src/linux-5.5.9/tools/perf/perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e cpu/umask=0x81,event=0xd0,period=${PERIOD},aux-sample-size=${SIZE}/u -o ${bin}.data -- ./${bin}_PTW $args
#   /home/kili337/src/linux-5.5.9/tools/perf/perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g  -e  branch-instructions/period=${PERIOD},aux-sample-size=${SIZE}/u -o ${bin}.data -- ./${bin}_PTW $args
fi 

if [ ${full} -ne ${zero} ]
then
   perf record -m 4M,4M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -o ${bin}.data -- ./${bin}_PTW $args 
   #/home/kili337/src/linux-5.5.9/tools/perf/perf record -m 4M,4M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -o ${bin}.data -- ./${bin}_PTW $args 
   #/home/kili337/src/linux-5.5.9/tools/perf/perf record -m 4M,4M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u --filter 'filter @distBuildLocalMapCounters' -o ${bin}.data -- ./${bin}_PTW $args 
#   PID=$!
#   ./intel_pt_sampling.sh $PID 0.01 &
#   PID2=$!
#   wait $PID
#   sleep 1
#   kill -9 $PID2
fi 
