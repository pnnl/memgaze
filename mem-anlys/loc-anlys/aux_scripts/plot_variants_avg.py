import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import re
import csv
import os

get_col_list=[None]*511
for i in range ( 0,255):
    get_col_list[i]='self'+'-'+str(255-i)
get_col_list[255]='self'
for i in range ( 1,256):
    get_col_list[255+i]='self'+'+'+str(i)
print(get_col_list)
if (1 ==0 ):
    data_avg=pd.read_csv('/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/average.csv',index_col='App-Metric')
    print(data_avg)
    df_sd1=data_avg.loc[(data_avg['Metric'] == 'SP') & (data_avg['type'] == 'value')]
    print(df_sd1)
    df_sd = df_sd1[get_col_list]
    print(df_sd)

    df_sd_plot=df_sd.T
    print(df_sd_plot)
    col_names=list(df_sd_plot)
    print(col_names)
    lexi_bfs=[None]*2
    lexi_bfs[0]='HiParTI-HiCOO-Lexi-SP'
    lexi_bfs[1]='HiParTI-HiCOO-BFS-SP'

    #df_lexi_bfs=df_sd_plot[lexi_bfs]
    #fig = plt.figure(figsize=(8,6))
    #ax=sns.lineplot(data=df_sd_plot)
    #ax.set_yscale('log')
    #ax.set(ylim=(-0.2,1.0))
    #ax=df_sd_plot.plot.line()
    #imageFileName='/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/average-line.pdf'
    #plt.show()

with open("/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/SP-SR.csv", 'r') as temp_f:
    # get No of columns in each line
    col_count = [ len(l.split(",")) for l in temp_f.readlines() ]

### Generate column names  (names will be 0, 1, 2, ..., maximum columns - 1)
column_names = [i for i in range(0, max(col_count))]

### Read csv
data_avg=pd.read_csv('/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/SP-SR.csv', header=None, delimiter=",", names=column_names)
data_avg.rename(columns={0: "App-Metric", 1: "Metric", 2:"location"},inplace=True)
print(data_avg)

df_sd1=data_avg.loc[(data_avg['Metric'] == 'SP')]
df_sd1['Variant-loc']= df_sd1['App-Metric']+'-'+df_sd1['location']
df_sd1.set_index('Variant-loc')
df_sd1.sort_index(inplace=True)
print(df_sd1)

col_list=df_sd1.columns.to_list()
print(col_list)
col_list.pop(0)
col_list.pop(0)
col_list.pop(0)
print(col_list)
col_list.insert(0,'Variant-loc')
col_list.pop()
df_sd1=df_sd1[col_list]

print('before drop size', df_sd1.shape)
for i in range(1,len(col_list)):
    count_na=pd.to_numeric(df_sd1[col_list[i]]).count()
    if(count_na >20):
        df_sd1.drop([col_list[i]],axis=1,inplace=True)

print('after drop size', df_sd1.shape)
col_list=df_sd1.columns.to_list()

print('final ', col_list)

df_plt=df_sd1[col_list]
df_plt.set_index('Variant-loc',inplace=True)

print(df_plt)
df_final_plot=df_plt.T

print(df_final_plot)

#fig = plt.figure(figsize=(12,12))
#ax=sns.lineplot(data=df_plt)
#ax=df_plt.plot.line()
#plt.show()
col_list.pop(0)
x=col_list
print ('x' , x)
final_col_list = df_final_plot.columns.to_list()
print(final_col_list)
print((df_final_plot[final_col_list[0]]).to_list())

marker_shape=['x','o','*','.']
# Constrained layout makes sure the labels don't overlap the axes.
fig, (ax0, ax1, ax2,ax3) = plt.subplots(nrows=4, constrained_layout=True,figsize=(12,10))
color_line=''
color_blue=['tab:blue','powderblue', 'slateblue','deepskyblue','navy','darkslateblue','dodgerblue']

#blue_itr=iter(color_blue)
color_red=['tab:red','lightcoral','indianred','coral','crimson','lightsalmon','tab:orange']
#red_iter=iter(color_red)
#color_blue='tab:blue'
#color_red='tab:orange'

for j in range (0, len(final_col_list)):
    if(('Lexi' in final_col_list[j] or 'BFS' in final_col_list[j])):

        if('Lexi') in final_col_list[j]:
            color_line=color_blue[j%len(color_blue)]
        if('BFS') in final_col_list[j]:
            color_line=color_red[j%len(color_red)]
            #color_line=next(red_iter)
        print(final_col_list[j], color_line)
        print(final_col_list[j])
        if(final_col_list[j].endswith('self') ):
            ax0.plot(x, df_final_plot[final_col_list[j]].to_list(),color_line, label=final_col_list[j]) #,marker=marker_shape[j%len(marker_shape)]
            ax0.legend(loc='lower left')
            ax0.set_xlabel('Observed blocks in 10 hottest pages of region')
            ax0.set_ylabel('SP for self')
            #ax0.set_ylim(-4,20)
        if('1' in final_col_list[j]):
            ax1.plot(x, df_final_plot[final_col_list[j]].to_list(),color_line,label=final_col_list[j]) #marker='x', markersize='1.5', linestyle=''
            ax1.legend(loc='upper left')
            ax1.set_xlabel('Observed blocks in 10 hottest pages of region')
            ax1.set_ylabel('SP for self+1 and self-1')
            #ax1.set_ylim(-4,20)
        if('2' in final_col_list[j]):
            ax2.plot(x, df_final_plot[final_col_list[j]].to_list(),color_line,label=final_col_list[j])
            ax2.legend(loc='upper left')
            ax2.set_xlabel('Observed blocks in 10 hottest pages of region')
            ax2.set_ylabel('SP for self+2 and self-2')
            #ax2.set_ylim(-4,20)
        if('3' in final_col_list[j]):
            ax3.plot(x, df_final_plot[final_col_list[j]].to_list(),color_line,label=final_col_list[j])
            ax3.legend(loc='lower left')
            ax3.set_xlabel('Observed blocks in 10 hottest pages of region')
            ax3.set_ylabel('SP for self+3 and self-3')
            #ax3.set_ylim(-4,20)
plt.legend()
plt.suptitle('Spatial Probability for HiParTI-HiCOO-MTTKRP reordering variants - Lexi and BFS')
#plt.xlabel('x label', fontsize=14)
#plt.ylabel('y label', fontsize=14)
#plt.show()
imageFileName='/Users/suri836/Projects/spatial_rud/HiParTi/mg-tensor-reorder/nell-U-0/Lexi-BFS-SP.pdf'
plt.savefig(imageFileName, bbox_inches='tight')
plt.close()
