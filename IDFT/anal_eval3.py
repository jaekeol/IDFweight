#! /usr/bin/python

import sys

semant = ["capital-common-countries", "city-in-state", "capital-world", "currency", "family"]

pred_file = sys.argv[1]
anw_file = sys.argv[2]

pred = {}
nmap = {}
hit = {}
dop = {}
tot = {}

sehit = {}
sedop = {}
setot = {}

hsum = 0
dsum = 0
tsum = 0
real_sum = 0

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

name ="noname"
sename = "syntactic"
afs = open(anw_file, "r")
for line in afs:
	line = line.strip("\r\n")
	psd = line.split(" ")
	if(psd[0] == ":"):
		name = psd[1]
		continue
	
	if(len(psd) != 4):
		continue
	w1 = psd[0].lower()
	w2 = psd[1].lower()
	w3 = psd[2].lower()
	w4 = psd[3].lower()
	
	key = w1 + " " + w2 + " " + w3

	if(key in pred and pred[key] == "OutOfVocab"):
		real_sum = real_sum + 1	
		continue


	if(name in semant):
		sename = "semantic"
	else:
		sename = "syntactic"

	if key in pred:
		p_ans = pred[key]
		if(p_ans == w4):
			if( name in hit):
				hit[name] = hit[name] + 1
			else:
				hit[name] = 1

			if( sename in sehit):
				sehit[sename] = sehit[sename] + 1
			else:
				sehit[sename] = 1

			hsum = hsum +1
		else:
			if( name in dop):
				dop[name] = dop[name] + 1	
			else:
				dop[name] = 1

			if( sename in sedop):
				sedop[sename] = sedop[sename] + 1	
			else:
				sedop[sename] = 1
			dsum = dsum +1
	else:
		if( name in dop):
			dop[name] = dop[name] + 1
		else:
			dop[name] = 1

		if( sename in sedop):
			sedop[sename] = sedop[sename] + 1	
		else:
			sedop[sename] = 1

		dsum = dsum +1

	if(name in tot):
		tot[name] = tot[name] + 1
	else:
		tot[name] = 1
	tsum = tsum + 1
	real_sum = real_sum + 1

	if(sename in setot):
		setot[sename] = setot[sename] + 1
	else:
		setot[sename] = 1

for name in tot:
	if(tot[name] == 0):
		tot[name] = 1
	if(name in hit):
		thit = hit[name]	
	else:
		hit[name] = 0
	ratio = float(hit[name]) / float(tot[name])
	print("%s\t%d\t%d\t%d\t%f" % (name, hit[name], dop[name], tot[name], ratio))

for name in setot:
	if(setot[name] == 0):
		setot[name] = 1
	ratio = float(sehit[name]) / float(setot[name])
	print("%s\t%d\t%d\t%d\t%f" % (name, sehit[name], sedop[name], setot[name], ratio))


ratio = float(hsum)/ float(tsum)
print("%s\t%d\t%d\t%d\t%f" % ("total", hsum, dsum, tsum, ratio))

ratio = float(tsum) / float(real_sum)
print("%s\t%d\t%d\t%f" % ("seen", tsum, real_sum, ratio))
