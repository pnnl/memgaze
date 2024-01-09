# Create intra-region SP and SI combined heatmap for selected blocks
# Only self, self+1 and ... self-* non-significant for SP & SI
    # STEP 0 - Add hot lines affinity to fill heatmap for hot cluster
    #        - repeat data from 'self', 'self+', 'self-' to respective hotlines
    #        - in application. It is not reported by location analysis (C code).
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
        # STEP 3e - Sample dataframe for 150 rows based on access count as the weight
        # STEP 4 - Visual to numeric value for SI, SD, SP
        # STEP 5a - Fill in hot lines with self data - not necessary after STEP 0
        # STEP 5b - Proceed to plot

import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import re
import csv
import os
import copy
import matplotlib as mpl
from matplotlib.colors import ListedColormap, LinearSegmentedColormap
import logging

logging.basicConfig(format='%(asctime)s,%(msecs)03d %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s',
    datefmt='%Y-%m-%d:%H:%M:%S',
    level=logging.INFO)
logger = logging.getLogger(__name__)
logger.propagate = False



sns.color_palette("light:#5A9", as_cmap=True)
sns.set()

from fileToDataframe import get_intra_obj,getFileColumnNames,getMetricColumns,getRearrangeColumns,getPageColList
from fileToDataframe import getFileColumnNamesPageRegion,getMetricColumnsPageRegion,getPageColListPageRegion
from fileToDataframe import getFileColumnNamesLineRegion,getMetricColumnsLineRegion,getPageColListLineRegion
from fileToDataframe import add_hot_lines

