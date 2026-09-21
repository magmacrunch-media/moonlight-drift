--!strict
-- Obstacle spawning, movement, collision and scoring, from
-- wii/source/obstacles.c, which mirrors web/js/obstacles.js.
--
-- Each obstacle is a stalactite above the gap and a stalagmite below it, and
-- both taper. profileTop() and profileBottom() give a column's left and right
-- edge at a given height; collision tests the pilot against them, and the
-- client draws the columns from the same two functions, so what you see is
-- exactly what kills you.

local Config = require(script.Parent.Config)
local Theme = require(script.Parent.Theme)

export type Obstacle = {
	x: number,
	topHeight: number,
	bottomY: number,
	passed: boolean,
	style: number, -- 0, 1 or 2
	seed: number,
	roughness: number,
	asymmetry: number,
	-- The spawn count when it is a multiple of THEME_CHANGE_INTERVAL, else 0.
	milestone: number,
	theme: Theme.Theme,
	id: number,
}

export type Field = {
	list: { Obstacle },
	totalCreated: number,
	lastThemeChange: number,
	theme: Theme.Theme,
}

local W = Config.OBSTACLE_WIDTH
local HALF_W = W / 2

local Obstacles = {}

function Obstacles.new(rng: () -> number): Field
	return {
		list = {},
		totalCreated = 0,
		lastThemeChange = 0,
		theme = Theme.generate(rng),
	}
end

function Obstacles.spawn(field: Field, rng: () -> number)
	if #field.list >= Config.MAX_OBSTACLES then
		return
	end

	local range = Config.MAX_OBSTACLE_HEIGHT - Config.MIN_OBSTACLE_HEIGHT
	local topHeight = Config.MIN_OBSTACLE_HEIGHT + math.floor(rng() * range)
	local style = math.floor(rng() * 3)
	local seed = rng()
	local roughness = 0.5 + rng() * 0.5
	local asymmetry = rng() * 2 - 1

	-- Count first, then test: the source game marks the 10th, 20th ...
	-- obstacle and uses the count itself as the label.
	field.totalCreated += 1
	local n = field.totalCreated

	-- One palette for every column on screen, rotating every ten spawns, so
	-- colours move in coherent runs rather than changing column by column.
	if n - field.lastThemeChange >= Config.THEME_CHANGE_INTERVAL then
		field.theme = Theme.generate(rng)
		field.lastThemeChange = n
	end

	table.insert(field.list, {
		x = Config.WORLD_WIDTH,
		topHeight = topHeight,
		bottomY = topHeight + Config.GAP,
		passed = false,
		style = style,
		seed = seed,
		roughness = roughness,
		asymmetry = asymmetry,
		milestone = if n % Config.THEME_CHANGE_INTERVAL == 0 then n else 0,
		theme = field.theme,
		id = n,
	})
end

function Obstacles.advance(field: Field)
	local list = field.list
	for i = #list, 1, -1 do
		local o = list[i]
		o.x -= Config.OBSTACLE_SPEED
		if o.x + W < 0 then
			table.remove(list, i)
		end
	end
end

-- Width and sideways offset of a column `depth` units from its root, where
-- `progress` runs 0 at the root to 1 at the tip. The three styles are the
-- three cases of check_collision_style(), and a style's wobble is a function
-- of depth alone, so the same obstacle always has the same outline.
local function wobble(o: Obstacle, depth: number): number
	if o.style == 0 then
		return math.sin(o.seed * 100 + depth * 0.1) * 6 * o.asymmetry
	elseif o.style == 1 then
		return math.sin(o.seed * 50 + depth * 0.2) * 4
	end
	return math.sin(o.seed * 200 + depth * 0.5) * 8 * o.roughness
end

local MIN_W = { [0] = 22, [1] = 24, [2] = 20 }
local TAPER = { [0] = 0.65, [1] = 0.6, [2] = 0.65 }

-- The stalactite's edges at world height y, for 0 <= y < topHeight. It is
-- widest at the ceiling and narrows toward its tip.
function Obstacles.profileTop(o: Obstacle, y: number): (number, number)
	local depth = math.clamp(y, 0, o.topHeight - 1)
	local progress = depth / o.topHeight
	local w = math.max(MIN_W[o.style], W * (1 - progress * TAPER[o.style]))
	local cx = o.x + HALF_W + wobble(o, depth)
	return cx - w / 2, cx + w / 2
end

-- The stalagmite's edges at world height y, for y > bottomY. It is narrowest
-- at its lip and widens downward, reaching full width by BOTTOM_TAPER_HEIGHT.
function Obstacles.profileBottom(o: Obstacle, y: number): (number, number)
	local taper = Config.BOTTOM_TAPER_HEIGHT
	local depth = math.min(y - o.bottomY, taper)
	local progress = depth / taper
	local w = math.min(W, MIN_W[o.style] + W * progress * TAPER[o.style])
	local cx = o.x + HALF_W + wobble(o, depth)
	return cx - w / 2, cx + w / 2
end

-- Tests one row of each column, as the source game does: the stalactite at
-- the pilot's top edge and the stalagmite at the pilot's feet, which sit
-- FLAME_HEIGHT above the bottom of the hitbox.
local function hits(o: Obstacle, left: number, right: number, top: number, bottom: number): boolean
	if right <= o.x or left >= o.x + W then
		return false
	end
	if top < o.topHeight then
		local l, r = Obstacles.profileTop(o, top)
		if right > l and left < r then
			return true
		end
	end
	if bottom > o.bottomY then
		local l, r = Obstacles.profileBottom(o, bottom)
		if right > l and left < r then
			return true
		end
	end
	return false
end

function Obstacles.collides(field: Field, x: number, y: number, w: number, h: number): boolean
	local bottom = y + h - Config.FLAME_HEIGHT
	for _, o in field.list do
		if hits(o, x, x + w, y, bottom) then
			return true
		end
	end
	return false
end

-- Marks every obstacle the pilot has fully cleared and returns how many that
-- was this frame.
function Obstacles.score(field: Field, pilotX: number): number
	local n = 0
	for _, o in field.list do
		if not o.passed and o.x + W < pilotX then
			o.passed = true
			n += 1
		end
	end
	return n
end

return table.freeze(Obstacles)
