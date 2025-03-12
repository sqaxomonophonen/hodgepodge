import sys
if len(sys.argv) != 2:
	sys.stderr.write("Usage: %s <file>\n" % sys.argv[0])
	sys.exit(1)
s=""
with open(sys.argv[1],"rb") as f:
	for b in f.read():
		t=s
		a="%d,"%b
		s+=a
		if len(s)>=78:
			print(t)
			s=a
if len(s)>0: print(s)
