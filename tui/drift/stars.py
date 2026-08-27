"""The starfield, in world coordinates.

Ported from ``wii/source/stars.c``, which cites ``js/stars.js``. Like the rest
of the simulation it knows nothing about a terminal — it holds positions in the
1280x720 world and a visibility flag per star, and something else decides what
a lit star looks like.

Two details carry the look, and both are the kind that get "simplified" out of
a port and then never quite look right again:

* **White is in the palette three times.** An evenly sampled palette reads as
  noise; the accents have to be rare enough to be accents.
* **The blink is a four-phase cycle that is dark on one phase**, so a star is
  lit 75% of the time. Toggling instead gives a 50% duty and reads as
  flickering rather than twinkling.

Shooting stars are deliberately not here yet. They are decoration on top of
decoration, and the core loop is the thing worth getting right first.
"""

from __future__ import annotations

import random
from dataclasses import dataclass, field

from drift import config

STAR_PATTERN_COUNT = 7

#: The web's ``SNES_STAR_COLORS``. White three times so it dominates, with pale
#: blue/pink and cyan/gold accents.
PALETTE: tuple[str, ...] = (
    "#ffffff",
    "#ffffff",
    "#ffffff",
    "#e0e0ff",
    "#ffe0e0",
    "#00d4ff",
    "#ffd700",
)

#: What a star is drawn as, by pattern. A terminal has one cell to say it in,
#: so the patterns become weights of ink rather than shapes.
GLYPHS: tuple[str, ...] = ("·", "*", "·", "+", "·", "✦", ".")


@dataclass
class Star:
    x: int
    y: int
    pattern: int
    color: str
    blink_speed: int
    is_pulsing: bool
    blink_frame: int = 0
    frame_counter: int = 0
    visible: bool = True
    pulse_state: int = 0

    @property
    def glyph(self) -> str:
        return GLYPHS[self.pattern % len(GLYPHS)]


@dataclass
class Starfield:
    """Every star, and the frame counter that twinkles them.

    A field rather than module state, for the reason
    :class:`drift.obstacles.ObstacleField` is: a launcher may seat this game
    twice in one session, and the second run should not inherit the first
    one's sky.
    """

    rng: random.Random = field(default_factory=random.Random)
    stars: list[Star] = field(default_factory=list)

    def seed(self, world_w: int, world_h: int = config.WORLD_HEIGHT) -> None:
        """Scatter across the *visible* world, not the whole 1280.

        How much of the field is on screen depends on the terminal's shape —
        see :func:`drift.projection.compute` — and stars seeded past the right
        edge would simply never be drawn.
        """
        world_w = max(1, world_w)
        world_h = max(1, world_h)
        self.stars = [
            Star(
                x=self.rng.randrange(world_w),
                y=self.rng.randrange(world_h),
                pattern=self.rng.randrange(STAR_PATTERN_COUNT),
                color=self.rng.choice(PALETTE),
                blink_speed=30 + self.rng.randrange(40),
                is_pulsing=self.rng.randrange(100) < 30,
            )
            for _ in range(config.STAR_COUNT)
        ]

    def update(self) -> None:
        for star in self.stars:
            star.frame_counter += 1
            if star.frame_counter < star.blink_speed:
                continue

            star.frame_counter = 0
            # Four phases, dark only on the last: lit 75% of the time.
            star.blink_frame = (star.blink_frame + 1) % 4
            if star.blink_frame == 3:
                star.visible = False
            else:
                star.visible = True
                if star.is_pulsing and star.blink_frame == 0:
                    star.pulse_state = (star.pulse_state + 1) % 2

    @property
    def lit(self) -> list[Star]:
        return [s for s in self.stars if s.visible]


__all__ = ["GLYPHS", "PALETTE", "STAR_PATTERN_COUNT", "Star", "Starfield"]
