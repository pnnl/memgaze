import sys
import os
import subprocess
import argparse
import string

# Use for reading source line
# grep -n 79070 -B 5 miniVite_O3-v1_obj_nuke_line | grep '\/'
dictFnMap={}
dictFnIdentify ={}

def readFile(inFile, outFile,appName):
    variantFile=''
    if(outFile !=''):
        f_out = open(outFile, 'w')
        f_obj_c = open('obj_C_insn.txt','w') 
        f_obj_l = open('obj_L_insn.txt','w') 
    if(appName == 'alexnet'):
        #variantFile ='/files0/suri836/RUD_Zoom/alexnet_single/darknet_s8192_p1000000.trace.final'
        variantFile = '/files0/suri836/RUD_Zoom/resnet152_single/darknet_s8192_p1000000.trace.final'
    elif(appName == 'resnet'):
        variantFile = '/files0/suri836/RUD_Zoom/resnet152_single/darknet_s8192_p1000000.trace.final'
    elif(appName == 'minivite_v1'):
        variantFile = '../MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1.trace.final'
    elif(appName == 'minivite_v2'):
        variantFile = '../MiniVite_O3_v2_nf_func_8k_P5M_n300k/miniVite_O3-v2.trace.final'
    elif(appName == 'minivite_v3'):
        variantFile = '../MiniVite_O3_v3_nf_func_8k_P5M_n300k/miniVite_O3-v3.trace.final'
    elif(appName == 'gap_pr_spmv'):
        variantFile = '/files0/suri836/RUD_Zoom/GAP_PR_SPMV'
    elif(appName == 'gap_pr'):
        variantFile = '/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/pr_O3_s8192_p5000000.trace.final'
    elif(appName == 'gap_cc_sv'):
        variantFile = '/files0/suri836/RUD_Zoom/GAP_CC_SV'
    elif(appName == 'gap_cc'):
        variantFile = '/files0/suri836/RUD_Zoom/GAP_CC'
    elif(appName == 'vec_store'):
        variantFile = '/home/suri836/Projects/run_memgaze/spatial_ubench/vec_store_large/vec_gpp_st_no_frame_gh'
    elif(appName == 'vec_store_lm'):
        variantFile = '/home/suri836/Projects/run_memgaze/spatial_ubench/vec_store_large_check_linemap/vec_gpp_st_no_frame_gh'
    elif(appName == 'vec_store_lm_st'):
        variantFile = '/home/suri836/Projects/run_memgaze/spatial_ubench/vec_store_large_check_linemap/line_map'
    with open(inFile) as f:
        blFnMap=0
        varVersion =''
        rangeIndex = 0
        verRegionFn=''
        prevVersion=''
        strResult=''
        curRange=''
        for fileLine in f:
            processLine=0
            data = fileLine.strip().replace('\t',' ').split(' ')
            #print(data[0]+'---')
            if data[0] == '--insn':
                #print (data[2], data[0])
                if (variantFile==''):
                  variantFile = data[6]
                  curRange = data[9]
                  index = data[6].rindex('.trace')
                  varVersion = data[6][index-2:index]
                  print (variantFile, varVersion, curRange)
                if (appName == 'amg'): 
                  logFile = '/home/suri836/Projects/run_memgaze/amg/mg-amg_O3/amg_O3-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/amg/mg-amg_O3/obj_amg_O3'
                  objFile_C = ''
                  varVersion = 'amg: '
                if (appName == 'vec_store'): 
                  logFile = '/home/suri836/Projects/run_memgaze/spatial_ubench/vec_store_large/vec_gpp_st_no_frame_gh/vec_gpp_exe-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/spatial_ubench/vec_store_large/vec_gpp_st_no_frame_gh/obj_vec_gpp'
                  objFile_C = ''
                  varVersion = 'vs: '
                if (appName == 'sd_darknet_al'): 
                  logFile = '/home/suri836/Projects/run_memgaze/darknet/mg-darknet_O3-capture_ABC/darknet_O3-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/darknet/mg-darknet_O3-capture_ABC/obj_darknet_O3'
                  objFile_C = ''
                  varVersion = 'sd_al: '
                if (appName == 'sd_darknet_rs'): 
                  logFile = '/home/suri836/Projects/run_memgaze/darknet/mg-darknet_O3-capture_ABC/darknet_O3-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/darknet/mg-darknet_O3-capture_ABC/obj_darknet_O3'
                  objFile_C = ''
                  varVersion = 'sd_rd: '
                if (appName == 'sd_darknet_al_gemm'): 
                  logFile = '/home/suri836/Projects/run_memgaze/darknet/mg-darknet-gemm/darknet-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/darknet/mg-darknet-gemm/obj_darknet'
                  objFile_C = ''
                  varVersion = 'sd_al: '
                if (appName == 'sd_darknet_rs_gemm'): 
                  logFile = '/home/suri836/Projects/run_memgaze/darknet/mg-darknet-gemm/darknet-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/darknet/mg-darknet-gemm/obj_darknet'
                  objFile_C = ''
                  varVersion = 'sd_rs: '
                if (appName == 'lulesh'): 
                  logFile = '/home/suri836/Projects/run_memgaze/LULESH/mg-lulesh/lulesh2.0-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/LULESH/mg-lulesh/obj_lulesh'
                  objFile_C = ''
                  varVersion = 'll: '
                if (appName == 'spmm-mat'): 
                  logFile = '/home/suri836/Projects/run_memgaze/HiParTI/4096-same-iter/mg-spmm-mat/spmm_mat-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/HiParTI/4096-same-iter/mg-spmm-mat/obj_spmm'
                  objFile_C = ''
                  varVersion = 'sp: '
                if (appName == 'spmm-csr'): 
                  logFile = '/home/suri836/Projects/run_memgaze/HiParTI/4096-same-iter/mg-csr/spmm_csr_mat-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/HiParTI/4096-same-iter/mg-csr/obj_spmm_csr_mat'
                  objFile_C = ''
                  varVersion = 'csr: '
                if (appName == 'spmm-hicoo'): 
                  logFile = '/home/suri836/Projects/run_memgaze/HiParTI/4096-same-iter/mg-spmm-hicoo/spmm_hicoo_mat-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/HiParTI/4096-same-iter/mg-spmm-hicoo/obj_spmm_hicoo'
                  objFile_C = ''
                  varVersion = 'hic: '
                if (appName == 'p-mttkrp-hicoo'): 
                  logFile = '/home/suri836/Projects/run_memgaze/ParTI/mg-tensor/mttkrp_hicoo-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/ParTI/mg-tensor/obj_mttkrp_hicoo'
                  objFile_C = ''
                  varVersion = 'mtt-hic: '
                if (appName == 'p-mttkrp'): 
                  logFile = '/home/suri836/Projects/run_memgaze/ParTI/mg-tensor/mttkrp-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/ParTI/mg-tensor/obj_mttkrp'
                  objFile_C = ''
                  varVersion = 'mtt: '
                if (appName == 'p-mttkrp-sel'): 
                  logFile = '/home/suri836/Projects/run_memgaze/ParTI/mg-tensor/mttkrp-sel-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/ParTI/mg-tensor/obj_mttkrp'
                  objFile_C = ''
                  varVersion = 'mtt-sel: '
                if (appName == 'p-mttkrp-hicoo-sel'): 
                  logFile = '/home/suri836/Projects/run_memgaze/ParTI/mg-tensor/mttkrp_hicoo-sel-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/ParTI/mg-tensor/obj_mttkrp_hicoo'
                  objFile_C = ''
                  varVersion = 'mtt-hic: '
                if (appName == 'mttkrp-hicoo'): 
                  logFile = '/home/suri836/Projects/run_memgaze/HiParTI/mg-tensor/mttkrp_hicoo-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/HiParTI/mg-tensor/obj_mttkrp_hicoo'
                  objFile_C = ''
                  varVersion = 'mtt-hic: '
                if (appName == 'mttkrp-sel'): 
                  logFile = '/home/suri836/Projects/run_memgaze/HiParTI/mg-tensor/mttkrp-sel-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/HiParTI/mg-tensor/obj_mttkrp'
                  objFile_C = ''
                  varVersion = 'mtt-sel: '
                if (appName == 'mttkrp'): 
                  logFile = '/home/suri836/Projects/run_memgaze/HiParTI/mg-tensor/mttkrp-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/HiParTI/mg-tensor/obj_mttkrp'
                  objFile_C = ''
                  varVersion = 'mtt: '
                if (appName == 'mttkrp-reorder-sel'): 
                  logFile = '/home/suri836/Projects/run_memgaze/HiParTI/mg-tensor-reorder/mttkrp-sel-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/HiParTI/mg-tensor-reorder/obj_mttkrp_hicoo_reorder_matrixtiling'
                  objFile_C = ''
                  varVersion = 'mt-re-sel: '
                if (appName == 'mini-v1'): 
                  logFile = '/home/suri836/Projects/run_memgaze/minivite-x/v1-mg-ld/miniVite-v1-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/minivite-x/v1-mg-ld/obj_v1'
                  objFile_C = ''
                  varVersion = 'm-v1: '
                if (appName == 'mini-v2'): 
                  logFile = '/home/suri836/Projects/run_memgaze/minivite-x/mini-memgaze-ld/miniVite-v2-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/minivite-x/mini-memgaze-ld/obj_v2'
                  objFile_C = ''
                  varVersion = 'm-v2: '
                if (appName == 'mini-v3'): 
                  logFile = '/home/suri836/Projects/run_memgaze/minivite-x/mini-memgaze-ld/miniVite-v3-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/minivite-x/mini-memgaze-ld/obj_v3'
                  objFile_C = ''
                  varVersion = 'm-v3: '
                if (appName == 'alpaca'): 
                  logFile = '/home/suri836/Projects/run_memgaze/alpaca.cpp-81bd894/mg-alpaca-noAVX/chat-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/alpaca.cpp-81bd894/mg-alpaca-noAVX/obj_chat'
                  objFile_C = ''
                  varVersion = 'al: '
                if (appName == 'xsbench'): 
                  logFile = '/home/suri836/Projects/run_memgaze/XSBench/XSBench/openmp-threading/memgaze-xs/XSBench-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/XSBench/XSBench/openmp-threading/memgaze-xs/obj_XSBench'
                  objFile_C = ''
                  varVersion = 'xs: '
                if (appName == 'vec_store_lm'): 
                  logFile = '/home/suri836/Projects/run_memgaze/spatial_ubench/vec_store_large_check_linemap/vec_gpp_st_no_frame_gh/vec_gpp_exe-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/spatial_ubench/vec_store_large_check_linemap/vec_gpp_st_no_frame_gh/obj_vec_gpp'
                  objFile_C = ''
                  varVersion = 'vsl: '
                if (appName == 'vec_store_lm_st'): 
                  logFile = '/home/suri836/Projects/run_memgaze/spatial_ubench/vec_store_large_check_linemap/line_map/vec_gpp_exe-memgaze.binanlys'
                  objFile = '/home/suri836/Projects/run_memgaze/spatial_ubench/vec_store_large_check_linemap/line_map/obj_vec_gpp'
                  objFile_C = ''
                  varVersion = 'vsl: '
                if (variantFile == '../MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1.trace.final'):
                  logFile = '/files0/suri836/RUD_Zoom/minivite_create_filtered/MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1*.log'
                  objFile = '/files0/suri836/RUD_Zoom/minivite_create_filtered/MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1_obj_nuke_line' 
                  objFile_C = '/files0/suri836/RUD_Zoom/minivite_create_filtered/MiniVite_O3_v1_nf_func_8k_P5M_n300k/miniVite_O3-v1_obj_C' 
                  varVersion = 'V1: '
                if (variantFile == '../MiniVite_O3_v2_nf_func_8k_P5M_n300k/miniVite_O3-v2.trace.final'):
                  logFile = '/files0/suri836/RUD_Zoom/minivite_create_filtered/MiniVite_O3_v2_nf_func_8k_P5M_n300k/miniVite_O3-v2*.log'
                  objFile = '/files0/suri836/RUD_Zoom/minivite_create_filtered/MiniVite_O3_v2_nf_func_8k_P5M_n300k/miniVite_O3-v2_obj_nuke_line' 
                  objFile_C = '/files0/suri836/RUD_Zoom/minivite_create_filtered/MiniVite_O3_v2_nf_func_8k_P5M_n300k/miniVite_O3-v2_obj_C' 
                  varVersion = 'V2: '
                if (variantFile == '../MiniVite_O3_v3_nf_func_8k_P5M_n300k/miniVite_O3-v3.trace.final'):
                  logFile = '/files0/suri836/RUD_Zoom/minivite_create_filtered/MiniVite_O3_v3_nf_func_8k_P5M_n300k/miniVite_O3-v3*.log'
                  objFile = '/files0/suri836/RUD_Zoom/minivite_create_filtered/MiniVite_O3_v3_nf_func_8k_P5M_n300k/miniVite_O3-v3_obj_nuke_line' 
                  objFile_C = '/files0/suri836/RUD_Zoom/minivite_create_filtered/MiniVite_O3_v3_nf_func_8k_P5M_n300k/miniVite_O3-v3_obj_C' 
                  varVersion = 'V3: '
                if (variantFile == '/files0/suri836/RUD_Zoom/alexnet_10/darknet_s8192_p1000000.trace.final' or \
                    variantFile == '/files0/suri836/RUD_Zoom/alexnet_single/darknet_s8192_p1000000.trace.final' or \
                    variantFile == '/files0/suri836/RUD_Zoom/alexnet_single_corrected/darknet_s8192_p1000000.trace.final'):
                  logFile = '/files0/suri836/RUD_Zoom/alexnet_10/darknet.log' 
                  objFile = '/files0/suri836/RUD_Zoom/alexnet_10/darknet_obj' 
                  objFile_C =''
                  varVersion = 'a10: '
                if ('/files0/suri836/RUD_Zoom/alexnet_layers/' in variantFile ):
                  #/files0/suri836/RUD_Zoom/alexnet_layers/darknet_s8192_p1000000.trace.final_focused_0
                  logFile = '/files0/suri836/RUD_Zoom/alexnet_10/darknet.log' 
                  objFile = '/files0/suri836/RUD_Zoom/alexnet_10/darknet_obj' 
                  objFile_C =''
                  layer_index = variantFile.rindex('_')
                  varVersion = 'al'+variantFile[layer_index:]+': '
                if (variantFile == '/files0/suri836/RUD_Zoom/GAP_cc_O3_g22_nf_func_8k_P5M_n1_Avg/cc_O3_s8192_p5000000.trace.final' or \
                    variantFile == '/files0/suri836/RUD_Zoom/GAP_cc_O3_g22_nf_func_8k_P5M_n1_Avg/cc_O3_s8192_p5000000.trace.final_build' or \
                    variantFile == '/files0/suri836/RUD_Zoom/GAP_cc_O3_g22_nf_func_8k_P5M_n1_Avg/cc_O3_s8192_p5000000.trace.final_analysis' or \
                    variantFile == '/files0/suri836/RUD_Zoom/cc_O3_PTW-trace-b16384-p500000/cc_O3_PTW_rank.trace'):
                  logFile = '/files0/suri836/RUD_Zoom/GAP_cc_O3_g22_nf_func_8k_P5M_n1_Avg/cc_O3.log'
                  objFile = '/files0/suri836/RUD_Zoom/GAP_cc_O3_g22_nf_func_8k_P5M_n1_Avg/GAP_cc_O3_obj_nuke'
                  objFile_C ='/files0/suri836/RUD_Zoom/GAP_cc_O3_g22_nf_func_8k_P5M_n1_Avg/GAP_cc_O3_obj'
                  varVersion = 'gc: '
                if (variantFile == '/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/pr_O3_s8192_p5000000.trace.final' or \
                 variantFile == '/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/pr_O3_s8192_p5000000.trace.final_build' or \
                 variantFile == '/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/pr_O3_s8192_p5000000.trace.final_analysis' ):
                  logFile = '/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/pr_O3.log'
                  objFile = '/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/GAP_pr_O3_obj_nuke'
                  objFile_C ='/files0/suri836/RUD_Zoom/GAP_pr_O3_g22_nf_func_8k_P5M_n1_Avg/GAP_pr_O3_obj'
                  varVersion = 'gr: '
                if (variantFile == '/files0/suri836/RUD_Zoom/GAP_PR_SPMV' ):
                  logFile = '/files0/suri836/RUD_Zoom/GAP_PR_SPMV/pr_spmv_O3.log'
                  objFile = '/files0/suri836/RUD_Zoom/GAP_PR_SPMV/obj_pr_spmv_O3'
                  objFile_C ='/files0/suri836/RUD_Zoom/GAP_PR_SPMV/pr_spmv_O3.dump'
                  varVersion = 'pr_sp: '
                if (variantFile == '/files0/suri836/RUD_Zoom/GAP_CC_SV' ):
                  logFile = '/files0/suri836/RUD_Zoom/GAP_CC_SV/cc_sv_O3.log'
                  objFile = '/files0/suri836/RUD_Zoom/GAP_CC_SV/obj_cc_sv_O3'
                  objFile_C ='/files0/suri836/RUD_Zoom/GAP_CC_SV/cc_sv_O3.dump'
                  varVersion = 'cc_sv: '
                if (variantFile == '/files0/suri836/RUD_Zoom/GAP_CC' ):
                  logFile = '/files0/suri836/RUD_Zoom/GAP_CC_P100k_b16k/GAP_cc_O3_g22_nf_func_8k_P500k_n1_Avg/cc_O3.log'
                  objFile = '/files0/suri836/RUD_Zoom/GAP_CC_P100k_b16k/GAP_cc_O3_g22_nf_func_8k_P500k_n1_Avg/cc_O3.dump'
                  objFile_C ='/files0/suri836/RUD_Zoom/GAP_CC_P100k_b16k/GAP_cc_O3_g22_nf_func_8k_P500k_n1_Avg/obj_cc_O3'
                  varVersion = 'gap_cc: '
                if (variantFile == '/files0/suri836/RUD_Zoom/resnet_single/darknet_s8192_p1000000.trace.final' or \
                     variantFile == '/files0/suri836/RUD_Zoom/resnet152_10/darknet_s8192_p1000000.trace.final' ):
                  logFile = '/files0/suri836/RUD_Zoom/resnet152_10/darknet.log'
                  objFile = '/files0/suri836/RUD_Zoom/resnet152_10/darknet_obj'
                  objFile_C =''
                  varVersion = 'rs: '
                if ('/files0/suri836/RUD_Zoom/resnet_layers/' in variantFile): 
                  logFile = '/files0/suri836/RUD_Zoom/resnet152_10/darknet.log'
                  objFile = '/files0/suri836/RUD_Zoom/resnet152_10/darknet_obj'
                  objFile_C =''
                  layer_index = variantFile.rindex('_')
                  varVersion = 'rs'+variantFile[layer_index:]+': '
                if (variantFile == '/files0/suri836/RUD_Zoom/resnet152_single/darknet_s8192_p1000000.trace.final'): 
                  logFile = '/files0/suri836/RUD_Zoom/resnet152_single/darknet.log'
                  objFile = '/files0/suri836/RUD_Zoom/resnet152_10/darknet_obj'
                  objFile_C =''
                  varVersion = 'rs: '
                if (variantFile == '/files0/suri836/RUD_Zoom/resnet_worst_perf/darknet_s8192_p1000000.trace.final'): 
                  logFile = '/files0/suri836/RUD_Zoom/resnet_worst_perf/darknet.log'
                  objFile = '/files0/suri836/RUD_Zoom/resnet_worst_perf/darknet_obj'
                  objFile_C =''
                  varVersion = 'rs: '
                if (variantFile == '/files0/suri836/RUD_Zoom/alexnet_worst_perf/darknet_s8192_p1000000.trace.final'): 
                  logFile = '/files0/suri836/RUD_Zoom/alexnet_worst_perf/darknet.log'
                  objFile = '/files0/suri836/RUD_Zoom/alexnet_worst_perf/darknet_obj'
                  objFile_C =''
                  varVersion = 'al: '
                if(blFnMap==0):
                    blFnMap =1
                    command = 'grep 00000000 '+ objFile+ ' | grep "<"| cut -d " " -f 1-3'
                    try:
                        strFnMap= subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT,universal_newlines=True)
                    except subprocess.CalledProcessError as grepexc:
                        print("error code ", command, grepexc.returncode, grepexc.output)
                    print(command )
                    if('\n' in strFnMap):
                        print(strFnMap)
                        arrStrFnMap= strFnMap.split('\n')
                        for i in range(0,len(arrStrFnMap)):
                            arrFnMapData = arrStrFnMap[i].split(' ')
                            #print(arrFnMapData)
                            if((len(arrFnMapData)>1) and ((all(c in string.hexdigits for c in arrFnMapData[0]))==True) and (arrFnMapData[0] != '')):
                                if (len(arrFnMapData)==3):
                                    dictFnMap[arrFnMapData[0]] = arrFnMapData[1] + arrFnMapData[2]
                                else:
                                    dictFnMap[arrFnMapData[0]] = arrFnMapData[1]
                                print(arrFnMapData[0],  '***')
                                dictFnIdentify[int(arrFnMapData[0],16)] = arrFnMapData[0]
                    #print(dictFnMap)
                    #print(dictFnIdentify)
            elif data[0].isnumeric():
                strMapping=''
                strBinLine=''
                #print(data[0]+'---'+data[1])
                if(data[1] != ''):
                  varInst = data[1]
                else:
                  varInst = data[2]
                print ('file ', varInst)
                varInstgr=varInst[2:]
                if (appName == 'vec_store'): 
                  traceIns = int(str(varInst), 16)
                  dyninstBase = int(str(1000), 16)
                  logInst = traceIns - dyninstBase
                  print ('1 ', traceIns, dyninstBase, logInst )
                  varInstgr=hex(logInst)
                  print ('2 ', logInst, varInstgr)
                command = 'grep '+varInstgr+' '+logFile
                strMapping =''
                try:
                  strMapping = subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT,universal_newlines=True)
                except subprocess.CalledProcessError as grepexc:
                  print("log file error code ", command, grepexc.returncode, grepexc.output)
                if (strMapping !=''):
                  strMapping = str(strMapping.strip())
                  if('\n' in strMapping):
                    print(" 2 lines", strMapping)
                    arrStrMap= strMapping.split('\n')
                    for strCorrectMap in arrStrMap:
                      if('0x' not in strCorrectMap):
                        strMapping = strCorrectMap
                  #print(strMapping)
                  grData=strMapping.split(' ')
                  #print (varInst, grData[5])
                  command = 'grep ' + grData[5] + ': '+objFile
                  strBinLine =''
                  try:
                    strBinLine = subprocess.check_output(command, shell=True)
                  except subprocess.CalledProcessError as grepexc:
                    print("objFile error code ", strMapping, command, grepexc.returncode, grepexc.output)
                  if(strBinLine !=''):
                    strBinLine = str(strBinLine.strip())
                    strBinLine1 = strBinLine.replace('\t','_').replace(' ','_')
                    strBinLine1 = strBinLine1[2:]
                    #print(strBinLine1)
                    if(objFile_C != ''):
                      command = 'grep ' + grData[5] + ': '+objFile_C
                      strBinLine_C =''
                      try:
                        strBinLine_C = subprocess.check_output(command, shell=True)
                      except subprocess.CalledProcessError as grepexc:
                        print("objFile_C error code ", strMapping, command, grepexc.returncode, grepexc.output)
                      strBinLine_C = str(strBinLine.strip())
                      if(strBinLine != strBinLine_C):
                        print ('Error ' , varVersion, varInst, grData[5], strBinLine, strBinLine_C)
                    command = 'grep -B 10 '+grData[5]+': '+objFile +' | grep \/'
                    strResult=''
                    try:
                      strResult = subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT,universal_newlines=True)
                    except subprocess.CalledProcessError as grepexc:
                      print("error code ", command, grepexc.returncode, grepexc.output)
                    if (strResult !=''):
                      strResult = str(strResult.strip())
                    if('\n' in strResult):
                      arrStrMap= strResult.split('\n')
                      print("**** Info  more than 1 lines")
                      print(command)
                      strResult = arrStrMap[len(arrStrMap)-1]
                    processLine = 1
                    if(objFile_C != ''):
                      f_obj_c.write( strBinLine_C)
                      f_obj_c.write('\n')
                    f_obj_l.write( strBinLine)
                    f_obj_l.write('\n')
                    fnStartIP=''
                    for key in sorted(dictFnIdentify):
                        #print( 'key ', int(key), "int(grData[5],16)", int(grData[5],16), "int(grData[5],16)) < int(key)", ((int(grData[5],16)) < int(key)))
                        if ((int(grData[5],16)) < key):
                            continue
                        else:
                            #print("ELSE", key)
                            fnStartIP = dictFnIdentify[key]
                            fnName = dictFnMap[fnStartIP]
                  writeLine = varVersion+' '+data[0]+ ' '+varInstgr+' '+ grData[5] + ' '+fnStartIP+ ' '+fnName+' '+ strResult+' \n'
            if(processLine ==0):
              f_out.write(fileLine)
            else:
              f_out.write(writeLine) 

        f.close()
        f_out.close()
        f_obj_c.close()
        f_obj_l.close()

n = len(sys.argv)
print("\nArguments passed:", end = " ")
for i in range(1, n):
    print(sys.argv[i], end = " ")

parser = argparse.ArgumentParser()
parser.add_argument('--i', type=str, required=True, help='Intput file')
parser.add_argument('--o', type=str, required=True, help ='Output File')
parser.add_argument('--app', type=str , help='Application Name to check for binary')
args = parser.parse_args()
readFile(args.i, args.o, args.app)




