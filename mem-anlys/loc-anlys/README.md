memgaze-analyze-loc - location based analysis on memory traces


### Compile ###
Run make to build the memgaze-analyze-loc executable file.

### RUN with default configuration ###
memgaze-analyze-loc needs a trace file to run
Trace format - [IP loadAddr core initialtime sample_id\n]

To run the analysis toolset using the default configuration and trace_file

    ./memgaze-analyze-loc trace_file --analysis

To list options

	./memgaze-analyze-loc --help


Notes from - original Fallacy analysis - not used in MemGaze
### RUN with customize configruation ###
The memCAM tool provides two kinds of configurations: usability configurations (such as #page and #queue) and architecture configurations (such as cache size, cache Latency, queue policy).

To change the usablity configuration, typing the follow comment for detail or see the usability bellow.

	./memCAM --help

To change the architecture configuration, see model.config for detail.







