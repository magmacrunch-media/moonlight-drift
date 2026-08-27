"""World coordinates onto character cells.

The arithmetic behind drawing, separated from the terminal it runs on — the
same split ``wii/source/projection.c`` makes against the console, and for the
same reason. The failure mode is silent: a wrong scale does not crash, it plays
the game at the wrong size, and :attr:`Projection.world_w` feeds the obstacle
spawn edge, the cull edge and the starfield spread. It changes how the game
*plays* and not just how it looks, so it is worth being able to check without
looking at a screen.

**A character cell is not square, and that is the whole problem** — the same
problem the Wii port has with non-square framebuffer pixels, arriving from the
other direction. There the pixel aspect had to be derived per video mode; here
it is one constant, because a cell is about twice as tall as it is wide in
every terminal font worth naming. That makes ``par`` a fixed 0.5 where the Wii
computes it from the display and framebuffer aspects.

The consequence is worth stating plainly: fitting the world's *height* to the
rows available is what keeps the gap and the player the share of the screen
they have in the browser. The width then follows, and is cropped — a short wide
terminal sees less of the 1280 than a tall one, exactly as a 4:3 television
sees less than a 16:9 one.
"""

from __future__ import annotations

from dataclasses import dataclass

from drift import config

#: How much taller than wide a character cell is. Two is right for essentially
#: every terminal font; it is a constant rather than a measurement because
#: nothing in a terminal will tell you the real number.
CELL_ASPECT = 2.0


@dataclass(frozen=True)
class Projection:
    """Where the world lands on the grid, and how much of it is visible."""

    #: Cells per world unit. ``scale_x`` is the larger of the two by
    #: :data:`CELL_ASPECT`, because a cell covers more world horizontally than
    #: it does vertically.
    scale_x: float
    scale_y: float
    origin_x: int
    origin_y: int
    #: How much of the 1280-wide world fits across the grid at this scale — the
    #: spawn edge, the cull edge, and how far the starfield spreads.
    world_w: int
    #: Pixel-aspect ratio: how much wider than tall one cell is. Always
    #: ``1 / CELL_ASPECT``. Carried out because it is the single value the whole
    #: projection turns on, and worth being able to assert directly.
    par: float
    #: False when the grid was degenerate. The caller keeps the identity
    #: projection rather than dividing by zero.
    valid: bool

    # -- World to cells ----------------------------------------------

    def x(self, world_x: float) -> float:
        return self.origin_x + world_x * self.scale_x

    def y(self, world_y: float) -> float:
        return self.origin_y + world_y * self.scale_y

    def w(self, world_w: float) -> float:
        return world_w * self.scale_x

    def h(self, world_h: float) -> float:
        return world_h * self.scale_y

    def col(self, world_x: float) -> int:
        return int(self.x(world_x))

    def row(self, world_y: float) -> int:
        return int(self.y(world_y))


def compute(cols: int, rows: int, origin_x: int = 0, origin_y: int = 0,
            cell_aspect: float = CELL_ASPECT) -> Projection:
    """The projection for a ``cols`` x ``rows`` region of the terminal.

    ``cols`` and ``rows`` are the *playfield*, not the whole window: the top and
    bottom of the world are lethal, so the kill line has to be somewhere the
    player can see, not underneath a score bar.
    """
    identity = Projection(scale_x=1.0, scale_y=1.0, origin_x=origin_x,
                          origin_y=origin_y, world_w=config.WORLD_WIDTH,
                          par=1.0, valid=False)
    if cols <= 0 or rows <= 0 or cell_aspect <= 0:
        return identity

    # A cell is `cell_aspect` times taller than wide, so as a pixel it is
    # `1 / cell_aspect` times as wide as tall. The Wii derives this from the
    # video mode; a terminal has one answer.
    par = 1.0 / cell_aspect

    # Fill vertically. The gap and the player keep exactly the share of the
    # screen they have on the web, which is what the game feels like.
    scale_y = rows / config.WORLD_HEIGHT
    scale_x = scale_y / par

    world_w = int(cols / scale_x)
    # Never show more game than exists.
    world_w = min(world_w, config.WORLD_WIDTH)

    return Projection(scale_x=scale_x, scale_y=scale_y, origin_x=origin_x,
                      origin_y=origin_y, world_w=world_w, par=par, valid=True)


__all__ = ["CELL_ASPECT", "Projection", "compute"]
