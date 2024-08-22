import re
import sys
import os
import subprocess
import argparse
import string

numDebug =0
def genGrepCmd(strPerfFile, strTraceFile):
  print('Running match for ', strPerfFile , ' ' , strTraceFile)
  strInstBin =''
  command =''
  strTraceCnt=''
  command = 'wc -l ' + strTraceFile + ' | cut -d " " -f 1'
  try:
    strTotalTrace= subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT,universal_newlines=True)
  except subprocess.CalledProcessError as grepexc:
    print("error code ", command, grepexc.returncode, grepexc.output)
  numTotalTrace=int(strTotalTrace) 
  with open(strPerfFile) as f:
    for fileLine in f:
      #print(fileLine)
      data=fileLine.strip().split(' ')
      data = [x for x in re.split("\s+",fileLine) if x]
      data=fileLine.strip().split()
      if (len(data) >=5):
        if ('%' in data[0] and 'XSB' in data[1]):
          perLoad = float(data[0].strip('%'))
          if (perLoad >= 1.0):
            strBinanlys = data[4][-4:]
            if(all(c in string.hexdigits for c in strBinanlys)):
              if (numDebug):
                print(perLoad , data[4], strBinanlys)
              command = 'grep '+ strBinanlys+ ' ./XSBench-memgaze.binanlys | cut -d " " -f 1'
              if (numDebug):
                print(command)
              try:
                strInstBin= subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT,universal_newlines=True)
              except subprocess.CalledProcessError as grepexc:
                print("error code ", command, grepexc.returncode, grepexc.output)
              if (numDebug):
                print(strInstBin)
              if (strInstBin != ''):
                strInstBin = strInstBin.strip('\n')
                command = 'grep -c '+ strInstBin + ' ' + strTraceFile 
                if (numDebug):
                  print(command)
                try:
                  strTraceCnt= subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT,universal_newlines=True)
                except subprocess.CalledProcessError as grepexc:
                  #print("error code ", command, grepexc.returncode, grepexc.output)
                  print(perLoad , data[4], strBinanlys, ' NO-match')
                strTraceCnt = strTraceCnt.strip('\n')
                if (numDebug):
                  print(strTraceCnt)
                if (strTraceCnt != ''):              
                  print(perLoad , data[4], strBinanlys, strTraceCnt, (float(strTraceCnt)/numTotalTrace)*100)
  f.close()
#genGrepCmd('./hist_loads_gridinit.txt', './XSBench-memgaze-trace-b8192-p5000000-hist-gridinit/XSBench-memgaze.trace')
#genGrepCmd('./event_k-0_loads_gridinit.txt', './XSBench-memgaze-trace-b8192-p5000000-event-k-0-gridinit/XSBench-memgaze.trace')
#genGrepCmd('./event_k-1_loads_gridinit.txt', './XSBench-memgaze-trace-b8192-p5000000-event-k-1-gridinit/XSBench-memgaze.trace')
genGrepCmd('./hist_loads.txt', './XSBench-memgaze-trace-b16384-p4000000-hist/XSBench-memgaze.trace')
genGrepCmd('./event_k-0_loads.txt', './XSBench-memgaze-trace-b16384-p4000000-event-k-0/XSBench-memgaze.trace')
genGrepCmd('./event_k-1_loads.txt', './XSBench-memgaze-trace-b16384-p4000000-event-k-1/XSBench-memgaze.trace')