# Works for spatial denity, Spatial Anticipation and Interval
def intraObjectPlot(strApp, strFileName,numRegion, strMetric=None, f_avg=None,listCombineReg=None,flWeighted=None,numExtraPages:int=None,affinityOption:int=None,flPlot=None):
    print('flWeighted ', flWeighted)
    flagPhysicalPages = 0
    flagHotPages =0
    flagHotLines = 0
    if(affinityOption==1):
        flagPhysicalPages = 1
    elif (affinityOption == 2):
        flagHotPages = 1
    elif (affinityOption == 3):
        flagHotLines = 1

    # STEP 0 - Add hot lines affinity to fill heatmap
    strOutFile = strFileName+'_hot_lines'
    add_hot_lines (strFileName, strOutFile)
    strFileName = strOutFile
    # STEP 1 - Read spatial.txt and write inter-region file
    strPath=strFileName[0:strFileName.rindex('/')]
    if('SP-SI' in strMetric  ):
        fileName='/inter_object_sp-si.txt'
        lineStart='---'
        lineEnd='<----'
        # For Minivite use
        #strMetricIdentifier='Spatial_Proximity'
        strMetricIdentifier='Spatial_Interval'
        strMetricTitle='Spatial Anticipation and Interval'

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
    print('orig arRegionId ', arRegionId)

    flagAllRefLines=0
    if(listCombineReg != None and 'ALL' in listCombineReg):
        listCombineReg.remove('ALL')
        listCombineReg.extend(x for x in  arRegionId)
        flagAllRefLines=1
        print ('ALL listCombineReg ', listCombineReg)

    # STEP 2a - Add combine regions to the list
    if (listCombineReg != None):
        arRegionId.extend(x for x in listCombineReg if x not in arRegionId)
    arRegionId.sort()
    print('added from listCombineReg -> arRegionId and sorted', arRegionId)
    data_list_combine_Reg =[]
    flagProcessCombine=0
    combineCount=0

    # STEP 3 - Loop through regions in the list and plot heatmaps
    for j in range(0, len(arRegionId)):
        regionIdNumName=arRegionId[j]
        regionIdNumName_copy=arRegionId[j]
        print('regionIdNumName ', regionIdNumName)
        regionIdName =regionIdNumName[regionIdNumName.index('-')+1:]
        regionIdNum= regionIdNumName[:regionIdNumName.index('-')]
        print('regionIdName ', regionIdName, 'regionIdNumName ', regionIdNumName, regionIdNum)
        data_list_intra_obj=[]
        #print(df_inter_data['Reg_Num-Name'])
        # STEP 3a - Combine regions if there are any - incremental heatmaps are written out as of March 2, 2023
        if(listCombineReg != None and regionIdNumName in listCombineReg):
            #print('Not None', regionIdNumName)
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
            print("  should break out of loop combineCount ", combineCount)
            continue
        else:
            print("Proceed to plot combineCount ", combineCount)
        # STEP 3b - Convert list to data frame
        #print(data_list_intra_obj)
        if (flagPhysicalPages== 1):
            list_col_names=getFileColumnNames(numExtraPages)
        elif(flagHotPages == 1):
            list_col_names =getFileColumnNamesPageRegion (regionIdNum, numRegionInFile)
        elif(flagHotLines == 1):
            list_col_names =getFileColumnNamesLineRegion (strFileName, numRegionInFile)
        #print(list_col_names)
        df_intra_obj=pd.DataFrame(data_list_intra_obj,columns=list_col_names)
        #print('columns ', df_intra_obj.columns.to_list())
        logger.info('original columns')
        #print(df_intra_obj[['Address', 'reg-page-blk','p-2','p-1','self','p+1','p+2']])

        # STEP 3c-0 - Process data frame to get access, lifetime totals
        # This is only for the blocks in ten pages analysis (not whole region)
        df_intra_obj = df_intra_obj.astype({"Access": int, "Lifetime": int})
        # STEP 3c-1 - Dataframe has all three metrics, so divide by 3 for access count reporting
        accessSumBlocks= df_intra_obj['Access'].sum()/3
        arRegPages=df_intra_obj['reg_page_id'].unique()
        #print (arRegPages)
        arRegPageAccess = np.empty([len(arRegPages),1])
        for arRegionBlockId in range(0, len(arRegPages)):
            #print(arRegionBlockId, df_intra_obj[df_intra_obj['reg_page_id']==arRegPages[arRegionBlockId]]['Access'].sum()/3)
            arRegPageAccess[int(arRegionBlockId)] = df_intra_obj[df_intra_obj['reg_page_id']==arRegPages[arRegionBlockId]]['Access'].sum()/3
        #print (arRegPageAccess)
        listCombine = zip(arRegPages, arRegPageAccess.tolist())
        df_reg_pages = pd.DataFrame(listCombine, columns=('reg-page', 'Access'))

        #df_reg_pages = df_reg_pages.astype({"Access": int})
        df_reg_pages.set_index('reg-page')
        df_reg_pages.sort_index(inplace=True)
        df_reg_pages["Access"] = df_reg_pages["Access"].apply(pd.to_numeric)
        df_reg_pages = df_reg_pages.astype({"Access": int})
        #df_reg_pages.sort_values(by=['Access'],ascending=False,inplace=True)
        arRegPageAccess=(df_reg_pages[['Access']]).to_numpy()
        arRegPages=df_reg_pages['reg-page'].to_list()

        if(flagPhysicalPages ==1):
            plot_SP_col = getMetricColumns(numExtraPages)
        elif(flagHotPages == 1):
            plot_SP_col = getMetricColumnsPageRegion(regionIdNum, numRegionInFile)
        elif(flagHotLines == 1):
            plot_SP_col = getMetricColumnsLineRegion(regionIdNum, numRegionInFile)

        #print('plot_SP_col', plot_SP_col)
        # Change data to numeric
        for i in range (0,len(plot_SP_col)):
            df_intra_obj[plot_SP_col[i]]=pd.to_numeric(df_intra_obj[plot_SP_col[i]])

        #print('before rearrange ', df_intra_obj.columns.to_list())
        colRearrangeList =[]
        if ((numExtraPages !=0) and (flagPhysicalPages ==1)):
            #print('in numExtraPages NOT 0')
            colRearrangeList = getRearrangeColumns(df_intra_obj.columns.to_list())
        else:
            #print('in numExtraPages 0')
            colRearrangeList = df_intra_obj.columns.to_list()

        df_intra_obj = df_intra_obj[colRearrangeList]
        #print('after rearrange ', df_intra_obj.columns.to_list())
        df_intra_obj.set_index('reg-page-blk')
        df_intra_obj.sort_index(inplace=True)
        self_bef_drop=df_intra_obj['self'].to_list()
        # STEP 3d - drop columns with "all" NaN values
        # DROP - columns with no values
        #print(df_intra_obj[['reg_page_id', 'reg-page-blk', 'Access', 'Type', 'Stack']])
        print('before replace drop - ', df_intra_obj.shape)#, df_intra_obj.columns.to_list())
        #csvFileName=strPath+'/'+strApp.replace(' ','')+'-'+regionIdNumName.replace(' ','').replace('&','-')+'-'+strMetric+'-df.csv'
        #df_intra_obj.to_csv(csvFileName)
        df_intra_obj_drop=df_intra_obj.dropna(axis=1,how='all')
        print('after replace drop - ', df_intra_obj.shape)#, df_intra_obj.columns.to_list())
        self_aft_drop=df_intra_obj_drop['self'].to_list()
        if(self_bef_drop == self_aft_drop):
            print(" After drop equal ")
        else:
            print("Error - not equal")
            print ("before drop", self_bef_drop)
            print ("after drop", self_aft_drop)
            break


        # STEP 3e - Sample dataframe for 150 rows based on access count as the weight
        # Combined heatmap for SP and SI
        # Sample 150 reg-page-blkid lines from all blocks based on Access counts
        #print('before sample - ', df_intra_obj.shape)
        num_sample=50
        print ('SP shape ', df_intra_obj[(df_intra_obj['Type'] == 'SP')].shape)
        if (df_intra_obj[(df_intra_obj['Type'] == 'SP')].shape[0] < num_sample):
            num_sample = df_intra_obj[(df_intra_obj['Type'] == 'SP')].shape[0]
        if(flagAllRefLines):
            # FOR each region - find line with largest accesses - only shows two regions - not ALL
            df_intra_obj_sample_SP=df_intra_obj[(df_intra_obj['Type'] == 'SP')]
            #df_intra_obj_sample_SP['reg']=df_intra_obj_sample_SP.loc[df_intra_obj_sample_SP['reg_page_id'][0:(df_intra_obj_sample_SP['reg_page_id'].index('-'))]]
            df_intra_obj_sample_SP['reg'] = df_intra_obj_sample_SP.reg_page_id.apply(lambda x: x[0:x.index('-')])
            cntForRegion = round(num_sample/combineCount,0)
            print('cntForRegion ', cntForRegion)
            #print(df_intra_obj_sample_SP[df_intra_obj_sample_SP.groupby('reg')['Access'].rank(ascending=False) <= cntForRegion])
            df_intra_obj_sample_SP=df_intra_obj_sample_SP[df_intra_obj_sample_SP.groupby('reg')['Access'].rank(ascending=False) <= cntForRegion]
            df_intra_obj_sample_SP = df_intra_obj_sample_SP.head(num_sample)

        else:
            df_intra_obj_sample_SP=df_intra_obj[(df_intra_obj['Type'] == 'SP')].nlargest(num_sample, 'Access')

        print('after sample - ', df_intra_obj_sample_SP)

        df_intra_obj_sample_SP.set_index('reg-page-blk')
        df_intra_obj_sample_SP.sort_index(inplace=True)
        # Arrange heatmap by access
        df_intra_obj_sample_SP.sort_values(by=['Access'],ascending=False,inplace=True)

        print('after sample - ', df_intra_obj_sample_SP.shape)
        #print('after sample blocks - ', df_intra_obj_sample_SP['reg-page-blk'])
        #print('after sample acccess - ', df_intra_obj_sample_SP['Access'])
        accessBlockCacheLine = (df_intra_obj_sample_SP[['Access']]).to_numpy()
        list_xlabel=df_intra_obj_sample_SP['reg-page-blk'].to_list()
        #print('SP blocks', list_xlabel)

        df_intra_obj_SP=df_intra_obj_drop[(df_intra_obj_drop['Type'] == 'SP')]
        df_intra_obj_SI=df_intra_obj_drop[(df_intra_obj_drop['Type'] == 'SI')]
        df_intra_obj_SD=df_intra_obj_drop[(df_intra_obj_drop['Type'] == 'SD')]
        print('after drop SP - ', df_intra_obj_SP.shape)
        print('after drop SI - ', df_intra_obj_SI.shape)
        print('after drop SD - ', df_intra_obj_SD.shape)
        #print('df_intra_obj_SP[Stack]\n ', df_intra_obj_SP[['reg_page_id', 'reg-page-blk', 'Access','Stack']])
        #print('df_intra_obj_SD[Stack]\n ', df_intra_obj_SD[['reg_page_id', 'reg-page-blk', 'Access','Stack']])

        flNormalize=False
        normSDMax=np.ones(len(arRegPages))
        normSPMax = np.ones(len(arRegPages))
        if(flagPhysicalPages == 1):
            plot_SP_col=getPageColList(plot_SP_col)
        elif(flagHotPages ==1):
            plot_SP_col=getPageColListPageRegion(plot_SP_col)
        elif(flagHotLines ==1):
            plot_SP_col=getPageColListLineRegion(plot_SP_col)
        plot_SP_col=[item for item in plot_SP_col if 'r-' not in item]
        #print('before normalize \n', plot_SP_col)
        logger.info("before normalize ")
        #print(' before normalize  ' , df_intra_obj_SP[['reg-page-blk', 'Access', 'self']])
        list_SP_SI_SD=[[None]*(3*len(plot_SP_col)+3) for i in range(len(list_xlabel))]

        list_col_names=['reg-page-blk','reg-page','Access']
        for blkCnt in range(0,len(list_xlabel)):
            blkid= list_xlabel[blkCnt]
            list_SP_SI_SD[blkCnt][0]=blkid
            list_SP_SI_SD[blkCnt][1]=blkid[0:blkid.rfind('-')]
            #print(blkid, df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'])
            list_SP_SI_SD[blkCnt][2]=df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item()
            weight_multiplier = 1.0
            if (flWeighted == True):
                #print('blk ', blkid, ' access ', df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item(), ' accessSumBlocks ', accessSumBlocks)
                #print('Before weight ', df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item())
                weightRegPageAccess = (df_reg_pages[(df_reg_pages['reg-page'] == blkid[0:blkid.rfind('-')])]['Access'].item())
                #print(blkid, arRegPageAccessIndex, weightRegPageAccess)
                # WEIGHT by REGION - Makes everything closer to zero
                #weight_multiplier = ( df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item() *100 )/ (accessSumBlocks)
                # Shouldnt the weight be based on the page access as all the metrics are calculated within the page
                # WEIGHT by PAGE's total access - introduces more importance to lines in minimal number of pages
                # INCORRECT - SHOULD NOT BE USED
                weight_multiplier = ( df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item() *100 )/ (weightRegPageAccess)
                #if (weight_multiplier > 1.0):
                    #print('blk ', blkid, ' access ', df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item(), ' accessSumBlocks ', weightRegPageAccess)
                    #print('weight_multiplier ', weight_multiplier)
            else:
                weight_multiplier = 1.0
            for plotCnt in range(0,len(plot_SP_col)):
                plotCol= plot_SP_col[plotCnt]
                if(plotCol in df_intra_obj_SP.columns):
                    condSP = (df_intra_obj_SP['reg-page-blk'] == blkid)
                    condSI = (df_intra_obj_SI['reg-page-blk'] == blkid)
                    condSD = (df_intra_obj_SD['reg-page-blk'] == blkid)
                    resultSP = df_intra_obj_SP[condSP][plotCol]
                    resultSI = df_intra_obj_SI[condSI][plotCol]
                    resultSD = df_intra_obj_SD[condSD][plotCol]
                    #print(plotCol)
                    if resultSP.size > 0 :
                        #if(not(np.isnan(resultSP.values[0]))):
                            #print(resultSP.values[0], weight_multiplier, round((resultSP.values[0]* weight_multiplier),2))
                        list_SP_SI_SD[blkCnt][plotCnt*3+3]=round((resultSP.values[0]* weight_multiplier),2)
                    if resultSI.size >0:
                        list_SP_SI_SD[blkCnt][plotCnt*3+4]=resultSI.values[0]
                    if resultSD.size >0:
                        #print(plotCol,  resultSP.values[0], weight_multiplier, round((resultSP.values[0]* weight_multiplier),2))
                        list_SP_SI_SD[blkCnt][plotCnt*3+5]=round(resultSD.values[0]* weight_multiplier,2)
                    #if (weight_multiplier > 1.0 and resultSP.size > 0 and resultSI.size >0 and resultSD.size >0):
                    #    print( 'SP ', list_SP_SI_SD[blkCnt][plotCnt*3+1], ' SI ', list_SP_SI_SD[blkCnt][plotCnt*3+2], ' SD ', list_SP_SI_SD[blkCnt][plotCnt*3+3])
                    #print ((list_SP_SI_SD[blkCnt][plotCnt*3+3] ), (list_SP_SI_SD[blkCnt][plotCnt*3+5]))
                    arRegPageIndex = arRegPages.index(blkid[0:blkid.rfind('-')])
                    if ((list_SP_SI_SD[blkCnt][plotCnt*3+3]  is not None) and (list_SP_SI_SD[blkCnt][plotCnt*3+3] > 1.0)):
                        if( normSPMax[arRegPageIndex] < list_SP_SI_SD[blkCnt][plotCnt*3+3]):
                            normSPMax[arRegPageIndex] = list_SP_SI_SD[blkCnt][plotCnt*3+3]
                            flNormalize = True
                    if ((list_SP_SI_SD[blkCnt][plotCnt*3+5] is not None) and (list_SP_SI_SD[blkCnt][plotCnt*3+5]> 1.0)):
                        #print('greater than 1 - column name ', plotCol, ' SP ',list_SP_SI_SD[blkCnt][plotCnt*3+1], ' SD ',  list_SP_SI_SD[blkCnt][plotCnt*3+3])
                        if (normSDMax[arRegPageIndex] < list_SP_SI_SD[blkCnt][plotCnt*3+5]):
                            normSDMax[arRegPageIndex] = list_SP_SI_SD[blkCnt][plotCnt*3+5]
                            flNormalize = True

        #print(list_SP_SI)
        #print('line 352', plot_SP_col)
        for plotCol in plot_SP_col:
            list_col_names.append('SP-'+plotCol)
            list_col_names.append('SI-'+plotCol)
            list_col_names.append('SD-'+plotCol)

        #print(normSPMax, normSDMax)
        #print(list_col_names)
        #pattern=re.compile('.*p.*[0-9]')
        #print('line 357', list(filter(pattern.match, list_col_names)))
        df_SP_SI_SD=pd.DataFrame(list_SP_SI_SD,columns=list_col_names)
        #print('before drop  \n' , df_SP_SI_SD[['reg-page-blk', 'SD-self', 'SP-self', 'SI-Stack', 'SD-Stack']])

        df_SP_SI_SD=df_SP_SI_SD.dropna(axis=1, how='all')
        #print('line 358', df_SP_SI_SD.columns.to_list())
        # To show all three in tandem
        # Get rows with more SP entries - non zero SP will result in SI and maybe SD
        #print(df_SP_SI_SD['reg-page-blk'])
        logger.info("Columns line")
        #print('columns: ',  df_SP_SI_SD.columns.to_list())
        cols_df_SP_SI_SD = df_SP_SI_SD.columns.to_list()
        if (flNormalize == True):
            print ('Normalize true SD max ', normSDMax, ' SP ', normSPMax)
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
            #print('after normalize  ' , df_SP_SI_SD[['reg-page-blk', 'Access', 'SD-self', 'SP-self']])
        cols_df_SI=[]
        cols_df_SP=[]
        cols_df_SD=[]
        # Filter SP, SI and SD to show in heatmap
        pattern = re.compile('SD-')
        cols_df_orig = list(filter(pattern.match, cols_df_SP_SI_SD))
        affinityColCount=0
        for col_orig_name in cols_df_orig:
            if ( (not (df_SP_SI_SD[col_orig_name].isnull().all())) and (np.sum(df_SP_SI_SD[col_orig_name]>=0.005) >= 4)):
                affinityColCount= affinityColCount+1

        print('affinityColCount  ', affinityColCount)
        if(affinityColCount >15):
            SDthreshold =0.005
            SDcount = 4
        else:
            SDthreshold =0.005
            SDcount = 1

        for col_orig_name in cols_df_orig:
            str_col_orig_name = str(col_orig_name)
            #print('column name', str_col_orig_name)#, df_SP_SI_SD[col_orig_name])
            #print(df_SP_SI_SD[col_orig_name].isnull().all())
            if ( (not (df_SP_SI_SD[col_orig_name].isnull().all())) and (np.sum(df_SP_SI_SD[col_orig_name]>=SDthreshold) >= SDcount)):
                #print(' return value ', np.sum(df_SP_SI_SD[col_orig_name]>=0.01))
                #print(col_orig_name)
                col_SP_name='SP-'+str_col_orig_name[3:]
                col_SI_name='SI-'+str_col_orig_name[3:]
                col_SD_name='SD-'+str_col_orig_name[3:]
                #print( 'col_SI_name ', col_SI_name, ' col_SD_name ', col_SD_name)
                cols_df_SP.append(col_SP_name)
                cols_df_SI.append(col_SI_name)
                cols_df_SD.append(col_SD_name)

        # Make visualization look better
        # Add columns so that self is not the only one shown
        if(len(cols_df_SP)<15):
            if (flagPhysicalPages ==1):
                pattern = re.compile('.*self-.*')
                negSelfCols = list(filter(pattern.match, cols_df_SD))
                pattern = re.compile('.*--.*')
                negPageCols = list(filter(pattern.match, cols_df_SD))
                if(len(negSelfCols) ==0 and len(negPageCols)==0 ):
                    cols_df_SP.insert(0,'SP-self-2')
                    cols_df_SP.insert(1,'SP-self-1')
                    df_SP_SI_SD['SP-self-2']=np.NaN
                    df_SP_SI_SD['SP-self-1']=np.NaN
                    cols_df_SD.insert(0,'SD-self-2')
                    cols_df_SD.insert(1,'SD-self-1')
                    df_SP_SI_SD['SD-self-2']=np.NaN
                    df_SP_SI_SD['SD-self-1']=np.NaN
                    cols_df_SI.insert(0,'SI-self-2')
                    cols_df_SI.insert(1,'SI-self-1')
                    df_SP_SI_SD['SI-self-2']=np.NaN
                    df_SP_SI_SD['SI-self-1']=np.NaN

                pattern = re.compile('.*self\+.*')
                posSelfCols = list(filter(pattern.match, cols_df_SD))
                pattern = re.compile('.*-\+.*')
                posPageCols = list(filter(pattern.match, cols_df_SD))
                if(len(posSelfCols) ==0 and len(posPageCols)==0):
                    cols_df_SP.append('SP-self+1')
                    cols_df_SP.append('SP-self+2')
                    df_SP_SI_SD['SP-self+1']=np.NaN
                    df_SP_SI_SD['SP-self+2']=np.NaN
                    cols_df_SD.append('SD-self+1')
                    cols_df_SD.append('SD-self+2')
                    df_SP_SI_SD['SD-self+1']=np.NaN
                    df_SP_SI_SD['SD-self+2']=np.NaN
                    cols_df_SI.append('SI-self+1')
                    cols_df_SI.append('SI-self+2')
                    df_SP_SI_SD['SI-self+1']=np.NaN
                    df_SP_SI_SD['SI-self+2']=np.NaN

            pattern = re.compile('.*self-.*')
            negSelfCols = list(filter(pattern.match, cols_df_SD))
            pattern = re.compile('.*self\+.*')
            posSelfCols = list(filter(pattern.match, cols_df_SD))
            print('cols_df_SD for ', strApp, ' ', cols_df_SD)
            print('posSelfCols ', posSelfCols)
            print('negSelfCols ', negSelfCols)
            if(len(negSelfCols)==0 ):
                colSelfIndex = cols_df_SD.index('SD-self') if 'SD-self' in cols_df_SD else -1
                if(colSelfIndex != -1):
                    cols_df_SP.insert(colSelfIndex, 'SP-self-2')
                    cols_df_SD.insert(colSelfIndex, 'SD-self-2')
                    cols_df_SI.insert(colSelfIndex, 'SI-self-2')
                    cols_df_SP.insert(colSelfIndex+1, 'SP-self-1')
                    cols_df_SD.insert(colSelfIndex+1, 'SD-self-1')
                    cols_df_SI.insert(colSelfIndex+1, 'SI-self-1')
                    df_SP_SI_SD['SP-self-1']=np.NaN
                    df_SP_SI_SD['SD-self-1']=np.NaN
                    df_SP_SI_SD['SI-self-1']=np.NaN
                    df_SP_SI_SD['SP-self-2']=np.NaN
                    df_SP_SI_SD['SD-self-2']=np.NaN
                    df_SP_SI_SD['SI-self-2']=np.NaN

            if(len(posSelfCols)==0):
                colSelfIndex = cols_df_SD.index('SD-self') if 'SD-self' in cols_df_SD else -1
                #print('posSelfCols ==0', colSelfIndex)
                if(colSelfIndex != -1):
                    cols_df_SP.insert(colSelfIndex+1, 'SP-self+1')
                    cols_df_SD.insert(colSelfIndex+1, 'SD-self+1')
                    cols_df_SI.insert(colSelfIndex+1, 'SI-self+1')
                    cols_df_SP.insert(colSelfIndex+2, 'SP-self+2')
                    cols_df_SD.insert(colSelfIndex+2, 'SD-self+2')
                    cols_df_SI.insert(colSelfIndex+2, 'SI-self+2')
                    df_SP_SI_SD['SP-self+1']=np.NaN
                    df_SP_SI_SD['SD-self+1']=np.NaN
                    df_SP_SI_SD['SI-self+1']=np.NaN
                    df_SP_SI_SD['SP-self+2']=np.NaN
                    df_SP_SI_SD['SD-self+2']=np.NaN
                    df_SP_SI_SD['SI-self+2']=np.NaN

        #print(cols_df_SP)
        #print(cols_df_SI)
        pattern = re.compile('SD-self-\d\d.*')
        negSelfCols = list(filter(pattern.match, cols_df_SD))
        if(len(negSelfCols)>10 ):
            print ('***** More negative cols', negSelfCols)
            for negSelfColsItem in negSelfCols:
                if((negSelfColsItem)in df_SP_SI_SD):
                    #print('more negative negSelfColsItem ', negSelfColsItem, 'count ', np.sum(df_SP_SI_SD[negSelfColsItem]>=SDthreshold) )
                    df_SP_SI_SD.drop(negSelfColsItem, axis=1, inplace=True)
                cols_df_SD.remove(negSelfColsItem)

            pattern = re.compile('SP-self-\d\d.*')
            negSelfCols = list(filter(pattern.match, cols_df_SP))
            if(len(negSelfCols)>10 ):
                print ('***** More negative cols', negSelfCols)
                for negSelfColsItem in negSelfCols:
                    #print('more negative negSelfColsItem ', negSelfColsItem, 'count ', np.sum(df_SP_SI_SD[negSelfColsItem]>=SDthreshold) )
                    if((negSelfColsItem)in df_SP_SI_SD):
                        df_SP_SI_SD.drop(negSelfColsItem, axis=1, inplace=True)
                    cols_df_SP.remove(negSelfColsItem)

            pattern = re.compile('SI-self-\d\d.*')
            negSelfCols = list(filter(pattern.match, cols_df_SI))
            if(len(negSelfCols)>10 ):
                print ('***** More negative cols', negSelfCols)
                for negSelfColsItem in negSelfCols:
                    if((negSelfColsItem)in df_SP_SI_SD):
                    #print('more negative negSelfColsItem ', negSelfColsItem, 'count ', np.sum(df_SP_SI_SD[negSelfColsItem]>=SDthreshold) )
                        df_SP_SI_SD.drop(negSelfColsItem, axis=1, inplace=True)
                    cols_df_SI.remove(negSelfColsItem)

        # STEP 4 - Visual to numeric value for SI, SD, SP
        # Visual to numeric value for SI, SD, SP - find hot reference blocks and hot affinity blocks
        # HOT blocks - hot lines and self-1, self, self+1, self+2 ??
        pattern = re.compile('SD-.*-.*-.*')
        #pattern = re.compile('SD-.*')
        listAffinityLines=list(filter(pattern.match, cols_df_SD))
        listAffinityLines.append('SD-self')
        #Adding more to the box is not helpful as it adds non-hot lines
        if (0):
            if('SD-self-1' in cols_df_SD):
                listAffinityLines.append('SD-self-1')
            if('SD-self+1' in cols_df_SD):
                listAffinityLines.append('SD-self+1')
            if('SD-self+1' in cols_df_SD):
                listAffinityLines.append('SD-self+2')

        for i in range(0,len(listAffinityLines)):
            listAffinityLines[i] = listAffinityLines[i].replace('SD-','')
        logger.info("HOT lines in affinity")
        print('hot lines in affinity', listAffinityLines)
        pattern = re.compile('self.*')
        listSelfLines = list(filter(pattern.match, listAffinityLines))
        print(' listSelfLines ' , listSelfLines)
        # Trial 1 - find hot reference blocks
        #print(df_SP_SI_SD['Access'].sum())
        #sumAccessRefBlocks = df_SP_SI_SD['Access'].sum()
        #val = pd.Index(df_SP_SI_SD.Access.cumsum()).get_loc(int((0.9*sumAccessRefBlocks)), 'nearest', tolerance=int(0.05*sumAccessRefBlocks))
        #print(val)
        #df_SP_SI_SD.at[val,'Access']
        #print(df_SP_SI_SD.at[val,'Access'])

        # Trial 2 - find hot reference blocks - use range as in the heatmap for block accesses
        #print(df_SP_SI_SD['Access'])
        hotRefMaxAccess=df_SP_SI_SD['Access'].max()
        hotRefMinAccess=df_SP_SI_SD['Access'].min()
        refRange=hotRefMaxAccess-hotRefMinAccess
        #print((df_SP_SI_SD['Access']-hotRefMinAccess)/refRange)
        df_SP_SI_SD['Hot-Access']=(df_SP_SI_SD['Access']-hotRefMinAccess)/refRange
        # ROWS - hot reference blocks
        if( flagAllRefLines ==1):
            list_hot_ref_blocks=df_SP_SI_SD[df_SP_SI_SD['Hot-Access'] > 0]['reg-page-blk'].to_list()
        else:
            list_hot_ref_blocks=df_SP_SI_SD[df_SP_SI_SD['Hot-Access'] > 0.1]['reg-page-blk'].to_list()
        logger.info("HOT lines in reference")
        print('list_hot_ref_blocks ', list_hot_ref_blocks)

        # COLUMNS - hot affinity blocks
        listHotAffColumns=['reg-page-blk','Access','Hot-Access']


        for i in range(0,len(listAffinityLines)):
            listHotAffColumns.append('SD-'+listAffinityLines[i])
            listHotAffColumns.append('SP-'+listAffinityLines[i])
            listHotAffColumns.append('SI-'+listAffinityLines[i])
        if( flagAllRefLines ==1):
             df_hot_SP_SI_SD=df_SP_SI_SD[listHotAffColumns].copy()
        else:
            df_hot_SP_SI_SD=df_SP_SI_SD[df_SP_SI_SD['Hot-Access'] > 0.1][listHotAffColumns].copy()

        #print(df_hot_SP_SI_SD.columns)
        #print(df_hot_SP_SI_SD)

        print('--- Info --- App', strApp, ' Reg ', regionIdNumName, ' hot DF size ',df_hot_SP_SI_SD.shape )
        print('df_hot_SP_SI_SD rows ', df_hot_SP_SI_SD['reg-page-blk'].tolist())
        print('df_hot_SP_SI_SD columns ' , df_hot_SP_SI_SD.columns.tolist())
        #df_hot_SP_SI_SD=df_hot_SP_SI_SD.dropna(axis=1, how='all')
        print('--- Info --- App', strApp, ' Reg ', regionIdNumName, ' hot DF size',df_hot_SP_SI_SD.shape )

        # Cannot count self affinity twice - so number of hot blocks in bounded box is less than the size of rectangle
        listHotRefLines = df_hot_SP_SI_SD['reg-page-blk'].to_list()
        listSelfHotAffLines = [x for x in listAffinityLines if x in listHotRefLines]
        print ('listSelfHotAffLines ', listSelfHotAffLines)

        pattern = re.compile('SD-.*')
        cntBlocksinHotBox = df_hot_SP_SI_SD.shape[0] *((df_hot_SP_SI_SD.shape[1]-3)/3)
        listAffinityLines=list(filter(pattern.match, df_hot_SP_SI_SD.columns))
        for i in range(0,len(listAffinityLines)):
            listAffinityLines[i] = listAffinityLines[i].replace('SD-','')
        if('Stack' in listAffinityLines):
            listAffinityLines.remove('Stack')
        print('df_hot_SP_SI_SD.columns match listAffinityLines ', listAffinityLines)
        #df_SP_SI_SD.loc[df_SP_SI_SD['reg-page'] == regPageId, [col_SD_name]]
        #print(df_hot_SP_SI_SD.loc[df_hot_SP_SI_SD['reg-page-blk'] == '1-1-116',['SP-self','SP-self+2','SP-1-1-116','SP-1-1-118']])
        #print(df_hot_SP_SI_SD.loc[df_hot_SP_SI_SD['reg-page-blk'] == '1-1-118',['SP-self','SP-self-2','SP-1-1-116','SP-1-1-118']])
        #print(df_hot_SP_SI_SD['reg-page-blk'])
        #listHotRefLines = df_hot_SP_SI_SD['reg-page-blk'].to_list()
        #print ('listHotRefLines ', listHotRefLines)
        #print(listAffinityLines)
        # Trial 1 for SA-SI combined metric - filter based on SA
        # Code section removed on Dec 21, 2023

        print(' Hot affinity lines 1 ', listAffinityLines)
        #print(df_hot_SP_SI_SD.columns)
        # Trial 2 for SA-SI combined metric - penalty based on SI
        # Not used - csv written and read by composite-metric-plot.py
        flGetSAforSI=1
        cntSABlocksinHotBox =0
        cntSDBlocksinHotBox =0
        if (flGetSAforSI):
            numBins = 100
            listSAbins = [0] * numBins
            listSDbins = [0] * numBins
            binRange = float(1/numBins)

            for index, row in df_hot_SP_SI_SD.iterrows():
                for i in range(0,len(listAffinityLines)):
                    cmpSAvalue=np.NaN
                    cmpSDvalue=np.NaN
                    #print(row['SP-'+listAffinityLines[i]])#, df_hot_SP_SI_SD['SP-'+listAffinityLines[i]])
                    if((~np.isnan(row['SI-'+listAffinityLines[i]])) and row['SI-'+listAffinityLines[i]] <= 63 ):
                        cmpSAvalue = row['SP-'+listAffinityLines[i]]
                        cmpSDvalue = row['SD-'+listAffinityLines[i]]
                    else:
                        valSIPenalty = (row['SI-'+listAffinityLines[i]]% 63)+1
                        cmpSAvalue = row['SP-'+listAffinityLines[i]] /valSIPenalty
                        cmpSDvalue = row['SD-'+listAffinityLines[i]] /valSIPenalty
                    if(~np.isnan(cmpSAvalue)):
                        cntSABlocksinHotBox = cntSABlocksinHotBox+1
                    if(~np.isnan(cmpSDvalue)):
                        cntSDBlocksinHotBox = cntSDBlocksinHotBox +1
                    listSAindex=np.NaN
                    listSDindex=np.NaN
                    for i in range (0, len(listSAbins)):
                        if(~np.isnan(cmpSAvalue)):
                            if( ((i * binRange) <= float(cmpSAvalue)) and (float(cmpSAvalue) < ((i+1) * binRange))):
                                listSAindex = i
                                break
                    for i in range (0, len(listSDbins)):
                        if (~np.isnan(cmpSDvalue)):
                            if( ((i * binRange) <= float(cmpSDvalue)) and (float(cmpSDvalue) < ((i+1) * binRange))):
                                listSDindex = i
                                break
                    if(~np.isnan(listSAindex)):
                        listSAbins[listSAindex] +=1
                    if(~np.isnan(listSDindex)):
                        listSDbins[listSDindex] +=1
                    #print(' SA ', cmpSAvalue, ' bin ', listSAindex, ' SD ', cmpSDvalue, ' bin ', listSDindex)

            #print('listSAbins ', listSAbins)
            #print('listSDbins ', listSDbins)
            print('cnt BlocksinHotBox ', cntBlocksinHotBox)
            print('cnt BlocksinHotBox without hot ref', (cntBlocksinHotBox-len(listSelfHotAffLines)))
            print('cnt SA BlocksinHotBox ', cntSABlocksinHotBox)
            print('cnt SD BlocksinHotBox ', cntSDBlocksinHotBox)

            if(flagAllRefLines ==1 ):
                if(cntBlocksinHotBox !=0):
                    listPercentSAbins = [(float(x) * 100/ (cntSABlocksinHotBox)) for x in listSAbins]
                    listPercentSDbins = [(float(x) * 100/ (cntSDBlocksinHotBox)) for x in listSDbins]
            else:
                if(cntBlocksinHotBox !=0):
                    listPercentSAbins = [(float(x) * 100/ (cntBlocksinHotBox-len(listSelfHotAffLines))) for x in listSAbins]
                    listPercentSDbins = [(float(x) * 100/ (cntBlocksinHotBox-len(listSelfHotAffLines))) for x in listSDbins]

            listPercentSAbins =  [ round(elem,1) for elem in listPercentSAbins ]
            listPercentSDbins =  [ round(elem,1) for elem in listPercentSDbins ]
            print('--- Info --- App', strApp, ' Reg ', regionIdNumName, ' listSAbins ', listSAbins)
            print('--- Info --- App', strApp, ' Reg ', regionIdNumName, '  SAbins  sumSAbins ', sum(listSAbins), ' bad ', sum(listSAbins[:25]) , ' good ', sum(listSAbins[25:]))
            print('--- Info --- App', strApp, ' Reg ', regionIdNumName, '  SAbins % sum ', sum(listPercentSAbins), ' bad ', sum(listPercentSAbins[:25]) , ' good ', sum(listPercentSAbins[25:]))
            print('--- Info --- App', strApp, ' Reg ', regionIdNumName, ' listSDbins ', listSDbins)
            print('--- Info --- App', strApp, ' Reg ', regionIdNumName, '  SDbins sumSDbins ', sum(listSDbins), ' bad ', sum(listSDbins[:5]) , ' good ', sum(listSDbins[5:]))
            print('--- Info --- App', strApp, ' Reg ', regionIdNumName, '  SDbins % sum ', sum(listPercentSDbins), ' bad ', sum(listPercentSDbins[:5]) , ' good ', sum(listPercentSDbins[5:]))
            print('--- Info --- App', strApp, ' Reg ', regionIdNumName, ' % SAbins ', listPercentSAbins)
            print('--- Info --- App', strApp, ' Reg ', regionIdNumName, ' % SDbins ', listPercentSDbins)

        pattern = re.compile('SD-.*-.*-.*')
        listAffinityLines=list(filter(pattern.match, cols_df_SD))
        for i in range(0,len(listAffinityLines)):
            listAffinityLines[i] = listAffinityLines[i].replace('SD-','')
        logger.info("HOT lines in affinity")
        #print('hot lines in affinity', listAffinityLines)

        # STEP 5a - Fill in hot lines with self data - for visualization only
        # Repeat 'self' values in hot lines in affinity - otherwise, there is a diagonal hole in heatmap
        # Completed by fill in STEP 0 - add_hot_lines
        #for i in range(0,len(listAffinityLines)):
            #print(df_SP_SI_SD.loc[df_SP_SI_SD['reg-page-blk'] == listAffinityLines[i], 'SD-self'].values)
            #df_SP_SI_SD.loc[df_SP_SI_SD['reg-page-blk'] == listAffinityLines[i],'SD-'+listAffinityLines[i]] = df_SP_SI_SD.loc[df_SP_SI_SD['reg-page-blk'] == listAffinityLines[i], 'SD-self']
            #df_SP_SI_SD.loc[df_SP_SI_SD['reg-page-blk'] == listAffinityLines[i],'SP-'+listAffinityLines[i]] = df_SP_SI_SD.loc[df_SP_SI_SD['reg-page-blk'] == listAffinityLines[i], 'SP-self']
            #df_SP_SI_SD.loc[df_SP_SI_SD['reg-page-blk'] == listAffinityLines[i],'SI-'+listAffinityLines[i]] = df_SP_SI_SD.loc[df_SP_SI_SD['reg-page-blk'] == listAffinityLines[i], 'SI-self']

        csvFileName=strPath+'/'+strApp.replace(' ','')+'-'+regionIdNumName.replace(' ','').replace('&','-')+'-'+strMetric+'-df.csv'
        dfAllCols=df_SP_SI_SD.columns.to_list()
        pattern=re.compile("S.*-")
        removeCols=list(filter(pattern.match, dfAllCols))
        csvColList=[]
        if(len(removeCols)!=0):
            subList = [x for x in dfAllCols if x not in removeCols]
        csvColList.extend(subList)
        print('csvColList ', csvColList)
        hmAffLines=[]
        hmAffLines.extend(cols_df_SP)
        hmAffLines.extend(cols_df_SI)
        hmAffLines.extend(cols_df_SD)
        print('hmAffLines ', hmAffLines)
        csvColList.extend(hmAffLines)
        df_hot_SP_SI_SD_csv=df_SP_SI_SD[csvColList]
        df_hot_SP_SI_SD_csv['TotalAccess']=arRegionAccess
        df_hot_SP_SI_SD_csv['PageAccess']=accessSumBlocks
        df_hot_SP_SI_SD_csv.to_csv(csvFileName)

        # STEP 5b - Proceed to plot
        if ((flPlot == True) and len(cols_df_SP) > 0 and len(cols_df_SI) >0 and len(cols_df_SD)>0):
            df_intra_obj_sample_hm_SP=df_SP_SI_SD[cols_df_SP]
            df_intra_obj_sample_hm_SI=df_SP_SI_SD[cols_df_SI]
            df_intra_obj_sample_hm_SD=df_SP_SI_SD[cols_df_SD]
            df_intra_obj_sample_hm_SP.apply(pd.to_numeric)
            df_hm_SP=np.empty([num_sample,len(cols_df_SP)])
            df_hm_SP=df_intra_obj_sample_hm_SP.to_numpy()
            df_hm_SP=df_hm_SP.astype('float64')
            df_hm_SP=df_hm_SP.transpose()
            print('df_hm_SP shape', df_hm_SP.shape)
            #print(df_hm_SP)

            df_intra_obj_sample_hm_SI.apply(pd.to_numeric)
            df_hm_SI=np.empty([num_sample,len(cols_df_SI)])
            df_hm_SI=df_intra_obj_sample_hm_SI.to_numpy()
            df_hm_SI=df_hm_SI.astype('float64')
            df_hm_SI=df_hm_SI.transpose()
            print('df_hm_SI shape', df_hm_SI.shape)

            df_intra_obj_sample_hm_SD.apply(pd.to_numeric)
            df_hm_SD=np.empty([num_sample,len(cols_df_SD)])
            df_hm_SD=df_intra_obj_sample_hm_SD.to_numpy()
            df_hm_SD=df_hm_SD.astype('float64')
            df_hm_SD=df_hm_SD.transpose()
            print('df_hm_SD shape', df_hm_SD.shape)

            fig = plt.figure(constrained_layout=True, figsize=(15, 12))
            gs = gridspec.GridSpec(1, 2, figure=fig,width_ratios=[12,3])
            gsleft = gridspec.GridSpecFromSubplotSpec(3, 1, subplot_spec = gs[0] ,hspace=0.02)#,height_ratios=[3.25,3.5,3.25])
            gsrightwhole=gridspec.GridSpecFromSubplotSpec(2, 1, height_ratios=[0.01,9.99], subplot_spec = gs[1] ,hspace=0.0)
            gsright = gridspec.GridSpecFromSubplotSpec(1, 2, subplot_spec = gsrightwhole[1] ,wspace=0.02)
            gsrighttop= fig.add_subplot(gsrightwhole[0])
            gsrighttop.set_title('Access')
            gsrighttop.axis('off')

            gsleft_ax_0 = fig.add_subplot(gsleft[0, 0])
            gsleft_ax_1 = fig.add_subplot(gsleft[1, 0])
            gsleft_ax_2 = fig.add_subplot(gsleft[2, 0])
            ax_2 = fig.add_subplot(gsright[0, 0])
            ax_3 = fig.add_subplot(gsright[0, 1])
            gsleft_ax_0 = sns.heatmap(df_hm_SD, cmap='mako_r',cbar=True, cbar_kws={"pad":0.02}, annot=False,ax=gsleft_ax_0,vmin=0.02,vmax=1.0)
            gsleft_ax_0.axvline(x=0, color='k',linewidth=1)
            gsleft_ax_0.axhline(y=0, color='k',linewidth=1)
            gsleft_ax_0.axhline(y=df_hm_SD.shape[0], color='k',linewidth=1)
            gsleft_ax_0.axvline(x=df_hm_SD.shape[1], color='k',linewidth=1)


            gsleft_ax_0.set_facecolor('white')
            gsleft_ax_1 = sns.heatmap(df_hm_SP, cmap='mako_r',cbar=True, cbar_kws={"pad":0.02}, annot=False,ax=gsleft_ax_1,vmin=0.02, vmax=1.0)
            gsleft_ax_1.axvline(x=0, color='k',linewidth=1)
            gsleft_ax_1.axhline(y=0, color='k',linewidth=1)
            gsleft_ax_1.axhline(y=df_hm_SP.shape[0], color='k',linewidth=1)
            gsleft_ax_1.axvline(x=df_hm_SP.shape[1], color='k',linewidth=1)

            gsleft_ax_1.set_facecolor('white')
            vmin=0
            vmax=64
            gsleft_ax_2 = sns.heatmap(df_hm_SI, cmap='mako',cbar=True, cbar_kws={"pad":0.02}, annot=False,ax=gsleft_ax_2,vmin=vmin, vmax=vmax)
            gsleft_ax_2.axvline(x=0, color='k',linewidth=1)
            gsleft_ax_2.axhline(y=0, color='k',linewidth=1)
            gsleft_ax_2.axhline(y=df_hm_SI.shape[0], color='k',linewidth=1)
            gsleft_ax_2.axvline(x=df_hm_SI.shape[1], color='k',linewidth=1)

            gsleft_ax_2.set_facecolor('white')
            gsleft_ax_0.set_ylabel("SD",fontsize=16,style='italic')
            gsleft_ax_1.set_ylabel("SA",fontsize=16,style='italic')
            gsleft_ax_2.set(xlabel="Region-Page-Block")
            gsleft_ax_2.set_ylabel("SI", fontsize=16,style='italic')
            custom_blue = sns.light_palette("#79C")
            background_color = mpl.colormaps["Blues"]
            hotLineColor='darkred'
            affRegionColor='dodgerblue'
            hotLineInRegionColor='indianred'
            refRegionColor='black'
            refRgionNamelist = regionIdNumName.replace(' ','').split('&')
            refRegionList=[]
            #print(refRgionNamelist)
            for item in refRgionNamelist:
                #print('region id', item.split('-')[0])
                refRegionList.append(item.split('-')[0])

            gsleft_ax_0.invert_yaxis()
            #list_y_ticks=gsleft_ax_0.get_yminorticklabels() # its always zero
            list_y_ticks=gsleft_ax_0.get_yticklabels()
            fig_ylabel=[]
            logger.info("SD labels")
            print('SD App ', strApp, 'len', len(cols_df_SD),' cols ',cols_df_SD)
            #print('len ', len(list_y_ticks),' list_y ', list_y_ticks)
            for y_label in list_y_ticks:
                if ('self' in cols_df_SD[int(y_label.get_text())]):
                    fig_ylabel.append(cols_df_SD[int(y_label.get_text())][7:])
                else:
                    strColName = cols_df_SP[int(y_label.get_text())][3:]
                    if(strColName.find('^') != -1):
                        strColName=strColName.replace('[2','[$2').replace('2^','2^{').replace('-2','}$-$2').replace(')','}$)')
                    if((strColName.find('r-') != -1)):
                        strColName=cols_df_SP[int(y_label.get_text())][5:]
                    fig_ylabel.append(strColName)
            list_x_ticks=gsleft_ax_0.get_xticklabels()
            fig_xlabel=[]
            for x_label in list_x_ticks:
                fig_xlabel.append(df_SP_SI_SD['reg-page-blk'][int(x_label.get_text())])
            gsleft_ax_0.set_yticklabels(fig_ylabel,rotation='horizontal')
            for ticklabel in gsleft_ax_0.get_yticklabels():
                if(ticklabel.get_text().count('-')==2):
                    if(ticklabel.get_text().split('-')[0]  in refRegionList):
                        ticklabel.set_color(hotLineInRegionColor)
                    else:
                        ticklabel.set_color(hotLineColor)
                elif(ticklabel.get_text().isdigit()):
                    if(ticklabel.get_text()  in refRegionList):
                        ticklabel.set_color(refRegionColor)
                    else:
                        ticklabel.set_color(affRegionColor)

            gsleft_ax_0.set_xticklabels([])

            gsleft_ax_1.invert_yaxis()
            #gsleft_ax_1.set_yticks(range(len(cols_df_SP)))
            list_y_ticks=gsleft_ax_1.get_yticklabels()
            fig_ylabel=[]
            #print('SP App ', strApp, ' list_y ', list_y_ticks, ' len ', len(list_y_ticks))
            for y_label in list_y_ticks:
                if ('self' in cols_df_SP[int(y_label.get_text())]):
                    fig_ylabel.append(cols_df_SP[int(y_label.get_text())][7:])
                else:
                    strColName = cols_df_SP[int(y_label.get_text())][3:]
                    if(strColName.find('^') != -1):
                        strColName=strColName.replace('[2','[$2').replace('2^','2^{').replace('-2','}$-$2').replace(')','}$)')
                    if((strColName.find('r-') != -1)):
                        strColName=cols_df_SP[int(y_label.get_text())][5:]

                    #print(strColName)
                    fig_ylabel.append(strColName)
            gsleft_ax_1.set_yticklabels(fig_ylabel,rotation='horizontal')
            for ticklabel in gsleft_ax_1.get_yticklabels():
                if(ticklabel.get_text().count('-')==2):
                    if(ticklabel.get_text().split('-')[0] in refRegionList):
                        ticklabel.set_color(hotLineInRegionColor)
                    else:
                        ticklabel.set_color(hotLineColor)
                elif(ticklabel.get_text().isdigit()):
                    if(ticklabel.get_text() in refRegionList):
                        ticklabel.set_color(refRegionColor)
                    else:
                        ticklabel.set_color(affRegionColor)
            gsleft_ax_1.set_xticklabels([])

            gsleft_ax_2.invert_yaxis()
            list_y_ticks=gsleft_ax_2.get_yticklabels()
            #print('SI App ', strApp, ' list_y ', list_y_ticks, ' len ', len(list_y_ticks))
            fig_ylabel=[]
            for y_label in list_y_ticks:
                if ('self' in cols_df_SI[int(y_label.get_text())]):
                    fig_ylabel.append(cols_df_SI[int(y_label.get_text())][7:])
                else:
                    strColName = cols_df_SP[int(y_label.get_text())][3:]
                    if(strColName.find('^') != -1):
                        strColName=strColName.replace('[2','[$2').replace('2^','2^{').replace('-2','}$-$2').replace(')','}$)')
                    if((strColName.find('r-') != -1)):
                        strColName=cols_df_SP[int(y_label.get_text())][5:]
                    fig_ylabel.append(strColName)
            list_x_ticks=gsleft_ax_2.get_xticklabels()
            fig_xlabel=[]
            my_colors=[]
            color_xlabel=[]
            accessLowValue = accessBlockCacheLine.min()
            accessHighValue = accessBlockCacheLine.max()
            accessRange= accessHighValue-accessLowValue
            for x_label in list_x_ticks:
                fig_xlabel.append(df_SP_SI_SD['reg-page-blk'][int(x_label.get_text())])
                color_xlabel.append((accessBlockCacheLine[int(x_label.get_text())].item()-accessLowValue)/accessRange)
                my_colors.append(background_color((accessBlockCacheLine[int(x_label.get_text())].item()-accessLowValue)/accessRange))
            gsleft_ax_2.set_yticklabels(fig_ylabel,rotation='horizontal')
            gsleft_ax_2.set_xticklabels(fig_xlabel,rotation='vertical')
            for ticklabel in gsleft_ax_2.get_yticklabels():
                if(ticklabel.get_text().count('-')==2):
                    if(ticklabel.get_text().split('-')[0] in refRegionList):
                        ticklabel.set_color(hotLineInRegionColor)
                    else:
                        ticklabel.set_color(hotLineColor)
                elif(ticklabel.get_text().isdigit()):
                    if(ticklabel.get_text()  in refRegionList):
                        ticklabel.set_color(refRegionColor)
                    else:
                        ticklabel.set_color(affRegionColor)

            i=0
            for ticklabel, tickcolor in zip(gsleft_ax_2.get_xticklabels(), my_colors):
                if(color_xlabel[i]>0.5):
                    ticklabel.set_color('white')
                ticklabel.set_backgroundcolor(tickcolor)
                ticklabel.set_fontsize(10)
                i = i+1


            list_xlabel=df_SP_SI_SD['reg-page-blk']
            if(len(accessBlockCacheLine) < 20):
                #print('accessBlockCacheLine  ', accessBlockCacheLine)
                sumAccessBlockCacheLine = accessBlockCacheLine.sum()
                accessBlockCacheLinePercent= np.asarray(((accessBlockCacheLine / sumAccessBlockCacheLine) *100)).astype(int)
                #print('accessBlockCacheLinePercent ', accessBlockCacheLinePercent)
                formatted_text = (np.asarray(["{0:,d} \n ({1:d}%)".format(spValue,srValue) for spValue, srValue \
                                      in zip(accessBlockCacheLine.flatten(), accessBlockCacheLinePercent.flatten())])).reshape(len(accessBlockCacheLine),1)
                #print(formatted_text)
                sns.heatmap(accessBlockCacheLine,cmap='Blues',cbar=False, annot=formatted_text, fmt="", annot_kws = {'size':12}, ax=ax_2)#,linecolor='black',linewidths=1)
            else:
                sns.heatmap(accessBlockCacheLine, cmap='Blues', cbar=False,annot=True, fmt=',d', annot_kws = {'size':12},  ax=ax_2)#,linecolor='black',linewidths=1)

            ax_2.set_xticks([0])

            length_xlabel= len(list_xlabel)
            list_blkcache_label=[]
            for i in range (0, length_xlabel):
                list_blkcache_label.append(list_xlabel[i])
            ax2_ylabel=[]
            list_y_ticks=ax_2.get_yticklabels()
            for y_label in list_y_ticks:
                ax2_ylabel.append(list_blkcache_label[int(y_label.get_text())])
            ax_2.set_yticklabels(list_blkcache_label, rotation='horizontal', wrap=True )
            ax_2.yaxis.set_ticks_position('right')
            for ticklabel in ax_2.get_yticklabels():
                if(ticklabel.get_text() in listAffinityLines):
                    ticklabel.set_color(hotLineInRegionColor)

            rndAccess = np.round((arRegPageAccess/1000).astype(float),2)
            annot = np.char.add(rndAccess.astype(str), 'K')
            sns.heatmap(rndAccess, cmap=custom_blue, cbar=False,annot=annot, fmt='', annot_kws = {'size':12},  ax=ax_3)
            ax_3.set_xticks([0])

            print(len(rndAccess))
            print(len(arRegPages))
            list_y_ticks=ax_3.get_yticklabels()
            fig_ylabel=[]
            for y_label in list_y_ticks:
                    fig_ylabel.append(arRegPages[int(y_label.get_text())])
            # TOO long - for all regions
            #ax_3.set_yticklabels(arRegPages, rotation='horizontal', wrap=True )
            ax_3.set_yticklabels(fig_ylabel, rotation='horizontal', wrap=True )
            ax_3.yaxis.set_ticks_position('right')


            strMetricTitle = 'Spatial Density, Anticipation, Interval'
            #plt.suptitle(strMetricTitle +' and Access heatmap for '+strApp +' region - '+regionIdNumName)
            strArRegionAccess = str(np.round((arRegionAccess/1000).astype(float),2))+'K'
            strAccessSumBlocks= str(np.round((accessSumBlocks/1000).astype(float),2))+'K'
            #strTitle = 'Region\'s access - ' + strArRegionAccess + ', Access count for selected pages - ' + strAccessSumBlocks +' ('+ ("{0:.1f}".format((accessSumBlocks/arRegionAccess)*100)) \
            #           +'%), Number of pages in region - '+ str(numRegionBlocks) + '\n Spatial Density'
            strTitle = 'Access: Region - ' + strArRegionAccess + ', Pages - '+ strAccessSumBlocks +' ('+ ("{0:.1f}".format((accessSumBlocks/arRegionAccess)*100))+'%)'
            #print(strTitle)
            gsleft_ax_0.set_title(strTitle)
            #gsleft_ax_1.set_title('Spatial Anticipation')
            #gsleft_ax_2.set_title('Spatial Interval')
            #ax_2.set_title('Access count for selected blocks and \n         hottest pages in the region',loc='left')
            #ax_2.set_title('Access: Blocks', loc='right')
            ax_2.set_title('Blocks')
            ax_3.set_title('Pages')
            fileNameLastSeg = '_hm_order_fill.pdf'
            if (flWeighted == True):
                fileNameLastSeg = '_hm_order_Wgt.pdf'
            imageFileName=strPath+'/'+strApp.replace(' ','')+'-'+regionIdNumName.replace(' ','').replace('&','-')+'-'+strMetric+fileNameLastSeg
            print(imageFileName)
            #plt.show()
            plt.savefig(imageFileName, bbox_inches='tight')
            plt.close()



