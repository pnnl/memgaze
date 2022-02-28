
MIAMI (Machine Independent Application Models for performance Insight)
======================================================================

Subfolder bin/ located in the MIAMI install directory contains several tool 
wrappers for analyzing x86 binaries.

Relevant tools:

cfgprof:   is a CFG profiler. The profiler statically discovers the control 
           flow graph (CFG) of each executed routine, the first time control
           flow enters that routine. During execution, it refines the
           discovered CFGs with any control flow information that could not
           be discovered statically. The profiler inserts counters on select
           CFG edges and basic blocks. For this reason, it is faster than 
           profilers that indiscriminately insert counters on every basic 
           block, while at the same time being able to infer the execution 
           frequencies of all CFG edges in addition to basic block execution
           frequencies.

memreuse:  is a tool for discovering data reuse patterns in applications
           and for collecting memory reuse distance histograms associated
           with each reuse pattern. Optionally, it can collect the working 
           set footprint of all program scopes.

streamsim: is a tool for understanding data streaming behavior in 
           applications. It classifies each memory access as streaming or
           non-streaming. On a streaming access, it also computes the number
           of live streams. It collects a histogram of this information at 
           instruction level.

miami:     is a post-processing tool that consumes dynamic analysis profiles
           produced by the previous tools. It combines dynamic analysis 
           results with static analysis to infer additional insight about 
           an application's performance.


Other tools:

miamicfg:  is another CFG profiler, however, it uses a purely dynamic
           approach to discover the control flow graph of executed routines. 
           It can be more resilient in the presence of anti reverse
           engineering techniques. However, MIAMI is not targeted at such 
           codes and one disadvantage of miamicfg is that it produces CFGs
           that may vary substantially from run to run when different inputs
           are used. miamicfg is maintained for backward compatibility, but 
           it is deprecated in favor of cfgprof. 

cachesim:  is a typical LRU cache simulator with user defined line size, 
           associativity and cache size. It is an old tool, not integrated 
           with the rest of the MIAMI toolkit, but it can be useful to get a
           measure of conflict misses.


Using the tools
---------------

See the INSTALL file in the MIAMI root folder for building instructions.
Add the MIAMI bin/ directory to your path or specify the full path when
executing a tool.

The general format for executing any of the dynamic profilers is:
$ <path_to_MIAMI>/bin/cfgprof [tool_args] -- <application> [app_args]

The double dash "--" is required. It separates the wrapper options from 
the target application and its parameters.

To profile an MPI application, place the wrapper in the position where you 
would normally specify your application binary, that is, after the mpirun 
command:
$ mpirun -np 16 <MAMI_ROOT>/bin/cfgprof [tool_args] -- <application> [app_args]


The output of the CFG profiler is needed for all the analyses performed by 
MIAMI.

'memreuse' is needed if you want to understand how data is reused.

'streamsim' is needed if you want to understand streaming behavior in your
application.

All the dynamic profilers support multi-threaded applications. They
automatically collect and save profile data separately for each thread.
The post-processing tool, miami, is executed offline. Thus, it does not need
explicit support for multi-threaded applications. A user will specify which
output profile files, correspondig to one of the threads, to pass to the 
post-processing tool.

Getting basic help
------------------
Use the -h argument to see a list of all available command line options for
each tool.
For the miami tool you can specify -h directly, or just execute miami
without any arguments.
For all the other tools, you must pass the name of an application
after a double dash (--). 'ls' will do.
Example: cfgprof -h -- ls

0) Before you start
----------------
You should compile your application with source line mapping information
enabled (flag -g for most compiler) to get the MIAMI performance data 
mapped to the application source code. You can specify any optimization 
level in addition to the -g flag.

The ISPASS'14 paper "MIAMI: A Framework for Application Performance
Diagnosis" presents an overview of the tools and provides some insight into 
the philosophy behind their designs.


1) Collecting CFG profiles
--------------------------
Profile an application to collect CFG profiles. For this, you have to
execute an application using the cfgprof wrapper.

<MIAMI_ROOT>/bin/cfgprof [options] -- <your_application> <your_arguments>

No additional options are required for the CFG profiler. This step creates 
one .cfg file per application thread.
By default, the output files are named:
  ExecutableName-<mpi_rank>-<thread_idx>-<process_pid>.cfg
 
  - MPI ranks are numbered from 0 to N-1
  - thread indices are numbered from 0 to T-1

