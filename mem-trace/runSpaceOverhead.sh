#!/bin/bash
for i in 0 #3 0  #0 3
do
  for j in 1 #{1..3}
  do
    echo "./experiment.sh Space_Overhead/  miniVite_O${i}-v${j} \"-n 300000\" 1 MiniVite_O${i}_v${j}_full"
    ./experiment.sh Space_Overhead  miniVite_O${i}-v${j} "-n 300000" 1 MiniVite_O${i}_v${j}_n300k_FULL
  done
done

#for i in 0 3
#do
#  for j in "pr" "pr_spmv" "cc" "cc_sv" 
#  do
#
#    echo "./experiment.sh Space_Overhead/  ${j}_O${i} \"-g 20 -n 1\" 1 Gap_${j}_O${i}_g22_full"
#    ./experiment.sh Space_Overhead  ${j}_O${i} "-g 22 -n 1" 1 GAP_${j}_O${i}_g22_nf_func_full
#  done
#done
#
#for i in 0 3
#do 
#  echo "Ubench"
#  ./experiment.sh Space_Overhead ubench-500k_O${i} "0xfff" 1 Ubench_O{$i}_500k_full
#done
