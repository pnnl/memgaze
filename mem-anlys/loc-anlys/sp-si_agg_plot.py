# Create intra-region heatmaps for Spatial affinity metrics
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
        #df_intra_obj.to_csv('/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/temp.csv')

        # STEP 3c - Process data frame to get access, lifetime totals
        df_intra_obj = df_intra_obj.astype({"Access": int, "Lifetime": int})
        accessSumBlocks= df_intra_obj['Access'].sum()/3
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
        get_col_list=[None]*512
        for i in range ( 0,255):
            get_col_list[i]='self'+'-'+str(255-i)
        get_col_list[255]='self'
        for i in range ( 1,256):
            get_col_list[255+i]='self'+'+'+str(i)
        get_col_list[511]='Type'
        # Change data to numeric
        for i in range (0,len(get_col_list)-1):
            df_intra_obj[get_col_list[i]]=pd.to_numeric(df_intra_obj[get_col_list[i]])

        # START Lexi-BFS - experiment #2 - plotting SP is helpful
        # Write Spatial probability data for self-3 to self+3 range to plot using plot_variants_avg.py
        if((1 ==0 ) and (strMetric == 'SP' or strMetric == 'SR')):
            f_variant_SP=open('/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/SP-SR.csv','a')
            get_SP_col=['self-3','self-2','self-1','self','self+1','self+2','self+3']
            for i in range(0, len(get_SP_col)):
                col_values=df_intra_obj[get_SP_col[i]]
                my_string = ','.join(map(str, col_values))
                f_variant_SP.write(strApp.replace(' ','')+'-'+strMetric+','+strMetric+','+get_SP_col[i]+','+my_string+'\n')
        # END Lexi-BFS - experiment #2 - plotting SP is helpful

        # STEP 3f - Sample dataframe for 50 rows based on access count as the weight
        # Sample 50 reg-page-blkid lines from all blocks based on Access counts

        #df_intra_obj_sample=df_intra_obj.sample(n=num_sample, random_state=1, weights='Access')
        df_intra_obj_sample=df_intra_obj
        df_intra_obj.set_index('reg-page-blk')
        df_intra_obj.sort_index(inplace=True)
        list_xlabel=df_intra_obj[(df_intra_obj['Type'] == 'SP')]['reg-page-blk'].to_list()
        accessBlockCacheLine = (df_intra_obj[(df_intra_obj['Type'] == 'SP')][['Access']]).to_numpy()
        #print(list_xlabel)
        print(accessBlockCacheLine)

        self_bef_drop=df_intra_obj['self'].to_list()
        # STEP 3g - drop columns with "all" NaN values
        # DROP - columns with no values
        print('before replace drop - ', df_intra_obj.shape)
        print('before drop columns ', df_intra_obj.columns)
        #df_intra_obj_sample_hm.applymap(lambda x: np.nan if (isinstance(x, float) and x < 0.25) else x)
        df_intra_obj_drop=df_intra_obj.dropna(axis=1,how='all')
        self_aft_drop=df_intra_obj_drop['self'].to_list()

        if(self_bef_drop == self_aft_drop):
            print(" After drop equal ")
        else:
            print("Error - not equal")
            print ("before drop", self_bef_drop)
            print ("after drop", self_aft_drop)
            break

        print('after drop columns ', df_intra_obj_drop.columns)
        print('after drop - ', df_intra_obj_drop.shape)
        list_xlabel=df_intra_obj_drop[(df_intra_obj_drop['Type'] == 'SP')]['reg-page-blk'].to_list()
        print(list_xlabel)
        df_intra_obj_SP=df_intra_obj_drop[(df_intra_obj_drop['Type'] == 'SP')]
        df_intra_obj_SI=df_intra_obj_drop[(df_intra_obj_drop['Type'] == 'SI')]
        df_intra_obj_SD=df_intra_obj_drop[(df_intra_obj_drop['Type'] == 'SD')]
        print('SP cols ', df_intra_obj_SP.columns)
        print('SI cols ', df_intra_obj_SI.columns)
        print('after drop SP - ', df_intra_obj_SP.shape)
        print('after drop SI - ', df_intra_obj_SI.shape)
        plot_SP_col=['self-3','self-2','self-1','self','self+1','self+2','self+3']
        list_SP_SI_SD=[[None]*(3*len(plot_SP_col)+1) for i in range(len(list_xlabel))]
        #[[0]*5 for i in range(5)]
        for blkCnt in range(0,len(list_xlabel)):
            blkid= list_xlabel[blkCnt]
            list_SP_SI_SD[blkCnt][0]=blkid
            for plotCnt in range(0,len(plot_SP_col)):
                plotCol= plot_SP_col[plotCnt]
                if(plotCol in df_intra_obj_SP.columns):
                    condSP = (df_intra_obj_SP['reg-page-blk'] == blkid) & (df_intra_obj_SP[plotCol] >= 0.0)
                    resultSP = df_intra_obj_SP[condSP][plotCol]
                    if resultSP.size > 0:
                        condSI = (df_intra_obj_SI['reg-page-blk'] == blkid)
                        resultSI = df_intra_obj_SI[condSI][plotCol]
                        condSD = (df_intra_obj_SD['reg-page-blk'] == blkid)
                        resultSD = df_intra_obj_SD[condSD][plotCol]
                        #print(blkid, plotCol, resultSP.values[0], resultSI.values[0])
                        list_SP_SI_SD[blkCnt][plotCnt*3+1]=resultSP.values[0]
                        list_SP_SI_SD[blkCnt][plotCnt*3+2]=resultSI.values[0]
                        list_SP_SI_SD[blkCnt][plotCnt*3+3]=resultSD.values[0]
        #print(list_SP_SI)
        list_col_names=['reg-page-blk']
        for plotCol in plot_SP_col:
            list_col_names.append('SP-'+plotCol)
            list_col_names.append('SI-'+plotCol)
            list_col_names.append('SD-'+plotCol)
        df_SP_SI_SD=pd.DataFrame(list_SP_SI_SD,columns=list_col_names)
        #print(df_SP_SI_SD)

        fig, ax_plots = plt.subplots(nrows=7, ncols=1,constrained_layout=True, figsize=(15, 10))
        #axs = ax_plots.flatten()
        axs = np.array(ax_plots)
        print(axs)

        for i, ax in enumerate(ax_plots.reshape(-1)):

            #print ('i value', i)
            color = 'tab:red'
            #axs[i].title(plot_SP_col[i])
            ax.set_title('SP and SI for '+ plot_SP_col[i])
            ax.set_xlabel('Blocks in region')
            ax.set_xticks([])
            ax.set_yticks([0.0, 0.25,0.5, 0.75, 1.0])
            ax.set_ylim(0, 1.0)
            ax.set_ylabel('Proximity', color=color)
            ax.plot(df_SP_SI_SD['reg-page-blk'], df_SP_SI_SD['SP-'+plot_SP_col[i]], color=color)
            #ax.plot(df_SP_SI_SD['reg-page-blk'], df_SP_SI_SD['SD-'+plot_SP_col[i]], color='green')
            xmin, xmax = ax.get_xlim()
            #ax.hlines(y=0.25, xmin=xmin, xmax=xmax, linewidth=1, color='black')

            ax.tick_params(axis='y', labelcolor=color)
            ax2 = ax.twinx()  # instantiate a second axes that shares the same x-axis
            color = 'tab:blue'
            ax2.set_ylabel('Interval', color=color)
            ax2.set_ylim(-10,100)
            ax2.plot(df_SP_SI_SD['reg-page-blk'], df_SP_SI_SD['SI-'+plot_SP_col[i]], color=color)
            ax2.tick_params(axis='y', labelcolor=color)

        strAccessSumBlocks= str(np.round((accessSumBlocks/1000).astype(float),2))+'K'
        plt.suptitle(strApp +' region '+regionIdNumName+', Access count '+strAccessSumBlocks)
        imageFileName=strPath+'/'+strApp.replace(' ','')+'-'+regionIdNumName+'-'+strMetric+'.pdf'
        plt.savefig(imageFileName, bbox_inches='tight')
        plt.close()
        #plt.show()

# Before Spatial Interval name change
# Change line 71 for these spatial files
# Before Spatial Interval name change
#intraObjectPlot('Minivite-V1','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v1_spatial_det.txt',1,strMetric='SP-SI')
#intraObjectPlot('Minivite-V2','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v2_spatial_det.txt',3,strMetric='SP-SI', \
#                listCombineReg=['1-A0000010','4-A0002000'] )
#intraObjectPlot('Minivite-V3','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v3_spatial_det.txt',3,strMetric='SP-SI', \
#                listCombineReg=['1-A0000001','5-A0001200'] )

#intraObjectPlot('HiParTI-HiCOO', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-0-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
intraObjectPlot('HiParTI-HiCOO-Lexi', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-1-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
intraObjectPlot('HiParTI-HiCOO-BFS', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-2-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
#intraObjectPlot('HiParTI-HiCOO-Random', '/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/mttsel-re-3-b16384-p4000000-U-0/sp-si/spatial.txt', 1,strMetric='SP-SI')
