import io



import pandas
import numpy
import math



import matplotlib.pyplot as pyplt
import matplotlib
#import locale
import seaborn
import VTuneCSV as vtcsv

time_str_grappolo = """
graph threads mode time vtune

friendster 16 dram 6108.146365 nan
friendster 32 dram 3291.561375 nan
friendster 64 dram 1737.327125 nan
friendster 128 dram 1206.608416 nan
friendster 192 dram 815.121272 nan
"""

time_data_grp = io.StringIO(time_str_grappolo)
time_dfrm_grp = pandas.read_csv(time_data_grp, sep='\s+', index_col=tm_index)
#print(time_dfrm_grp)


ax = seaborn.lineplot(data=dfrm_me, x='threads', y=y_metric,
hue='mode', ax=axes,
palette=plt_sty, marker=mrk_sty, linestyle=ln_sty)
