import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt

sns.color_palette("light:#5A9", as_cmap=True)

sns.set()

def spatialPlot(filename, colSelect, strApp,sampleSize):
    print(filename)
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
            print(data)
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
    if(strApp=='ubench'):
        df1['Obj_name'] = ['vec-5-0','vec-5-1','vec-4-0','vec-4-1','vec-1-0','vec-1-1','vec-3-0','vec-3-1','vec-2-0','vec-2-1','vec-6-0','vec-6-1']
    elif(strApp=='ubench-array'):
        df1['Obj_name'] = ['E','D','C','B','A']
    elif(strApp=='Minivite-v3'):
        df1['Obj_name'] = ['Graph_0','Map_0','Others','Comm','Graph_1','Map_1']
    else:
        df1['Obj_name'] = df1['RegionId']
    print('before sort')
    print(df1)
    df1.sort_values(by='Obj_name',inplace=True)
    df1.reset_index(inplace=True,drop=True)
    print('after sort')
    print(df1)

    # READ Dataframe done - Sampling here - Used in AMG
    # Might not work correctly with the sorting of objects
    if (1 ==0):
        if(sampleSize==0):
            df_sample=df1
        else:
            df_sample=df1.sample(n=sampleSize, random_state=1, weights='Access count')
        #df_sample.sort_index(inplace=True)
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
            vecLabel_X=df_sample['RegionId'].to_list()
            dfHeatMap=df_sample[listNumRegion]

    df_sample=df1
    dfHeatMap=df1[listNumRegion]
    print('printing heatmap')
    print(dfHeatMap)
    vecLabel_Y=df1['Obj_name'].tolist()
    vecLabel_X=df1['Obj_name'].tolist()
    df_cols=dfHeatMap.columns.to_list()
    col_names=vecLabel_Y
    if(strApp == 'Minivite-v3'):
        col_names=['Graph_0','Map_0','Others','Comm','Graph_1','Map_1']
    elif (strApp =='ubench-array'):
        col_names=['E','D','C','B','A']
    for k in range(0,len(df_cols)):
        print(df_cols[k],vecLabel_Y[k])
        dfHeatMap.rename(columns={df_cols[k] : col_names[k]},inplace=True)

    print(dfHeatMap)
    df_cols=dfHeatMap.columns.to_list()
    df_cols.sort()
    dfHeatMap=dfHeatMap[df_cols]
    print(dfHeatMap)

    dfHeatMap.apply(pd.to_numeric)
    arSpHeatMap=np.empty([len(arRegionId),len(arRegionId)])
    arSpHeatMap= dfHeatMap.to_numpy()
    arSpHeatMap= arSpHeatMap.astype('float64')
    accessTotal =df_sample['Access count'].sum()
    arAccessPercent=(df_sample[['Access count']].div(accessTotal)).to_numpy()
    if(strApp=='ubench'):
        arBlk = df_sample[['Block count']].to_numpy()/256
    else:
        arBlk = df_sample[['Block count']].to_numpy()
    arBlk = arBlk.astype('int')

    arAccessBlk = np.divide(arAccessPercent,arBlk)
    arAccessBlk = arAccessBlk.astype('float64')
    fig, ax =plt.subplots(1,3, constrained_layout=True, figsize=(12, 10),gridspec_kw={'width_ratios': [9, 1,1]})
    sns.heatmap(arSpHeatMap,cmap='mako_r',cbar=False, annot=True, annot_kws = {'size':12}, ax=ax[0],vmin=0.0,vmax=1.0)
    ax[0].invert_yaxis()
    ax[0].set_yticklabels(vecLabel_Y,rotation='horizontal', wrap=True)
    #ax[0].set_yticklabels(['vec_5','vec_5','vec_4','vec_4','vec_1','vec_1','vec_3','vec_3','vec_2','vec_2','vec_6','vec_6'],rotation='horizontal', wrap=True)
    if(strApp=='ubench-array'):
        ax[0].set_xticklabels(df_cols,rotation='horizontal')
    else:
        ax[0].set_xticklabels(vecLabel_X,rotation='horizontal')
    #ax[0].set_xticklabels(['vec_5','vec_5','vec_4','vec_4','vec_1','vec_1','vec_3','vec_3','vec_2','vec_2','vec_6','vec_6'],rotation='horizontal', wrap=True)

    ax[0].set_title('Inter-region spatial density heatmap')

    sns.heatmap(arAccessPercent, cmap='PuBu', cbar=False,annot=True, fmt='.3f', annot_kws = {'size':12},  ax=ax[1],vmin=0.0,vmax=0.5)
    ax[1].invert_yaxis()
    ax[1].set_xticks([0])
    ax[1].set_yticks([0])
    #ax[1].set_yticklabels(vecLabel_Y,rotation='horizontal', wrap=True)
    ax[1].set_title('% Access')

    sns.heatmap(arBlk, cmap='Blues', cbar=False,annot=True, fmt='d', annot_kws = {'size':12},  ax=ax[2])
    ax[2].invert_yaxis()
    ax[2].set_xticks([0])
    ax[2].set_yticklabels(vecLabel_Y,rotation='horizontal', wrap=True)
    ax[2].yaxis.set_ticks_position('right')
    ax[2].set_title('# Pages')

    #sns.heatmap(arAccessBlk, cmap='BuGn', cbar=False,annot=True, fmt='.4f', annot_kws = {'size':12},  ax=ax[3])
    #ax[3].invert_yaxis()
    #ax[3].set_xticks([0])
    #ax[3].set_yticklabels(['vec_5','vec_5','vec_4','vec_4','vec_1','vec_1','vec_3','vec_3','vec_2','vec_2','vec_6','vec_6'],rotation='horizontal', wrap=True)
    #ax[3].yaxis.set_ticks_position('right')
    #ax[3].set_title('% Access / Block')
    rndAccess = np.round((accessTotal/1000).astype(float),2)
    plt.suptitle('Inter-region spatial density heatmap for '+strApp+'\n Total access - '+str(rndAccess)+'K', fontsize=14)

    imageFileName=filename[0:filename.rindex('/')]+'/'+strApp.replace(' ','-')+'-inter-sd.pdf'
    print(imageFileName)
    plt.show()
    #plt.savefig(imageFileName, bbox_inches='tight')

