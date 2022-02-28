#!/bin/bash
set -x


#for i in 0 3
#do 
  for mask  in "0x001"  #"0x002" "0x004" "0x008" "0x200" "0x010" "0x040" "0x080" "0x100" #"0x001"
  do
    bench=ubench_1D_Str1_x1_dyninst
    if [[ $mask == "0x001" ]]
    then
      bench=ubench_1D_Str1_x1_dyninst
    elif [[ $mask == "0x002" ]]
    then
      bench=ubench_1D_Str1_x2_dyninst
    elif [[ $mask == "0x004" ]]
    then
      bench=ubench_1D_Str8_x1_dyninst
    elif [[ $mask == "0x008" ]]
    then
      bench=ubench_1D_Str8_x2_dyninst
    elif [[ $mask == "0x200" ]]
    then
      bench=ubench_multi_func_dyninst
    elif [[ $mask == "0x010" ]]
    then
      bench=ubench_1D_Ind_x1_shfl_dyninst
    elif [[ $mask == "0x040" ]]
    then
      bench=ubench_1D_Ind_x2_dyninst
    elif [[ $mask == "0x080" ]]
    then
      bench=ubench_1D_Ind_halfx1_dyninst
    elif [[ $mask == "0x100" ]]
    then
      bench=ubench_1D_If_halfx1_dyninst
    fi
    ./experiment.sh LDLAT/ ubench_500k_usleep_O0 """$mask""" 1 8192 10000 $bench 1 UB-US_O0-$bench-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O0 """$mask""" 1 8192 10000 $bench 1 UB_O0-$bench-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O0 """$mask""" 1 UB-US_O0-$bench-Full 
    ./experiment.sh LDLAT/ ubench_500k_usleep_O3 """$mask""" 1 8192 10000 $bench 1 UB-US_O3-$bench-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O3 """$mask""" 1 8192 10000 $bench 1 UB_O3-$bench-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 """$mask""" 1 UB-US_O3-$bench-Full  
  done
#done
#
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x001" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O0 "0x001" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x001" 1 UB-US_O0-ubench_1D_Str1_x1-Full 
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x001" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O3 "0x001" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x001" 1 UB-US_O3-ubench_1D_Str1_x1-Full 
#
#./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x002" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O0 "0x002" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x002" 1 UB-US_O0-ubench_1D_Str1_x1-Full 
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x002" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O3 "0x002" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x002" 1 UB-US_O3-ubench_1D_Str1_x1-Full  
#
#./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x004" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O0 "0x004" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x004" 1 UB-US_O0-ubench_1D_Str1_x1-Full 
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x004" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O3 "0x004" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x004" 1 UB-US_O3-ubench_1D_Str1_x1-Full  
#
#./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x008" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O0 "0x008" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x008" 1 UB-US_O0-ubench_1D_Str1_x1-Full 
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x008" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O3 "0x008" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x008" 1 UB-US_O3-ubench_1D_Str1_x1-Full  
#
#./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x200" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O0 "0x200" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x200" 1 UB-US_O0-ubench_1D_Str1_x1-Full 
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x200" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O3 "0x200" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x200" 1 UB-US_O3-ubench_1D_Str1_x1-Full  
#
#./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x010" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O0 "0x010" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x010" 1 UB-US_O0-ubench_1D_Str1_x1-Full 
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x010" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O3 "0x010" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x010" 1 UB-US_O3-ubench_1D_Str1_x1-Full  
#
#./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x040" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O0 "0x040" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x040" 1 UB-US_O0-ubench_1D_Str1_x1-Full 
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x040" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O3 "0x040" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x040" 1 UB-US_O3-ubench_1D_Str1_x1-Full  
#
#./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x080" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O0 "0x080" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x080" 1 UB-US_O0-ubench_1D_Str1_x1-Full 
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x080" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O3 "0x080" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x080" 1 UB-US_O3-ubench_1D_Str1_x1-Full  
#
#
#./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x100" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O0 "0x100" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O0-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0x100" 1 UB-US_O0-ubench_1D_Str1_x1-Full 
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x100" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB-US_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench-500k_O3 "0x100" 1 8192 100000 ubench_1D_Str1_x1_dyninst 1 UB_O3-ubench_1D_Str1_x1-Sampled  
#    ./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0x100" 1 UB-US_O3-ubench_1D_Str1_x1-Full  
#
#
#
#
#  done
#done
