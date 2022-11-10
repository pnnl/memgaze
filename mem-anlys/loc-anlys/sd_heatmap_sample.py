import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt

sns.color_palette("light:#5A9", as_cmap=True)
sns.set()

def spatialPlot(filename, colSelect, strApp,sampleSize):
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

    #sns.heatmap(arAccessBlk, cmap='BuGn', cbar=False,annot=True, fmt='.4f', annot_kws = {'size':12},  ax=ax[3])
    #ax[3].invert_yaxis()
    #ax[3].set_xticks([0])
    #ax[3].set_yticklabels(['vec_5','vec_5','vec_4','vec_4','vec_1','vec_1','vec_3','vec_3','vec_2','vec_2','vec_6','vec_6'],rotation='horizontal', wrap=True)
    #ax[3].yaxis.set_ticks_position('right')
    #ax[3].set_title('% Access / Block')
    plt.suptitle(strApp+'\n \n Total Access '+str(accessTotal), fontsize=14)

    imageFileName=filename[0:filename.rindex('/')]+'/'+strApp.replace(' ','-')+'.pdf'
    print(imageFileName)
    #plt.show()
    plt.savefig(imageFileName, bbox_inches='tight')

#filename='/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/spatial_density_detail_intra.txt'
def callPlot(plotApp):
    if(plotApp=='Resnet-B'):
        filename='/Users/suri836/Projects/spatial_rud/darknet_cluster/resnet152_single/sd_detailed_B00000000.txt'
        colSelect=1
        appName='Resnet - intra region - hottest matrix B'
        sampleSize=40
    elif(plotApp=='AMG-inter'):
        filename='/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/spatial_heatmap.txt'
        colSelect=0
        appName='AMG-inter region'
        sampleSize=0
    elif(plotApp=='AMG-inter-hot'):
        filename='/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/spatial_heatmap.txt'
        colSelect=0
        appName='AMG-inter region - Sampled 15 hot regions vs. all regions'
        sampleSize=15
    elif(plotApp=='AMG-intra-hottest'):
        #filename='/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/sd_hot_C20000000.txt'
        filename='/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/rep.txt'
        colSelect=0
        appName='AMG-intra region - Hottest region'
        sampleSize=0
    elif(plotApp=='Alexnet-inter'):
        filename='/Users/suri836/Projects/spatial_rud/darknet_cluster/alexnet_single/sd_inter_region.txt'
        colSelect=0
        appName='Alexnet-inter region'
        sampleSize=0
    elif(plotApp=='Alexnet-hottest_region'):
        filename='/Users/suri836/Projects/spatial_rud/darknet_cluster/alexnet_single/sd_B00000001.txt'
        colSelect=1
        appName='Alexnet-intra hottest region'
        sampleSize=40
    spatialPlot(filename, colSelect, appName,sampleSize)
# For separate plots
#callPlot('AMG-inter')
#callPlot('AMG-inter-hot')
#callPlot('AMG-intra-hottest')
#callPlot('Alexnet-inter')
#callPlot('Alexnet-hottest_region')
#callPlot('Resnet-B')


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
    spatialPlot(plotFilename, colSelect, appName,sampleSize)

    df_inter=pd.read_table(strInterRegionFile, sep=" ", skipinitialspace=True, usecols=range(2,14),
                     names=['RegionId_Name','Page', 'RegionId_Num','colon', 'ar', 'Address Range', 'lf', 'Lifetime', 'ac', 'Access count', 'bc', 'Block count'])
    df_inter_data=df_inter[['RegionId_Name', 'RegionId_Num', 'Address Range', 'Lifetime', 'Access count', 'Block count']]
    df_inter_data['Reg_Num-Name']=df_inter_data.apply(lambda x:'%s-%s' % (x['RegionId_Num'],x['RegionId_Name']),axis=1)

    df_inter_data_sample=df_inter_data.nlargest(n=numRegion,  columns=['Access count'])

    arRegionId = df_inter_data_sample['Reg_Num-Name'].values.flatten().tolist()
    for regionIdNumName in arRegionId:
        regionIdName =regionIdNumName[regionIdNumName.index('-')+1:]
        print(regionIdName)
        blockId=0
        strIntraRegionFile=strPath+'/'+regionIdNumName+'_'+str(blockId)+'.txt'
        f_out = open(strIntraRegionFile,'w')
        lineCnt=0
        with open(strFileName) as f:
            for fileLine in f:
                data=fileLine.strip().split(' ')
                if (data[0] == '***' and (data[2][0:len(data[2])-1]) == regionIdName):
                    if(int(data[2][-1]) ==blockId):
                        f_out.write(fileLine)
                        lineCnt=lineCnt+1
                    else:
                        f_out.close()
                        plotFilename=strIntraRegionFile
                        appName=strApp +' ' +regionIdNumName+'_'+str(blockId)
                        if(lineCnt >40):
                            colSelect=1
                            sampleSize=40
                        else:
                            colSelect=0
                            sampleSize=0
                        print(plotFilename,colSelect,appName,sampleSize)
                        spatialPlot(plotFilename, colSelect, appName,sampleSize)

                        lineCnt=0
                        blockId=int(data[2][-1])
                        strIntraRegionFile=strPath+'/'+regionIdNumName+'_'+str(blockId)+'.txt'
                        f_out = open(strIntraRegionFile,'w')
                        f_out.write(fileLine)
        f.close()
        f_out.close()
        plotFilename=strIntraRegionFile
        appName=strApp +' ' +regionIdNumName+'_'+str(blockId)
        if(lineCnt >40):
            colSelect=1
            sampleSize=40
        else:
            colSelect=0
            sampleSize=0
        print(plotFilename,colSelect,appName,sampleSize)
        spatialPlot(plotFilename, colSelect, appName,sampleSize)

#intraObjectPlot('AMG', '/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/spatial.txt',6)
intraObjectPlot('AlexNet', '/Users/suri836/Projects/spatial_rud/darknet_cluster/alexnet_single/spatial.txt',5)
#intraObjectPlot('ResNet', '/Users/suri836/Projects/spatial_rud/darknet_cluster/resnet152_single/spatial.txt',3)
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