flWeight=False
f_avg1=None
mainPath='/Users/suri836/Projects/spatial_rud/'
flPlot=True

if(0):
    flWeight=True
    #intraObjectPlot('Alpaca',mainPath+'spatial_pages_exp/alpaca/mg-alpaca-noinline/chat-trace-b32768-p6000000-questions_copy/spatial.txt',7,strMetric='SD-SP-SI', \
    #         listCombineReg=['0-A0000000', '1-A0000010','2-A0000011','3-A0000012','4-A0000013'], flWeighted=flWeight,affinityOption=3)
    #intraObjectPlot('Alpaca',mainPath+'spatial_pages_exp/alpaca/mg-alpaca-noinline/chat-trace-b32768-p6000000-questions_copy/spatial.txt',3,strMetric='SD-SP-SI', \
    #         listCombineReg=['5-HotIns-11', '6-HotIns-12'], flWeighted=flWeight,affinityOption=3)
    #intraObjectPlot('Alpaca',mainPath+'spatial_pages_exp/alpaca/mg-alpaca-noinline/chat-trace-b32768-p6000000-story-copy/spatial.txt',3,strMetric='SD-SP-SI', \
    #         listCombineReg=['6-HotIns-11', '7-HotIns-12'], flWeighted=flWeight,affinityOption=3)
    #intraObjectPlot('Alpaca',mainPath+'spatial_pages_exp/alpaca/mg-alpaca-noinline/chat-trace-b32768-p6000000-story-noseed-copy/spatial.txt',3,strMetric='SD-SP-SI', \
    #         listCombineReg=['6-HotIns-11', '7-HotIns-12'], flWeighted=flWeight,affinityOption=3)
    #intraObjectPlot('Alpaca',mainPath+'spatial_pages_exp/alpaca/mg-alpaca/chat-trace-b16384-p5000000/spatial.txt',2,strMetric='SD-SP-SI', \
    #         flWeighted=flWeight,affinityOption=3)
    #intraObjectPlot('Alpaca',mainPath+'spatial_pages_exp/alpaca/mg-alpaca-sel/chat-trace-b16384-p5000000/spatial.txt',3,strMetric='SD-SP-SI', \
    #         flWeighted=flWeight,affinityOption=3)
    #intraObjectPlot('Alpaca',mainPath+'spatial_pages_exp/alpaca/mg-alpaca-sel/chat-trace-b16384-p4000000/spatial.txt',3,strMetric='SD-SP-SI', \
    #         flWeighted=flWeight,affinityOption=3)
    #intraObjectPlot('Alpaca',mainPath+'spatial_pages_exp/alpaca/mg-alpaca-sel/chat-trace-b32768-p4000000/spatial.txt',5,strMetric='SD-SP-SI', \
    #         flWeighted=flWeight,affinityOption=3)
    #intraObjectPlot('Alpaca',mainPath+'spatial_pages_exp/alpaca/alpaca-O2/chat-trace-b32768-p6000000-questions/spatial.txt',6,strMetric='SD-SP-SI', \
    #         listCombineReg=['0-A0000000', '1-A0000010','2-A0000011','3-A0000012','4-A0000013'], flWeighted=flWeight,affinityOption=3)
    #intraObjectPlot('Alpaca',mainPath+'spatial_pages_exp/alpaca/alpaca-O2/chat-trace-b32768-p6000000-story/spatial.txt',7,strMetric='SD-SP-SI', \
    #         listCombineReg=['0-A0000000', '1-A0000100','2-A0000101','3-A0000102','4-A0000103','5-A0000104'], flWeighted=flWeight,affinityOption=3)
    #intraObjectPlot('Alpaca',mainPath+'spatial_pages_exp/alpaca/alpaca-O2/chat-trace-b32768-p6000000-story-noseed/spatial.txt',9,strMetric='SD-SP-SI', \
    #         listCombineReg=['0-A0000000', '1-A0000010','2-A0000011','3-A0000012','4-A0000013','5-A0000014'], flWeighted=flWeight,affinityOption=3)


