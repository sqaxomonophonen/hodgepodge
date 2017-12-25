
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
	snowflake_sweep_2.svg

fractal: fractal.c
	$(CC) $(CFLAGS) $< -o $@

mandelbrot_snowflake_sweep_no2.svg: mandelbrot_snowflake_sweep_no2.fractal fractal 
	./fractal -o $@ -n 4 -s G0 $<

mandala_stroke.svg: mandala.fractal fractal
	./fractal -o $@ -n 4 -s BW_STROKE $<

mandala_fill.svg: mandala.fractal fractal
	./fractal -o $@ -n 4 -s G0 $<

gosper.svg: gosper.fractal fractal
	./fractal -o $@ -n 4 -s G0 $<

koch.svg: koch.fractal fractal
	./fractal -o $@ -n 6 -s G0 $<

monkeys_tree.svg: monkeys_tree.fractal fractal
	./fractal -o $@ -n 4 -s G0 $<

r7.svg: r7.fractal fractal
	./fractal -o $@ -n 5 -s BW_STROKE $<

snowflake_sweep.svg: snowflake_sweep.fractal fractal
	./fractal -o $@ -n 5 -s G0 $<

snowflake_sweep_2.svg: snowflake_sweep_2.fractal fractal
	./fractal -o $@ -n 5 -s G0 $<

clean:
	rm -f fractal *.svg
