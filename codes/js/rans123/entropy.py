#!/usr/bin/env python
import sys
if len(sys.argv) != 2:
	sys.stderr.write("Usage: %s <input>" % sys.argv[0])
	sys.exit(1)
bins = [0]*256
for b in open(sys.argv[1],"rb").read():
	bins[b] += 1
vmax=max(bins)
pos = lambda x: int(round((80*x)/vmax))
vavg=sum(bins)/len(bins)
avgpos = pos(vavg)
for i,v in enumerate(bins):
	d = ["x"] * pos(v)
	if len(d) > avgpos:
		d[avgpos] = '+'
	else:
		while len(d) < avgpos:
			d.append(' ')
		d.append(':')
	d = "".join(d)
	print("%.2x v=%.4x %s" % (i,v,d))
