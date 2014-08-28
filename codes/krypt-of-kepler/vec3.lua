local vec3 = {}
vec3.__index = vec3

local vec3_new = function (obj)
	obj = obj or {0,0,0}
	return setmetatable(obj, vec3)
end

vec3.__add = function(a,b)
	return vec3_new({
		a[1] + b[1],
		a[2] + b[2],
		a[3] + b[3]
	})
end

vec3.add = function(self, b)
	for i=1,3 do self[i] = self[i] + b[i] end
end

vec3.__sub = function(a,b)
	return vec3_new({
		a[1] - b[1],
		a[2] - b[2],
		a[3] - b[3]
	})
end

vec3.__mul = function(v,s)
	return vec3_new({
		v[1] * s,
		v[2] * s,
		v[3] * s
	})
end

vec3.__div = function(v,s)
	return v * (1/s)
end

vec3.cross = function(self, other)
	return vec3_new({
		self[2]*other[3] - self[3]*other[2],
		self[3]*other[1] - self[1]*other[3],
		self[1]*other[2] - self[2]*other[1]
	})
end

vec3.dot = function(self, other)
	return self[1]*other[1] + self[2]*other[2] + self[3]*other[3]
end

vec3.length = function(self)
	return math.sqrt(self:dot(self))
end

vec3.unit = function(self)
	return self / self:length()
end

vec3.dump = function(self, what)
	print(
		what,
		string.format("%.5f", self[1]),
		string.format("%.5f", self[2]),
		string.format("%.5f", self[3]))
end

vec3.clone = function(self)
	return vec3_new({self[1], self[2], self[3]})
end

return setmetatable(vec3, {__call = function (_, ...) return vec3_new(...) end})

