"""The source game's canvas, and the only coordinate space the simulation knows.

Every constant here is a measurement in the web build's 1280x720 canvas,
straight from ``js/config.js`` — the same numbers ``wii/source/config.h``
carries, for the same reason. :mod:`drift.projection` maps that space onto
whatever the terminal happens to be; **the terminal's own size is deliberately
not named here**, because the moment both spaces have constants in the same
module they get mixed.

Running these constants straight onto a character grid would not be a port of
the game, it would be a different game. `wii/source/playfield.h` spells out
what that costs: the gap and the player stop being the share of the screen they
are on the web, and an obstacle crosses in a different number of frames while
still spawning every 120. Physics, collision and scoring stay in world space
and are byte-identical to the browser; only drawing goes through the
projection.
"""

from __future__ import annotations

WORLD_WIDTH = 1280
WORLD_HEIGHT = 720

GAP = 180
OBSTACLE_WIDTH = 60
OBSTACLE_SPEED = 3
OBSTACLE_SPAWN_INTERVAL = 120

PLAYER_X = 80
PLAYER_Y_INITIAL = 300

DEFAULT_THRUST = -0.6
DEFAULT_GRAVITY = 0.4
DEFAULT_MAX_VELOCITY = 10.0

#: The exhaust plume is drawn below the hitbox and is not solid — collision
#: measures the player's bottom edge above it. See ``obstacles_check_collision``.
FLAME_HEIGHT = 15

MIN_OBSTACLE_HEIGHT = 50
MAX_OBSTACLE_HEIGHT = WORLD_HEIGHT - GAP - MIN_OBSTACLE_HEIGHT

#: Obstacles between palette rotations, and the interval milestones land on.
THEME_CHANGE_INTERVAL = 10

STAR_COUNT = 60

#: Default hitbox, before a character overrides it.
PLAYER_WIDTH = 40
PLAYER_HEIGHT = 35

__all__ = [
    "DEFAULT_GRAVITY", "DEFAULT_MAX_VELOCITY", "DEFAULT_THRUST",
    "FLAME_HEIGHT", "GAP", "MAX_OBSTACLE_HEIGHT", "MIN_OBSTACLE_HEIGHT",
    "OBSTACLE_SPAWN_INTERVAL", "OBSTACLE_SPEED", "OBSTACLE_WIDTH",
    "PLAYER_HEIGHT", "PLAYER_WIDTH", "PLAYER_X", "PLAYER_Y_INITIAL",
    "STAR_COUNT", "THEME_CHANGE_INTERVAL", "WORLD_HEIGHT", "WORLD_WIDTH",
]
