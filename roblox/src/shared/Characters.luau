--!strict
-- The roster, in the web game's load order. Hitboxes and physics are copied
-- from wii/source/characters.c (itself generated from web/js/characters/ and
-- web/js/character-balance.js). Glyphs and accents are the terminal port's,
-- from tui/drift/characters.py and tui/drift/portraits.py: the accent is
-- derived from each character's own sprite, so it is the right colour to
-- stand in for the art when the sprite sheet has not been uploaded.
--
-- `origin` is where the character's own (0, 0) sits inside its 128x128
-- sprite, also from characters.c, which measured it by re-rendering each web
-- draw() call. The sprite is drawn with that point on the pilot's position.

export type Hitbox = { w: number, h: number, ox: number, oy: number }
export type Point = { x: number, y: number }

export type Character = {
	key: string,
	name: string,
	glyph: string,
	accent: string,
	description: string,
	hitbox: Hitbox,
	origin: Point,
	thrust: number,
	gravity: number,
	maxVelocity: number,
}

-- What each pilot flies like, from the `description` fields in
-- web/js/character-balance.js, which the website's character modal shows.
-- beava has no entry there and takes getCharacterPhysics()'s fallback, whose
-- physics are the ones its roster row already carries.
-- stylua: ignore
local DESCRIPTIONS: { [string]: string } = {
	["cat-synth"] = "Wide profile but smaller overall - stable and responsive",
	["beava"] = "Standard physics",
	["vinny-bobarino"] = "Well-balanced square hitbox",
	["darius-hodgekins"] = "Default physics - well-balanced for beginners",
	["roderick-tron"] = "Default physics - well-balanced for beginners",
	["juanito-thompson"] = "Default physics - well-balanced for beginners",
	["carl-spatski"] = "Large hitbox compensated by powerful thrust",
	["dspum-balloon"] = "Balloon physics: floats more, harder to control precisely",
	["foresters-soul"] = "Larger hitbox but enhanced speed and control",
	["grocery-harrison"] = "Compact and well-balanced, standard controls",
	["didgeridoo-man"] = "Very wide but powerful thrust and floaty feel",
	["mountain-gnome"] = "Transcendent meditation master - floaty and serene controls, taps into reservoir of cosmic energy",
	["prince-vince"] = "Slender profile with responsive controls",
	["dag-henderson"] = "Default physics - well-balanced for beginners",
	["ban-daniel"] = "Close to standard size and handling",
	["plantain-jane"] = "Standard handling and size",
	["fire-toad"] = "Compact toad with explosive, fiery movement",
	["backpack-man"] = "Slightly smaller than average, standard handling",
	["tollbooth-lady"] = "Default physics - well-balanced for beginners",
	["svfp-van"] = "Ultra-flat profile perfect for hugging bottom gaps - specialty vehicle",
	["tardigrade"] = "Tiny and nimble! Extra agility compensates for precise hitbox",
	["elektra"] = "Cool poetry chick with smooth, artistic controls - beatnik style",
	["strawberto"] = "Sophisticated strawberry gentleman - refined and stable controls",
	["carl"] = "Psychedelic pineapple with energetic, trippy physics - rides the rainbow",
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
	maxVelocity: number,
	originX: number,
	originY: number
): Character
	return table.freeze({
		key = key,
		name = name,
		glyph = glyph,
		accent = accent,
		description = DESCRIPTIONS[key] or "Standard physics",
		hitbox = table.freeze({ w = w, h = h, ox = ox, oy = oy }),
		origin = table.freeze({ x = originX, y = originY }),
		thrust = thrust,
		gravity = gravity,
		maxVelocity = maxVelocity,
	})
end

