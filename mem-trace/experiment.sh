#!/bin/bash
set -x
if [ "$#" -ne 9 ] && [ "$#" -ne 5 ] &&[ "$#" -ne 8 ]
then
  echo "$#"
  echo "Run in following format args should be in \"\" and put empty \"\" if no args"
  echo "./experiment.sh <FOLDER> <BIN> <ARGS> <0(time)/1(load)>  <BufferSIZE(def=8192)> <PERIOD(def=37500000)> <Mask(Rec=0xffffffffffff)> <# of runs> <folderName>"
  echo "if you want to get full trace run the following"
  echo "./experiment.sh <FOLDER> <BIN> <ARGS>< # of runs> <folderName> "
  exit 0
fi

FOLDER=$1
BIN=$2
ARGS=$3
LOAD=$4
FUNC=$7
echo $ARGS
echo """$ARGS"""
SIZE=$5
PERIOD=$6
if [ "$#" -eq 8 ]
then
  RUN=$7
  NAME=$8
  mkdir ${FOLDER}/${NAME}
  cp ${FOLDER}/${BIN} ${FOLDER}/${NAME}
  for i in $(seq 1 $RUN)
  do
    if [ ${LOAD} -eq 0 ]
    then
      CPUCLOCK=$(lscpu | grep 'CPU min MHz' | head -1 | awk -F: '{    print $2/1024}')
      Period=$(awk '{print $1*$2}' <<<"${PERIOD} ${CPUCLOCK}")
      Period=$(awk '{OFMT="%f";print $1/$2}' <<<"${Period} 1000000")
    fi
    if [ $i == 1 ]
    then
      ./compile.sh ${FOLDER}/${NAME} ${BIN} """$ARGS""" ${LOAD} ${SIZE} ${PERIOD}  &> ${FOLDER}/${NAME}/RESULTS_${BIN}_P${Period}ms_B${SIZE}_${NAME}_r${i}.txt
    else 
      ./run.sh ${FOLDER}/${NAME} ${BIN} """$ARGS""" ${LOAD} ${SIZE} ${PERIOD}  &> ${FOLDER}/${NAME}/RESULTS_${BIN}_P${Period}ms_B${SIZE}_${NAME}_r${i}.txt
    fi
#    ./rename.sh ${FOLDER}/${NAME} ${BIN} run_${i} ${SIZE} ${PERIOD} 
  done
fi


if [ "$#" -eq 9 ]
then
  RUN=$8
  NAME=$9
  mkdir ${FOLDER}/${NAME}
  cp ${FOLDER}/${BIN} ${FOLDER}/${NAME}
  for i in $(seq 1 $RUN)
  do
    if [ ${LOAD} -eq 0 ]
    then
      CPUCLOCK=$(lscpu | grep 'CPU min MHz' | head -1 | awk -F: '{    print $2/1024}')
      Period=$(awk '{print $1*$2}' <<<"${PERIOD} ${CPUCLOCK}")
      Period=$(awk '{OFMT="%f";print $1/$2}' <<<"${Period} 1000000")
    fi
    if [ $i == 1 ]
    then
      ./compile.sh ${FOLDER}/${NAME} ${BIN} """$ARGS""" ${LOAD} ${SIZE} ${PERIOD} ${FUNC} &> ${FOLDER}/${NAME}/RESULTS_${BIN}_P${Period}ms_B${SIZE}_${NAME}_r${i}.txt
    else 
      ./run.sh ${FOLDER}/${NAME} ${BIN} """$ARGS""" ${LOAD} ${SIZE} ${PERIOD} ${FUNC} &> ${FOLDER}/${NAME}/RESULTS_${BIN}_P${Period}ms_B${SIZE}_${NAME}_r${i}.txt
    fi
#    ./rename.sh ${FOLDER}/${NAME} ${BIN} run_${i} ${SIZE} ${PERIOD} 
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
#    ./rename.sh ${FOLDER}/${NAME} ${BIN} run_${i} 
  done
fi

