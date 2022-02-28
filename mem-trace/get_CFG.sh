#!/bin/bash
#$1=binary
bin=$1
args=$2

echo "bin: $bin args: $args"

~/Projects/IPPD/gitlab/miami-nw/install/bin/cfgprof -stats -statistic   -- ${bin} $args
mv *.cfg ${bin}.cfg
