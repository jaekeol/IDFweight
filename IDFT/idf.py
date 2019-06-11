#! /usr/bin/python

import sys
import numpy as np
import math

tcnt=1
for line in sys.stdin:
	line = line.strip("\r\n")
	psd = line.split("\t")
	if(len(psd)<2):
		tcnt = float(psd[0])
		continue

	ngram = psd[0]
	dc = float(psd[1])

	idf = math.log( (tcnt - dc + 1) / ( dc + 1) )

	sigidf = 1 / (1+math.exp(-1*idf))

	print("%s\t%.4f\t%d\t%.4f" % (ngram, sigidf, dc, idf))
