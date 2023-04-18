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



def get_intra_obj (data_intra_obj, fileline,blockid,regionIdNum):
    add_row=[None]*517
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
    if(data[0] == '==='):
        add_row[516] = 'SP'
    elif(data[0] == '---'):
        add_row[516] = 'SI'
    elif(data[0] == '***'):
        add_row[516] = 'SD'
    data_intra_obj.append(add_row)

# Works for spatial denity, Spatial Probability and Proximity
def intraObjectPlot(strApp, strFileName,numRegion, strMetric=None, f_avg=None,listCombineReg=None):
    # STEP 1 - Read spatial.txt and write inter-region file
    strPath=strFileName[0:strFileName.rindex('/')]
    if(strMetric == 'SP-SI'):
        fileName='/inter_object_sp-si.txt'
        lineStart='---'
        lineEnd='<----'
        # For Minivite use
        #strMetricIdentifier='Spatial_Proximity'
        strMetricIdentifier='Spatial_Interval'
        strMetricTitle='Spatial Proximity and Interval'

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
        list_col_names=[None]*517
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
        list_col_names[516]='Type'
        #print((list_col_names))
        df_intra_obj=pd.DataFrame(data_list_intra_obj,columns=list_col_names)
        print(df_intra_obj.shape)
        #print(df_intra_obj['Type'])
        #print(df_intra_obj[['Address', 'reg-page-blk','self-2','self-1','self','self+1','self+2']])

        # STEP 3c-0 - Process data frame to get access, lifetime totals
        # This is only for the blocks in ten pages analysis (not whole region)
        df_intra_obj = df_intra_obj.astype({"Access": int, "Lifetime": int})
        # STEP 3c-1 - Dataframe has all three metrics, so divide by 3 for access count reporting
        accessSumBlocks= df_intra_obj['Access'].sum()/3
        arRegionBlocks=df_intra_obj['blockid'].unique()
        #print (arRegionBlocks)
        arBlockIdAccess = np.empty([len(arRegionBlocks),1])
        for arRegionBlockId in range(0, len(arRegionBlocks)):
            #print(arRegionBlockId, df_intra_obj[df_intra_obj['blockid']==arRegionBlocks[arRegionBlockId]]['Access'].sum())
            arBlockIdAccess[int(arRegionBlockId)] = df_intra_obj[df_intra_obj['blockid']==arRegionBlocks[arRegionBlockId]]['Access'].sum()
        #print (arBlockIdAccess)

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
        df_intra_obj_sample_SP=df_intra_obj[(df_intra_obj['Type'] == 'SP')].sample(n=num_sample, random_state=1, weights='Access')
        df_intra_obj_sample_SP.set_index('reg-page-blk')
        df_intra_obj_sample_SP.sort_index(inplace=True)
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
        #print(plot_SP_col)
        list_SP_SI_SD=[[None]*(3*len(plot_SP_col)+1) for i in range(len(list_xlabel))]

        list_col_names=['reg-page-blk']
        for blkCnt in range(0,len(list_xlabel)):
            blkid= list_xlabel[blkCnt]
            list_SP_SI_SD[blkCnt][0]=blkid
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
                        list_SP_SI_SD[blkCnt][plotCnt*3+1]=resultSP.values[0]
                    if resultSI.size >0:
                        list_SP_SI_SD[blkCnt][plotCnt*3+2]=resultSI.values[0]
                    if resultSD.size >0:
                        list_SP_SI_SD[blkCnt][plotCnt*3+3]=resultSD.values[0]
        #print(list_SP_SI)

        for plotCol in plot_SP_col:
            list_col_names.append('SP-'+plotCol)
            list_col_names.append('SI-'+plotCol)
            list_col_names.append('SD-'+plotCol)

        df_SP_SI_SD=pd.DataFrame(list_SP_SI_SD,columns=list_col_names)
        df_SP_SI_SD=df_SP_SI_SD.dropna(axis=1, how='all')
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

        df_intra_obj_sample_hm_SP=df_SP_SI_SD[cols_df_SP]
        df_intra_obj_sample_hm_SI=df_SP_SI_SD[cols_df_SI]
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

        fig = plt.figure(constrained_layout=True, figsize=(15, 10))
        gs = gridspec.GridSpec(1, 2, figure=fig,width_ratios=[11,4])
        gs0 = gridspec.GridSpecFromSubplotSpec(2, 1, subplot_spec = gs[0] ,hspace=0.10)
        gs1 = gridspec.GridSpecFromSubplotSpec(1, 2, subplot_spec = gs[1] ,wspace=0.07)
        ax_0 = fig.add_subplot(gs0[0, 0])
        ax_1 = fig.add_subplot(gs0[1, 0])
        ax_2 = fig.add_subplot(gs1[0, 0])
        ax_3 = fig.add_subplot(gs1[0, 1])

        ax_0 = sns.heatmap(df_hm_SP, cmap='mako_r',cbar=True, annot=False,ax=ax_0,vmin=0.0,vmax=1.0)
        ax_1 = sns.heatmap(df_hm_SI, cmap='mako',cbar=True, annot=False,ax=ax_1,vmin=0.0, vmax=10.0)
        ax_0.invert_yaxis()
        list_y_ticks=ax_0.get_yticklabels()
        fig_ylabel=[]
        for y_label in list_y_ticks:
            fig_ylabel.append(cols_df_SP[int(y_label.get_text())][3:])
        list_x_ticks=ax_0.get_xticklabels()
        fig_xlabel=[]
        for x_label in list_x_ticks:
            fig_xlabel.append(df_SP_SI_SD['reg-page-blk'][int(x_label.get_text())])
        ax_0.set_yticklabels(fig_ylabel,rotation='horizontal')
        ax_0.set_xticklabels(fig_xlabel,rotation='vertical')

        ax_1.invert_yaxis()
        list_y_ticks=ax_1.get_yticklabels()
        fig_ylabel=[]
        for y_label in list_y_ticks:
            fig_ylabel.append(cols_df_SI[int(y_label.get_text())][3:])
        list_x_ticks=ax_1.get_xticklabels()
        fig_xlabel=[]
        for x_label in list_x_ticks:
            fig_xlabel.append(df_SP_SI_SD['reg-page-blk'][int(x_label.get_text())])
        ax_1.set_yticklabels(fig_ylabel,rotation='horizontal')
        ax_1.set_xticklabels(fig_xlabel,rotation='vertical')
        ax_0.set(xlabel="Region-Page-Block", ylabel="Affinity to contiguous blocks")
        ax_1.set(xlabel="Region-Page-Block", ylabel="Affinity to contiguous blocks")

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
        rndAccess = np.round((arBlockIdAccess/3000).astype(float),2)
        annot = np.char.add(rndAccess.astype(str), 'K')
        sns.heatmap(rndAccess, cmap='Blues', cbar=False,annot=annot, fmt='', annot_kws = {'size':12},  ax=ax_3)
        ax_3.invert_yaxis()
        ax_3.set_xticks([0])
        list_blk_label=arRegionBlocks.tolist()
        ax_3.set_yticklabels(list_blk_label, rotation='horizontal', wrap=True )
        ax_3.yaxis.set_ticks_position('right')

        strMetricTitle = 'Spatial Proximity, Interval'
        plt.suptitle(strMetricTitle +' and Access heatmap for '+strApp +' region - '+regionIdNumName)
        strArRegionAccess = str(np.round((arRegionAccess/1000).astype(float),2))+'K'
        strAccessSumBlocks= str(np.round((accessSumBlocks/1000).astype(float),2))+'K'
        strTitle = 'Region\'s access - ' + strArRegionAccess + ', Access count for selected pages - ' + strAccessSumBlocks +' ('+ ("{0:.1f}".format((accessSumBlocks/arRegionAccess)*100)) \
                   +'%), Number of pages in region - '+ str(numRegionBlocks) + '\n Spatial Proximity'
        #print(strTitle)
        ax_0.set_title(strTitle)
        ax_1.set_title('Spatial Interval')
        ax_2.set_title('Access count for selected blocks and \n         hottest pages in the region',loc='left')

        imageFileName=strPath+'/'+strApp.replace(' ','')+'-'+regionIdNumName+'-'+strMetric+'_hm.pdf'
        plt.savefig(imageFileName, bbox_inches='tight')
        plt.close()
        #plt.show()

#intraObjectPlot('Minivite-V1','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v1_spatial_det.txt',1,strMetric='SP-SI')
#intraObjectPlot('Minivite-V2','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v2_spatial_det.txt',3,strMetric='SP-SI', \
#                listCombineReg=['1-A0000010','4-A0002000'] )
#intraObjectPlot('Minivite-V3','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v3_spatial_det.txt',3,strMetric='SP-SI', \
#                listCombineReg=['1-A0000001','5-A0001200'] )

#intraObjectPlot('HiParTI-HiCOO', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
#intraObjectPlot('HiParTI-HiCOO-Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
intraObjectPlot('HiParTI-HiCOO-BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
#intraObjectPlot('HiParTI-HiCOO-Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
