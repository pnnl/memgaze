#!/bin/bash
set -x
echo "Running ubench-500k_O0 sampled"
#./experiment.sh IPDPS/ ubench-500k_O0 "0xfff" 1 16384 4000 1 UBENCH_O0_500k_16kb_P4k 
#./experiment.sh IPDPS/ ubench-500k_O0 "0xfff" 1 16384 8000 1 UBENCH_O0_500k_16kb_P8k 
#./experiment.sh IPDPS/ ubench-50M_O0 "0xfff" 1 8192 100000 1 UBENCH_O0_500k_8kb_P100k 
#./experiment.sh IPDPS/ ubench-n100-s500k-O0 "0xfff" 1 8192 10000 1 UBENCH_O0_n100_s500k_8kb_P10k 
#./experiment.sh IPDPS/ ubench-n20-s500k-O0 "0xfff" 1 8192 10000 1 UBENCH-O0_n20_s500k_8kb_P10k_validate
#./experiment.sh IPDPS/ ubenchUS-O0-n20-s500k "0xfff" 1 8192 10000 1 UBENCH-US_O0_n20_s500k_8kb_P10k 
#./experiment.sh IPDPS/ ubench-n100-s50k-O0 "0xfff" 1 8192 4000 1 UBENCH_O0_n100_s50k_8kb_P4k 
sleep 10
echo "Running ubench-500k_O0 Full"
#./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0xfff" 1 UBENCH_O0_Usleep_500k_full
#./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0xfff" 1 UBENCH_O0_Usleep_500k_full
#./experiment.sh IPDPS/ ubench_500k_usleep_O0 "0xfff" 1 UBENCH_O0_Usleep_500k_full
#./experiment.sh IPDPS/ ubench-n20-s500k-O0 "0xfff"  1 UBENCH-O0_n20_s500k_8kb_full 
#./experiment.sh IPDPS/ ubenchUS-O0-n20-s500k "0xfff"  1 UBENCH-US_O0_n20_s500k_8kb_full 
sleep 10
echo "Running ubench-500k_O3 sampled"
#./experiment.sh IPDPS/ ubench-500k_O3 "0xfff" 1 8192 4000 1 UBENCH_O3_500k_8kb_P4k
#./experiment.sh IPDPS/ ubench-n100-s500k-O3 "0xfff" 1 8192 10000 1 UBENCH_O3_n100_s500k_8kb_P10k
#./experiment.sh IPDPS/ ubench-n20-s500k-O3 "0xfff" 1 8192 8000 1 UBENCH-O3_n20_s500k_8kb_P8k_validation
#./experiment.sh IPDPS/ ubench-n20-s500k-O3 "0xfff" 1 8192 10000 1 UBENCH-O3_n20_s500k_8kb_P10k_validation
#./experiment.sh IPDPS/ ubench-n20-s500k-O3 "0xfff" 1 16384 10000 1 UBENCH-O3_n20_s500k_16kb_P10k_validation
#./experiment.sh IPDPS/ ubench-O3_n50_500k "0xfff" 1 8192 10000 1 UBENCH-O3_n50_s500k_8kb_P10k_validation
sleep 2
#./experiment.sh IPDPS/ ubench-O3_n100_500k "0xfff" 1 8192 10000 1 UBENCH-O3_n100_s500k_8kb_P10k_validation
#./experiment.sh IPDPS/ ubench-n100-s50k-O3 "0xfff" 1 8192 4000 1 UBENCH_O3_n100_s50k_8kb_P4k
#./experiment.sh IPDPS/ ubench-50M_O3 "0xfff" 1 8192 100000 1 UBENCH_O3_500k_8kb_P100k
sleep 10
echo "Running ubench-500k_O3 full" 
#./experiment.sh IPDPS/ ubenchUS-O3-n20-s500k "0xfff"  1 UBENCH-US_O3_n20_s500k_8kb_full_validation
./experiment.sh IPDPS/ ubenchUS-O3-n100-s500k "0xfff"  1 UBENCH-US_O3_n100_s500k_full_validation
#./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0xfff" 1  UBENCH_O3_Usleep_500k_full
#./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0xfff" 1  UBENCH_O3_Usleep_500k_full
#./experiment.sh IPDPS/ ubench_500k_usleep_O3 "0xfff" 1  UBENCH_O3_Usleep_500k_full
echo "Usleep"
#./experiment.sh IPDPS/ ubenchUS-O0-n20-s500k "0xfff"  1 UBENCH-US_O0_n20_s500k_8kb_full 
#./experiment.sh IPDPS/ ubenchUS-O0-n20-s500k "0xfff" 1 8192 10000 1 UBENCH-US_O0_n20_s500k_8kb_P10k 
