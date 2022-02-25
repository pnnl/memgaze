this tool uses a memory access trace collected using ptwrite
and instruction type data to calculate fp and access diagnostics.

to compile:

make

to run:

  ./fp_intel_pt.x <inputTrace> <loadClassifications> <hpcstructFile> <outputfile> <debug 1 if needed> \

For the examples in the input_files you can run as follow: 

  cd input_files
  ../fp_intel_pt.x small.trace loadClass.txt test2M_PTW.hpcstruct outputFile.txt 1
  