I've seen overhead for this tool from 20% to 10x. So, it depends a lot on 
the application.

You can alter the output file names using any combination of the following 
options:
-o <output_prefix>: use the specified prefix instead of the executable name.
-no_pid  : do not append the process pid to the output file names
-no_mpi_rank : do not append the mpi rank to the output file names

If you omit both the mpi rank and the process PID for an MPI run, threads 
from different processes of a parallel job will attempt to write to the same 
file(s) and the behavior is undefined.

2) Collecting memory reuse distance profiles
--------------------------------------------
For this, you have to execute an application using the memreuse wrapper.

<MIAMI_ROOT>/bin/memreuse [options] -- <your_application> <your_arguments>

The same considerations as before apply. 

Due to the level of analysis performed on each memory access, this tool
adds significant overhead, on the order of 200x time overhead when the carry
and footprint analyses are enabled.

There are a few options that are important for this tool.
-B <block_size>  : specifies the block size (in bytes) for the data reuse 
                   analysis. The default value is 64, useful for x86
                   target architectures.
-carry   : performs additional analysis to understand the carrying scope of
           a data reuse pattern. Adds overhead, but it provides useful 
           insight.
-footprint  : collects footprint information for scopes. This option implies
              the '-carry' flag, so you do not have to specify both.
              It adds slighly more overhead over the '-carry' option.
-text   : enabled by default. It causes the profile data to be saved in text 
          format. There is currently no support for parsing binary MRD 
          profiles.

This step creates one .mrdt (if text mode), or .mrdb (if binary mode) file
per thread. The default name is:
  ExecutableName-:<block_size>:-<mpi_rank>-<thread_idx>-<process_pid>.mrdt

As before, you can alter the output file names using any combination of the 
following options:
-o <output_prefix>: use the specified prefix instead of the executable name.
-no_pid  : do not append the process pid to the output file names
-no_mpi_rank : do not append the mpi rank to the output file names

The ISPASS'08 paper "Pinpointing and Exploiting Opportunities for Enhancing
Data Reuse" provides some insight into the concept of data reuse patterns and
how to interpret them. The newer ISPASS'14 paper reinforces some of those
insights.

**
* We provided a sample script, <MIAMI_ROOT>/Scripts/collect_miami_data.sh, 
* that you can copy to your working directory and use as is, or modify it. It
* performs a CFG profile run and one memreuse run with a block size of 64,
* and other common options that we use.
**

3) Collecting streaming profiles
-----------------------------
For this, you have to execute an application using the streamsim wrapper.
<MIAMI_ROOT>/bin/streamsim [options] -- <your_application> <your_arguments>

Most of the same considerations as before apply. 

This tool performs analysis on each memory access. Thus, it also adds
significant overhead, on the order of 100x time overhead.

There are a few options that are important for this step.
-A  <int>  - [default 48] cache associativity (use 1 for direct mapped, 
             0 for fully-associative)
-C  <int>  - [default 6144] cache size in kilobytes
-L  <int>  - [default 64] cache line size; also granularity for stream strides
-M  <int>  - [default 3] maximum allowed stream stride, in lines
-H  <int>  - [default 256] history table size in entries. A value of 0 
             disables stream detection.
-T  <int>  - [default 128] stream table size in entries. A value of 0 disables 
             stream detection.

For a better understanding of what these options do, see the ICS'13 paper
"Diagnosis and Optimization of Application Prefetching Performance."

This step creates one .srd file per thread. The default output name is:

ExecutableName-hist_size.table_size-line_size.max_stride-<mpi_rank>-<thread_idx>-<process_pid>.srd

As before, you can alter the output file names using any combination of the 
following options:
-o <output_prefix>: use the specified prefix instead of 
              executable_name-hist_size.table_size-line_size.max_stride.
-no_pid  : do not append the process pid to the output file names
-no_mpi_rank : do not append the mpi rank to the output file names


4) Post-processing the data, creating performance databases
-----------------------------------------------------------
For this, you have to use the 'miami' post-processing tool.
You have to pass one .cfg file, and a number of other optional arguments.
Note that you do not have to specify the path to the executable for this
step. That information is taken from the profile files.

