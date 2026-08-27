"""The player, in world coordinates.

A direct port of ``wii/source/player.c``, which is itself a port of
``updatePlayer()`` in the browser build's ``js/player.js``. Nothing here knows
about a terminal, a framebuffer or a canvas — it is arithmetic on the 1280x720
world, and every number in it is shared with the other two versions.

Thrust is a *held* input: while it is down the velocity gets ``thrust`` (a
negative number, so it climbs), and while it is up it gets ``gravity``. That
distinction is the whole game, and it is the one thing a terminal makes
awkward — see :class:`drift.arcade.DriftGame` for how a keyboard that reports
presses but never releases is made to stand in for a held button.
"""

from __future__ import annotations

from dataclasses import dataclass

from drift import config


@dataclass
class Player:
    """Position, velocity and the physics that act on them."""

    x: float = float(config.PLAYER_X)
    y: float = float(config.PLAYER_Y_INITIAL)
    velocity: float = 0.0

    thrust: float = config.DEFAULT_THRUST
    gravity: float = config.DEFAULT_GRAVITY
    max_velocity: float = config.DEFAULT_MAX_VELOCITY

    width: int = config.PLAYER_WIDTH
    height: int = config.PLAYER_HEIGHT
    offset_x: int = 0
    offset_y: int = 0

    def set_physics(self, thrust: float, gravity: float,
                    max_velocity: float) -> None:
        self.thrust = thrust
        self.gravity = gravity
        self.max_velocity = max_velocity

    def set_hitbox(self, w: int, h: int, ox: int = 0, oy: int = 0) -> None:
        self.width = w
        self.height = h
        self.offset_x = ox
        self.offset_y = oy

    def update(self, thrust_active: bool,
               canvas_height: int = config.WORLD_HEIGHT) -> bool:
        """Advance one frame. Returns True if the player died on this one.

        Boundary death is measured against the *hitbox*, not the sprite box, so
        a character whose art sits off its origin does not die early. Matches
        ``updatePlayer()`` in ``js/player.js``.
        """
        if thrust_active:
            self.velocity += self.thrust
        else:
            self.velocity += self.gravity

        if self.velocity > self.max_velocity:
            self.velocity = self.max_velocity
        if self.velocity < -self.max_velocity:
            self.velocity = -self.max_velocity

        self.y += self.velocity

        if self.y + self.offset_y < 0:
            return True
        if self.y + self.offset_y + self.height > canvas_height:
            return True
        return False

    @property
    def hitbox(self) -> tuple[int, int, int, int]:
        """``(x, y, w, h)`` in world coordinates."""
        return (int(self.x) + self.offset_x, int(self.y) + self.offset_y,
                self.width, self.height)


def new_player() -> Player:
    """A player at the start of a run, with the default physics."""
    return Player()


__all__ = ["Player", "new_player"]
