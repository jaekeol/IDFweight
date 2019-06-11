#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright (c) 2016-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import numpy as np
from scipy import stats
import os
import math
import argparse


def compat_splitting(line):
    return line.decode('utf8').split()


def similarity(v1, v2):
    n1 = np.linalg.norm(v1)
    n2 = np.linalg.norm(v2)
    return np.dot(v1, v2) / n1 / n2

def findNear(newv,vectors,word1,word2,word3):
    maxsim=0
    sim = 0
    maxword = ""
    for word in vectors:
        #if(word == word3 or word == word2 or word == word3):
        if(word == word3):
            continue
        vt = vectors[word]
        sim = similarity(newv,vt)
        if(sim > maxsim):
            maxsim = sim
            maxword = word
    return maxword

parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument(
    '--model',
    '-m',
    dest='modelPath',
    action='store',
    required=True,
    help='path to model'
)
parser.add_argument(
    '--data',
    '-d',
    dest='dataPath',
    action='store',
    required=True,
    help='path to data'
)
args = parser.parse_args()

vectors = {}
fin = open(args.modelPath, 'rb')
for _, line in enumerate(fin):
    try:
        tab = compat_splitting(line)
        vec = np.array(tab[1:], dtype=float)
        word = tab[0]
        if np.linalg.norm(vec) == 0:
            continue
        if not word in vectors:
            vectors[word] = vec
    except ValueError:
        continue
    except UnicodeDecodeError:
        continue
fin.close()

mysim = []
gold = []
drop = 0.0
hit = 0
nwords = 0.0

tdrop = 0.0
thit = 0
tnwords = 0.0
bline = "__b__";
fin = open(args.dataPath, 'rb')
for line in fin:
    tline = compat_splitting(line)
    if( tline[0] == ":"):
        if(bline != "__b__"):
            print(bline)
            print(tnwords,thit,tdrop)
        bline = line 
        tnwords = 0
        thit = 0
        tdrop = 0
        continue

    if(len(tline) != 4):
        continue
    word1 = tline[0].lower()
    word2 = tline[1].lower()
    word3 = tline[2].lower()
    word4 = tline[3].lower()
    nwords = nwords + 1.0
    tnwords = tnwords + 1.0

    if (word1 in vectors) and (word2 in vectors) and (word3 in vectors):
        v1 = vectors[word1]
        v2 = vectors[word2]
        v3 = vectors[word3]
        v4 = vectors[word4]

        newv = v2 - v1 + v3
        near_word = findNear(newv, vectors, word1,word2,word3)
        if (near_word == word4):
            hit = hit + 1
            thit = thit + 1
        else:
            drop = drop + 1
            tdrop = tdrop + 1
        #print("%s\t%s\t%s\t%s\t%s" % (word1,word2,word3,word4,near_word))
    else:
        drop = drop + 1
        tdrop = tdrop + 1
fin.close()

dataset = os.path.basename(args.dataPath)
print(bline)
print(tnwords,thit,tdrop)
print(nwords,hit,drop)
#print(
    #"{0:20s}: {1:2.0f}  (OOV: {2:2.0f}%)"
    #.format(dataset, corr[0] * 100, math.ceil(drop / nwords * 100.0))
#)
