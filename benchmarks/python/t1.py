#!/usr/bin/env python
# -*-Mode: python;-*-

#****************************************************************************
# $HeadURL$
# $Id$
#
# Nathan Tallent
#****************************************************************************

import os
import sys
import time
#import argparse

#import re
#from dataclasses import dataclass
#import collections

import pandas as pd
import numpy as np
#import math



#****************************************************************************
#
#****************************************************************************

def main():
    #-------------------------------------------------------
    # 
    #-------------------------------------------------------

    sz = 1000

    A, B = matmul_init(sz)
    timeit("matmul0:  ", lambda : matmul0(A, B))

    A, B = matmul_init(sz)
    timeit("matmul1:  ", lambda : matmul1(A, B))

    A, B = matmul_init(sz)
    timeit("matmul2a: ", lambda : matmul2a(A, B))

    A, B = matmul_init(sz)
    timeit("matmul2b: ", lambda : matmul2b(A, B))

    #-------------------------------------------------------
    # Pandas 1
    #-------------------------------------------------------

    df = pandas1_init()
    timeit("pandas1a: ", lambda : pandas1a(df))

    df = pandas1_init()
    timeit("pandas1b: ", lambda : pandas1b(df))

    df = pandas1_init()
    timeit("pandas1c: ", lambda : pandas1c(df))


    #-------------------------------------------------------
    # Pandas 2
    #-------------------------------------------------------

    df = pandas2_init()
    timeit("pandas2a: ", lambda : pandas2a(df))

    df = pandas2_init()
    timeit("pandas2b: ", lambda : pandas2b(df))

        
#****************************************************************************

def timeit(msg, fun):
    #-------------------------------------------------------
    # 
    #-------------------------------------------------------

    n_trial = 5
    timeA_s = np.zeros(n_trial) # dtype=np.ulonglong

    for i in range(n_trial):
        t1_ns = time.perf_counter_ns()
        fun()
        t2_ns = time.perf_counter_ns()

        t_s = float(t2_ns - t1_ns) / 1e9
        timeA_s[i] = t_s
        
    t_min = timeA_s.min()
    t_avg = timeA_s.mean()
    
    MSG.msg("{} min: {} s, avg: {} s".format(msg, t_min, t_avg))

    

#****************************************************************************
#
#****************************************************************************

def matmul_init(sz):
    A = np.random.rand(sz, sz)
    B = np.random.rand(sz, sz)
    return A, B


def matmul0(a: np.array, b: np.array):
    return np.matmul(a, b)

# Naive
def matmul1(a: np.array, b: np.array):
    ret = np.empty((a.shape[0], b.shape[1]))
    for rownum in range(a.shape[0]):
        for colnum in range(b.shape[1]):
            row = a[rownum]
            col = b[:, colnum]
            val = np.dot(row, col)
            ret[rownum, colnum] = val
    return ret


# inlined copy + np.dot
def matmul2a(a: np.array, b: np.array):
    ret = np.empty((a.shape[0], b.shape[1]))
    for rownum in range(a.shape[0]):
        for colnum in range(b.shape[1]):
            row = a[rownum]
            col = b.T[colnum].copy()
            val = np.dot(row, col)
            ret[rownum, colnum] = val
    return ret


# hoisted copy + np.dot
def matmul2b(a: np.array, b: np.array):
    T = b.T.copy()
    ret = np.empty((a.shape[0], b.shape[1]))
    for rownum in range(a.shape[0]):
        for colnum in range(b.shape[1]):
            row = a[rownum]
            col = T[colnum]
            val = np.dot(row, col)
            ret[rownum, colnum] = val
    return ret


#****************************************************************************
#
#****************************************************************************

# * https://towardsdatascience.com/do-you-use-apply-in-pandas-there-is-a-600x-faster-way-d2497facfa66
# * https://pythonspeed.com/articles/pandas-vectorization/

def pandas1_init():
    df = pd.DataFrame(np.random.randint(0, 11, size=(1000000, 5)), columns=('a','b','c','d','e'))
    return df


def pandas1a(df):

    def func(a,b,c,d,e):
        if e == 10:
            return c*d
        elif (e < 10) and (e>=5):
            return c+d
        elif e < 5:
            return a+b

    df['new'] = df.apply(lambda x: func(x['a'], x['b'], x['c'], x['d'], x['e']), axis=1)


def pandas1b(df):
    df['new'] = df['c'] * df['d'] #default case e = =10
    mask = df['e'] < 10

    df.loc[mask,'new'] = df['c'] + df['d']
    mask = df['e'] < 5

    df.loc[mask,'new'] = df['a'] + df['b']


def pandas1c(df):
    for col in ('a','b','c','d'):
        df[col] = df[col].astype(np.int16)
    pandas1b(df)

#****************************************************************************

def pandas2_init():
    df = pd.DataFrame(np.random.randint(0, 11, size=(100000000, 5), dtype=np.int16), columns=('a','b','c','d','e'))
    return df

def pandas2a(df):
    df['new'] = df[['a','b','c','d']].sum(axis=1) * df['e']

def pandas2b(df):
    df['new'] = df[['a','b','c','d']].values.sum(axis=1) * df['e'].values


#****************************************************************************
# 
#****************************************************************************

class MSG:
    # https://www.geeksforgeeks.org/print-colors-python-terminal/
    clr_nil  = ''
    clr_bold  = '\033[01m'
    clr_red   = '\033[31m'
    clr_purpl = '\033[35m'
    clr_reset = '\033[0m'

    @staticmethod
    def msg(str):
        MSG.do(MSG.clr_nil, '', str)

    @staticmethod
    def msg_b(str):
        MSG.do(MSG.clr_bold, '', str)
        
    @staticmethod
    def warn(str):
        MSG.do(MSG.clr_bold + MSG.clr_purpl, 'Warning', str)

    @staticmethod
    def err(str, code=1):
        MSG.do(MSG.clr_bold + MSG.clr_red, 'Error', str)
        exit(code)

    @staticmethod
    def do(color, info, str):
        clr_beg = color
        clr_end = MSG.clr_reset

        if (not sys.stdout.isatty()):
            clr_beg = ''
            clr_end = ''
        
        if (info):
            try:
                name = sys._getframe(2).f_code.co_name
            except (IndexError, TypeError, AttributeError): # something went wrong
                name = "<unknown>"

            print("{}{} [{}]: {}{}".format(clr_beg, info, name, str, clr_end))
        else:
            print("{}{}{}".format(clr_beg, str, clr_end))



#****************************************************************************

if (__name__ == "__main__"):
    sys.exit(main())
