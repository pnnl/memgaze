#!/usr/bin/python
import sys 
import subprocess
import os

def twos_comp(val, bits):
  """compute the 2's complement of int value val"""
  if (val & (1 << (bits - 1))) != 0: # if sign bit is set e.g., 8bit: 128-255
    val = val - (1 << bits)        # compute negative value
  return val                         # return positive value as is

#it will get the trace and the binary to add base off set for the IP addreses.
#arg input file , binary, lc file,  output file
def main():
  if len(sys.argv) != 5:
    print ("Run as following\n./add_base_IP.py <input trace> <binary path> <fixed LC_file> <output trace>")

  print (sys.argv[1]) 
  print (sys.argv[2]) 
  print (sys.argv[3]) 

  inputTrace = open(sys.argv[1], "r")
  inputLC = open(sys.argv[3], "r")

  binaryPath = sys.argv[2]
  outputTrace = open(sys.argv[4], "w")
  
  offsetMap = {}
  scaleMap = {}
  while True:
    line  = inputLC.readline()
    if not line:
      break
#    print line
    l =  line.split()
    ip = int(l[0], 16)
    lc = int(l[1], 10)
    offset = twos_comp(int(l[2],16), 32)
    scale = int(l[3], 16)
    offsetMap[ip]=offset
    scaleMap[ip]=scale

#  print offsetMap

  command = "objdump -h {} | grep .dyninstInst".format(binaryPath)
  dumpOut=subprocess.Popen(['objdump', '-h', binaryPath],stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
  stdout,stderr = dumpOut.communicate()
  objdump = stdout.split()
  index = objdump.index( ".dyninstInst")
  baseAddr = int(objdump[index+2], 16)
  print ("base addres is:{}".format(hex(baseAddr)))

  add_offset = True
  str_output = ""
  prev_addr = 0
  prev_cpu = ""
  prev_time = ""
  prev_ip = 0
  while True:
    line = inputTrace.readline()
    if not line:
      break
    words =  line.split()
    IP=int(words[0],16)
    Addr=int(words[1],16)
    CPU=words[2]
    Time=words[3]
    IP=IP+baseAddr
#    print "IP: {} prevIP: {}".format(IP,prev_ip)

    if prev_ip+5 == IP:
      scale = 1
      if IP in scaleMap:
        scale =  scaleMap[IP]
      Addr = Addr + prev_addr*scale
      IP = prev_ip
      CPU = prev_cpu
      Time = prev_time
#      add_offset = False
    else:  
      outputTrace.write(str_output)
#      add_offset = True
#    if add_offset:
    if IP in offsetMap:
      offset =  offsetMap[IP]
#        print "I am addding offset {}".format(offset, 'x')
    else : 
      offset = 0
    Addr = Addr + offset
    str_output = "{} {} {} {}\n".format(hex(IP),hex(Addr),CPU,Time)
#    outputTrace.write("{} {} {} {}\n".format(hex(IP),Addr,CPU,Time))
    prev_ip = IP
    prev_cpu = CPU
    prev_time = Time
    prev_addr = Addr
  
  outputTrace.write(str_output)
  outputTrace.close()
  inputTrace.close()

if __name__ == "__main__":
  main()
