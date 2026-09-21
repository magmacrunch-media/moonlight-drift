--!strict
-- One run of the game, stepped a frame at a time. The order of operations is
-- update_playing() in wii/source/main.c: fly, maybe spawn, move the columns,
-- test for a crash, then count what was cleared. Nothing here draws, reads
-- input or touches a Roblox service, so it runs unchanged under Lune, which
-- is where tests/ exercises it.

local Config = require(script.Parent.Config)
local Characters = require(script.Parent.Characters)
local Pilot = require(script.Parent.Pilot)
local Obstacles = require(script.Parent.Obstacles)

export type Crash = "bounds" | "obstacle"

export type Run = {
	frame: number,
	score: number,
	pilot: Pilot.Pilot,
	field: Obstacles.Field,
	rng: () -> number,
	crash: Crash?,
}

local Run = {}

-- rng returns a number in [0, 1). Pass a seeded one to replay a run exactly.
function Run.new(character: Characters.Character, rng: () -> number): Run
	return {
		frame = 0,
		score = 0,
		pilot = Pilot.new(character),
		field = Obstacles.new(rng),
		rng = rng,
		crash = nil,
	}
end

-- Advances one 60 Hz frame. Returns the crash, if this frame ended the run,
-- and how many points it scored. A finished run does not advance.
function Run.step(run: Run, thrustHeld: boolean): (Crash?, number)
	if run.crash then
		return run.crash, 0
	end
	run.frame += 1

	if Pilot.step(run.pilot, thrustHeld) then
		run.crash = "bounds"
		return run.crash, 0
	end

	if run.frame % Config.OBSTACLE_SPAWN_INTERVAL == 0 then
		Obstacles.spawn(run.field, run.rng)
	end
	Obstacles.advance(run.field)

	if Obstacles.collides(run.field, Pilot.box(run.pilot)) then
		run.crash = "obstacle"
		return run.crash, 0
	end

	local gained = Obstacles.score(run.field, Pilot.trunc(run.pilot.x))
	run.score += gained
	return nil, gained
end

-- The first frame on which the n-th obstacle can be counted: it spawns on
-- frame n * SPAWN_INTERVAL at the right edge and has to travel until its
-- trailing edge is behind the pilot.
function Run.firstFrameForScore(n: number): number
	if n <= 0 then
		return 0
	end
	local spawnX = Config.WORLD_WIDTH - Config.OBSTACLE_SPEED
	local travel = spawnX - (Config.PLAYER_X - Config.OBSTACLE_WIDTH) + 1
	return n * Config.OBSTACLE_SPAWN_INTERVAL + math.ceil(travel / Config.OBSTACLE_SPEED)
end

-- The largest score a run lasting `seconds` of wall-clock time could honestly
-- reach. The server holds submissions to this. It is generous on purpose: a
-- little network jitter and a fast client clock must never cost a real score,
-- and anything it lets through is still bounded by how long the run took.
function Run.maxPlausibleScore(seconds: number): number
	local frames = seconds * Config.FRAME_RATE * 1.05 + Config.FRAME_RATE
	local first = Run.firstFrameForScore(1)
	if frames < first then
		return 0
	end
	return 1 + math.floor((frames - first) / Config.OBSTACLE_SPAWN_INTERVAL)
end

return table.freeze(Run)
