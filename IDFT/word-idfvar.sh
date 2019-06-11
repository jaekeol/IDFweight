#!/usr/bin/env bash
#
# Copyright (c) 2016-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#

RESULTDIR=result_idfvar
#RESULTDIR=result_400K
#RESULTDIR=result_600K_300D_5EP
DATADIR=data
DATA=fil9
#RATIO=0.1
RATIO=1.0
#DATA=wiki.600K

mkdir -p "${RESULTDIR}"
mkdir -p "${DATADIR}"

#if [ ! -f "${DATADIR}/fil9" ]
#then
#  wget -c http://mattmahoney.net/dc/enwik9.zip -P "${DATADIR}"
#  unzip "${DATADIR}/enwik9.zip" -d "${DATADIR}"
#  perl wikifil.pl "${DATADIR}/enwik9" > "${DATADIR}"/fil9
#fi
#
#if [ ! -f "${DATADIR}/rw/rw.txt" ]
#then
#  wget -c https://nlp.stanford.edu/~lmthang/morphoNLM/rw.zip -P "${DATADIR}"
#  unzip "${DATADIR}/rw.zip" -d "${DATADIR}"
#fi
#
#make

./fasttext skipgram -input "${DATADIR}"/"${DATA}" -output "${RESULTDIR}"/"${DATA}" -lr 0.025 -dim 300 \
  -ws 5 -epoch 3 -minCount 5 -neg 5 -loss ns -bucket 2000000 \
  -minn 3 -maxn 6 -thread 4 -t 1e-4 -lrUpdateRate 100 -idf "${RESULTDIR}"/ngrams.idf_sel.$RATIO

#cut -f 1,2 "${DATADIR}"/rw/rw.txt | awk '{print tolower($0)}' | tr '\t' '\n' > "${DATADIR}"/queries.txt

#cat "${DATADIR}"/queries.txt | ./fasttext print-word-vectors "${RESULTDIR}"/"${DATA}".bin "${RESULTDIR}"/ngrams.idf_sel > "${RESULTDIR}"/vectors.txt

#python3 eval.py -m "${RESULTDIR}"/vectors.txt -d "${DATADIR}"/rw/rw.txt
