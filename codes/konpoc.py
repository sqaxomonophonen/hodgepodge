#!/usr/bin/env python3

modules = {}
current_module = None

class Expr:
	def __init__(self, *tup):
		self.tup = tuple(tup)

	def __invert__(self):
		return Expr("NOT", self)

	def __and__(self, other):
		return Expr("AND", self, other)

	def __or__(self, other):
		return Expr("OR", self, other)

	def tuplify(self):
		ys = [self.tup[0]]
		for x in self.tup[1:]:
			if isinstance(x, Expr):
				ys.append(x.tuplify())
			else:
				ys.append(x)
		return tuple(ys)

	def __repr__(self):
		return repr(self.tuplify())

class Sink:
	def __init__(self, instance_index, port_name):
		self.instance_index = instance_index
		self.port_name = port_name
		self.expr = None

	def assign(self, expr):
		assert isinstance(expr, Expr)
		assert self.expr is None, "re-assignment"
		self.expr = expr

	def __repr__(self):
		receiver = ""
		if self.instance_index is not None:
			receiver += "%d:" % self.instance_index
		receiver += self.port_name
		return receiver + " := " + repr(self.expr)


class Instance:
	def __init__(self, ports):
		self.ports = ports

	def __getitem__(self, key):
		return self.ports[key]


class Module:
	def __init__(self, module_name):
		self.module_name = module_name
		self.inputs = []
		self.outputs = []
		self.port_names = set()
		self.sinks = []
		self.instances = []

	def _regport(self, port_name):
		if port_name in self.port_names: raise KeyError("re-definition of %s" % port_name)
		self.port_names.add(port_name)

	def _mksink(self, instance_index, port_name):
		sink = Sink(instance_index, port_name)
		self.sinks.append(sink)
		return sink

	def input(self, port_name, comment):
		self._regport(port_name)
		assert port_name not in self.inputs
		self.inputs.append(port_name)
		return Expr("IN", port_name)

	def output(self, port_name, comment):
		self._regport(port_name)
		assert port_name not in self.outputs
		self.outputs.append(port_name)
		return self._mksink(None, port_name)

	def module(self, module_name):
		ports = {}
		instance_index = len(self.instances)

		if module_name[0] == "@":
			if module_name == "@UNIT_DELAY":
				ports["IN"] = self._mksink(instance_index, "IN")
				ports["OUT"] = Expr("MODOUT", instance_index, "OUT")
			else:
				raise KeyError(module_name)
		else:
			assert module_name in modules, "no such module %s" % module_name
			m = modules[module_name]
			for port_name in m.inputs:
				assert port_name not in ports
				ports[port_name] = self._mksink(instance_index, port_name)
			for port_name in m.outputs:
				assert port_name not in ports
				ports[port_name] = Expr("MODOUT", instance_index, port_name)

		instance = Instance(ports)
		self.instances.append(module_name)
		return instance

	def finalize(self):
		for sink in self.sinks:
			assert sink.expr is not None

		# count state bits
		bits = 0
		for module_name in self.instances:
			if module_name[0] == "@":
				if module_name == "@UNIT_DELAY":
					bits += 1
				else:
					raise KeyError(module_name)
			else:
				assert module_name in modules, "no such module %s" % module_name
				m = modules[module_name]
				bits += m.bits
		self.bits = bits

	def print(self):
		print("===============================================================================")
		print("MODULE: " + self.module_name)
		print("Inputs: %s" % ", ".join(self.inputs))
		print("Outputs: %s" % ", ".join(self.outputs))
		print("Instances: %s" % ", ".join(self.instances))
		print("Bit count: %d" % self.bits)
		print("Sinks:")
		for sink in self.sinks:
			print("  ", sink)
		print("===============================================================================")
		print("")


class ModuleBuilder:
	def __init__(self, module_name):
		self.module_name = module_name

	def __enter__(self):
		assert self.module_name not in modules, "re-definition of %s" % self.module_name
		global current_module
		assert current_module is None, "re-entrant module definition"
		current_module = Module(self.module_name)

	def __exit__(self, exc_type, exc_value, traceback):
		global current_module
		current_module.finalize()
		current_module.print()
		modules[self.module_name] = current_module
		current_module = None


def module_def(module_name):
	return ModuleBuilder(module_name)

def input(port_name, comment = None):
	return current_module.input(port_name, comment)

def output(port_name, comment = None):
	return current_module.output(port_name, comment)

def module(module_name):
	return current_module.module(module_name)

def unit_delay():
	return module("@UNIT_DELAY")


##############################################################################
##############################################################################
##############################################################################


def compile(module_name):
	assert module_name in modules, "no such module %s" % module_name
	# TODO


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

prg = compile("Ram64k")


