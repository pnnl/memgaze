memgaze-analyze-loc - location based analysis on memory traces


### Compile ###
Run make to build the memgaze-analyze-loc executable file.

### RUN with default configuration ###
memgaze-analyze-loc needs a trace file to run
Trace format - [<IP> <Addrs> <CPU> <time> <sampleID> <DSO_id>]

To run the analysis toolset using the default configuration and trace_file

    ./memgaze-analyze-loc trace_file --analysis

To list options

	./memgaze-analyze-loc --help


##### Notes from Fallacy analysis - not used in MemGaze #####
RUN with custom configuration - memgaze-analyze-loc tool provides two configurations: usability configurations (such as #page and #queue) and architecture configurations (such as cache size, cache Latency, queue policy).

To change the usability configuration.

	./memgaze-analyze-loc --help

To change the architecture configuration, see memorymodeling.h, model.config for details.







