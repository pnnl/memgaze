# Create intra-region heatmaps for Spatial affinity metrics
# Creates self-*, self, self+* for single metric
    # STEP 1 - Read spatial.txt and write inter-region file
    # STEP 2 - Get region ID's from inter-region file
    # STEP 2a - Add combine regions to the list
    # STEP 3 - Loop through regions in the list and plot heatmaps
        # STEP 3a - Combine regions if there are any - incremental heatmaps are written out as of March 2, 2023
        # STEP 3b - Read original spatial data input file to gather the pages-blocks in the region to a list
        # STEP 3b - Convert list to data frame
        # STEP 3c - Process data frame to get access, lifetime totals
        # STEP 3d - Get average of self before sampling for highest access blocks
        # STEP 3e - Range and Count mean, standard deviation to understand the original spread of heatmap
        # STEP 3e - Range and Count mean, calculate before sampling
        # STEP 3f - Sample dataframe for 50 rows based on access count as the weight
        # STEP 3g - drop columns with "all" NaN values
        # STEP 3g - add some columns above & below self line to visualize better
        # STEP 3h - get columns that are useful
        # STEP 3i - Start Heatmap Visualization

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

#STEP 0 - Heatmap for inter-region - function not used
def readFile(filename, strApp):
    df=pd.read_table(filename, sep=" ", skipinitialspace=True, usecols=range(4,14),
        names=['RegionId','colon', 'ar', 'Address Range', 'lf', 'Lifetime', 'ac', 'Access count', 'bc', 'Block count'])
    df1=df[['RegionId', 'Address Range', 'Lifetime', 'Access count', 'Block count']]
    npRegionId = df['RegionId'].values.flatten()
    arRegionId = npRegionId.tolist()
    #print(arRegionId)
    listNumRegion = []
    for i in range(0, len(arRegionId)):
        listNumRegion.append('sp_'+str(arRegionId[i]))
        df1['sp_'+str(arRegionId[i])] =0
    #print (listNumRegion)
    with open(filename) as f:
        indexCnt =0
        for fileLine in f:
            listSpatialDensity=np.empty(len(arRegionId))
            listSpatialDensity.fill(0)
            data = fileLine.strip().split(' ')
            print (data)
            dfIndex=int(data[4])
            for i in range(15,len(data)):
                strSplit = data[i]
                commaIndex = strSplit.find(',')
                arSplit = strSplit.split(',')
                try:
                    insertIndex = arRegionId.index(int(arSplit[0]))
                    listSpatialDensity[insertIndex] = arSplit[1]
                except ValueError:
                    continue
            for j in range(0,len(arRegionId)):
                strColName="sp_"+str(arRegionId[j])
                df_col_index=df1.columns.get_loc(strColName)
                df1.loc[indexCnt,'sp_'+str(arRegionId[j])] = listSpatialDensity[j]
                df1.loc[indexCnt,df_col_index] = listSpatialDensity[j]
            indexCnt = indexCnt +1
    #print('after loop')
    # READ Dataframe done - Sampling here
    if(sampleSize==0):
        df_sample=df1
    else:
        df_sample=df1.sample(n=sampleSize, random_state=1, weights='Access count')
    df_sample.sort_index(inplace=True)
    vecLabel_Y=df_sample['RegionId'].tolist()
    if (colSelect == 1):
        vecLabel_X=df_sample['RegionId'].to_list()
        colSelList=[]
        for i in range(0,len(vecLabel_Y)):
            colSelList.append('sp_'+str(vecLabel_Y[i]))
        #print(colSelList)
        #print(df_sample.columns)
        dfHeatMap=df_sample[colSelList]
    else:
        vecLabel_X=df1['RegionId'].to_list()
        dfHeatMap=df_sample[listNumRegion]

    dfHeatMap.apply(pd.to_numeric)
    arSpHeatMap=np.empty([len(arRegionId),len(arRegionId)])
    arSpHeatMap= dfHeatMap.to_numpy()
    arSpHeatMap= arSpHeatMap.astype('float64')
    accessTotal =df_sample['Access count'].sum()
    arAccessPercent=(df_sample[['Access count']].div(accessTotal)).to_numpy()
    arBlk = df_sample[['Block count']].to_numpy()
    arBlk = arBlk.astype('int')

    arAccessBlk = np.divide(arAccessPercent,arBlk)
    arAccessBlk = arAccessBlk.astype('float64')
    fig, ax =plt.subplots(1,3, figsize=(15, 10),gridspec_kw={'width_ratios': [9, 1,1]})
    sns.heatmap(arSpHeatMap,cmap='BuGn',cbar=False, annot=True, annot_kws = {'size':8}, ax=ax_0)
    ax_0.invert_yaxis()
    ax_0.set_yticklabels(vecLabel_Y,rotation='horizontal', wrap=True)
    ax_0.set_xticklabels(vecLabel_X,rotation='vertical')
    ax_0.set_title('Spatial density heatmap')

    sns.heatmap(arAccessPercent, cmap='BuGn', cbar=False,annot=True, fmt='.3f', annot_kws = {'size':12},  ax=ax_1)
    ax_1.invert_yaxis()
    ax_1.set_xticks([0])
    ax_1.set_yticks([0])
    ax_1.set_title(' % Access')

    sns.heatmap(arBlk, cmap='BuGn', cbar=False,annot=True, fmt='d', annot_kws = {'size':12},  ax=ax_2)
    ax_2.invert_yaxis()
    ax_2.set_xticks([0])
    ax_2.set_yticklabels(vecLabel_Y,rotation='horizontal', wrap=True)
    ax_2.yaxis.set_ticks_position('right')
    ax_2.set_title(' # Blocks')
    plt.suptitle(strApp+'\n \n Total Access '+str(accessTotal), fontsize=14)

    imageFileName=filename[0:filename.rindex('/')]+'/'+strApp.replace(' ','-')+'.pdf'
    print(imageFileName)
    #plt.show()
    plt.savefig(imageFileName, bbox_inches='tight')

