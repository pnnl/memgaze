#!/bin/bash
#set -x

for f in "ubench_1D_Str8_x1_dyninst" "ubench_1D_Str8_x2_dyninst" "ubench_1D_Str1_x1_dyninst" "ubench_1D_Str1_x2_dyninst" "ubench_1D_Ind_x1_shfl_dyninst" "ubench_1D_Ind_x1_rand_dyninst" "ubench_1D_Ind_x2_dyninst" "ubench_1D_Ind_halfx1_dyninst" "ubench_1D_If_halfx1_dyninst" 
do 
  echo ${f}
#  ../fp_intel_pt.x -t test//testUB_focused_multi_func_P1k/ubench-500K_O0_s8192_p1000.trace.final -l test//testUB_focused_multi_func_P1k/ubench-500K_O0.lc_Fixed -s test//testUB_focused_multi_func_P1k/ubench-500K_O0_PTW.hpcstruct -o test//testUB_focused_multi_func_P1k/ubench-500K_O0_graph_ubench_multi_func_dyninst.txt -m 1 -p 1000 -f ${f}
  ../fp_intel_pt.x -t IPDPS//UBENCH_O0_500k_8kb_P10k/ubench-500k_O0_s8192_p10000.trace.final -l IPDPS//UBENCH_O0_500k_8kb_P10k/ubench-500k_O0.lc_Fixed -s IPDPS//UBENCH_O0_500k_8kb_P10k/ubench-500k_O0_PTW.hpcstruct -o IPDPS//UBENCH_O0_500k_8kb_P10k/ubench-500k_O0_graph_${f}.txt -m 1 -p 10000 -f ${f}
  ../fp_intel_pt.x -t IPDPS//UBENCH_O3_500k_8kb_P10k/ubench-500k_O3_s8192_p10000.trace.final -l IPDPS//UBENCH_O3_500k_8kb_P10k/ubench-500k_O3.lc_Fixed -s IPDPS//UBENCH_O3_500k_8kb_P10k/ubench-500k_O3_PTW.hpcstruct -o IPDPS//UBENCH_O3_500k_8kb_P10k/ubench-500k_O3_graph_${f}.txt -m 1 -p 10000 -f ${f}
  ../fp_intel_pt.x -t IPDPS//UBENCH_O0_Usleep_500k_full/ubench_500k_usleep_O0_full.trace.final -l IPDPS//UBENCH_O0_Usleep_500k_full/ubench_500k_usleep_O0.lc_Fixed -s IPDPS//UBENCH_O0_Usleep_500k_full/ubench_500k_usleep_O0_PTW.hpcstruct -o IPDPS//UBENCH_O0_Usleep_500k_full/ubench_500k_usleep_O0_graph_FullTrace_${f}.txt -f ${f}
  ../fp_intel_pt.x -t IPDPS//UBENCH_O3_Usleep_500k_full/ubench_500k_usleep_O3_full.trace.final -l IPDPS//UBENCH_O3_Usleep_500k_full/ubench_500k_usleep_O3.lc_Fixed -s IPDPS//UBENCH_O3_Usleep_500k_full/ubench_500k_usleep_O3_PTW.hpcstruct -o IPDPS//UBENCH_O3_Usleep_500k_full/ubench_500k_usleep_O3_graph_FullTrace_${f}.txt -f ${f}
done
