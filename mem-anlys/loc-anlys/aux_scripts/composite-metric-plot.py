import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
import re
import matplotlib.patches as mpatches
plt.rcParams['font.family'] = 'sans-serif'
sns.set(font_scale=4.0,style="white",palette='deep')
#plt.rcParams.update({"text.usetex": True,})


SI_good=49
# the value is 50, but SI calculations in affinity analysis start at 0, so the first 50 entries go to bin 1
# Represent 1/4 of load queue in Bignuke - actual value 192
max_bins=5
# Samples are generally 250 accesses with 8K buffer from Memgaze

def read_file_df(strFileName, intHotRef:None, intHotAff:None, strApp,dfForPlot,dictVarAccess, listUnRealizedpairs):
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
    #print(df_local.Access.cumsum())
    #print(pd.Index(df_local.Access.cumsum()))
    val = pd.Index(df_local.Access.cumsum()).get_indexer([int((0.9*sumAccessRefBlocks))], method="nearest", tolerance=int(0.05*sumAccessRefBlocks))
    #print(val[0])
    accessThreshold=df_local.at[val[0],'Access']
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
    if (intHotAff==2 or intHotAff==4):
        pattern = re.compile('SD-.*')
        listAffinityLines=list(filter(pattern.match, dfCols))

    for item in listAffinityLines:
        if ('stack' in item.lower()):
            listAffinityLines.remove(item)

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
    listAffinityLines.sort()
    print('hot lines in affinity before region', listAffinityLines)
    if(intHotAff ==4):
        for x in listAffinityLines:
            if((len(x.split('-')) >=3) and (not x[0] in listRefRegions)):
                listAffinityLines.remove(x)

        listAffinityLines.sort()
    print('hot lines in affinity', listAffinityLines)

    pattern = re.compile('self.*')
    listSelfAffLines = list(filter(pattern.match, listAffinityLines))
    #print(' listSelfAffLines ' , listSelfAffLines)
    listHotRefLines = df_local[df_local['Hot-Access'] >0.1]['reg-page-blk'].to_list()
    listHotRefLines.sort()
    print('Reference (unused) - listHotRefLines ', listHotRefLines)
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
    # When checking realized affinity, keep log of unrealized affinity pairs
    # Lines that can be changed to improve realized vs potential affinity
    listFinalRefLines=[]
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
            if(regPageBlock == listAffinityLines[i] or listAffinityLines[i] == 'self'):
                #print('line 150 flIncludeSA ', flIncludeSA)
                flIncludeSA=False
                #print('line 151 flIncludeSA ', flIncludeSA, listAffinityLines[i])
            if((intHotAff==3)):
                #print('opt 3 ', listAffinityLines[i], flIncludeSA, flInclude)
                if(not(listAffinityLines[i] in ['self', 'self-1', 'self+1'] or len(listAffinityLines[i].split('-'))>=3)):
                    flIncludeSD=False
                if(not(listAffinityLines[i] in ['self+1', 'self+2'] or len(listAffinityLines[i].split('-'))>=3)):
                    #print('non SA ', listAffinityLines[i])
                    flIncludeSA=False
                #else:
                    #print ('------ else SA ', listAffinityLines[i], flIncludeSA, ' flInclude ', flInclude, row['Hot-Access'])
            if((flInclude==True ) and \
                        #((intHotRef == 0 and (row['Hot-Access']>= 0.1 and (row['Access']/row['TotalAccess'] >= 0.0025))) \
                            ((intHotRef == 0 and  (row['Hot-Access']>= 0.1)) \
                                     # this needs to be changed
                                     or (intHotRef == 1 and row['Access']>= accessThreshold)  or \
                                        (intHotRef == 2 ))):
                #print('in include', row['SI-'+listAffinityLines[i]])
                #print ('------ in add SA ', listAffinityLines[i], flIncludeSA, ' flInclude ', flInclude, row['Hot-Access'], row['SI-'+listAffinityLines[i]],row['SP-'+listAffinityLines[i]] )
                listFinalRefLines.append(regPageBlock)
                blkSIValue=np.nan
                blkSAValue=np.nan
                blkSDValue=np.nan
                # This doesnt work if SI is zero
                #if((row['SI-'+listAffinityLines[i]])) and (~np.isnan(row['SI-'+listAffinityLines[i]])):
                if(~np.isnan(row['SI-'+listAffinityLines[i]])):
                    blkSIValue = row['SI-'+listAffinityLines[i]]
                if((row['SD-'+listAffinityLines[i]])) and (~np.isnan(row['SD-'+listAffinityLines[i]])):
                    blkSDValue = row['SD-'+listAffinityLines[i]]
                if((row['SP-'+listAffinityLines[i]])) and (~np.isnan(row['SP-'+listAffinityLines[i]])):
                    blkSAValue = row['SP-'+listAffinityLines[i]]
                if (not(np.isnan(blkSIValue))):
                    #print('in SI , SA -', flIncludeSA, ' SD ', flIncludeSD, listAffinityLines[i])
                    valSIBin = int(blkSIValue /SI_good) +1
                    if (blkSIValue > SI_good):
                        print('blkSIValue > ',str(SI_good),' val ', str(blkSIValue), str(blkSDValue), str(blkSAValue))
                    if(valSIBin > max_bins):
                        valSIBin = max_bins
                    valSIDiscount = float((max_bins-valSIBin+1)/max_bins)
                    #print (regPageBlock,listAffinityLines[i],blkSDValue, blkSIValue, blkSAValue, valSIDiscount,flIncludeSA)
                    df_local.loc[df_local['reg-page-blk'] == regPageBlock, ['SD-'+listAffinityLines[i]]] = round((blkSDValue * valSIDiscount),3)
                    df_local.loc[df_local['reg-page-blk'] == regPageBlock, ['SP-'+listAffinityLines[i]]] = round((blkSAValue * valSIDiscount),3)
                    #print (regPageBlock, 'SI ', blkSIValue, blkSDValue, blkSAValue, 'BIN - ', valSIBin, valSIDiscount, \
                           #df_local[(df_local['reg-page-blk'] == regPageBlock)]['SD-'+listAffinityLines[i]].item(), \
                           #df_local[(df_local['reg-page-blk'] == regPageBlock)]['SP-'+listAffinityLines[i]].item() )
                    if(flIncludeSA == True and flIncludeSD ==True):
                        #print('***** both true ', regPageBlock, listAffinityLines[i], round((blkSAValue * valSIDiscount),3))
                        dfForPlot.loc[len(dfForPlot)]=[strApp,regPageBlock,row['Access'], row['Hot-Access'], round((blkSDValue * valSIDiscount),3),round((blkSAValue * valSIDiscount),3),hotAccessWeight]
                    elif (flIncludeSA == False and flIncludeSD ==True):
                        #print('***** SD true ', regPageBlock, listAffinityLines[i], round((blkSDValue * valSIDiscount),3))
                        dfForPlot.loc[len(dfForPlot)]=[strApp,regPageBlock,row['Access'], row['Hot-Access'], round((blkSDValue * valSIDiscount),3),np.nan,hotAccessWeight]
                    elif (flIncludeSA == True and flIncludeSD ==False):
                        #print('**** SA true ', regPageBlock, listAffinityLines[i], round((blkSAValue * valSIDiscount),3))
                        dfForPlot.loc[len(dfForPlot)]=[strApp,regPageBlock,row['Access'], row['Hot-Access'], np.nan, round((blkSAValue * valSIDiscount),3),hotAccessWeight]

                    # Unrealized - check in intHotAff == 2
                    if(intHotAff ==2):
                        if(not(listAffinityLines[i] in ['self+1', 'self+2'] or len(listAffinityLines[i].split('-'))>=3)):
                            if(listAffinityLines[i] != 'self' and regPageBlock != listAffinityLines[i] and not(np.isnan(blkSAValue))):
                                listUnRealizedpairs.append((strApp,regPageBlock, listAffinityLines[i], row['Access'], row['Hot-Access'], round((blkSAValue * valSIDiscount),3), 'SA'))
                        if(not(listAffinityLines[i] in ['self', 'self-1', 'self+1'] or len(listAffinityLines[i].split('-'))>=3)):
                           if(not(np.isnan(blkSDValue))):
                               listUnRealizedpairs.append((strApp,regPageBlock, listAffinityLines[i], row['Access'], row['Hot-Access'], round((blkSDValue * valSIDiscount),3),'SD'))

                #else:
                #    if(flIncludeSA == True and flIncludeSD ==True):
                #        dfForPlot.loc[len(dfForPlot)]=[strApp,regPageBlock,row['Access'], row['Hot-Access'], 0, 0,hotAccessWeight]
                    #if(strApp=='Random' or strApp=='Default'):
                    #    print(row['Access'], row['TotalAccess'])

    listFinalRefLines=list(set(listFinalRefLines))
    listFinalRefLines.sort()
    #print('Actual (used) - listFinalRefLines ', listFinalRefLines)
    #print(*listUnRealizedpairs,sep='\n')
    #print(dfForPlot.shape)
    #print(dfForPlot)
    #filename='/Users/suri836/Projects/spatial_rud/spatial_pages_exp/XSBench/openmp-noflto/memgaze-xs-read/plot_df_ref-'+str(intHotRef)+'_aff-'+str(intHotAff)+'.csv'
    #dfForPlot.to_csv(filename)

