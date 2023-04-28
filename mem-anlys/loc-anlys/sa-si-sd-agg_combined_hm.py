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

    # STEP 2a - Add combine regions to the list
    if (listCombineReg != None):
        arRegionId.extend(x for x in listCombineReg if x not in arRegionId)
    arRegionId.sort()
    #print(arRegionId)

    data_list_combine_Reg =[]
    flagProcessCombine=0

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
            data_list_combine_Reg.extend(data_list_intra_obj)
            data_list_intra_obj=copy.deepcopy(data_list_combine_Reg)
            print('***** after regionIdNumName ', regionIdNumName, 'list data_list_intra_obj length', len(data_list_intra_obj), 'list data_list_combine_Reg', len(data_list_combine_Reg))

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
        print (arRegPages)
        arRegPageAccess = np.empty([len(arRegPages),1])
        for arRegionBlockId in range(0, len(arRegPages)):
            #print(arRegionBlockId, df_intra_obj[df_intra_obj['blockid']==arRegPages[arRegionBlockId]]['Access'].sum())
            arRegPageAccess[int(arRegionBlockId)] = df_intra_obj[df_intra_obj['reg_page_id']==arRegPages[arRegionBlockId]]['Access'].sum()
        #print (arRegPageAccess)

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
        print('before sample - ', df_intra_obj.shape)
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
                arRegPageAccessIndex = arRegPages.tolist().index(blkid[0:blkid.rfind('-')])
                weightRegPageAccess = arRegPageAccess[arRegPageAccessIndex].item() /3
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
                        arRegPageIndex = arRegPages.tolist().index(blkid[0:blkid.rfind('-')])
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
        df_SP_SI_SD=pd.DataFrame(list_SP_SI_SD,columns=list_col_names)
        df_SP_SI_SD=df_SP_SI_SD.dropna(axis=1, how='all')
        # Get rows above self - self+1, self+2...
        if (1 ==0):
            #print(df_SP_SI_SD['reg-page-blk'])
            #print('columns',  df_SP_SI_SD.columns.to_list())
            cols_df_SP_SI_SD = df_SP_SI_SD.columns.to_list()
            cols_df_SP=['SP-self']
            cols_df_SI=['SI-self']
            pattern = re.compile('SP-self\+.*')
            newlist = list(filter(pattern.match, cols_df_SP_SI_SD))
            cols_df_SP.extend(newlist)
            pattern = re.compile('SI-self\+.*')
            newlist = list(filter(pattern.match, cols_df_SP_SI_SD))
            cols_df_SI.extend(newlist)
            #print(cols_df_SP)
            #(cols_df_SI)

        # To show all three in tandem
        # Get rows with more SA entries - non zero SA will result in  SI and maybe SD
        #print(df_SP_SI_SD['reg-page-blk'])
        #print('columns',  df_SP_SI_SD.columns.to_list())

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
                    #df1.loc[df1['stream'] == 2, ['feat','another_feat']] = 'aaaa'
                pattern = re.compile('SP-')
                cols_df_SP = list(filter(pattern.match, cols_df_SP_SI_SD))
                for col_SP_name in cols_df_SP:
                    df_SP_SI_SD.loc[df_SP_SI_SD['reg-page'] == regPageId, [col_SP_name]] = df_SP_SI_SD.loc[df_SP_SI_SD['reg-page'] == regPageId, [col_SP_name]].div(normSPMax[regPageIndex])
                regPageIndex = regPageIndex+1
            print('after normalize  ' , df_SP_SI_SD[['reg-page-blk', 'SD-self', 'SP-self']])
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
            if ( (not (df_SP_SI_SD[col_orig_name].isnull().all())) and (np.sum(df_SP_SI_SD[col_orig_name]>=0.005) > 2)):
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

        print(cols_df_SP)
        print(cols_df_SI)

        if (len(cols_df_SP) > 3 and len(cols_df_SI) >3 and len(cols_df_SD)>3):
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
            gs0 = gridspec.GridSpecFromSubplotSpec(3, 1, subplot_spec = gs[0] ,hspace=0.10)
            gs1 = gridspec.GridSpecFromSubplotSpec(1, 2, subplot_spec = gs[1] ,wspace=0.07)
            gs0_ax_0 = fig.add_subplot(gs0[0, 0])
            gs0_ax_1 = fig.add_subplot(gs0[1, 0])
            gs0_ax_2 = fig.add_subplot(gs0[2, 0])
            ax_2 = fig.add_subplot(gs1[0, 0])
            ax_3 = fig.add_subplot(gs1[0, 1])

            gs0_ax_0 = sns.heatmap(df_hm_SD, cmap='mako_r',cbar=True, annot=False,ax=gs0_ax_0,vmin=0.0,vmax=1.0)
            gs0_ax_1 = sns.heatmap(df_hm_SP, cmap='mako_r',cbar=True, annot=False,ax=gs0_ax_1,vmin=0.0, vmax=1.0)
            if ('minivite' in strApp.lower()):
                vmin = 0
                vmax= 20
            else:
                vmin=0
                vmax=10
            gs0_ax_2 = sns.heatmap(df_hm_SI, cmap='mako',cbar=True, annot=False,ax=gs0_ax_2,vmin=vmin, vmax=vmax)
            gs0_ax_0.set(ylabel="Spatial Density")
            gs0_ax_1.set(ylabel="Spatial Anticipation")
            gs0_ax_2.set(xlabel="Region-Page-Block", ylabel="Spatial Interval")

            gs0_ax_0.invert_yaxis()
            list_y_ticks=gs0_ax_0.get_yticklabels()
            fig_ylabel=[]
            for y_label in list_y_ticks:
                fig_ylabel.append(cols_df_SD[int(y_label.get_text())][3:])
            list_x_ticks=gs0_ax_0.get_xticklabels()
            fig_xlabel=[]
            for x_label in list_x_ticks:
                fig_xlabel.append(df_SP_SI_SD['reg-page-blk'][int(x_label.get_text())])
            gs0_ax_0.set_yticklabels(fig_ylabel,rotation='horizontal')
            #gs0_ax_0.set_xticklabels(fig_xlabel,rotation='vertical')
            gs0_ax_0.set_xticklabels([])

            gs0_ax_1.invert_yaxis()
            list_y_ticks=gs0_ax_1.get_yticklabels()
            fig_ylabel=[]
            for y_label in list_y_ticks:
                fig_ylabel.append(cols_df_SP[int(y_label.get_text())][3:])
            list_x_ticks=gs0_ax_1.get_xticklabels()
            fig_xlabel=[]
            for x_label in list_x_ticks:
                fig_xlabel.append(df_SP_SI_SD['reg-page-blk'][int(x_label.get_text())])
            gs0_ax_1.set_yticklabels(fig_ylabel,rotation='horizontal')
            #gs0_ax_1.set_xticklabels(fig_xlabel,rotation='vertical')
            gs0_ax_1.set_xticklabels([])

            gs0_ax_2.invert_yaxis()
            list_y_ticks=gs0_ax_2.get_yticklabels()
            fig_ylabel=[]
            for y_label in list_y_ticks:
                fig_ylabel.append(cols_df_SI[int(y_label.get_text())][3:])
            list_x_ticks=gs0_ax_2.get_xticklabels()
            fig_xlabel=[]
            for x_label in list_x_ticks:
                fig_xlabel.append(df_SP_SI_SD['reg-page-blk'][int(x_label.get_text())])
            gs0_ax_2.set_yticklabels(fig_ylabel,rotation='horizontal')
            gs0_ax_2.set_xticklabels(fig_xlabel,rotation='vertical')

            list_xlabel=df_SP_SI_SD['reg-page-blk']
            sns.heatmap(accessBlockCacheLine, cmap="PuBu", cbar=False,annot=True, fmt='g', annot_kws = {'size':12},  ax=ax_2)
            ax_2.invert_yaxis()
            ax_2.set_xticks([0])
            length_xlabel= len(list_xlabel)
            list_blkcache_label=[]
            for i in range (0, length_xlabel):
                list_blkcache_label.append(list_xlabel[i])
            ax2_ylabel=[]
            list_y_ticks=ax_2.get_yticklabels()
            for y_label in list_y_ticks:
                ax2_ylabel.append(list_blkcache_label[int(y_label.get_text())])
            ax_2.set_yticks(range(0,len(list_blkcache_label)),list_blkcache_label, rotation='horizontal', wrap=True )
            ax_2.yaxis.set_ticks_position('right')

            # All three metrics are in here, access includes all three, so divide by 3000 instead of 1000
            rndAccess = np.round((arRegPageAccess/3000).astype(float),2)
            annot = np.char.add(rndAccess.astype(str), 'K')
            sns.heatmap(rndAccess, cmap='Blues', cbar=False,annot=annot, fmt='', annot_kws = {'size':12},  ax=ax_3)
            ax_3.invert_yaxis()
            ax_3.set_xticks([0])
            list_blk_label=arRegPages.tolist()
            ax_3.set_yticklabels(list_blk_label, rotation='horizontal', wrap=True )
            ax_3.yaxis.set_ticks_position('right')

            strMetricTitle = 'Spatial Density, Anticipation, Interval'
            plt.suptitle(strMetricTitle +' and Access heatmap for '+strApp +' region - '+regionIdNumName)
            strArRegionAccess = str(np.round((arRegionAccess/1000).astype(float),2))+'K'
            strAccessSumBlocks= str(np.round((accessSumBlocks/1000).astype(float),2))+'K'
            strTitle = 'Region\'s access - ' + strArRegionAccess + ', Access count for selected pages - ' + strAccessSumBlocks +' ('+ ("{0:.1f}".format((accessSumBlocks/arRegionAccess)*100)) \
                       +'%), Number of pages in region - '+ str(numRegionBlocks) + '\n Spatial Density'
            #print(strTitle)
            gs0_ax_0.set_title(strTitle)
            gs0_ax_1.set_title('Spatial Anticipation')
            gs0_ax_2.set_title('Spatial Interval')
            ax_2.set_title('Access count for selected blocks and \n         hottest pages in the region',loc='left')

            imageFileName=strPath+'/'+strApp.replace(' ','')+'-'+regionIdNumName.replace(' ','').replace('&','-')+'-'+strMetric+'_hm.pdf'
            plt.savefig(imageFileName, bbox_inches='tight')
            plt.close()
            #plt.show()

