#!/usr/bin/env luajit

local vec3 = require('vec3')
local kepler = require('kepler')
local initial = require('orbit')

local elements = kepler.calc_elements(initial)

print("orbital name: " .. initial.name)
print("orbital period: " .. elements.orbital_period)
print("orbital perimeter: " .. elements.perimeter)

local Mu = kepler.G * (initial.M + initial.m)

local acc = function(state)
	local ru = state.r:unit() * -1
	return ru * (Mu / state.r:dot(state.r))
end

local function leapfrog(state, dt)
	state.v:add(acc(state) * 0.5 * dt)
	state.r:add(state.v * dt)
	state.v:add(acc(state) * 0.5 * dt)
end

local methods = {
	-- see: http://www.artcompsci.org/kali/vol/two_body_problem_2/.yo6body.rb.html
	-- http://www.artcompsci.org/kali/vol/two_body_problem_2/.yo8body.rb.html
	-- in general: http://www.artcompsci.org
	eulerforward = function(N, dt)
		local state = {
			r = initial.r:clone(),
			v = initial.v:clone(),
			traveled = 0
		}
		for i=1,N do
			local a = acc(state)
			local da = a * dt
			state.v:add(da)
			local dv = state.v * dt
			state.r:add(dv)
			state.traveled = state.traveled + dv:length()
		end
		return state
	end,
	yoshida4th = function(N, dt)
		local state = {
			r = initial.r:clone(),
			v = initial.v:clone(),
			traveled = 0
		}
		local d = {1.351207191959657, -1.702414383919315}
		for i=1,N do
			local prev_r = state.r:clone()
			leapfrog(state, dt * d[1])
			leapfrog(state, dt * d[2])
			leapfrog(state, dt * d[1])
			state.traveled = state.traveled + (state.r - prev_r):length()
		end
		return state
	end,
	yoshida6th = function(N, dt)
		local state = {
			r = initial.r:clone(),
			v = initial.v:clone(),
			traveled = 0
		}
		local d = {
			0.784513610477560,
			0.235573213359357,
			-1.17767998417887,
			1.31518632068391
		}
		for i=1,N do
			local prev_r = state.r:clone()
			leapfrog(state, dt * d[1])
			leapfrog(state, dt * d[2])
			leapfrog(state, dt * d[3])
			leapfrog(state, dt * d[4])
			leapfrog(state, dt * d[3])
			leapfrog(state, dt * d[2])
			leapfrog(state, dt * d[1])
			state.traveled = state.traveled + (state.r - prev_r):length()
		end
		return state
	end,
	yoshida8th = function(N, dt)
		local state = {
			r = initial.r:clone(),
			v = initial.v:clone(),
			traveled = 0
		}
		local d = {
			0.104242620869991e1,
			0.182020630970714e1,
			0.157739928123617e0,
			0.244002732616735e1,
			-0.716989419708120e-2,
			-0.244699182370524e1,
			-0.161582374150097e1,
			-0.17808286265894516e1
		}
		for i=1,N do
			local prev_r = state.r:clone()
			leapfrog(state, dt * d[1])
			leapfrog(state, dt * d[2])
			leapfrog(state, dt * d[3])
			leapfrog(state, dt * d[4])
			leapfrog(state, dt * d[5])
			leapfrog(state, dt * d[6])
			leapfrog(state, dt * d[7])
			leapfrog(state, dt * d[8])
			leapfrog(state, dt * d[7])
			leapfrog(state, dt * d[6])
			leapfrog(state, dt * d[5])
			leapfrog(state, dt * d[4])
			leapfrog(state, dt * d[3])
			leapfrog(state, dt * d[2])
			leapfrog(state, dt * d[1])
			state.traveled = state.traveled + (state.r - prev_r):length()
		end
		return state
	end,
}

for _,N in ipairs({1e2,1e3,1e4}) do
	local dt = elements.orbital_period / N
	print()
	print("N=" .. N .. " (dt=" .. string.format("%.4f", dt) .. ")")
	for _,method in ipairs({'eulerforward', 'yoshida4th', 'yoshida6th', 'yoshida8th'}) do
		print(" " .. method)
		local t0 = os.clock()
		local endstate = methods[method](N, dt)
		print("   position-diff:  " .. string.format("%.8f", (initial.r - endstate.r):length()) .. " km")
		print("   velocity-diff:  " .. string.format("%.8f", (initial.v - endstate.v):length()) .. " km")
		print("   perimeter-diff: " .. string.format("%.8f", endstate.traveled - elements.perimeter) .. " km") -- XXX beware of "polygonization"
		print("   cpu time:       " .. string.format("%.5f", os.clock() - t0) .. " s")
	end
end


