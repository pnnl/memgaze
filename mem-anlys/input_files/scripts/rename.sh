#!/bin/bash
set -x
if [ "$#" -ne 5 ] && [ "$#" -ne 3 ]
then
  echo "$#"
  echo "Run in following format args should be in \"\" and put empty \"\" if no args"
  echo "./compile.sh <FOLDER> <BIN> <ARGS> <BufferSIZE(def=8192)> <PERIOD(def=37500000)> <Mask(Rec=0xffffffffff00)>"
  echo "if you want to get full trace run the following"
  echo "./compile.sh <FOLDER> <BIN> <ARGS>"
  exit 0
fi

FOLDER=$1
BIN=$2
RENAME=$3
SIZE=$4
PWD=$(pwd)
if [ "$#" -eq 5 ]
then
  PERIOD=$5
  per=$5
  echo ${PERIOD}
  CPUCLOCK=$(lscpu | grep 'CPU min MHz' | head -1 | awk -F: '{    print $2/1024}')
  PERIOD=$(awk '{print $1*$2}' <<<"${PERIOD} ${CPUCLOCK}")
  echo ${PERIOD}
  PERIOD=$(awk '{OFMT="%d";print $1/$2}' <<<"${PERIOD} 1000000")
  echo ${PERIOD}

  cp ${FOLDER}/${BIN}.log ${FOLDER}/${BIN}_P${PERIOD}ms_B${SIZE}_${RENAME}.log
  cp ${FOLDER}/${BIN}.dump ${FOLDER}/${BIN}_P${PERIOD}ms_B${SIZE}_${RENAME}.dump
  cp ${FOLDER}/${BIN}_PTW.dump ${FOLDER}/${BIN}_PTW_P${PERIOD}ms_B${SIZE}_${RENAME}.dump
  cp ${FOLDER}/${BIN}.lc_Fixed ${FOLDER}/${BIN}_P${PERIOD}ms_B${SIZE}_${RENAME}.lc_Fixed 
  cp ${FOLDER}/${BIN}_PTW.hpcstruct ${FOLDER}/${BIN}_PTW_P${PERIOD}ms_B${SIZE}_${RENAME}.hpcstruct
  cp ${FOLDER}/${BIN}_s${SIZE}_p${per}.data ${FOLDER}/${BIN}_P${PERIOD}ms_B${SIZE}_${RENAME}.data
  cp ${FOLDER}/${BIN}_s${SIZE}_p${per}.trace ${FOLDER}/${BIN}_P${PERIOD}ms_B${SIZE}_${RENAME}.trace
  cp ${FOLDER}/${BIN}_s${SIZE}_p${per}.trace.clean ${FOLDER}/${BIN}_P${PERIOD}ms_B${SIZE}_${RENAME}.trace.clean
  cp ${FOLDER}/${BIN}_s${SIZE}_p${per}.trace.final ${FOLDER}/${BIN}_P${PERIOD}ms_B${SIZE}_${RENAME}.trace.final
fi
if [ "$#" -eq 3 ]
then
  cp ${FOLDER}/${BIN}.log ${FOLDER}/${BIN}_${RENAME}.log
  cp ${FOLDER}/${BIN}.dump ${FOLDER}/${BIN}_${RENAME}.dump
  cp ${FOLDER}/${BIN}_PTW.dump ${FOLDER}/${BIN}_PTW_${RENAME}.dump
  cp ${FOLDER}/${BIN}.lc_Fixed ${FOLDER}/${BIN}_${RENAME}.lc_Fixed 
  cp ${FOLDER}/${BIN}_PTW.hpcstruct ${FOLDER}/${BIN}_PTW_${RENAME}.hpcstruct
  cp ${FOLDER}/${BIN}_full.data ${FOLDER}/${BIN}_full_${RENAME}.data
  cp ${FOLDER}/${BIN}_full.trace ${FOLDER}/${BIN}_full_${RENAME}.trace
  cp ${FOLDER}/${BIN}_full.trace.clean ${FOLDER}/${BIN}_full_${RENAME}.trace.clean
  cp ${FOLDER}/${BIN}_full.trace.final ${FOLDER}/${BIN}_full_${RENAME}.trace.final
fi