#filename='/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/spatial_density_detail_intra.txt'
def callPlot(plotApp):
    filename=''
    colSelect=0
    appName=''
    sampleSize=0
    if (plotApp=='ubench'):
        #filename='/Users/suri836/Projects/spatial_rud/ubench_simple_vector/sel_func/vec_gpp_exe-memgaze-trace-b8192-p40000/spatial_inter.txt'
        filename='/Users/suri836/Projects/spatial_rud/ubench_simple_array/paper_data/arr_exe-memgaze-trace-b16384-p250000/spatial_inter.txt'
        colSelect=0
        appName='ubench-array'
        sampleSize=0
    elif(plotApp=='Minivite-v3'):
        #filename='/Users/suri836/Projects/spatial_rud/minivite_detailed_look/spatial_clean/v3_inter_sd.txt'
        filename='/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v3_spatial_inter.txt'
        colSelect=0
        appName='Minivite-v3'
        sampleSize=0
    elif(plotApp=='Minivite-v2'):
        filename='/Users/suri836/Projects/spatial_rud/minivite_detailed_look/spatial_clean/v2_inter_sd.txt'
        colSelect=0
        appName='Minivite-v2'
        sampleSize=0
    elif(plotApp=='Minivite-v1'):
        #filename='/Users/suri836/Projects/spatial_rud/minivite_detailed_look/spatial_clean/v1_inter_sd.txt'
        filename='/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v1_spatial_inter.txt'
        colSelect=0
        appName='Minivite-v1'
        sampleSize=0
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
    spatialPlot(filename, colSelect, appName,sampleSize)

# For separate plots
#callPlot('AMG-inter')
#callPlot('AMG-inter-hot')
callPlot('ubench')
callPlot('Minivite-v3')
#callPlot('Minivite-v2')
#callPlot('Minivite-v1')