def draw_plot(strApp, strFileName, optionHotRef, optionHotAff,dfForPlot,dictVarAccess,flPlot:bool=False):
    #print(dictVarAccess)
    print(dfForPlot.columns.to_list())
    print(dfForPlot.tail(2))
    #print(dfForPlot)
    #HACK - added to show the KDE plot without the spike
    if(strApp=='XSBench'):
        dfForPlot.loc[len(dfForPlot)]=['k0','0-2-201',40640, 1.0, 0.001, np.nan,0.426003]
        dfForPlot.loc[len(dfForPlot)]=['k1','0-2-201',40640, 1.0, 0.001, np.nan,0.426003]
        #0   XSBench-event-k0      0-2-201   40640  ...  0.05    NaN         0.426003

    print(dfForPlot.tail(2))

    #print(dfForPlot.tail)
    arrVariants=list(set(dfForPlot["Variant"].to_list()))
    arrVariants.sort()
    #print(arrVariants)
    dfForPlot['ScoreSA'] = (dfForPlot.SA * dfForPlot.HotLineWeight)
    dfForPlot['ScoreSD'] = (dfForPlot.SD * dfForPlot.HotLineWeight)
    #print(dfForPlot['SA'])
    #print(dfForPlot['ScoreSA'])
    for i in range(len(arrVariants)):
        print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " --- NO filter Region weight - SA score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSA'] ).sum()*dictVarAccess[arrVariants[i]][2])
        print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " --- NO filter Region weight - SD score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSD'] ).sum()*dictVarAccess[arrVariants[i]][2])
    #for i in range(len(arrVariants)):
        #print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " ... NO filter - SA score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSA'] ).sum())#*dictVarAccess[arrVariants[i]][2])
        #print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " ... NO filter - SD score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSD'] ).sum())#*dictVarAccess[arrVariants[i]][2])

    dfForPlot['ScoreSA'] = (dfForPlot.SA * dfForPlot.HotLineWeight).where(dfForPlot.SA >= 0.25)
    dfForPlot['ScoreSD'] = (dfForPlot.SD * dfForPlot.HotLineWeight).where(dfForPlot.SD >= 0.05)
    for i in range(len(arrVariants)):
        print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " *** Filter Region weight - SA score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSA'] ).sum()*dictVarAccess[arrVariants[i]][2])
        print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " *** Filter Region weight - SD score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSD'] ).sum()*dictVarAccess[arrVariants[i]][2])
    #for i in range(len(arrVariants)):
        #print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " ... Filter - SA score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSA'] ).sum())#*dictVarAccess[arrVariants[i]][2])
        #print(strApp, "ref - ", str(optionHotRef)," hot - ",str(optionHotAff)," ",arrVariants[i], " ... Filter - SD score",  (dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ] ['ScoreSD'] ).sum())#*dictVarAccess[arrVariants[i]][2])

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
                          kde=False, kde_kws={'bw_adjust':0.1,  'clip':(0.00,1.0)}, aspect=2, height=10, alpha=1,facet_kws=dict(legend_out=False))
        p.set(xlabel="$\it{"+strMetric+"}^{*}$")
        p.set(ylabel="Number of block pairs")
        #p.axes.flat[0].set_xlim(0.0,)
        #if(strApp=='miniVite'):
        #p.axes.flat[0].set_ylim(0,50)
        sns.move_legend(p,"upper center",bbox_to_anchor=(.8, .9))
        #plt.setp(p._legend().get_texts(), fontsize='22') # for legend text
        #plt.setp(p._legend().get_title(), fontsize='32') # for legend title

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
        p.set(title=strApp+" variants $\it{"+strMetric+"}^{*}(j|i)$")
        if(strApp == 'HiParTI-HiCOO tensor reordering'):
            p.set(title=strApp+" variants \n $\it{"+strMetric+"}^{*}(j|i)$ density plot")

        #p.map(plt.axvline, x=0.05, color='black', linewidth=1,linestyle='--')
        strPath=strFileName[0:strFileName.rindex('/')]
        imageFileName=strPath+'/'+strApp.replace(' ','-')+'-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot.pdf'
        print(imageFileName)
        plt.savefig(imageFileName, bbox_inches='tight')

        clip_limit=(0,1.0)
        p = sns.displot(dfForPlot, x=strMetric,  kind="kde", hue="Variant", bw_adjust=0.15,  clip=clip_limit ,aspect=2,height=10,linewidth=3, alpha=1,facet_kws=dict(legend_out=False))
        p.set(xlabel="$\it{"+strMetric+"}^{*}$")
        p.set(ylabel="Rel. Frequency")
        if(strApp=='XSBench'):
            sns.move_legend(p,"upper center",bbox_to_anchor=(.6, .9))
        else:
            sns.move_legend(p,"upper center",bbox_to_anchor=(.8, .9))
        p.set(title=strApp+" variants $\it{"+strMetric+"}^{*}(j|i)$ density plot")
        if(strApp == 'HiParTI-HiCOO tensor reordering'):
            p.set(title=strApp+" variants \n $\it{"+strMetric+"}^{*}(j|i)$ density plot")

        strPath=strFileName[0:strFileName.rindex('/')]
        imageFileName=strPath+'/'+strApp.replace(' ','-')+'-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot-kde.pdf'
        print(imageFileName)
        plt.savefig(imageFileName, bbox_inches='tight')

        strMetric="SA"
        #stat='percent',common_norm=False,
        p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,  multiple="dodge", \
                        kde=False, kde_kws={'bw_adjust':0.15,'clip':(0.00,1.0)},  aspect=2, alpha=1,facet_kws=dict(legend_out=False))
        #p = sns.displot(dfForPlot, x=strMetric, hue="Variant", bins=50,  multiple="dodge", \
        #                kde=False, kde_kws={'bw_adjust':0.15,'clip':(0.05,1.0)},  aspect=2, alpha=1,facet_kws=dict(legend_out=False))
        p.set(xlabel="$\it{"+strMetric+"}^{*}$")
        p.set(ylabel="Number of block pairs")
        p.axes.flat[0].set_ylim(0,50)
        #p.set(xticks=np.arange(0,1.05,0.05))
        #p.set_xticklabels(np.round(np.arange(0,1.05,0.05),2))
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
        p.set(title=strApp+" variants $\it{"+strMetric+"}^{*}(j|i)$")
        if(strApp == 'HiParTI-HiCOO tensor reordering'):
            p.set(title=strApp+" variants \n $\it{"+strMetric+"}^{*}(j|i)$ density plot")

        #p.map(plt.axvline, x=0.25, color='black', linewidth=1,linestyle='--')
        strPath=strFileName[0:strFileName.rindex('/')]
        imageFileName=strPath+'/'+strApp.replace(' ','-')+'-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot.pdf'
        print(imageFileName)
        plt.savefig(imageFileName, bbox_inches='tight')

        p = sns.displot(dfForPlot, x=strMetric, hue="Variant", kind="kde", bw_adjust=0.15, clip=(0.0,1.0), aspect=2, height=10,linewidth=3,alpha=1,facet_kws=dict(legend_out=False))
        p.set(xlabel="$\it{"+strMetric+"}^{*}$")
        p.set(ylabel="Rel. Frequency")
        sns.move_legend(p,"upper center",bbox_to_anchor=(.8, .9))
        p.set(title=strApp+" variants $\it{"+strMetric+"}^{*}(j|i)$ density plot")
        if(strApp == 'HiParTI-HiCOO tensor reordering'):
            p.set(title=strApp+" variants \n $\it{"+strMetric+"}^{*}(j|i)$ density plot")
        strPath=strFileName[0:strFileName.rindex('/')]
        imageFileName=strPath+'/'+strApp.replace(' ','-')+'-SI-'+strMetric+'_ref-'+str(optionHotRef)+'_aff-'+str(optionHotAff)+'-displot-kde.pdf'
        print(imageFileName)
        plt.savefig(imageFileName, bbox_inches='tight')

