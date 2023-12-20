import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
import re

SI_good=63
def read_file_df(strFileName, intHotRef:None, intHotAff:None, strApp,dfForPlot):
    # Three ways to choose hot ref blocks
        # intHotRef = 0 - Option 1 - 90% in range Hot-Access - DEFAULT
        # intHotRef = 1 - Option 2 - 90% of total access
        # intHotRef = 2 - Option 3 - ALL blocks - all reference blocks for whole application
    # Two ways to choose hot affinity blocks
        # intHotAff = 0 - Option 1 - only hot lines and self
        # intHotAff = 1 - Option 2 - only hot lines and self, self-1, self+1, self+2 - DEFAULT
        # intHotAff = 2 - Option 3 - all heatmap rows

    if(intHotRef==None):
        intHotRef =0
    if(intHotAff==None):
        intHotAff =1
    df_local=pd.read_csv(strFileName)
    df_local.insert(0,'Variant',strApp)
    #print(df_local.shape)
    df_local.drop('Unnamed: 0',inplace=True,axis=1)
    df_local.sort_values(by=['Access'],ascending=False,inplace=True)
    #print(df_local['Access'].sum())
    sumAccessRefBlocks = df_local['Access'].sum()
    val = pd.Index(df_local.Access.cumsum()).get_loc(int((0.9*sumAccessRefBlocks)), 'nearest', tolerance=int(0.05*sumAccessRefBlocks))
    #print(val)
    print(' All lines above access count ' , df_local.at[val,'Access'])
    accessThreshold=df_local.at[val,'Access']

    dfCols = df_local.columns.to_list()
    listAffinityLines=[]
    if (intHotAff==0):
        pattern = re.compile('SD-.*-.*-.*')
        listAffinityLines=list(filter(pattern.match, dfCols))
        listAffinityLines.append('SD-self')
    if (intHotAff==1):
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
    for i in range(0,len(listAffinityLines)):
        listAffinityLines[i] = listAffinityLines[i].replace('SD-','')
    print('hot lines in affinity', listAffinityLines)
    pattern = re.compile('self.*')
    listSelfAffLines = list(filter(pattern.match, listAffinityLines))
    print(' listSelfAffLines ' , listSelfAffLines)

    for index, row in df_local.iterrows():
        for i in range(0,len(listAffinityLines)):
            flInclude=True
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
            if(checkAffBlock in listAffinityLines):
                print ( strApp , ' Not incluidng contig block for ', regPageBlock, listAffinityLines[i], checkAffBlock)
                flInclude=False

            if(regPageBlock in listAffinityLines and listAffinityLines[i] =='self'):
                print (strApp , 'Not incluidng self for ', regPageBlock, listAffinityLines[i])
                flInclude=False

            if((flInclude==True ) and \
                        ((intHotRef == 0 and row['Hot-Access']>= 0.1) \
                                     or (intHotRef == 1 and row['Access']>= accessThreshold)  or \
                                        (intHotRef == 2 ))):
                if(~np.isnan(row['SI-'+listAffinityLines[i]])) and (row['SI-'+listAffinityLines[i]]):
                    blkSIValue = row['SI-'+listAffinityLines[i]]
                if(~np.isnan(row['SD-'+listAffinityLines[i]])) and (row['SD-'+listAffinityLines[i]]):
                    blkSDValue = row['SD-'+listAffinityLines[i]]
                if(~np.isnan(row['SP-'+listAffinityLines[i]])) and (row['SP-'+listAffinityLines[i]]):
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
                    dfForPlot.loc[len(dfForPlot)]=[strApp,regPageBlock,round((blkSDValue * valSIDiscount),3),round((blkSAValue * valSIDiscount),3)]

    print(dfForPlot.shape)
    #print(dfForPlot)

