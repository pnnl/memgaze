#!/bin/bash
set -x

./ip_converter.py ISC/miniVite_O3-v1.log ISC/miniVite_O3-v1.lc
./ip_converter.py ISC/miniVite_O3-v2.log ISC/miniVite_O3-v2.lc
./ip_converter.py ISC/miniVite_O3-v3.log ISC/miniVite_O3-v3.lc


/home/kili337/src/linux-5.5.9/tools/perf/perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u   -g -e cpu/umask=0x81,event=0xd0,period=1000000,aux-sample-size=8192,call-graph=lbr/u -o ISC/miniVite_O3-v1.data -- ./ISC/miniVite_O3-v1_PTW -n 300000
/home/kili337/src/linux-5.5.9/tools/perf/perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u   -g -e cpu/umask=0x81,event=0xd0,period=1000000,aux-sample-size=8192,call-graph=lbr/u -o ISC/miniVite_O3-v2.data -- ./ISC/miniVite_O3-v2_PTW -n 300000
/home/kili337/src/linux-5.5.9/tools/perf/perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u   -g -e cpu/umask=0x81,event=0xd0,period=1000000,aux-sample-size=8192,call-graph=lbr/u -o ISC/miniVite_O3-v3.data -- ./ISC/miniVite_O3-v3_PTW -n 300000

./extract_data.sh ISC/miniVite_O3-v1
./extract_data.sh ISC/miniVite_O3-v2
./extract_data.sh ISC/miniVite_O3-v3

./removeErros.sh ISC/miniVite_O3-v1.trace
./removeErros.sh ISC/miniVite_O3-v2.trace
./removeErros.sh ISC/miniVite_O3-v3.trace

./add_base_IP.py ISC/miniVite_O3-v1.trace.clean ISC/miniVite_O3-v1_PTW ISC/miniVite_O3-v1.lc_Fixed ISC/miniVite_O3-v1.trace.final
./add_base_IP.py ISC/miniVite_O3-v2.trace.clean ISC/miniVite_O3-v2_PTW ISC/miniVite_O3-v2.lc_Fixed ISC/miniVite_O3-v2.trace.final
./add_base_IP.py ISC/miniVite_O3-v3.trace.clean ISC/miniVite_O3-v3_PTW ISC/miniVite_O3-v3.lc_Fixed ISC/miniVite_O3-v3.trace.final

./execute.sh ISC/miniVite_O3-v1.hpcstruct ISC/miniVite_O3-v1.lc_Fixed ISC/miniVite_O3-v1.trace.final 1000000 1 &> ISC/Result-v1
./execute.sh ISC/miniVite_O3-v2.hpcstruct ISC/miniVite_O3-v2.lc_Fixed ISC/miniVite_O3-v2.trace.final 1000000 1 &> ISC/Result-v2
./execute.sh ISC/miniVite_O3-v3.hpcstruct ISC/miniVite_O3-v3.lc_Fixed ISC/miniVite_O3-v3.trace.final 1000000 1 &> ISC/Result-v3
