import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt

sns.color_palette("light:#5A9", as_cmap=True)
sns.set()

def spatialPlot(filename, strApp,strMetricType, colSelect=None, sampleSize=None):
    print(filename)
    range1 = [0,2]
    range2 = [i for i in range(4,14)]
    usecols_range = range1 + range2

    df=pd.read_table(filename, sep=" ", skipinitialspace=True, usecols=usecols_range,
                     names=['Metric-type', 'RegName', 'RegionId','colon', 'ar', 'Address Range', 'lf', 'Lifetime', 'ac', 'Access count', 'bc', 'Block count'])

    df.replace('***', 'SD',inplace=True)
    df.replace('===', 'SP',inplace=True)
    df.replace('---', 'SR',inplace=True)
    print('read file\n', df)
    npRegionId = df[df['Metric-type']==strMetricType]['RegionId'].values.flatten()
    arRegionId = npRegionId.tolist()
    print(arRegionId)

    df1=df[df['Metric-type']==strMetricType][['Metric-type', 'RegName', 'RegionId', 'Address Range', 'Lifetime', 'Access count', 'Block count']]
    df1.reset_index(inplace=True)
    listNumRegion = []
    listSDRegion =[]
    listSPRegion =[]
    listSRRegion =[]
    for i in range(0, len(arRegionId)):
        if (strMetricType=='SD'):
            listNumRegion.append('sd_'+str(arRegionId[i]))
        # Spatial probability and Spatial proximity provide better value when they are apired together
        elif (strMetricType=='SP'):
            listNumRegion.append('sp_'+str(arRegionId[i]))
        listSDRegion.append('sd_'+str(arRegionId[i]))
        listSPRegion.append('sp_'+str(arRegionId[i]))
        listSRRegion.append('sr_'+str(arRegionId[i]))
    df1[[listSDRegion]] =0
    df1[[listSPRegion]] =0
    df1[[listSRRegion]] =0
    print (listNumRegion)

    print('df1 columns\n', df1.columns.to_list)
    strTitle = '$SD$'
    if(strMetricType=='SD'):
        strTitle = '$SD$'
    elif(strMetricType=='SP'):
        strTitle = '$SA$ & $SI$'
    elif (strMetricType=='SR'):
        strTitle = '$SI$'
    line_identifier=['***','===','---']
    with open(filename) as f:
        indexCnt =0
        for fileLine in f:
            listSpatialDensity=np.empty(len(arRegionId))
            listSpatialDensity.fill(0)
            data = fileLine.strip().split(' ')
            #print(data)
            if((len(data) > 4) and (data[0] in line_identifier)):
                strRegName = data[2]
                if (data[0] == '***'):
                    strColIndicator='sd_'
                    listColumns = listSDRegion
                elif (data[0] == '==='):
                    strColIndicator='sp_'
                    listColumns = listSPRegion
                elif (data[0] == '---'):
                    strColIndicator='sr_'
                    listColumns = listSRRegion
                #print(data[0])
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
                    #df1.loc[[df['RegName'] == strRegName],strColIndicator+str(arRegionId[j])] = listSpatialDensity[j]
                    indexCnt =0
                for colName in (listColumns):
                    df1.loc[df['RegName'] == strRegName,colName] = listSpatialDensity [indexCnt]
                    indexCnt = indexCnt +1

    #print('after loop')
    print(df1.columns.to_list())
    print ('before sort' , df1['RegName'],df1['RegionId'])
    if(strApp.lower()=='minivite-v3-nuke'):
        obj_list = ['graph_0','map_0','others','comm','graph_1','map_1']
    elif(strApp=='ubench-array'):
        obj_list = ['E','D','C','B','A']
    elif(strApp.lower()=='minivite-v3'):
        obj_list=['graph','comm','others_0','others_1','map_0','map_1','map_2']
    else:
        obj_list = df1['RegName'].to_list()
    df1['Obj_name'] = obj_list
    print(df1['RegionId'])
    print(obj_list)
    dictRegObjMap = dict(zip(df1['RegionId'].to_list(), obj_list))
    print(dictRegObjMap)
    df1.sort_values(by='Access count',ascending=False, inplace=True)
    df1.reset_index(inplace=True,drop=True)
    print(listNumRegion)
    dfHeatMap=df1[listNumRegion]
    print('printing heatmap\n' ,dfHeatMap)
    vecLabel_X=df1['Obj_name'].tolist()
    df_cols=dfHeatMap.columns.to_list()
    print(dictRegObjMap)
    for k in range(0,len(df_cols)):
        print(df_cols[k],dictRegObjMap[int(df_cols[k][3:])])
        dfHeatMap.rename(columns={df_cols[k] : dictRegObjMap[int(df_cols[k][3:])]},inplace=True)
    vecLabel_Y=dfHeatMap.columns.to_list()
    print(vecLabel_X, vecLabel_Y)
    #Rearrange columns to match row order
    dfHeatMap = dfHeatMap[vecLabel_X]
    vecLabel_Y=dfHeatMap.columns.to_list()
    print(vecLabel_X, vecLabel_Y)
    print(dfHeatMap)

    dfHeatMapAdd = {}
    if(strMetricType =='SP'):
        dfHeatMapAdd=df1[listSRRegion]
        df_add_cols=dfHeatMapAdd.columns.to_list()
        for k in range(0,len(df_add_cols)):
            dfHeatMapAdd.rename(columns={df_add_cols[k] : dictRegObjMap[int(df_cols[k][3:])]},inplace=True)
        dfHeatMapAdd = dfHeatMapAdd[vecLabel_X]
        print(dfHeatMapAdd.columns.to_list())
        print(dfHeatMapAdd)


    dfHeatMap.apply(pd.to_numeric)
    arSpHeatMap=np.empty([len(arRegionId),len(arRegionId)])
    arSpHeatMap= dfHeatMap.to_numpy().transpose()
    arSpHeatMap= arSpHeatMap.astype('float64')
    print(arSpHeatMap)

    if (strMetricType =='SP'):
        arSpatialProximity=np.empty([len(arRegionId),len(arRegionId)])
        arSpatialProximity=dfHeatMapAdd.to_numpy().transpose()
        print(arSpatialProximity.size)
        print(arSpHeatMap.size)
        print(arSpHeatMap)
        print(arSpatialProximity)
        formatted_text = (np.asarray(["{0:.2f} ({1})".format(spValue,srValue) for spValue, srValue \
                                      in zip(arSpHeatMap.flatten(), arSpatialProximity.flatten())])).reshape(len(arRegionId),len(arRegionId))

    accessTotal =df1['Access count'].sum()
    arAccessPercent=(df1[['Access count']].div(accessTotal)).to_numpy() *100
    if(strApp=='ubench'):
        arBlk = df1[['Block count']].to_numpy()/256
    else:
        arBlk = df1[['Block count']].to_numpy()
    arBlk = arBlk.astype('int')

    arAccessBlk = np.divide(arAccessPercent,arBlk)
    arAccessBlk = arAccessBlk.astype('float64')
    fig, ax =plt.subplots(1,3, constrained_layout=True, figsize=(12, 10),gridspec_kw={'width_ratios': [9, 1,1]})
    if(strMetricType =='SP'):
        sns.heatmap(arSpHeatMap,cmap='mako_r',cbar=False, annot=formatted_text, fmt="", annot_kws = {'size':12}, ax=ax[0],vmin=0.0,vmax=1.0)
    else:
        sns.heatmap(arSpHeatMap,cmap='mako_r',cbar=False, annot=True, annot_kws = {'size':12}, ax=ax[0],vmin=0.0,vmax=1.0)

    ax[0].set_yticklabels(vecLabel_Y,rotation='horizontal', wrap=True)
    ax[0].set_xticklabels(vecLabel_X,rotation='vertical')

    sns.heatmap(arAccessPercent, cmap='Blues', cbar=False,annot=True, fmt='.1f', annot_kws = {'size':12},  ax=ax[1],vmin=0.0,vmax=100)
    #ax[1].invert_yaxis()
    ax[1].set_xticks([0])
    ax[1].set_yticks([0])

    sns.heatmap(arBlk, cmap='PuBu', cbar=False,annot=True, fmt='d', annot_kws = {'size':12},  ax=ax[2])
    #ax[2].invert_yaxis()
    ax[2].set_xticks([0])
    ax[2].set_yticklabels(vecLabel_Y,rotation='horizontal', wrap=True)
    ax[2].yaxis.set_ticks_position('right')

    #sns.heatmap(arAccessBlk, cmap='BuGn', cbar=False,annot=True, fmt='.4f', annot_kws = {'size':12},  ax=ax[3])
    #ax[3].set_xticks([0])
    rndAccess = np.round((accessTotal/1000).astype(float),2)
    #plt.suptitle('Inter-region '+strTitle+' heatmap for '+strApp+'\n Total access - '+str(rndAccess)+'K', fontsize=14)
    ax[0].set_title('Inter-region '+strTitle+' heatmap, total access : '+str(rndAccess)+'K')
    ax[1].set_title('% Access')
    ax[2].set_title('# Pages')


    imageFileName=filename[0:filename.rindex('/')]+'/'+strApp.replace(' ','-')+'-inter-'+strMetricType.lower()+'.pdf'
    print(imageFileName)
    #plt.show()
    plt.savefig(imageFileName, bbox_inches='tight')