if (0): #Unused
    intraObjectPlot('XSB-NoInl-rd-HIST',mainPath+'spatial_pages_exp/XSBench/memgaze-xs-read-noinline/XSBench-memgaze-trace-b16384-p4000000-hist/spatial.txt', 9, strMetric='SD-SP-SI', \
        listCombineReg=['1-B0000000','2-B0010000','3-B0020000','4-B0030000','5-B0040000','6-B0050000','7-B0060000'] ,flWeighted=flWeight,affinityOption=3)
    intraObjectPlot('XSB-NoInl-rd-EVENT_k0',mainPath+'spatial_pages_exp/XSBench/memgaze-xs-read-noinline/XSBench-memgaze-trace-b16384-p4000000-event-k-0/spatial.txt', 2, strMetric='SD-SP-SI', \
        listCombineReg=['1-B0000000','2-B0010000','3-B0020000','4-B0030000','5-B0040000','6-B0050000','7-B0060000'] ,flWeighted=flWeight,affinityOption=3)
    intraObjectPlot('XSB-NoInl-rd-EVENT_OPT_k1',mainPath+'spatial_pages_exp/XSBench/memgaze-xs-read-noinline/XSBench-memgaze-trace-b16384-p4000000-event-k-1/spatial.txt', 9, strMetric='SD-SP-SI', \
        listCombineReg=['1-B0000000','2-B0010000','3-B0020000','4-B0030000','5-B0040000','6-B0050000','7-B0060000'] ,flWeighted=flWeight,affinityOption=3)

