#!/usr/bin/env python
import curses
import sys

I_UP    =0  ; M_UP    =1<<I_UP
I_RIGHT =1  ; M_RIGHT =1<<I_RIGHT
I_DOWN  =2  ; M_DOWN  =1<<I_DOWN
I_LEFT  =3  ; M_LEFT  =1<<I_LEFT

BLACK=0
RED=1
GREEN=2
YELLOW=3
BLUE=4
MAGENTA=5
CYAN=6
WHITE=7

COLOR_PAIRS = {}
COLOR = lambda c0,c1: COLOR_PAIRS[(c0,c1)]
BOLD = curses.A_BOLD
DIM = curses.A_DIM
REVERSE = curses.A_REVERSE

ESC=0x1b

def mkgfx(r0,r1,r2):
	assert len(r0)==3 and len(r1)==3 and len(r2)==3
	return r0+r1+r2

def wire(m):       return "wire.%d" % m
def under(i,m):    return "under.%d.%d" % (i,m)
def transistor(i): return "transistor.%d" % i
def terminal(m,name):   return "terminal.%d.%s" % (m,name)

v2add = lambda a,b: (a[0]+b[0], a[1]+b[1])

def arrowv2(k):
	if k == curses.KEY_UP:
		return (0,-1)
	elif k == curses.KEY_RIGHT:
		return (1,0)
	elif k == curses.KEY_DOWN:
		return (0,1)
	elif k == curses.KEY_LEFT:
		return (-1,0)
	return None

def arrowi(k):
	if k == curses.KEY_UP:
		return I_UP
	elif k == curses.KEY_RIGHT:
		return I_RIGHT
	elif k == curses.KEY_DOWN:
		return I_DOWN
	elif k == curses.KEY_LEFT:
		return I_LEFT
	return None


def in_interval(v, v0, v1):
	vmin = min(v0,v1)
	vmax = max(v0,v1)
	return vmin <= v <= vmax

