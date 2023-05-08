# Create intra-region SP and SI combined heatmap for selected blocks
# Only self, self+1 and ... self-* non-significant for SP & SI
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

sns.color_palette("light:#5A9", as_cmap=True)
sns.set()



def get_intra_obj (data_intra_obj, fileline,reg_page_id,regionIdNum):
    #print(reg_page_id)
    add_row=[None]*517
    data = fileline.strip().split(' ')
    #print("in fill_data_frame", data[2], data[4], data[15:])
    str_index=regionIdNum+'-'+data[2][-1]+'-'+data[4] #regionid-pageid-cacheline
    add_row[0]=reg_page_id
    add_row[1]=str_index
    add_row[2] = data[11] # access count
    add_row[3]=data[9] # lifetime
    add_row[4]=data[7] # Address range
    linecache = int(data[4])
    # Added value for 'self' so that it doesnt get dropped in dropna
    add_row[260]=0.0
    for i in range(15,len(data)):
        cor_data=data[i].split(',')
        if(int(cor_data[0])<=linecache):
            add_row_index=5+255-(linecache-int(cor_data[0]))
            #print("if ", linecache, cor_data[0], 5+255-(linecache-int(cor_data[0])))
        else:
            add_row_index=5+255+(int(cor_data[0])-linecache)
            #print("else", linecache, cor_data[0], 5+255+(int(cor_data[0])-linecache))
        add_row[add_row_index]=cor_data[1]
        #if(add_row_index == 260 ):
            #print('address', add_row[4], 'index', add_row_index, ' value ', add_row[add_row_index])
    if(data[0] == '==='):
        add_row[516] = 'SP'
    elif(data[0] == '---'):
        add_row[516] = 'SI'
    elif(data[0] == '***'):
        add_row[516] = 'SD'
    data_intra_obj.append(add_row)

