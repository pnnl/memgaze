# Create intra-region SD, SP and SI plots for all blocks
# Combination of plots is SP-SI or SP-SI-SD
# Contiguous locations of interest in line 193
# plot_SP_col=['self-1','self','self+1','self+2','self+3','self+4', 'self+5']
    # STEP 1 - Read spatial.txt and write inter-region file
    # STEP 2 - Get region ID's from inter-region file
    # STEP 2a - Add combine regions to the list
    # STEP 3 - Loop through regions in the list
        # STEP 3a - Combine regions if there are any - incremental heatmaps are written out as of March 2, 2023
        # STEP 3b - Read original spatial data input file to gather the pages-blocks in the region to a list
        # STEP 3b - Convert list to data frame
        # STEP 3c-0 - Process data frame to get access, lifetime totals
        # STEP 3c-1 - Dataframe has all three metrics, so divide by 3 for access count reporting
        # STEP 3d - drop columns with "all" NaN values
import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import re
import csv
import os
import copy
from fileToDataframe import get_intra_obj,getFileColumnNames,getMetricColumns,getRearrangeColumns
from fileToDataframe import getFileColumnNamesPageRegion,getMetricColumnsPageRegion,getPageColListPageRegion
from fileToDataframe import getFileColumnNamesLineRegion,getMetricColumnsLineRegion,getPageColListLineRegion

sns.color_palette("light:#5A9", as_cmap=True)
sns.set()