# df=pd.read_table(filename, sep=" ", skipinitialspace=True, usecols=range(4,14),
# names=['RegionId','colon', 'ar', 'Address Range', 'lf', 'Lifetime', 'ac', 'Access count', 'bc', 'Block count'])
def get_intra_obj (data_intra_obj, fileline,blockid,regionIdNum):
    add_row=[None]*516
    data = fileline.strip().split(' ')
    #print("in fill_data_frame", data[2], data[4], data[15:])
    str_index=regionIdNum+'-'+data[2][-1]+'-'+data[4] #regionid-pageid-cacheline
    add_row[0]=blockid
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
    data_intra_obj.append(add_row)

# Works for spatial denity, Spatial Probability and Proximity
def intraObjectPlot(strApp, strFileName,numRegion, strMetric=None, f_avg=None,listCombineReg=None,flWeight=None):
    # STEP 1 - Read spatial.txt and write inter-region file
    strPath=strFileName[0:strFileName.rindex('/')]
    if (strMetric == 'SD' or strMetric == None):
        strMetric='SD'
        fileName='/inter_object_sd.txt'
        lineStart='***'
        lineEnd='==='
        strMetricIdentifier='Spatial_Density'
        strMetricTitle='Spatial Density'
    elif(strMetric == 'SP'):
        fileName='/inter_object_sp.txt'
        lineStart='==='
        lineEnd='---'
        strMetricIdentifier='Spatial_Prob'
        strMetricTitle='Spatial Probability'
    elif(strMetric == 'SI'):
        fileName='/inter_object_si.txt'
        lineStart='---'
        lineEnd='<----'
        strMetricIdentifier='Spatial_Interval'
        strMetricTitle='Spatial Interval'
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
        with open(strFileName) as f:
            for fileLine in f:
                data=fileLine.strip().split(' ')
                if (data[0] == lineStart and (data[2][0:len(data[2])-1]) == regionIdName):
                    blockId=data[2][-1]
                    #print('region line' , regionIdNumName, blockId, data[2])
                    get_intra_obj(data_list_intra_obj,fileLine,regionIdNum+'-'+blockId,regionIdNum)
        f.close()
        print('**** before regionIdNumName ', regionIdNumName, 'list length', len(data_list_intra_obj))
        if(listCombineReg != None and regionIdNumName_copy in listCombineReg):
            data_list_combine_Reg.extend(data_list_intra_obj)
            data_list_intra_obj=copy.deepcopy(data_list_combine_Reg)
            print('***** after regionIdNumName ', regionIdNumName, 'list data_list_intra_obj length', len(data_list_intra_obj), 'list data_list_combine_Reg', len(data_list_combine_Reg))

        # STEP 3b - Convert list to data frame
        #print(data_list_intra_obj)
        list_col_names=[None]*516
        list_col_names[0]='blockid'
        list_col_names[1]='reg-page-blk'
        list_col_names[2]='Access'
        list_col_names[3]='Lifetime'
        list_col_names[4]='Address'
        for i in range ( 0,255):
            list_col_names[5+i]='self'+'-'+str(255-i)
        list_col_names[260]='self'
        for i in range ( 1,256):
            list_col_names[260+i]='self'+'+'+str(i)
        #print((list_col_names))
        df_intra_obj=pd.DataFrame(data_list_intra_obj,columns=list_col_names)
        #print(df_intra_obj[['Address', 'reg-page-blk','self-2','self-1','self','self+1','self+2']])
        #df_intra_obj.to_csv('/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/temp.csv')

        # STEP 3c - Process data frame to get access, lifetime totals
        df_intra_obj = df_intra_obj.astype({"Access": int, "Lifetime": int})
        accessSumBlocks= df_intra_obj['Access'].sum()
        arRegionBlocks=df_intra_obj['blockid'].unique()
        #print (arRegionBlocks)
        arBlockIdAccess = np.empty([len(arRegionBlocks),1])
        for arRegionBlockId in range(0, len(arRegionBlocks)):
            #print(arRegionBlockId, df_intra_obj[df_intra_obj['blockid']==arRegionBlocks[arRegionBlockId]]['Access'].sum())
            arBlockIdAccess[int(arRegionBlockId)] = df_intra_obj[df_intra_obj['blockid']==arRegionBlocks[arRegionBlockId]]['Access'].sum()
        #print (arBlockIdAccess)

        # STEP 3d - Get average of self before sampling for highest access blocks
        average_sd= pd.to_numeric(df_intra_obj["self"]).mean()
        print('*** Before sampling Average '+strMetric+' for '+strApp+', Region '+regionIdNumName+' '+str(average_sd))
        get_col_list=[None]*511
        for i in range ( 0,255):
            get_col_list[i]='self'+'-'+str(255-i)
        get_col_list[255]='self'
        for i in range ( 1,256):
            get_col_list[255+i]='self'+'+'+str(i)
        # Change data to numeric
        for i in range (0,len(get_col_list)):
            df_intra_obj[get_col_list[i]]=pd.to_numeric(df_intra_obj[get_col_list[i]])


        # START Lexi-BFS - experiment #2 - plotting SP is helpful
        # Write Spatial probability data for self-3 to self+3 range to plot using plot_variants_avg.py
        if((1 ==0 ) and (strMetric == 'SP' or strMetric == 'SI')):
            f_variant_SP=open('/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/SP-SI.csv','a')
            get_SP_col=['self-3','self-2','self-1','self','self+1','self+2','self+3']
            for i in range(0, len(get_SP_col)):
                col_values=df_intra_obj[get_SP_col[i]]
                my_string = ','.join(map(str, col_values))
                f_variant_SP.write(strApp.replace(' ','')+'-'+strMetric+','+strMetric+','+get_SP_col[i]+','+my_string+'\n')
        # END Lexi-BFS - experiment #2 - plotting SP is helpful

        # STEP 3e - Range and Count mean, standard deviation to understand the original spread of heatmap
        # STEP 3e - Range and Count mean, calculate before sampling
        # START Lexi-BFS - experiment #3 - SD range & gap analysis
        print('dataframe length', len(df_intra_obj))
        list_blk_range_gap=[]
        for df_id in range (0, len(df_intra_obj)):
            blk_id=df_intra_obj.iloc[df_id,1]
            df_row=pd.to_numeric(df_intra_obj.iloc[df_id,5:516]).squeeze()
            first_index=df_row.first_valid_index()
            last_index=df_row.last_valid_index()
            if ( first_index == 'self'):
                first_value=0
            else:
                first_value=int(''.join(filter(str.isdigit, first_index)))
            if ( last_index == 'self'):
                last_value=0
            else:
                last_value=int(''.join(filter(str.isdigit, last_index)))
            valid_range=first_value+last_value+1
            list_blk_range_gap.append([blk_id,first_value,last_value,valid_range, df_row.count() ])
        #print(list_blk_range_gap)
        df_range_gap=pd.DataFrame(list_blk_range_gap,columns=['reg-page-blk','first','last','range','count'])
        df_range_gap = df_range_gap.astype({"range": int, "count": int})
        print(strApp,' ', strMetric, ', Region ', regionIdNumName, ' Range mean ', pd.to_numeric(df_range_gap["range"]).mean())
        print(strApp,' ', strMetric, ', Region ', regionIdNumName, ' Range std ', pd.to_numeric(df_range_gap["range"]).std())
        print(strApp,' ', strMetric, ', Region ', regionIdNumName, ' Count mean ', pd.to_numeric(df_range_gap["count"]).mean())
        print(strApp,' ', strMetric, ', Region ', regionIdNumName, ' Count std ', pd.to_numeric(df_range_gap["count"]).std())
        # END Lexi-BFS - experiment #3 - SD range & gap analysis

        if(f_avg != None):
            f_avg.write(strApp+' '+ strMetric+ ' '+', Region '+regionIdNumName+ 'Range mean '+ str(pd.to_numeric(df_range_gap["range"]).mean())+'\n')
            f_avg.write(strApp+' '+ strMetric+ ' '+', Region '+regionIdNumName+ 'Range std '+ str(pd.to_numeric(df_range_gap["range"]).std())+'\n')
            f_avg.write(strApp+' '+ strMetric+ ' '+', Region '+regionIdNumName+ 'Count mean '+ str(pd.to_numeric(df_range_gap["count"]).mean())+'\n')
            f_avg.write(strApp+' '+ strMetric+ ' '+', Region '+regionIdNumName+ 'Count std '+ str(pd.to_numeric(df_range_gap["count"]).std())+'\n')

        # STEP 3f - Sample dataframe for 50 rows based on access count as the weight
        # Sample 50 reg-page-blkid lines from all blocks based on Access counts
        num_sample=50
        df_intra_obj_rows=df_intra_obj.shape[0]
        if ( df_intra_obj_rows < num_sample):
            num_sample = df_intra_obj_rows
        df_intra_obj_sample=df_intra_obj.sample(n=num_sample, random_state=1, weights='Access')
        df_intra_obj_sample.set_index('reg-page-blk')
        df_intra_obj_sample.sort_index(inplace=True)
        list_xlabel=df_intra_obj_sample['reg-page-blk'].to_list()
        accessBlockCacheLine = (df_intra_obj_sample[['Access']]).to_numpy()
        df_intra_obj_sample_hm=df_intra_obj_sample[get_col_list]
        self_bef_drop=df_intra_obj_sample_hm['self'].to_list()
        #print('before dropna shape = ' , df_intra_obj_sample_hm.shape)

        # STEP 3g - drop columns with "all" NaN values
        # DROP - columns with no values
        df_intra_obj_drop=df_intra_obj_sample_hm.dropna(axis=1,how='all')
        get_col_list=df_intra_obj_drop.columns.to_list()
        # STEP 3g - add some columns above & below self line to visualize better
        flAddSelfBelow=1
        flAddSelfAbove=1
        pattern = re.compile('self-.*')
        if any((match := pattern.match(x)) for x in get_col_list):
            flAddSelfBelow=0
        pattern = re.compile('self\+.*')
        if any((match := pattern.match(x)) for x in get_col_list):
            #print(match.group(0))
            flAddSelfAbove=0
        if(flAddSelfBelow == 1):
            for i in range (1,10):
                get_col_list.insert(0,'self-'+str(i))
        if(flAddSelfAbove == 1):
            for i in range (1,10):
                get_col_list.append('self+'+str(i))

        # STEP 3h - get columns that are useful
        df_intra_obj_sample_hm=df_intra_obj_sample[get_col_list]
        average_sd= pd.to_numeric(df_intra_obj_sample_hm["self"]).mean()
        print('*** After sampling Average '+strMetric+' for '+strApp+', Region '+regionIdNumName+' '+str(average_sd))
        if (f_avg != None):
            f_avg.write ( '*** Average '+strMetric+' for '+strApp+', Region '+regionIdNumName+' '+str(average_sd)+'\n')
        self_aft_drop=df_intra_obj_sample_hm['self'].to_list()
        #print(self_aft_drop)
        if(self_bef_drop == self_aft_drop):
            print(" After drop equal ")
        else:
            print("Error - not equal")
            print ("before drop", self_bef_drop)
            print ("after drop", self_aft_drop)
            break

        # STEP 3i - Start Heatmap Visualization
        df_intra_obj_sample_hm.apply(pd.to_numeric)
        df_hm=np.empty([num_sample,len(get_col_list)])
        df_hm=df_intra_obj_sample_hm.to_numpy()
        df_hm=df_hm.astype('float64')
        df_hm=df_hm.transpose()

        #fig, ax =plt.subplots(1,3, figsize=(15, 10),gridspec_kw={'width_ratios': [11, 1.5, 1.5]})
        fig = plt.figure(constrained_layout=True, figsize=(15, 10))
        gs = gridspec.GridSpec(1, 2, figure=fig,width_ratios=[11,4])
        gs0 = gridspec.GridSpecFromSubplotSpec(1, 1, subplot_spec = gs[0] )
        gs1 = gridspec.GridSpecFromSubplotSpec(1, 2, subplot_spec = gs[1] ,wspace=0.07)
        ax_0 = fig.add_subplot(gs0[0, :])
        ax_1 = fig.add_subplot(gs1[0, 0])
        ax_2 = fig.add_subplot(gs1[0, 1])

        if(strMetric == 'SI'):
            vmin_val=0.0
            vmax_val=100.0
        elif(strMetric == 'SP'):
            vmin_val=0.0
            vmax_val=1.0
        else:
            vmin_val=0.0
            vmax_val=1.0

        if ('minivite' in strApp.lower() and strMetric == 'SI'):
            df_mask = df_intra_obj_sample_hm.applymap(lambda x: True if (x >150) else False).transpose().to_numpy().astype(bool)
            print(df_mask,' \n', df_hm)
            print ('mask ', df_mask.shape, ' df_hm ', df_hm.shape)
            ax_0 = sns.heatmap(df_hm, mask=df_mask, cmap='mako_r',cbar=True, annot=False,ax=ax_0,vmin=vmin_val,vmax=vmax_val)
        else:
            ax_0 = sns.heatmap(df_hm,cmap='mako_r',cbar=True, annot=False,ax=ax_0,vmin=vmin_val,vmax=vmax_val)
        ax_0.invert_yaxis()
        list_y_ticks=ax_0.get_yticklabels()
        fig_ylabel=[]
        for y_label in list_y_ticks:
            fig_ylabel.append(get_col_list[int(y_label.get_text())])
        list_x_ticks=ax_0.get_xticklabels()
        fig_xlabel=[]
        for x_label in list_x_ticks:
            fig_xlabel.append(list_xlabel[int(x_label.get_text())])
        ax_0.set_yticklabels(fig_ylabel,rotation='horizontal', wrap=True)
        ax_0.set_xticklabels(fig_xlabel,rotation='vertical')
        ax_0.set(xlabel="Region-Page-Block", ylabel="Affinity to contiguous blocks")
        #ax_0.set_ylabel("")

        sns.heatmap(accessBlockCacheLine, cmap="PuBu", cbar=False,annot=True, fmt='g', annot_kws = {'size':12},  ax=ax_1)
        ax_1.invert_yaxis()
        ax_1.set_xticks([0])
        length_xlabel= len(list_xlabel)
        list_blkcache_label=[]
        for i in range (0, length_xlabel):
            list_blkcache_label.append(list_xlabel[i])
        ax1_ylabel=[]
        list_y_ticks=ax_1.get_yticklabels()
        for y_label in list_y_ticks:
            ax1_ylabel.append(list_blkcache_label[int(y_label.get_text())])
        ax_1.set_yticks(range(0,len(list_blkcache_label)),list_blkcache_label, rotation='horizontal', wrap=True )
        ax_1.yaxis.set_ticks_position('right')

        rndAccess = np.round((arBlockIdAccess/1000).astype(float),2)
        annot = np.char.add(rndAccess.astype(str), 'K')
        sns.heatmap(rndAccess, cmap='Blues', cbar=False,annot=annot, fmt='', annot_kws = {'size':12},  ax=ax_2)
        ax_2.invert_yaxis()
        ax_2.set_xticks([0])
        list_blk_label=arRegionBlocks.tolist()
        ax_2.set_yticklabels(list_blk_label, rotation='horizontal', wrap=True )
        ax_2.yaxis.set_ticks_position('right')

        plt.suptitle(strMetricTitle +' and Access heatmap for '+strApp +' region - '+regionIdNumName)
        strArRegionAccess = str(np.round((arRegionAccess/1000).astype(float),2))+'K'
        strAccessSumBlocks= str(np.round((accessSumBlocks/1000).astype(float),2))+'K'
        strTitle = strMetricTitle+' \n Region\'s access - ' + strArRegionAccess + ', Access count for selected pages - ' + strAccessSumBlocks \
                   +' ('+ ("{0:.1f}".format((accessSumBlocks/arRegionAccess)*100)) +'%)\n Number of pages in region - '+ str(numRegionBlocks)
        #print(strTitle)
        ax_0.set_title(strTitle)
        ax_1.set_title('Access count for selected blocks and \n          hottest pages in the region\n ',loc='left')

        #plt.show()
        imageFileName=strPath+'/'+strApp.replace(' ','')+'-'+regionIdNumName.replace(' ','').replace('&','-')+'-'+strMetric+'_hm.pdf'
        print(imageFileName)
        plt.savefig(imageFileName, bbox_inches='tight')
        plt.close()


