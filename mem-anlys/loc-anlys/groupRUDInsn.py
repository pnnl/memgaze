dictFunctions ={}
dictAccess = {}
dictBlkCnt={}
dictMinRUD={}
dictMaxRUD={}
dictAvgRUD={}
dictSampleMinRUD={}
dictSampleMaxRUD={}
dictSampleAvgRUD={}
dictRegionCnt ={}
dictVarFunctions={}

def readFile(filename,outFile):

    f_out = open(outFile, 'w')
    with open(filename) as f:
        for fileLine in f:
            if(('zoom' in fileLine) and (('O3_v' in fileLine) \
                                         or ('al_10' in fileLine ) or ('al_z' in fileLine) \
                                         or ('res' in fileLine ) or ('rs_z' in fileLine) \
                                        or ('gap' in fileLine ))):
                data = fileLine.strip().split(' ')
                if ('O3_v' in fileLine):
                    version = data[0]
                    varVersion = version[3:5].upper()
                elif ('al_10' in fileLine):
                    varVersion = 'a10'
                elif ('res' in fileLine):
                    varVersion = 'rs'
                elif ('gap' in fileLine):
                    varVersion = 'gap'
                elif ('al_z' in fileLine):
                    version = data[0]
                    varVersion = version[0:4]
                    print('first ', varVersion)
                elif ('rs_z' in fileLine):
                    version = data[0]
                    varVersion = version[0:4]
                    print('first ', varVersion)
                range = data[1]
                accessCnt = data[5]
                dictAccess[varVersion+'_'+range] = accessCnt
                dictBlkCnt[varVersion+'_'+range] = int(data[3])
                #print(data[6])
                dictMinRUD[varVersion+'_'+range] = data[7].strip('(').strip(',')
                dictMaxRUD[varVersion+'_'+range] = data[8].strip(',')
                dictAvgRUD[varVersion+'_'+range] = data[9].strip(')')
                dictSampleMinRUD[varVersion+'_'+range] = data[12].strip('(').strip(',')
                dictSampleMaxRUD[varVersion+'_'+range] = data[13].strip(',')
                dictSampleAvgRUD[varVersion+'_'+range] = data[14].strip(')')
                print (varVersion,range,accessCnt)
    f.close()
    with open(filename) as f:
        varVersion =''
        curRange =''
        rangeIndex = 0
        verRegionFn=''
        prevVersion=''
        for fileLine in f:
            processLine=0
            data = fileLine.strip().replace('\t',' ').split(' ')
            if data[0] == 'configuration':
                curRange = data[3].replace('range:','').replace('--','-')
                rangeIndex = rangeIndex+1
                #print (data[0] , curRange)
                continue
            if data[0] == 'V1:':
                if (prevVersion==''):
                    rangeIndex = 0
                varVersion = 'V1'
                prevVersion = varVersion
                processLine=1
            elif data[0] =='V2:':
                if (prevVersion=='V1'):
                    rangeIndex = 0
                varVersion = 'V2'
                prevVersion = varVersion
                processLine=1
            elif data[0] == 'V3:':
                varVersion = 'V3'
                if (prevVersion=='V2'):
                    rangeIndex = 0
                processLine=1
                prevVersion = varVersion
            elif data[0] == 'a10:':
                print ('in a10')
                if (prevVersion==''):
                    rangeIndex = 0
                varVersion = 'a10'
                processLine=1
                prevVersion = varVersion
            elif 'al_' in data[0] and data[0][3:4].isdigit() and ('zoom' not in data[0]):
                print('in al_', data[0])
                varVersion = data[0][3:4]+'_'+data[0][0:2]
                if (prevVersion=='' or prevVersion != varVersion):
                    rangeIndex = 0
                processLine=1
                prevVersion = varVersion
                #print('version mod ', varVersion)
            elif 'rs_' in data[0] and data[0][3:4].isdigit() and ('zoom' not in data[0]):
                #print(data[0])
                varVersion = data[0][3:4]+'_'+data[0][0:2]
                if (prevVersion=='' or prevVersion != varVersion):
                    rangeIndex = 0
                processLine=1
                prevVersion = varVersion
                #print('version mod ', varVersion)
            elif data[0] == 'rs:':
                if (prevVersion==''):
                    rangeIndex = 0
                varVersion = 'rs'
                processLine=1
                prevVersion = varVersion
            elif data[0] == 'gr:':
                if (prevVersion==''):
                    rangeIndex = 0
                varVersion = 'gap'
                processLine=1
                prevVersion = varVersion
            elif data[0] == 'gc:':
                if (prevVersion==''):
                    rangeIndex = 0
                varVersion = 'gap'
                processLine=1
                prevVersion = varVersion
            #print(data)
            if(processLine==1):
                shortFnName =''
                verRange = varVersion+'_'+curRange
                #print(verRange)
                #print(dictAccess)
                totalAccess = int(dictAccess[verRange])
                curAccess = int(data[2])
                if (len(data)>6 and ((curAccess/totalAccess)*100)>2 ):
                    #print(varVersion ,'R'+str(rangeIndex) , data[3], data[4])
                    fnStartIP = str(data[5])
                    if(fnStartIP != '---'):
                        print(fileLine)
                        fnName = data[6]
                        if(len(data)>7):
                            codeLine = data[7]
                            codeIndex = codeLine.rfind("/")
                            codeLine=data[7][0:codeIndex]
                            codeIndex=codeLine.rfind("/")
                        else:
                            codeLine =''
                        varVerionRgn = varVersion+'_R'+str(rangeIndex)
                        if( 'distBuildLocalMapCounter' in fnName):
                            shortFnName = 'blmc'
                        if( 'distExecuteLouvainIteration' in fnName):
                            shortFnName = 'Louvain'
                        if( 'distGetMaxIndex' in fnName):
                            shortFnName = 'GetMax'
                        if( 'std' in fnName and 'tsl' in fnName):
                            shortFnName = 'std_tsl'
                        if( 'distComputeModularity' in fnName ):
                            shortFnName = 'compMod'
                        writeLine = varVerionRgn+' '+ curRange+' ' \
                                + dictMinRUD[verRange]+' '+str(dictMaxRUD[verRange])+' '+str(dictAvgRUD[verRange])+' '\
                                + dictSampleMinRUD[verRange]+' '+str(dictSampleMaxRUD[verRange])+' '+str(dictSampleAvgRUD[verRange]) +' '\
                                +str(dictBlkCnt[verRange])+' '+str(curAccess) +' '\
                                +str(totalAccess)+' '+str((curAccess/totalAccess)*100)+' '+  fnStartIP+' '+ fnName +' '\
                                +shortFnName +' '+ data[7][codeIndex+1:]+'\n'
                    else:
                        writeLine = varVerionRgn+' '+ curRange+' ' \
                                + dictMinRUD[verRange]+' '+str(dictMaxRUD[verRange])+' '+str(dictAvgRUD[verRange])+' '\
                                + dictSampleMinRUD[verRange]+' '+str(dictSampleMaxRUD[verRange])+' '+str(dictSampleAvgRUD[verRange]) +' '\
                                +str(dictBlkCnt[verRange])+' '+str(curAccess) +' '\
                                +str(totalAccess)+' '+str((curAccess/totalAccess)*100)+' - - '\
                                + data[6]+'\n'

                    f_out.write(writeLine)
                else:
                    writeLine = fileLine
                print(writeLine)
        f.close()
        f_out.close()

