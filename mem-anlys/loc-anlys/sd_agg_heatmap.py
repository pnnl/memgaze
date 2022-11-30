import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt

sns.color_palette("light:#5A9", as_cmap=True)
sns.set()

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
            dfIndex=int(data[4])
            for i in range(15,len(data)):
                strSplit = data[i]
                commaIndex = strSplit.find(',')
                arSplit = strSplit.split(',')
                #print('arRegion', arRegionId)
                #print('arSplit', arSplit)
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
    sns.heatmap(arSpHeatMap,cmap='BuGn',cbar=False, annot=True, annot_kws = {'size':8}, ax=ax[0])
    ax[0].invert_yaxis()
    ax[0].set_yticklabels(vecLabel_Y,rotation='horizontal', wrap=True)
    ax[0].set_xticklabels(vecLabel_X,rotation='vertical')
    ax[0].set_title('Spatial density heatmap')

    sns.heatmap(arAccessPercent, cmap='BuGn', cbar=False,annot=True, fmt='.3f', annot_kws = {'size':12},  ax=ax[1])
    ax[1].invert_yaxis()
    ax[1].set_xticks([0])
    ax[1].set_yticks([0])
    ax[1].set_title(' % Access')

    sns.heatmap(arBlk, cmap='BuGn', cbar=False,annot=True, fmt='d', annot_kws = {'size':12},  ax=ax[2])
    ax[2].invert_yaxis()
    ax[2].set_xticks([0])
    ax[2].set_yticklabels(vecLabel_Y,rotation='horizontal', wrap=True)
    ax[2].yaxis.set_ticks_position('right')
    ax[2].set_title(' # Blocks')
    plt.suptitle(strApp+'\n \n Total Access '+str(accessTotal), fontsize=14)

    imageFileName=filename[0:filename.rindex('/')]+'/'+strApp.replace(' ','-')+'.pdf'
    print(imageFileName)
    #plt.show()
    plt.savefig(imageFileName, bbox_inches='tight')

# df=pd.read_table(filename, sep=" ", skipinitialspace=True, usecols=range(4,14),
# names=['RegionId','colon', 'ar', 'Address Range', 'lf', 'Lifetime', 'ac', 'Access count', 'bc', 'Block count'])
def get_intra_obj (data_intra_obj, region_blk, fileline,blockid):
    add_row=[None]*516
    data = fileline.strip().split(' ')
    #print("in fill_data_frame", data[2], data[4], data[15:])
    str_index=data[2][-1]+'-'+data[4] #regionid-cacheline
    add_row[0]=blockid
    add_row[1]=str_index
    add_row[2] = data[11] # access count
    add_row[3]=data[9] # lifetime
    add_row[4]=data[7] # Address range
    linecache = int(data[4])
    for i in range(15,len(data)):
        cor_data=data[i].split(',')
        # Added value for 'self' so that it doesnt get dropped in dropna
        add_row[5+255]=0
        if(int(cor_data[0])<linecache):
            add_row_index=5+255-(linecache-int(cor_data[0]))
            #print("if ", linecache, cor_data[0], 5+255-(linecache-int(cor_data[0])))
        else:
            add_row_index=5+255+(int(cor_data[0])-linecache)
            #print("else", linecache, cor_data[0], 5+255+(int(cor_data[0])-linecache))
        add_row[add_row_index]=cor_data[1]
    #if(linecache<5 or linecache>250):
    #    print(add_row)
    data_intra_obj.append(add_row)



