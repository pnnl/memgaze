#!/bin/bash
for i in 3 # 0  #0 3
do
  for j in 3 #{1..3}
  do
    echo "./experiment.sh IPDPS/  miniVite_O${i}-v${j} \"-n 300000\" 1 8192 10000000 distBuildLocalMapCounter_dyninst 1 MiniVite_O${i}_v${j}_nf_func_8k_P5M_n500k"
    ./experiment.sh IPDPS/  miniVite_O${i}-v${j} "-n 300000" 1 8192 500000 distBuildLocalMapCounter_dyninst 1 MiniVite_O${i}_v${j}_nf_func_8k_P500k_n300k_sampled
#    ./experiment.sh IPDPS/  miniVite_O${i}-v${j} "-n 10000" 1 MiniVite_O${i}_v${j}_n10k_FULL
#    ./execute.sh PPoPP_Acns/MiniVite_O${i}_v${j}_nf_func_8k_P5M_pm500k_Avg/miniVite_O${i}-v${j}_PTW.hpcstruct PPoPP_Acns/MiniVite_O${i}_v${j}_nf_func_8k_P5M_pm500k_Avg/miniVite_O${i}-v${j}.lc_Fixed PPoPP_Acns/MiniVite_O${i}_v${j}_nf_func_8k_P5M_pm500k_Avg/miniVite_O${i}-v${j}_s8192_p5000000.trace.final 5000000 1 distBuildLocalMapCounter_dyninst
#    ./compile.sh ${FOLDER}/${NAME} ${BIN} """$ARGS""" ${LOAD} ${SIZE} ${PERIOD}  &> ${FOLDER}/${NAME}/RESULTS_${BIN}_P${Period}ms_B${SIZE}_${NAME}_r${i}.txt
  done
done