#filename='/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/spatial_density_detail_intra.txt'
def callPlot(plotApp):
    filename=''
    colSelect=0
    appName=''
    sampleSize=0
    strMetricType='SP'

    if (plotApp.lower()=='ubench'):
        #filename='/Users/suri836/Projects/spatial_rud/ubench_simple_vector/sel_func/vec_gpp_exe-memgaze-trace-b8192-p40000/spatial_inter.txt'
        filename='/Users/suri836/Projects/spatial_rud/ubench_simple_array/paper_data/arr_exe-memgaze-trace-b16384-p250000/spatial_inter.txt'
        appName='ubench-array'
    elif(plotApp.lower()=='alpaca'):
        filename='/Users/suri836/Projects/spatial_rud/spatial_pages_exp/alpaca/mg-alpaca-noinline/chat-trace-b32768-p6000000-questions_copy/spatial_inter.txt'
        appName='Alpaca'
    elif(plotApp.lower()=='minivite-v3-nuke'):
        filename='/Users/suri836/Projects/spatial_rud/spatial_pages_exp/miniVite/hot_lines/v3_spatial_inter_sp_si_sd.txt'
        appName='minivite-v3'
    elif(plotApp.lower()=='minivite-v3'): # data from bignuke - arabic.bin graph
        filename='/Users/suri836/Projects/spatial_rud/spatial_pages_exp/miniVite/bignuke_run/mini-memgaze-ld/miniVite-v3-memgaze-trace-b16384-p5000000-anlys/v3_spatial_inter_sp_si_sd.txt'
        appName='minivite-v3'
    elif(plotApp.lower()=='minivite-v2'):
        filename='/Users/suri836/Projects/spatial_rud/minivite_detailed_look/spatial_clean/v2_inter_sd.txt'
        appName='miniVite-v2'
    elif(plotApp.lower()=='minivite-v1'):
        #filename='/Users/suri836/Projects/spatial_rud/minivite_detailed_look/spatial_clean/v1_inter_sd.txt'
        filename='/Users/suri836/Projects/spatial_rud/minivite_detailed_look/inter-region/v1_spatial_inter.txt'
        appName='miniVite-v1'
    elif(plotApp.lower()=='amg-inter'):
        filename='/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/spatial_heatmap.txt'
        colSelect=0
        appName='AMG-inter region'
        sampleSize=0
    elif(plotApplower()=='amg-inter-hot'):
        filename='/Users/suri836/Projects/spatial_rud/mg-amg_O3/amg-trace-b8192-p4000000/spatial_heatmap.txt'
        colSelect=0
        appName='AMG-inter region - Sampled 15 hot regions vs. all regions'
        sampleSize=15
    spatialPlot(filename, appName,strMetricType, colSelect,sampleSize)

# For separate plots
#callPlot('AMG-inter')
#callPlot('AMG-inter-hot')
#callPlot('ubench')
#callPlot('Minivite-v2')
#callPlot('Minivite-v1')
#Paper plots
callPlot('miniVite-v3')
#callPlot('Alpaca')


