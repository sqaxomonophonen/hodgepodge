#!/usr/bin/env python

import curses,time,sys

class Font:
	def __init__(self, s):
		digit = -1
		width = 0
		rows = []
		self.digits = [None]*10
		self.height = None
		for line in s.splitlines():
			w = len(line)
			if w==0: continue
			if line[0]=="~":
				if digit >= 0:
					rows = [row + (" " * (width-len(row))) for row in rows]
					assert self.digits[digit] is None
					self.digits[digit] = rows
					if self.height is None:
						self.height = len(rows)
					else:
						assert self.height == len(rows)

				digit += 1
				width = 0
				rows = []
				continue
			width=max(width,w)
			rows.append(line)

	def render(self, value):
		sdigits = str(int(value))
		out = [""] * self.height
		for sdigit in sdigits:
			glyph = self.digits[int(sdigit)]
			assert len(glyph) == self.height
			do_strip = 1000
			for y in range(self.height):
				out_n_trailing_spaces = len(out[y]) - len(out[y].rstrip())
				glyph_n_leading_spaces = len(glyph[y]) - len(glyph[y].lstrip())
				n_strip = out_n_trailing_spaces + glyph_n_leading_spaces
				if len(out[y])> 0 and out[y][-out_n_trailing_spaces-1] == glyph[y][glyph_n_leading_spaces]:
					n_strip += 1
				do_strip = min(do_strip, n_strip)

			for y in range(self.height):
				r = do_strip
				g = glyph[y]
				while r > 0:
					if g[0] == " ":
						g = g[1:]
					elif out[y][-1] == " ":
						out[y] = out[y][0:-1]
					elif g[0] == out[y][-1]:
						g = g[1:]
					else:
						assert False, "strip failed"
					r -= 1
				out[y] += g
		return out


# stolen from /usr/local/share/figlet/big.flf
font = Font("""
~
  ___
 / _ \\
| | | |
| | | |
| |_| |
 \\___/
~
 __
/_ |
 | |
 | |
 | |
 |_|
~
 ___
|__ \\
   ) |
  / /
 / /_
|____|
~
 ____
|___ \\
  __) |
 |__ <
 ___) |
|____/
~
 _  _
| || |
| || |_
|__   _|
   | |
   |_|
~
 _____
| ____|
| |__
|___ \\
 ___) |
|____/
~
   __
  / /
 / /_
| '_ \\
| (_) |
 \\___/
~
 ______
|____  |
    / /
   / /
  / /
 /_/
~
  ___
 / _ \\
| (_) |
 > _ <
| (_) |
 \\___/
~
  ___
 / _ \\
| (_) |
 \\__, |
   / /
  /_/
~
"""
)

def main(stdscr):
	times = []
	curses.curs_set(False)
	curses.start_color()

	curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLACK)
	curses.init_pair(2, curses.COLOR_BLACK, curses.COLOR_WHITE)
	curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_YELLOW)
	curses.init_pair(4, curses.COLOR_BLACK, curses.COLOR_GREEN)
	curses.init_pair(5, curses.COLOR_BLACK, curses.COLOR_RED)
	curses.init_pair(6, curses.COLOR_BLACK, curses.COLOR_BLUE)

	seq = [2,3,4,5,6]
	tps = 0.035

	stdscr.timeout(30)
	while True:
		stdscr.clear()
		h,w = stdscr.getmaxyx()
		if len(times) < 2:
			msg = ["START TAPPING"]
			msg.insert(0," "*len(msg[0]))
		else:
			a = 0
			n = 0
			for i in range(len(times)-1):
				a += times[i+1]-times[i]
				n += 1
			bpm = round(60/(a/n))
			msg = font.render(bpm)
			msg[-1] += " BPM"
		y,x = h//2 - len(msg)//2, w//2 - len(msg[0])//2
		ly0 = y-1
		ly1 = y+len(msg)+1

		if len(times) > 0:
			i = int((time.time() - times[-1]) / tps)
			if i < len(seq):
				c = seq[i]
				for dx in range(w):
					stdscr.addstr(ly0, dx, " ", curses.color_pair(c))
					stdscr.addstr(ly1, dx, " ", curses.color_pair(c))
		for dy in range(len(msg)):
			stdscr.addstr(y+dy, x, "%s" % msg[dy], curses.color_pair(1))
		stdscr.refresh()
		ch = stdscr.getch()
		if ch > 0:
			times.append(time.time())
			times = times[-8:]


curses.wrapper(main)
