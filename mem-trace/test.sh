#!/bin/bash
WORD=$(objdump -h /home/kili337/Projects/IPPD/gitlab/palm/intelPT_FP/Experiments/UbenchSet/ubench_set_PTW | grep .dyninstInst)
echo $WORD
for w in $WORD
do
  echo $w
done
stringarray=($WORD)
echo ${stringarray[5]}