# Works for spatial denity, Spatial Probability and Proximity
def intraObjectPlot(strApp, strFileName,numRegion, strMetric=None, f_avg=None,listCombineReg=None,flWeighted=None):
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
    #print(df_inter_data)
    df_inter_data_sample=df_inter_data.nlargest(n=numRegion,  columns=['Access count'])
    arRegionId = df_inter_data_sample['Reg_Num-Name'].values.flatten().tolist()
    print(arRegionId)
    # STEP 2a - Add combine regions to the list
    if (listCombineReg != None):
        arRegionId.extend(x for x in listCombineReg if x not in arRegionId)
    arRegionId.sort()
    print(arRegionId)

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
        with open(strFileName) as f:
            for fileLine in f:
                data=fileLine.strip().split(' ')
                if ((data[0] in lineStartList) and (data[2][0:len(data[2])-1]) == regionIdName):
                    pageId=data[2][-1]
                    #print('region line' , regionIdNumName, pageId, data[2])
                    get_intra_obj(data_list_intra_obj,fileLine,regionIdNum+'-'+pageId,regionIdNum)
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
        list_col_names=[None]*517
        list_col_names[0]='reg_page_id'
        list_col_names[1]='reg-page-blk'
        list_col_names[2]='Access'
        list_col_names[3]='Lifetime'
        list_col_names[4]='Address'
        for i in range ( 0,255):
            list_col_names[5+i]='self'+'-'+str(255-i)
        list_col_names[260]='self'
        for i in range ( 1,256):
            list_col_names[260+i]='self'+'+'+str(i)
        list_col_names[516]='Type'
        #print((list_col_names))
        df_intra_obj=pd.DataFrame(data_list_intra_obj,columns=list_col_names)
        #print('columns ', df_intra_obj.columns.to_list())
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
        df_reg_pages.sort_values(by=['Access'],ascending=False,inplace=True)
        arRegPageAccess=(df_reg_pages[['Access']]).to_numpy()
        arRegPages=df_reg_pages['reg-page'].to_list()

        get_col_list=[None]*512
        plot_SP_col=[None]*511
        for i in range ( 0,255):
            get_col_list[i]='self'+'-'+str(255-i)
            plot_SP_col[i]='self'+'-'+str(255-i)
        get_col_list[255]='self'
        plot_SP_col[255]='self'
        for i in range ( 1,256):
            get_col_list[255+i]='self'+'+'+str(i)
            plot_SP_col[255+i]='self'+'+'+str(i)
        get_col_list[511]='Type'
        # Change data to numeric
        for i in range (0,len(get_col_list)-1):
            df_intra_obj[get_col_list[i]]=pd.to_numeric(df_intra_obj[get_col_list[i]])

        df_intra_obj.set_index('reg-page-blk')
        df_intra_obj.sort_index(inplace=True)
        self_bef_drop=df_intra_obj['self'].to_list()
        # STEP 3d - drop columns with "all" NaN values
        # DROP - columns with no values
        print('before replace drop - ', df_intra_obj.shape)
        listColPlotRange=['self-1','self+1']
        #print(df_intra_obj.columns.difference(listColPlotRange))
        df_intra_obj_drop=df_intra_obj.dropna(axis=1,how='all')
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
        df_intra_obj_sample_SP=df_intra_obj[(df_intra_obj['Type'] == 'SP')].nlargest(num_sample, 'Access')
        df_intra_obj_sample_SP.set_index('reg-page-blk')
        df_intra_obj_sample_SP.sort_index(inplace=True)
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
        #print('SP cols ', df_intra_obj_SP.columns)
        #print('SI cols ', df_intra_obj_SI.columns)
        print('after drop SP - ', df_intra_obj_SP.shape)
        print('after drop SI - ', df_intra_obj_SI.shape)
        print('after drop SD - ', df_intra_obj_SD.shape)
        flNormalize=False
        normSDMax=np.ones(len(arRegPages))
        normSPMax = np.ones(len(arRegPages))
        #print(plot_SP_col)
        list_SP_SI_SD=[[None]*(3*len(plot_SP_col)+2) for i in range(len(list_xlabel))]

        list_col_names=['reg-page-blk','reg-page']
        for blkCnt in range(0,len(list_xlabel)):
            blkid= list_xlabel[blkCnt]
            list_SP_SI_SD[blkCnt][0]=blkid
            list_SP_SI_SD[blkCnt][1]=blkid[0:blkid.rfind('-')]
            weight_multiplier = 1.0
            if (flWeighted == True):
                #print('blk ', blkid, ' access ', df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item(), ' accessSumBlocks ', accessSumBlocks)
                #print('Before weight ', df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item())
                weightRegPageAccess = (df_reg_pages[(df_reg_pages['reg-page'] == blkid[0:blkid.rfind('-')])]['Access'].item())
                #print(blkid, arRegPageAccessIndex, weightRegPageAccess)
                # Makes everything closer to zero
                # Shouldnt the weight be based on the page access as all the metrics are calculated within the page
                #weight_multiplier = ( df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item() *100 )/ (accessSumBlocks)
                # Weight by the page's total access
                weight_multiplier = ( df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item() *100 )/ (weightRegPageAccess)
                #if (weight_multiplier > 1.0):
                    #print('blk ', blkid, ' access ', df_intra_obj_SP[(df_intra_obj_SP['reg-page-blk'] == blkid)]['Access'].item(), ' accessSumBlocks ', accessSumBlocks)
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
                    if resultSP.size > 0:
                        list_SP_SI_SD[blkCnt][plotCnt*3+2]=round((resultSP.values[0]* weight_multiplier),2)
                    if resultSI.size >0:
                        list_SP_SI_SD[blkCnt][plotCnt*3+3]=resultSI.values[0]
                    if resultSD.size >0:
                        list_SP_SI_SD[blkCnt][plotCnt*3+4]=round(resultSD.values[0]* weight_multiplier,2)
                    #if (weight_multiplier > 1.0 and resultSP.size > 0 and resultSI.size >0 and resultSD.size >0):
                    #    print( 'SP ', list_SP_SI_SD[blkCnt][plotCnt*3+1], ' SI ', list_SP_SI_SD[blkCnt][plotCnt*3+2], ' SD ', list_SP_SI_SD[blkCnt][plotCnt*3+3])
                    if ((list_SP_SI_SD[blkCnt][plotCnt*3+2] > 1.0) or (list_SP_SI_SD[blkCnt][plotCnt*3+4]> 1.0)):
                        #print('greater than 1 - column name ', plotCol, ' SA ',list_SP_SI_SD[blkCnt][plotCnt*3+1], ' SD ',  list_SP_SI_SD[blkCnt][plotCnt*3+3])
                        arRegPageIndex = arRegPages.index(blkid[0:blkid.rfind('-')])
                        if (normSDMax[arRegPageIndex] < list_SP_SI_SD[blkCnt][plotCnt*3+4]):
                            normSDMax[arRegPageIndex] = list_SP_SI_SD[blkCnt][plotCnt*3+4]
                        if( normSPMax[arRegPageIndex] < list_SP_SI_SD[blkCnt][plotCnt*3+2]):
                            normSPMax[arRegPageIndex] = list_SP_SI_SD[blkCnt][plotCnt*3+2]
                        flNormalize = True

        #print(list_SP_SI)

        for plotCol in plot_SP_col:
            list_col_names.append('SP-'+plotCol)
            list_col_names.append('SI-'+plotCol)
            list_col_names.append('SD-'+plotCol)

        print(normSPMax, normSDMax)
        listColPlotRange=['SP-self-1', 'SP-self+1', 'SD-self-1', 'SD-self+1', 'SI-self-1', 'SI-self+1']
        df_SP_SI_SD=pd.DataFrame(list_SP_SI_SD,columns=list_col_names)

        df_SP_SI_SD=df_SP_SI_SD.dropna(axis=1, how='all')

       # To show all three in tandem
        # Get rows with more SA entries - non zero SA will result in  SI and maybe SD
        #print(df_SP_SI_SD['reg-page-blk'])
        #print('columns',  df_SP_SI_SD.columns.to_list())
        cols_df_SP_SI_SD = df_SP_SI_SD.columns.to_list()
        if (flNormalize == True):
            print ('Normalize true SD max ', normSDMax, ' SP ', normSPMax)
            #print('before normalize  ' , df_SP_SI_SD[['reg-page-blk', 'SD-self', 'SP-self']])
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
            #print('after normalize  ' , df_SP_SI_SD[['reg-page-blk', 'SD-self', 'SP-self']])
        cols_df_SI=[]
        cols_df_SP=[]
        cols_df_SD=[]
        # Filter SP, SI and SD to show in heatmap
        pattern = re.compile('SD-self')
        cols_df_orig = list(filter(pattern.match, cols_df_SP_SI_SD))
        for col_orig_name in cols_df_orig:
            str_col_orig_name = str(col_orig_name)
            #print('column name', str_col_orig_name, df_SP_SI_SD[col_orig_name])
            #print(df_SP_SI_SD[col_orig_name].isnull().all())
            if ( (not (df_SP_SI_SD[col_orig_name].isnull().all())) and (np.sum(df_SP_SI_SD[col_orig_name]>=0.01) > 2)):
                #print(' return value ', np.sum(df_SP_SI_SD[col_orig_name]>=0.01))
                #print(col_orig_name)
                col_SP_name='SP-'+str_col_orig_name[3:]
                col_SI_name='SI-'+str_col_orig_name[3:]
                col_SD_name='SD-'+str_col_orig_name[3:]
                #print( 'col_SI_name ', col_SI_name, ' col_SD_name ', col_SD_name)
                cols_df_SP.append(col_SP_name)
                cols_df_SI.append(col_SI_name)
                cols_df_SD.append(col_SD_name)


        if (len(cols_df_SP) < 15):
            cols_df_SI=[]
            cols_df_SP=[]
            cols_df_SD=[]
            # Filter SP, SI and SD to show in heatmap
            pattern = re.compile('SD-self')
            cols_df_orig = list(filter(pattern.match, cols_df_SP_SI_SD))
            for col_orig_name in cols_df_orig:
                str_col_orig_name = str(col_orig_name)
                if ( (not (df_SP_SI_SD[col_orig_name].isnull().all())) and (np.sum(df_SP_SI_SD[col_orig_name]>=0.005) > 1)):
                    #print(col_orig_name)
                    col_SP_name='SP-'+str_col_orig_name[3:]
                    col_SI_name='SI-'+str_col_orig_name[3:]
                    col_SD_name='SD-'+str_col_orig_name[3:]
                    #print( 'col_SI_name ', col_SI_name, ' col_SD_name ', col_SD_name)
                    cols_df_SP.append(col_SP_name)
                    cols_df_SI.append(col_SI_name)
                    cols_df_SD.append(col_SD_name)

        # Add columns so that self is not the only one shown
            pattern = re.compile('.*self-')
            add_cols = list(filter(pattern.match, cols_df_SP))
            if(len(add_cols) ==0):
                cols_df_SP.insert(0,'SP-self-2')
                cols_df_SP.insert(1,'SP-self-1')
                df_SP_SI_SD['SP-self-2']=np.NaN
                df_SP_SI_SD['SP-self-1']=np.NaN
            add_cols = list(filter(pattern.match, cols_df_SD))
            if(len(add_cols) ==0):
                cols_df_SD.insert(0,'SD-self-2')
                cols_df_SD.insert(1,'SD-self-1')
                df_SP_SI_SD['SD-self-2']=np.NaN
                df_SP_SI_SD['SD-self-1']=np.NaN
            add_cols = list(filter(pattern.match, cols_df_SI))
            if(len(add_cols) ==0):
                cols_df_SI.insert(0,'SI-self-2')
                cols_df_SI.insert(1,'SI-self-1')
                df_SP_SI_SD['SI-self-2']=np.NaN
                df_SP_SI_SD['SI-self-1']=np.NaN
            pattern = re.compile('.*self\+.*')
            print(cols_df_SP)
            add_cols = list(filter(pattern.match, cols_df_SP))
            print(add_cols)
            print(len(add_cols))
            if(len(add_cols) ==0):
                cols_df_SP.append('SP-self+1')
                cols_df_SP.append('SP-self+2')
                df_SP_SI_SD['SP-self+1']=np.NaN
                df_SP_SI_SD['SP-self+2']=np.NaN
            add_cols = list(filter(pattern.match, cols_df_SD))
            if(len(add_cols) ==0):
                cols_df_SD.append('SD-self+1')
                cols_df_SD.append('SD-self+2')
                df_SP_SI_SD['SD-self+1']=np.NaN
                df_SP_SI_SD['SD-self+2']=np.NaN
            add_cols = list(filter(pattern.match, cols_df_SI))
            if(len(add_cols) ==0):
                cols_df_SI.append('SI-self+1')
                cols_df_SI.append('SI-self+2')
                df_SP_SI_SD['SI-self+1']=np.NaN
                df_SP_SI_SD['SI-self+2']=np.NaN


        #print(cols_df_SP)
        #print(cols_df_SI)

        if (len(cols_df_SP) > 0 and len(cols_df_SI) >0 and len(cols_df_SD)>0):
            df_intra_obj_sample_hm_SP=df_SP_SI_SD[cols_df_SP]
            df_intra_obj_sample_hm_SI=df_SP_SI_SD[cols_df_SI]
            df_intra_obj_sample_hm_SD=df_SP_SI_SD[cols_df_SD]

            df_intra_obj_sample_hm_SP.apply(pd.to_numeric)
            df_hm_SP=np.empty([num_sample,len(cols_df_SP)])
            df_hm_SP=df_intra_obj_sample_hm_SP.to_numpy()
            df_hm_SP=df_hm_SP.astype('float64')
            df_hm_SP=df_hm_SP.transpose()
            print('df_hm_SP shape', df_hm_SP.shape)

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
            print('df_hm_SP shape', df_hm_SP.shape)

            fig = plt.figure(constrained_layout=True, figsize=(15, 10))
            gs = gridspec.GridSpec(1, 2, figure=fig,width_ratios=[12,3])
            gsleft = gridspec.GridSpecFromSubplotSpec(3, 1, subplot_spec = gs[0] ,hspace=0.02)
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
            gsleft_ax_1 = sns.heatmap(df_hm_SP, cmap='mako_r',cbar=True, cbar_kws={"pad":0.02}, annot=False,ax=gsleft_ax_1,vmin=0.02, vmax=1.0)
            if ('minivite' in strApp.lower()):
                vmin = 0
                vmax= 20
            else:
                vmin=0
                vmax=20
            gsleft_ax_2 = sns.heatmap(df_hm_SI, cmap='mako',cbar=True, cbar_kws={"pad":0.02}, annot=False,ax=gsleft_ax_2,vmin=vmin, vmax=vmax)
            gsleft_ax_0.set_ylabel("SD",fontsize=16,style='italic')
            gsleft_ax_1.set_ylabel("SA",fontsize=16,style='italic')
            gsleft_ax_2.set(xlabel="Region-Page-Block")
            gsleft_ax_2.set_ylabel("SI", fontsize=16,style='italic')

            gsleft_ax_0.invert_yaxis()
            list_y_ticks=gsleft_ax_0.get_yticklabels()
            fig_ylabel=[]
            for y_label in list_y_ticks:
                fig_ylabel.append(cols_df_SD[int(y_label.get_text())][7:])
            list_x_ticks=gsleft_ax_0.get_xticklabels()
            fig_xlabel=[]
            for x_label in list_x_ticks:
                fig_xlabel.append(df_SP_SI_SD['reg-page-blk'][int(x_label.get_text())])
            gsleft_ax_0.set_yticklabels(fig_ylabel,rotation='horizontal')
            #gsleft_ax_0.set_xticklabels(fig_xlabel,rotation='vertical')
            gsleft_ax_0.set_xticklabels([])
            # Colors for Access heatmap, and labels for affinity matrix
            custom_blue = sns.light_palette("#79C")
            background_color = mpl.colormaps["Blues"]

            gsleft_ax_1.invert_yaxis()
            list_y_ticks=gsleft_ax_1.get_yticklabels()
            fig_ylabel=[]
            for y_label in list_y_ticks:
                fig_ylabel.append(cols_df_SP[int(y_label.get_text())][7:])
            list_x_ticks=gsleft_ax_1.get_xticklabels()
            fig_xlabel=[]
            for x_label in list_x_ticks:
                fig_xlabel.append(df_SP_SI_SD['reg-page-blk'][int(x_label.get_text())])
            gsleft_ax_1.set_yticklabels(fig_ylabel,rotation='horizontal')
            #gsleft_ax_1.set_xticklabels(fig_xlabel,rotation='vertical')
            gsleft_ax_1.set_xticklabels([])

            gsleft_ax_2.invert_yaxis()
            list_y_ticks=gsleft_ax_2.get_yticklabels()
            fig_ylabel=[]
            for y_label in list_y_ticks:
                fig_ylabel.append(cols_df_SI[int(y_label.get_text())][7:])
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
            i=0
            for ticklabel, tickcolor in zip(gsleft_ax_2.get_xticklabels(), my_colors):
                if(color_xlabel[i]>0.5):
                    ticklabel.set_color('white')
                ticklabel.set_backgroundcolor(tickcolor)
                ticklabel.set_fontsize(10)
                i = i+1


            list_xlabel=df_SP_SI_SD['reg-page-blk']
            sns.heatmap(accessBlockCacheLine, cmap='Blues', cbar=False,annot=True, fmt='g', annot_kws = {'size':12},  ax=ax_2)
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

            # All three metrics are in here, access includes all three, so divide by 3000 instead of 1000
            rndAccess = np.round((arRegPageAccess/1000).astype(float),2)
            annot = np.char.add(rndAccess.astype(str), 'K')
            sns.heatmap(rndAccess, cmap=custom_blue, cbar=False,annot=annot, fmt='', annot_kws = {'size':12},  ax=ax_3)
            ax_3.set_xticks([0])
            ax_3.set_yticklabels(arRegPages, rotation='horizontal', wrap=True )
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
            fileNameLastSeg = '_hm_order.pdf'
            if (flWeight == True):
                fileNameLastSeg = '_hm_order_Wgt.pdf'
            imageFileName=strPath+'/'+strApp.replace(' ','')+'-'+regionIdNumName.replace(' ','').replace('&','-')+'-'+strMetric+fileNameLastSeg
            print(imageFileName)
            #plt.show()
            plt.savefig(imageFileName, bbox_inches='tight')
            plt.close()


