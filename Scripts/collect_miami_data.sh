#!/bin/bash

# Copy this script and edit it as needed.
# First, define the location where the MIAMI framework is installed on your 
# system
MIAMI_ROOT=${HOME}/MIAMI

# This script runs the application several times. 
# (1) it executes the application natively, with no profiling
# (2) a CFG profile run
# (3) a memory reuse profile
# (4) a streaming behavior profile
#
# Comment out the unwanted runs, or edit them to pass different parameters.

arg="$@"
#echo "Arguments: ${arg}"
tag=`basename "${arg}" | tr " " "_"`
echo "Using tag: ${tag}"

echo "Running ${arg} native ..."
time ${arg} > log_${tag}_orig.txt 2>&1

echo "Collect cfgprof information for ${arg} ..."
time ${MIAMI_ROOT}/bin/cfgprof -o ${tag} -no_pid -stats -- \
    ${arg} > log_${tag}_cfg.txt 2>&1

echo "Collect MRD information for ${arg} ..."
time ${MIAMI_ROOT}/bin/memreuse -no_pid \
    -block_mask 0 -footprint -B 64 -o ${tag} -- \
    ${arg} > log_${tag}_mrd64.txt 2>&1

echo "Collect streaming information for ${arg} ..."
# the next configuration detects streams with an access stride of at most
# 1 cache line, produced by cache misses in a 64KB, 2-way associative cache.
# This is the configuration for the DC prefetcher on an AMD Istanbul machine.
time ${MIAMI_ROOT}/bin/streamsim -M 1 -C 64 -A 2 \
    -o ${tag}-L1 -no_pid -- \
    ${arg} > log_${tag}_srdL1.txt 2>&1
