#!/usr/bin/env bash
set -xe
width=50
height=25
prev=""
while true ; do
	now=$(date +%s.%N)
	rm -f fps1
	c++ -DNOW=${now} -DWIDTH=${width} -DHEIGHT=${height} -O0 -g fps.c -o fps1 -lm
	./fps1
	echo "fps: $(echo "1.0/($now - $prev)" | bc -l)"
	prev=$now
done