#intraObjectPlot('miniVite-v3','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v3_spatial_det.txt',3,strMetric='SD-SP-SI-W', \
#                listCombineReg=['1-A0000001','5-A0001200'] ,flWeighted=True)

flWeight=True
mainPath='/Users/suri836/Projects/spatial_rud/'
#intraObjectPlot('miniVite-v3',mainPath+'minivite_detailed_look/inter-region/v3_spatial_det.txt',2,strMetric='SD-SP-SI', \
#                listCombineReg=['1-A0000001','5-A0001200'] ,flWeighted=flWeight)
f_avg1=None
#intraObjectPlot('HiParTi - CSR',mainPath+'HiParTi/4096-same-iter/mg-csr/spmm_csr_mat-trace-b8192-p4000000/spatial_affinity/spatial.txt',2,f_avg=f_avg1,strMetric='SD-SP-SI',flWeighted=flWeight)

if( 1 == 0):
    intraObjectPlot('miniVite-v1',mainPath+'minivite_detailed_look/inter-region/v1_spatial_det.txt',1,strMetric='SD-SP-SI',flWeighted=flWeight)
    intraObjectPlot('miniVite-v2',mainPath+'minivite_detailed_look/inter-region/v2_spatial_det.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['1-A0000010','4-A0002000'] ,flWeighted=flWeight)
    intraObjectPlot('miniVite-v3',mainPath+'minivite_detailed_look/inter-region/v3_spatial_det.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['1-A0000001','5-A0001200'] ,flWeighted=flWeight)

if (1 ==0):
    intraObjectPlot('HiParTI-HiCOO', mainPath+'HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SD-SP-SI',flWeighted=flWeight)
    intraObjectPlot('HiParTI-HiCOO-Lexi', mainPath+'HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SD-SP-SI',flWeighted=flWeight)
    intraObjectPlot('HiParTI-HiCOO-BFS', mainPath+'HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SD-SP-SI',flWeighted=flWeight)
    intraObjectPlot('HiParTI-HiCOO-Random', mainPath+'HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SD-SP-SI',flWeighted=flWeight)

if ( 1 == 0):
    intraObjectPlot('ResNet', mainPath+'darknet_cluster/resnet152_single/spatial_affinity/spatial.txt',1,strMetric='SD-SP-SI',flWeighted=flWeight)
    intraObjectPlot('AlexNet',mainPath+'darknet_cluster/alexnet_single/spatial_affinity/spatial.txt',5, \
                    listCombineReg=['5-B1000000','6-B1001000','7-B1010000','8-B1011000'],strMetric='SD-SP-SI',flWeighted=flWeight)

if ( 1 == 1):
    f_avg1=None
    intraObjectPlot('HiParTi - CSR',mainPath+'HiParTi/4096-same-iter/mg-csr/spmm_csr_mat-trace-b8192-p4000000/spatial_affinity/spatial.txt',2,f_avg=f_avg1,strMetric='SD-SP-SI',flWeighted=flWeight)
    intraObjectPlot('HiParTi - COO',mainPath+'HiParTi/4096-same-iter/mg-spmm-mat/spmm_mat-U-0-trace-b8192-p4000000/spatial_affinity/spatial.txt',3,f_avg=f_avg1,\
                    listCombineReg=['1-A0000010','2-A0000020'], strMetric='SD-SP-SI', flWeighted=flWeight)
    intraObjectPlot('HiParTi - COO-Reduce',mainPath+'HiParTi/4096-same-iter/mg-spmm-mat/spmm_mat-U-1-trace-b8192-p4000000/spatial_affinity/spatial.txt', \
                    2,listCombineReg=['0-A0000000', '1-A1000000', '2-A2000000','3-A2000010'],f_avg=f_avg1, strMetric='SD-SP-SI', flWeighted=flWeight)
    intraObjectPlot('HiParTi - HiCOO',mainPath+'HiParTi/4096-same-iter/mg-spmm-hicoo/spmm_hicoo-U-0-trace-b8192-p4000000/spatial_affinity/spatial.txt',2,f_avg=f_avg1,strMetric='SD-SP-SI', flWeighted=flWeight)
    intraObjectPlot('HiParTi - HiCOO-Schedule',mainPath+'HiParTi/4096-same-iter/mg-spmm-hicoo/spmm_hicoo-U-1-trace-b8192-p4000000/spatial_affinity/spatial.txt',3,f_avg=f_avg1,strMetric='SD-SP-SI', flWeighted=flWeight)

#intraObjectPlot('HiParTI-HiCOO', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
#intraObjectPlot('HiParTI-HiCOO-Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
#intraObjectPlot('HiParTI-HiCOO-BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
#intraObjectPlot('HiParTI-HiCOO-Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
