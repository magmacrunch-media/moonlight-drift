--!strict
-- The player's physics, from wii/source/player.c and web/js/player.js.
-- Named Pilot rather than Player so it cannot be confused with Roblox's
-- Player instance, which is somebody at a keyboard, not a thing that flies.

local Config = require(script.Parent.Config)
local Characters = require(script.Parent.Characters)

export type Pilot = {
	x: number,
	y: number,
	velocity: number,
	thrust: number,
	gravity: number,
	maxVelocity: number,
	hitbox: Characters.Hitbox,
}

-- C's (int) cast truncates toward zero; math.floor would not, for negatives.
local function trunc(v: number): number
	return if v < 0 then math.ceil(v) else math.floor(v)
end

local Pilot = {}

function Pilot.new(character: Characters.Character): Pilot
	return {
		x = Config.PLAYER_X,
		y = Config.PLAYER_Y_INITIAL,
		velocity = 0,
		thrust = character.thrust,
		gravity = character.gravity,
		maxVelocity = character.maxVelocity,
		hitbox = character.hitbox,
	}
end

-- One frame of flight. Returns true when the pilot has left the world, which
-- is measured against the hitbox rather than the drawing, so a character
-- whose art sits off its origin does not die early.
function Pilot.step(p: Pilot, thrustHeld: boolean): boolean
	p.velocity += if thrustHeld then p.thrust else p.gravity
	p.velocity = math.clamp(p.velocity, -p.maxVelocity, p.maxVelocity)
	p.y += p.velocity

	local top = p.y + p.hitbox.oy
	return top < 0 or top + p.hitbox.h > Config.WORLD_HEIGHT
end

-- The hitbox in world units, truncated the way the C port hands it to
-- obstacles_check_collision().
function Pilot.box(p: Pilot): (number, number, number, number)
	return trunc(p.x) + p.hitbox.ox, trunc(p.y) + p.hitbox.oy, p.hitbox.w, p.hitbox.h
end

Pilot.trunc = trunc

return table.freeze(Pilot)