#intraObjectPlot('HiParTi - COO-Reduce','/Users/suri836/Projects/spatial_rud/HiParTi/4096-same-iter/mg-spmm-mat/spmm_mat-U-1-trace-b8192-p4000000/spatial.txt', \
#                2,listCombineReg=['0-A0000000', '1-A1000000', '2-A2000000','3-A2000010'])
#intraObjectPlot('Minivite-V3','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v3_spatial_det.txt',3,strMetric='SI', \
#                listCombineReg=['1-A0000001','5-A0001200'] )
#intraObjectPlot('Minivite-V3','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v3_spatial_det.txt',3,strMetric='SP', \
#                listCombineReg=['1-A0000001','5-A0001200'] )
#intraObjectPlot('Minivite-V1','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v1_spatial_det.txt',1,strMetric='SP')
#intraObjectPlot('Minivite-V1','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v1_spatial_det.txt',1,strMetric='SI')

#intraObjectPlot('HiParTI-HiCOO-BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1, 'SP')

#Minivite - paper plots - Combine regions
if ( 1==0):
    f_avg1=open('/Users/suri836/Projects/spatial_rud/minivite_detailed_look/spatial_clean/sd_avg_log','w')
    intraObjectPlot('miniVite-v1','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v1_spatial_det.txt',1, f_avg=f_avg1)
    intraObjectPlot('miniVite-v2','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v2_spatial_det.txt',3,listCombineReg=['1-A0000010','4-A0002000'] , f_avg=f_avg1)
    intraObjectPlot('miniVite-v3','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v3_spatial_det.txt',3,listCombineReg=['1-A0000001','5-A0001200'] , f_avg=f_avg1)
    f_avg1.close()

