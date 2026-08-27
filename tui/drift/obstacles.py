"""The obstacle field, in world coordinates.

Ported from ``wii/source/obstacles.c``, which cites ``js/obstacles.js``. As
with :mod:`drift.player`, nothing here knows what it is being drawn on.

**The collision shape is not a rectangle**, and that is the part a port gets
wrong. Each column tapers along its length and wanders sideways on a sine of
its own seed, and the collision test evaluates that silhouette at the player's
own y rather than testing the bounding box. A rectangular test would be a
harder game near the mouth of a column and an easier one at its tip, in ways a
player would feel long before anyone could point at a line of code.

The three styles differ only in how fast they taper, how wide they bottom out
and how much they wander — but they are the same three the browser draws, so
the shape you see is the shape you hit.
"""

from __future__ import annotations

import math
import random
from dataclasses import dataclass, field

from drift import config

MAX_OBSTACLES = 20

#: The bottom column tapers over this many world units before it stops
#: widening. A magic number in ``js/obstacles.js``; kept because the silhouette
#: is the collision shape and changing it changes the game.
BOTTOM_TAPER_LENGTH = 500.0


@dataclass
class Obstacle:
    """One column pair: solid above ``top_height``, solid below ``bottom_y``."""

    x: float
    top_height: int
    bottom_y: int
    style_index: int
    #: 0..1, standing in for ``Math.random()`` in the browser build.
    seed: float
    roughness: float
    asymmetry: float
    passed: bool = False
    #: The obstacle's own number when it is a multiple of the theme interval,
    #: else 0. The browser uses the count itself as the label.
    milestone: int = 0
    #: Index into the palette, rotated in runs rather than per obstacle.
    theme: int = 0

    def silhouette(self, world_y: int) -> tuple[float, float]:
        """``(left, right)`` of the solid part at ``world_y``.

        Returns an empty span when ``world_y`` is inside the gap.
        """
        centre = self.x + config.OBSTACLE_WIDTH / 2.0

        if world_y < self.top_height:
            y_pos = min(max(world_y, 0), self.top_height - 1)
            progress = y_pos / self.top_height
            if self.style_index == 0:
                w = max(22.0, config.OBSTACLE_WIDTH * (1.0 - progress * 0.65))
                offset = math.sin(self.seed * 100.0 + y_pos * 0.1) * 6.0 * self.asymmetry
            elif self.style_index == 1:
                w = max(24.0, config.OBSTACLE_WIDTH * (1.0 - progress * 0.6))
                offset = math.sin(self.seed * 50.0 + y_pos * 0.2) * 4.0
            else:
                w = max(20.0, config.OBSTACLE_WIDTH * (1.0 - progress * 0.65))
                offset = math.sin(self.seed * 200.0 + y_pos * 0.5) * 8.0 * self.roughness
            return (centre - w / 2.0 + offset, centre + w / 2.0 + offset)

        if world_y > self.bottom_y:
            y_pos = min(world_y - self.bottom_y, int(BOTTOM_TAPER_LENGTH))
            progress = y_pos / BOTTOM_TAPER_LENGTH
            if self.style_index == 0:
                w = min(config.OBSTACLE_WIDTH,
                        22.0 + config.OBSTACLE_WIDTH * progress * 0.65)
                offset = math.sin(self.seed * 100.0 + y_pos * 0.1) * 6.0 * self.asymmetry
            elif self.style_index == 1:
                w = min(config.OBSTACLE_WIDTH,
                        24.0 + config.OBSTACLE_WIDTH * progress * 0.6)
                offset = math.sin(self.seed * 50.0 + y_pos * 0.2) * 4.0
            else:
                w = min(config.OBSTACLE_WIDTH,
                        20.0 + config.OBSTACLE_WIDTH * progress * 0.65)
                offset = math.sin(self.seed * 200.0 + y_pos * 0.5) * 8.0 * self.roughness
            return (centre - w / 2.0 + offset, centre + w / 2.0 + offset)

        return (0.0, 0.0)


