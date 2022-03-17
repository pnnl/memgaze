-*-Mode: markdown;-*-

$Id$

-----------------------------------------------------------------------------
MemGaze Pipeline
=============================================================================

MemGaze has 3 mains steps. To run all three main steps use compile.sh 

```./compile.sh <FOLDER> <BIN> <ARGS> <0(time)/1(load)> <BufferSIZE(def=8192)> <PERIOD(def=37500000)> <MaskedBits(def=0)> <FUNCTION-NAME if need to focus>```

1-) Analysis and Instrumentation of the Binary (bin-anyls): 
  
  ```./instument_binary.sh ${FOLDER}/${BIN}```

  -> Read binary with dyninst

  -> Create internal CFG for each Routine

  -> Analyze each routine to find each load and their types ( strided, indirect, constant)

  -> Instrument with ptwrite for selective loads

  -> Print original and instrumented IP address

  -> Print load classification file

  -> Adjust IP addresses
  
  ```./ip_converter.py ${FOLDER}/${BIN}.log ${FOLDER}/${BIN}.binanlys```

  -> Create hpcstruct file
  
  ```hpcstruct ${BIN}_PTW```

2-) Trace Collection (mem-trace)
  
  -> Run Instrumented Binary with perf
  
  ```./collect_data.sh ${FOLDER}/${BIN} """$ARGS""" ${SIZE} ${PERIOD} 0 ${LOAD}```
  
  -> Extract data file with perf-script by using a python script
  
  ```./extract_data.sh ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}```
  
  -> Remove data collection errors from trace
  
  ```./removeErros.sh ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}.trace```
  
  -> Fix IP address by adding base address

  ```./add_base_IP.py ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}.trace.clean ${FOLDER}/${BIN}_PTW ${FOLDER}/${BIN}.binanlys_Fixed ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}.trace.final```

  -> Separate trace and Call Path graph into two different file from the perf script output
  add_base_ip.py does this as well

3-) Memory Analysis (mem-anlys)

```./execute.sh ${FOLDER}/${BIN}_PTW.hpcstruct ${FOLDER}/${BIN}.lc_Fixed ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}.trace.final $Period $LOAD $FUNC $MAKEDBIT```

  -> Read multiple file:

    -> CallPath  (*.callpath)
    -> Trace     (*.trace)
    -> hpcstruct (*.hpcstruct)
    -> load classification (*.lc)
  
  -> Build calling context tree (work in progress)
  
  -> Do memeory analysis on the tree  or per routine



-----------------------------------------------------------------------------
Linux Perf
=============================================================================

- Using kernel 5.5.9: no changes needed.
    
- Modifications to Linux perf (user level):
  - perf script (static ip address instead of dynamic)
  - we played with perf driver, but are not using it

- Perf command we used:
  - Collecting trace by sampling based on number of loads:
  
  ```perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e cpu/umask=0x81,event=0xd0,period=${PERIOD},aux-sample-size=${SIZE},call-graph=lbr/u -o ${bin}.data -- ./${bin} $args```

  
  - Collecting trace by sampling based on time:

  ```perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e ref-cycles/period=${PERIOD},aux-sample-size=${SIZE},call-graph=lbr/u -- ${bin} ${args}```

  - Using filter to focus a single function

  ```perf record -m 4M,4M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u --filter 'filter @distBuildLocalMapCounters' -o ${bin}.data -- ./${bin} $args```

  - To extract trace created usin ptwrite

  ```perf script --script=/home/kili337/Projects/Fallacy/scripts/intel-pt-events.py -i ${bin}.data > ${bin}.trace```

  - To collect trace using ldlat for intel machines

  ```perf record -W -d -e cpu/mem-loads,ldlat=1,period=100/upp```
  
  - To extract ldlat trace

  ```perf script --script=../ldlat-events.py -i <input>```

- system wide monitoring and ordering/timestamps

    ```
    - without --per-thread, perf opens a fd on each core because it has a buffer/event per cpu; size (129 pages  4 k)
    - perf-event-disable (ioctl) should have an effect when it returns (write to msr)
    - libunwind happens in userland: perf copies context/stack into its buffer

    > perf report -D
    > perf -g + pt
    > strace -etrace=perf_event_open
    ```


-----------------------------------------------------------------------------
Trace format
=============================================================================

MemGaze:   insn-pc mem-addr cpu-id timestamp sample-id

MemCAMera: insn-pc mem-addr cpu-id timestamp [mem-addr2 <cpu-id> <timestamp>]
