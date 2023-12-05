
#--- Info --- App miniVite-v1  Reg  1-A0000001              % SAbins  [6, 0, 1, 0, 7, 8, 1, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 4, 0, 8, 4, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0]
#--- Info --- App miniVite-v2  Reg  1-A0000010&4-A0002000  % SAbins  [24, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, 2, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0, 2, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2]
#--- Info --- App miniVite-v3  Reg  1-A0000001&5-A0001200  % SAbins  [14, 1, 1, 0, 0, 0, 1, 2, 1, 0, 1, 0, 1, 0, 0, 1, 4, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0]
#--- Info --- App miniVite-v1  Reg  1-A0000001              % SDbins  [7, 11, 0, 0, 0, 18, 3, 3, 1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
#--- Info --- App miniVite-v2  Reg  1-A0000010&4-A0002000  % SDbins  [0, 2, 3, 5, 2, 0, 0, 0, 2, 0, 2, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
#--- Info --- App miniVite-v3  Reg  1-A0000001&5-A0001200  % SDbins  [16, 5, 2, 2, 2, 2, 5, 1, 1, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
#grep 'Info' visual_log.txt_only_hot | grep 'App' | grep '% S.*\[' > composite-SI-SA-SI-SD.txt

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

indexSA = [0,5,10,15,20,25,30,35,40,45,50,100]
indexSD = [0,1,2,3,4,5,10,20,50,100]
indexSABinMark=25
indexSDBinMark=5
indexSABinMark=0
indexSDBinMark=0
col_names=[None]*(101)
col_names[0]='App'
for i in range (1,101):
    col_names[i]= str(round((i)*0.01,2))

if (0):
    # data from https://allisonhorst.github.io/palmerpenguins/
    species = ("Adelie", "Chinstrap", "Gentoo")
    penguin_means = {
        'Bill Depth': (18.35, 18.43, 14.98),
        'Bill Length': (38.79, 48.83, 47.50),
        'Flipper Length': (189.95, 195.82, 217.19),
        'height': (20,30,40),
    }
    x = np.arange(len(species))  # the label locations
    width = 0.05  # the width of the bars
    multiplier = 0
    fig, ax = plt.subplots(layout='constrained')
    for attribute, measurement in penguin_means.items():
        offset = width * multiplier
        rects = ax.bar(x + offset, measurement, width, label=attribute)
        ax.bar_label(rects, padding=0)
        multiplier += 1
    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax.set_ylabel('Length (mm)')
    ax.set_title('Penguin attributes by species')
    ax.set_xticks(x + width, species)
    ax.legend(loc='upper left',ncol=3)
    ax.set_ylim(0, 250)
    plt.show()

def plot_bar(strFileName,strMetric,dictValues ):
    print(dictValues)
    if(strMetric == 'SA'):
        print(indexSA)
        indexMetric=indexSA
    else:
        indexMetric=indexSD
    barLabels=[]
    for i in range(0, len(indexMetric)-1):
        #strKey = str(round(indexMetric[i]*0.01,2))+'-'+str(round(indexMetric[i+1]*0.01,2))
        strKey = str(round(indexMetric[i+1]*0.01,2))
        barLabels.append(strKey)
    print('barLabels ', barLabels)
    x = np.arange(len(barLabels))  # the label locations
    light_colors=['azure','coral','lightgreen']
    dark_colors=['blue','orange','green']

    width = 0.25  # the width of the bars
    multiplier = 0
    fig, ax = plt.subplots(layout='constrained',figsize=(4, 3))
    for varVersion, numBlocks in dictValues.items():
        offset = width * multiplier
        print('multiplier ', multiplier, ' offset ', offset, ' x ', x)
        rects = ax.bar(x + offset, numBlocks, width, label=varVersion)
        multiplier += 1

    # Add some text for labels, title and custom x-axis tick labels, etc.
    #ax.set_ylabel('Length (mm)')
    ax.set_title("$\it{SI}$-$\it{"+strMetric+ "}$ cumulative distribution",fontsize=14)
    ax.set_xticks(x + width, barLabels,rotation='vertical')
    ymin, ymax = ax.get_ylim()
    if(strMetric=='SA'):
        ax.axvline(x=(5-(width)),  linewidth=1, linestyle='dashed', color='black')
    else:
        ax.axvline(x=(5-(width)),  linewidth=1, linestyle='dashed', color='black')
    ax.legend(loc='upper left')#,ncol=len(dictValues.keys()))
    ax.set_ylabel('Percentage of block pairs')
    ax.set_xlabel("Composite $\it{SI}$-$\it{"+strMetric+ "}$")
    ax.xaxis.set_tick_params(rotation=45)

    strPath=strFileName[0:strFileName.rindex('/')]
    imageFileName=strPath+'/composite-SI-'+strMetric+'.pdf'
    print(imageFileName)
    #plt.show()
    plt.savefig(imageFileName, bbox_inches='tight')
    plt.close()

