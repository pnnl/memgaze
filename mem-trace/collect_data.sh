#!/bin/bash
set -x
bin=$1
args=$2
SIZE=$3
PERIOD=$4 #default=37000000 = 30ms
full=$5
zero=0
LOAD=$6
CURR=$7
FUNC=$8

echo "collecting data for $bin with $args"
if [ ${full} -eq ${zero} ]
then
  if [ ${LOAD} -eq ${zero} ]
  then
    /home/kili337/src/linux-5.5.9/tools/perf/perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e ref-cycles/period=${PERIOD},aux-sample-size=${SIZE},call-graph=lbr/u -o ${bin}.data -- ./${bin}_PTW $args
    #/home/kili337/src/linux-5.5.9/tools/perf/perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e cycles/period=${PERIOD},aux-sample-size=${SIZE},call-graph=lbr/u -o ${bin}.data -- ./${bin}_PTW $args
  else
    if [ "$#" -eq 8 ]
    then
      #FILTER=("""filter ${FUNC} @ ${CURR}/${bin}_PTW""")
      FILTER_S=("""start ${FUNC} @ ${CURR}/${bin}_PTW""")
      FILTER_E=("""stop distExecuteLouvainIteration_dyninst @ ${CURR}/${bin}_PTW""")
      echo "'"$FILTER"'"
      /home/kili337/src/linux-5.5.9/tools/perf/perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u --filter ''"${FILTER_S}"'' --filter ''"${FILTER_E}"'' -g -e cpu/umask=0x81,event=0xd0,period=${PERIOD},aux-sample-size=${SIZE},call-graph=lbr/u -o ${bin}.data -- ./${bin}_PTW $args
    else
      /home/kili337/src/linux-5.5.9/tools/perf/perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u  -g -e cpu/umask=0x81,event=0xd0,period=${PERIOD},aux-sample-size=${SIZE},call-graph=lbr/u -o ${bin}.data -- ./${bin}_PTW $args
    fi
  fi
fi 

if [ ${full} -ne ${zero} ]
then
   /home/kili337/src/linux-5.5.9/tools/perf/perf record -m 4M,4M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -o ${bin}.data -- ./${bin}_PTW $args 
   #/home/kili337/src/linux-5.5.9/tools/perf/perf record -m 4M,4M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u --filter 'filter @distBuildLocalMapCounters' -o ${bin}.data -- ./${bin}_PTW $args 
#   PID=$!
#   ./intel_pt_sampling.sh $PID 0.01 &
#   PID2=$!
#   wait $PID
#   sleep 1
#   kill -9 $PID2
fi 