if (0): #Unused
    intraObjectPlot('XSB-AOS-rd-HIST',mainPath+'spatial_pages_exp/XSBench/memgaze-xs-read-code-change/XSBench-memgaze-trace-b16384-p4000000-hist/spatial.txt', 9, strMetric='SD-SP-SI', \
        listCombineReg=['2-B0000','3-B0010','4-B0020','5-B0030','6-B0040','7-B0050','8-B0060'] ,flWeighted=flWeight,affinityOption=3)
    intraObjectPlot('XSB-AOS-rd-EVENT_k0',mainPath+'spatial_pages_exp/XSBench/memgaze-xs-read-code-change/XSBench-memgaze-trace-b16384-p4000000-event-k-0/spatial.txt', 9, strMetric='SD-SP-SI', \
        listCombineReg=['2-B0000','3-B0010','4-B0020','5-B0030','6-B0040','7-B0050','8-B0060'] ,flWeighted=flWeight,affinityOption=3)
    intraObjectPlot('XSB-AOS-rd-EVENT_OPT_k1',mainPath+'spatial_pages_exp/XSBench/memgaze-xs-read-code-change/XSBench-memgaze-trace-b16384-p4000000-event-k-1/spatial.txt', 3, strMetric='SD-SP-SI', \
        flWeighted=flWeight,affinityOption=3)