def plot_lines(strFileName,strMetric,dictValues):
    print(dictValues)
    if(strMetric == 'SA'):
        print(indexSA)
        indexMetric=indexSA
    else:
        indexMetric=indexSD
    xLabels=[]
    for i in range(0, len(indexMetric)-1):
        strKey = str(round(indexMetric[i+1]*0.01,2))
        xLabels.append(strKey)
    print(xLabels)
    fig, ax = plt.subplots(layout='constrained',figsize=(5, 4))
    ax.set_title("$\it{SI}$-$\it{"+strMetric+ "}$ distribution",fontsize=16)
    #ax.set_xticks(xLabels ,rotation='vertical')
    my_colors=['red','green','blue']
    i=0
    for key, value in dictValues.items():
        ax.plot(xLabels, value, color=my_colors[i],label=key)
        i = i+1
    ax.legend(loc='upper center',ncol=len(dictValues.keys()))
    for ticklabel in ax.get_xticklabels():
        print(ticklabel.get_text())
    ymin, ymax = ax.get_ylim()
    ax.vlines(x='0.25', ymin=ymin, ymax=ymax, linewidth=1, color='black')
    plt.show()
    strPath=strFileName[0:strFileName.rindex('/')]
    imageFileName=strPath+'/composite-SI-'+strMetric+'-plot.pdf'
    print(imageFileName)
    plt.savefig(imageFileName, bbox_inches='tight')
    plt.close()


def plot_cdf(strFileName,strMetric,dfValues ):
    print(dfValues)
    fig, ax = plt.subplots(layout='constrained',figsize=(5, 4))
    ax.set_title("$\it{SI}$-$\it{"+strMetric+ "}$ distribution",fontsize=16)
    penguins = sns.load_dataset("penguins")
    print(penguins[["species", "flipper_length_mm"]])
    sns.ecdfplot(data=penguins, y="flipper_length_mm")
    #sns.ecdfplot(data=dfValues[col_names[1:]], x=col_names[1:], hue="App")
    plt.show()
    strPath=strFileName[0:strFileName.rindex('/')]
    imageFileName=strPath+'/composite-SI-'+strMetric+'-cdf.pdf'
    print(imageFileName)
    #plt.savefig(imageFileName, bbox_inches='tight')
    plt.close()

def read_file_dict(strFileName, strMetric,dictValues):
     with open(strFileName) as f:
        for fileLine in f:
            if (strMetric in fileLine):
                lineSplit = fileLine.strip().split(' ')
                strApp = lineSplit[4]
                strbinData = fileLine[fileLine.index('[')+1: fileLine.index(']')]
                arrbinData = np.fromstring(strbinData, dtype=int, sep=',')
                if(strMetric == 'SA'):
                    indexMetric=indexSA
                else:
                    indexMetric=indexSD
                arrBins = np.empty(len(indexMetric)-1)
                for i in range(0,len(indexMetric)-1):
                    #print(arrbinData[indexMetric[i]:indexMetric[i+1]])
                    arrBins[i] = np.sum(arrbinData[indexMetric[i]:indexMetric[i+1]])
                #arrBins[i+1] = np.sum(arrbinData[indexMetric[i+1]:])
                #print( strApp,  arrbinData)
                dictValues[strApp] = arrBins