#Darknet - paper plots
if ( 1 == 0):
    intraObjectPlot('AlexNet','/Users/suri836/Projects/spatial_rud/darknet_cluster/alexnet_single/spatial_clean/spatial.txt',3)
    intraObjectPlot('ResNet', '/Users/suri836/Projects/spatial_rud/darknet_cluster/resnet152_single/spatial_clean/spatial.txt',1)

#HiParTi - HiCOO - Matrix - paper plots
if ( 1 == 0):
    f_avg1=open('/Users/suri836/Projects/spatial_rud/HiParTi/4096-same-iter/sd_agg_log','w')
    intraObjectPlot('HiParTi - CSR','/Users/suri836/Projects/spatial_rud/HiParTi/4096-same-iter/mg-csr/spmm_csr_mat-trace-b8192-p4000000/spatial.txt',2,f_avg=f_avg1)
    intraObjectPlot('HiParTi - COO','/Users/suri836/Projects/spatial_rud/HiParTi/4096-same-iter/mg-spmm-mat/spmm_mat-U-0-trace-b8192-p4000000/spatial.txt',3,f_avg=f_avg1)
    intraObjectPlot('HiParTi - COO-Reduce','/Users/suri836/Projects/spatial_rud/HiParTi/4096-same-iter/mg-spmm-mat/spmm_mat-U-1-trace-b8192-p4000000/spatial.txt', \
                    2,listCombineReg=['0-A0000000', '1-A1000000', '2-A2000000','3-A2000010'])
    intraObjectPlot('HiParTi - HiCOO','/Users/suri836/Projects/spatial_rud/HiParTi/4096-same-iter/mg-spmm-hicoo/spmm_hicoo-U-0-trace-b8192-p4000000/spatial.txt',2,f_avg=f_avg1)
    intraObjectPlot('HiParTi - HiCOO-Schedule','/Users/suri836/Projects/spatial_rud/HiParTi/4096-same-iter/mg-spmm-hicoo/spmm_hicoo-U-1-trace-b8192-p4000000/spatial.txt',3,f_avg=f_avg1)
    f_avg1.close()

