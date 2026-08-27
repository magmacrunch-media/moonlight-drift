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

__all__ = [
    "BANNER", "BG", "COLUMN", "COLUMN_EDGE", "DEAD", "DIM", "FIELD_BG",
    "FLAME", "FOOTER_ROWS", "GROUND", "HEADER_ROWS", "LABEL", "MENU_BORDER",
    "MENU_BOX", "MENU_ITEM_H", "MENU_MIN_COLS", "MENU_MIN_ROWS", "MENU_PAD",
    "MENU_SELECTED", "MENU_SELECTION_BG", "MENU_TITLE_H", "MENU_W", "MIN_COLS",
    "MIN_ROWS", "SUBTITLE", "TAGLINE", "TITLE", "TITLE_LADDER", "VALUE", "WARN",
]
