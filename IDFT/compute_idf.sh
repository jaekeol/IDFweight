#! /usr/bin/bash

DATA=wiki.600K
RESULTDIR=result_600K
HEADN=50000

if [ ! -d "$RESULTDIR" ]; then
	mkdir $RESULTDIR
fi

./fasttext print-idf-of-ngrams ./data/"${DATA}" > "${RESULTDIR}"/ngrams.idf

cat "${RESULTDIR}"/ngrams.idf | ./idf.py > "${RESULTDIR}"/ngrams.idf_sig

sort -nk 2 -t "	" "${RESULTDIR}"/ngrams.idf_sig > "${RESULTDIR}"/ngrams.idf_sig.sort

head -n "${HEADN}" "${RESULTDIR}"/ngrams.idf_sig.sort > "${RESULTDIR}"/ngrams.idf_sel