$ <MIAMI_ROOT>/bin/miami -c <cfg_file> [options]

Run miami with -h to see all the available options. 

Some useful options:
-mrd <mrdt_file>  : pass a memory reuse distance file. You can pass this
                    option multiple times, using profiles collected for 
                    different block sizes.
-m <machine_model> : specify a machine model. Required for most analyses.
                     You can only get instruction mixes and streaming
                     concurrency results without a machine model. 
                     There is a Sandy Bridge machine model provided in
                     <MIAMI_ROOT>/share/machines/. Note that machine models
                     are text files. You can copy an existing model and
                     modify it, or create a new one.
-imix  : produces an instruction mix report at loop level, in CSV format. 
         Classifies instructions into a restricted set of instruction 
         categories.
-iwmix : another option for getting instruction mixes. Classifies
         instructions into a fixed set of instruction categories depending 
         on the instruction type and the bit width of the instruction result.
         This option provides a finer grain classification, but both options
         generate a fixed number of categories in CSV format, for easier 
         post-processing with a script.
-f <floating_point_num> : The threashold must be between 0 and 1. Specifies 
         a minimum threshold that a program scope must satify to be included 
         into the output performance databases. 
         A scope must account for at least this fraction of any one of a set 
         of primary metrics. The default threshold is 0, which means that all 
         scopes are included. This option is useful when analyzing large 
         applications where database sizes can grow to gigabytes.

If a machine model is specified, miami will try to produce an instruction
schedule for the target machine. It computes an instruction execution time 
and many other metrics in the process.

It also computes metrics about the memory hierarchy using the memory reuse
distance profiles, if any provided.

To compute only data reuse information, but no instruction scheduling, you must 
specify a machine model with option -m and the option -no_scheduling.

-o <output_prefix> : specifies the prefix used for all the output files.
'miami' can output multiple xml files, and possibly some .csv files. They 
will all start with the specified prefix.
By default, the prefix is ExecutableName-MachineName

By default, the process PID is appended to the output files.
As before, you can use option -no_pid to prevent adding the process pid.


You can process an .srd file together with other profile data, but the total 
number of computed metrics would only go up, making it harder to browse
the databases. You can also process just the streaming data separately.
To process streaming concurrency profiles, you need to pass one .srd file
and one .cfg file to the miami offline tool. No need to pass a machine file
in this case. Pass also option -xml to have the reslts saved in XML
format. Otherwise, MIAMI will output a CSV file.


The miami tool is slightly or overly verbose, depending on flags used
during compilation. It is best to redirect stderr to a file for this step. 

**
* We provided a sample script, <MIAMI_ROOT>/Scripts/process_miami_data.sh, 
* that you can copy to your working directory and use as is, or modify it. 
* It runs miami with the CFG and MRDT profiles that you collected with the
* first script. The script includes a second run that shows how to process 
* SRD data using one .srd and one .cfg file, together with the -xml option.
*
* A second script, <MIAMI_ROOT>/Scripts/process_mrd_data.sh processes just
* the mrd data. It skips the instruction scheduling part using the
* -no_scheduling flag mentioned before.
**

5) Collecting the source code files
-----------------------------------
If you got the xml files, things are going fairly well, but you are not
finished just yet. You need to gather all of the application's source files 
to be able to browse the performance data mapped to the application source 
code.

Unfortunatelly, this is not a fully automated process yet, but we tried to 
automate parts of it using a collection of shell scripts. 
You must run the following shell script, specifying one of the generated XML
files as a parameter:
<MIAMI_ROOT>/ExtractSourceFiles/copySourceFiles.sh <xml_db_file>

If everything goes well, this script will create a folder src/ in the
current directory in which it copied all the application's source files that
it could find.
It also creates a second shell script, script_updatePaths.sh.
You must run this latter script on each of the xml files.

for ff in <prefix>*.xml; do ./script_updatePaths.sh ${ff}; done

The src/ folder together with the generated (and updated) .xml files, plus
any eventual .csv files, represent the performance database.
You can visualize them in place or copy them to a different machine. All the
information is contained within the xml and csv files, and the src/ folder.


