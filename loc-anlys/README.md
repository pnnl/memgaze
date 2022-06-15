memCAMera - location based analysis on memory traces

### Dependencies ###
gcc version 7.0+

### Compile ###
Run make to build the memCAM executable file.

### RUN with default configuration ###
memCAM needs a memorytraces file to run
Trace format - [IP loadAddr core initialtime\n]

To run the analysis toolset using the default configuration and memorytraces

    ./memCAM memorytraces.out --analysis

To list options

	./memCAM --help


Notes from - original Fallacy analysis - not used in MemGaze
### RUN with customize configruation ###
The memCAM tool provides two kinds of configurations: usability configurations (such as #page and #queue) and architecture configurations (such as cache size, cache Latency, queue policy).

To change the usablity configuration, typing the follow comment for detail or see the usability bellow.

	./memCAM --help

To change the architecture configuration, see model.config for detail.







