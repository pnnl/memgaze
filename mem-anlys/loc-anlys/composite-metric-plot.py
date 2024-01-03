import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
import re
import matplotlib.patches as mpatches
plt.rcParams['font.family'] = 'monospace'
#plt.rcParams.update({"text.usetex": True,})

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
    accessThreshold=df_local.at[val,'Access']
    print(' All lines above access count ' , accessThreshold)
    #accessThreshold=round((0.05*sumAccessRefBlocks),0)
    #print(' All lines above access count ' , accessThreshold)


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
    print('Columns in df_local ', df_local.columns.to_list())
    pattern = re.compile('SD-.*')
    dfSDCols =  list(filter(pattern.match, df_local.columns.to_list()))
    for item in listAffinityLines:
        if (not (item in dfSDCols)):
            print( 'item ', item, dfSDCols)
            listAffinityLines.remove(item)

    for i in range(0,len(listAffinityLines)):
        listAffinityLines[i] = listAffinityLines[i].replace('SD-','')

    print('hot lines in affinity', listAffinityLines)
    pattern = re.compile('self.*')
    listSelfAffLines = list(filter(pattern.match, listAffinityLines))
    print(' listSelfAffLines ' , listSelfAffLines)
    listHotRefLines = df_local[df_local['Hot-Access'] >0.1]['reg-page-blk'].to_list()
    print('listHotRefLines ', listHotRefLines)
    for index, row in df_local.iterrows():
        for i in range(0,len(listAffinityLines)):
            flInclude=True
            flIncludeSA = True
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
                #print ( strApp , ' Not incluidng contig block for ', regPageBlock, listAffinityLines[i], checkAffBlock)
                flInclude=False

            if(regPageBlock in listAffinityLines and listAffinityLines[i] =='self'):
                #print (strApp , 'Not incluidng self for ', regPageBlock, listAffinityLines[i])
                flInclude=False

            if(regPageBlock == listAffinityLines[i] ):
                #print (strApp , 'Not incluidng self for ', regPageBlock, listAffinityLines[i])
                print(regPageBlock, 'self self ',  listAffinityLines[i])
                flIncludeSA=False

            if((flInclude==True ) and \
                        ((intHotRef == 0 and row['Hot-Access']>= 0.1) \
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
                    if(flIncludeSA == True):
                        dfForPlot.loc[len(dfForPlot)]=[strApp,regPageBlock,round((blkSDValue * valSIDiscount),3),round((blkSAValue * valSIDiscount),3)]
                    else:
                        dfForPlot.loc[len(dfForPlot)]=[strApp,regPageBlock,round((blkSDValue * valSIDiscount),3),np.nan]
    print(dfForPlot.shape)
    #print(dfForPlot)
    dfForPlot.to_csv('/Users/suri836/Projects/spatial_rud/spatial_pages_exp/miniVite/bignuke_run/mini-memgaze-ld/miniVite-v3-memgaze-trace-b16384-p5000000-anlys/plot_df.csv')

def plot_app(strApp, optionHotRef, optionHotAff):
    strFileName=''
    strPath='/Users/suri836/Projects/spatial_rud/spatial_pages_exp/'
    dfForPlot= pd.DataFrame(columns=['Variant', 'reg-page-blk', 'SD', 'SA'])

    if(strApp.lower()=='minivite-nuke'):
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

    if(strApp.lower()=='minivite'):
        strFileName='miniVite/bignuke_run/mini-memgaze-ld/miniVite-v1-memgaze-trace-b16384-p5000000-anlys/miniVite-v1-1-A0001000-2-B0000000-3-B1100000-SD-SP-SI-df.csv'
        strAppVar='miniVite-v1'
        strFileName=strPath+strFileName
        read_file_df(strFileName, optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)

        strFileName='miniVite/bignuke_run/mini-memgaze-ld/miniVite-v2-memgaze-trace-b16384-p5000000-anlys/miniVite-v2-2-A0002000-3-B0000000-4-B1000000-SD-SP-SI-df.csv'
        strAppVar='miniVite-v2'
        strFileName=strPath+strFileName
        read_file_df(strFileName, optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)

        strFileName='miniVite/bignuke_run/mini-memgaze-ld/miniVite-v3-memgaze-trace-b16384-p5000000-anlys/miniVite-v3-4-A0002000-5-B0000000-6-B1100000-SD-SP-SI-df.csv'
        strAppVar='miniVite-v3'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)
        strFileName=strPath+'miniVite/bignuke_run/mini-memgaze-ld/'

    if(strApp == 'HiParTI-HiCOO tensor MTTKRP'):
        strFileName='HICOO-tensor/mttsel-re-1-b16384-p4000000-U-0/hot_lines/HiParTI-HiCOO-Lexi-0-B0000000-SD-SP-SI-df.csv'
        strAppVar='Lexi'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)

        strFileName='HICOO-tensor/mttsel-re-2-b16384-p4000000-U-0/hot_lines/HiParTI-HiCOO-BFS-0-B0000000-SD-SP-SI-df.csv'
        strAppVar='BFS'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)

        strFileName='HICOO-tensor/mttsel-re-0-b16384-p4000000-U-0/hot_lines/HiParTI-HiCOO-0-B0000000-SD-SP-SI-df.csv'
        strAppVar='Default'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)

        strFileName='HICOO-tensor/mttsel-re-3-b16384-p4000000-U-0/hot_lines/HiParTI-HiCOO-Random-0-B0000000-SD-SP-SI-df.csv'
        strAppVar='Random'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)
        strFileName=strPath+'HICOO-tensor/'

    if(strApp == 'HiParTI-matrix'):
        strFileName='HICOO-matrix/4096-same-iter/hot_lines/csr/HiParTi-CSR-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='CSR'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)

        strFileName='HICOO-matrix/4096-same-iter/hot_lines/coo_u_0/HiParTi-COO-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='COO'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)

        strFileName='HICOO-matrix/4096-same-iter/hot_lines/coo_u_1/HiParTi-COO-Reduce-4-A3000000-SD-SP-SI-df.csv'
        strAppVar='COO-Reduce'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)

        strFileName='HICOO-matrix/4096-same-iter/hot_lines/hicoo_u_0/HiParTi-HiCOO-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='HiCOO'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)

        strFileName='HICOO-matrix/4096-same-iter/hot_lines/hicoo_u_1/HiParTi-HiCOO-Schedule-2-B0010000-SD-SP-SI-df.csv'
        strAppVar='HiCOO-Schedule'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)

        strFileName=strPath+'HICOO-matrix/4096-same-iter/hot_lines/'

    if(strApp.lower()=='xsb-noinline'):
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

    if(strApp.lower()=='xsb-noflto'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p3000000-event-k-0/XSB-rd-EVENT_k0-0-A0000000-1-B0000000-2-B0000001-3-B0000002-4-B0000003-5-B0000004-6-B1000000-7-HotIns-11-8-HotIns-12-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p3000000-event-k-1/XSB-rd-EVENT_OPT_k1-0-A0000000-1-B0000000-2-B0000001-3-B0000002-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read/'


    #print(dfForPlot)
    dfForPlot.sort_values(by='Variant', ascending=True, inplace=True)
    arrVariants=list(set(dfForPlot["Variant"].to_list()))
    arrVariants.sort()
    print(arrVariants)
    for i in range(len(arrVariants)):
        print(arrVariants[i], " SA all ",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['SA'] >0 ).sum())
        print(arrVariants[i], " SA good ",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['SA'] >=0.25).sum())
        print(arrVariants[i], " SD all ",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['SD'] >0 ).sum())
        print(arrVariants[i], " SD good",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['SD'] >=0.05).sum())

    binArray=[0.01,0.02,0.03,0.04,0.05,0.1,0.2,0.4,0.6,0.8,1.0]
    listDarkPalette=['#0343df','#650021','#3f9b0b']

    # Area under good region
    #strSDText='Variant'.center(12) +' - Number of - Good (%)\n' + 'block pairs'.center(40)+' \n'
    strSDText='Variant'.center(18) +'- '+'Num. of block pairs'.center(22)+'\n' #' - Good (%) \n'
    strSAText='Variant'.center(18) +'- '+'Num. of block pairs'.center(22)+'\n' #' - Good (%) \n'
    dictSA={}
    dictSD={}

    for i in range(len(arrVariants)):
        for strMetric in ['SD', 'SA']:
            dfVariant = dfForPlot[dfForPlot["Variant"] ==arrVariants[i]]
            print(arrVariants[i], strMetric, dfVariant.shape)

            dfArea=dfVariant[strMetric].dropna()
            print(arrVariants[i], strMetric, dfArea.shape)
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
            print(strApp , 'variant ', arrVariants[i], ' number of block pairs ', numBlockPairs,  'metric ', strMetric, area)
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


    # Plots
    strMetric="SD"
    p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,   multiple="dodge",  stat='percent', common_norm=False, \
                      kde=True, kde_kws={'bw_adjust':0.2}, aspect=2, alpha=1,facet_kws=dict(legend_out=False))
    #p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,   multiple="dodge",  stat='percent', common_norm=False, \
    #                   kde=True, kde_kws={'bw_adjust':1.0}, aspect=2, alpha=1,facet_kws=dict(legend_out=False))
    p.set(xlabel="$\it{"+strMetric+"}^{*}$")
    p.set(ylabel="Percentage of block pairs")
    sns.move_legend(p,"upper center",bbox_to_anchor=(.8, .9))
    leg = p.axes.flat[0].get_legend()
    if leg is None: leg = p._legend
    print(leg.get_title().get_text())
    new_title = str(leg.get_title().get_text()).rjust(15) + ' - Num. of block pairs'
    leg.set_title(new_title)
    for t in leg.texts:
        t.set_text(str(t.get_text()).ljust(14) + '- '+ dictSA[t.get_text()][0].rjust(8))
        t.set_ha('right')
    p.set(title=strApp+" variants - composite $\it{"+strMetric+"}^{*}$")
    #p.map(plt.axvline, x=0.045, color='black', linewidth=1)
    #ax.axvline(x='0.05', ymin=ymin, ymax=ymax, linewidth=1, color='black')
    strPath=strFileName[0:strFileName.rindex('/')]
    imageFileName=strPath+'/composite-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot-perc.pdf'
    print(imageFileName)
    plt.savefig(imageFileName, bbox_inches='tight')

    p = sns.displot(dfForPlot, x=strMetric,  kind="kde", hue="Variant", bw_adjust=0.2, common_norm=False, aspect=2, alpha=1,facet_kws=dict(legend_out=False))
    p.set(xlabel="$\it{"+strMetric+"}^{*}$")
    #p.set(ylabel="Density")
    sns.move_legend(p,"upper center",bbox_to_anchor=(.8, .9))
    leg = p.axes.flat[0].get_legend()
    if leg is None: leg = p._legend
    print(leg.get_title().get_text())
    new_title = str(leg.get_title().get_text()).rjust(15) + ' - Num. of block pairs'
    leg.set_title(new_title)
    for t in leg.texts:
        t.set_text(str(t.get_text()).ljust(14) + '- '+ dictSD[t.get_text()][0].rjust(8))
        t.set_ha('right')
    p.set(title=strApp+" variants - composite $\it{"+strMetric+"}^{*}$")
    strPath=strFileName[0:strFileName.rindex('/')]
    imageFileName=strPath+'/composite-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot-kde.pdf'
    print(imageFileName)
    plt.savefig(imageFileName, bbox_inches='tight')


    strMetric="SA"
    p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,  multiple="dodge",  stat='percent',common_norm=False,\
                    kde=True, kde_kws={'bw_adjust':0.2}, aspect=2, alpha=1,facet_kws=dict(legend_out=False))
    p.set(xlabel="$\it{"+strMetric+"}^{*}$")
    p.set(ylabel="Percentage of block pairs")
    p.set(xticks=np.arange(0,1.05,0.05))
    p.set_xticklabels(np.round(np.arange(0,1.05,0.05),2))
    sns.move_legend(p,"upper center",bbox_to_anchor=(.8, .9))
    leg = p.axes.flat[0].get_legend()
    if leg is None: leg = p._legend
    print(leg.get_title().get_text())
    new_title = str(leg.get_title().get_text()).rjust(15) + ' - Num. of block pairs'
    leg.set_title(new_title)
    for t in leg.texts:
        t.set_text(str(t.get_text()).ljust(14) + '- '+ dictSA[t.get_text()][0].rjust(8))
        t.set_ha('right')
    #p.fig.text(0.8, 0.7, strSAText,  ha='left', verticalalignment='top',bbox=dict(boxstyle="round",facecolor='none', edgecolor='grey',alpha=0.5))
    p.set(title=strApp+" variants - composite $\it{"+strMetric+"}^{*}$")
    #p.map(plt.axvline, x=0.225, color='black', linewidth=1)
    strPath=strFileName[0:strFileName.rindex('/')]
    imageFileName=strPath+'/composite-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot-perc-noself.pdf'
    print(imageFileName)
    plt.savefig(imageFileName, bbox_inches='tight')

    p = sns.displot(dfForPlot, x=strMetric, hue="Variant", kind="kde", bw_adjust=0.2,  common_norm=False, aspect=2, alpha=1,facet_kws=dict(legend_out=False))
    p.set(xlabel="$\it{"+strMetric+"}^{*}$")
    #p.set(xticks=np.arange(0,1.05,0.05))
    #p.set_xticklabels(np.round(np.arange(0,1.05,0.05),2))
    sns.move_legend(p,"upper center",bbox_to_anchor=(.8, .9))
    leg = p.axes.flat[0].get_legend()
    if leg is None: leg = p._legend
    print(leg.get_title().get_text())
    new_title = str(leg.get_title().get_text()).rjust(15) + ' - Num. of block pairs'
    leg.set_title(new_title)
    for t in leg.texts:
        t.set_text(str(t.get_text()).ljust(14) + '- '+ dictSA[t.get_text()][0].rjust(8))
        t.set_ha('right')
    p.set(title=strApp+" variants - composite $\it{"+strMetric+"}^{*}$")
    strPath=strFileName[0:strFileName.rindex('/')]
    imageFileName=strPath+'/composite-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot-kde.pdf'
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

#Data for paper
plot_app('miniVite',0,2) # used - all hm - more blocks
plot_app('HiParTI-HiCOO tensor MTTKRP',0,2)
plot_app('HiParTI-matrix',0,2)