if (0): # Instrumentation - NOP stops
    #intraObjectPlot('XSB-rd-HIST',mainPath+'spatial_pages_exp/XSBench/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-hist/spatial.txt', 2, strMetric='SD-SP-SI', \
    #     flWeighted=flWeight,affinityOption=3)
    intraObjectPlot('XSB-rd-EVENT_k0',mainPath+'spatial_pages_exp/XSBench/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0/hot_insn-force-1/spatial.txt', 1, strMetric='SD-SP-SI', \
          flWeighted=flWeight,affinityOption=3)
    intraObjectPlot('XSB-rd-EVENT_OPT_k1',mainPath+'spatial_pages_exp/XSBench/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1/spatial.txt', 1, strMetric='SD-SP-SI', \
         flWeighted=flWeight,affinityOption=3)

if (0):
    intraObjectPlot('XSB-rd-EVENT_k0', \
        mainPath+'spatial_pages_exp/XSBench/openmp-threading-noinline/memgaze-xs-sel-gs-2/XSBench-memgaze-trace-b32768-p3000000-event-k-0/hot-insn-rud/spatial.txt', 4, strMetric='SD-SP-SI', \
        listCombineReg=['8-B0060001', '9-B0060002'],flWeighted=flWeight,affinityOption=3)
        #listCombineReg=['ALL'],flWeighted=flWeight,affinityOption=3)
    intraObjectPlot('XSB-rd-EVENT_OPT_k1', \
        mainPath+'spatial_pages_exp/XSBench/openmp-threading-noinline/memgaze-xs-sel-gs-2/XSBench-memgaze-trace-b32768-p3000000-event-k-1/hot-insn-rud/spatial.txt', \
        4, strMetric='SD-SP-SI', \
        listCombineReg=['7-HotIns-02','8-HotIns-01'], flWeighted=flWeight,affinityOption=3)
        #listCombineReg=['ALL'], flWeighted=flWeight,affinityOption=3)