# Read spatial.txt and write inter-region file, intra-region files for callPlot
# Works for spatial denity now - ***
def intraObjectPlot(strApp, strFileName,numRegion):
    strPath=strFileName[0:strFileName.rindex('/')]
    #print(strPath)
    # Write inter-object_sd.txt
    strInterRegionFile=strPath+'/inter_object_sd.txt'
    f_out=open(strInterRegionFile,'w')
    with open(strFileName) as f:
        for fileLine in f:
            data=fileLine.strip().split(' ')
            if (data[0] == '***'):
                #print (fileLine)
                f_out.write(fileLine)
            if (data[0] == '==='):
                f_out.close()
                break
    f.close()
    plotFilename=strInterRegionFile
    appName=strApp
    colSelect=0
    sampleSize=0
    print(plotFilename,colSelect,appName,sampleSize)
    #spatialPlot(plotFilename, colSelect, appName,sampleSize)

    df_inter=pd.read_table(strInterRegionFile, sep=" ", skipinitialspace=True, usecols=range(2,14),
                     names=['RegionId_Name','Page', 'RegionId_Num','colon', 'ar', 'Address Range', 'lf', 'Lifetime', 'ac', 'Access count', 'bc', 'Block count'])
    df_inter_data=df_inter[['RegionId_Name', 'RegionId_Num', 'Address Range', 'Lifetime', 'Access count', 'Block count']]
    df_inter_data['Reg_Num-Name']=df_inter_data.apply(lambda x:'%s-%s' % (x['RegionId_Num'],x['RegionId_Name']),axis=1)
    df_inter_data_sample=df_inter_data.nlargest(n=numRegion,  columns=['Access count', 'Block count'])
    arRegionId = df_inter_data_sample['Reg_Num-Name'].values.flatten().tolist()
    print(arRegionId)
    for j in range(0, len(arRegionId)):
    #for j in range(0, 1):
        regionIdNumName=arRegionId[j]
        arRegionAccess = df_inter_data_sample[ df_inter_data_sample['Reg_Num-Name']==regionIdNumName]['Access count'].values.flatten()[0]
        numRegionBlocks = df_inter_data_sample[ df_inter_data_sample['Reg_Num-Name']==regionIdNumName]['Block count'].values.flatten()[0]
        data_list_intra_obj=[]
        regionIdName =regionIdNumName[regionIdNumName.index('-')+1:]
        print(regionIdName)
        with open(strFileName) as f:
            for fileLine in f:
                data=fileLine.strip().split(' ')
                if (data[0] == '***' and (data[2][0:len(data[2])-1]) == regionIdName):
                    blockId=data[2][-1]
                    #print('region line' , regionIdNumName, blockId, data[2])
                    get_intra_obj(data_list_intra_obj,data[2],fileLine,blockId)
        f.close()
        #print(data_list_intra_obj)
        list_col_names=[None]*516
        list_col_names[0]='blockid'
        list_col_names[1]='blk-cache'
        list_col_names[2]='Access'
        list_col_names[3]='Lifetime'
        list_col_names[4]='Address'
        for i in range ( 0,256):
            list_col_names[5+i]='self'+'-'+str(255-i)
        list_col_names[260]='self'
        for i in range ( 1,256):
            list_col_names[260+i]='self'+'+'+str(i)
        #print(list_col_names)
        #print(data_list_intra_obj)
        df_intra_obj=pd.DataFrame(data_list_intra_obj,columns=list_col_names)
        df_intra_obj = df_intra_obj.astype({"Access": int, "Lifetime": int})
        accessSumBlocks= df_intra_obj['Access'].sum()
        arRegionBlocks=df_intra_obj['blockid'].unique()
        #print (arRegionBlocks)
        arBlockIdAccess = np.empty([len(arRegionBlocks),1])
        for arRegionBlockId in range(0, len(arRegionBlocks)):
            #print(arRegionBlockId, df_intra_obj[df_intra_obj['blockid']==str(arRegionBlockId)]['Access'].sum())
            arBlockIdAccess[int(arRegionBlockId)] = df_intra_obj[df_intra_obj['blockid']==str(arRegionBlockId)]['Access'].sum()
        print (arBlockIdAccess)

        # Sample 50 blk-cacheid lines from all blocks
        num_sample=50

        df_intra_obj_rows=df_intra_obj.shape[0]
        if ( df_intra_obj_rows < num_sample):
            num_sample = df_intra_obj_rows

        df_intra_obj_sample=df_intra_obj.sample(n=num_sample, random_state=1, weights='Access')
        df_intra_obj_sample.set_index('blk-cache')
        df_intra_obj_sample.sort_index(inplace=True)
        list_xlabel=df_intra_obj_sample['blk-cache'].to_list()
        get_col_list=[None]*511
        for i in range ( 0,255):
            get_col_list[i]='self'+'-'+str(255-i)
        get_col_list[255]='self'
        for i in range ( 1,256):
            get_col_list[255+i]='self'+'+'+str(i)
        #print('col list to get')
        #print(get_col_list)

        accessBlockCacheLine = (df_intra_obj_sample[['Access']]).to_numpy()
        df_intra_obj_sample_hm=df_intra_obj_sample[get_col_list]
        self_bef_drop=df_intra_obj_sample_hm['self'].to_list()

        df_intra_obj_drop=df_intra_obj_sample_hm.dropna(axis=1,how='all')
        get_col_list=df_intra_obj_drop.columns.to_list()
        df_intra_obj_sample_hm=df_intra_obj_sample[get_col_list]
        print('after drop na shape = ' , df_intra_obj_sample_hm.shape)
        self_aft_drop=df_intra_obj_sample_hm['self'].to_list()
        if(self_bef_drop == self_aft_drop):
            print(" equal ")
        else:
            print("Error - not equal")
            print ("before drop", self_bef_drop)
            print ("after drop", self_aft_drop)
            break

        df_intra_obj_sample_hm.apply(pd.to_numeric)
        df_hm=np.empty([num_sample,len(get_col_list)])
        df_hm=df_intra_obj_sample_hm.to_numpy()
        df_hm=df_hm.astype('float64')
        df_hm=df_hm.transpose()

        fig, ax =plt.subplots(1,3, figsize=(15, 10),gridspec_kw={'width_ratios': [9, 1,1]})
        ax[0] = sns.heatmap(df_hm,cmap='BuGn',cbar=True, annot=False,ax=ax[0])
        ax[0].invert_yaxis()
        list_y_ticks=ax[0].get_yticklabels()
        fig_ylabel=[]
        for y_label in list_y_ticks:
            fig_ylabel.append(get_col_list[int(y_label.get_text())])
        list_x_ticks=ax[0].get_xticklabels()
        fig_xlabel=[]
        for x_label in list_x_ticks:
            fig_xlabel.append(list_xlabel[int(x_label.get_text())])
        ax[0].set_yticklabels(fig_ylabel,rotation='horizontal', wrap=True)
        ax[0].set_xticklabels(fig_xlabel,rotation='vertical')

        sns.heatmap(accessBlockCacheLine, cmap='BuGn', cbar=False,annot=True, fmt='g', annot_kws = {'size':12},  ax=ax[1])
        ax[1].invert_yaxis()
        ax[1].set_xticks([0])
        length_xlabel= len(list_xlabel)
        list_blkcache_label=[]
        for i in range (0, length_xlabel):
            list_blkcache_label.append(list_xlabel[i])
        ax1_ylabel=[]
        list_y_ticks=ax[1].get_yticklabels()
        for y_label in list_y_ticks:
            ax1_ylabel.append(list_blkcache_label[int(y_label.get_text())])
        ax[1].set_yticks(range(0,len(list_blkcache_label)),list_blkcache_label, rotation='horizontal', wrap=True )
        ax[1].yaxis.set_ticks_position('right')

        sns.heatmap(arBlockIdAccess, cmap='BuGn', cbar=False,annot=True, fmt='g', annot_kws = {'size':12},  ax=ax[2])
        ax[2].invert_yaxis()
        ax[2].set_xticks([0])
        list_blk_label=arRegionBlocks.tolist()
        list_blk_label.reverse()
        ax[2].set_yticklabels(list_blk_label, rotation='horizontal', wrap=True )
        ax[2].yaxis.set_ticks_position('right')

        plt.suptitle('Spatial density and Access count Heatmap for '+strApp +' region - '+regionIdNumName)
        strTitle = 'Spatial Density \n Region\'s total Access  - ' + str(arRegionAccess) + ', Total accesses for blocks shown - ' + str(accessSumBlocks) +' ('+ ("{0:.4f}".format((accessSumBlocks/arRegionAccess)*100))+' %)\n Total number of blocks in region - '+ str(numRegionBlocks)
        print(strTitle)
        ax[0].set_title(strTitle)
        ax[1].set_title('Access count for blocks \n \n')
        #plt.show()
        imageFileName=strPath+'/'+strApp+'-'+regionIdNumName+'.pdf'
        plt.savefig(imageFileName, bbox_inches='tight')
        plt.close()

