#!/usr/bin/env python3

import sys

if len(sys.argv) != 2:
	sys.stderr.write("Usage: <input>\n")
	sys.stderr.write("Format is lines of:\n")
	sys.stderr.write("YYYY-MM-DD hh:mm-hh:mm[,hh:mm-hh:mm]...\n")
	sys.exit(1)

def time_unpack(s):
	hrs, mins = map(int, s.split(":"))
	return hrs*60 + mins

def duration_fmt(minutes):
	hrs = minutes // 60
	mins = minutes % 60
	if mins == 0:
		return "%st" % (hrs)
	else:
		return "%st %sm" % (hrs, mins)

with open(sys.argv[1]) as f:
	total_duration = 0
	subtotal_duration = 0
	def print_subtotal():
		global subtotal_duration
		print("       SUB: %s" % duration_fmt(subtotal_duration))
		subtotal_duration = 0
	for line in f.readlines():
		line = line.strip()
		if len(line) == 0: continue
		if "#" in line: line = line[0:line.index("#")]
		if line[0] == "=":
			print_subtotal()
			print("")
			continue
		date, tail = line.split(" ", 1)

		duration = 0
		still_working = False
		for period in tail.split(","):
			period = period.strip()
			from_time, to_time = period.split("-")
			from_time = from_time.strip()
			to_time = to_time.strip()
			if len(to_time) == 0:
				still_working = True
			else:
				duration += time_unpack(to_time) - time_unpack(from_time)
				total_duration += duration
				subtotal_duration += duration
		summary = ""
		if duration > 0: summary += duration_fmt(duration)
		if still_working: summary += " (still working...)"
		print("%s # %s" % (line, summary))

	print_subtotal()
	print("\n     TOTAL: %s" % duration_fmt(total_duration))
