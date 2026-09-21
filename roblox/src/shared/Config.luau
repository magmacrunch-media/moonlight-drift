--!strict
-- The simulation's constants, from wii/source/config.h, which took them from
-- web/js/config.js and web/js/player.js. Every length is in the source game's
-- 1280x720 world and every rate is per 60 Hz frame. The client projects that
-- world onto whatever screen it has; nothing here knows a pixel.

local WORLD_HEIGHT = 720
local GAP = 180
local MIN_OBSTACLE_HEIGHT = 50

return table.freeze({
	WORLD_WIDTH = 1280,
	WORLD_HEIGHT = WORLD_HEIGHT,

	GAP = GAP,
	OBSTACLE_WIDTH = 60,
	OBSTACLE_SPEED = 3,
	OBSTACLE_SPAWN_INTERVAL = 120,
	MIN_OBSTACLE_HEIGHT = MIN_OBSTACLE_HEIGHT,
	MAX_OBSTACLE_HEIGHT = WORLD_HEIGHT - GAP - MIN_OBSTACLE_HEIGHT,
	MAX_OBSTACLES = 20,
	-- The bottom column tapers over this many units below its lip, whatever
	-- the column's real height. A fixed number in check_collision_style().
	BOTTOM_TAPER_HEIGHT = 500,

	PLAYER_X = 80,
	PLAYER_Y_INITIAL = 300,
	-- The bottom of every hitbox is exhaust, and obstacles ignore it.
	FLAME_HEIGHT = 15,

	THEME_CHANGE_INTERVAL = 10,
	STAR_COUNT = 60,

	FRAME_RATE = 60,
	HIGH_SCORE_COUNT = 10,
})
