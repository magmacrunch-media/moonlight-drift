--!nocheck
-- Host tests for the rules in src/shared, run with Lune from roblox/:
--
--     lune run tests/run
--
-- The shared modules are ordinary Roblox ModuleScripts that find each other
-- with require(script.Parent.X). Lune has no `script`, so each module is
-- loaded here with a stand-in: script.Parent.X resolves to src/shared/X.luau.
-- That keeps the game code idiomatic Roblox and still lets it run on a host.

local fs = require("@lune/fs")
local luau = require("@lune/luau")
local process = require("@lune/process")

local SHARED = "src/shared/"

local BUILTINS = {
	assert = assert,
	error = error,
	ipairs = ipairs,
	math = math,
	next = next,
	pairs = pairs,
	pcall = pcall,
	print = print,
	rawequal = rawequal,
	rawget = rawget,
	select = select,
	setmetatable = setmetatable,
	string = string,
	table = table,
	tonumber = tonumber,
	tostring = tostring,
	type = type,
	typeof = typeof,
}

local cache = {}

local function load(name: string)
	if cache[name] ~= nil then
		return cache[name]
	end
	local parent = setmetatable({}, {
		__index = function(_, child)
			return { __shared = child }
		end,
	})
	local env = setmetatable({
		script = { Name = name, Parent = parent },
		require = function(ref)
			assert(type(ref) == "table" and ref.__shared, "shared modules may only require script.Parent.<Module>")
			return load(ref.__shared)
		end,
	}, { __index = BUILTINS })
	local chunk = luau.load(fs.readFile(SHARED .. name .. ".luau"), {
		debugName = name,
		environment = env,
		injectGlobals = false,
	})
	local module = chunk()
	cache[name] = module
	return module
end

local Config = load("Config")
local Characters = load("Characters")
local Theme = load("Theme")
local Pilot = load("Pilot")
local Obstacles = load("Obstacles")
local Run = load("Run")

-- Park-Miller: small enough to stay exact in a double, so a seed replays the
-- same stream on every host.
local function seeded(seed: number)
	local s = seed
	return function()
		s = (s * 16807) % 2147483647
		return s / 2147483647
	end
end

local passed, failed = 0, 0
local function test(name: string, fn: () -> ())
	local ok, err = pcall(fn)
	if ok then
		passed += 1
	else
		failed += 1
		print(("FAIL  %s\n      %s"):format(name, tostring(err)))
	end
end

local function eq(actual, expected, what: string?)
	if actual ~= expected then
		error(("%s: expected %s, got %s"):format(what or "value", tostring(expected), tostring(actual)), 2)
	end
end

local function near(actual: number, expected: number, what: string?)
	if math.abs(actual - expected) > 1e-9 then
		error(("%s: expected %s, got %s"):format(what or "value", tostring(expected), tostring(actual)), 2)
	end
end

local function obstacle(fields)
	local o = {
		x = 100,
		topHeight = 200,
		bottomY = 380,
		passed = false,
		style = 1,
		seed = 0,
		roughness = 1,
		asymmetry = 0,
		milestone = 0,
		theme = Theme.generate(seeded(1)),
		id = 1,
	}
	for k, v in fields or {} do
		o[k] = v
	end
	return o
end

local function fieldWith(...)
	return { list = { ... }, totalCreated = 0, lastThemeChange = 0, theme = Theme.generate(seeded(1)) }
end

------------------------------------------------------------------------------
-- Config and roster

test("config matches wii/source/config.h", function()
	eq(Config.WORLD_WIDTH, 1280)
	eq(Config.WORLD_HEIGHT, 720)
	eq(Config.GAP, 180)
	eq(Config.OBSTACLE_WIDTH, 60)
	eq(Config.OBSTACLE_SPEED, 3)
	eq(Config.OBSTACLE_SPAWN_INTERVAL, 120)
	eq(Config.MAX_OBSTACLE_HEIGHT, 490)
	eq(Config.FLAME_HEIGHT, 15)
end)

