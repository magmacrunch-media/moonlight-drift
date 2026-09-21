--!strict
-- The roster, in the web game's load order. Hitboxes and physics are copied
-- from wii/source/characters.c (itself generated from web/js/characters/ and
-- web/js/character-balance.js). Glyphs and accents are the terminal port's,
-- from tui/drift/characters.py and tui/drift/portraits.py: the accent is
-- derived from each character's own sprite, so it is the right colour to
-- stand in for the art until the sprites are uploaded as Roblox assets.

export type Hitbox = { w: number, h: number, ox: number, oy: number }

export type Character = {
	key: string,
	name: string,
	glyph: string,
	accent: string,
	hitbox: Hitbox,
	thrust: number,
	gravity: number,
	maxVelocity: number,
}

local function c(
	key: string,
	name: string,
	glyph: string,
	accent: string,
	w: number,
	h: number,
	ox: number,
	oy: number,
	thrust: number,
	gravity: number,
	maxVelocity: number
): Character
	return table.freeze({
		key = key,
		name = name,
		glyph = glyph,
		accent = accent,
		hitbox = table.freeze({ w = w, h = h, ox = ox, oy = oy }),
		thrust = thrust,
		gravity = gravity,
		maxVelocity = maxVelocity,
	})
end

-- stylua: ignore
local ROSTER: { Character } = {
	c("cat-synth",        "Synth Cat",        "=^=", "#ffa31a", 42, 30, -1, 18, -0.6,  0.36, 10.5),
	c("beava",            "Beava",            "<@>", "#ffd11a", 40, 35,  0,  0, -0.6,  0.4,  10.0),
	c("vinny-bobarino",   "Vinny Bobarino",   "<D>", "#36e28c", 35, 35, 15,  0, -0.6,  0.4,  10.0),
	c("darius-hodgekins", "Darius Hodgekins", "<H>", "#f0c0a8", 40, 35,  0,  0, -0.6,  0.4,  10.0),
	c("roderick-tron",    "Roderick Tron",    "[T]", "#4d4dcb", 40, 35,  0,  0, -0.6,  0.4,  10.0),
	c("juanito-thompson", "Juanito Thompson", "<J>", "#f0c0a8", 40, 35,  0,  0, -0.6,  0.4,  10.0),
	c("carl-spatski",     "Carl Spatski",     "<%>", "#f06030", 42, 42, -1, -2, -0.65, 0.4,  10.5),
	c("dspum-balloon",    "dspum balloon",    "(o)", "#ff1a1a", 30, 38,  5, -3, -0.5,  0.3,   9.0),
	c("foresters-soul",   "Forester's Soul",  "}|{", "#c0d8f0", 40, 40,  0, -2, -0.62, 0.38, 11.0),
	c("grocery-harrison", "Grocery Harrison", "[G]", "#ff1a1a", 35, 30,  8,  5, -0.6,  0.4,  10.0),
	c("didgeridoo-man",   "Didgeridoo Man",   "==D", "#ff751a", 45, 35,  0,  0, -0.7,  0.35,  9.5),
	c("mountain-gnome",   "Mountain Gnome",   "/A\\", "#e68033", 32, 32, 4,  0, -0.55, 0.32,  9.5),
	c("prince-vince",     "Prince Vince",     "<P>", "#ffd11a", 27, 38,  7, -3, -0.65, 0.38, 10.5),
	c("dag-henderson",    "Dag Henderson",    "<b>", "#f0c0a8", 40, 35,  0,  0, -0.6,  0.4,  10.0),
	c("ban-daniel",       "BANDANIEL",        "<B>", "#ffcc1a", 38, 38,  1, -2, -0.6,  0.4,  10.0),
	c("plantain-jane",    "PLANTAIN JANE",    "<J>", "#cbcb4d", 38, 38,  1, -2, -0.6,  0.4,  10.0),
	c("fire-toad",        "Fire Toad",        "<*>", "#b3d940", 32, 32,  4,  0, -0.65, 0.38, 10.5),
	c("backpack-man",     "Backpack Man",     "[M]", "#f0c0a8", 34, 36,  6, -2, -0.6,  0.4,  10.0),
	c("tollbooth-lady",   "Tollbooth Lady",   "<T>", "#e8ff1a", 40, 35,  0,  0, -0.6,  0.4,  10.0),
	c("svfp-van",         "SVFP Van",         "[V]", "#ff1aa3", 36, 18,  4, 12, -0.6,  0.4,  10.0),
	c("tardigrade",       "Tardigrade",       "(~)", "#cd5353", 26, 25,  7,  8, -0.7,  0.35, 11.0),
	c("elektra",          "Elektra",          "<E>", "#ff751a", 38, 38,  1,  0, -0.58, 0.38, 10.5),
	c("strawberto",       "Strawberto",       "<o>", "#e63333", 36, 38,  2,  0, -0.6,  0.39, 10.0),
	c("carl",             "Carl",             "<@>", "#ff1a1a", 38, 38,  1,  0, -0.62, 0.37, 11.0),
}

local byKey: { [string]: number } = {}
for i, ch in ROSTER do
	byKey[ch.key] = i
end

local Characters = {}

Characters.list = table.freeze(ROSTER)

function Characters.count(): number
	return #ROSTER
end

function Characters.get(index: number): Character
	return ROSTER[math.clamp(index, 1, #ROSTER)]
end

-- The roster index for a key, or nil. Saved selections are stored by key so a
-- reordered roster cannot hand somebody a different pilot.
function Characters.indexOf(key: string): number?
	return byKey[key]
end

return table.freeze(Characters)