-- key, name, glyph, accent, hitbox w h ox oy, thrust, gravity, maxVelocity, origin x y
-- stylua: ignore
local ROSTER: { Character } = {
	c("cat-synth",        "Synth Cat",        "=^=", "#ffa31a", 42, 30, -1, 18, -0.6,  0.36, 10.5, 44, 44),
	c("beava",            "Beava",            "<@>", "#ffd11a", 40, 35,  0,  0, -0.6,  0.4,  10.0, 37, 48),
	c("vinny-bobarino",   "Vinny Bobarino",   "<D>", "#36e28c", 35, 35, 15,  0, -0.6,  0.4,  10.0, 45, 44),
	c("darius-hodgekins", "Darius Hodgekins", "<H>", "#f0c0a8", 40, 35,  0,  0, -0.6,  0.4,  10.0, 54, 42),
	c("roderick-tron",    "Roderick Tron",    "[T]", "#4d4dcb", 40, 35,  0,  0, -0.6,  0.4,  10.0, 55, 39),
	c("juanito-thompson", "Juanito Thompson", "<J>", "#f0c0a8", 40, 35,  0,  0, -0.6,  0.4,  10.0, 46, 54),
	c("carl-spatski",     "Carl Spatski",     "<%>", "#f06030", 42, 42, -1, -2, -0.65, 0.4,  10.5, 58, 44),
	c("dspum-balloon",    "dspum balloon",    "(o)", "#ff1a1a", 30, 38,  5, -3, -0.5,  0.3,   9.0, 44, 44),
	c("foresters-soul",   "Forester's Soul",  "}|{", "#c0d8f0", 40, 40,  0, -2, -0.62, 0.38, 11.0, 59, 44),
	c("grocery-harrison", "Grocery Harrison", "[G]", "#ff1a1a", 35, 30,  8,  5, -0.6,  0.4,  10.0, 54, 46),
	c("didgeridoo-man",   "Didgeridoo Man",   "==D", "#ff751a", 45, 35,  0,  0, -0.7,  0.35,  9.5, 23, 38),
	c("mountain-gnome",   "Mountain Gnome",   "/A\\", "#e68033", 32, 32, 4,  0, -0.55, 0.32,  9.5, 44, 44),
	c("prince-vince",     "Prince Vince",     "<P>", "#ffd11a", 27, 38,  7, -3, -0.65, 0.38, 10.5, 60, 43),
	c("dag-henderson",    "Dag Henderson",    "<b>", "#f0c0a8", 40, 35,  0,  0, -0.6,  0.4,  10.0, 48, 44),
	c("ban-daniel",       "BANDANIEL",        "<B>", "#ffcc1a", 38, 38,  1, -2, -0.6,  0.4,  10.0, 40, 44),
	c("plantain-jane",    "PLANTAIN JANE",    "<J>", "#cbcb4d", 38, 38,  1, -2, -0.6,  0.4,  10.0, 42, 44),
	c("fire-toad",        "Fire Toad",        "<*>", "#b3d940", 32, 32,  4,  0, -0.65, 0.38, 10.5, 35, 44),
	c("backpack-man",     "Backpack Man",     "[M]", "#f0c0a8", 34, 36,  6, -2, -0.6,  0.4,  10.0, 44, 44),
	c("tollbooth-lady",   "Tollbooth Lady",   "<T>", "#e8ff1a", 40, 35,  0,  0, -0.6,  0.4,  10.0, 56, 44),
	c("svfp-van",         "SVFP Van",         "[V]", "#ff1aa3", 36, 18,  4, 12, -0.6,  0.4,  10.0, 52, 42),
	c("tardigrade",       "Tardigrade",       "(~)", "#cd5353", 26, 25,  7,  8, -0.7,  0.35, 11.0, 51, 45),
	c("elektra",          "Elektra",          "<E>", "#ff751a", 38, 38,  1,  0, -0.58, 0.38, 10.5, 44, 44),
	c("strawberto",       "Strawberto",       "<o>", "#e63333", 36, 38,  2,  0, -0.6,  0.39, 10.0, 44, 37),
	c("carl",             "Carl",             "<@>", "#ff1a1a", 38, 38,  1,  0, -0.62, 0.37, 11.0, 49, 39),
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

-- The sprite sheet, assets/pilots.png, which tools/make_atlas.py packs from
-- wii/sprites/: every pilot's idle then thrust sprite, in roster order, left
-- to right and top to bottom. Each 128x128 sprite sits in a 130x130 cell with
-- a transparent pixel on every side, so filtering at a sprite's edge samples
-- nothing from its neighbour. The tool reads these numbers from this file.
Characters.SPRITE_SIZE = 128
Characters.SHEET_COLUMNS = 7
Characters.SHEET_CELL = 130
Characters.SHEET_PAD = 1

-- Top-left of a pilot's sprite in the sheet, in pixels.
function Characters.spriteOffset(index: number, thrust: boolean): (number, number)
	local k = (index - 1) * 2 + (if thrust then 1 else 0)
	local cols, cell, pad = Characters.SHEET_COLUMNS, Characters.SHEET_CELL, Characters.SHEET_PAD
	return (k % cols) * cell + pad, (k // cols) * cell + pad
end

return table.freeze(Characters)
