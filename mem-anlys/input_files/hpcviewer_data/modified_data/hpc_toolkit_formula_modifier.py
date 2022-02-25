#!/bin/python
import sys
import os

def main():   
   print("Hello!")  
   if len(sys.argv) !=  3:
      print "Usage: ./hpc_toolkit_formula_modifier.py <WordSize>  <XMLPath>"
      sys.exit()
   word = 4
   word = int(sys.argv[1]) 
   index = 0
   #Read Metrics
   inMetrics = False 
   MetricDict = {}
   file =  open(sys.argv[2], "r+") 
   for line in file:   
      if "<MetricTable>" in line:
         inMetrics = True
      if inMetrics:
         items = line.split()
         if "<Metric" == items[0]:
            key = "empty"
            value = "empty"
            for item in items:
               s1 = items[1].split("=")
               s2 = items[2].split("=")
               s3 = items[3]
               value = s1[1].strip("\"")
               key = s2[1]+s3
               key = key.strip("\"")
#               print "Value=",value," Key=",key
               MetricDict[key] = value
               index = int(value)
#         print line
      if "</MetricTable>" in line:
         print MetricDict
         inMetrics = False
         break
   
   
   #Write new formulas
   file.close()
   f1 =  open(sys.argv[2], "r") 
   lines = f1.readlines()
   f2 =  open(sys.argv[2], "w") 
   index +=1
   for line in lines:
      if "</MetricTable>" in line:
         #1 ~pf_wpl (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"npf_wpl (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"max(${}-${}-${}-${}-${}, ${})/${} \"/>\n".format(MetricDict['LOADS:Mean(I)'],MetricDict['L1_HIT:Mean(I)'],MetricDict['FB_HIT:Mean(I)'],MetricDict['L2_HIT:Mean(I)'],MetricDict['L3_HIT:Mean(I)'],MetricDict['L3_MISS:Mean(I)'],MetricDict['L3_MISS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["npf_wpl(I)"]=str(index)
         #1 ~pf_wpl (E)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"npf_wpl (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"max(${}-${}-${}-${}-${}, ${})/${} \"/>\n".format(MetricDict['LOADS:Mean(E)'],MetricDict['L1_HIT:Mean(E)'],MetricDict['FB_HIT:Mean(E)'],MetricDict['L2_HIT:Mean(E)'],MetricDict['L3_HIT:Mean(E)'],MetricDict['L3_MISS:Mean(E)'],MetricDict['L3_MISS:Mean(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["npf_wpl(E)"]=str(index+1)
         index+=2

         #2 pf_wpl_min (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"pf_wpl_min (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}+${}+${}+${}+(${}*${})+${}-${})/${} \"/>\n".format(MetricDict['L1_HIT:Mean(I)'],MetricDict['FB_HIT:Mean(I)'],MetricDict['L2_HIT:Mean(I)'],MetricDict['L3_HIT:Mean(I)'],MetricDict['L3_MISS:Mean(I)'],MetricDict['npf_wpl(I)'],MetricDict['PF_MISS:Mean(I)'],MetricDict['LOADS:Mean(I)'],MetricDict['PF_MISS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["pf_wpl_min(I)"]=str(index)
         #2 pf_wpl_min (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"pf_wpl_min (E)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}+${}+${}+${}+(${}*${})+${}-${})/${} \"/>\n".format(MetricDict['L1_HIT:Mean(E)'],MetricDict['FB_HIT:Mean(E)'],MetricDict['L2_HIT:Mean(E)'],MetricDict['L3_HIT:Mean(E)'],MetricDict['L3_MISS:Mean(E)'],MetricDict['npf_wpl(I)'],MetricDict['PF_MISS:Mean(E)'],MetricDict['LOADS:Mean(E)'],MetricDict['PF_MISS:Mean(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["pf_wpl_min(E)"]=str(index+1)
         index+=2
         
         #3 pf% (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"pf_percent (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}*${})/(${}+${}+(${}*${})+(${}*${})) \"/>\n".format(MetricDict['PF_MISS:Mean(I)'],MetricDict['pf_wpl_min(I)'],MetricDict['L2_HIT:Mean(I)'],MetricDict['L3_HIT:Mean(I)'],MetricDict['PF_MISS:Mean(I)'],MetricDict['pf_wpl_min(I)'],MetricDict['L3_MISS:Mean(I)'],MetricDict['npf_wpl(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["pf_percent(I)"]=str(index)      
         #3 pf% (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"pf_percent (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}*${})/(${}+${}+(${}*${})+(${}*${})) \"/>\n".format(MetricDict['PF_MISS:Mean(E)'],MetricDict['pf_wpl_min(E)'],MetricDict['L2_HIT:Mean(E)'],MetricDict['L3_HIT:Mean(E)'],MetricDict['PF_MISS:Mean(E)'],MetricDict['pf_wpl_min(E)'],MetricDict['L3_MISS:Mean(E)'],MetricDict['npf_wpl(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["pf_percent(E)"]=str(index+1)     
         index+=2
         
         #4 L1|strided (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"L1_strided (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}*${}) \"/>\n".format(MetricDict['pf_percent(I)'],MetricDict['L1_HIT:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["L1_strided(I)"]=str(index)      
         #4 L1|strided (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"L1_strided (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}*${}) \"/>\n".format(MetricDict['pf_percent(E)'],MetricDict['L1_HIT:Mean(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["L1_strided(E)"]=str(index+1)      
         index+=2

         #5 pf_wpl<=(pf%) (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"pf_wpl_max1 (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}/${}) \"/>\n".format(MetricDict['L1_strided(I)'],MetricDict['PF_MISS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["pf_wpl_max1(I)"]=str(index)      
         #5 pf_wpl<=(pf%) (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"pf_wpl_max1 (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}/${}) \"/>\n".format(MetricDict['L1_strided(E)'],MetricDict['PF_MISS:Mean(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["pf_wpl_max1(E)"]=str(index+1)      
         index+=2

         #5 pf_wpl<=(L1) (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"pf_wpl_max2 (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}/${}) \"/>\n".format(MetricDict['L1_HIT:Mean(I)'],MetricDict['PF_MISS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["pf_wpl_max2(I)"]=str(index)      
         #5 pf_wpl<=(L1) (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"pf_wpl_max2 (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}/${}) \"/>\n".format(MetricDict['L1_HIT:Mean(E)'],MetricDict['PF_MISS:Mean(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["pf_wpl_max2(E)"]=str(index+1)      
         index+=2

         #5 pf_wpl<=(line) (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"pf_wpl_max3 (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(64/{}) \"/>\n".format(str(word)))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["pf_wpl_max3(I)"]=str(index)      
         #5 pf_wpl<=(L1) (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"pf_wpl_max2 (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(64/{}) \"/>\n".format(str(word)))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["pf_wpl_max3(E)"]=str(index+1)      
         index+=2

         #6 pf_wpl* (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"pf_wpl (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"min(${},${},${}) \"/>\n".format(MetricDict['pf_wpl_max1(I)'],MetricDict['pf_wpl_max2(I)'],MetricDict['pf_wpl_max3(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["pf_wpl(I)"]=str(index)      
         #6 pf_wpl* (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"pf_wpl (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"min(${},${},${}) \"/>\n".format(MetricDict['pf_wpl_max1(E)'],MetricDict['pf_wpl_max2(E)'],MetricDict['pf_wpl_max3(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["pf_wpl(E)"]=str(index+1)      
         index+=2

         #7 Mem (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"Mem (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}-sum((${}-(${}*${})),${},${},${}) \"/>\n".format(MetricDict['LOADS:Mean(I)'],MetricDict['L1_HIT:Mean(I)'],MetricDict['PF_MISS:Mean(I)'],MetricDict['pf_wpl(I)'],MetricDict['FB_HIT:Mean(I)'],MetricDict['L2_HIT:Mean(I)'],MetricDict['L3_HIT:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["Mem(I)"]=str(index)      
         #7 Mem (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"Mem (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}-sum((${}-(${}*${})),${},${},${}) \"/>\n".format(MetricDict['LOADS:Mean(E)'],MetricDict['L1_HIT:Mean(E)'],MetricDict['PF_MISS:Mean(E)'],MetricDict['pf_wpl(E)'],MetricDict['FB_HIT:Mean(E)'],MetricDict['L2_HIT:Mean(E)'],MetricDict['L3_HIT:Mean(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["Mem(E)"]=str(index+1)      
         index+=2

         #7 Mem/npf (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"Mem_div_npf (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"max(${}-${}-${}-${}-${},${}) \"/>\n".format(MetricDict['LOADS:Mean(I)'],MetricDict['L1_HIT:Mean(I)'],MetricDict['FB_HIT:Mean(I)'],MetricDict['L2_HIT:Mean(I)'],MetricDict['L3_HIT:Mean(I)'],MetricDict['L3_MISS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["Mem_div_npf(I)"]=str(index)      
         #7 Mem/npf (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"Mem_div_npf (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"max(${}-${}-${}-${}-${},${}) \"/>\n".format(MetricDict['LOADS:Mean(E)'],MetricDict['L1_HIT:Mean(E)'],MetricDict['FB_HIT:Mean(E)'],MetricDict['L2_HIT:Mean(E)'],MetricDict['L3_HIT:Mean(E)'],MetricDict['L3_MISS:Mean(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["Mem_div_npf(E)"]=str(index+1)      
         index+=2

         #7 Mem/pf (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"Mem_div_pf (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}-${}) \"/>\n".format(MetricDict['Mem(I)'],MetricDict['Mem_div_npf(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["Mem_div_pf(I)"]=str(index)      
         #7 Mem/pf (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"Mem_div_pf (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}-${}) \"/>\n".format(MetricDict['Mem(E)'],MetricDict['Mem_div_npf(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["Mem_div_pf(E)"]=str(index+1)      
         index+=2

         #8 L1_hits Survival (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"L1_hit_surv (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}+${}+${}+${} \"/>\n".format(MetricDict['FB_HIT:Mean(I)'],MetricDict['L2_HIT:Mean(I)'],MetricDict['L3_HIT:Mean(I)'],MetricDict['Mem(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["L1_hit_surv(I)"]=str(index)      
         #8 L1_hits Survival (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"L1_hit_surv (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}+${}+${}+${} \"/>\n".format(MetricDict['FB_HIT:Mean(E)'],MetricDict['L2_HIT:Mean(E)'],MetricDict['L3_HIT:Mean(E)'],MetricDict['Mem(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["L1_hit_surv(E)"]=str(index+1)      
         index+=2

         #8 L2_hits Survival (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"L2_hit_surv (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}+${} \"/>\n".format(MetricDict['L3_HIT:Mean(I)'],MetricDict['Mem(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["L2_hit_surv(I)"]=str(index)      
         #8 L1_hits Survival (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"L2_hit_surv (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}+${} \"/>\n".format(MetricDict['L3_HIT:Mean(E)'],MetricDict['Mem(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["L2_hit_surv(E)"]=str(index+1)      
         index+=2

         #8 L3_hits Survival (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"L3_hit_surv (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${} \"/>\n".format(MetricDict['Mem(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["L3_hit_surv(I)"]=str(index)      
         #8 L3_hits Survival (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"L3_hit_surv (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${} \"/>\n".format(MetricDict['Mem(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["L3_hit_surv(E)"]=str(index+1)      
         index+=2

         #8 L3_hits/npf Survival (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"L3_hit_npf_surv (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${} \"/>\n".format(MetricDict['Mem_div_npf(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["L3_hit_npf_surv(I)"]=str(index)      
         #8 L3_hits Survival (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"L3_hit_npf_surv (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${} \"/>\n".format(MetricDict['Mem_div_npf(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["L3_hit_npf_surv(E)"]=str(index+1)      
         index+=2

         #9 im-L1 (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"im-L1 (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['LOADS:Mean(I)'],MetricDict['L1_hit_surv(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["im-L1(I)"]=str(index)      
         #9 im-L1 (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"im-L1 (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['LOADS:Mean(E)'],MetricDict['L1_hit_surv(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["im-L1(E)"]=str(index+1)      
         index+=2

         #9 im-L2 (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"im-L2 (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['LOADS:Mean(I)'],MetricDict['L2_hit_surv(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["im-L2(I)"]=str(index)      
         #9 im-L2 (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"im-L2 (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['LOADS:Mean(E)'],MetricDict['L2_hit_surv(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["im-L2(E)"]=str(index+1)      
         index+=2

         #9 im-L3 (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"im-L3 (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['LOADS:Mean(I)'],MetricDict['L3_hit_surv(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["im-L3(I)"]=str(index)      
         #9 im-L3 (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"im-L3 (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['LOADS:Mean(E)'],MetricDict['L3_hit_surv(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["im-L3(E)"]=str(index+1)      
         index+=2

         #9 im-L3_npf (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"im-L3_npf (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['LOADS:Mean(I)'],MetricDict['L3_hit_npf_surv(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["im-L3_npf(I)"]=str(index)      
         #9 im-L3_npf (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"im-L3_npf (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['LOADS:Mean(E)'],MetricDict['L3_hit_npf_surv(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["im-L3_npf(E)"]=str(index+1)      
         index+=2

         #10 fp_L3_npf (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"fp-L3_npf (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}*{})/${} \"/>\n".format(MetricDict['LOADS:Mean(I)'],str(word),MetricDict['im-L3_npf(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L3_npf(I)"]=str(index)      
         #10 fp_L3_npf (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"fp-L3_npf (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"(${}*{})/${} \"/>\n".format(MetricDict['LOADS:Mean(E)'],str(word),MetricDict['im-L3_npf(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L3_npf(E)"]=str(index+1)      
         index+=2

         #10 fp_L1 (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"fp-L1 (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}*{}/${} \"/>\n".format(MetricDict['LOADS:Mean(I)'],str(word),MetricDict['im-L1(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L1(I)"]=str(index)      
         #10 fp_L1 (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"fp-L1 (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}*{}/${} \"/>\n".format(MetricDict['LOADS:Mean(E)'],str(word),MetricDict['im-L1(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L1(E)"]=str(index+1)      
         index+=2

         #10 fp_L2 (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"fp-L2 (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}*{}/${} \"/>\n".format(MetricDict['LOADS:Mean(I)'],str(word),MetricDict['im-L2(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L2(I)"]=str(index)      
         #10 fp_L2 (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"fp-L2 (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}*{}/${} \"/>\n".format(MetricDict['LOADS:Mean(E)'],str(word),MetricDict['im-L2(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L2(E)"]=str(index+1)      
         index+=2

         #10 fp_L3 (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"fp-L3 (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}*{}/${} \"/>\n".format(MetricDict['LOADS:Mean(I)'],str(word),MetricDict['im-L3(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L3(I)"]=str(index)      
         #10 fp_L3 (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"fp-L3 (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}*{}/${} \"/>\n".format(MetricDict['LOADS:Mean(E)'],str(word),MetricDict['im-L3(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L3(E)"]=str(index)      
         index+=2

         #10 fp%L3 (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"fp-L3_percent (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['fp-L3_npf(I)'],MetricDict['fp-L3(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L3_percent(I)"]=str(index)      
         #10 fp%L3 (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"fp-L3_percent (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['fp-L3_npf(E)'],MetricDict['fp-L3(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L3_percent(E)"]=str(index)      
         index+=2

         #11 fp_L1_rate (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"fp-L1_rate (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['fp-L1(I)'],MetricDict['LOADS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L1_rate(I)"]=str(index)      
         #11 fp_L1_rate (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"fp-L1_rate (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['fp-L1(E)'],MetricDict['LOADS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L1_rate(E)"]=str(index+1)      
         index+=2

         #11 fp_L2_rate (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"fp-L2_rate (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['fp-L2(I)'],MetricDict['LOADS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L2_rate(I)"]=str(index)      
         #10 fp_L2_rate (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"fp-L2_rate (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['fp-L2(E)'],MetricDict['LOADS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L2_rate(E)"]=str(index+1)      
         index+=2

         #11 fp_L3_rate (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"fp-L3_rate (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['fp-L3(I)'],MetricDict['LOADS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L3_rate(I)"]=str(index)      
         #11 fp_L3_rate (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"fp-L3_rate (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['fp-L3(E)'],MetricDict['LOADS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L3_rate(E)"]=str(index+1)      
         index+=2

         #11 fp_L3_npf_rate (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"fp-L3_npf_rate (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['fp-L3_npf(I)'],MetricDict['LOADS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L3_npf_rate(I)"]=str(index)      
         #11 fp_L3_npf_rate (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"fp-L3_npf_rate (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['fp-L3_npf(E)'],MetricDict['LOADS:Mean(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-L3_npf_rate(E)"]=str(index+1)      
         index+=2

         #12 fp_npf_rate (I)
         f2.write("    <Metric i=\""+str(index)+"\" n=\"fp-npf_rate (I)\" v=\"derived-incr\" t=\"inclusive\" partner=\""+str(index+1)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['fp-L3_npf_rate(I)'],MetricDict['fp-L3_rate(I)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-npf_rate(I)"]=str(index)      
         #12 fp_npf_rate (E)
         f2.write("    <Metric i=\""+str(index+1)+"\" n=\"fp-npf_rate (E)\" v=\"derived-incr\" t=\"exclusive\" partner=\""+str(index)+"\" show=\"1\" show-percent=\"0\">\n")
         f2.write("        <MetricFormula t=\"finalize\" frm=\"${}/${} \"/>\n".format(MetricDict['fp-L3_npf_rate(E)'],MetricDict['fp-L3_rate(E)']))
         f2.write("        <Info><NV n=\"units\" v=\"events\"/></Info>\n")
         f2.write("    </Metric>\n")
         MetricDict["fp-npf_rate(E)"]=str(index+1)      
         index+=2
     
      
      f2.write(line)

#Test print
#   print MetricDict.get('Mem(I)')
#   print "\t<Metric i=\""+MetricDict['Mem(E)']+"\" n=\"Mem(E)\" v=\"derived-incr\" t=\"exclusive\" partner=\"100\" show=\"1\" show-percent=\"0\">"
#   print "\t\t<MetricFormula t=\"finalize\" frm=\"$62 + $42\"/>"
#   print "\t\t<Info><NV n=\"units\" v=\"events\"/></Info>"
#   print "\t </Metric>"
#   # MetricDict['']
   
   
#Clean up
   f1.close
   f2.close
   



if __name__== "__main__":   main()
