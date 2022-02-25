#!/usr/bin/python
import sys 

#arg 1 is the File Arg 2 is the name of function
def main(): 
  res = open(sys.argv[1], "r")
  function = sys.argv[2]
  size = 0
  ArrayFP = 0
  FP = 0
  strided = 0
  indirect = 0
  constant = 0
  level = 0
  start = 0
  prevLvL= 0
  prevStart=0
  sameLvlCounter=0
  stack = []
  while True:
    line = res.readline()
#    print line
    if not line:
      break
    if function in line:
#      print line,
      words = line.split()
      if function in words[0]:
        break
      start = int(words[0].split(":")[0])
      level = int(words[0].split(":")[1])
      ID = (start,level)
      stack.append(ID)
#      print " NL"
      if level >= prevLvL:
        del stack[:]
        prevLvL = level
        prevStart = start
        sameLvlCounter = 0
        print line,
        for word in words:
          if "size" in word:
            size += float(word.split(":")[1])
          if "FP" in word:
            FP += float(word.split(":")[1])
          if "Strided" in word:
            strided += float(word.split(":")[1])
          if "Indirect" in word:
            indirect += float(word.split(":")[1])
          if "Constant" in word:
            constant += float(word.split(":")[1])
#      else:
#        while True:
#          item=stack.pop()
#          if 
#
#        MAX=len(stack)
#        for i in range(MAX-1, 0, -1):
#          tID= stack[i]
#          if tID[0] != start 
#            tID[1]==level:
#            print line
#        stack[stack.size()-1][0] != start :
#        sameLvlCounter+=1
#        if sameLvlCounter > 1:
#          print line,
#          for word in words:
#            if "size" in word:
#              size += float(word.split(":")[1])
#            if "FP" in word:
#              FP += float(word.split(":")[1])
#            if "Strided" in word:
#              strided += float(word.split(":")[1])
#            if "Indirect" in word:
#              indirect += float(word.split(":")[1])
#            if "Constant" in word:
#              constant += float(word.split(":")[1])


  print "Function:      FP       Str     Ind   Cnst   size      %ind"
  if FP!=0:
    print function+" "+str(FP)+" "+str(strided)+" "+str(indirect)+" "+str(constant)+" "+str(size)+" "+str(indirect/FP)
  else:
    print function+" "+str(FP)+" "+str(strided)+" "+str(indirect)+" "+str(constant)+" "+str(size)+" SOMETHING WRONG"
      

if __name__ == "__main__":
  main()