#intraObjectPlot('AMG', '/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/spatial.txt',6)
#intraObjectPlot('AlexNet', '/Users/suri836/Projects/spatial_rud/darknet_cluster/alexnet_single/spatial.txt',5)
#intraObjectPlot('ResNet', '/Users/suri836/Projects/spatial_rud/darknet_cluster/resnet152_single/spatial.txt',3)
#intraObjectPlot('Resnet-Gemm', '/Users/suri836/Projects/spatial_rud/agg_sample_data/resnet-1img-trace-b16384-p500000-5p/spatial.txt',5)
#intraObjectPlot('Hicoo-U-1', '/Users/suri836/Projects/spatial_rud/HiParTi/spmm_hicoo-U-1-trace-b16384-p2000000/spatial.txt',3)
#intraObjectPlot('Hicoo','/Users/suri836/Projects/spatial_rud/HiParTi/spmm_hicoo-trace-b16384-p2000000/spatial.txt',3)
#intraObjectPlot('Minivite-V1','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/spatial_10_blocks/v1_spatial_det.txt',2)
#intraObjectPlot('Minivite-V2','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/spatial_10_blocks/v2_spatial_det.txt',3)
#intraObjectPlot('Minivite-V3','/Users/suri836/Projects/spatial_rud/minivite_detailed_look/spatial_10_blocks/v3_spatial_det.txt',3)
intraObjectPlot('AlexNet','/Users/suri836/Projects/spatial_rud/darknet_cluster/alexnet_single/spatial_10_blocks/spatial.txt',3)



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