@dataclass
class ObstacleField:
    """Every column on screen, and the spawn bookkeeping behind them.

    A field rather than module state — the Wii port keeps these in file
    statics, which is fine for one console running one game and wrong for a
    launcher that may seat this twice in a session.
    """

    rng: random.Random = field(default_factory=random.Random)
    obstacles: list[Obstacle] = field(default_factory=list)
    total_created: int = 0
    theme: int = 0
    _last_theme_change: int = 0

    #: How many palettes the renderer offers. Set by whoever draws.
    theme_count: int = 1

    def reset(self) -> None:
        self.obstacles.clear()
        self.total_created = 0
        self._last_theme_change = 0
        self.theme = self.rng.randrange(self.theme_count)

    def _current_theme(self) -> int:
        """One palette shared by every column on screen, rotated in runs.

        A fresh theme per obstacle makes every column a different random
        colour, which is the main reason the Wii port did not read like the
        arcade version until this was fixed there. Mirrors
        ``getCurrentTheme()`` in ``js/obstacles.js``.
        """
        if self.total_created - self._last_theme_change >= config.THEME_CHANGE_INTERVAL:
            self.theme = self.rng.randrange(self.theme_count)
            self._last_theme_change = self.total_created
        return self.theme

    def create(self, world_width: int) -> None:
        """Spawn a column pair at the right edge of the visible world."""
        if len(self.obstacles) >= MAX_OBSTACLES:
            return

        top = config.MIN_OBSTACLE_HEIGHT + self.rng.randrange(
            config.MAX_OBSTACLE_HEIGHT - config.MIN_OBSTACLE_HEIGHT)
        self.total_created += 1

        # Count first, then test: the browser marks the 10th, 20th ... obstacle
        # and uses the count itself as the label.
        milestone = (self.total_created
                     if self.total_created % config.THEME_CHANGE_INTERVAL == 0
                     else 0)

        self.obstacles.append(Obstacle(
            x=float(world_width),
            top_height=top,
            bottom_y=top + config.GAP,
            style_index=self.rng.randrange(3),
            seed=self.rng.random(),
            roughness=0.5 + self.rng.random() * 0.5,
            asymmetry=self.rng.random() * 2.0 - 1.0,
            milestone=milestone,
            theme=self._current_theme(),
        ))

    def update(self) -> None:
        """Scroll every column left, dropping the ones that have left."""
        for obstacle in self.obstacles:
            obstacle.x -= config.OBSTACLE_SPEED
        self.obstacles = [o for o in self.obstacles
                          if o.x + config.OBSTACLE_WIDTH >= 0]

    def collides(self, hitbox: tuple[int, int, int, int]) -> bool:
        """Whether the player's hitbox is inside any column's silhouette.

        The bottom edge excludes the exhaust plume: it is drawn below the body
        and is not solid.
        """
        px, py, pw, ph = hitbox
        left, right = px, px + pw
        top, bottom = py, py + ph - config.FLAME_HEIGHT

        for o in self.obstacles:
            if right <= o.x or left >= o.x + config.OBSTACLE_WIDTH:
                continue
            # The upper column is tested at the player's top edge and the lower
            # one at its bottom edge — not both against both. Testing every
            # edge against every column would fail a player whose bottom edge
            # is level with a *tapering* upper column it is nowhere near, which
            # is a harder game than the browser plays.
            if top < o.top_height:
                s_left, s_right = o.silhouette(top)
                if right > s_left and left < s_right:
                    return True
            if bottom > o.bottom_y:
                s_left, s_right = o.silhouette(bottom)
                if right > s_left and left < s_right:
                    return True
        return False

    def score(self, player_x: int) -> int:
        """How many columns the player cleared this frame."""
        gained = 0
        for o in self.obstacles:
            if not o.passed and o.x + config.OBSTACLE_WIDTH < player_x:
                o.passed = True
                gained += 1
        return gained


__all__ = ["BOTTOM_TAPER_LENGTH", "MAX_OBSTACLES", "Obstacle", "ObstacleField"]
