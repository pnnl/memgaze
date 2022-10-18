# intel-pt-events.py: Print Intel PT Power Events and PTWRITE
# Copyright (c) 2017, Intel Corporation.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.

from __future__ import print_function

import os
import sys
import struct
import pefile
import collections

sys.path.append(os.environ['PERF_EXEC_PATH'] + \
	'/scripts/python/Perf-Trace-Util/lib/Perf/Trace')
# These perf imports are not used at present
#from perf_trace_context import *
#from Core import *

#Counter for each sample window
sample_cnt = collections.Counter()

def trace_begin():
    a="hello"
	#print("Intel PT Power Events and PTWRITE")

def trace_end():
    b="hello"
	#print("End")

def trace_unhandled(event_name, context, event_fields_dict):
    print(' '.join(['%s=%s'%(k,str(v))for k,v in sorted(event_fields_dict.items())]))

def print_ptwrite(raw_buf):
	data = struct.unpack_from("<IQ", raw_buf)
	flags = data[0]
	payload = data[1]
	exact_ip = flags & 1
	print( " %#x" %  (payload), end=' ')

def print_cbr(raw_buf):
	data = struct.unpack_from("<BBBBII", raw_buf)
	cbr = data[0]
	f = (data[4] + 500) / 1000
	p = ((cbr * 1000 / data[2]) + 5) / 10
	print("%3u  freq: %4u MHz  (%3u%%)" % (cbr, f, p), end=' ')

def print_mwait(raw_buf):
	data = struct.unpack_from("<IQ", raw_buf)
	payload = data[1]
	hints = payload & 0xff
	extensions = (payload >> 32) & 0x3
	print("hints: %#x extensions: %#x" % (hints, extensions), end=' ')

def print_pwre(raw_buf):
	data = struct.unpack_from("<IQ", raw_buf)
	payload = data[1]
	hw = (payload >> 7) & 1
	cstate = (payload >> 12) & 0xf
	subcstate = (payload >> 8) & 0xf
	print("hw: %u cstate: %u sub-cstate: %u" % (hw, cstate, subcstate),end=' ')

def print_exstop(raw_buf):
	data = struct.unpack_from("<I", raw_buf)
	flags = data[0]
	exact_ip = flags & 1
	print("IP: %u" % (exact_ip), end=' ')

def print_pwrx(raw_buf):
	data = struct.unpack_from("<IQ", raw_buf)
	payload = data[1]
	deepest_cstate = payload & 0xf
	last_cstate = (payload >> 4) & 0xf
	wake_reason = (payload >> 8) & 0xf
	print("deepest cstate: %u last cstate: %u wake reason: %#x" %
		(deepest_cstate, last_cstate, wake_reason), end=' ')

def print_common_start(comm, sample, name):
	ts = sample["time"]
	cpu = sample["cpu"]
	pid = sample["pid"]
	tid = sample["tid"]
	print(" %03u %9u.%09u " %
		(cpu, ts / 1000000000, ts %1000000000),
		end=' ')

def print_common_ip(sample, symbol, dso):
	ip = sample["ip"]
	print("%16x " % (ip))

def print_ptwrite_ozgur(raw_buf, sample, param_dict):
    data = struct.unpack_from("<IQ", raw_buf)
    flags = data[0]
    payload = data[1]
    exact_ip = flags & 1
    ip = sample["ip"]
    cpu = sample["cpu"]
    ts = sample["time"]
    if "dso" in param_dict:
        dso = param_dict["dso"]
    else:
        dso = "[unknown]"
	#oldprint( "%16x %16x %03u %9u.%09u \n" %  (ip, payload, cpu, ts / 1000000000, ts %1000000000), end=' ')
#the one I was using	print( "%lx %08lx %d %u.%09u\n" %  (ip, payload, cpu, ts / 1000000000, ts %1000000000), end='')
    #print( "%lx %08lx %d %u.%09u %d\n" %  (ip, payload, cpu, ts / 1000000000, ts %1000000000, sample_cnt[0]), end='')
    print( "%lx %08lx %d %u.%09u %d %s\n" %  (ip, payload, cpu, ts / 1000000000, ts %1000000000, sample_cnt[0], dso), end='')
	#print( "%lx %08lx %d %u.%09u\n" %  (ip, payload, cpu, ts / 1000000000, ts %1000000000), end='')
