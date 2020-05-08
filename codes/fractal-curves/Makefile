
CFLAGS=-std=c99 -Wall

all: fractal \
	mandelbrot_snowflake_sweep_no2.svg \
	mandala_stroke.svg \
	mandala_fill.svg \
	gosper.svg \
	koch.svg \
	monkeys_tree.svg \
	r7.svg \
	snowflake_sweep.svg \
	snowflake_sweep_2.svg \
	dragon_of_eve.svg \
	dragon.svg \
	aks.svg \
	aks2.svg \
	fukuda_gosper.svg

fractal: fractal.c
	$(CC) $(CFLAGS) $< -o $@

wh=-w 1900 -h 900

mandelbrot_snowflake_sweep_no2.svg: mandelbrot_snowflake_sweep_no2.fractal fractal Makefile
	./fractal -o $@ -n 4 -s G0 $<

mandala_stroke.svg: mandala.fractal fractal
	./fractal -o $@ -n 4 -s BW_STROKE $<

mandala_fill.svg: mandala.fractal fractal Makefile
	./fractal -o $@ -n 4 -s G0 $<

gosper.svg: gosper.fractal fractal Makefile
	./fractal -o $@ -n 4 -s G0 $(wh) $<

fukuda_gosper.svg: fukuda_gosper.fractal fractal Makefile
	./fractal -o $@ -n 2 -s G0 $(wh) $<

koch.svg: koch.fractal fractal Makefile
	./fractal -o $@ -n 5 -s G0 $<

monkeys_tree.svg: monkeys_tree.fractal fractal
	./fractal -o $@ -n 4 -s G0 $<

r7.svg: r7.fractal fractal
	./fractal -o $@ -n 5 -s BW_STROKE $<

snowflake_sweep.svg: snowflake_sweep.fractal fractal Makefile
	./fractal -o $@ -n 5 -s G0 $(wh) $<

snowflake_sweep_2.svg: snowflake_sweep_2.fractal fractal Makefile
	./fractal -o $@ -n 2 -s G0 $<

dragon_of_eve.svg: dragon_of_eve.fractal fractal Makefile
	./fractal -o $@ -n 10 -s BW_STROKE $<

dragon.svg: dragon.fractal fractal
	./fractal -o $@ -n 5 -s BW_STROKE $<

aks.svg: aks.fractal fractal Makefile
	./fractal -o $@ -n 8 -s BW_STROKE -w 1900 -h 900 $<

aks2.svg: aks2.fractal fractal
	./fractal -o $@ -n 8 -s BW_STROKE $<

clean:
	rm -f fractal *.svg