# Works for spatial denity, Spatial Probability and Proximity
def intraObjectPlot(strApp, strFileName,numRegion, strMetric=None, f_avg=None,listCombineReg=None,flWeight=None,numExtraPages:int=0,affinityOption:int=None,flAccessWeight:bool=False):
    flagPhysicalPages = 0
    flagHotPages =0
    flagHotLines = 0
    if(affinityOption==1):
        flagPhysicalPages = 1
    elif (affinityOption == 2):
        flagHotPages = 1
    elif (affinityOption == 3):
        flagHotLines = 1
    # STEP 1 - Read spatial.txt and write inter-region file
    strPath=strFileName[0:strFileName.rindex('/')]
    if('SP-SI' in strMetric):
        fileName='/inter_object_sp-si.txt'
        lineStart='---'
        lineEnd='<----'
        # For Minivite use
        #strMetricIdentifier='Spatial_Proximity'
        strMetricIdentifier='Spatial_Interval'
        strMetricTitle='Spatial Proximity and Interval'
    if(strMetric == 'SP-SI-SD'):
        strMetricTitle='Spatial Density, Proximity and Interval'
    #print(strPath)
    # Write inter-object_sd.txt
    strInterRegionFile=strPath+fileName
    f_out=open(strInterRegionFile,'w')
    with open(strFileName) as f:
        for fileLine in f:
            data=fileLine.strip().split(' ')
            if (data[0] == lineStart):
                #print (fileLine)
                f_out.write(fileLine)
            if (data[0] == lineEnd):
                f_out.close()
                break
    f.close()
    plotFilename=strInterRegionFile
    appName=strApp
    colSelect=0
    sampleSize=0
    print(plotFilename,colSelect,appName,sampleSize)
    if(f_avg != None):
        f_avg.write('filename ' + plotFilename + ' col select '+ str(colSelect)+ ' app '+appName+ ' sample '+str(sampleSize)+'\n')
    #spatialPlot(plotFilename, colSelect, appName,sampleSize)

    # STEP 2 - Get region ID's from inter-region file
    df_inter=pd.read_table(strInterRegionFile, sep=" ", skipinitialspace=True, usecols=range(2,15),
                     names=['RegionId_Name','Page', 'RegionId_Num','colon', 'ar', 'Address Range', 'lf', 'Lifetime', 'ac', 'Access count', 'bc', 'Block count','Type'])
    df_inter = df_inter[df_inter['Type'] == strMetricIdentifier]
    #print(df_inter)
    df_inter_data=df_inter[['RegionId_Name', 'RegionId_Num', 'Address Range', 'Lifetime', 'Access count', 'Block count']]
    df_inter_data['Reg_Num-Name']=df_inter_data.apply(lambda x:'%s-%s' % (x['RegionId_Num'],x['RegionId_Name']),axis=1)
    numRegionInFile = df_inter_data['RegionId_Num'].count()

    #print(df_inter_data)
    df_inter_data_sample=df_inter_data.nlargest(n=numRegion,  columns=['Access count'])
    arRegionId = df_inter_data_sample['Reg_Num-Name'].values.flatten().tolist()

    # STEP 2a - Add combine regions to the list
    if (listCombineReg != None):
        arRegionId.extend(x for x in listCombineReg if x not in arRegionId)
    arRegionId.sort()
    #print(arRegionId)

    data_list_combine_Reg =[]
    flagProcessCombine=0
    combineCount=0

    # STEP 3 - Loop through regions in the list and plot heatmaps
    for j in range(0, len(arRegionId)):
        regionIdNumName=arRegionId[j]
        regionIdNumName_copy=arRegionId[j]
        regionIdName =regionIdNumName[regionIdNumName.index('-')+1:]
        regionIdNum= regionIdNumName[:regionIdNumName.index('-')]
        print('regionIdName ', regionIdName, 'regionIdNumName ', regionIdNumName, regionIdNum)
        data_list_intra_obj=[]

        # STEP 3a - Combine regions if there are any - incremental heatmaps are written out as of March 2, 2023
        if(listCombineReg != None and regionIdNumName in listCombineReg):
            print('Not None', regionIdNumName)
            if(flagProcessCombine == 0):
                flagProcessCombine=1
                combRegionIdNumName=arRegionId[j]
                arCombRegionAccess = df_inter_data[ df_inter_data['Reg_Num-Name']==regionIdNumName]['Access count'].values.flatten()[0]
                numCombRegionBlocks = df_inter_data[ df_inter_data['Reg_Num-Name']==regionIdNumName]['Block count'].values.flatten()[0]
            else:
                arCombRegionAccess += df_inter_data[ df_inter_data['Reg_Num-Name']==regionIdNumName]['Access count'].values.flatten()[0]
                numCombRegionBlocks += df_inter_data[ df_inter_data['Reg_Num-Name']==regionIdNumName]['Block count'].values.flatten()[0]
                combRegionIdNumName = combRegionIdNumName+' & '+arRegionId[j]
            regionIdNumName = combRegionIdNumName
            arRegionAccess = arCombRegionAccess
            numRegionBlocks = numCombRegionBlocks
        else:
            arRegionAccess = df_inter_data[ df_inter_data['Reg_Num-Name']==regionIdNumName]['Access count'].values.flatten()[0]
            numRegionBlocks = df_inter_data[ df_inter_data['Reg_Num-Name']==regionIdNumName]['Block count'].values.flatten()[0]

        # STEP 3b - Read original spatial data input file to gather the pages-blocks in the region to a list
        # Read both SP and SI to dataframe
        lineStartList=['---','===','***']
        if (flagPhysicalPages== 1):
            numExtra = numExtraPages
        elif(flagHotPages ==1):
            numExtra = 10+numRegionInFile+2
        elif(flagHotLines ==1 ):
            numExtra = 10+numRegionInFile+2
        with open(strFileName) as f:
            for fileLine in f:
                data=fileLine.strip().split(' ')
                if ((data[0] in lineStartList) and (data[2][0:len(data[2])-1]) == regionIdName):
                    pageId=data[2][-1]
                    #print('region line' , regionIdNumName, pageId, data[2])
                    get_intra_obj(data_list_intra_obj,fileLine,regionIdNum+'-'+pageId,regionIdNum,numExtra)
        f.close()
        print('**** before regionIdNumName ', regionIdNumName, 'list length', len(data_list_intra_obj))
        if(listCombineReg != None and regionIdNumName_copy in listCombineReg):
            combineCount= combineCount+1
            data_list_combine_Reg.extend(data_list_intra_obj)
            data_list_intra_obj=copy.deepcopy(data_list_combine_Reg)
            print('***** after regionIdNumName ', regionIdNumName, 'list data_list_intra_obj length', len(data_list_intra_obj), 'list data_list_combine_Reg', len(data_list_combine_Reg))
        if((listCombineReg != None) and (regionIdNumName_copy in listCombineReg) and (combineCount < len(listCombineReg))):
            print(" should break out of loop")
            continue
        else:
            print("Proceed to plot")

        # STEP 3b - Convert list to data frame
        #print(data_list_intra_obj)
        # STEP 3b - Convert list to data frame
        #print(data_list_intra_obj)
        if (flagPhysicalPages== 1):
            list_col_names=getFileColumnNames(numExtraPages)
        elif(flagHotPages == 1):
            list_col_names =getFileColumnNamesPageRegion (regionIdNum, numRegionInFile)
        elif(flagHotLines == 1):
            list_col_names =getFileColumnNamesLineRegion (strFileName, numRegionInFile)
        df_intra_obj=pd.DataFrame(data_list_intra_obj,columns=list_col_names)
        print(df_intra_obj.shape)
        #print(df_intra_obj['Type'])
        #print(df_intra_obj[['Address', 'reg-page-blk','self-2','self-1','self','self+1','self+2']])

        # STEP 3c-0 - Process data frame to get access, lifetime totals
        # This is only for the blocks in ten pages analysis (not whole region)
        df_intra_obj = df_intra_obj.astype({"Access": int, "Lifetime": int})
        # STEP 3c-1 - Dataframe has all three metrics, so divide by 3 for access count reporting
        accessSumBlocks= df_intra_obj['Access'].sum()/3
        arRegPages=df_intra_obj['reg_page_id'].unique()
        #print (arRegPages)
        arRegPageAccess = np.empty([len(arRegPages),1])
        for arRegionBlockId in range(0, len(arRegPages)):
            #print(arRegionBlockId, df_intra_obj[df_intra_obj['reg-page']==arRegPages[arRegionBlockId]]['Access'].sum())
            arRegPageAccess[int(arRegionBlockId)] = df_intra_obj[df_intra_obj['reg_page_id']==arRegPages[arRegionBlockId]]['Access'].sum()/3
        #print (arRegPageAccess)
        listCombine = zip(arRegPages, arRegPageAccess.tolist())
        df_reg_pages = pd.DataFrame(listCombine, columns=('reg-page', 'Access'))
        df_reg_pages.set_index('reg-page')
        df_reg_pages.sort_index(inplace=True)
        df_reg_pages["Access"] = df_reg_pages["Access"].apply(pd.to_numeric)
        df_reg_pages = df_reg_pages.astype({"Access": int})
        arRegPageAccess=(df_reg_pages[['Access']]).to_numpy()
        arRegPages=df_reg_pages['reg-page'].to_list()

        #plot_SP_col=['self-3','self-2','self-1','self','self+1','self+2','self+3']
        plot_SP_col=['self-1', 'self','self+1','self+2'] #,'self+3','self+4']
        plot_SP_col.reverse()
        for colname in plot_SP_col:
            if( 1==0):
                average_sd= pd.to_numeric(df_intra_obj[(df_intra_obj['Type'] == 'SD')][colname]).mean()
                print('*** Before sampling Average SD for '+strApp+', Region '+regionIdNumName+' for '+colname+ ' '+str(average_sd))
                average_sp= pd.to_numeric(df_intra_obj[(df_intra_obj['Type'] == 'SP')][colname]).mean()
                print('*** Before sampling Average SP for '+strApp+', Region '+regionIdNumName+' for '+colname+ ' '+str(average_sp))
                average_si= pd.to_numeric(df_intra_obj[(df_intra_obj['Type'] == 'SI')][colname]).mean()
                print('*** Before sampling Average SI for '+strApp+', Region '+regionIdNumName+' for '+colname+ ' '+str(average_si))

        get_col_list = getMetricColumns(numExtraPages)
        # Change data to numeric
        for i in range (0,len(get_col_list)):
            df_intra_obj[get_col_list[i]]=pd.to_numeric(df_intra_obj[get_col_list[i]])

        df_intra_obj.set_index('reg-page-blk')
        df_intra_obj.sort_index(inplace=True)

        self_bef_drop=df_intra_obj['self'].to_list()
        # STEP 3d - drop columns with "all" NaN values
        # DROP - columns with no values
        print('before replace drop - ', df_intra_obj.shape)
        df_intra_obj_drop=df_intra_obj.dropna(axis=1,how='all')
        self_aft_drop=df_intra_obj_drop['self'].to_list()

        if(self_bef_drop == self_aft_drop):
            print(" After drop equal ")
        else:
            print("Error - not equal")
            print ("before drop", self_bef_drop)
            print ("after drop", self_aft_drop)
            break

        #print('after drop columns ', df_intra_obj_drop.columns)
        list_xlabel=df_intra_obj_drop[(df_intra_obj_drop['Type'] == 'SP')]['reg-page-blk'].to_list()
        #print(list_xlabel)
        df_intra_obj_SP=df_intra_obj_drop[(df_intra_obj_drop['Type'] == 'SP')]
        df_intra_obj_SI=df_intra_obj_drop[(df_intra_obj_drop['Type'] == 'SI')]
        df_intra_obj_SD=df_intra_obj_drop[(df_intra_obj_drop['Type'] == 'SD')]
        #print('SP cols ', df_intra_obj_SP.columns)
        #print('SI cols ', df_intra_obj_SI.columns)
        print('after filter Type SP - ', df_intra_obj_SP.shape)
        print('after filter Type drop SI - ', df_intra_obj_SI.shape)
        #print('blk ',list_xlabel[0], ' access ', df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == list_xlabel[0])]['Access'], ' accessSumBlocks ', accessSumBlocks, ' maxAccess ', maxAccess)

        flNormalize=False
        normSDMax=np.ones(len(arRegPages))
        normSPMax = np.ones(len(arRegPages))
        list_SP_SI_SD=[[None]*(3*len(plot_SP_col)+3) for i in range(len(list_xlabel))]
        #[[0]*5 for i in range(5)]
        for blkCnt in range(0,len(list_xlabel)):
            blkid= list_xlabel[blkCnt]
            list_SP_SI_SD[blkCnt][0]=blkid
            list_SP_SI_SD[blkCnt][1]=blkid[0:blkid.rfind('-')]
            list_SP_SI_SD[blkCnt][2]=df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item()
            if (flWeight == True):
                weightRegPageAccess = df_reg_pages[(df_reg_pages['reg-page'] == blkid[0:blkid.rfind('-')])]['Access'].values[0]
                #print(df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item(), weightRegPageAccess)
                weight_multiplier =  df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item() *(100 / weightRegPageAccess)
            else:
                weight_multiplier = 1.0
            for plotCnt in range(0,len(plot_SP_col)):
                plotCol= plot_SP_col[plotCnt]
                if(plotCol in df_intra_obj_SP.columns):
                    condSP = (df_intra_obj_SP['reg-page-blk'] == blkid) & (df_intra_obj_SP[plotCol] >= 0.0)
                    resultSP = df_intra_obj_SP[condSP][plotCol]
                    condSI = (df_intra_obj_SI['reg-page-blk'] == blkid)
                    resultSI = df_intra_obj_SI[condSI][plotCol]
                    condSD = (df_intra_obj_SD['reg-page-blk'] == blkid)
                    resultSD = df_intra_obj_SD[condSD][plotCol]
                    if resultSP.size > 0:
                        #print(blkid, plotCol, resultSP.values[0], resultSI.values[0])
                        list_SP_SI_SD[blkCnt][plotCnt*3+3]=round(resultSP.values[0] * weight_multiplier,2)
                    if resultSI.size >0:
                        list_SP_SI_SD[blkCnt][plotCnt*3+4]=resultSI.values[0]
                    if resultSD.size >0:
                        list_SP_SI_SD[blkCnt][plotCnt*3+5]=round(resultSD.values[0] * weight_multiplier,2)
                    arRegPageIndex = arRegPages.index(blkid[0:blkid.rfind('-')])
                    if (isinstance(list_SP_SI_SD[blkCnt][plotCnt*3+3], float) and (list_SP_SI_SD[blkCnt][plotCnt*3+3] > 1.0)  ):
                        if( normSPMax[arRegPageIndex] < list_SP_SI_SD[blkCnt][plotCnt*3+3]):
                            normSPMax[arRegPageIndex] = list_SP_SI_SD[blkCnt][plotCnt*3+3]
                            flNormalize = True
                    if (isinstance(list_SP_SI_SD[blkCnt][plotCnt*3+5], float) and (list_SP_SI_SD[blkCnt][plotCnt*3+5]> 1.0)):
                        if (normSDMax[arRegPageIndex] < list_SP_SI_SD[blkCnt][plotCnt*3+5]):
                            normSDMax[arRegPageIndex] = list_SP_SI_SD[blkCnt][plotCnt*3+5]
                            flNormalize = True
        #print(list_SP_SI)
        list_col_names=['reg-page-blk', 'reg-page','Access']
        for plotCol in plot_SP_col:
            list_col_names.append('SP-'+plotCol)
            list_col_names.append('SI-'+plotCol)
            list_col_names.append('SD-'+plotCol)
        df_SP_SI_SD=pd.DataFrame(list_SP_SI_SD,columns=list_col_names)
        #print(df_SP_SI_SD)

        cols_df_SP_SI_SD = df_SP_SI_SD.columns.to_list()
        if (flNormalize == True):
            print ('Normalize true SD max ', normSDMax, ' SP ', normSPMax)
            print('before normalize  ' , df_SP_SI_SD[['reg-page-blk', 'SD-self', 'SP-self']])
            regPageIndex=0
            for regPageId in arRegPages:
                #print( regPageIndex, regPageId, normSDMax[regPageIndex], normSPMax[regPageIndex])
                #print(df_SP_SI_SD['reg-page'] == regPageId)
                pattern = re.compile('SD-')
                cols_df_SD = list(filter(pattern.match, cols_df_SP_SI_SD))
                for col_SD_name in cols_df_SD:
                    df_SP_SI_SD.loc[df_SP_SI_SD['reg-page'] == regPageId,[col_SD_name]] = df_SP_SI_SD.loc[df_SP_SI_SD['reg-page'] == regPageId, [col_SD_name]].div(normSDMax[regPageIndex])
                pattern = re.compile('SP-')
                cols_df_SP = list(filter(pattern.match, cols_df_SP_SI_SD))
                for col_SP_name in cols_df_SP:
                    df_SP_SI_SD.loc[df_SP_SI_SD['reg-page'] == regPageId, [col_SP_name]] = df_SP_SI_SD.loc[df_SP_SI_SD['reg-page'] == regPageId, [col_SP_name]].div(normSPMax[regPageIndex])
                regPageIndex = regPageIndex+1
            print('after normalize  ' , df_SP_SI_SD[['reg-page-blk', 'SD-self', 'SP-self']])

        # Composite SD and SA - include SI discount
        SI_good=63
        for index, row in df_SP_SI_SD.iterrows():
            for i in range(0,len(plot_SP_col)):
                blkSDValue=np.NaN
                blkSIValue=np.NaN
                blkSAValue=np.NaN
                regPageBlock= row['reg-page-blk']
                if((row['SI-'+plot_SP_col[i]])) and (~np.isnan(row['SI-'+plot_SP_col[i]])):
                    blkSIValue = row['SI-'+plot_SP_col[i]]
                if((row['SD-'+plot_SP_col[i]])) and (~np.isnan(row['SD-'+plot_SP_col[i]])):
                    blkSDValue = row['SD-'+plot_SP_col[i]]
                if((row['SP-'+plot_SP_col[i]])) and (~np.isnan(row['SP-'+plot_SP_col[i]])):
                    blkSAValue = row['SP-'+plot_SP_col[i]]
                if (not(np.isnan(blkSIValue))):
                    valSIBin = int(blkSIValue /SI_good) +1
                    if(valSIBin > 10):
                        valSIBin = 10
                    valSIDiscount = float((10-valSIBin+1)/10)
                    if(blkSIValue >=63):
                    #print(row[['reg-page-blk', 'SI-'+plot_SP_col[i], 'SP-'+plot_SP_col[i], 'SD-'+plot_SP_col[i]]])
                        print(df_SP_SI_SD.loc[index,'reg-page-blk'],"SI ", df_SP_SI_SD.loc[index,'SI-'+plot_SP_col[i]], " SD ", \
                              df_SP_SI_SD.loc[index,'SD-'+plot_SP_col[i]]," SA ",df_SP_SI_SD.loc[index,'SP-'+plot_SP_col[i]])
                    df_SP_SI_SD.loc[df_SP_SI_SD['reg-page-blk'] == regPageBlock, 'SD-'+plot_SP_col[i]] = round((blkSDValue * valSIDiscount),3)
                    df_SP_SI_SD.loc[df_SP_SI_SD['reg-page-blk'] == regPageBlock, 'SP-'+plot_SP_col[i]] = round((blkSAValue * valSIDiscount),3)
                    #df_local.loc[df_local['reg-page-blk'] == regPageBlock, ['SD-'+listAffinityLines[i]]]
                    #print(regPageBlock,plot_SP_col)
                    if(blkSIValue >=63):
                        #print(row[['reg-page-blk', 'SI-'+plot_SP_col[i], 'SP-'+plot_SP_col[i], 'SD-'+plot_SP_col[i]]])
                        print(" Change ", df_SP_SI_SD.loc[index,'reg-page-blk'], "SD " ,df_SP_SI_SD.loc[index,'SD-'+plot_SP_col[i]], " SA ",df_SP_SI_SD.loc[index,'SP-'+plot_SP_col[i]])
                        #print(df_SP_SI_SD.loc[index,'SI-'+plot_SP_col[i]])
                    #df_SP_SI_SD.loc[df_SP_SI_SD['reg-page-blk'] == regPageBlock, ['SP-'+plot_SP_col[i]]] = round((blkSAValue * valSIDiscount),3)


        # Add weighting based on Access - hotness
        if (flAccessWeight == True):
            hotRefMaxAccess=df_SP_SI_SD['Access'].max()
            hotRefMinAccess=df_SP_SI_SD['Access'].min()
            refRange=hotRefMaxAccess-hotRefMinAccess
            #print((df_SP_SI_SD['Access']-hotRefMinAccess)/refRange)
            df_SP_SI_SD['Hot-Access']=(df_SP_SI_SD['Access']-hotRefMinAccess)/refRange
            for index, row in df_SP_SI_SD.iterrows():
                for i in range(0,len(plot_SP_col)):
                    regPageBlock= row['reg-page-blk']
                    if((row['SD-'+plot_SP_col[i]])) and (~np.isnan(row['SD-'+plot_SP_col[i]])):
                        blkSDValue = row['SD-'+plot_SP_col[i]]
                    if((row['SP-'+plot_SP_col[i]])) and (~np.isnan(row['SP-'+plot_SP_col[i]])):
                        blkSAValue = row['SP-'+plot_SP_col[i]]
                    df_SP_SI_SD.loc[df_SP_SI_SD['reg-page-blk'] == regPageBlock, 'SD-'+plot_SP_col[i]] = round((blkSDValue * row['Hot-Access']),3)
                    df_SP_SI_SD.loc[df_SP_SI_SD['reg-page-blk'] == regPageBlock, 'SP-'+plot_SP_col[i]] = round((blkSAValue * row['Hot-Access']),3)


        fig, ax_plots = plt.subplots(nrows=len(plot_SP_col)+1, ncols=1,constrained_layout=True, figsize=(15, 10))
        SP_color='tab:orange'
        SD_color='green'
        SI_color='tab:blue'
        for i, ax in enumerate(ax_plots.reshape(-1)[:len(plot_SP_col)]):
            #ax.set_title('Spatial affinity for '+ plot_SP_col[i])
            ax.set_xticks([])
            ax.set_facecolor('white')
            ax.grid(color='grey')
            ax.set_yticks([0, 0.25,0.5, 0.75, 1.0])
            ax.set_yticklabels([0, 0.25,0.5, 0.75, 1.0], fontsize=12)
            #ax.set_yticklabels([0, 0.125,0.25, 0.625, 0.75], fontsize=12)
            ax.set_ylim(-0.25, 1.0)
            ax.plot(df_SP_SI_SD['reg-page-blk'], df_SP_SI_SD['SP-'+plot_SP_col[i]], color=SP_color,label='SP')
            ax.plot(df_SP_SI_SD['reg-page-blk'], df_SP_SI_SD['SD-'+plot_SP_col[i]], color=SD_color,label='SD')
            ax.tick_params(axis='y', labelcolor='black')
            label_y = -0.05
            ax.text(label_y, 0.3, "$\it{SA}^{*}$", color=SP_color, style='italic', fontsize='16' , rotation='vertical', transform=ax.transAxes)
            ax.text(label_y, 0.55, r"& ", color='black', style='italic',fontsize='16' ,rotation='vertical', transform=ax.transAxes)
            ax.text(label_y, 0.70, "$\it{SD}^{*}$", color=SD_color, style='italic',fontsize='16' ,rotation='vertical', transform=ax.transAxes)
            # To draw a horizantal threshold line
            #xmin, xmax = ax.get_xlim()
            #ax.hlines(y=0.25, xmin=xmin, xmax=xmax, linewidth=1, color='black')
            # Instantiate a second axes that shares the same x-axis
            #ax2 = ax.twinx()
            #ax2.set_ylabel('SI', fontsize='16' ,style='italic',color=SI_color)
            #ax2.set_ylim(0,200)
            #ax2.set_yticks([0,40,80,120,160,200])
            #ax2.set_yticklabels([0,40,80,120,160,200],fontsize=12)

            #ax2.plot(df_SP_SI_SD['reg-page-blk'], df_SP_SI_SD['SI-'+plot_SP_col[i]], color=SI_color,label='SI')
            #ax2.tick_params(axis='y', labelcolor=SI_color)
            ax.text(0.01, 0.80, plot_SP_col[i][4:], color='black', size='16', rotation = 'horizontal', transform=ax.transAxes)

        ax=ax_plots.reshape(-1)[len(plot_SP_col)]
        ax.plot(df_SP_SI_SD['reg-page-blk'], df_intra_obj_drop[(df_intra_obj_drop['Type'] == 'SP')]['Access'], color=SI_color,label='Access')
        ax.set_facecolor('white')
        ax.grid(color='grey')
        ax.set_yscale('log')
        ax.set_ylim(0,100000)
        #ax.set_ylabel('Access')
        ax.set_xticks([])
        #ax.set_title('Access counts for blocks')
        ax.set_xlabel('Blocks',fontsize='16')
        label_y = -0.045
        ax.text(label_y, 0.3, r"Access", color=SI_color, fontsize='16', rotation='vertical', transform=ax.transAxes)
        ax.text(0.01, 0.80, 'Access frequency', color='black', size='16', rotation = 'horizontal', transform=ax.transAxes)


        strAccessSumBlocks= str(np.round((accessSumBlocks/1000).astype(float),2))+'K'
        strArRegionAccess = str(np.round((arRegionAccess/1000).astype(float),2))+'K'

        #strTitle = strApp +' region '+regionIdNumName+' \n Region\'s access  - ' + strArRegionAccess + ', Access count for selected pages - ' \
        #           + strAccessSumBlocks +' ('+ ("{0:.1f}".format((accessSumBlocks/arRegionAccess)*100))+'%), Number of pages in region - '+ str(numRegionBlocks)
        strTitle = 'Access: Region - ' + strArRegionAccess + ', Pages - '+ strAccessSumBlocks +' ('+ ("{0:.1f}".format((accessSumBlocks/arRegionAccess)*100))+'%)'

        plt.suptitle(strTitle)
        fileNameLastSeg = '_plot-no-SI.pdf'
        if(flAccessWeight == True):
            fileNameLastSeg = '_plot-no-SI-wi.pdf'
        if (flWeight == True):
            fileNameLastSeg = '_plot_Wgt.pdf'
        imageFileName=strPath+'/'+strApp.replace(' ','')+'-'+regionIdNumName.replace(' ','').replace('&','-')+'-'+strMetric+fileNameLastSeg
        print(imageFileName)
        #plt.show()
        plt.savefig(imageFileName, bbox_inches='tight')
        plt.close()