#	if "dso" in param_dict:
#	  dso = param_dict["dso"]
#          print (dso)
#          command = "objdump -h {} | grep .dyninstInst".format(dso)
#          addresses = os.system("ls -l")
#          print (addresses)
#          pe = pefile.PE(dso)
#          for section in pe.sections:
#              if section.Name == '.dyninstInst':
#                print (section)
#                  print ("%s %s" % (section.PointerToRawData),hex(section.Misc_VirtualSize))
#          print( "%lx %08lx %d %u.%09u\n" %  (ip+0, payload, cpu, ts / 1000000000, ts %1000000000), end='')
#	else:
#	  print( "%lx %08lx %d %u.%09u\n" %  (ip+0x400000, payload, cpu, ts / 1000000000, ts %1000000000), end='')


def print_ptwrite_ozgur_test(raw_buf, sample, param_dict):
    data = struct.unpack_from("<IQ", raw_buf)
    flags = data[0]
    payload = data[1]
    exact_ip = flags & 1
    ip = sample["ip"]
    cpu = sample["cpu"]
    ts = sample["time"]
    attr = param_dict["attr"]
    
    if "dso" in param_dict:
        dso = param_dict["dso"]
    else:
        print ("DSO not found")
        return
    if "symbol" in param_dict:
        sym = param_dict["symbol"]
    else:
        print("SYM NOT FOUND")
        return
    print ("SYM:")
    print (sym)
    print ("ATTR:")
    print (attr)
    attrs = struct.unpack_from("<IIQQQQQQQQQQQQQQ",attr)
    print ("ATTRS:")
    print (attrs)
    print ("raw_BUF:")
    print (data)

    symstart = sym["start"]
    symend = sym["end"]
    symname = sym["name"]
	#oldprint( "%16x %16x %03u %9u.%09u \n" %  (ip, payload, cpu, ts / 1000000000, ts %1000000000), end=' ')
	#print( "%lx %08lx %d %u.%09u\n" %  (ip, payload, cpu, ts / 1000000000, ts %1000000000), end='')
    print( "Sym Name: %s DSO: %s START:%lx END:%lx IP:%lx  %08lx %d %u.%09u\n" %  (symname, dso, symstart, symend, ip, payload, cpu, ts / 1000000000, ts %1000000000), end='')


def process_event(param_dict):
    #print ("Printing the dictionary\n")
    #print( param_dict)
    event_attr = param_dict["attr"]
    sample	 = param_dict["sample"]
    raw_buf	= param_dict["raw_buf"]
    comm	   = param_dict["comm"]
    name	   = param_dict["ev_name"]
    callGraph = param_dict["callchain"]
    if ("umask=0x81" in name) or ("umask=0x83" in name):
        sample_cnt[0] +=1
    elif "mem_inst_retired.all_loads" in name:
        sample_cnt[0] +=1
    if "cycles" in name:
        sample_cnt[0] +=1
    for i in callGraph:
        if "sym" in i:
            callSym = i["sym"]
            callName =  callSym["name"]
            print ("CG-LBR :*: %s :*: %d" % (callName ,sample_cnt[0])) 

	# Symbol and dso info are not always resolved
    if "dso" in param_dict:
        dso = param_dict["dso"]
    else:
        dso = "[unknown]"
#        print (dso)
    if "symbol" in param_dict:
        symbol = param_dict["symbol"]
    else:
        symbol = "[unknown]"
#        print (symbol)
    if name == "ptwrite":
#		print_common_ip(sample, symbol, dso)
#		print_ptwrite(raw_buf)
#		print_common_start(comm, sample, name)
        print_ptwrite_ozgur(raw_buf, sample, param_dict)
#		print_ptwrite_ozgur_test(raw_buf, sample, param_dict)
#	elif name == "cbr":
#		print_common_start(comm, sample, name)
#		print_cbr(raw_buf)
#		print_common_ip(sample, symbol, dso)
#	elif name == "mwait":
#		print_common_start(comm, sample, name)
#		print_mwait(raw_buf)
#		print_common_ip(sample, symbol, dso)
#	elif name == "pwre":
#		print_common_start(comm, sample, name)
#		print_pwre(raw_buf)
#		print_common_ip(sample, symbol, dso)
#	elif name == "exstop":
#		print_common_start(comm, sample, name)
#		print_exstop(raw_buf)
#		print_common_ip(sample, symbol, dso)
#	elif name == "pwrx":
#		print_common_start(comm, sample, name)
#		print_pwrx(raw_buf)
#		print_common_ip(sample, symbol, dso)
