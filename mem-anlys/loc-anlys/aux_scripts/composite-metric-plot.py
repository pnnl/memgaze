import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
import re
import matplotlib.patches as mpatches
plt.rcParams['font.family'] = 'monospace'
#plt.rcParams.update({"text.usetex": True,})

SI_good=63
def read_file_df(strFileName, intHotRef:None, intHotAff:None, strApp,dfForPlot,dictVarAccess):
    # Three ways to choose hot ref blocks
        # intHotRef = 0 - Option 0 - 90% in range Hot-Access - DEFAULT
        # intHotRef = 1 - Option 1 - 90% of total access
        # intHotRef = 2 - Option 2 - ALL blocks - all reference blocks for whole application
    # Two ways to choose hot affinity blocks
        # intHotAff = 0 - Option 0 - only hot lines and self
        # intHotAff = 1 - Option 1 - only hot lines and self, self-1, self+1, self+2 - DEFAULT
        # intHotAff = 2 - Option 2 - all heatmap rows
        # intHotAff = 3 - Option 3 - only hot lines and (self-1, self, self+1, self+2) for SA, (self-1, self+1, self ) for SD

    if(intHotRef==None):
        intHotRef =0
    if(intHotAff==None):
        intHotAff =1
    df_local=pd.read_csv(strFileName)
    df_local.insert(0,'Variant',strApp)
    print('Start ', strApp, strFileName)
    #print(df_local.shape)
    df_local.drop('Unnamed: 0',inplace=True,axis=1)
    df_local.sort_values(by=['Access'],ascending=False,inplace=True)
    #print(df_local['Access'].sum())
    sumAccessRefBlocks = df_local['Access'].sum()
    # This is not correct - heatmap only has selected lines - ordered by access, not all lines in region
    val = pd.Index(df_local.Access.cumsum()).get_loc(int((0.9*sumAccessRefBlocks)), 'nearest', tolerance=int(0.05*sumAccessRefBlocks))
    #print(val)
    accessThreshold=df_local.at[val,'Access']
    #print(' All lines above access count ' , accessThreshold)
    #accessThreshold=round((0.05*sumAccessRefBlocks),0)
    #print(' All lines above access count ' , accessThreshold)

    listRefLines = df_local['reg-page-blk'].to_list()
    listRefRegions=list(set([x[0] for x in listRefLines]))
    print(listRefRegions)
    #print(df_local.columns)

    dfCols = df_local.columns.to_list()
    listAffinityLines=[]
    if (intHotAff==0):
        pattern = re.compile('SD-.*-.*-.*')
        listAffinityLines=list(filter(pattern.match, dfCols))
        listAffinityLines.append('SD-self')
    if (intHotAff==1 or intHotAff==3):
        pattern = re.compile('SD-.*-.*-.*')
        listAffinityLines=list(filter(pattern.match, dfCols))
        listAffinityLines.append('SD-self')
        listAffinityLines.append('SD-self-1')
        listAffinityLines.append('SD-self+1')
        listAffinityLines.append('SD-self+2')
    if (intHotAff==2):
        pattern = re.compile('SD-.*')
        listAffinityLines=list(filter(pattern.match, dfCols))

    #print(dfCols)
    #print('Columns in df_local ', df_local.columns.to_list())
    pattern = re.compile('SD-.*')
    dfSDCols =  list(filter(pattern.match, df_local.columns.to_list()))
    for item in listAffinityLines:
        if (not (item in dfSDCols)):
            print( 'item ', item, dfSDCols)
            listAffinityLines.remove(item)

    for i in range(0,len(listAffinityLines)):
        listAffinityLines[i] = listAffinityLines[i].replace('SD-','')

    #Include only hot lines from the region
    if(0):
        print('hot lines in affinity before region', listAffinityLines)
        for x in listAffinityLines:
            if((len(x.split('-')) >=3) and (not x[0] in listRefRegions)):
                listAffinityLines.remove(x)
    print('hot lines in affinity', listAffinityLines)

    pattern = re.compile('self.*')
    listSelfAffLines = list(filter(pattern.match, listAffinityLines))
    #print(' listSelfAffLines ' , listSelfAffLines)
    listHotRefLines = df_local[df_local['Hot-Access'] >0.1]['reg-page-blk'].to_list()
    print(listHotRefLines)
    totRegionAccess=df_local['TotalAccess'].iloc[0]
    totRefBlockAccess=df_local['Access'].sum()
    hotRefAccess=df_local[df_local['Hot-Access'] >0.1]['Access'].sum()
    allPageAccessRatio=df_local['PageAccess'].iloc[0]/df_local['TotalAccess'].iloc[0]
    hotAccessWeight=hotRefAccess/totRegionAccess # Weight for the histogram bin
    print( strApp, " hot Access ", hotRefAccess, "ratio ", hotRefAccess/df_local['TotalAccess'].iloc[0])
    print( strApp, " all ref block Access ratio",   totRefBlockAccess/df_local['TotalAccess'].iloc[0])
    allBlockAccessRatio=totRefBlockAccess/df_local['TotalAccess'].iloc[0]
    #print(" total access", totAccess)
    #print(" hot access ", hotAccess)
    dictVarAccess[strApp] = [totRegionAccess,hotRefAccess,allPageAccessRatio]
    print('listHotRefLines ', listHotRefLines)
    for index, row in df_local.iterrows():
        for i in range(0,len(listAffinityLines)):
            flInclude=True
            flIncludeSA = True
            flIncludeSD = True
            blkSDValue=np.NaN
            blkSIValue=np.NaN
            blkSAValue=np.NaN
            regPageBlock= row['reg-page-blk']
            regPage=regPageBlock[0:regPageBlock.rindex('-')+1]
            curBlock=regPageBlock[regPageBlock.rindex('-')+1:]
            #print('regPage ', regPage, ' curBlock ', curBlock)
            contigLocation = listAffinityLines[i][4:]
            #print('contigLocation ', contigLocation)
            intAffBlock=0
            if ('-' in contigLocation):
                intAffBlock=int(curBlock) - int(contigLocation[1:])
            if('+' in contigLocation):
                intAffBlock=int(curBlock) + int(contigLocation[1:])
            checkAffBlock = regPage + str(intAffBlock)
            #print(checkAffBlock)
            # check for hot line in contiguous locations
            if(checkAffBlock in listAffinityLines):
                #print ( strApp , ' Not incluidng contig block for ', regPageBlock, listAffinityLines[i], checkAffBlock)
                flInclude=False
            # check for hot line in self location - otherwise it'll be counted twice
            if(regPageBlock in listAffinityLines and listAffinityLines[i] =='self'):
                flInclude=False
            # not include 'SELF' for SA
            if(regPageBlock == listAffinityLines[i] ):
                flIncludeSA=False
            if((intHotAff==3) and (not(listAffinityLines[i] in ['self', 'self-1', 'self+1'] or len(listAffinityLines[i].split('-'))>=3))):
                flIncludeSD=False
            if((flInclude==True ) and \
                        #((intHotRef == 0 and (row['Hot-Access']>= 0.1 and (row['Access']/row['TotalAccess'] >= 0.0025))) \
                            ((intHotRef == 0 and  (row['Hot-Access']>= 0.1)) \
                                     # this needs to be changed
                                     or (intHotRef == 1 and row['Access']>= accessThreshold)  or \
                                        (intHotRef == 2 ))):
                if((row['SI-'+listAffinityLines[i]])) and (~np.isnan(row['SI-'+listAffinityLines[i]])):
                    blkSIValue = row['SI-'+listAffinityLines[i]]
                if((row['SD-'+listAffinityLines[i]])) and (~np.isnan(row['SD-'+listAffinityLines[i]])):
                    blkSDValue = row['SD-'+listAffinityLines[i]]
                if((row['SP-'+listAffinityLines[i]])) and (~np.isnan(row['SP-'+listAffinityLines[i]])):
                    blkSAValue = row['SP-'+listAffinityLines[i]]
                if (not(np.isnan(blkSIValue))):
                    #print (regPageblocks[i],dfCols[j],blkSDValue, blkSIValue, blkSAValue, valSIPenalty)
                    valSIBin = int(blkSIValue /SI_good) +1
                    if(valSIBin > 10):
                        valSIBin = 10
                    valSIDiscount = float((10-valSIBin+1)/10)
                    df_local.loc[df_local['reg-page-blk'] == regPageBlock, ['SD-'+listAffinityLines[i]]] = round((blkSDValue * valSIDiscount),3)
                    df_local.loc[df_local['reg-page-blk'] == regPageBlock, ['SP-'+listAffinityLines[i]]] = round((blkSAValue * valSIDiscount),3)
                    #print (regPageBlock, 'SI ', blkSIValue, blkSDValue, blkSAValue, 'BIN - ', valSIBin, valSIDiscount, \
                           #df_local[(df_local['reg-page-blk'] == regPageBlock)]['SD-'+listAffinityLines[i]].item(), \
                           #df_local[(df_local['reg-page-blk'] == regPageBlock)]['SP-'+listAffinityLines[i]].item() )
                    if(flIncludeSA == True and flIncludeSD ==True):
                        dfForPlot.loc[len(dfForPlot)]=[strApp,regPageBlock,row['Access'], row['Hot-Access'], round((blkSDValue * valSIDiscount),3),round((blkSAValue * valSIDiscount),3),hotAccessWeight]
                        #print( strApp,regPageBlock,  round((blkSDValue * valSIDiscount),3))
                    elif (flIncludeSA == False and flIncludeSD ==True):
                        dfForPlot.loc[len(dfForPlot)]=[strApp,regPageBlock,row['Access'], row['Hot-Access'], round((blkSDValue * valSIDiscount),3),np.nan,hotAccessWeight]
                    elif (flIncludeSA == True and flIncludeSD ==False):
                        dfForPlot.loc[len(dfForPlot)]=[strApp,regPageBlock,row['Access'], row['Hot-Access'], np.nan, round((blkSAValue * valSIDiscount),3),hotAccessWeight]
                #else:
                    #dfForPlot.loc[len(dfForPlot)]=[strApp,regPageBlock,row['Access'], row['Hot-Access'], 0, 0]

                    #if(strApp=='Random' or strApp=='Default'):
                    #    print(row['Access'], row['TotalAccess'])
    #print(dfForPlot.shape)
    #print(dfForPlot)
    #dfForPlot.to_csv('/Users/suri836/Projects/spatial_rud/spatial_pages_exp/miniVite/bignuke_run/mini-memgaze-ld/miniVite-v3-memgaze-trace-b16384-p5000000-anlys/plot_df.csv')

