# Benchmarks

- [Gapbs](https://github.com/sbeamer/gapbs)
	- cc (-g 22)
	- pr (-g 22)
- [minivite](https://gitlab.pnnl.gov/perf-lab/minivite-x) (v2 or v3)
    - need to add -lm in Makefile in line 28
	- add -n 300000 (specify rank)
- [darknet](https://github.com/pjreddie/darknet)
	- resnet152
	- alexnet
- [AMG](https://github.com/LLNL/AMG)
    - make sure that Makefile.include has the -fopenmp option
- [sw4lite](https://github.com/geodynamics/sw4lite)
    - need to use the line: make ckernel=yes openmp=yes
    
# Pipeline Stemps

1. inst
2. run
3. xtrace
4. analyze or analyze-loc

# Dependencies

- gcc
- g++
- make
- openMP
- CUDA toolkit
- openCV
- ar
- cuDNN
- pthread

# Notes

- Add libz in [bin-anlys/src/Scheduler/Makefile](../bin-anlys/src/Scheduler/Makefile)