6) Viewing the databases
------------------------
A java viewer is provided in <MIAMI_ROOT>/Viewer/hpcviewer.jar.
To open an XML file:
  java -jar <MIAMI_ROOT>/Viewer/hpcviewer.jar <xml_file>

For large database files, you may want to increase the maximum heap size
of your JVM using option -Xmx.
Ex: java -Xmx1024m -jar <MIAMI_ROOT>/Viewer/hpcviewer.jar <xml_file> 
will increase the heap size limit to 1GB.

The viewer can be used on most mainstream OSes, including MacOS, Linux and 
Windows.


7) Interpreting the MIAMI metrics
---------------------------------
Step number (5) computes a few different xml files, representing different
views of the performance data:
 - <prefix>.xml : is the main performance database. Reported performance 
   metrics are aggregated based on the program static structure. Thus, the
   values reported for a routine represent the contribution of the code
   inside the routine, including any loop nests inside that routine. 
   At the highest level are the application load modules. Each module may
   have associated several source files, and each source file may have
   several routines. To compare routines across all modules and files, one
   should "flatten" the scope tree twice (using the viewer button with a
   tree picture and a red circle), to remove the entries for modules and 
   files.
   This file enables a top-down navigation of the data in the viewer. For 
   example, one can sort the database by a metric and then drill down to 
   find the loop that contributes the most to that particular metric.
   Most metrics are aggregated in this way. The exceptions are the number of
   misses carried by a scope, and the scope footprint metrics, which do not
   make sense being aggregated by program static structure.
   >> File Main_Metrics.txt provides an overview of the reported metrics
      (some metric names may have changed slightly since its writting).

 - <prefix>-flat.xml : reports the same performance metrics as the main xml
   file, however, results are not aggregated based on program structure.
   Thus, the values reported for a particular scope include only the
   contribution of the code in that particular loop or routine, but excluding
   the contribution of any inner loops. This file lists all program scopes,
   including routines, and outer and inner loops, in a flat listing inside 
   each module. You should "flatten" the profile once after you open the 
   database file in the viewer, to remove the entries for modules.
   This file is mostly useful to look for scopes that contribute the most to
   metrics that are not aggregated based on the static program structure, 
   such as carried misses, and to a lesser degree, footprint information.
   >> File Main_Metrics.txt provides an overview of the reported metrics.

 - <prefix>-units.xml : reports how many cycles each machine execution unit
   has been 'busy' during the execution of a program scope. It is a 
   top-down file, where metrics are aggregated based on the program static 
   structure.
   >> File Units_Metrics.txt provides an overview of the reported metrics.

 - <prefix>-reuse.xml : reports information about data reuse patterns in
   the application.
   >> File Reuse_Metrics.txt provides an overview of the reported metrics.


Obtaining a routine's control flow graph (optional)
---------------------------------------------------
You can pass argument -r <routine_name> either to the CFG profilers or to
the miami tool to obtain a dot format representation of the routine's CFG.
Depending on the step where the CFG is generated, the graph may be annotated
with basic block and edge execution frequencies.

This option always outputs .dot files in pairs:
RoutineName_cfg[_idx].dot  - basic blocks and edges are colored based on
                             their type.
RoutineName_tarj[_idx].dot - basic blocks and edges are colored based on
                             their loop nesting level.
A numeric index is appended and auto-incremented to avoid overwriting 
existing dot files.


------------------------
NOTICE: The .cfg, .mrdt and .srd files contain information mapped to binary 
addresses. For this reason, they are valid only with the original
executables that were used during data collection. The profile files contain 
paths to the executable and any shared libraries used during the profiling 
runs. 

The post-processing step (with miami) uses those paths to locate the
binaries, decode their instructions and perform static analysis. You should 
not delete or move your binaries before running the post processing step.


Other options
-------------

Optionally, you can resume and pause data collection dynamically. For this, 
you must modify the application's source code to insert calls to two, user 
defined empty functions, one for starting and one for stopping data collection.
You can choose any name for these two caliper functions.

To use the calippers, you must pass these additional parameters to the 
wrapper:

-q -start <name_of_start_function> -stop <name_of_stop_function>

Be sure to pass the *mangled* names of the functions (the names of the
symbols as they exist in the binary, e.g., '_Z20start_hpct_profilingv' 
instead of 'start_hpct_profiling' for a C++ compiled function).


