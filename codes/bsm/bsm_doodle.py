#!/usr/bin/env python

import sys, os, time, argparse, subprocess, signal, base64

red_dot_png = base64.b64decode('''iVBORw0KGgoAAAANSUhEUgAAAQAAAAEAAQMAAABmvDolAAAAIGNIUk0AAHomAACAhAAA+gAAAIDoAAB1MAAA6mAAADqYAAAXcJy6UTwAAAAGUExURf8AAP///0EdNBEAAAABYktHRAH/Ai3eAAAAH0lEQVRo3u3BAQ0AAADCoPdPbQ43oAAAAAAAAAAAvg0hAAABfxmcpwAAAABJRU5ErkJggg==''') # red.png 256x256 red image

parser = argparse.ArgumentParser(prog = 'bsm_doodle')
parser.add_argument("c_source_file")
parser.add_argument("function_name")
parser.add_argument("-W", "--width")
parser.add_argument("-H", "--height")
parser.add_argument("-P", "--pixels-per-unit")
args = parser.parse_args()

class FEH:
	def __init__(self, path):
		self.path = path
		self.process = None

	def load(self):
		if self.process is None:
			self.process = subprocess.Popen(["feh", self.path])
		else:
			self.process.send_signal(signal.SIGUSR1)

image_path = "_bsm_doodle_%s.png" % args.function_name
viewer=FEH(image_path)

executable = "_bsm_doodle"
prev_ino = -1
set_error = False
while True:
	if set_error:
		with open(image_path,"wb") as f: f.write(red_dot_png)
		viewer.load()
		set_error = False
	try:
		ino = os.stat(args.c_source_file).st_ino
		if ino == prev_ino:
			time.sleep(0.05)
			continue
		prev_ino = ino
	except FileNotFoundError:
		print("%s not found..." % args.c_source_file)
		time.sleep(0.05)
		continue

	t0 = time.time()
	cc_argv = ["cc", "-DBSM_DOODLE=%s" % args.function_name]
	if args.width is not None:           cc_argv += ["-DBSM_DOODLE_WIDTH=%s" % args.width]
	if args.height is not None:          cc_argv += ["-DBSM_DOODLE_HEIGHT=%s" % args.height]
	if args.pixels_per_unit is not None: cc_argv += ["-DBSM_DOODLE_PIXELS_PER_UNIT=%s" % args.pixels_per_unit]
	cc_argv += ["bsm.c", args.c_source_file, "-o", executable, "-lm"]
	e = subprocess.run(cc_argv)
	if e.returncode != 0:
		set_error = True
		continue
	dt = time.time() - t0
	print("%s took %.3f seconds" % (cc_argv, dt))

	e = subprocess.run(["./%s" % executable])
	if e.returncode != 0:
		set_error = True
		continue

	viewer.load()
