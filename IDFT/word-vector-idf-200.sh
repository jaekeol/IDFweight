#!/usr/bin/env bash
#
# Copyright (c) 2016-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#

RESULTDIR=result_200
DATADIR=data

mkdir -p "${RESULTDIR}"
mkdir -p "${DATADIR}"

if [ ! -f "${DATADIR}/fil9" ]
then
  wget -c http://mattmahoney.net/dc/enwik9.zip -P "${DATADIR}"
  unzip "${DATADIR}/enwik9.zip" -d "${DATADIR}"
  perl wikifil.pl "${DATADIR}/enwik9" > "${DATADIR}"/fil9
fi

if [ ! -f "${DATADIR}/rw/rw.txt" ]
then
  wget -c https://nlp.stanford.edu/~lmthang/morphoNLM/rw.zip -P "${DATADIR}"
  unzip "${DATADIR}/rw.zip" -d "${DATADIR}"
fi

make

./fasttext skipgram -input "${DATADIR}"/fil9 -output "${RESULTDIR}"/fil9 -lr 0.025 -dim 200 \
  -ws 5 -epoch 1 -minCount 5 -neg 5 -loss ns -bucket 2000000 \
  -minn 3 -maxn 6 -thread 4 -t 1e-4 -lrUpdateRate 100 -idf ngrams.idf_sel

cut -f 1,2 "${DATADIR}"/rw/rw.txt | awk '{print tolower($0)}' | tr '\t' '\n' > "${DATADIR}"/queries.txt

cat "${DATADIR}"/queries.txt | ./fasttext print-word-vectors "${RESULTDIR}"/fil9.bin ngrams.idf_sel > "${RESULTDIR}"/vectors.txt

python3 eval.py -m "${RESULTDIR}"/vectors.txt -d "${DATADIR}"/rw/rw.txt
