import sys
import os
import subprocess

# Use for reading source line
# grep -n 79070 -B 5 miniVite_O3-v1_obj_nuke_line | grep '\/'
dictFnMap={}
dictFnIdentify ={}

def readFile(inFile, outFile):
    if(outFile !=''):
        f_out = open(outFile, 'w')
        f_obj_c = open('obj_C_insn.txt','w') 
        f_obj_l = open('obj_L_insn.txt','w') 
    with open(inFile) as f:
        blFnMap=0
        varVersion =''
        rangeIndex = 0
        verRegionFn=''
        prevVersion=''
        strResult=''
        curRange=''
        for fileLine in f:
            processLine=0
            data = fileLine.strip().replace('\t',' ').split(' ')
            #print(data[0]+'---')
            if data[0] == '--insn':
                #print (data)
                variantFile = data[6]
                curRange = data[9]
                index = data[6].rindex('.trace.final')
                varVersion = data[6][index-2:index]
                print (variantFile, varVersion, curRange)
                if (variantFile == '../MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1.trace.final'):
                  logFile = '/files0/suri836/minivite_create_filtered/MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1*.log'
                  objFile = '/files0/suri836/minivite_create_filtered/MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1_obj_nuke_line' 
                  objFile_C = '/files0/suri836/minivite_create_filtered/MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1_obj_C' 
                  varVersion = 'V1: '
                if (variantFile == '../MiniVite_O3_v2_nf_func_8k_P5M_n300k/miniVite_O3-v2.trace.final'):
                  logFile = '/files0/suri836/minivite_create_filtered/MiniVite_O3_v2_nf_func_8k_P5M_n300k/miniVite_O3-v2*.log'
                  objFile = '/files0/suri836/minivite_create_filtered/MiniVite_O3_v2_nf_func_8k_P5M_n300k/miniVite_O3-v2_obj_nuke_line' 
                  objFile_C = '/files0/suri836/minivite_create_filtered/MiniVite_O3_v2_nf_func_8k_P5M_n300k/miniVite_O3-v2_obj_C' 
                  varVersion = 'V2: '
                if (variantFile == '../MiniVite_O3_v3_nf_func_8k_P5M_n300k/miniVite_O3-v3.trace.final'):
                  logFile = '/files0/suri836/minivite_create_filtered/MiniVite_O3_v3_nf_func_8k_P5M_n300k/miniVite_O3-v3*.log'
                  objFile = '/files0/suri836/minivite_create_filtered/MiniVite_O3_v3_nf_func_8k_P5M_n300k/miniVite_O3-v3_obj_nuke_line' 
                  objFile_C = '/files0/suri836/minivite_create_filtered/MiniVite_O3_v3_nf_func_8k_P5M_n300k/miniVite_O3-v3_obj_C' 
                  varVersion = 'V3: '
                if (variantFile == '/files0/suri836/RUD_Zoom/alexnet_10/darknet_s8192_p1000000.trace.final' or \
                    variantFile == '/files0/suri836/RUD_Zoom/alexnet_single/darknet_s8192_p1000000.trace.final' or \
                    variantFile == '/files0/suri836/RUD_Zoom/alexnet_single_corrected/darknet_s8192_p1000000.trace.final'):
                  logFile = '/files0/suri836/RUD_Zoom/alexnet_10/darknet.log' 
                  objFile = '/files0/suri836/RUD_Zoom/alexnet_10/darknet_obj' 
                  objFile_C =''
                  varVersion = 'a10: '
                if ('/files0/suri836/RUD_Zoom/alexnet_layers/' in variantFile ):
                  #/files0/suri836/RUD_Zoom/alexnet_layers/darknet_s8192_p1000000.trace.final_focused_0
                  logFile = '/files0/suri836/RUD_Zoom/alexnet_10/darknet.log' 
                  objFile = '/files0/suri836/RUD_Zoom/alexnet_10/darknet_obj' 
                  objFile_C =''
                  layer_index = variantFile.rindex('_')
                  varVersion = 'al'+variantFile[layer_index:]+': '
                if (variantFile == '/files0/suri836/RUD_Zoom/GAP_cc_O3_g22_nf_func_8k_P5M_n1_Avg/cc_O3_s8192_p5000000.trace.final' or \
                    variantFile == '/files0/suri836/RUD_Zoom/GAP_cc_O3_g22_nf_func_8k_P5M_n1_Avg/cc_O3_s8192_p5000000.trace.final_build' or \
                    variantFile == '/files0/suri836/RUD_Zoom/GAP_cc_O3_g22_nf_func_8k_P5M_n1_Avg/cc_O3_s8192_p5000000.trace.final_analysis'):
                  logFile = '/files0/suri836/RUD_Zoom/GAP_cc_O3_g22_nf_func_8k_P5M_n1_Avg/cc_O3.log'
                  objFile = '/files0/suri836/RUD_Zoom/GAP_cc_O3_g22_nf_func_8k_P5M_n1_Avg/GAP_cc_O3_obj_nuke'
                  objFile_C ='/files0/suri836/RUD_Zoom/GAP_cc_O3_g22_nf_func_8k_P5M_n1_Avg/GAP_cc_O3_obj'
                  varVersion = 'gc: '
                if (variantFile == '/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/pr_O3_s8192_p5000000.trace.final' or \
                 variantFile == '/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/pr_O3_s8192_p5000000.trace.final_build' or \
                 variantFile == '/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/pr_O3_s8192_p5000000.trace.final_analysis' ):
                  logFile = '/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/pr_O3.log'
                  objFile = '/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/GAP_pr_O3_obj_nuke'
                  objFile_C ='/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/GAP_pr_O3_obj'
                  varVersion = 'gr: '
                if (variantFile == '/files0/suri836/RUD_Zoom/resnet_single/darknet_s8192_p1000000.trace.final' or \
                     variantFile == '/files0/suri836/RUD_Zoom/resnet152_10/darknet_s8192_p1000000.trace.final' ):
                  logFile = '/files0/suri836/RUD_Zoom/resnet152_10/darknet.log'
                  objFile = '/files0/suri836/RUD_Zoom/resnet152_10/darknet_obj'
                  objFile_C =''
                  varVersion = 'rs: '
                if ('/files0/suri836/RUD_Zoom/resnet_layers/' in variantFile): 
                  logFile = '/files0/suri836/RUD_Zoom/resnet152_10/darknet.log'
                  objFile = '/files0/suri836/RUD_Zoom/resnet152_10/darknet_obj'
                  objFile_C =''
                  layer_index = variantFile.rindex('_')
                  varVersion = 'rs'+variantFile[layer_index:]+': '
                if (variantFile == '/files0/suri836/RUD_Zoom/resnet152_single/darknet_s8192_p1000000.trace.final'): 
                  logFile = '/files0/suri836/RUD_Zoom/resnet152_single/darknet.log'
                  objFile = '/files0/suri836/RUD_Zoom/resnet152_10/darknet_obj'
                  objFile_C =''
                  varVersion = 'rs: '
                if (variantFile == '/files0/suri836/RUD_Zoom/resnet_worst_perf/darknet_s8192_p1000000.trace.final'): 
                  logFile = '/files0/suri836/RUD_Zoom/resnet_worst_perf/darknet.log'
                  objFile = '/files0/suri836/RUD_Zoom/resnet_worst_perf/darknet_obj'
                  objFile_C =''
                  varVersion = 'rs: '
                if (variantFile == '/files0/suri836/RUD_Zoom/alexnet_worst_perf/darknet_s8192_p1000000.trace.final'): 
                  logFile = '/files0/suri836/RUD_Zoom/alexnet_worst_perf/darknet.log'
                  objFile = '/files0/suri836/RUD_Zoom/alexnet_worst_perf/darknet_obj'
                  objFile_C =''
                  varVersion = 'al: '
                if(blFnMap==0):
                    blFnMap =1
                    command = 'grep 00000000 '+ objFile+ ' | grep "<"| cut -d " " -f 1-3'
                    try:
                        strFnMap= subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT,universal_newlines=True)
                    except subprocess.CalledProcessError as grepexc:
                        print("error code ", command, grepexc.returncode, grepexc.output)
                    if('\n' in strFnMap):
                        arrStrFnMap= strFnMap.split('\n')
                        for i in range(0,len(arrStrFnMap)):
                            arrFnMapData = arrStrFnMap[i].split(' ')
                            #print(arrFnMapData)
                            if(len(arrFnMapData)>1):
                                if (len(arrFnMapData)==3):
                                    dictFnMap[arrFnMapData[0]] = arrFnMapData[1] + arrFnMapData[2]
                                else:
                                    dictFnMap[arrFnMapData[0]] = arrFnMapData[1]
                                dictFnIdentify[int(arrFnMapData[0],16)] = arrFnMapData[0]
                    #print(dictFnMap)
                    #print(dictFnIdentify)
            elif data[0].isnumeric():
                strMapping=''
                strBinLine=''
                #print(data[0]+'---'+data[1])
                if(data[1] != ''):
                  varInst = data[1]
                else:
                  varInst = data[2]
                #print (varInst)
                varInstgr=varInst[2:]
                command = 'grep '+varInstgr+' '+logFile
                strMapping =''
                try:
                  strMapping = subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT,universal_newlines=True)
                except subprocess.CalledProcessError as grepexc:
                  print("error code ", command, grepexc.returncode, grepexc.output)
                if (strMapping !=''):
                  strMapping = str(strMapping.strip())
                  if('\n' in strMapping):
                    print(" 2 lines", strMapping)
                    arrStrMap= strMapping.split('\n')
                    for strCorrectMap in arrStrMap:
                      if('0x' not in strCorrectMap):
                        strMapping = strCorrectMap
                  #print(strMapping)
                  grData=strMapping.split(' ')
                  #print (varInst, grData[0])
                  command = 'grep ' + grData[0] + ': '+objFile
                  strBinLine =''
                  try:
                    strBinLine = subprocess.check_output(command, shell=True)
                  except subprocess.CalledProcessError as grepexc:
                    print("error code ", strMapping, command, grepexc.returncode, grepexc.output)
                  if(strBinLine !=''):
                    strBinLine = str(strBinLine.strip())
                    strBinLine1 = strBinLine.replace('\t','_').replace(' ','_')
                    strBinLine1 = strBinLine1[2:]
                    #print(strBinLine1)
                    if(objFile_C != ''):
                      command = 'grep ' + grData[0] + ': '+objFile_C
                      strBinLine_C =''
                      try:
                        strBinLine_C = subprocess.check_output(command, shell=True)
                      except subprocess.CalledProcessError as grepexc:
                        print("error code ", strMapping, command, grepexc.returncode, grepexc.output)
                      strBinLine_C = str(strBinLine.strip())
                      if(strBinLine != strBinLine_C):
                        print ('Error ' , varVersion, varInst, grData[0], strBinLine, strBinLine_C)
                    command = 'grep -B 10 '+grData[0]+': '+objFile +' | grep \/'
                    strResult=''
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
                    processLine = 1
                    if(objFile_C != ''):
                      f_obj_c.write( strBinLine_C)
                      f_obj_c.write('\n')
                    f_obj_l.write( strBinLine)
                    f_obj_l.write('\n')
                    for key in dictFnIdentify:
                        if (int(grData[0],16)) < key:
                            continue
                        else:
                            fnStartIP = dictFnIdentify[key]
                            fnName = dictFnMap[fnStartIP]
                  writeLine = varVersion+' '+data[0]+ ' '+varInst+' '+ grData[0] + ' '+fnStartIP+ ' '+fnName+' '+ strResult+' \n'
            if(processLine ==0):
              f_out.write(fileLine)
            else:
              f_out.write(writeLine) 

        f.close()
        f_out.close()
        f_obj_c.close()
        f_obj_l.close()

n = len(sys.argv)
print("\nArguments passed:", end = " ")
for i in range(1, n):
    print(sys.argv[i], end = " ")
if (len(sys.argv)>2 and sys.argv[2] != ''):
  readFile(sys.argv[1], sys.argv[2])
else:
  readFile(sys.argv[1])




