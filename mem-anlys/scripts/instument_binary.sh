#!/bin/bash
#$1 = binary
bin=$1
#for b in ubench_1D_Str1_x1 ubench_1D_Str1_x2 ubench_1D_Str8_x1 ubench_1D_Str8_x2 ubench_1D_Ind_x1_shfl ubench_1D_Ind_x1_rand ubench_1D_Ind_x2 ubench_1D_Ind_halfx1  ubench_1D_If_halfx1
#do 
   echo "Instrumenting binary"
#   /home/kili337/Projects/IPPD/gitlab/palm-memory/install/bin/miami  --bin_path ${bin}  --load_class 1 --inst_loads=1 --inst_stores=1 --inst_strided=1 --inst_indirect=1 --inst_frame=0  &> ${bin}.log
   /home/kili337/Projects/IPPD/gitlab/palm-memory/install/bin/miami  -m ~/x86_SandyBridge_EP.mdf  --bin_path ${bin}  --load_class 1 --inst_loads=1 --inst_stores=0 --inst_strided=1 --inst_indirect=1 --inst_frame=0 --lcFile=${bin}.lc   ${bin}.log &> ${bin}.log
   #mv ${bin}_PTW ${bin}.out
#done