flWeight=False
mainPath='/Users/suri836/Projects/spatial_rud/'

# Not very useful plots for Minivite, heatmap shows interesting information
#intraObjectPlot('Minivite-V1',mainPath+'minivite_detailed_look/inter-region/v1_spatial_det.txt',1,strMetric='SP-SI-SD', flWeight=flWeight)
#intraObjectPlot('Minivite-V2',mainPath+'minivite_detailed_look/inter-region/v2_spatial_det.txt',3,strMetric='SP-SI-SD', listCombineReg=['1-A0000010','4-A0002000'], flWeight=flWeight)
#intraObjectPlot('Minivite-V3',mainPath+'minivite_detailed_look/inter-region/v3_spatial_det.txt',3,strMetric='SP-SI-SD', listCombineReg=['1-A0000001','5-A0001200'] ,flWeight=flWeight)
if( 1 ==0):
    intraObjectPlot('HiParTI-HiCOO', mainPath+'HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI-SD', flWeight=flWeight)
    intraObjectPlot('HiParTI-HiCOO-BFS', mainPath+'HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI-SD', flWeight=flWeight)
    intraObjectPlot('HiParTI-HiCOO-Lexi', mainPath+'HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI-SD', flWeight=flWeight)
    intraObjectPlot('HiParTI-HiCOO-Random', mainPath+'HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI-SD', flWeight=flWeight)

