#!/usr/bin/env python
import curses
import traceback
import cPickle as pickle
import os.path
import time
import datetime
import math

class Interval(object):
	def __init__(self, begin, end):
		self.begin = begin
		self.end = end

	def intersection(self, other):
		if other.end < self.begin or other.begin > self.end:
			return None
		else:
			begin = max(self.begin, other.begin)
			end = min(self.end, other.end)
			return Interval(begin, end)

	def duration(self):
		return self.end - self.begin


class State(object):
	def __init__(self):
		self.working = False
		self.state_file = ".krow.p"
		if os.path.exists(self.state_file):
			with open(self.state_file) as f:
				self.state = pickle.load(f)
		else:
			self.state = {"work_intervals":[]}

	def _start(self):
		self.start_time = time.time()

	def _stop(self):
		self.state["work_intervals"].append((self.start_time, time.time()))

	def is_working(self):
		return self.working

	def toggle_working(self):
		if not self.working:
			self._start()
		else:
			self._stop()
		self.working = not self.working

	def _interval_duration(self, begin, end = None):
		if not end:
			end = 1<<48 # far into the future
		i0 = Interval(begin, end)
		total = 0.0
		extra = [] if not self.working else [(self.start_time, time.time())]
		for ibegin, iend in (self.state["work_intervals"] + extra):
			x = i0.intersection(Interval(ibegin, iend))
			if x:
				total += x.duration()
		return total

	def _format_interval(self, begin, end = None):
		duration = int(math.floor(self._interval_duration(begin, end)))
		seconds = duration % 60
		minutes = (duration / 60) % 60
		hours = duration / 3600
		return "%.3dh %.2dm %.2ds" % (hours, minutes, seconds)

	def _boundary_ts(self, d):
		daystart = 5*3600
		return int((datetime.date.fromtimestamp(time.time() - daystart) - datetime.timedelta(days = d)).strftime('%s')) + daystart

	def _daysback(self, d):
		return self._format_interval(self._boundary_ts(d))

	def today(self):
		return self._daysback(0)

	def yesterday(self):
		return self._format_interval(begin = self._boundary_ts(1), end = self._boundary_ts(0))

	def week(self):
		return self._daysback(time.localtime().tm_wday)

	def month(self):
		return self._daysback(time.localtime().tm_mday - 1)

	def year(self):
		return self._daysback(time.localtime().tm_yday - 1)

	def forever(self):
		return self._format_interval(0)

	def exit(self):
		if self.working:
			self._stop()
		with open(self.state_file, "w") as f:
			pickle.dump(self.state, f)

state = State()

# TODO stats: like "days" and "weeks"; skip curses and just query state to
# print stats.


w = curses.initscr()

curses.noecho()
curses.cbreak()
curses.curs_set(0)
curses.start_color()
curses.use_default_colors()

w.timeout(1000)

class Colors(object):
	def __init__(self):
		self.next_color_pair = 1

	def mk(self, fg, bg):
		curses.init_pair(self.next_color_pair, fg, bg)
		cp = curses.color_pair(self.next_color_pair)
		self.next_color_pair += 1
		return cp
colors = Colors()

color_work_border = colors.mk(curses.COLOR_BLACK, curses.COLOR_BLUE)
color_slack_border = colors.mk(curses.COLOR_BLACK, curses.COLOR_MAGENTA)
color_text = colors.mk(curses.COLOR_WHITE, curses.COLOR_BLACK)


exit_exception = None
try:
	while 1:
		w.clear()

		border_color = color_work_border if state.is_working() else color_slack_border

		maxyx = w.getmaxyx()

		border_width = 10
		border_height = 4
		for dy in range(border_height):
			w.addstr(dy, 0, ' ' * maxyx[1], border_color)
			w.addstr(maxyx[0]-2-dy, 0, ' ' * maxyx[1], border_color)
		for y in range(2,maxyx[0]-3):
			for dx in range(border_width):
				w.addstr(y, dx, ' ', border_color)
				w.addstr(y, maxyx[1]-1-dx, ' ', border_color)

		x = border_width + 4
		y = border_height + 2

		w.addstr(y, x, "Status: " + ("working \\o/" if state.is_working() else "not working :-<"), color_text); y += 1
		y += 1
		w.addstr(y, x, "Today:     %s" % state.today()); y += 1
		w.addstr(y, x, "Yesterday: %s" % state.yesterday()); y += 1
		w.addstr(y, x, "Week:      %s" % state.week()); y += 1
		w.addstr(y, x, "Month:     %s" % state.month()); y += 1
		w.addstr(y, x, "Year:      %s" % state.year()); y += 1
		w.addstr(y, x, "Forever:   %s" % state.forever()); y += 1

		c = w.getch()

		if c == -1:
			pass
		elif c == ord(' '):
			state.toggle_working()
		elif c == ord('q') or c == 27:
			break
except KeyboardInterrupt as e:
	pass
except Exception as e:
	exit_exception = traceback.format_exc()

state.exit()

curses.nocbreak()
curses.echo()
curses.endwin()

if exit_exception:
	print exit_exception