def plot_app(strApp, optionHotRef, optionHotAff):
    strFileName=''
    strPath='/Users/suri836/Projects/spatial_rud/spatial_pages_exp/'
    dfForPlot= pd.DataFrame(columns=['Variant', 'reg-page-blk', 'SD', 'SA'])
    if(strApp.lower()=='minivite'):
        strFileName='miniVite/hot_lines/miniVite-v1-1-A0000001-SD-SP-SI-df.csv'
        strAppVar='miniVite-v1'
        strFileName=strPath+strFileName
        read_file_df(strFileName, optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)

        strFileName='miniVite/hot_lines/miniVite-v2-1-A0000010-4-A0002000-SD-SP-SI-df.csv'
        strAppVar='miniVite-v2'
        strFileName=strPath+strFileName
        read_file_df(strFileName, optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)

        strFileName='miniVite/hot_lines/miniVite-v3-1-A0000001-5-A0001200-SD-SP-SI-df.csv'
        strAppVar='miniVite-v3'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)

    if(strApp.lower()=='xsb'):
        strFileName='XSBench/openmp-threading-noinline/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1/XSB-rd-EVENT_OPT_k1-0-A0000000-1-B0000000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-threading-noinline/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0/XSB-rd-EVENT_k0-0-A0000000-1-B0000000-2-B0010000-3-B0020000-4-B0030000-5-B0040000-6-B0050000-7-B0060000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-threading-noinline/memgaze-xs-read/'

    print(dfForPlot)
    dfForPlot.sort_values(by='Variant', ascending=True, inplace=True)
    arrVariants=list(set(dfForPlot["Variant"].to_list()))
    print(arrVariants)
    for i in range(len(arrVariants)):
        print(arrVariants[i], " SA all ",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['SA'] >0 ).sum())
        print(arrVariants[i], " SA good ",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['SA'] >=0.25).sum())
        print(arrVariants[i], " SD all ",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['SD'] >0 ).sum())
        print(arrVariants[i], " SD ",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['SD'] >=0.05).sum())

    binArray=[0.01,0.02,0.03,0.04,0.05,0.1,0.2,0.4,0.6,0.8,1.0]
    listDarkPalette=['#0343df','#650021','#3f9b0b']

    strMetric="SD"
    p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,   multiple="dodge",  aspect=2, alpha=1,facet_kws=dict(legend_out=False))#,stat="percent")# col="App")
    p.set(xlabel="$\it{"+strMetric+"}^{*}$")
    p.set(ylabel="Count of block pairs")
    sns.move_legend(p,"upper center",ncol=len(arrVariants),bbox_to_anchor=(.55, .95))
    p.set(title=strApp+" variants - composite $\it{"+strMetric+"}^{*}$")
    p.map(plt.axvline, x=0.05, color='black', dashes=(2, 1), zorder=0,linewidth=2)
    #ax.axvline(x='0.05', ymin=ymin, ymax=ymax, linewidth=1, color='black')

    strPath=strFileName[0:strFileName.rindex('/')]
    imageFileName=strPath+'/composite-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot-percent.pdf'
    print(imageFileName)
    plt.savefig(imageFileName, bbox_inches='tight')

    strMetric="SA"
    p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,  multiple="dodge",   aspect=2, alpha=1,facet_kws=dict(legend_out=False))# col="App")
    #p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,  multiple="dodge", aspect=2,kde=True,palette=listDarkPalette,alpha=1,facet_kws=dict(legend_out=False))# col="App")
    p.set(xlabel="$\it{"+strMetric+"}^{*}$")
    p.set(ylabel="Count of block pairs")
    p.set(xticks=np.arange(0,1.05,0.05))
    p.set_xticklabels(np.round(np.arange(0,1.05,0.05),2))
    sns.move_legend(p,"upper center",ncol=len(arrVariants),bbox_to_anchor=(.55, .95))
    p.set(title=strApp+" variants - composite $\it{"+strMetric+"}^{*}$")
    p.map(plt.axvline, x=0.25, color='black', dashes=(2, 1), zorder=0,linewidth=2)
    strPath=strFileName[0:strFileName.rindex('/')]
    imageFileName=strPath+'/composite-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot-percent.pdf'
    print(imageFileName)
    plt.savefig(imageFileName, bbox_inches='tight')

    #p.set(xlabel="$\it{SI}$-$\it{"+strMetric+ "}$")
    #sns.displot(dfForPlot, x="SA", hue="Variant", bins=100,  multiple="dodge", aspect=2)# col="Variant")
    #sns.displot(dfForPlot, x="SA", hue="Variant", bins=binArray,  multiple="dodge", aspect=2)# col="Variant")
    #sns.displot(dfForPlot, x="SA", hue="Variant", bins=100,  col="Variant",legend=False)
    #plt.show()

#def read_file_df(strFileName, intHotRef:None, intHotAff:None, strApp,dfForPlot):
#def plot_app(strApp, optionHotRef, optionHotAff)
#plot_app('miniVite', 0, 0)
#plot_app('miniVite', 0, 1)
#plot_app('miniVite', 1, 0)
#plot_app('miniVite', 1, 1)
#plot_app('miniVite', 1, 2)
plot_app('xsb', 2, 1)
plot_app('xsb', 2, 2)

