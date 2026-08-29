"""Palette and layout, in character cells.

Split out so :mod:`drift.scenes` and :mod:`drift.app` can share it without
importing each other. Nothing here imports the engine.

The colours are the browser build's: the page's night-sky gradient runs
``#0f0c29 → #302b63 → #24243e``, and a terminal takes the darkest of the three
as a flat background because it has no gradients. Obstacle colours are not
here — each column carries its own generated palette, the way the browser makes
them. See :func:`drift.obstacles.generate_theme`.

Every measurement is **cells**, not world units. The world is 1280x720 and
lives in :mod:`drift.config`; the two spaces are kept in separate modules on
purpose, because the moment they share one they get mixed.
"""

from __future__ import annotations

import colorsys

# ── Palette ─────────────────────────────────────────────────────────

#: The darkest stop of the page gradient.
BG = "#0f0c29"
#: One step up, for the playfield behind the stars.
FIELD_BG = "#14102f"

TITLE = "#8ad3ff"
SUBTITLE = "#ff2e9c"
LABEL = "#6b6b8f"
VALUE = "#e8e8f4"
DIM = "#4a4a6a"
WARN = "#ffd54f"
DEAD = "#ff5f5f"

MENU_BOX = "#1b1730"
MENU_SELECTED = "#8ad3ff"
MENU_SELECTION_BG = "#23204a"

#: The exhaust plume, when thrust is held.
FLAME = "#ff7043"

# ── Layout ──────────────────────────────────────────────────────────

#: The score bar above the playfield and the key hints below it. The playfield
#: is what is left, and it is what the projection is computed against — the top
#: and bottom of the world are lethal, so the kill line has to be somewhere the
#: player can actually see.
HEADER_ROWS = 1
FOOTER_ROWS = 1

#: Smallest terminal the game draws in. The gap is a quarter of the world's
#: height at any size, so 18 playfield rows puts it at four and a half — tight,
#: and the floor rather than the target.
MIN_COLS = 60
MIN_ROWS = HEADER_ROWS + 18 + FOOTER_ROWS

#: The title screen is the smaller of the two, so it sets its own floor.
MENU_MIN_COLS = 44
MENU_MIN_ROWS = 14

# Menu geometry, in cells. Passed to the engine's Menu widget in place of its
# pixel defaults (280 wide, 32-cell rows), which would sit entirely off-screen.
MENU_W = 34
MENU_ITEM_H = 1
MENU_TITLE_H = 0
MENU_PAD = 1
MENU_BORDER = 1

BANNER = "MOONLIGHT DRIFT"
TAGLINE = "hold to climb, release to fall"

#: How the name is set, best first: the block text, and whatever is left of it
#: in plain text beneath. Every rung spells the whole name — see the two other
#: terminal games, which do the same.
TITLE_LADDER = (
    ("MOONLIGHT\nDRIFT", ""),
    ("DRIFT", "MOONLIGHT"),
)

#: What the world is drawn with. Full block for columns, so a column reads as
#: solid at one cell wide; a lighter shade for the tapered edge.
COLUMN = "█"
COLUMN_EDGE = "▓"
GROUND = "─"

# ── Contrast ────────────────────────────────────────────────────────
#
# The starfield used to be louder than the ship. A white star sat at 18.3:1
# against the playfield and the dimmest pilot at 2.8:1 — sixty bright marks and
# one dim one, with the dim one being the thing you steer. These put the
# hierarchy the right way up and let a test assert it rather than leaving it to
# taste.

#: How much of its stated colour a star keeps. Stars are scenery.
#:
#: The palette itself is left alone: it is ported from the browser's
#: SNES_STAR_COLORS and should keep saying what the browser says. What differs
#: is the medium. On the web a star is a few pixels of a 1280x720 canvas; in a
#: terminal it is a whole cell out of about 1,700, so the same star is two
#: orders of magnitude more prominent. Dimming corrects for that rather than
#: redesigning the sky. At 0.45 a white star lands near 3.8:1 — still plainly
#: a star, no longer shouting.
STAR_DIM = 0.45

#: The floor a ship must clear against :data:`FIELD_BG`. Roughly twice the
#: brightest dimmed star, so the ship wins the screen wherever it is.
SHIP_CONTRAST = 7.0


def _channel(value: float) -> float:
    return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4


def luminance(colour: str) -> float:
    """Relative luminance of ``#rrggbb``, per WCAG."""
    r, g, b = (int(colour[i:i + 2], 16) / 255 for i in (1, 3, 5))
    return (0.2126 * _channel(r) + 0.7152 * _channel(g) + 0.0722 * _channel(b))


def contrast(a: str, b: str) -> float:
    """Contrast ratio between two colours, 1.0 to 21.0."""
    lo, hi = sorted((luminance(a), luminance(b)))
    return (hi + 0.05) / (lo + 0.05)


def dim(colour: str, factor: float = STAR_DIM) -> str:
    """``colour`` scaled towards black."""
    r, g, b = (int(colour[i:i + 2], 16) for i in (1, 3, 5))
    return f"#{int(r * factor):02x}{int(g * factor):02x}{int(b * factor):02x}"


def readable(colour: str, against: str = FIELD_BG,
             target: float = SHIP_CONTRAST) -> str:
    """``colour`` lightened until it clears ``target`` contrast.

    Lightness is not luminance, which is the trap the sprite-derived accents
    fell into: blue contributes 0.0722 to luminance against green's 0.7152, so
    a blue raised to a perfectly respectable HSL lightness is still dark.
    Roderick Tron's #4d4dcb is "light" and sits at 2.8:1. Lifting against the
    measurement instead of against lightness is what makes every pilot legible
    rather than most of them.
    """
    r, g, b = (int(colour[i:i + 2], 16) / 255 for i in (1, 3, 5))
    hue, light, sat = colorsys.rgb_to_hls(r, g, b)
    best = colour
    for step in range(61):
        lifted = min(0.95, light + step * 0.01)
        rr, gg, bb = colorsys.hls_to_rgb(hue, lifted, sat)
        best = f"#{round(rr * 255):02x}{round(gg * 255):02x}{round(bb * 255):02x}"
        if contrast(best, against) >= target:
            break
    return best


__all__ = [
    "BANNER", "BG", "COLUMN", "COLUMN_EDGE", "DEAD", "DIM", "FIELD_BG",
    "FLAME", "FOOTER_ROWS", "GROUND", "HEADER_ROWS", "LABEL", "MENU_BORDER",
    "MENU_BOX", "MENU_ITEM_H", "MENU_MIN_COLS", "MENU_MIN_ROWS", "MENU_PAD",
    "MENU_SELECTED", "MENU_SELECTION_BG", "MENU_TITLE_H", "MENU_W",
    "MIN_COLS", "MIN_ROWS", "SHIP_CONTRAST", "STAR_DIM", "SUBTITLE",
    "TAGLINE", "TITLE", "TITLE_LADDER", "VALUE", "WARN", "contrast", "dim",
    "luminance", "readable",
]