if (0): # Separate source code 391 line - doesnt add any value
    intraObjectPlot('XSB-rd-HIST',mainPath+'spatial_pages_exp/XSBench/memgaze-xs-read/XSBench-memgaze-trace-b8192-p6000000-hist/spatial.txt', 2, strMetric='SD-SP-SI', \
         flWeighted=flWeight,affinityOption=3)
    intraObjectPlot('XSB-rd-EVENT_k0',mainPath+'spatial_pages_exp/XSBench/memgaze-xs-read/XSBench-memgaze-trace-b8192-p6000000-event-k-0/spatial.txt', 3, strMetric='SD-SP-SI', \
          flWeighted=flWeight,affinityOption=3)
    intraObjectPlot('XSB-rd-EVENT_OPT_k1',mainPath+'spatial_pages_exp/XSBench/memgaze-xs-read/XSBench-memgaze-trace-b8192-p6000000-event-k-1/spatial.txt', 4, strMetric='SD-SP-SI', \
         listCombineReg=['2-C0000000','3-C0000001','4-C0000002'], flWeighted=flWeight,affinityOption=3)

if (0): # still useless for composite # latest try for all regions combined # Dec 20
    intraObjectPlot('XSB-rd-EVENT_k0',mainPath+'spatial_pages_exp/XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0/spatial.txt', 6, strMetric='SD-SP-SI', \
    listCombineReg=['4-HotIns-11', '5-HotIns-12'], flWeighted=flWeight,affinityOption=3)
    #listCombineReg=['ALL'], flWeighted=flWeight,affinityOption=3)
    intraObjectPlot('XSB-rd-EVENT_OPT_k1',mainPath+'spatial_pages_exp/XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1/spatial.txt', 3, strMetric='SD-SP-SI', \
    listCombineReg=['1-B0000000','2-B0000001','3-B0000002'], flWeighted=flWeight,affinityOption=3)
    #listCombineReg=['ALL'], flWeighted=flWeight,affinityOption=3)

