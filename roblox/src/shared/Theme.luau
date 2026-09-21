--!strict
-- The obstacle palette, from magnolia's source/theme.c, which mirrors
-- generateRandomTheme() in web/js/obstacles.js. Colours are plain 0..1 RGB
-- triples rather than Color3 so this module runs under Lune as well as in
-- Roblox; the client converts them when it draws.

export type RGB = { r: number, g: number, b: number }

export type Theme = {
	hue: number,
	sat: number,
	light: number,
	primary: RGB,
	secondary: RGB,
	accent: RGB,
}

local function clamp01(v: number): number
	return math.clamp(v, 0, 1)
end

local function hueToRgb(p: number, q: number, t: number): number
	if t < 0 then
		t += 1
	end
	if t > 1 then
		t -= 1
	end
	if t < 1 / 6 then
		return p + (q - p) * 6 * t
	end
	if t < 1 / 2 then
		return q
	end
	if t < 2 / 3 then
		return p + (q - p) * (2 / 3 - t) * 6
	end
	return p
end

local Theme = {}

function Theme.hslToRgb(h: number, s: number, l: number): RGB
	if s == 0 then
		return { r = l, g = l, b = l }
	end
	local q = if l < 0.5 then l * (1 + s) else l + s - l * s
	local p = 2 * l - q
	return {
		r = hueToRgb(p, q, h + 1 / 3),
		g = hueToRgb(p, q, h),
		b = hueToRgb(p, q, h - 1 / 3),
	}
end

-- rng returns a number in [0, 1), like Math.random().
function Theme.generate(rng: () -> number): Theme
	local h = math.floor(rng() * 360) / 360
	local s = (40 + math.floor(rng() * 40)) / 100
	local l = (30 + math.floor(rng() * 30)) / 100
	return {
		hue = h,
		sat = s,
		light = l,
		primary = Theme.hslToRgb(h, s, l),
		secondary = Theme.hslToRgb(h, clamp01(s + 0.1), clamp01(l - 0.1)),
		accent = Theme.hslToRgb(h, clamp01(s - 0.1), clamp01(l + 0.2)),
	}
end

return table.freeze(Theme)