#readFile('./results_16384_128_10p_step_insn/RUD_instruction_group.txt', './results_16384_128_10p_step_insn/RUD_instruction_result.txt')
#readFile('./alexnet_10/results_16384_128_10p/RUD_instruction_group.txt', './alexnet_10/results_16384_128_10p/RUD_instruction_result.txt')
#readFile('./darknet/res_inc_stack/res_RUD_instruction_group.txt', './darknet/res_inc_stack/res_RUD_instruction_result.txt')
#readFile('./darknet/res_inc_stack/al_RUD_instruction_group.txt', './darknet/res_inc_stack/al_RUD_instruction_result.txt')
#readFile('./darknet/al_RUD_instruction_group.txt', './darknet/al_RUD_instruction_result.txt')
#readFile('./results_inc_stack_sep_par/alexnet_RUD_group.txt', './results_inc_stack_sep_par/alexnet_RUD_result.txt')
#readFile('./results_inc_stack_sep_par/res_RUD_group.txt', './results_inc_stack_sep_par/res_RUD_result.txt')
#readFile('./results_inc_stack_sep_par/gap_cc_RUD_group.txt', './results_inc_stack_sep_par/gap_cc_RUD_result.txt')
#readFile('./results_inc_stack_sep_par/gap_pr_RUD_group.txt', './results_inc_stack_sep_par/gap_pr_RUD_result.txt')
#readFile('./darknet_layers/alexnet_RUD_instruction_group.txt', './darknet_layers/alexnet_RUD_instruction_results.txt',)
#readFile('./darknet_layers/resnet_RUD_instruction_group.txt', './darknet_layers/resnet_RUD_instruction_results.txt',)
#readFile('./darknet_worst/al_RUD_instruction_group.txt','./darknet_worst/al_RUD_instruction_result.txt')
#readFile('./darknet_worst/rs_RUD_instruction_group.txt','./darknet_worst/rs_RUD_instruction_result.txt')
#readFile('./darknet_layer_groupat_16777216/res_RUD_instruction_group.txt','./darknet_layer_groupat_16777216/res_RUD_instruction_result.txt')
#readFile('./darknet_layer_groupat_16777216/al_RUD_instruction_group.txt','./darknet_layer_groupat_16777216/al_RUD_instruction_result.txt')
#readFile('./GAP_build_analysis/pr_analysis_RUD_instruction_group.txt','./GAP_build_analysis/pr_analysis_RUD_instruction_result.txt')
#readFile('./GAP_build_analysis/pr_build_RUD_instruction_group.txt','./GAP_build_analysis/pr_build_RUD_instruction_result.txt')
#readFile('./GAP_build_analysis/cc_analysis_RUD_instruction_group.txt','./GAP_build_analysis/cc_analysis_RUD_instruction_result.txt')
readFile('./GAP_build_analysis/cc_build_RUD_instruction_group.txt','./GAP_build_analysis/cc_build_RUD_instruction_result.txt')

#print(dictAccess)
#print (dictFunctions)
#print(dictRegionCnt)

for key in dictVarFunctions:
    #print(key, dictFunctions[key])
    verRange = key
    index = verRange.find('_')
    verRange=verRange[0:index]
    fnIP = key[key.rindex('_')+1:]
    print( verRange, fnIP, dictVarFunctions[key])