def plot_app(strApp, optionHotRef, optionHotAff,flPlot:bool=False):
    strFileName=''
    strPath='/Users/suri836/Projects/spatial_rud/spatial_pages_exp/'
    dfForPlot= pd.DataFrame(columns=['Variant', 'reg-page-blk','Access','HotLineWeight', 'SD', 'SA', 'HotAccessWeight'])
    dictVarAccess={}
    listUnRealizedpairs=[]


    if(strApp.lower()=='minivite-nuke'):
        strFileName='miniVite/hot_lines/miniVite-v1-1-A0000001-SD-SP-SI-df.csv'
        strAppVar='v1'
        strFileName=strPath+strFileName
        read_file_df(strFileName, optionHotRef,optionHotAff , strAppVar,dfForPlot,dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='miniVite/hot_lines/miniVite-v2-1-A0000010-4-A0002000-SD-SP-SI-df.csv'
        strAppVar='v2'
        strFileName=strPath+strFileName
        read_file_df(strFileName, optionHotRef,optionHotAff , strAppVar,dfForPlot,dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='miniVite/hot_lines/miniVite-v3-1-A0000001-5-A0001200-SD-SP-SI-df.csv'
        strAppVar='v3'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot,dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

    if(strApp.lower()=='minivite'):
        strFileName='miniVite/bignuke_run/mini-memgaze-ld/miniVite-v1-memgaze-trace-b16384-p5000000-anlys/miniVite-v1-1-A0001000-2-B0000000-3-B1100000-SD-SP-SI-df.csv'
        strAppVar='v1'
        strFileName=strPath+strFileName
        read_file_df(strFileName, optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='miniVite/bignuke_run/mini-memgaze-ld/miniVite-v2-memgaze-trace-b16384-p5000000-anlys/miniVite-v2-2-A0002000-3-B0000000-4-B1000000-SD-SP-SI-df.csv'
        strAppVar='v2'
        strFileName=strPath+strFileName
        read_file_df(strFileName, optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='miniVite/bignuke_run/mini-memgaze-ld/miniVite-v3-memgaze-trace-b16384-p5000000-anlys/miniVite-v3-4-A0002000-5-B0000000-6-B1100000-SD-SP-SI-df.csv'
        strAppVar='v3'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'miniVite/bignuke_run/mini-memgaze-ld/'

    if(strApp == 'HiParTI-HiCOO tensor reordering'):
        strFileName='HICOO-tensor/mttsel-re-0-b16384-p4000000-U-0/hot_lines/HiParTI-HiCOO-0-B0000000-SD-SP-SI-df.csv'
        strAppVar='Default'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='HICOO-tensor/mttsel-re-3-b16384-p4000000-U-0/hot_lines/HiParTI-HiCOO-Random-0-B0000000-SD-SP-SI-df.csv'
        strAppVar='Random'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='HICOO-tensor/mttsel-re-2-b16384-p4000000-U-0/hot_lines/HiParTI-HiCOO-BFS-0-B0000000-SD-SP-SI-df.csv'
        strAppVar='BFS'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='HICOO-tensor/mttsel-re-1-b16384-p4000000-U-0/hot_lines/HiParTI-HiCOO-Lexi-0-B0000000-SD-SP-SI-df.csv'
        strAppVar='Lexi'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName=strPath+'HICOO-tensor/'

    if(strApp == 'HiParTI-matrix'):
        strFileName='HICOO-matrix/4096-same-iter/hot_lines/csr/HiParTi-CSR-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='CSR'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)

        strFileName='HICOO-matrix/4096-same-iter/hot_lines/coo_u_0/HiParTi-COO-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='COO'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)

        strFileName='HICOO-matrix/4096-same-iter/hot_lines/coo_u_1/HiParTi-COO-Reduce-4-A3000000-SD-SP-SI-df.csv'
        strAppVar='COO-Reduce'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)

        strFileName='HICOO-matrix/4096-same-iter/hot_lines/hicoo_u_0/HiParTi-HiCOO-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='HiCOO'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)

        strFileName='HICOO-matrix/4096-same-iter/hot_lines/hicoo_u_1/HiParTi-HiCOO-Schedule-2-B0010000-SD-SP-SI-df.csv'
        strAppVar='HiCOO-Schedule'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)

        strFileName=strPath+'HICOO-matrix/4096-same-iter/hot_lines/'

    if(strApp.lower()=='xsb-noinline'):
        strFileName='XSBench/openmp-threading-noinline/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1/XSB-rd-EVENT_OPT_k1-0-A0000000-1-B0000000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-threading-noinline/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0/XSB-rd-EVENT_k0-0-A0000000-1-B0000000-2-B0010000-3-B0020000-4-B0030000-5-B0040000-6-B0050000-7-B0060000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-threading-noinline/memgaze-xs-read/'

    if(strApp.lower()=='xsb-noflto-other-grid'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0-anlys/XSB-rd-EVENT_k0-4-HotIns-11-5-HotIns-12-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1-anlys/XSB-rd-EVENT_OPT_k1-14-B2000000-15-B2000001-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read/'
        strApp='XSB grid-XS-data'

    if(strApp.lower()=='xsb-noflto-mat-conc'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0-anlys/XSB-rd-EVENT_k0-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1-anlys/XSB-rd-EVENT_OPT_k1-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read/'
        strApp='XSBench'

    if(strApp.lower()=='xsb-noflto-grid-index'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0-anlys/XSB-rd-EVENT_k0-2-B0000001-3-B0000002-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1-anlys/XSB-rd-EVENT_OPT_k1-12-B0001000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read/'
        strApp='XSB grid-indices'

    if(strApp.lower()=='xsb-noflto-all'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0-anlys/XSB-rd-EVENT_k0-ALL-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1-anlys/XSB-rd-EVENT_OPT_k1-ALL-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read/'
        strApp='XSB all'

    if(strApp.lower()=='xsb-noflto-irr-grid-index'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read-irregular/XSBench-memgaze-trace-b32768-p5000000-event-k-0/XSB-rd-EVENT_k0-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-noflto/memgaze-xs-read-irregular/XSBench-memgaze-trace-b32768-p5000000-event-k-1/XSB-rd-EVENT_OPT_k1-7-A1000000-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read-irregular/'
        strApp='XSB grid-indices'

    if(strApp.lower()=='xsb-noflto-irr-other-grid'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read-irregular/XSBench-memgaze-trace-b32768-p5000000-event-k-0/XSB-rd-EVENT_k0-3-HotIns-11-4-HotIns-12-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName='XSBench/openmp-noflto/memgaze-xs-read-irregular/XSBench-memgaze-trace-b32768-p5000000-event-k-1/XSB-rd-EVENT_OPT_k1-10-A3000002-11-A3000003-12-A3000004-13-A3000010-14-A3000011-8-A3000000-9-A3000001-SD-SP-SI-df.csv'
        strAppVar='XSBench-event-k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read-irregular/'
        strApp='XSB grid-XS-data'

    if(strApp.lower()=='alpaca-row'):
        strFileName='alpaca/mg-alpaca-noinline/chat-trace-b32768-p6000000-questions_copy/Alpaca-5-HotIns-11-6-HotIns-12-SD-SP-SI-df.csv'
        strAppVar='alpaca-row'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

    if(strApp.lower()=='alpaca-col'):
        strFileName='alpaca/mg-alpaca-noinline/chat-trace-b32768-p6000000-questions_copy/Alpaca-0-A0000000-1-A0000010-2-A0000011-3-A0000012-4-A0000013-SD-SP-SI-df.csv'
        strAppVar='alpaca-col'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'alpaca/mg-alpaca-noinline/chat-trace-b32768-p6000000-questions_copy/'
        strApp='Alpaca'

    if(strApp.lower()=='darknet'):
        strFileName='Darknet/alexnet_single/hot_lines/AlexNet-4-B0000000-SD-SP-SI-df.csv'
        strAppVar='alexnet-4'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName='Darknet/alexnet_single/hot_lines/AlexNet-5-B1000000-6-B1001000-7-B1010000-8-B1011000-SD-SP-SI-df.csv'
        strAppVar='alexnet-5-8'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName='Darknet/resnet152_single/hot_lines/ResNet-10-B0000000-SD-SP-SI-df.csv'
        strAppVar='resnet-10'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)

        strFileName=strPath+'Darknet/'
        strApp='Darknet'

    #print(dfForPlot)
    #print(dfForPlot.head(5))
    #print(dfForPlot.tail(5))
    if(strApp.lower()=='xsb-large-mat'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read-large/XSBench-memgaze-trace-b16384-p5000000-event-k-1/XSB-rd-EVENT_OPT_k1-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='k1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName='XSBench/openmp-noflto/memgaze-xs-read-large/XSBench-memgaze-trace-b16384-p5000000-event-k-0/XSB-rd-EVENT_k0-0-A0000000-SD-SP-SI-df.csv'
        strAppVar='k0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read-large/'
        strApp='XSBench'

    if(strApp.lower()=='xsb-large-grid'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read-large/XSBench-memgaze-trace-b16384-p5000000-event-k-1/XSB-rd-EVENT_OPT_k1-3-B0001000-4-B1000000-SD-SP-SI-df.csv'
        strAppVar='XSB-k-1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName='XSBench/openmp-noflto/memgaze-xs-read-large/XSBench-memgaze-trace-b16384-p5000000-event-k-0/XSB-rd-EVENT_k0-2-B0010000-3-B1000000-SD-SP-SI-df.csv'
        strAppVar='XSB-k-0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read-large/'
        strApp='XSBench-g'

    if(strApp.lower()=='xsb-large-all'):
        strFileName='XSBench/openmp-noflto/memgaze-xs-read-large/XSBench-memgaze-trace-b16384-p5000000-event-k-1/XSB-rd-EVENT_OPT_k1-ALL-SD-SP-SI-df.csv'
        strAppVar='XSB-k-1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName='XSBench/openmp-noflto/memgaze-xs-read-large/XSBench-memgaze-trace-b16384-p5000000-event-k-0/XSB-rd-EVENT_k0-ALL-SD-SP-SI-df.csv'
        strAppVar='XSB-k-0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-noflto/memgaze-xs-read-large/'
        strApp='XSBench'

    if(strApp.lower()=='xsb-noinl-large-all'):
        strFileName='XSBench/openmp-threading-noinline/memgaze-xs-large/memgaze-xs-large/XSBench-memgaze-trace-b8192-p5000000-event-k-1/XSB-rd-EVENT_OPT_k1-ALL-SD-SP-SI-df.csv'
        strAppVar='XSB-k-1'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName='XSBench/openmp-threading-noinline/memgaze-xs-large/memgaze-xs-large/XSBench-memgaze-trace-b8192-p5000000-event-k-0/XSB-rd-EVENT_k0-ALL-SD-SP-SI-df.csv'
        strAppVar='XSB-k-0'
        strFileName=strPath+strFileName
        read_file_df(strFileName,  optionHotRef,optionHotAff , strAppVar,dfForPlot, dictVarAccess,listUnRealizedpairs)
        print(dfForPlot.shape)
        strFileName=strPath+'XSBench/openmp-threading-noinline/memgaze-xs-large/memgaze-xs-large/'
        strApp='XSBench'

    if(0):
        arrVariants=list(set(dfForPlot["Variant"].to_list()))
        for i in range(len(arrVariants)):
            print(arrVariants[i])
            print((dfForPlot[dfForPlot["Variant"] ==arrVariants[i] ][['Access','HotLineWeight']]))

    if (strApp != 'HiParTI-HiCOO tensor reordering'):
        dfForPlot.sort_values(by='Variant', ascending=True, inplace=True)
    draw_plot(strApp,strFileName, optionHotRef,optionHotAff,dfForPlot,dictVarAccess,flPlot)

    if(len(listUnRealizedpairs) !=0):
        dfUnRealizedPairs=pd.DataFrame(listUnRealizedpairs)
        dfUnRealizedPairs.columns=['Variant', 'reg-page-blk','aff-block', 'Access','HotLineWeight', 'Value', 'Type']
        dfUnRealizedPairs.sort_values(['Variant','Type', 'Value'], ascending=[True, True, False],inplace=True)
        arrVariants=list(set(dfUnRealizedPairs["Variant"].to_list()))
        for i in range(len(arrVariants)):
            print(arrVariants[i], '--- SA')
            print((dfUnRealizedPairs[(dfUnRealizedPairs.Variant ==arrVariants[i]) & (dfUnRealizedPairs.Type == 'SA') & (dfUnRealizedPairs.Value >= 0.05)]))
            print(arrVariants[i], '--- SD')
            print((dfUnRealizedPairs[(dfUnRealizedPairs.Variant ==arrVariants[i]) & (dfUnRealizedPairs.Type == 'SD') & (dfUnRealizedPairs.Value >= 0.0)]))

        #print(dfUnRealizedPairs[dfUnRealizedPairs["Type"]=='SA'])
        #print(dfUnRealizedPairs[dfUnRealizedPairs["Type"]=='SD'])
    else:
        print('Unrealized list - size 0')


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
# Three ways to choose hot ref blocks
    # intHotRef = 0 - Option 0 - 90% in range Hot-Access - DEFAULT
    # intHotRef = 1 - Option 1 - 90% of total access
    # intHotRef = 2 - Option 2 - ALL blocks - all reference blocks for whole application
# Two ways to choose hot affinity blocks - SA doesnt include "self"
    # intHotAff = 0 - Option 0 - only hot lines and self
    # intHotAff = 1 - Option 1 - only hot lines and self, self-1, self+1, self+2 - DEFAULT
    # intHotAff = 2 - Option 2 - all heatmap rows
    # intHotAff = 3 - Option 3 - only hot lines and (self+1, self+2) for SA, (self-1, self+1, self) for SD

# Scores in table

flPlot=True
flScore=True


if(flScore):
    plot_app('miniVite',0,3,flPlot) # used for score table data - all hm - more blocks
    plot_app('miniVite',0,2,flPlot) # used for score table data - all hm - more blocks
    plot_app('HiParTI-HiCOO tensor reordering',0,3, flPlot) # used for score table data - all hm - more blocks
    plot_app('HiParTI-HiCOO tensor reordering',0,2, flPlot) # used for score table data - all hm - more blocks
    plot_app('xsb-large-mat',0,3,flPlot) # used for score table data - all hm - more blocks
    plot_app('xsb-large-mat',0,2,flPlot) # used for score table data - all hm - more blocks
    flPlot=False
    plot_app('alpaca-row',0,3,flPlot) # used for score table data - all hm - more blocks
    plot_app('alpaca-row',0,2,flPlot) # used for score table data - all hm - more blocks
    plot_app('alpaca-col',0,3,flPlot) # used for score table data - all hm - more blocks
    plot_app('alpaca-col',0,2,flPlot) # used for score table data - all hm - more blocks
  #plot_app('darknet',0,3,flPlot) # used for score table data - all hm - more blocks
    #plot_app('darknet',0,2,flPlot) # used for score table data - all hm - more blocks
    
flScore=False
flPlot = False
if(flScore):
    plot_app('xsb-large-mat',0,3,flPlot) # used for score table data - all hm - more blocks
    plot_app('xsb-large-mat',0,2,flPlot) # used for score table data - all hm - more blocks
    plot_app('xsb-large-grid',0,3,flPlot) # used for score table data - all hm - more blocks
    plot_app('xsb-large-grid',0,2,flPlot) # used for score table data - all hm - more blocks
    plot_app('xsb-large-all',0,3,flPlot) # used for score table data - all hm - more blocks
    plot_app('xsb-large-all',0,2,flPlot) # used for score table data - all hm - more blocks
  #  plot_app('xsb-noinl-large-all',0,3,flPlot) # used for score table data - all hm - more blocks
  #  plot_app('xsb-noinl-large-all',0,2,flPlot) # used for score table data - all hm - more blocks
  #  plot_app('xsb-noinl-large-all',2,3,flPlot) # used for score table data - all hm - more blocks
  #  plot_app('xsb-noinl-large-all',2,2,flPlot) # used for score table data - all hm - more blocks
 #   plot_app('xsb-noflto-other-grid', 0, 3,flPlot) # used for score table data - all hm - more blocks
 #   plot_app('xsb-noflto-other-grid', 0, 2,flPlot) # used for score table data - all hm - more blocks
 #   plot_app('xsb-noflto-grid-index', 0, 3,flPlot) # used for score table data - all hm - more blocks
 #   plot_app('xsb-noflto-grid-index', 0, 2,flPlot) # used for score table data - all hm - more blocks
 #   plot_app('xsb-noflto-mat-conc',0,3,flPlot) # used for score table data - all hm - more blocks
 #   plot_app('xsb-noflto-mat-conc',0,2,flPlot) # used for score table data - all hm - more blocks

#plot_app('xsb-noflto-all',2,2,True)
#plot_app('HiParTI-HiCOO tensor reordering',0,3, flPlot) # used for score table data - all hm - more blocks
#plot_app('HiParTI-HiCOO tensor reordering',0,2, flPlot) # used for score table data - all hm - more blocks
#plot_app('miniVite',0,2,flPlot)

#Plots in paper
flPlot=True
if (0):
    #plot_app('xsb-noflto-mat-conc',0,2,flPlot)
    #plot_app('xsb-noflto-mat-conc',2,2,flPlot)
    #plot_app('miniVite',0,2,flPlot)
    plot_app('HiParTI-HiCOO tensor reordering',0,2, flPlot)

#plot_app('xsb-noflto-other-grid', 0,3,flPlot) # No useful data - irregular accesses
#plot_app('xsb-noflto-other-grid', 0,2,flPlot) # No useful data - irregular accesses
#plot_app('xsb-noflto-grid-index', 0,2,flPlot) # No significant data for intHotAff = 3
#plot_app('xsb-noflto-irr-other-grid',0,2,flPlot) # No useful data - irregular accesses
#plot_app('xsb-noflto-irr-grid-index',0,2,flPlot) # No useful data - irregular accesses

# Trial for targetted blocks in score
if(0):
    plot_app('miniVite',2,3,flPlot)
    plot_app('HiParTI-HiCOO tensor reordering',2,3, flPlot)
    plot_app('xsb-noflto-other-grid', 2, 3,flPlot)
    plot_app('xsb-noflto-grid-index', 2, 3,flPlot)
    plot_app('xsb-noflto-mat-energy',2,3,flPlot)
