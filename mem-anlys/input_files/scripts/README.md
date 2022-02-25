To run full experiment from the start to end  you can use one of the following three command:

To run a set of tests with sampling:
```
  ./experiment.sh <FOLDER> <BIN> <"ARGS"> <BufferSIZE(def=8192)> <PERIOD(def=37500000)> <Mask(Rec=0xffffffffffff)> <#runs> <name for new folder>
```

to compile and run a single test with sampling
```
  ./compile.sh <FOLDER> <BIN> <"ARGS"> <BufferSIZE(def=8192)> <PERIOD(def=37500000)> <Mask(Rec=0xffffffffffff)>
```

if you already instrument the binary to run single test with sampling
```
  ./run.sh <FOLDER> <BIN> <ARGS> <BufferSIZE(def=8192)> <PERIOD(def=37500000)> <Mask(Rec=0xffffffffffff)>
```

To run each of them without sampling just do not enter any Buffersize, Period or mask
```
  ./experiment.sh <FOLDER> <BIN> <#runs> <name for new folder>
  ./compile.sh <FOLDER> <BIN> <ARGS>
  ./run.sh <FOLDER> <BIN> <ARGS>
```

while most of the scrips has relative path to current directory some scripts require special paths

To able to run  compile.sh and experiment.sh you will need to install HPCtoolkit and set correct path for hpcstruct.

for instrument_binary.sh you will need to set correct path to you miami executable in your palm-memory directory.

extract_data.sh requires a modified perf script. and a python script that has been used with modified perf.  you would need to patch (TODO Creat a patch) you perf  and set the paths accordingly


