#!/usr/bin/env python

# a hack for simulating typing latencies, to experience how they feel.

# feelings:
#  10ms ±1ms feels instant.
#  50ms ±5ms is almost unnoticeable.
#  100ms ±10ms is fine.
#  200ms ±20ms is "not great, not terrible".
#  500ms ±200ms starts to be annoying.

import curses,time,sys,random

if len(sys.argv) != 3:
	sys.stderr.write("Usage: %s <gauss mu milliseconds> <gauss sigma milliseconds>\n")
	sys.exit(1)

mu    = float(sys.argv[1])*1e-3
sigma = float(sys.argv[2])*1e-3

def main(stdscr):
	stdscr.timeout(1)
	text = "> "
	queue = []
	spin = 0
	while True:
		now = time.time()
		while len(queue) > 0 and queue[0][1] < now:
			ch = queue[0][0]
			queue = queue[1:]
			if (32 <= ch <= 126) or (ch == 10):
				text += chr(ch)
			elif ch == 263:
				text = text[:-1]
			else:
				raise RuntimeError(ch)
		stdscr.erase()
		stdscr.addstr(0, 0, "/-\\|"[spin%4])
		stdscr.addstr(1, 0, text)
		stdscr.refresh()
		ch = stdscr.getch()
		if ch > 0:
			spin += 1
			queue.append([ch, now + random.gauss(mu, sigma)])

curses.wrapper(main)