# HiParTI - HiCOO - Reorder heatmaps
if (1 ==1):
    intraObjectPlot('HiParTI-HiCOO', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/sp-si/spatial.txt', 1)
    intraObjectPlot('HiParTI-HiCOO-Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/sp-si/spatial.txt', 1)
    intraObjectPlot('HiParTI-HiCOO-BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1)
    intraObjectPlot('HiParTI-HiCOO-Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/sp-si/spatial.txt', 1)

# ParTi - Tensor variants
#intraObjectPlot('ParTI-COO - m-0', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor/mttkrp-m-0-sel-trace-b8192-p5000000/spatial.txt', 6)
#intraObjectPlot('ParTI-COO - m-1', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor/mttkrp-m-1-sel-trace-b8192-p5000000/spatial.txt', 6)
#intraObjectPlot('ParTI-COO - m-2', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor/mttkrp-m-2-sel-trace-b8192-p5000000/spatial.txt', 6)

#intraObjectPlot('ParTI-HiCOO - m-0', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor/mttkrp_hicoo-m-0-sel-trace-b8192-p5000000/spatial.txt', 2)
#intraObjectPlot('ParTI-HiCOO - m-1', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor/mttkrp_hicoo-m-1-sel-trace-b8192-p5000000/spatial.txt', 3)
#intraObjectPlot('ParTI-HiCOO - m-2', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor/mttkrp_hicoo-m-2-sel-trace-b8192-p5000000/spatial.txt', 2)

# HiParTi - Tensor variants
if ( 1 == 0):
    intraObjectPlot('HiParTI-HiCOO', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/spatial.txt', 1)
    intraObjectPlot('HiParTI-HiCOO-Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/spatial.txt', 1)
    intraObjectPlot('HiParTI-HiCOO-BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/spatial.txt', 1)
    intraObjectPlot('HiParTI-HiCOO-Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/spatial.txt', 1)

if ( 1 == 0):
    intraObjectPlot('HiParTI-HiCOO', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/sp-si/spatial.txt', 1, 'SP')
    intraObjectPlot('HiParTI-HiCOO', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/sp-si/spatial.txt', 1, 'SI')
    intraObjectPlot('HiParTI-HiCOO-Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/sp-si/spatial.txt', 1, 'SP')
    intraObjectPlot('HiParTI-HiCOO-Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/sp-si/spatial.txt', 1, 'SI')
    intraObjectPlot('HiParTI-HiCOO-BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1, 'SP')
    intraObjectPlot('HiParTI-HiCOO-BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1, 'SI')
    intraObjectPlot('HiParTI-HiCOO-Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/sp-si/spatial.txt', 1, 'SP')
    intraObjectPlot('HiParTI-HiCOO-Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/sp-si/spatial.txt', 1, 'SI')

# HiParTi - Tensor variants - Use buffer
#intraObjectPlot('HiParTI-HiCOO ', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-1/mttsel-re-0-b16384-p4000000-U-1/spatial.txt', 2)
#intraObjectPlot('HiParTI-HiCOO Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-1/mttsel-re-1-b16384-p4000000-U-1/spatial.txt', 4)
#intraObjectPlot('HiParTI-HiCOO BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-1/mttsel-re-2-b16384-p4000000-U-1/spatial.txt', 3)
#intraObjectPlot('HiParTI-HiCOO Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-1/mttsel-re-3-b16384-p4000000-U-1/spatial.txt', 4)

# HiParTi - Tensor variants - Freebase tensor
#intraObjectPlot('HiParTI-HiCOO ', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/fb-U-0/mttsel-fb-re-0-b16384-p4000000-U-0/spatial.txt', 3)
#intraObjectPlot('HiParTI-HiCOO Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/fb-U-0/mttsel-fb-re-1-b16384-p4000000-U-0/spatial.txt', 2)
#intraObjectPlot('HiParTI-HiCOO BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/fb-U-0/mttsel-fb-re-2-b16384-p4000000-U-0/spatial.txt', 4)
#intraObjectPlot('HiParTI-HiCOO Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/fb-U-0/mttsel-fb-re-3-b16384-p4000000-U-0/spatial.txt', 3)

# For debug
#/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/C2000000_1.txt 1 AMG C2000000_1 40
#filename='/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/C2000000_1.txt'
#colSelect=1
#appName='AMG C2000000_1'
#sampleSize=40
#/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/B0000000_0.txt 1 AMG B0000000_0 40
#filename='/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/B0000000_0.txt'
#colSelect=1
#appName='AMG B0000000_0'
#sampleSize=40
#spatialPlot(filename, colSelect, appName,sampleSize)
