local G = 6.674275965768001e-20

local function ellipse_perimeter(semi_major_axis, eccentricity, n)
	local len = 1
	local fact = 1
	--local semi_minor_axis = semi_major_axis * math.sqrt(1 - eccentricity * eccentricity)
	for i=1,n do
		fact = fact * (i*2*(i*2-1)) / (i*i*4)
		len = len - fact * fact * math.pow(eccentricity,2*i) / (2*i-1)
	end
	return len * math.pi * 2 * semi_major_axis
end



local function calc_elements(orbit)
	local r = orbit.r
	local v = orbit.v
	local mass_major = orbit.M
	local mass_minor = orbit.m

	local Mu = G * (mass_major + mass_minor)
	local a = 1 / (2 / r:length() - v:dot(v) / Mu)

	local H = r:cross(v)

	local p = H:dot(H) / Mu
	local E = math.sqrt(1 - p / a)


	local i = math.acos(H[3] / H:length())

	local lan = 0
	if i ~= 0 then
		lan = math.atan2(H[1], -H[2])
	end

	local q = r:dot(v)

	local TAx = H:dot(H) / (r:length() * Mu) - 1
	local TAy = H:length() * q / (r:length() * Mu)
	local TA = math.atan2(TAy, TAx)

	local Cw = (r[1] * math.cos(lan) + r[2] * math.sin(lan)) / r:length()

	local Sw = 0
	if i == 0 or i == math.pi then -- XXX really?
		Sw = (r[2] * math.cos(lan) - r[1] * math.sin(lan)) / r:length()
	else
		Sw = r[3] / (r:length() * math.sin(i))
	end

	local W = math.atan2(Sw, Cw) - TA
	if W < 0 then W=W+2*math.pi end -- ??!

	local Ex = 1 - r:length() / a
	local Ey = q / math.sqrt(a * Mu)

	local u = math.atan2(Ey, Ex)
	local M = u - E * math.sin(u)

	return {
		semi_major_axis = a,
		periapsis = a - a * E,
		apoapsis = a + a * E,
		eccentricity = E,
		inclination = i,
		longitude_of_ascending_node = lan,
		argument_of_periapsis = W,
		mean_anomaly = M,
		velocity = v:length(),
		orbital_period = math.pi * 2 * math.sqrt((a*a*a) / Mu),
		perimeter = ellipse_perimeter(a, E, 1e4)
	}
end

return {
	G = G,
	calc_elements = calc_elements,
	ellipse_perimeter = ellipse_perimeter
}
