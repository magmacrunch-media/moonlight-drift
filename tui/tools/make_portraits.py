"""Turn the Wii port's sprite PNGs into terminal portraits.

Run at development time, never at runtime. It writes ``drift/portraits.py``,
which is committed — the game keeps its zero runtime dependencies and there is
no image decoding when somebody plays. Same arrangement as the block face in
``texastoast.ui.bigtext``: the drawing is data, and the tool that made it is
not shipped.

    pip install pillow
    python tools/make_portraits.py

**Why half blocks.** A terminal cell is about twice as tall as it is wide, so
one character per pixel gives a portrait squashed to half height. ``▀`` with a
foreground and a background colour is two stacked pixels in one cell, which
makes the grid square again and doubles the vertical resolution. Fourteen cells
across is a real 14x12 image rather than a pictogram.

**Why the accent comes from here too.** The accents were picked by eye first and
several were simply wrong — Fire Toad was colour-picked from its flames rather
than the toad, and the SVFP Van was cyan when the van is magenta. The sprite
knows; nobody needs to guess. The most saturated colour with real coverage wins,
because that is what a person would call the character's colour, where the mean
is always mud.

The source PNGs are ``wii/sprites/<key>-idle.png``, exported from the browser
build's canvas ``draw()`` calls when the Wii port was written. Regenerating
after a character is redrawn is the whole point of doing it this way.
"""

from __future__ import annotations

import colorsys
import pathlib
import sys
from collections import Counter

try:
    from PIL import Image
except ImportError:
    sys.exit("needs Pillow: pip install pillow")

REPO = pathlib.Path(__file__).resolve().parent.parent.parent
SPRITES = REPO / "wii" / "sprites"
OUT = pathlib.Path(__file__).resolve().parent.parent / "drift" / "portraits.py"

#: The box a portrait is fitted inside, in cells. Fitted rather than scaled to
#: the width, because these sprites are not all the same shape: scaling by
#: width alone gave portraits between five and fifteen rows tall, and fifteen
#: does not fit beside a roster on an 80x24 terminal. Every portrait now sits
#: in the same box, so a caller can lay out around one size.
COLS = 14
ROWS = 8

#: Below this the pixel is transparent and nothing is drawn. Generous, because
#: these sprites are anti-aliased against nothing and their edges fade out.
ALPHA = 96


def load(key: str) -> Image.Image:
    """The sprite, cropped to what is actually drawn.

    The PNGs are 128x128 with the character somewhere inside — Fire Toad fills
    601 of 16384 pixels. Cropping first is what makes a fixed output size mean
    the same thing for every character.
    """
    im = Image.open(SPRITES / f"{key}-idle.png").convert("RGBA")
    return im.crop(im.getbbox())


def accent(im: Image.Image) -> str:
    """The colour a person would call this character's.

    Saturation-weighted rather than the most common colour: sprites are mostly
    shading, so frequency alone returns a dark neutral for nearly everyone. The
    weight is squared so a small bright marking beats a large muted field,
    which is how the eye picks a character's colour out too.
    """
    scores: Counter = Counter()
    for x in range(im.width):
        for y in range(im.height):
            r, g, b, a = im.getpixel((x, y))
            if a <= ALPHA:
                continue
            h, light, sat = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
            if light < 0.12 or light > 0.94:
                continue        # near-black and near-white carry no hue
            bucket = (r // 24 * 24, g // 24 * 24, b // 24 * 24)
            scores[bucket] += sat * sat
    if not scores:
        return "#c8c8c8"
    r, g, b = scores.most_common(1)[0][0]
    # Lift it clear of the background: these are drawn on a night sky.
    h, light, sat = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
    r, g, b = colorsys.hls_to_rgb(h, max(light, 0.55), max(sat, 0.55))
    return f"#{round(r * 255):02x}{round(g * 255):02x}{round(b * 255):02x}"


def cells(im: Image.Image, cols: int = COLS, rows: int = ROWS
          ) -> list[list[tuple[str, str] | None]]:
    """One entry per cell: ``(top, bottom)`` hex, or None for nothing drawn.

    Fitted inside the box rather than filling it, so a wide character does not
    come out stretched tall and a tall one does not come out squashed. A cell
    holds two pixels, which is what makes the aspect work out square.
    """
    scale = min(cols / im.width, rows * 2 / im.height)
    w = max(1, round(im.width * scale))
    h = max(2, round(im.height * scale / 2) * 2)
    cols, rows = w, h // 2
    small = im.resize((w, h), Image.LANCZOS)
    out = []
    for r in range(rows):
        line: list[tuple[str, str] | None] = []
        for c in range(cols):
            top = small.getpixel((c, r * 2))
            bot = small.getpixel((c, r * 2 + 1))
            ta, ba = top[3] > ALPHA, bot[3] > ALPHA
            if not ta and not ba:
                line.append(None)
            else:
                # A transparent half takes its partner's colour; the caller
                # draws a half block so only the opaque half is inked.
                fg = top if ta else bot
                bg = bot if ba else top
                line.append((f"#{fg[0]:02x}{fg[1]:02x}{fg[2]:02x}",
                             f"#{bg[0]:02x}{bg[1]:02x}{bg[2]:02x}",
                             ta, ba))
        out.append(line)
    return out


def main() -> None:
    keys = sorted(p.stem[:-5] for p in SPRITES.glob("*-idle.png"))
    print(f"{len(keys)} sprites in {SPRITES}")

    parts = ['''"""Portraits, generated from the Wii port's sprite PNGs.

**Do not edit by hand.** Regenerate with ``python tools/make_portraits.py``
after a character is redrawn and re-exported. The generator's docstring
explains the half-block encoding and why the accents come from here.

Each portrait is a tuple of rows; each row a tuple of cells; each cell either
None for nothing drawn, or ``(glyph, foreground, background)``. Committed as
data so the game needs no image library and decodes nothing at runtime.
"""

from __future__ import annotations

#: Cells across, for a caller laying out around one.
WIDTH = ''' + str(COLS) + '''

ACCENTS: dict[str, str] = {
''']
    art_parts = []
    for key in keys:
        im = load(key)
        parts.append(f'    "{key}": "{accent(im)}",\n')
        grid = cells(im)
        rows_src = []
        for row in grid:
            cells_src = []
            for cell in row:
                if cell is None:
                    cells_src.append("None")
                else:
                    fg, bg, ta, ba = cell
                    glyph = "\\u2580" if ta else "\\u2584"
                    if ta and ba:
                        cells_src.append(f'("\\u2580", "{fg}", "{bg}")')
                    else:
                        cells_src.append(f'("{glyph}", "{fg}", None)')
            rows_src.append("        (" + ", ".join(cells_src) + "),")
        art_parts.append(f'    "{key}": (\n' + "\n".join(rows_src) + "\n    ),\n")
    parts.append("}\n\nPORTRAITS: dict[str, tuple] = {\n")
    parts.extend(art_parts)
    parts.append("}\n\n__all__ = [\"ACCENTS\", \"PORTRAITS\", \"WIDTH\"]\n")

    OUT.write_text("".join(parts), encoding="utf-8", newline="\n")
    size = OUT.stat().st_size
    print(f"wrote {OUT.relative_to(OUT.parent.parent)} — {size // 1024} KB")


if __name__ == "__main__":
    main()
