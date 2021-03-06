#!/usr/bin/env python3

class Expr:
	def __init__(self, *tup):
		self.tup = tuple(tup)
		self.refcount = 0

	def __invert__(self):
		return Expr("NOT", self.ref())

	def __and__(self, other):
		return Expr("AND", self.ref(), other.ref())

	def __or__(self, other):
		return Expr("OR", self.ref(), other.ref())

	def ref(self):
		self.refcount += 1
		return self

	def tuplify(self):
		ys = [self.tup[0]]
		for x in self.tup[1:]:
			if isinstance(x, Expr):
				ys.append(x.tuplify())
			else:
				ys.append(x)
		return tuple(ys)

	def children(self):
		op = self.tup[0]
		if op in ("NOT","AND","OR"):
			return self.tup[1:]
		else:
			return []

	def trace(self):
		t = self.tup
		op = t[0]
		if op == "IN":
			return [(None, t[1])]
		elif op == "MODOUT":
			return [(t[1], t[2])]
		else:
			xs = []
			for c in self.children(): xs += c.trace()
			return xs

	def should_tmpify(self):
		# tmpify non-leafs referenced more than once
		return self.refcount >= 2 and len(self.children()) > 0

	def tmpify(self, tmp_index):
		clone = Expr(*self.tup).ref()
		self.tup = ("GETTMP", tmp_index)
		return clone

	def __repr__(self):
		return repr(self.tuplify())


def mk_sink_key_output(port): return ("OUT", port)
def mk_sink_key_module_input(instance_index, port): return ("MODIN", instance_index, port)
def mk_sink_key_temporary(index): return ("TMP", index)

class Sink:
	def __init__(self, key, expr = None):
		self.key = key
		self.expr = expr

	def assign(self, expr):
		assert isinstance(expr, Expr)
		assert self.expr is None, "re-assignment"
		self.expr = expr.ref()

	def _keystr(self):
		return ":".join([str(x) for x in self.key])

	def __repr__(self):
		return self._keystr() + " := " + repr(self.expr)


class Proxy:
	def __init__(self, ports):
		self.ports = ports

	def __getitem__(self, key):
		return self.ports[key]

class BaseModule:
	def get_inputs(self):  return self.inputs
	def get_outputs(self): return self.outputs

class UnitDelay(BaseModule):
	# unit delays store exactly one bit
	bits = 1
	inputs  = ["IN"]
	outputs = ["OUT"]
	# OUT "loosely" depends on IN, but delayed by one tick, so for the
	# purpose of trace analysis, OUT has no dependencies (however it must
	# be declared to avoid lookup failure)
	output_dependencies = {"OUT": []}

modules = {
	"!UNIT_DELAY": UnitDelay(),
}

class Module(BaseModule):
	def __init__(self):
		self.inputs = []
		self.outputs = []
		self.port_names = set()
		self.sinks = []
		self.instances = []

	def _regport(self, port):
		if port in self.port_names: raise KeyError("re-definition of %s" % port)
		self.port_names.add(port)

	def _addsink(self, key):
		sink = Sink(key)
		self.sinks.append(sink)
		return sink

	def input(self, port, comment):
		self._regport(port)
		assert port not in self.inputs
		self.inputs.append(port)
		return Expr("IN", port)

	def output(self, port, comment):
		self._regport(port)
		assert port not in self.outputs
		self.outputs.append(port)
		return self._addsink(mk_sink_key_output(port))

	def module(self, module_name):
		module = modules[module_name]
		assert isinstance(module, BaseModule)

		ports = {}
		instance_index = len(self.instances)

		for port in module.get_inputs():
			assert port not in ports
			ports[port] = self._addsink(mk_sink_key_module_input(instance_index, port))
		for port in module.get_outputs():
			assert port not in ports
			ports[port] = Expr("MODOUT", instance_index, port)

		self.instances.append(module_name)
		return Proxy(ports)

	def _mk_sink_map(self):
		return dict([(sink.key, sink) for sink in self.sinks])


	def finalize(self):
		# check that sinks have values
		for sink in self.sinks:
			assert sink.expr is not None

		# enumerate inputs/outputs
		self.input_indices = {}
		for index,p in enumerate(self.inputs): self.input_indices[p] = index
		self.output_indices = {}
		for index,p in enumerate(self.outputs): self.output_indices[p] = index

		# count state bits
		offsets = []
		n = 0
		for module_name in self.instances:
			offsets.append(n)
			module = modules[module_name]
			n += module.bits
		self.bits = n
		self.instance_offsets = offsets

		# trace output dependencies
		sink_map = self._mk_sink_map()
		def trace_port(port):
			input_ports = set()
			def trace_sink(key):
				for (instance_index, port) in sink_map[key].expr.trace():
					if instance_index is None:
						input_ports.add(port)
					else:
						for mport in modules[self.instances[instance_index]].output_dependencies[port]:
							trace_sink(mk_sink_key_module_input(instance_index, mport))
			trace_sink(mk_sink_key_output(port))
			return sorted(list(input_ports), key = lambda p: self.get_input_index(p))
		self.output_dependencies = dict([(port, trace_port(port)) for port in self.outputs])

		tmp_sinks = []
		def split(expr):
			t = expr.tup
			op = t[0]
			if op == "GETTMP": return
			if expr.should_tmpify():
				tmp_index = len(tmp_sinks)
				expr = expr.tmpify(tmp_index)
				tmp_sinks.append(Sink(mk_sink_key_temporary(tmp_index), expr))
			for c in expr.children(): split(c)
		for sink in self.sinks:
			split(sink.expr)
		tmp_sinks.reverse()
		self.sinks = tmp_sinks + self.sinks





	def get_output_dependencies(self, port):
		return self.output_dependencies[port]

	def get_input_index(self, port):
		return self.input_indices[port]

	def get_output_index(self, port):
		return self.output_indices[port]

	def dump(self, name):
		print("===============================================================================")
		print("MODULE: " + name)
		print("Inputs:   %s" % ", ".join(self.inputs))
		print("Outputs:  %s" % ", ".join(self.outputs))
		print("I/O deps: %s" % self.output_dependencies)

		iwos = []
		for i, module_name in enumerate(self.instances):
			bits = modules[module_name].bits
			if bits == 0:
				iwos.append(module_name)
			else:
				iwos.append("%s@%d" % (module_name, self.instance_offsets[i]))
		print("Instances: %s" % ", ".join(iwos))

		print("Bit count: %d" % self.bits)
		print("Sinks:")
		for sink in self.sinks:
			print("  ", sink)
		print("===============================================================================")
		print("")


