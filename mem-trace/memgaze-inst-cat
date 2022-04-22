#!/usr/bin/python
import sys 
#TO run first arg will be instrumentation log
#second arg is load classigficaton file .lc
def main():
  instMap = {}  
  fmap = open(sys.argv[1], "r")
  inSection = False
  while True:
    line = fmap.readline()
#    print line
    if not line:
      break
    if "Start Writing to a file" in line:
      inSection = True
      continue
    if inSection:
      if "I Wrote to a file named" in line:
        inSection = False
#        print ">>>>>>>Exiting<<<<<"
        break
      l =  line.split()
      hex_int = int(l[1], 16)
#      new_int = hex_int + 0x1000
      new_int = hex_int
#      print new_int
      instMap[l[0]] = hex(new_int)

#  print instMap
  floadclass = open(sys.argv[2],"r")
  loadClassMap = {}
  while True:
    line = floadclass.readline()
    if not line:
      break
#    print line
    l = line.split()
    if l[0] in instMap:
      inst = instMap[l[0]]
    else:
      inst = l[0]
    v = (l[1],hex(int(l[2],16)),hex(int(l[3],16)))
#    print v
    loadClassMap[inst] = v
#  print loadClassMap
  fmap.close()
  floadclass.close()
  writeFile =  sys.argv[2] + "_Fixed"
  floadclass = open(writeFile, "w")
#  floadclass = open("deneme.txt", "w")
  for key, value in loadClassMap.items():
    text = key + " " + value[0] + " " + value[1] + " " + value[2] + "\n"
    floadclass.write(text)

if __name__ == "__main__":
  main()

