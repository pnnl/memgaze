#!/bin/bash
for i in  3 #0
do
  for j in "pr" "cc" #"cc_sv" "pr_spmv" 
  do
#    if [ $j = 'cc' ]
#    then
#      func="Afforest"
#    elif [ $j = "pr" ];
#    then
#      func="PageRankPullGS"
#    elif [ $j = "pr_spmv" ]
#    then
#      func="PageRankPull"
#    elif [ $j = "cc_sv" ]
#    then
#      func="ShiloachVishkin"
#    elif [ $j = "tc" ]
#    then
#      func="OrderedCount"
#    elif [ $j = "bfs" ]
#    then
#      func="DOBFS"
#    fi
#    echo "./experiment.sh PPoPP_Acns/  ${j}_O${i} \"-g 20 -n 1\" 1 8192 5000000 ${func}_dyninst 1 Gap_${j}_O${i}_nf_func_8k_P5M_g20_n1_Avg"
#    ./experiment.sh PPoPP_Acns/  ${j}_O${i} "-g 20 -n 1" 1 8192 5000000 ${func}_dyninst 1 GAP_${j}_O${i}_g20_nf_func_8k_P5M_n1_Avg
#    echo "./experiment.sh PPoPP_Acns/  ${j}_O${i} \"-u 20 -n 1\" 1 8192 5000000 ${func}_dyninst 1 Gap_${j}_O${i}_nf_func_8k_P5M_g20_n1_Avg"
#    ./experiment.sh PPoPP_Acns/  ${j}_O${i} "-u 20 -n 1" 1 8192 5000000 ${func}_dyninst 1 GAP_${j}_O${i}_u20_nf_func_8k_P5M_n1_Avg

    echo "./experiment.sh IPDPS/  ${j}_O${i} \"-g 20 -n 1\" 1 8192 5000000  1 Gap_${j}_O${i}_nf_func_8k_P5M_g20_n1_Avg"
    #./experiment.sh IPDPS/  ${j}_O${i} "-g 22 -n 1" 1 8192 5000000  1 GAP_${j}_O${i}_g22_nf_func_8k_P5M_n1_Avg
    ./experiment.sh IPDPS/  ${j}_O${i} "-g 22 -n 1" 1 8192 1000000  1 GAP_${j}_O${i}_g22_nf_func_8k_P1M_n1_Avg
    ./experiment.sh IPDPS/  ${j}_O${i} "-g 22 -n 1" 1 8192 500000  1 GAP_${j}_O${i}_g22_nf_func_8k_P500k_n1_Avg
#    ./execute.sh PPoPP_Acns/GAP_${j}_O${i}_g20_nf_func_8k_P5M_n1_Avg/${j}_O${i}_PTW.hpcstruct PPoPP_Acns/GAP_${j}_O${i}_g20_nf_func_8k_P5M_n1_Avg/${j}_O${i}.lc_Fixed PPoPP_Acns/GAP_${j}_O${i}_g20_nf_func_8k_P5M_n1_Avg/${j}_O${i}_s8192_p5000000.trace.final 5000000 1
#    echo "./experiment.sh IPDPS/  ${j}_O${i} \"-u 20 -n 1\" 1 8192 5000000  1 Gap_${j}_O${i}_nf_func_8k_P5M_g20_n1_Avg"
#    ./experiment.sh IPDPS/  ${j}_O${i} "-u 22 -n 1" 1 8192 5000000  1 GAP_${j}_O${i}_u20_nf_func_8k_P5M_n1_Avg
#    ./execute.sh PPoPP_Acns/GAP_${j}_O${i}_u20_nf_func_8k_P5M_n1_Avg/${j}_O${i}_PTW.hpcstruct PPoPP_Acns/GAP_${j}_O${i}_u20_nf_func_8k_P5M_n1_Avg/${j}_O${i}.lc_Fixed PPoPP_Acns/GAP_${j}_O${i}_u20_nf_func_8k_P5M_n1_Avg/${j}_O${i}_s8192_p5000000.trace.final 5000000 1
  done
done
