#!/bin/bash
set -x
if [ "$#" -ne 8 ] && [ "$#" -ne 5 ]
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
SIZE=$4
PERIOD=$5
if [ "$#" -eq 8 ]
then
  RUN=$7
  NAME=$8
  mkdir ${FOLDER}/${NAME}
  cp ${FOLDER}/${BIN} ${FOLDER}/${NAME}
  for i in $(seq 1 $RUN)
  do
    
    CPUCLOCK=$(lscpu | grep 'CPU min MHz' | head -1 | awk -F: '{    print $2/1024}')
    Period=$(awk '{print $1*$2}' <<<"${PERIOD} ${CPUCLOCK}")
    Period=$(awk '{OFMT="%f";print $1/$2}' <<<"${Period} 1000000")
    if [ $i == 1 ]
    then
      ./compile.sh ${FOLDER}/${NAME} ${BIN} """$ARGS""" ${SIZE} ${PERIOD} ${MASK} &> ${FOLDER}/${NAME}/RESULTS_${BIN}_P${Period}ms_B${SIZE}_${NAME}_r${i}.txt
    else 
      ./run.sh ${FOLDER}/${NAME} ${BIN} """$ARGS""" ${SIZE} ${PERIOD} ${MASK} &> ${FOLDER}/${NAME}/RESULTS_${BIN}_P${Period}ms_B${SIZE}_${NAME}_r${i}.txt
    fi
    ./rename.sh ${FOLDER}/${NAME} ${BIN} run_${i} ${SIZE} ${PERIOD} 
  done
fi
if [ "$#" -eq 5 ]
then
  RUN=$4
  NAME=$5
  mkdir ${FOLDER}/${NAME}
  cp ${FOLDER}/${BIN} ${FOLDER}/${NAME}
  for i in $(seq 1 $RUN)
  do
    if [ $i == 1 ]
    then
      ./compile.sh ${FOLDER}/${NAME} ${BIN} """$ARGS"""  &> ${FOLDER}/${NAME}/RESULTS_${BIN}_full_${NAME}_r${i}.txt
    else
      ./run.sh ${FOLDER}/${NAME} ${BIN} """$ARGS"""  &> ${FOLDER}/${NAME}/RESULTS_${BIN}_full_${NAME}_r${i}.txt

    fi
    ./rename.sh ${FOLDER}/${NAME} ${BIN} run_${i} 
  done
fi