if (0):  # latest try for all regions combined # Dec 27
    intraObjectPlot('XSB-rd-EVENT_k0',mainPath+'spatial_pages_exp/XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b32768-p5000000-event-k-0/spatial.txt', 7, strMetric='SD-SP-SI', \
    #listCombineReg=['ALL'], flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    listCombineReg=['5-HotIns-11', '6-HotIns-12'], flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('XSB-rd-EVENT_OPT_k1',mainPath+'spatial_pages_exp/XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b32768-p5000000-event-k-1/spatial.txt', 9, strMetric='SD-SP-SI', \
    #listCombineReg=['ALL'], flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    listCombineReg=['5-C0000000','6-C0000001','7-C0000002','8-C0000010'], flWeighted=flWeight,affinityOption=3,flPlot=flPlot)

if (1):  # Jan 8
    intraObjectPlot('XSB-rd-EVENT_k0',mainPath+'spatial_pages_exp/XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0-anlys/spatial.txt', 6, strMetric='SD-SP-SI', \
    #listCombineReg=['4-HotIns-11', '5-HotIns-12'], flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    listCombineReg=[ '2-B0000001','3-B0000002'], flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    #listCombineReg=['0-A0000000', '2-B0000001','3-B0000002','4-HotIns-11', '5-HotIns-12'], flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    #intraObjectPlot('XSB-rd-EVENT_OPT_k1',mainPath+'spatial_pages_exp/XSBench/openmp-noflto/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1-anlys/spatial.txt', 16, strMetric='SD-SP-SI', \
    #listCombineReg=['14-B2000000','15-B2000001'], flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    #listCombineReg=['0-A0000000','12-B0001000','14-B2000000','15-B2000001'], flWeighted=flWeight,affinityOption=3,flPlot=flPlot)

if (0): # works for all combined regions composite plot - Dec 20
    intraObjectPlot('XSB-rd-EVENT_k0', \
        mainPath+'spatial_pages_exp/XSBench/openmp-threading-noinline/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-0/spatial.txt', 10, strMetric='SD-SP-SI', \
        listCombineReg=['ALL'],flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('XSB-rd-EVENT_OPT_k1', \
        mainPath+'spatial_pages_exp/XSBench/openmp-threading-noinline/memgaze-xs-read/XSBench-memgaze-trace-b16384-p4000000-event-k-1/spatial.txt', \
        9, strMetric='SD-SP-SI', \
        listCombineReg=['ALL'], flWeighted=flWeight,affinityOption=3,flPlot=flPlot)


if(0): #Data from bignuke runs
    intraObjectPlot('miniVite-v1',mainPath+'spatial_pages_exp/miniVite/bignuke_run/mini-memgaze-ld/miniVite-v1-memgaze-trace-b16384-p5000000-anlys/spatial.txt',4,strMetric='SD-SP-SI', \
                 listCombineReg=['1-A0001000','2-B0000000','3-B1100000'], flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('miniVite-v2',mainPath+'spatial_pages_exp/miniVite/bignuke_run/mini-memgaze-ld/miniVite-v2-memgaze-trace-b16384-p5000000-anlys/spatial.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['2-A0002000','3-B0000000','4-B1000000'], flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('miniVite-v3',mainPath+'spatial_pages_exp/miniVite/bignuke_run/mini-memgaze-ld/miniVite-v3-memgaze-trace-b16384-p5000000-anlys/spatial.txt',7,strMetric='SD-SP-SI', \
                listCombineReg=['4-A0002000','5-B0000000','6-B1100000'],flWeighted=flWeight,affinityOption=3,flPlot=flPlot)

if(0): # Data from nuke - CLUSTER paper data - v1 - has map polluted by graph
    intraObjectPlot('miniVite-v1',mainPath+'spatial_pages_exp/miniVite/hot_lines/v1_spatial_det.txt',1,strMetric='SD-SP-SI', \
                 flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('miniVite-v2',mainPath+'spatial_pages_exp/miniVite/hot_lines/v2_spatial_det.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['1-A0000010','4-A0002000'] ,flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('miniVite-v3',mainPath+'spatial_pages_exp/miniVite/hot_lines/v3_spatial_det.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['1-A0000001','5-A0001200'] ,flWeighted=flWeight,affinityOption=3,flPlot=flPlot)

if ( 1 == 0):
    intraObjectPlot('ResNet', mainPath+'spatial_pages_exp/Darknet/resnet152_single/hot_lines/spatial.txt',1,strMetric='SD-SP-SI',flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('AlexNet',mainPath+'spatial_pages_exp/Darknet/alexnet_single/hot_lines/spatial.txt',5, \
                    listCombineReg=['5-B1000000','6-B1001000','7-B1010000','8-B1011000'],strMetric='SD-SP-SI',flWeighted=flWeight,affinityOption=3,flPlot=flPlot)

if(0):
    intraObjectPlot('HiParTi - CSR',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/hot_lines/csr/spatial.txt',2,f_avg=f_avg1,strMetric='SD-SP-SI',\
                    flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('HiParTi - COO',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/hot_lines/coo_u_0/spatial.txt',3,f_avg=f_avg1,\
                    listCombineReg=['1-A0000010','2-A0000020'], strMetric='SD-SP-SI', flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('HiParTi - COO-Reduce',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/hot_lines/coo_u_1/spatial.txt', \
                    2,listCombineReg=['0-A0000000', '1-A1000000', '2-A2000000','3-A2000010'],f_avg=f_avg1, strMetric='SD-SP-SI', flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('HiParTi - HiCOO',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/hot_lines/hicoo_u_0/spatial.txt',2,f_avg=f_avg1,strMetric='SD-SP-SI', \
                    flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('HiParTi - HiCOO-Schedule',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/hot_lines/hicoo_u_1/spatial.txt',2,f_avg=f_avg1,strMetric='SD-SP-SI', \
                    flWeighted=flWeight,affinityOption=3,flPlot=flPlot)

if(0):
    intraObjectPlot('HiParTI-HiCOO', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-0-b16384-p4000000-U-0/hot_lines/spatial.txt', 1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('HiParTI-HiCOO-Lexi', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-1-b16384-p4000000-U-0/hot_lines/spatial.txt', 1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('HiParTI-HiCOO-BFS', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-2-b16384-p4000000-U-0/hot_lines/spatial.txt', 1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight,affinityOption=3,flPlot=flPlot)
    intraObjectPlot('HiParTI-HiCOO-Random', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-3-b16384-p4000000-U-0/hot_lines/spatial.txt', 1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight,affinityOption=3,flPlot=flPlot)

if( 1 == 0):
    intraObjectPlot('miniVite-v1',mainPath+'minivite_detailed_look/inter-region/v1_spatial_det.txt',1,strMetric='SD-SP-SI',flWeighted=flWeight)
    intraObjectPlot('miniVite-v2',mainPath+'minivite_detailed_look/inter-region/v2_spatial_det.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['1-A0000010','4-A0002000'] ,flWeighted=flWeight)
    intraObjectPlot('miniVite-v3',mainPath+'minivite_detailed_look/inter-region/v3_spatial_det.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['1-A0000001','5-A0001200'] ,flWeighted=flWeight)
if(0):
    numExtraPages=64
    intraObjectPlot('miniVite-v1',mainPath+'spatial_pages_exp/miniVite/include_all_pages/v1_spatial_det.txt',1,strMetric='SD-SP-SI',\
                    flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=1)
    intraObjectPlot('miniVite-v2',mainPath+'spatial_pages_exp/miniVite/include_all_pages/v2_spatial_det.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['1-A0000010','4-A0002000'] ,flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=1)
    intraObjectPlot('miniVite-v3',mainPath+'spatial_pages_exp/miniVite/include_all_pages/v3_spatial_det.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['1-A0000001','5-A0001200'] ,flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=1)

if(0):
    numExtraPages=64
    intraObjectPlot('miniVite-v1',mainPath+'spatial_pages_exp/miniVite/page_region/v1_spatial_det.txt',1,strMetric='SD-SP-SI',\
                    flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=2)
    intraObjectPlot('miniVite-v2',mainPath+'spatial_pages_exp/miniVite/page_region/v2_spatial_det.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['1-A0000010','4-A0002000'] ,flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=2)
    intraObjectPlot('miniVite-v3',mainPath+'spatial_pages_exp/miniVite/page_region/v3_spatial_det.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['1-A0000001','5-A0001200'] ,flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=2)

if(0):
    numExtraPages=64
    intraObjectPlot('HiParTI-HiCOO', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-0-b16384-p4000000-U-0/include_all_pages/spatial.txt', 1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=1)
    intraObjectPlot('HiParTI-HiCOO-Lexi', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-1-b16384-p4000000-U-0/include_all_pages/spatial.txt', 1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=1)
    intraObjectPlot('HiParTI-HiCOO-BFS', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-2-b16384-p4000000-U-0/include_all_pages/spatial.txt', 1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=1)
    intraObjectPlot('HiParTI-HiCOO-Random', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-3-b16384-p4000000-U-0/include_all_pages/spatial.txt', 1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=1)

if(0):
    numExtraPages=64
    intraObjectPlot('HiParTI-HiCOO', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-0-b16384-p4000000-U-0/pages_region/spatial.txt', 1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=2)
    intraObjectPlot('HiParTI-HiCOO-Lexi', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-1-b16384-p4000000-U-0/pages_region/spatial.txt', 1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=2)
    intraObjectPlot('HiParTI-HiCOO-BFS', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-2-b16384-p4000000-U-0/pages_region/spatial.txt', 1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=2)
    intraObjectPlot('HiParTI-HiCOO-Random', mainPath+'spatial_pages_exp/HICOO-tensor/mttsel-re-3-b16384-p4000000-U-0/pages_region/spatial.txt', 1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=2)


if ( 1 == 0):
    f_avg1=None
    intraObjectPlot('HiParTi - CSR',mainPath+'HiParTi/4096-same-iter/mg-csr/spmm_csr_mat-trace-b8192-p4000000/spatial_affinity_time/spatial.txt',2,f_avg=f_avg1,\
                    strMetric='SD-SP-SI',flWeighted=flWeight)
    intraObjectPlot('HiParTi - COO',mainPath+'HiParTi/4096-same-iter/mg-spmm-mat/spmm_mat-U-0-trace-b8192-p4000000/spatial_affinity_time/spatial.txt',3,f_avg=f_avg1,\
                    listCombineReg=['1-A0000010','2-A0000020'], strMetric='SD-SP-SI', flWeighted=flWeight)
    intraObjectPlot('HiParTi - COO-Reduce',mainPath+'HiParTi/4096-same-iter/mg-spmm-mat/spmm_mat-U-1-trace-b8192-p4000000/spatial_affinity_time/spatial.txt', \
                    2,listCombineReg=['0-A0000000', '1-A1000000', '2-A2000000','3-A2000010'],f_avg=f_avg1, strMetric='SD-SP-SI', flWeighted=flWeight)
    intraObjectPlot('HiParTi - HiCOO',mainPath+'HiParTi/4096-same-iter/mg-spmm-hicoo/spmm_hicoo-U-0-trace-b8192-p4000000/spatial_affinity_time/spatial.txt',2,f_avg=f_avg1,\
                    strMetric='SD-SP-SI', flWeighted=flWeight)
    intraObjectPlot('HiParTi - HiCOO-Schedule',mainPath+'HiParTi/4096-same-iter/mg-spmm-hicoo/spmm_hicoo-U-1-trace-b8192-p4000000/spatial_affinity_time/spatial.txt',3,f_avg=f_avg1,\
                    strMetric='SD-SP-SI', flWeighted=flWeight)

if ( 1 == 0):
    f_avg1=None
    numExtraPages=64
    intraObjectPlot('HiParTi - CSR',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/include_all_pages/csr/spatial.txt',2,f_avg=f_avg1,strMetric='SD-SP-SI',\
                    flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=1)
    intraObjectPlot('HiParTi - COO',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/include_all_pages/coo_u_0/spatial.txt',3,f_avg=f_avg1,\
                    listCombineReg=['1-A0000010','2-A0000020'], strMetric='SD-SP-SI', flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=1)
    intraObjectPlot('HiParTi - COO-Reduce',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/include_all_pages/coo_u_1/spatial.txt', \
                    2,listCombineReg=['0-A0000000', '1-A1000000', '2-A2000000','3-A2000010'],f_avg=f_avg1, strMetric='SD-SP-SI', flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=1)
    intraObjectPlot('HiParTi - HiCOO',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/include_all_pages/hicoo_u_0/spatial.txt',2,f_avg=f_avg1,strMetric='SD-SP-SI', \
                    flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=1)
    intraObjectPlot('HiParTi - HiCOO-Schedule',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/include_all_pages/hicoo_u_1/spatial.txt',2,f_avg=f_avg1,strMetric='SD-SP-SI', \
                    flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=1)

if ( 1 == 0):
    f_avg1=None
    numExtraPages=64
    intraObjectPlot('HiParTi - CSR',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/pages_region/csr/spatial.txt',2,f_avg=f_avg1,strMetric='SD-SP-SI',flWeighted=flWeight, \
                    numExtraPages=numExtraPages,affinityOption=2)
    intraObjectPlot('HiParTi - COO',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/pages_region/coo_u_0/spatial.txt',3,f_avg=f_avg1,\
                    listCombineReg=['1-A0000010','2-A0000020'], strMetric='SD-SP-SI', flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=2)
    intraObjectPlot('HiParTi - COO-Reduce',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/pages_region/coo_u_1/spatial.txt', \
                    2,listCombineReg=['0-A0000000', '1-A1000000', '2-A2000000','3-A2000010'],f_avg=f_avg1, strMetric='SD-SP-SI', flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=2)
    intraObjectPlot('HiParTi - HiCOO',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/pages_region/hicoo_u_0/spatial.txt',2,f_avg=f_avg1,strMetric='SD-SP-SI', \
                    flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=2)
    intraObjectPlot('HiParTi - HiCOO-Schedule',mainPath+'spatial_pages_exp/HICOO-matrix/4096-same-iter/pages_region/hicoo_u_1/spatial.txt',2,f_avg=f_avg1,strMetric='SD-SP-SI', \
                    flWeighted=flWeight,numExtraPages=numExtraPages,affinityOption=2)

#intraObjectPlot('HiParTI-HiCOO', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
#intraObjectPlot('HiParTI-HiCOO-Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
#intraObjectPlot('HiParTI-HiCOO-BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
#intraObjectPlot('HiParTI-HiCOO-Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
