#! /usr/bin/python

import sys

pred_file = sys.argv[1]
anw_file = sys.argv[2]

pred = {}
nmap = {}
hit = [0,0,0,00,0,0,0,0,0,0,0,0,0,0,0,0]
dop = [0,0,0,00,0,0,0,0,0,0,0,0,0,0,0,0]
tot = [0,0,0,00,0,0,0,0,0,0,0,0,0,0,0,0]

pfs = open(pred_file, "r")
for line in pfs:
	line = line.strip("\r\n")
	psd = line.split(" ")
	if(len(psd) < 4):
		continue
	wa = psd[0]
	wb = psd[1]
	wc = psd[2]
	predw = psd[3]

	key = wb + " " + wa + " " + wc
	pred[key] = predw

pfs.close()

name =""
num = 0
afs = open(anw_file, "r")
for line in afs:
	line = line.strip("\r\n")
	psd = line.split(" ")
	if(psd[0] == ":"):
		name = psd[1]
		nmap[num] = name
		num = num + 1
		continue
	
	if(len(psd) != 4):
		continue
	w1 = psd[0].lower()
	w2 = psd[1].lower()
	w3 = psd[2].lower()
	w4 = psd[3].lower()

	key = w1 + " " + w2 + " " + w3

	if key in pred:
		p_ans = pred[key]
		if(p_ans == w4):
			hit[num] = hit[num] + 1
		else:
			dop[num] = dop[num] + 1	
	else:
		dop[num] = dop[num] + 1

	tot[num] = tot[num] + 1

for name in range(1,len(tot)):
	if(tot[name] == 0):
		tot[name] = 1
	ratio = float(hit[name]) / float(tot[name])
	print("%s\t%d\t%d\t%d\t%f" % (nmap.get(name), hit[name], dop[name], tot[name], ratio))