class ModuleBuilder:
	def __init__(self):
		self.current_module = None

	def begin(self, module_name):
		self.module_name = module_name
		return self

	def input(self, port, comment = None):
		return self.current_module.input(port, comment)

	def output(self, port, comment = None):
		return self.current_module.output(port, comment)

	def module(self, module_name):
		return self.current_module.module(module_name)

	def __enter__(self):
		assert self.module_name not in modules, "re-definition of %s" % self.module_name
		assert self.current_module is None, "re-entrant module definition"
		self.current_module = Module()

	def __exit__(self, exc_type, exc_value, traceback):
		self.current_module.finalize()
		self.current_module.dump(self.module_name)
		modules[self.module_name] = self.current_module
		self.module_name = None
		self.current_module = None

builder = ModuleBuilder()

def module_def(module_name):
	return builder.begin(module_name)

def input(port, comment = None):
	return builder.input(port, comment)

def output(port, comment = None):
	return builder.output(port, comment)

def module(module_name):
	return builder.module(module_name)

def unit_delay():
	return module("!UNIT_DELAY")


##############################################################################
##############################################################################
##############################################################################

class BitArray:
	def __init__(self, n_bits):
		n_bytes = (n_bits+7)>>3
		self.buf8 = bytearray(n_bytes)

	def _coerce(self, value):
		return {True:1, False:0, 1:1, 0:0}[value]

	def __getitem__(self, key):
		return self._coerce((self.buf8[key>>3] & (1<<(key&7))) != 0)

	def __setitem__(self, key, value):
		value = self._coerce(value)
		m = 1<<(key&7)
		if value == 0:
			self.buf8[key>>3] &= ((~m)&255)
		else:
			self.buf8[key>>3] |= m


class VM:
	def __init__(self, module_name):
		m = modules[module_name]
		self.module = m
		self.state = BitArray(m.bits)
		self.input_data = BitArray(len(m.get_inputs()))
		self.output_data = BitArray(len(m.get_outputs()))

	def get_input_ports(self):
		return self.module.get_inputs()

	def get_output_ports(self):
		return self.module.get_outputs()

	def __setitem__(self, key, value):
		self.input_data[self.module.get_input_index(key)] = value

	def __getitem__(self, key):
		return self.output_data[self.module.get_output_index(key)]

	def get_bus_value(self, prefix, size):
		value = 0
		for i in range(size):
			value |= self["%s%d" % (prefix, i)] << i
		return value

	def set_bus_value(self, prefix, size, value):
		assert value >= 0, "value must be unsigned"
		assert value < (1<<size), "value out-of-bounds"
		for i in range(size):
			self["%s%d" % (prefix, i)] = (value & (1<<i)) != 0

	def tick(self):
		pass # XXX TODO



##############################################################################
##############################################################################
##############################################################################

def address_bus(n):
	return [input("A%d" % i) for i in range(n)]

def input_bus(n):
	return [input("I%d" % i) for i in range(n)]

def output_bus(n):
	return [output("O%d" % i) for i in range(n)]

def module_array(n, id):
	ms = []
	for i in range(n):
		ms.append(module(id))
	return ms

def emit_decoder(n):
	A = address_bus(n)
	for i in range(1<<n):
		expr = None
		for j in range(n):
			if i&(1<<j) == 0:
				x = ~A[j]
			else:
				x = A[j]
			if expr is None:
				expr = x
			else:
				expr = expr & x
		output("S%d" % i).assign(expr)


with module_def("Decode1to2"):  emit_decoder(1)
with module_def("Decode2to4"):  emit_decoder(2)
with module_def("Decode3to8"):  emit_decoder(3)
with module_def("Decode4to16"): emit_decoder(4)