def read_file_cum_dict(strFileName, strMetric,dictValues):
     with open(strFileName) as f:
        for fileLine in f:
            if (strMetric in fileLine):
                indexBinMarker=0
                startIndex=0
                lineSplit = fileLine.strip().split(' ')
                strApp = lineSplit[4]
                strbinData = fileLine[fileLine.index('[')+1: fileLine.index(']')]
                arrbinData = np.fromstring(strbinData, dtype=int, sep=',')
                if(strMetric == 'SA'):
                    indexMetric=indexSA
                    indexBinMarker=indexSABinMark
                else:
                    indexMetric=indexSD
                    indexBinMarker=indexSDBinMark
                print(arrbinData)
                arrBins = np.empty(len(indexMetric)-1)
                for i in range(0,len(indexMetric)-1):
                    if(indexMetric[i]<indexBinMarker):
                        startIndex=0
                    else:
                        startIndex=indexBinMarker
                    #print('i ', i, ' start ', startIndex, ' end  ', indexMetric[i+1], ' values ', arrbinData[startIndex:indexMetric[i+1]])
                    arrBins[i] = np.sum(arrbinData[startIndex:indexMetric[i+1]])
                #arrBins[i+1] = np.sum(arrbinData[indexMetric[i+1]:])
                #print( strApp,  arrbinData)
                dictValues[strApp] = arrBins

def read_file_df(strFileName, strMetric,listValues):
     with open(strFileName) as f:
        for fileLine in f:
            if (strMetric in fileLine):
                listLine=[]
                lineSplit = fileLine.strip().split(' ')
                strApp = lineSplit[4]
                strbinData = fileLine[fileLine.index('[')+1: fileLine.index(']')]
                arrbinData = np.fromstring(strbinData, dtype=int, sep=',')
                listLine.insert(0,strApp)
                #print(arrbinData.tolist())
                listLine.extend(arrbinData.tolist())
                #dfValues['App'] = strApp
                print(listLine)
                listValues.append(listLine)
     print(listValues)

strFileName='/Users/suri836/Projects/spatial_rud/spatial_pages_exp/miniVite/hot_lines/composite-SI-SA-SI-SD.txt'
dictSABins ={}
if (1):
    read_file_dict(strFileName, 'SA', dictSABins)
    print('dictSABins ', dictSABins)
    plot_bar(strFileName, 'SA', dictSABins)
    dictSABins ={}
    read_file_cum_dict(strFileName, 'SA', dictSABins)
    print('dictSABins ', dictSABins)
    plot_bar(strFileName, 'SA', dictSABins)
    #dictSABins ={}
    #read_file_cum_dict(strFileName, 'SA', dictSABins)
    #print('dictSABins CUM ', dictSABins)
    #plot_lines(strFileName, 'SA', dictSABins)

if(1):
    dictSDBins ={}
    read_file_cum_dict(strFileName, 'SD', dictSDBins)
    print('dictSDBins ', dictSDBins)
    plot_bar(strFileName,'SD', dictSDBins)

if(0):
    dfSDBins=pd.DataFrame()
    listValue=[]
    read_file_df(strFileName, 'SD', listValue)
    print('main', listValue)
    dfSDBins=pd.DataFrame(listValue,columns=col_names)
    print(dfSDBins)
    plot_cdf(strFileName,'SD', dfSDBins)


if(0):
    strFileName='/Users/suri836/Projects/spatial_rud/spatial_pages_exp/XSBench/openmp-threading-noinline/memgaze-xs-sel-gs-2/composite-SI-SA-SI-SD.txt'
    dictSABins ={}
    read_file_dict(strFileName, 'SA', dictSABins)
    print('dictSABins ', dictSABins)
    plot_bar(strFileName, 'SA', dictSABins)
    dictSDBins ={}
    read_file_dict(strFileName, 'SD', dictSDBins)
    print('dictSDBins ', dictSDBins)
    plot_bar(strFileName,'SD', dictSDBins)
