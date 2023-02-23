from math import sin,cos,radians,asin,sqrt
from time import time
from random import random

earth_radius_km = 6371.0
N = 10000000

rnd = lambda max: random() * max
args = []
for i in range(N):
	args.append(rnd(360.0)-180.0)
	args.append(rnd(180.0)-90.0)
	args.append(rnd(360.0)-180.0)
	args.append(rnd(180.0)-90.0)

def haversine_of_degrees(x0,y0,x1,y1,r):
	dy = radians(y1-y0)
	dx = radians(x1-x0)
	y0r = radians(y0)
	y1r = radians(y1)
	syroot = sin(dy * 0.5)
	sxroot = sin(dx * 0.5)
	root_term = syroot*syroot + cos(y0r)*cos(y1r)*sxroot*sxroot
	return 2.0 * r * asin(sqrt(root_term))


N_TRIALS=10
best_rate = 0
for i0 in range(N_TRIALS):
	sum = 0.0
	offset = 0
	t0 = time()
	for i in range(N):
		sum += haversine_of_degrees(args[offset+0], args[offset+1], args[offset+2], args[offset+3], earth_radius_km)
		offset += 4
	dt = time() - t0
	rate = float(N)/dt
	if rate > best_rate: best_rate = rate
	print("avg is %.2f; %.0f haversines/s" % (sum/float(N), rate))
print("best: %.0f haversines/s" % best_rate)

