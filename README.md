-*-Mode: markdown;-*-

$Id$

-----------------------------------------------------------------------------
MemGaze Pipeline
=============================================================================

MemGaze has 3 mains steps. To run all three main steps use compile.sh 

'''./compile.sh <FOLDER> <BIN> <ARGS> <0(time)/1(load)> <BufferSIZE(def=8192)> <PERIOD(def=37500000)> <MaskedBits(def=0)> <FUNCTION-NAME if need to focus>'''

1-) Analysis and Instrumentation of the Binary (bin-anyls): 
  '''./instument\_binary.sh ${FOLDER}/${BIN}'''

  -> Read binary with dyninst

  -> Create internal CFG for each Routine

  -> Analyze each routine to find each load and their types ( strided, indirect, constant)

  -> Instrument with ptwrite for selective loads

  -> Print original and instrumented IP address

  -> Print load classification file

  -> Adjust IP addresses
  '''./ip\_converter.py ${FOLDER}/${BIN}.log ${FOLDER}/${BIN}.binanlys'''

  -> Create hpcstruct file
  '''hpcstruct ${BIN}\_PTW'''

2-) Trace Collection (mem-trace)
  
  -> Run Instrumented Binary with perf
  '''./collect\_data.sh ${FOLDER}/${BIN} """$ARGS""" ${SIZE} ${PERIOD} 0 ${LOAD}'''
  
  -> Extract data file with perf-script by using a python script
  '''./extract\_data.sh ${FOLDER}/${BIN}\_s${SIZE}\_p${PERIOD}'''
  
  -> Remove data collection errors from trace
  '''./removeErros.sh ${FOLDER}/${BIN}\_s${SIZE}\_p${PERIOD}.trace'''
  
  -> Fix IP address by adding base address
  '''./add\_base\_IP.py ${FOLDER}/${BIN}\_s${SIZE}\_p${PERIOD}.trace.clean ${FOLDER}/${BIN}\_PTW ${FOLDER}/${BIN}.binanlys\_Fixed ${FOLDER}/${BIN}\_s${SIZE}\_p${PERIOD}.trace.final'''

  -> Separate trace and Call Path graph into two different file from the perf script output
  add\_base\_ip.py does this as well

3-) Memory Analysis (mem-anlys)
''./execute.sh ${FOLDER}/${BIN}_PTW.hpcstruct ${FOLDER}/${BIN}.lc_Fixed ${FOLDER}/${BIN}_s${SIZE}_p${PERIOD}    .trace.final $Period $LOAD $FUNC $MAKEDBIT'''

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

[[Ozgur: todo]]

- Using kernel 5.5.9: no changes needed.
    
- Modifications to Linux perf (user level):
  - perf script (static ip address instead of dynamic)
  - we played with perf driver, but are not using it


-----------------------------------------------------------------------------
Trace format
=============================================================================

MemGaze:   insn-pc mem-addr cpu-id timestamp sample-id

MemCAMera: insn-pc mem-addr cpu-id timestamp [mem-addr2 <cpu-id> <timestamp>]
