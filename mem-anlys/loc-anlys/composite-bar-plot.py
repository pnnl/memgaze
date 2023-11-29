
#--- Info --- App miniVite-v1  Reg  1-A0000001              % SAbins  [6, 0, 1, 0, 7, 8, 1, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 4, 0, 8, 4, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0]
#--- Info --- App miniVite-v2  Reg  1-A0000010&4-A0002000  % SAbins  [24, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, 2, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0, 2, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2]
#--- Info --- App miniVite-v3  Reg  1-A0000001&5-A0001200  % SAbins  [14, 1, 1, 0, 0, 0, 1, 2, 1, 0, 1, 0, 1, 0, 0, 1, 4, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0]
#--- Info --- App miniVite-v1  Reg  1-A0000001              % SDbins  [7, 11, 0, 0, 0, 18, 3, 3, 1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
#--- Info --- App miniVite-v2  Reg  1-A0000010&4-A0002000  % SDbins  [0, 2, 3, 5, 2, 0, 0, 0, 2, 0, 2, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
#--- Info --- App miniVite-v3  Reg  1-A0000001&5-A0001200  % SDbins  [16, 5, 2, 2, 2, 2, 5, 1, 1, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
#grep 'Info' visual_log.txt_only_hot | grep 'App' | grep '% S.*\[' > composite-SI-SA-SI-SD.txt

import matplotlib.pyplot as plt
import numpy as np
indexSA = [0,5,10,15,20,25,30,35,40,45,50,100]
indexSD = [0,1,2,3,4,5,10,20,50,100]
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
        strKey = str(round(indexMetric[i]*0.01,2))+'-'+str(round(indexMetric[i+1]*0.01,2))
        #print(strKey)
        barLabels.append(strKey)
    print(barLabels)
    x = np.arange(len(barLabels))  # the label locations
    width = 0.25  # the width of the bars
    multiplier = 0
    fig, ax = plt.subplots(layout='constrained',figsize=(5, 4))
    for varVersion, numBlocks in dictValues.items():
        offset = width * multiplier
        rects = ax.bar(x + offset, numBlocks, width, label=varVersion)
        #ax.bar_label(rects, padding=3)
        multiplier += 1
    # Add some text for labels, title and custom x-axis tick labels, etc.
    #ax.set_ylabel('Length (mm)')
    ax.set_title("$\it{SI-"+strMetric+ "}$ distribution",fontsize=16)
    ax.set_xticks(x + width, barLabels,rotation='vertical')
    ax.legend(loc='upper center',ncol=len(dictValues.keys()))
    #ax.set_ylim(0, 250)
    #plt.show()
    strPath=strFileName[0:strFileName.rindex('/')]
    imageFileName=strPath+'/composite-SI-'+strMetric+'.pdf'
    print(imageFileName)
            #plt.show()
    plt.savefig(imageFileName, bbox_inches='tight')
    plt.close()

def read_file(strFileName, strMetric,dictValues):
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

strFileName='/Users/suri836/Projects/spatial_rud/spatial_pages_exp/miniVite/hot_lines/composite-SI-SA-SI-SD.txt'
dictSABins ={}
read_file(strFileName, 'SA', dictSABins)
print('dictSABins ', dictSABins)
plot_bar(strFileName, 'SA', dictSABins)
dictSDBins ={}
read_file(strFileName, 'SD', dictSDBins)
print('dictSDBins ', dictSDBins)
plot_bar(strFileName,'SD', dictSDBins)

strFileName='/Users/suri836/Projects/spatial_rud/spatial_pages_exp/XSBench/openmp-threading-noinline/memgaze-xs-sel-gs-2/composite-SI-SA-SI-SD.txt'
dictSABins ={}
read_file(strFileName, 'SA', dictSABins)
print('dictSABins ', dictSABins)
plot_bar(strFileName, 'SA', dictSABins)
dictSDBins ={}
read_file(strFileName, 'SD', dictSDBins)
print('dictSDBins ', dictSDBins)
plot_bar(strFileName,'SD', dictSDBins)