def plot_app(strApp, optionHotRef, optionHotAff,flPlot:bool=False):
    strFileName=''
    strPath='/Users/suri836/Projects/spatial_rud/spatial_pages_exp/'
    dfForPlot= pd.DataFrame(columns=['Variant', 'reg-page-blk','Access','HotLineWeight', 'SD', 'SA', 'HotAccessWeight'])
    dictVarAccess={}

    if(strApp.lower()=='minivite-nuke'):
        strFileName='miniVite/hot_lines/miniVite-v1-1-A0000001-SD-SP-SI-df.csv'
        strAppVar='miniVite-v1'
        strFileName=strPath+strFileName
        read_file_df(strFileName, optionHotRef,optionHotAff , strAppVar,dfForPlot,dictVarAccess)
        print(dfForPlot.shape)

        strFileName='miniVite/hot_lines/miniVite-v2-1-A0000010-4-A0002000-SD-SP-SI-df.csv'
        strAppVar='miniVite-v2'
        strFileName=strPath+strFileName
        read_file_df(strFileName, optionHotRef,optionHotAff , strAppVar,dfForPlot,dictVarAccess)
        print(dfForPlot.shape)

        strFileName='miniVite/hot_lines/miniVite-v3-1-A0000001-5-A0001200-SD-SP-SI-df.csv'
        strAppVar='miniVite-v3'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot,dictVarAccess)
        print(dfForPlot.shape)

    if(strApp.lower()=='minivite'):
        strFileName='miniVite/bignuke_run/mini-memgaze-ld/miniVite-v1-memgaze-trace-b16384-p5000000-anlys/miniVite-v1-1-A0001000-2-B0000000-3-B1100000-SD-SP-SI-df.csv'
        strAppVar='miniVite-v1'
        strFileName=strPath+strFileName
        read_file_df(strFileName, optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)

        strFileName='miniVite/bignuke_run/mini-memgaze-ld/miniVite-v2-memgaze-trace-b16384-p5000000-anlys/miniVite-v2-2-A0002000-3-B0000000-4-B1000000-SD-SP-SI-df.csv'
        strAppVar='miniVite-v2'
        strFileName=strPath+strFileName
        read_file_df(strFileName, optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)

        strFileName='miniVite/bignuke_run/mini-memgaze-ld/miniVite-v3-memgaze-trace-b16384-p5000000-anlys/miniVite-v3-4-A0002000-5-B0000000-6-B1100000-SD-SP-SI-df.csv'
        strAppVar='miniVite-v3'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)
        strFileName=strPath+'miniVite/bignuke_run/mini-memgaze-ld/'

    if(strApp == 'HiParTI-HiCOO tensor MTTKRP'):
        strFileName='HICOO-tensor/mttsel-re-1-b16384-p4000000-U-0/hot_lines/HiParTI-HiCOO-Lexi-0-B0000000-SD-SP-SI-df.csv'
        strAppVar='Lexi'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)

        strFileName='HICOO-tensor/mttsel-re-2-b16384-p4000000-U-0/hot_lines/HiParTI-HiCOO-BFS-0-B0000000-SD-SP-SI-df.csv'
        strAppVar='BFS'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)

        strFileName='HICOO-tensor/mttsel-re-0-b16384-p4000000-U-0/hot_lines/HiParTI-HiCOO-0-B0000000-SD-SP-SI-df.csv'
        strAppVar='Default'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)

        strFileName='HICOO-tensor/mttsel-re-3-b16384-p4000000-U-0/hot_lines/HiParTI-HiCOO-Random-0-B0000000-SD-SP-SI-df.csv'
        strAppVar='Random'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)
        strFileName=strPath+'HICOO-tensor/'

    if(strApp == 'HiParTI-matrix'):
        strFileName='HICOO-matrix/4096-same-iter/hot_lines/csr/HiParTi-CSR-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='CSR'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)

        strFileName='HICOO-matrix/4096-same-iter/hot_lines/coo_u_0/HiParTi-COO-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='COO'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)

        strFileName='HICOO-matrix/4096-same-iter/hot_lines/coo_u_1/HiParTi-COO-Reduce-4-A3000000-SD-SP-SI-df.csv'
        strAppVar='COO-Reduce'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)

        strFileName='HICOO-matrix/4096-same-iter/hot_lines/hicoo_u_0/HiParTi-HiCOO-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='HiCOO'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)

        strFileName='HICOO-matrix/4096-same-iter/hot_lines/hicoo_u_1/HiParTi-HiCOO-Schedule-2-B0010000-SD-SP-SI-df.csv'
        strAppVar='HiCOO-Schedule'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)

        strFileName=strPath+'HICOO-matrix/4096-same-iter/hot_lines/'

    if(strApp.lower()=='xsb-noinline'):
        strFileName='XSBench/openmp-threading-noinline/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1/XSB-rd-EVENT_OPT_k1-0-A0000000-1-B0000000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-threading-noinline/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0/XSB-rd-EVENT_k0-0-A0000000-1-B0000000-2-B0010000-3-B0020000-4-B0030000-5-B0040000-6-B0050000-7-B0060000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-threading-noinline/memgaze-xs-read/'

    if(strApp.lower()=='xsb-noflto-other-grid'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0-anlys/XSB-rd-EVENT_k0-4-HotIns-11-5-HotIns-12-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1-anlys/XSB-rd-EVENT_OPT_k1-14-B2000000-15-B2000001-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read/'

    if(strApp.lower()=='xsb-noflto-mat-energy'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0-anlys/XSB-rd-EVENT_k0-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1-anlys/XSB-rd-EVENT_OPT_k1-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read/'

    if(strApp.lower()=='xsb-noflto-grid-index'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0-anlys/XSB-rd-EVENT_k0-2-B0000001-3-B0000002-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1-anlys/XSB-rd-EVENT_OPT_k1-12-B0001000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read/'

    if(strApp.lower()=='xsb-noflto-all'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0-anlys/XSB-rd-EVENT_k0-ALL-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1-anlys/XSB-rd-EVENT_OPT_k1-ALL-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read/'

    #print(dfForPlot)
    #print(dfForPlot.head(5))
    #print(dfForPlot.tail(5))

    if(0):
        arrVariants=list(set(dfForPlot["Variant"].to_list()))
        for i in range(len(arrVariants)):
            print(arrVariants[i])
            print((dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ][['Access','HotLineWeight']]))

    dfForPlot.sort_values(by='Variant', ascending=True, inplace=True)
    #print(dictVarAccess)
    arrVariants=list(set(dfForPlot["Variant"].to_list()))
    arrVariants.sort()
    #print(arrVariants)
    dfForPlot['ScoreSA'] = (dfForPlot.SA * dfForPlot.HotLineWeight).where(dfForPlot.SA >= 0.25)
    dfForPlot['ScoreSD'] = (dfForPlot.SD * dfForPlot.HotLineWeight).where(dfForPlot.SD >= 0.05)
    for i in range(len(arrVariants)):
        print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " *** Region weight - SA score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSA'] ).sum()*dictVarAccess[arrVariants[i]][2])
        print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff), " " ,arrVariants[i], " *** Region weight - SD score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSD'] ).sum()*dictVarAccess[arrVariants[i]][2])
    for i in range(len(arrVariants)):
        print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " Hot - SA score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSA'] ).sum())#*dictVarAccess[arrVariants[i]][2])
        print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " Hot - SD score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSD'] ).sum())#*dictVarAccess[arrVariants[i]][2])

    dfForPlot['ScoreSA'] = (dfForPlot.SA * dfForPlot.HotLineWeight)
    dfForPlot['ScoreSD'] = (dfForPlot.SD * dfForPlot.HotLineWeight)
    for i in range(len(arrVariants)):
        print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " --- No filter Region weight - SA score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSA'] ).sum()*dictVarAccess[arrVariants[i]][2])
        print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " --- No filter Region weight - SD score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSD'] ).sum()*dictVarAccess[arrVariants[i]][2])

    for i in range(len(arrVariants)):
        print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " No filter Hot - SA score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSA'] ).sum())#*dictVarAccess[arrVariants[i]][2])
        print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " No filter Hot - SD score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSD'] ).sum())#*dictVarAccess[arrVariants[i]][2])

    binArray=[0.01,0.02,0.03,0.04,0.05,0.1,0.2,0.4,0.6,0.8,1.0]
    listDarkPalette=['#0343df','#650021','#3f9b0b']

    # Area under good region
    #strSDText='Variant'.center(12) +' - Number of - Good (%)\n' + 'block pairs'.center(40)+' \n'
    strSDText='Variant'.center(18) +'- '+'Num. block pairs'.center(22)+'\n' #' - Good (%) \n'
    strSAText='Variant'.center(18) +'- '+'Num. block pairs'.center(22)+'\n' #' - Good (%) \n'
    dictSA={}
    dictSD={}

    # Trial (Area under the curve)
    if (0):
        for i in range(len(arrVariants)):
            for strMetric in ['SD', 'SA']:
                dfVariant = dfForPlot[dfForPlot["Variant"] ==arrVariants[i]]
                dfArea=dfVariant[strMetric].dropna()
                #print(arrVariants[i], strMetric, dfArea.shape)
                #print(dfArea)
                numBlockPairs= dfArea.shape[0]
                #sns.kdeplot(dfArea, linewidth=2, fill=True)
                ax = sns.kdeplot(dfArea)
                # Get all the lines used to draw density curve
                kde_lines = ax.get_lines()[-1]
                kde_x, kde_y = kde_lines.get_data()
                low_limit =0
                if (strMetric == 'SA'):
                    low_limit=0.25
                if(strMetric == 'SD'):
                    low_limit = 0.05
                mask = (kde_x >= low_limit)
                filled_x, filled_y = kde_x[mask], kde_y[mask]
                area = np.trapz(filled_y, filled_x)
                #print('filled_x \n' , filled_x)
                #print('filled_y \n', filled_y)
                #print(strApp , 'variant ', arrVariants[i], ' number of block pairs ', numBlockPairs,  'metric ', strMetric, area)
                if (strMetric == 'SA'):
                    strSAText = strSAText + arrVariants[i].center(15) + '- ' + str(numBlockPairs).center(22) + '\n' #'- ' + str(round(area*100,1)) + '\n'
                    dictSA[arrVariants[i]]=[str(numBlockPairs),str(round(area*100,1))]
                if(strMetric == 'SD'):
                    strSDText = strSDText + arrVariants[i].center(15) + '- ' + str(numBlockPairs).center(22) + '\n' #'- ' + str(round(area*100,1)) + '\n'
                    dictSD[arrVariants[i]]=[str(numBlockPairs),str(round(area*100,1))]

        strSAText=strSAText.strip('\n')
        strSDText=strSDText.strip('\n')
        print('SA \n', strSAText)
        print('SD \n' ,strSDText)
        print(dictSA)
        print(dictSD)
        print(dfForPlot[["Variant", "HotAccessWeight"]])


    if(flPlot == True):
        # Plots
        strMetric="SD"
        #p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,   multiple="dodge",  \
        #                  kde=False, kde_kws={'bw_adjust':0.15,  'clip':(0.02,1.0)}, weights="HotAccessWeight", aspect=2, alpha=1,facet_kws=dict(legend_out=False))
        p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,   multiple="dodge",  \
                          kde=False, kde_kws={'bw_adjust':0.15,  'clip':(0.00,1.0)}, aspect=2, alpha=1,facet_kws=dict(legend_out=False))
        #p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,   multiple="dodge",  stat='percent', common_norm=False, \
        #                  kde=True, kde_kws={'bw_adjust':0.2}, aspect=2, alpha=1,facet_kws=dict(legend_out=False))
        p.set(xlabel="$\it{"+strMetric+"}^{*}$")
        p.set(ylabel="Number of block pairs")
        p.axes.flat[0].set_xlim(0.0,)
        if(strApp=='miniVite'):
            p.axes.flat[0].set_ylim(0,20)
        sns.move_legend(p,"upper center",bbox_to_anchor=(.8, .9))
        if(0):
            leg = p.axes.flat[0].get_legend()
            if leg is None: leg = p._legend
            #print(leg.get_title().get_text())
            strTitle= str(leg.get_title().get_text()).center(20) + 'Hot ref. blocks'.center(15)+'Num. of'.center(15)+'Block pairs with'.rjust(15)+'\n' \
                        +str(' ').center(20) + 'access(%)'.center(15)+'block pairs'.center(15)+"$\it{"+strMetric+"}^{*}$ >= 0.05".center(15)
            new_title = str(leg.get_title().get_text()).center(28) + 'Hot Access(%)'.ljust(15)+'Num. block pairs'.center(15)+'Above 0.05'.rjust(15)
            leg.set_title(strTitle)
            for t in leg.texts:
                t.set_text(str(t.get_text()).ljust(14) +  str(round((dictVarAccess[t.get_text()][1]*100/dictVarAccess[t.get_text()][0]),2)).center(15) \
                           + dictSD[t.get_text()][0].center(15)+ str(round(float(dictSD[t.get_text()][1])*float(dictSD[t.get_text()][0])/100)).center(15))
                t.set_ha('right')
        p.set(title=strApp+" variants - composite $\it{"+strMetric+"}^{*}$")
        #p.map(plt.axvline, x=0.045, color='black', linewidth=1)
        #ax.axvline(x='0.05', ymin=ymin, ymax=ymax, linewidth=1, color='black')
        p.map(plt.axvline, x=0.05, color='black', linewidth=1,linestyle='--')
        strPath=strFileName[0:strFileName.rindex('/')]
        imageFileName=strPath+'/'+strApp+'-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot.pdf'
        print(imageFileName)
        plt.savefig(imageFileName, bbox_inches='tight')

        p = sns.displot(dfForPlot, x=strMetric,  kind="kde", hue="Variant", bw_adjust=0.15, clip=(0.00,1.0) ,aspect=2, alpha=1,facet_kws=dict(legend_out=False))
        p.set(xlabel="$\it{"+strMetric+"}^{*}$")
        #p.set(ylabel="Density")
        sns.move_legend(p,"upper center",bbox_to_anchor=(.8, .9))
        p.map(plt.axvline, x=0.05, color='black', linewidth=1,linestyle='--')
        if(0):
            leg = p.axes.flat[0].get_legend()
            if leg is None: leg = p._legend
            #print(leg.get_title().get_text())
            strTitle= str(leg.get_title().get_text()).center(20) + 'Hot ref. blocks'.center(15)+'Num. of'.center(15)+'Block pairs with'.rjust(15)+'\n' \
                        +str(' ').center(20) + 'access(%)'.center(15)+'block pairs'.center(15)+"$\it{"+strMetric+"}^{*}$ >= 0.05".center(15)
            new_title = str(leg.get_title().get_text()).center(28) + 'Hot Access(%)'.ljust(15)+'Num. block pairs'.center(15)+'Above 0.05'.rjust(15)
            leg.set_title(strTitle)
            for t in leg.texts:
                t.set_text(str(t.get_text()).ljust(14) +  str(round((dictVarAccess[t.get_text()][1]*100/dictVarAccess[t.get_text()][0]),2)).center(15) \
                           + dictSD[t.get_text()][0].center(15)+ str(round(float(dictSD[t.get_text()][1])*float(dictSD[t.get_text()][0])/100)).center(15))
                t.set_ha('right')
        p.set(title=strApp+" variants - composite $\it{"+strMetric+"}^{*}$ density plot")
        strPath=strFileName[0:strFileName.rindex('/')]
        imageFileName=strPath+'/'+strApp+'-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot-kde.pdf'
        print(imageFileName)
        plt.savefig(imageFileName, bbox_inches='tight')


        strMetric="SA"
        #stat='percent',common_norm=False,
        p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,  multiple="dodge", \
                        kde=False, kde_kws={'bw_adjust':0.15,'clip':(0.05,1.0)},  aspect=2, alpha=1,facet_kws=dict(legend_out=False))
        #p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,  multiple="dodge", \
        #                kde=False, kde_kws={'bw_adjust':0.15,'clip':(0.05,1.0)},  aspect=2, alpha=1,facet_kws=dict(legend_out=False))

        p.set(xlabel="$\it{"+strMetric+"}^{*}$")
        p.set(ylabel="Number of block pairs")
        p.set(xticks=np.arange(0,1.05,0.05))
        p.set_xticklabels(np.round(np.arange(0,1.05,0.05),2))
        sns.move_legend(p,"upper center",bbox_to_anchor=(.8, .9))
        if (0):
            leg = p.axes.flat[0].get_legend()
            if leg is None: leg = p._legend
            #print(leg.get_title().get_text())
            strTitle= str(leg.get_title().get_text()).center(20) + 'Hot ref. blocks'.center(15)+'Num. of'.center(15)+'Block pairs with'.rjust(15)+'\n' \
                        +str(' ').center(20) + 'access(%)'.center(15)+'block pairs'.center(15)+"$\it{"+strMetric+"}^{*}$ >= 0.25".center(15)
            leg.set_title(strTitle)
            for t in leg.texts:
                t.set_text(str(t.get_text()).ljust(14) +  str(round((dictVarAccess[t.get_text()][1]*100/dictVarAccess[t.get_text()][0]),2)).center(15) \
                           + dictSA[t.get_text()][0].center(15)+ str(round(float(dictSA[t.get_text()][1])*float(dictSA[t.get_text()][0])/100)).center(15))
                t.set_ha('right')
        #p.fig.text(0.8, 0.7, strSAText,  ha='left', verticalalignment='top',bbox=dict(boxstyle="round",facecolor='none', edgecolor='grey',alpha=0.5))
        p.set(title=strApp+" variants - composite $\it{"+strMetric+"}^{*}$")
        p.map(plt.axvline, x=0.25, color='black', linewidth=1,linestyle='--')
        strPath=strFileName[0:strFileName.rindex('/')]
        imageFileName=strPath+'/'+strApp+'-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot.pdf'
        print(imageFileName)
        plt.savefig(imageFileName, bbox_inches='tight')

        p = sns.displot(dfForPlot, x=strMetric, hue="Variant", kind="kde", bw_adjust=0.15, aspect=2, alpha=1,facet_kws=dict(legend_out=False))
        p.set(xlabel="$\it{"+strMetric+"}^{*}$")
        #p.set(xticks=np.arange(0,1.05,0.05))
        #p.set_xticklabels(np.round(np.arange(0,1.05,0.05),2))
        sns.move_legend(p,"upper center",bbox_to_anchor=(.8, .9))
        p.map(plt.axvline, x=0.25, color='black', linewidth=1,linestyle='--')
        if(0):
            leg = p.axes.flat[0].get_legend()
            if leg is None: leg = p._legend
            #print(leg.get_title().get_text())
            strTitle= str(leg.get_title().get_text()).center(20) + 'Hot ref. blocks'.center(15)+'Num. of'.center(15)+'Block pairs with'.rjust(15)+'\n' \
                        +str(' ').center(20) + 'access(%)'.center(15)+'block pairs'.center(15)+"$\it{"+strMetric+"}^{*}$ >= 0.25".center(15)
            leg.set_title(strTitle)
            for t in leg.texts:
                t.set_text(str(t.get_text()).ljust(14) +  str(round((dictVarAccess[t.get_text()][1]*100/dictVarAccess[t.get_text()][0]),2)).center(15) \
                           + dictSA[t.get_text()][0].center(15)+ str(round(float(dictSA[t.get_text()][1])*float(dictSA[t.get_text()][0])/100)).center(15))
                t.set_ha('right')
        p.set(title=strApp+" variants - composite $\it{"+strMetric+"}^{*}$ density plot")
        strPath=strFileName[0:strFileName.rindex('/')]
        imageFileName=strPath+'/'+strApp+'-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot-kde.pdf'
        print(imageFileName)
        plt.savefig(imageFileName, bbox_inches='tight')

        #p.set(xlabel="$\it{SI}$-$\it{"+strMetric+ "}$")
        #sns.displot(dfForPlot, x="SA", hue="Variant", bins=100,  multiple="dodge", aspect=2)# col="Variant")
        #sns.displot(dfForPlot, x="SA", hue="Variant", bins=binArray,  multiple="dodge", aspect=2)# col="Variant")
        #sns.displot(dfForPlot, x="SA", hue="Variant", bins=100,  col="Variant",legend=False)
        #plt.show()