with module_def("MemoryBit"):
	WE = input("WE", "write enable")
	IN = input("IN")
	OUT = output("OUT")

	dly = unit_delay()

	dly["IN"].assign((~WE & dly["OUT"]) | (WE & IN))
	OUT.assign(dly["OUT"])


with module_def("MemoryByte"):
	N = 8

	RE = input("RE", "read enable")
	WE = input("WE", "write enable")
	IN = input_bus(N)
	OUT = output_bus(N)

	bits = [module("MemoryBit") for i in range(N)]
	for i in range(N):
		bit = bits[i]
		bit["WE"].assign(WE)
		bit["IN"].assign(IN[i])
		OUT[i].assign(RE & bit["OUT"])


def emit_ram16(address_bus_size, ram_module):
	assert address_bus_size >= 4, "address bus must be at least 4 wide"
	RE = input("RE", "read enable")
	WE = input("WE", "write enable")
	IN = input_bus(8)
	OUT = output_bus(8)
	A = address_bus(address_bus_size)

	demux = module("Decode4to16")
	memarr = module_array(16, ram_module)

	select_addr = A[-4:]
	pass_addr = A[:-4]

	assert len(A) == (len(select_addr) + len(pass_addr))

	# most significant 4 address bits go to demux
	for i, a in enumerate(select_addr):
		demux["A%d" % i].assign(a)

	for i in range(16):
		mem = memarr[i]
		select = demux["S%d" % i]
		mem["WE"].assign(WE & select)
		mem["RE"].assign(RE & select)
		# assign least significant address bits (if any)
		for j, pa in enumerate(pass_addr):
			mem["A%d" % j].assign(pa)
		# connect data inputs
		for j in range(8):
			mem["I%d" % j].assign(IN[j])

	for i in range(8):
		expr = None
		for j in range(16):
			x = memarr[j]["O%d" % i]
			if expr is None:
				expr = x
			else:
				expr = expr | x
		OUT[i].assign(expr)


with module_def("Ram16"):  emit_ram16( 4, "MemoryByte")
with module_def("Ram256"): emit_ram16( 8, "Ram16")
with module_def("Ram4k"):  emit_ram16(12, "Ram256")
with module_def("Ram64k"): emit_ram16(16, "Ram4k")

##############################################################################
##############################################################################
##############################################################################

import random, time

class Timer:
	def __init__(self, what):
		self.what = what

	def __enter__(self):
		self.t0 = time.time()

	def __exit__(self, exc_type, exc_value, traceback):
		dt = time.time() - self.t0
		print("*", self.what, "took", ("%.2fs" % dt))


class RamTest:
	def __init__(self, module_name, address_bus_size):
		print("TEST:    %s (address bus size: %d)" % (module_name, address_bus_size))
		vm = VM(module_name)
		print("Inputs: ", ", ".join(vm.get_input_ports()))
		print("Outputs:", ", ".join(vm.get_output_ports()))
		self.vm = vm
		self.address_bus_size = address_bus_size

	def set_address(self, value):
		self.vm.set_bus_value("A", self.address_bus_size, value)

	def set_input(self, value):
		self.vm.set_bus_value("I", 8, value)

	def get_output(self):
		return self.vm.get_bus_value("O", 8)

	def set_write_enable(self, value):
		self.vm["WE"] = value

	def set_read_enable(self, value):
		self.vm["RE"] = value

	def tick(self):
		self.vm.tick()

	def selftest(self):
		fmtaddr = lambda v: ("$%." + str((self.address_bus_size+3) >> 2) + "X") % v
		fmtimm = lambda v: "#$%.2X" % v

		with Timer("TOTAL"):
			n = 1 << self.address_bus_size

			with Timer("FILL"):
				print("Filling memory (n=%d) ..." % n)
				self.set_write_enable(1)
				self.set_read_enable(0)
				vs = []
				for a in range(n):
					self.set_address(a)
					v = random.randint(0,255)
					vs.append(v)
					self.set_input(v)
					self.tick()

			with Timer("LINEAR"):
				print("Verifying memory; linear access (n=%d) ..." % n)
				self.set_write_enable(0)
				self.set_read_enable(1)
				for a in range(n):
					self.set_address(a)
					self.tick()
					actual = self.get_output()
					expected = vs[a]
					if actual != expected:
						print("FAILED READ at %s; expected %s; got %s" % (fmtaddr(a), fmtimm(expected), fmtimm(actual)))

			with Timer("RANDOM"):
				nr = 100
				print("Verifying memory; random access (n=%d) ..." % nr)
				self.set_write_enable(0)
				self.set_read_enable(1)
				for i in range(nr):
					a = random.randint(0, n-1)
					self.set_address(a)
					self.tick()
					actual = self.get_output()
					expected = vs[a]
					if actual != expected:
						print("FAILED READ at %s; expected %s; got %s" % (fmtaddr(a), fmtimm(expected), fmtimm(actual)))



#t = RamTest("Ram16",  4)
#t = RamTest("Ram256", 8)
#t = RamTest("Ram4k",  12)
#t = RamTest("Ram64k", 16)
#t.selftest()
