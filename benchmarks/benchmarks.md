# Benchmarks

- [Gapbs](https://github.com/sbeamer/gapbs)
	- cc (-g 22)
        - 1 thread
            - g=22 -> 0.15 s
            - g=24 -> 0.72 s
        - 8 threads
            - g=22 -> 0.02 s
            - g=24 -> 0.1 s
	- pr (-g 22)
- [minivite](https://gitlab.pnnl.gov/perf-lab/minivite-x) (v2 or v3)
    - need to add -lm in Makefile in line 28
	- add -n 300000 (specify rank)
    - 1 thread
        - n=300000 -> 1.4 s
    - 8 threads
        - n=300000 -> 0.25 s
        - n=600000 -> 0.5 s
- [darknet](https://github.com/pjreddie/darknet)
	- resnet152
	- alexnet
- [AMG](https://github.com/LLNL/AMG)
    - make sure that Makefile.include has the -fopenmp option
    - omp = 8
        - 256 -> 26 s
        - 512 -> 3.7 m
    - 1024 -> mem overflow
- [sw4lite](https://github.com/geodynamics/sw4lite)
    - need to use the line: make ckernel=yes openmp=yes
    - 1 thread
        - n=1 poinsource.in -> 17.5419 s
    - 8 threads
        - n=1 poinsource.in -> 8.6982 s

# Pipeline Steps

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

- knobs
    - affinity
    - threads (8 <= n <= 16)
        - use only 1 hw thread per p-core
    - problem size (start w/ moderate size)

- metrics
    - time
        - \time -f "overview: elapsed %es, user %Us, system %Ss, cpu %P, ctxt-switch %c, waits %w, exit %x\nmemory: res-set-max %MK, res-set-avg %tK, mem-avg %KK, unshared-data-avg %DK, unshared-stack-avg %pK, shared-txt-avg %XK, major-faults %F, minor-faults %R, swaps %W\nio: inputs %I, outputs %O, sckt-rcv %r, sckt-snd %s, signals %k"

- first order statistics: compute means and stardard deviations of all experiments (-> to catch outliars)