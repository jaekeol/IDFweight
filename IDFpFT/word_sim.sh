#!/usr/bin/env bash
#
# Copyright (c) 2016-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#

RESULTDIR=model127
#RESULTDIR=model600
DATADIR=data
DATA=fil9.300D.5EP.v2
#DATA=W600.300D.5EP

mkdir -p "${RESULTDIR}"
mkdir -p "${DATADIR}"

#./fasttext skipgram -input "${DATADIR}"/"${DATA}" -output "${RESULTDIR}"/"${DATA}" -lr 0.025 -dim 300 \
#  -ws 5 -epoch 3 -minCount 5 -neg 5 -loss ns -bucket 2000000 \
#  -minn 3 -maxn 6 -thread 4 -t 1e-4 -lrUpdateRate 100

function compute_sim {
	SIMDATA=$1
	cut -f 1,2 "${DATADIR}"/word_similarity_datasets/"${SIMDATA}" | awk '{print tolower($0)}' | tr '\t' '\n' > "${DATADIR}"/queries.txt

	cat "${DATADIR}"/queries.txt | ./multift print-word-vectors "${RESULTDIR}"/"${DATA}".bin ngrams.idf_sel > "${RESULTDIR}"/vectors.txt

	python3 eval.py -m "${RESULTDIR}"/vectors.txt -d "${DATADIR}"/word_similarity_datasets/"${SIMDATA}"
}

compute_sim "EN-RW-STANFORD.txt"
compute_sim "EN-WS-353-ALL.txt"
compute_sim "SCWS_2003.txt"
compute_sim "simlex999.txt"
compute_sim "EN-MC-30.txt"
compute_sim "EN-RG-65.txt"
compute_sim "EN-MEN-TR-3k.txt"
compute_sim "EN-YP-130.txt"

