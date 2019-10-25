#!/usr/bin/env bash
#
# Copyright (c) 2016-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#

#RESULTDIR=result_600K_300D_5EP
#RESULTDIR=result_120K_300D_5EP
#RESULTDIR=result_170K_300D_5EP_L
RESULTDIR=result_600K_300D_5EP_L
#RESULTDIR=result_idfvar
DATADIR=data
DATA=wiki.600K
#DATA=fil9

mkdir -p "${RESULTDIR}"
mkdir -p "${DATADIR}"

function compute_sim {
#	echo $1
	cut -f 1,2 "${DATADIR}"/wordsim/"$1" | awk '{print tolower($0)}' | tr '\t' '\n' > "${DATADIR}"/wordsim/q_"$1"
	cat "${DATADIR}"/wordsim/q_"$1" | ./fasttext print-word-vectors "${RESULTDIR}"/"${DATA}".bin "${RESULTDIR}"/ngrams.idf_sel.learned > "${RESULTDIR}"/vector_"$1"
	python3 eval.py -m "${RESULTDIR}"/vector_"$1" -d "${DATADIR}"/wordsim/"$1"
}

compute_sim "EN-RW-STANFORD.txt"
compute_sim "EN-WS-353-ALL.txt"
compute_sim "SCWS_2003.txt"
compute_sim "simlex999.txt"
compute_sim "EN-MC-30.txt"
compute_sim "EN-RG-65.txt"
compute_sim "EN-MEN-TR-3k.txt"
compute_sim "EN-YP-130.txt"

#EN-MC-30.txt      EN-MTurk-287.txt  EN-RG-65.txt        EN-WS-353-ALL.txt  EN-WS-353-SIM.txt  SCWS_2003.txt   scws.txt
#EN-MEN-TR-3k.txt  EN-MTurk-771.txt  EN-RW-STANFORD.txt  EN-WS-353-REL.txt  EN-YP-130.txt      SimLex-999.txt  simlex999.txt

