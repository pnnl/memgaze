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

    timeit("matmul0:  ", lambda : matmul0(A.copy(), B.copy()))
    timeit("matmul1:  ", lambda : matmul1(A.copy(), B.copy()))
    timeit("matmul2a: ", lambda : matmul2a(A.copy(), B.copy()))
    timeit("matmul2b: ", lambda : matmul2b(A.copy(), B.copy()))

    A, b = gauss_init(sz)
    timeit("gauss1: ", lambda : gauss1(A.copy(), b.copy(), False))
    timeit("gauss2: ", lambda : gauss2(A.copy(), b.copy(), False))

    
    #-------------------------------------------------------
    # Pandas 1
    #-------------------------------------------------------

    df = pandas1_init()
    
    timeit("pandas1a: ", lambda : pandas1a(df.copy()))
    timeit("pandas1b: ", lambda : pandas1b(df.copy()))
    timeit("pandas1c: ", lambda : pandas1c(df.copy()))


    #-------------------------------------------------------
    # Pandas 2
    #-------------------------------------------------------

    df = pandas2_init()

    timeit("pandas2a: ", lambda : pandas2a(df.copy()))
    timeit("pandas2b: ", lambda : pandas2b(df.copy()))



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

# Ideas for reuse:
#   reuse subsets of matrices
#   iterate over rows vs. columns?
#   reuse in GAP PR and CC

# Cholesky?

# Gauss vectorized vs. not (Gauss will have value and spatial reuse)
#   https://gist.github.com/tkralphs/7554375
#   https://www.codesansar.com/numerical-methods/gauss-elimination-method-python-program.htm

def gauss_init(sz):
    A = np.random.rand(sz, sz)
    b = np.random.rand(sz, 1)
    return A, b


# def gauss0(A: np.array, b: np.array):
#     return np.linalg.solve(A, b)
    
def gauss1(A: np.array, b: np.array, doPricing = True):
    '''
    https://gist.github.com/tkralphs/7554375

    Gaussian elimination with partial pivoting.
    
    input: A is an n x n numpy matrix
           b is an n x 1 numpy array
    output: x is the solution of Ax=b 
            with the entries permuted in 
            accordance with the pivoting 
            done by the algorithm
    post-condition: A and b have been modified.
    '''
    n = len(A)
    if b.size != n:
        raise ValueError("incompatible sizes for A & b.", b.size, n)

    # k represents the current pivot row. Since GE traverses the matrix in the 
    # upper right triangle, we also use k for indicating the k-th diagonal
    # column index.
    
    # Elimination
    for k in range(n-1):
        if doPricing:
            # Pivot
            maxindex = abs(A[k:,k]).argmax() + k
            if A[maxindex, k] == 0:
                raise ValueError("Matrix is singular.")
            # Swap
            if maxindex != k:
                A[[k,maxindex]] = A[[maxindex, k]]
                b[[k,maxindex]] = b[[maxindex, k]]
        else:
            if A[k, k] == 0: # FIXME: test with epsilon
                raise ValueError("Pivot element is zero. Try setting doPricing to True.")

        #Eliminate
        for row in range(k+1, n):
            multiplier = A[row,k]/A[k,k]
            A[row, k:] = A[row, k:] - multiplier*A[k, k:]
            b[row] = b[row] - multiplier*b[k]

    # Back Substitution
    x = np.zeros(n)
    for k in range(n-1, -1, -1):
        x[k] = (b[k] - np.dot(A[k,k+1:],x[k+1:]))/A[k,k]
    return x


# TODO: finish
def gauss2(A: np.array, b: np.array, doPricing = True):
    # Copy of above without vectorization
    # cf. https://www.codesansar.com/numerical-methods/gauss-elimination-method-python-program.htm
    n = len(A)
    if b.size != n:
        raise ValueError("incompatible sizes for A & b.", b.size, n)

    # Elimination
    for k in range(n-1):
        if doPricing:
            # Pivot
            maxindex = abs(A[k:,k]).argmax() + k
            if A[maxindex, k] == 0:
                raise ValueError("Matrix is singular.")
            # Swap
            if maxindex != k:
                A[[k,maxindex]] = A[[maxindex, k]]
                b[[k,maxindex]] = b[[maxindex, k]]
        else:
            if A[k, k] == 0: # FIXME: test with epsilon
                raise ValueError("Pivot element is zero. Try setting doPricing to True.")

        #Eliminate
        for row in range(k+1, n):
            multiplier = A[row,k]/A[k,k]

            #tallent: A[row, k:] = A[row, k:] - multiplier*A[k, k:]
            for col in range(k,n):
                A[row, col] = A[row, col] - multiplier*A[k, col]

            b[row] = b[row] - multiplier*b[k]

    # Back Substitution
    x = np.zeros(n)
    for k in range(n-1, -1, -1):
        #tallent: x[k] = (b[k] - np.dot(A[k,k+1:],x[k+1:]))/A[k,k]
        for col in range(k+1,n):
            x[k] = (b[k] - np.dot(A[k,col],x[col]))/A[k,k]
    return x


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
