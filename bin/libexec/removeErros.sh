#!/bin/bash
set -x
cat $1 |grep -v error > ${1}.clean 
