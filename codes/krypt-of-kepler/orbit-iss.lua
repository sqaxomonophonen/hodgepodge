local vec3 = require('vec3')

local earth_radius = 6371.1
local iss_altitude = earth_radius + 423
local iss_velocity = 7.66
local r = vec3({iss_altitude,0,0})
local v = vec3({0,iss_velocity,0})

return {
	name = "ISS/earth orbit",
	radius = earth_radius,
	r = r,
	v = v,
	M = 5.972e24, -- kg
	m = 1e3
}