#intraObjectPlot('miniVite-v3','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v3_spatial_det.txt',3,strMetric='SD-SP-SI-W', \
#                listCombineReg=['1-A0000001','5-A0001200'] ,flWeighted=True)

if(1 == 0):
    intraObjectPlot('miniVite-v1','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v1_spatial_det.txt',1,strMetric='SD-SP-SI')
    intraObjectPlot('miniVite-v2','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v2_spatial_det.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['1-A0000010','4-A0002000'] )
    intraObjectPlot('miniVite-v3','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v3_spatial_det.txt',3,strMetric='SD-SP-SI', \
                listCombineReg=['1-A0000001','5-A0001200'] )
if( 1 == 1):
    intraObjectPlot('miniVite-v1','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v1_spatial_det.txt',1,strMetric='SD-SP-SI-W',flWeighted=True)
    intraObjectPlot('miniVite-v2','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v2_spatial_det.txt',3,strMetric='SD-SP-SI-W', \
                listCombineReg=['1-A0000010','4-A0002000'] ,flWeighted=True)
    intraObjectPlot('miniVite-v3','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v3_spatial_det.txt',3,strMetric='SD-SP-SI-W', \
                listCombineReg=['1-A0000001','5-A0001200'] ,flWeighted=True)

if (1 ==0):
    intraObjectPlot('HiParTI-HiCOO', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SD-SP-SI')
    intraObjectPlot('HiParTI-HiCOO-Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SD-SP-SI')
    intraObjectPlot('HiParTI-HiCOO-BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SD-SP-SI')
    intraObjectPlot('HiParTI-HiCOO-Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SD-SP-SI')

if (1 ==1):
    intraObjectPlot('HiParTI-HiCOO', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SD-SP-SI-W',flWeighted=True)
    intraObjectPlot('HiParTI-HiCOO-Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SD-SP-SI-W',flWeighted=True)
    intraObjectPlot('HiParTI-HiCOO-BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SD-SP-SI-W',flWeighted=True)
    intraObjectPlot('HiParTI-HiCOO-Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SD-SP-SI-W',flWeighted=True)

#intraObjectPlot('HiParTI-HiCOO', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
#intraObjectPlot('HiParTI-HiCOO-Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
#intraObjectPlot('HiParTI-HiCOO-BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
#intraObjectPlot('HiParTI-HiCOO-Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