class TileSet:
	def __init__(self):
		self.name_graphics_map = {}
		self.define_tiles()

	def get_gfx(self, name):
		ps = name.split(".")
		if ps[0] == "terminal":
			gname = ".".join([ps[0], ps[1], "%"])
			return self.name_graphics_map[gname].replace("%", ps[2])
		else:
			return self.name_graphics_map[name]

	def define_tile(self, name, gfx):
		assert len(gfx) == 9
		assert gfx not in self.graphics_collision_test_set, "tileset construction collision"
		self.graphics_collision_test_set.add(gfx)
		if 0:
			print()
			print(name)
			print(gfx[0:3]+"\n"+gfx[3:6]+"\n"+gfx[6:9]+"\n")
		self.name_graphics_map[name] = gfx

	def define_tiles(self):
		self.graphics_collision_test_set = set()

		self.define_tile("void", mkgfx("   ","   ","   "))

		# wire tiles
		for i0 in range(16):
			if i0 == 0: continue
			n_exits = 0
			for j in range(4):
				if (i0&(1<<j)) != 0:
					n_exits += 1
			mid = "+"
			if n_exits == 2:
				if i0 == (M_UP+M_RIGHT) or i0 == (M_DOWN+M_LEFT):
					mid = "\\"
				elif i0 == (M_UP+M_LEFT) or i0 == (M_DOWN+M_RIGHT):
					mid = "/"
				elif i0 == (M_UP+M_DOWN):
					mid = "|"
				elif i0 == (M_LEFT+M_RIGHT):
					mid = "-"
				else:
					assert False, "unexpected nx=%d i0=%d" % (n_exits, i0)
			r0 = " " + ((i0&M_UP)==0 and " " or "|") + " "
			r1 = ((i0&M_LEFT)==0 and " " or "-") + mid + ((i0&M_RIGHT)==0 and " " or "-")
			r2 = " " + ((i0&M_DOWN)==0 and " " or "|") + " "
			self.define_tile(wire(i0), mkgfx(r0,r1,r2))

		# under/sub tiles
		for i0 in range(4):
			for i1 in range(16):
				if (i1 & (1<<i0)) != 0: continue
				dc="^>V<"[i0]
				# which is best?
				#mid = "*"
				mid = dc
				r0 = " " + (i0==I_UP and dc or (((i1&M_UP)!=0) and "|" or " ")) + " "
				r1 = (i0==I_LEFT and dc or (((i1&M_LEFT)!=0) and "-" or " ")) + mid + (i0==I_RIGHT and dc or (((i1&M_RIGHT)!=0) and "-" or " "))
				r2 = " " + (i0==I_DOWN and dc or (((i1&M_DOWN)!=0) and "|" or " ")) + " "
				self.define_tile(under(i0,i1), mkgfx(r0,r1,r2))

		# terminal tiles
		for i0 in range(16):
			mid = "%" # replaced by terminal name (0-9,a-z,A-Z)
			r0 = " " + ((i0&M_UP)==0 and " " or "|") + " "
			r1 = ((i0&M_LEFT)==0 and " " or "-") + mid + ((i0&M_RIGHT)==0 and " " or "-")
			r2 = " " + ((i0&M_DOWN)==0 and " " or "|") + " "
			self.define_tile(terminal(i0, "%"), mkgfx(r0,r1,r2))

		# transistor tiles
		for i0 in range(4):
			C="#" # channel char
			G="&" # gate char
			r0 = " " + ((i0==I_UP) and G or (i0==I_LEFT or i0==I_RIGHT) and C or " ") + " "
			r1 = ((i0==I_LEFT) and G or (i0==I_UP or i0==I_DOWN) and C or " ") + C + ((i0==I_RIGHT) and G or (i0==I_UP or i0==I_DOWN) and C or " ")
			r2 = " " + ((i0==I_DOWN) and G or (i0==I_LEFT or i0==I_RIGHT) and C or " ") + " "
			self.define_tile(transistor(i0), mkgfx(r0,r1,r2))

		self.graphics_collision_test_set = None

	def wire_patch(self, old, i):
		if old is None: old = "wire.0" # hack
		ps = old.split(".")
		m = 1<<i
		if ps[0] == "wire":
			assert len(ps) == 2
			o = int(ps[1])
			if (o & m) != 0: return None # already has requested wire; don't edit
			ps[1] = str(int(ps[1]) | m)
		elif ps[0] == "terminal":
			assert len(ps) == 3
			o = int(ps[1])
			if (o & m) != 0: return None # already has requested wire; don't edit
			ps[1] = str(int(ps[1]) | m)
		elif ps[0] == "under":
			assert len(ps) == 3
			if int(ps[1]) == i: return None # cannot add wire parallel with underground wire
			if (int(ps[2]) & m) != 0: return None # already has requested wire; don't edit
			ps[2] = str(ps[2] | m)
		elif ps[0] == "transistor":
			assert len(ps) == 2
			return None # transistors don't have extra wires
		else:
			assert False, "unexpected"
		return ".".join(ps)

	def omap(self, old, imap):
		if not old: return None
		ps = old.split(".")
		p0 = ps[0]
		pi = list(map(int, ps[1:]))
		ps = None # prevent accidental usage; reconstructed as return value
		def maskmap(mask):
			newmask = 0
			for i in range(4):
				m = 1<<i
				if mask & m: newmask |= 1<<imap(i)
			return newmask
		if p0 == "wire" or p0 == "terminal":
			assert len(pi) == 1
			pi[0] = maskmap(pi[0])
		elif p0 == "under":
			assert len(pi) == 2
			pi[0] = imap(pi[0])
			pi[1] = maskmap(pi[1])
		elif p0 == "transistor":
			assert len(pi) == 1
			pi[0] = imap(pi[0])
		else:
			assert False, "unexpected"
		assert False not in [type(i) is int for i in pi], ("expected integer array, got %s" % repr(pi))
		return ".".join([p0] + list(map(str,pi)))

	def rotate(self, old, n):
		return self.omap(old, lambda x: (x+n)%4)

	def xflip(self, old):
		def m(x):
			if x == I_LEFT: return I_RIGHT
			if x == I_RIGHT: return I_LEFT
			return x
		return self.omap(old, m)

	def yflip(self, old):
		def m(x):
			if x == I_UP: return I_DOWN
			if x == I_DOWN: return I_UP
			return x
		return self.omap(old, m)


MODE0="MODE0"
MODE_VISUAL="MODE_VISUAL"
MODE_WIRE="MODE_WIRE"
MODE_TRANSISTOR="MODE_TRANSISTOR"
MODE_TERMINAL="MODE_TERMINAL"

