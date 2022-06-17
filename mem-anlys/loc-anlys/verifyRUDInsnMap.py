import sys
import os
import subprocess

# Use for reading source line
# grep -n 79070 -B 5 miniVite_O3-v1_obj_nuke_line | grep '\/'

def readFile(inFile):
    with open(inFile) as f:
        varVersion =''
        curRange =''
        rangeIndex = 0
        verRegionFn=''
        prevVersion=''
        for fileLine in f:
            processLine=0
            data = fileLine.strip().replace('\t',' ').split(' ')
            strResult=''
            #print(data[0]+'---')
            if data[0] == '--insn':
                #print (data)
                variantFile = data[6]
                index = data[6].rindex('.trace.final')
                varVersion = data[6][index-2:index]
                if (variantFile == '../MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1.trace.final'):
                  objFile = '/files0/suri836/minivite_create_filtered/MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1_obj_nuke_line' 
                  varVersion = 'V1: '
                if (variantFile == '../MiniVite_O3_v2_nf_func_8k_P5M_n300k/miniVite_O3-v2.trace.final'):
                  objFile = '/files0/suri836/minivite_create_filtered/MiniVite_O3_v2_nf_func_8k_P5M_n300k/miniVite_O3-v2_obj_nuke_line' 
                  varVersion = 'V2: '
                if (variantFile == '../MiniVite_O3_v3_nf_func_8k_P5M_n300k/miniVite_O3-v3.trace.final'):
                  objFile = '/files0/suri836/minivite_create_filtered/MiniVite_O3_v3_nf_func_8k_P5M_n300k/miniVite_O3-v3_obj_nuke_line' 
                  varVersion = 'V3: '
                if (variantFile == '/files0/suri836/RUD_Zoom/alexnet_10/darknet_s8192_p1000000.trace.final'):
                  objFile = '/files0/suri836/RUD_Zoom/alexnet_10/darknet_obj'
                  varVersion = 'a10: '
            elif (( data[0][0:1] == 'V') or (data[0] =='a10:') ) :
                #print(data[0]+'---'+data[1])
                if(len(data) >= 5):
                  grepValue = data[4]
                  if(len(data) >= 8):
                    compareValue = data[7]
                    #-B 5 miniVite_O3-v1_obj_nuke_line | grep '\/'
                    command = 'grep -B 5 '+grepValue+': '+objFile +' | grep \/'
                    try:
                      strResult = subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT,universal_newlines=True)
                    except subprocess.CalledProcessError as grepexc:
                      print("error code ", command, grepexc.returncode, grepexc.output)
                    if (strResult !=''):
                      strResult = str(strResult.strip())
                    if('\n' in strResult):
                      arrStrMap= strResult.split('\n')
                      print("**** Info  more than 1 lines")
                      print(command)
                      strResult = arrStrMap[len(arrStrMap)-1]
                    if( strResult != compareValue):
                      print("---- Error - no match")
                      print(command)
                      print("Result - ", strResult )
                      print("Expected - ", compareValue)
        f.close()

n = len(sys.argv)
print("\nArguments passed:", end = " ")
for i in range(1, n):
    print(sys.argv[i], end = " ")
readFile(sys.argv[1])