if(0):
    numExtraPages=0
    intraObjectPlot('HiParTI-HiCOO-Lexi', mainPath+'spatial_pages_exp/mttsel-re-1-b16384-p4000000-U-0/spatial_pages_'+str(numExtraPages)+'/spatial.txt', 1,strMetric='SP-SI-SD',flWeight=flWeight,numExtraPages=numExtraPages)
    intraObjectPlot('HiParTI-HiCOO-BFS', mainPath+'spatial_pages_exp/mttsel-re-2-b16384-p4000000-U-0/spatial_pages_'+str(numExtraPages)+'/spatial.txt', 1,strMetric='SP-SI-SD',flWeight=flWeight,numExtraPages=numExtraPages)

if(0):
    numExtraPages=16
    intraObjectPlot('HiParTI-HiCOO-Lexi', mainPath+'spatial_pages_exp/mttsel-re-1-b16384-p4000000-U-0/spatial_pages_'+str(numExtraPages)+'/spatial.txt', 1,strMetric='SP-SI-SD',flWeight=flWeight,numExtraPages=numExtraPages)
    intraObjectPlot('HiParTI-HiCOO-BFS', mainPath+'spatial_pages_exp/mttsel-re-2-b16384-p4000000-U-0/spatial_pages_'+str(numExtraPages)+'/spatial.txt', 1,strMetric='SP-SI-SD',flWeight=flWeight,numExtraPages=numExtraPages)

if(1):
    intraObjectPlot('HiParTI-HiCOO-Lexi', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-1-b16384-p4000000-U-0/hot_lines/spatial.txt', 1,strMetric='SP-SI-SD',flWeight=flWeight,affinityOption=3)
    intraObjectPlot('HiParTI-HiCOO-BFS', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-2-b16384-p4000000-U-0/hot_lines/spatial.txt', 1,strMetric='SP-SI-SD',flWeight=flWeight,affinityOption=3)