test("roster has all 24 pilots with unique keys", function()
	eq(Characters.count(), 24)
	local seen = {}
	for i, ch in Characters.list do
		assert(not seen[ch.key], "duplicate key " .. ch.key)
		seen[ch.key] = true
		eq(Characters.indexOf(ch.key), i, ch.key)
		assert(ch.thrust < 0 and ch.gravity > 0 and ch.maxVelocity > 0, ch.key .. " physics")
		assert(ch.accent:match("^#%x%x%x%x%x%x$"), ch.key .. " accent")
		assert(#ch.description > 0, ch.key .. " description")
	end
end)

test("roster spot checks against characters.c", function()
	local cat = Characters.get(1)
	eq(cat.key, "cat-synth")
	eq(cat.hitbox.w, 42)
	eq(cat.hitbox.oy, 18)
	near(cat.gravity, 0.36)
	local balloon = Characters.get(Characters.indexOf("dspum-balloon"))
	near(balloon.thrust, -0.5)
	near(balloon.maxVelocity, 9)
	eq(Characters.get(24).key, "carl")
end)

test("every pilot has both sprites, and an origin inside them", function()
	for _, ch in Characters.list do
		for _, pose in { "idle", "thrust" } do
			local path = ("../wii/sprites/%s-%s.png"):format(ch.key, pose)
			assert(fs.isFile(path), "missing " .. path)
		end
		assert(ch.origin.x >= 0 and ch.origin.x < 128 and ch.origin.y >= 0 and ch.origin.y < 128, ch.key .. " origin")
	end
	eq(Characters.get(1).origin.x, 44, "cat-synth origin, from characters.c")
	eq(Characters.get(Characters.indexOf("didgeridoo-man")).origin.x, 23)
end)

test("sprite offsets tile the sheet without overlapping", function()
	local seen = {}
	local size, cell = Characters.SPRITE_SIZE, Characters.SHEET_CELL
	local extent = Characters.SHEET_COLUMNS * cell
	for i = 1, Characters.count() do
		for _, thrust in { false, true } do
			local x, y = Characters.spriteOffset(i, thrust)
			local id = x .. "," .. y
			assert(not seen[id], "two sprites at " .. id)
			seen[id] = true
			assert(x % cell >= 1 and x % cell + size <= cell - 1, "sprite touches its cell's edge at x " .. x)
			assert(x + size <= extent and y + size <= extent, "sprite outside the sheet at " .. id)
		end
	end
	local x, y = Characters.spriteOffset(1, true)
	eq(x, 131, "cat-synth thrust is the second cell")
	eq(y, 1)
end)

test("Characters.get clamps out-of-range indices", function()
	eq(Characters.get(0).key, "cat-synth")
	eq(Characters.get(99).key, "carl")
end)

------------------------------------------------------------------------------
-- Theme

test("hslToRgb primaries", function()
	local red = Theme.hslToRgb(0, 1, 0.5)
	near(red.r, 1)
	near(red.g, 0)
	near(red.b, 0)
	local grey = Theme.hslToRgb(0.3, 0, 0.4)
	near(grey.r, 0.4)
	near(grey.b, 0.4)
end)

test("generated themes stay in the source game's ranges", function()
	local rng = seeded(9)
	for _ = 1, 200 do
		local t = Theme.generate(rng)
		assert(t.sat >= 0.4 and t.sat < 0.8, "sat")
		assert(t.light >= 0.3 and t.light < 0.6, "light")
		for _, c in { t.primary, t.secondary, t.accent } do
			for _, v in { c.r, c.g, c.b } do
				assert(v >= 0 and v <= 1, "channel out of range")
			end
		end
	end
end)

test("the complementary colour is opposite and bright", function()
	local rng = seeded(11)
	for _ = 1, 100 do
		local t = Theme.generate(rng)
		local c = Theme.complementary(t)
		local light = (math.max(c.r, c.g, c.b) + math.min(c.r, c.g, c.b)) / 2
		assert(light >= 0.5 - 1e-9 and light <= 0.7 + 1e-9, "lightness " .. light)
	end
	-- A red theme's complement is cyan: the hue moves half a turn.
	local red = { hue = 0, sat = 1, light = 0.5, primary = Theme.hslToRgb(0, 1, 0.5), secondary = Theme.hslToRgb(0, 1, 0.5), accent = Theme.hslToRgb(0, 1, 0.5) }
	local c = Theme.complementary(red)
	near(c.r, 0.2, "red channel")
	near(c.g, 1, "green channel")
	near(c.b, 1, "blue channel")
end)

------------------------------------------------------------------------------
-- Pilot

test("gravity and thrust accumulate per frame", function()
	local p = Pilot.new(Characters.get(2)) -- Beava: -0.6 / 0.4 / 10
	eq(Pilot.step(p, false), false)
	near(p.velocity, 0.4)
	near(p.y, 300.4)
	Pilot.step(p, true)
	near(p.velocity, -0.2)
end)

test("velocity clamps to maxVelocity both ways", function()
	local p = Pilot.new(Characters.get(2))
	p.y = 360
	p.velocity = 9.9
	Pilot.step(p, false)
	near(p.velocity, 10)
	p.velocity = -9.9
	Pilot.step(p, true)
	near(p.velocity, -10)
end)

test("bounds are measured against the hitbox, not the origin", function()
	local van = Pilot.new(Characters.get(Characters.indexOf("svfp-van"))) -- oy 12, h 18
	van.velocity = 0
	van.y = -11.5 -- origin above the world, hitbox top at 0.5: still alive
	van.gravity = 0
	eq(Pilot.step(van, false), false)
	van.y = 720 - 30 + 0.5 -- hitbox bottom at 720.5
	eq(Pilot.step(van, false), true)
end)

test("Pilot.box truncates toward zero like a C cast", function()
	local p = Pilot.new(Characters.get(2))
	p.y = 99.9
	local _, y = Pilot.box(p)
	eq(y, 99)
	eq(Pilot.trunc(-2.5), -2)
end)

------------------------------------------------------------------------------
-- Obstacles

test("spawn places a column at the right edge with a 180 gap", function()
	local rng = seeded(3)
	local f = Obstacles.new(rng)
	for _ = 1, 50 do
		Obstacles.spawn(f, rng)
	end
	eq(#f.list, 20, "capped at MAX_OBSTACLES")
	for _, o in f.list do
		eq(o.x, 1280)
		assert(o.topHeight >= 50 and o.topHeight < 490, "topHeight " .. o.topHeight)
		eq(o.bottomY, o.topHeight + 180)
		assert(o.style == 0 or o.style == 1 or o.style == 2, "style")
	end
end)

test("every tenth spawn is a milestone and rotates the palette", function()
	local rng = seeded(5)
	local f = Obstacles.new(rng)
	local first = f.theme
	for _ = 1, 20 do
		Obstacles.spawn(f, rng)
	end
	for i, o in f.list do
		eq(o.milestone, if i % 10 == 0 then i else 0, "milestone " .. i)
	end
	eq(f.list[9].theme, first, "first nine share the opening palette")
	assert(f.list[10].theme ~= first, "the tenth brings a new palette")
	eq(f.list[11].theme, f.list[10].theme, "and the run continues in it")
end)

test("columns move 3 a frame and are culled past the left edge", function()
	local f = fieldWith(obstacle({ x = -57 }), obstacle({ x = 500 }))
	Obstacles.advance(f)
	eq(#f.list, 2, "x + 60 = 0 is still on screen")
	eq(f.list[2].x, 497)
	Obstacles.advance(f)
	eq(#f.list, 1)
end)

test("a pilot in the gap does not collide", function()
	local f = fieldWith(obstacle())
	-- Box spans y 260..295; the stalactite ends at 200, the stalagmite starts at 380.
	eq(Obstacles.collides(f, 110, 260, 40, 35), false)
end)

test("a pilot in the stalactite collides", function()
	local f = fieldWith(obstacle())
	eq(Obstacles.collides(f, 110, 20, 40, 35), true)
end)

test("a pilot in the stalagmite collides", function()
	local f = fieldWith(obstacle())
	eq(Obstacles.collides(f, 110, 400, 40, 35), true)
end)

test("the flame does not collide with the stalagmite", function()
	local f = fieldWith(obstacle())
	-- Bottom of the hitbox at 390, ten units into the stalagmite, but the
	-- last 15 are exhaust, so the feet are at 375, above its lip at 380.
	eq(Obstacles.collides(f, 110, 355, 40, 35), false)
end)

test("a pilot clear of the column horizontally does not collide", function()
	local f = fieldWith(obstacle())
	eq(Obstacles.collides(f, 60, 20, 40, 35), false, "right edge exactly at x")
	eq(Obstacles.collides(f, 160, 20, 40, 35), false, "left edge exactly at x + width")
end)

test("the stalactite's thin tip can be slipped past at the edge", function()
	-- Style 0 with asymmetry 0 has no wobble, so the column is centred on 130
	-- and narrows to its 22-unit minimum at the tip.
	local f = fieldWith(obstacle({ style = 0, asymmetry = 0 }))
	local l, r = Obstacles.profileTop(f.list[1], 199)
	near(r - l, 22, "tip width")
	-- Collision reads the row at the pilot's top edge, 190, which is 22.95
	-- wide: 118.525 .. 141.475.
	eq(Obstacles.collides(f, 78, 190, 40, 35), false, "box ends at 118")
	eq(Obstacles.collides(f, 80, 190, 40, 35), true, "box ends at 120")
end)

test("score counts each cleared column once", function()
	local f = fieldWith(obstacle({ x = 10 }), obstacle({ x = 25 }))
	eq(Obstacles.score(f, 80), 1, "x + 60 = 70 < 80, but 85 is not")
	eq(Obstacles.score(f, 80), 0, "already counted")
	f.list[2].x = 19
	eq(Obstacles.score(f, 80), 1)
end)

------------------------------------------------------------------------------
-- Run

test("an idle pilot falls out of the world", function()
	local run = Run.new(Characters.get(2), seeded(42))
	local crash
	for _ = 1, 1000 do
		crash = Run.step(run, false)
		if crash then
			break
		end
	end
	eq(crash, "bounds")
	eq(run.frame, 51)
end)

test("a pilot holding thrust hits the ceiling", function()
	local run = Run.new(Characters.get(2), seeded(42))
	local crash
	repeat
		crash = Run.step(run, true)
	until crash
	eq(crash, "bounds")
	eq(run.frame, 38)
end)

test("the first column spawns on frame 120", function()
	local run = Run.new(Characters.get(2), seeded(1))
	run.pilot.gravity = 0
	for _ = 1, 119 do
		Run.step(run, false)
	end
	eq(#run.field.list, 0)
	Run.step(run, false)
	eq(#run.field.list, 1)
	eq(run.field.list[1].x, 1277, "spawned and moved on the same frame")
end)

test("a finished run does not advance", function()
	local run = Run.new(Characters.get(2), seeded(42))
	repeat
	until Run.step(run, true)
	local frame = run.frame
	Run.step(run, true)
	eq(run.frame, frame)
end)

-- Flies at the middle of the next gap. Sized against a Python mirror of these
-- rules: it survives a minute on every pilot and seed tried, with room to spare.
local function bot(run)
	local p = run.pilot
	local left = p.x + p.hitbox.ox
	local target = Config.WORLD_HEIGHT / 2
	for _, o in run.field.list do
		if o.x + Config.OBSTACLE_WIDTH >= left then
			target = o.topHeight + Config.GAP / 2
			break
		end
	end
	local centre = p.y + p.hitbox.oy + (p.hitbox.h - Config.FLAME_HEIGHT) / 2
	return centre + p.velocity * 8 > target
end

test("every pilot can fly a full minute, and scores the most possible", function()
	local frames = 60 * Config.FRAME_RATE
	-- The n-th column counts on frame firstFrameForScore(n), so a minute holds
	-- exactly this many.
	local best = 0
	while Run.firstFrameForScore(best + 1) <= frames do
		best += 1
	end
	eq(best, 26)
	for _, seed in { 1, 42 } do
		for _, ch in Characters.list do
			local run = Run.new(ch, seeded(seed))
			for _ = 1, frames do
				local crash = Run.step(run, bot(run))
				if crash then
					error(("%s seed %d crashed (%s) on frame %d"):format(ch.key, seed, crash, run.frame))
				end
			end
			eq(run.score, best, ch.key .. " seed " .. seed)
		end
	end
end)

test("the plausibility cap never refuses an honest run", function()
	for seconds = 0, 600, 0.5 do
		local frames = math.floor(seconds * Config.FRAME_RATE)
		local honest = 0
		while Run.firstFrameForScore(honest + 1) <= frames do
			honest += 1
		end
		assert(Run.maxPlausibleScore(seconds) >= honest, ("refuses %d after %.1fs"):format(honest, seconds))
	end
end)

test("the plausibility cap stays tight", function()
	eq(Run.maxPlausibleScore(0), 0)
	eq(Run.maxPlausibleScore(5), 0, "the first column cannot count before 9 s")
	eq(Run.maxPlausibleScore(60), 28, "a minute honestly holds 26")
	-- Ten minutes honestly holds 296; the 5% slack and one second allow 312.
	eq(Run.maxPlausibleScore(600), 312)
end)

print(("%d passed, %d failed"):format(passed, failed))
process.exit(if failed == 0 then 0 else 1)
