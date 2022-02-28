#!/usr/bin/env python
# -*-Mode: python;-*-

import io

import pandas
import numpy
import math

import matplotlib.pyplot as pyplt
import matplotlib
#import locale

import seaborn


time_str_grappolo = u"""
graph        threads  mode   time          vtune

friendster	16	dram	6108.146365	nan
friendster	32	dram	3291.561375	nan
friendster	64	dram	1737.327125	nan
friendster	128	dram	1206.608416	nan
friendster	192	dram	815.121272	nan

friendster	16	mem	9049.91349	nan
friendster	32	mem	4712.569829	nan
friendster	64	mem	2395.902007	nan
friendster	128	mem	1472.592003	nan
friendster	192	mem	1001.308132	nan

friendster     16     kdax   6115.285544     nan
friendster     32     kdax   3277.245072     nan
friendster     64     kdax   1730.002233     nan
friendster    128     kdax   1217.872113     nan
friendster    192     kdax   831.718546   nan
"""


tm_index = [0,1,2] # graph threads type

time_data_grp = io.StringIO(time_str_grappolo)
time_dfrm_grp = pandas.read_csv(time_data_grp, sep='\s+', index_col=tm_index)
#print(time_dfrm_grp)


ln_sty1 = '-' # ':' # --
ln_sty2 = '--' # ':' # 
mrk_sty1 = 'o' # --
mrk_sty2 = 'x' # --

plt_sty1 = 'bright' # 'Set2' # 'muted' # 'bright'
plt_sty2 = 'deep' # 'dark'

fig, axesL = pyplt.subplots(nrows=1, ncols=2, figsize=(8, 3)) # squeeze=False


mode_dfrm = time_dfrm_grp.xs('friendster', level='graph')
ax = seaborn.lineplot(data=mode_dfrm, x='threads', y='time', hue='mode',
                      palette=plt_sty2, ax=axesL[0], marker=mrk_sty1, linestyle=ln_sty1)

ax = seaborn.scatterplot(data=mode_dfrm, x='threads', y='time', hue='mode',
                         palette=plt_sty2, ax=axesL[1] , marker=mrk_sty1, linestyle=ln_sty1)

pyplt.show()
