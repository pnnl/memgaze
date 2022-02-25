#!/bin/bash
set -x
if [ "$#" -ne 6 ] && [ "$#" -ne 3 ]
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
ARGS=$3
MASK=$6
echo $ARGS
echo """$ARGS"""
SIZE=8192
PERIOD=37500000
PWD=$(pwd)
#echo $PWD
#./get_CFG.sh ${FOLDER}/${BIN} \"${ARGS}\"
time ./instument_binary.sh ${FOLDER}/${BIN}
objdump -DC  ${FOLDER}/${BIN} >  ${FOLDER}/${BIN}.dump
objdump -DC  ${FOLDER}/${BIN}_PTW >  ${FOLDER}/${BIN}_PTW.dump
time ./ip_converter.py ${FOLDER}/${BIN}.log ${FOLDER}/${BIN}.lc 
cd ${FOLDER}
#time /home/kili337/tools/hpctoolkit_linemap/bin/hpcstruct ${BIN}_PTW
time hpcstruct ${BIN}_PTW
cd -
echo $#
if [ "$#" -eq 6 ]
then
  SIZE=$4
  PERIOD=$5
  time ./collect_data.sh ${FOLDER}/${BIN} """$ARGS""" ${SIZE} ${PERIOD} 0
  mv ${FOLDER}/${BIN}.data ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}.data
  time ./extract_data.sh ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}
  time ./removeErros.sh ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}.trace
  time ./add_base_IP.py ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}.trace.clean ${FOLDER}/${BIN}_PTW ${FOLDER}/${BIN}.lc_Fixed ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}.trace.final
  CPUCLOCK=$(lscpu | grep 'CPU min MHz' | head -1 | awk -F: '{    print $2/1024}')
  Period=$(awk '{print $1*$2}' <<<"${PERIOD} ${CPUCLOCK}")
  #Period=$(awk '{OFMT="%f";print $1*$2}' <<<"${Period} 0.5")
  time ./execute.sh ${FOLDER}/${BIN}_PTW.hpcstruct ${FOLDER}/${BIN}.lc_Fixed ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}.trace.final $MASK $Period
fi
if [ "$#" -eq 3 ]
then
  time ./collect_data.sh ${FOLDER}/${BIN} """$ARGS""" ${SIZE} ${PERIOD} 1
  mv ${FOLDER}/${BIN}.data ${FOLDER}/${BIN}_full.data
  time ./extract_data.sh ${FOLDER}/${BIN}_full
  time ./removeErros.sh ${FOLDER}/${BIN}_full.trace
  time ./add_base_IP.py ${FOLDER}/${BIN}_full.trace.clean ${FOLDER}/${BIN}_PTW ${FOLDER}/${BIN}.lc_Fixed  ${FOLDER}/${BIN}_full.trace.final
  time ./execute.sh ${FOLDER}/${BIN}_PTW.hpcstruct ${FOLDER}/${BIN}.lc_Fixed ${FOLDER}/${BIN}_full.trace.final
fi

