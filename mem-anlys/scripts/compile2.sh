#!/bin/bash
set -x
if [ "$#" -ne 5 ] && [ "$#" -ne 3 ]
then
  echo "Run in following format args should be in \"\" and put empty \"\" if no args"
  echo "./compile.sh <FOLDER> <BIN> <ARGS> <BufferSIZE(def=8192)> <PERIOD(def=37500000)>"
  echo "if you want to get full trace run the following"
  echo "./compile.sh <FOLDER> <BIN> <ARGS>"
  exit 0
fi

FOLDER=$1
BIN=$2
ARGS=$3
SIZE=8192
PERIOD=37500000
PWD=$(pwd)
#echo $PWD
#./get_CFG.sh ${FOLDER}/${BIN} \"${ARGS}\"
./instument_binary2.sh ${FOLDER}/${BIN}
./ip_converter.py ${FOLDER}/${BIN}.log ${FOLDER}/${BIN}.lc 
cd ${FOLDER}
/home/kili337/tools/hpctoolkit_linemap/bin/hpcstruct ${BIN}_PTW
cd -

if [ "$#" -eq 5 ]
then
  SIZE=$4
  PERIOD=$5
  ./collect_data.sh ${FOLDER}/${BIN} \"${ARGS}\" ${SIZE} ${PERIOD} 0
  mv ${FOLDER}/${BIN}.data ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}.data
  ./extract_data.sh ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}
  ./removeErros.sh ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}.trace
  ./execute.sh ${FOLDER}/${BIN}_PTW.hpcstruct ${FOLDER}/${BIN}.lc_Fixed ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}.trace.clean
fi
if [ "$#" -eq 3 ]
then
  ./collect_data.sh ${FOLDER}/${BIN} \"${ARGS}\" ${SIZE} ${PERIOD} 1
  mv ${FOLDER}/${BIN}.data ${FOLDER}/${BIN}_full.data
  ./extract_data.sh ${FOLDER}/${BIN}_full
  ./removeErros.sh ${FOLDER}/${BIN}_full.trace
  ./execute.sh ${FOLDER}/${BIN}_PTW.hpcstruct ${FOLDER}/${BIN}.lc_Fixed ${FOLDER}/${BIN}_full.trace.clean
fi