class Editor:
	def __init__(self):
		self.tileset = TileSet()
		self.canvas = {}
		self.pan = (0,0)
		self.cursor = (0,0)
		self.mode = MODE0
		self.visual0 = self.visual1 = None

	def curses_init(self):
		curses.start_color()
		curses.curs_set(0)
		i = 1
		for i0 in range(8):
			for i1 in range(8):
				curses.init_pair(i, i0, i1)
				global COLOR_PAIRS
				COLOR_PAIRS[(i0,i1)] = curses.color_pair(i)
				i += 1

	def wipe(self, p0, p1 = None):
		if p1 is None: p1 = p0
		x0 = min(p0[0], p1[0])
		x1 = max(p0[0], p1[0])
		y0 = min(p0[1], p1[1])
		y1 = max(p0[1], p1[1])
		for y in range(y0,y1+1):
			for x in range(x0,x1+1):
				p = (x,y)
				if p in self.canvas: del self.canvas[p]
	def at(self, p):
		if p in self.canvas:
			return self.canvas[p]
		else:
			return None

	def center(self):
		self.pan = (self.cursor[0]*3, self.cursor[1]*3)

	def get_ptx(self):
		p0 = p1 = None
		if self.visual0:
			p0 = self.visual0
			p1 = self.visual1
		else:
			p0 = p1 = self.cursor
		p0x, p0y = p0
		p1x, p1y = p1
		return (min(p0x,p1x), min(p0y,p1y)), (max(p0x,p1x), max(p0y,p1y))

	def patch_canvas(self, patch):
		for k,v in patch.items():
			if v is None:
				if k in self.canvas:
					del self.canvas[k]
			else:
				self.canvas[k] = v

	def __rotate(self, n):
		p0,p1 = self.get_ptx()
		patch = {}
		for y in range(p0[1], p1[1]+1):
			for x in range(p0[0], p1[0]+1):
				p = (x,y)
				t = self.at(p)
				pp = p # XXX rotate!
				if t:
					patch[pp] = self.tileset.rotate(t, n)
				else:
					patch[pp] = None
		self.patch_canvas(patch)

	def rotate(self, n):
		while n < 0: n += 4
		n %= 4
		if n == 0: return
		p0,p1 = self.get_ptx()
		h = p1[1]-p0[1]+1
		w = p1[0]-p0[0]+1

		if w == h:
			for i in range(n):
				patch = {}
				for dy in range(h):
					for dx in range(w):
						p = (p0[0]+dx, p0[1]+dy)
						t = self.at(p)
						pp = (p1[0]-dy, p0[1]+dx)
						if t:
							patch[pp] = self.tileset.rotate(t, 1)
						else:
							patch[pp] = None
				self.patch_canvas(patch)
		else:
			pass # TODO non-square rotation?

	def xflip(self):
		p0,p1 = self.get_ptx()
		patch = {}
		for y in range(p0[1], p1[1]+1):
			for x in range(p0[0], p1[0]+1):
				t = self.at((x,y))
				pp = (p0[0]+p1[0]-x, y)
				if t:
					patch[pp] = self.tileset.xflip(t)
				else:
					patch[pp] = None
		self.patch_canvas(patch)

	def yflip(self):
		p0,p1 = self.get_ptx()
		patch = {}
		for y in range(p0[1], p1[1]+1):
			for x in range(p0[0], p1[0]+1):
				t = self.at((x,y))
				pp = (x, p0[1]+p1[1]-y)
				if t:
					patch[pp] = self.tileset.yflip(t)
				else:
					patch[pp] = None
		self.patch_canvas(patch)

	def __call__(self, s):
		self.curses_init()
		k = None

		while True:
			s.erase()
			height, width = s.getmaxyx()
			mx, my = width // 2, height // 2

			for sy in range(height-1):
				for sx in range(width):
					x = self.pan[0] - mx + sx
					y = self.pan[1] - my + sy
					tx = x//3
					ty = y//3
					bg = BLACK
					attr = 0
					if (self.visual0 is not None) and in_interval(tx, self.visual0[0], self.visual1[0]) and in_interval(ty, self.visual0[1], self.visual1[1]):
						bg = MAGENTA
						attr |= BOLD
					elif tx == self.cursor[0] or ty == self.cursor[1]:
						bg = BLUE
						attr |= BOLD

					ch = " "
					p = (tx,ty)
					tile = self.at(p)
					if tile is not None:
						gfx = self.tileset.get_gfx(tile)
						lx = x - tx*3
						ly = y - ty*3
						ch = gfx[lx + ly*3]
					s.insch(sy, sx, ord(ch), COLOR(YELLOW,bg) + attr)

			barx = 0
			bary = height-1
			if self.mode == MODE0:
				s.insstr(bary, barx, "Root Mode / arrows:cursor / z:center / w:wire", COLOR(WHITE,BLACK)+BOLD)
			elif self.mode == MODE_VISUAL:
				s.insstr(bary, barx, "Visual Mode / escape:exit", COLOR(WHITE,BLACK)+BOLD)
			elif self.mode == MODE_WIRE:
				s.insstr(bary, barx, "Wire Mode / escape:exit", COLOR(WHITE,BLACK)+BOLD)
			elif self.mode == MODE_TRANSISTOR:
				s.insstr(bary, barx, "Transistor Mode / choose gate direction with arrow key / cancel with escape or q", COLOR(WHITE,BLACK)+BOLD)
			elif self.mode == MODE_TERMINAL:
				s.insstr(bary, barx, "Terminal Mode / press 0-9,a-z,A-Z for terminal name / cancel with escape", COLOR(WHITE,BLACK)+BOLD)

			s.refresh()
			k = s.getch()
			arrow_v2 = arrowv2(k)
			arrow_i = arrowi(k)

			if self.mode in (MODE0, MODE_VISUAL):
				if k == ord("z"):
					self.center()
				elif k == ord("r"):
					self.rotate(1)
				elif k == ord("R"):
					self.rotate(-1)
				elif k == ord("t"):
					self.yflip()
				elif k == ord("f"):
					self.xflip()

			if self.mode == MODE0:
				if arrow_v2 is not None:
					self.cursor = v2add(self.cursor, arrow_v2)
				elif k == ord("v"):
					self.visual0 = self.visual1 = self.cursor
					self.mode = MODE_VISUAL
				elif k == ord("w"):
					self.mode = MODE_WIRE
				elif k == ord("q"):
					self.mode = MODE_TRANSISTOR
				elif k == ord("c"):
					self.mode = MODE_TERMINAL
				elif k == ESC:
					return
				elif k == ord("x"):
					self.wipe(self.cursor)
			elif self.mode == MODE_VISUAL:
				if arrow_v2 is not None:
					self.cursor = self.visual1 = v2add(self.visual1, arrow_v2)
				elif k == ESC or k == ord("v"):
					self.visual0 = self.visual1 = None
					self.mode = MODE0
				elif k == ord("x"):
					self.wipe(self.visual0, self.visual1)
			elif self.mode == MODE_WIRE:
				if arrow_v2 is not None:
					c0 = self.cursor
					i0 = arrowi(k)
					self.cursor = v2add(self.cursor, arrow_v2)
					c1 = self.cursor
					i1 = (i0+2)%4
					nt0 = self.tileset.wire_patch(self.at(c0), i0)
					nt1 = self.tileset.wire_patch(self.at(c1), i1)
					if nt0 is not None: self.canvas[c0] = nt0
					if nt1 is not None: self.canvas[c1] = nt1
				elif k == ord("w") or k == ESC:
					self.mode = MODE0
				elif k == ord("z"):
					self.center()
			elif self.mode == MODE_TRANSISTOR:
				if k == ord("q") or k == ESC:
					self.mode = MODE0
				elif arrow_i is not None:
					self.canvas[self.cursor] = transistor(arrow_i)
					self.mode = MODE0
			elif self.mode == MODE_TERMINAL:
				if k == ESC:
					self.mode = MODE0
				if (ord("0") <= k <= ord("9")) or (ord("a") <= k <= ord("z")) or (ord("A") <= k <= ord("Z")):
					self.canvas[self.cursor] = terminal(0, chr(k))
					self.mode = MODE0


if len(sys.argv) != 2:
	sys.stderr.write("Usage: %s <.asciiic>\n" % sys.argv[0])
	sys.exit(1)

editor = Editor()
curses.wrapper(editor)