#def read_file_df(strFileName, intHotRef:None, intHotAff:None, strApp,dfForPlot):
#def plot_app(strApp, optionHotRef, optionHotAff)

#plot_app('miniVite-nuke', 0, 0)
#plot_app('miniVite-nuke', 0, 2) # used - all hm - more blocks
#plot_app('xsb-noflto', 2, 2) # used
#plot_app('xsb-noinline', 2, 1) # used
#plot_app('xsb-noinline', 2, 2)
#plot_app('xsb-noflto', 2, 1) # unused
#plot_app('hicoo-tensor',0,1)
#plot_app('HiParTI-matrix',0,2)

#Data for paper
flPlot=True
#plot_app('miniVite',0,2,flPlot) # used for Plots in paper- all hm - more blocks
#plot_app('miniVite',2,2,flPlot) # used for score table data - all hm - more blocks
#plot_app('HiParTI-HiCOO tensor MTTKRP',2,2, flPlot) # used for score table data - all hm - more blocks
#plot_app('xsb-noflto-other-grid', 2, 2,flPlot) # used for score table data - all hm - more blocks
#plot_app('xsb-noflto-grid-index', 2, 2,flPlot) # used for score table data - all hm - more blocks
#plot_app('xsb-noflto-mat-energy',2,2,flPlot) # used for score table data - all hm - more blocks

#plot_app('xsb-noflto-all',2,2,True)

plot_app('miniVite',0,2,flPlot)
#plot_app('xsb-noflto-mat-energy',2  ,2,flPlot)
#plot_app('xsb-noflto-grid-index', 2, 3,flPlot)

# Trial for targetted blocks in score
if(0):
    plot_app('miniVite',2,3,flPlot)
    plot_app('HiParTI-HiCOO tensor MTTKRP',2,3, flPlot)
    plot_app('xsb-noflto-other-grid', 2, 3,flPlot)
    plot_app('xsb-noflto-grid-index', 2, 3,flPlot)
    plot_app('xsb-noflto-mat-energy',2,3,flPlot)