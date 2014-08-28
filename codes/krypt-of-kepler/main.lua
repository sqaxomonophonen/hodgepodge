local vec3 = require('vec3')
local kepler = require('kepler')

love.keyboard.setKeyRepeat(true)

local DRAW_SCALE = 10
local VDRAW_SCALE = 1e-2


local orbit = require('orbit')

local P = orbit.r / DRAW_SCALE
local V = P + orbit.v / (DRAW_SCALE*VDRAW_SCALE)

V.radius = 4
local hndls = {P,V}

local function find_nearest(s, vs)
	local t = 20
	local n = nil
	for _,v in ipairs(vs) do
		--if n == nil or (s-v):length() < (s-n):length() or (t == nil or (s-v):length() <= t) then
		if (s-v):length() <= t and (n == nil or (s-v):length() < (s-n):length()) then
			n = v
		end
	end
	return n
end

local anchor = nil

local lastm = nil


local function get_mvec()
	local w,h = love.graphics.getDimensions()
	local mx, my = love.mouse.getPosition()
	return vec3({mx-w/2,my-h/2,0})
end


local function print_kepler_elements(kes)
	local units = {
		semi_major_axis = ' km',
		periapsis = ' km',
		apoapsis = ' km',
		velocity = ' km/s',
		orbital_period = ' s',
		inclination = '°',
		longitude_of_ascending_node = '°',
		argument_of_periapsis = '°',
		mean_anomaly = '°'
	}
	local y = 10
	for _,k in ipairs({'semi_major_axis', 'periapsis', 'apoapsis', 'eccentricity', 'inclination', 'longitude_of_ascending_node', 'argument_of_periapsis', 'mean_anomaly', 'velocity', 'orbital_period'}) do
		local value = kes[k]
		local unit = units[k] or ""
		if unit == '°' then -- XXX HAX (radians to degrees)
			value = value/math.pi*180
		end
		love.graphics.print(k .. ": " .. string.format("%.3f", value) .. unit, 20, y)
		y = y + 20
	end
end

function get_coords_at_true_anomaly(true_anomaly, eccentricity, argument_of_periapsis, semi_major_axis)
	local phi = argument_of_periapsis + true_anomaly
	local semi_latus_rectum = (semi_major_axis * (1 - eccentricity*eccentricity)) -- semi-latus rectum
	local r = semi_latus_rectum / (1 + eccentricity * math.cos(true_anomaly))
	if math.abs(eccentricity) < 1 or (math.abs(eccentricity) * math.cos(true_anomaly)) > -1 then
		local x = math.cos(phi) * r
		local y = math.sin(phi) * r
		return x,y
	else
		return nil,nil
	end
end

local function plot_kepler_elements(kes, DRAW_SCALE)
	local ps = {}
	local N = 1000
	local w,h = love.graphics.getDimensions()
	for i = 0,N do
		local true_anomaly = (i / N) * math.pi * 2
		local x,y = get_coords_at_true_anomaly(true_anomaly, kes.eccentricity, kes.argument_of_periapsis, kes.semi_major_axis)
		if x or y then
			table.insert(ps, x/DRAW_SCALE+w/2)
			table.insert(ps, y/DRAW_SCALE+h/2)
		end
	end
	love.graphics.line(ps)
end

function love.draw()
	orbit.r = P * DRAW_SCALE
	orbit.v = (V-P) * DRAW_SCALE * VDRAW_SCALE

	local kes = kepler.calc_elements(orbit)

	love.graphics.setColor({255,200,155})
	print_kepler_elements(kes)
	plot_kepler_elements(kes, DRAW_SCALE)

	local w,h = love.graphics.getDimensions()
	local m = get_mvec()
	love.graphics.setColor({255,255,255})
	love.graphics.circle('fill', w/2, h/2, 2, 10)

	if orbit.radius then
		love.graphics.setColor({0,100,255})
		love.graphics.circle('line', w/2, h/2, orbit.radius / DRAW_SCALE, 500)
	end

	love.graphics.setColor({100,200,255,50})
	love.graphics.line(w/2 + P[1], h/2 + P[2],w/2 + V[1], h/2 + V[2])

	local n = find_nearest(m, hndls)
	for _,hndl in ipairs(hndls) do
		if n == hndl then
			love.graphics.setColor({100,255,100,100})
		else
			love.graphics.setColor({100,100,100,100})
		end
		love.graphics.circle('fill', w/2 + hndl[1], h/2 + hndl[2], hndl.radius or 6, 10)
	end

	if anchor and lastm then
		local d = m-lastm
		anchor:add(d)
	end

	lastm = m
end

function love.keypressed(key, isrepeat)
	if key == 'escape' then
		love.event.quit()
	end
	local moves = {
		up = {0,-1,0},
		down = {0,1,0},
		left = {-1,0,0},
		right = {1,0,0}
	}
	local move = moves[key]
	if move then
		V:add(vec3(move))
	end
	moves = {
		w = {0,-1,0},
		s = {0,1,0},
		a = {-1,0,0},
		d = {1,0,0}
	}
	move = moves[key]
	if move then
		P:add(vec3(move))
		V:add(vec3(move))
	end
end

function love.mousepressed(mx, my, button)
	if button == 'l' then
		local m = get_mvec()
		local n = find_nearest(m, hndls)
		if n then
			anchor = n
		end
	end
end

function love.mousereleased(mx, my, button)
	if button == 'l' then
		anchor = nil
	end
end




