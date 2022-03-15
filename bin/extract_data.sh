#!/bin/bash
#$1 binary
set -x
bin=$1

#for b in ubench_1D_Str1_x1 ubench_1D_Str1_x2 ubench_1D_Str8_x1 ubench_1D_Str8_x2 ubench_1D_Ind_x1_shfl ubench_1D_Ind_x1_rand ubench_1D_Ind_x2 ubench_1D_Ind_halfx1  ubench_1D_If_halfx1
#do 
   echo "extracting data for  $bin"
   /home/kili337/src/linux-5.5.9/tools/perf/perf script --script=/home/kili337/Projects/Fallacy/scripts/intel-pt-events.py -i ${bin}.data > ${bin}.trace
#done
