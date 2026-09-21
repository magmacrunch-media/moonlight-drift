"""Pack the pilots' sprites into one sheet for Roblox.

Roblox shows an image by asset id, and every id is a separate upload. So the 48
sprites in wii/sprites/ (idle and thrust for each of the 24 pilots) go into one
PNG, assets/pilots.png, and the game picks a sprite out of it with
ImageRectOffset. One upload instead of 48.

The layout is not decided here. The roster order and the sheet's grid numbers
are read from src/shared/Characters.luau, which the game uses to find each
sprite, so the two cannot disagree.

    python tools/make_atlas.py           # write assets/pilots.png
    python tools/make_atlas.py --check   # exit 1 if it is missing or stale

The check compares pixels, not bytes, so a different Pillow version encoding
the same image differently does not fail it.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

from PIL import Image, ImageChops

ROBLOX = Path(__file__).resolve().parent.parent
SPRITES = ROBLOX.parent / "wii" / "sprites"
CHARACTERS = ROBLOX / "src" / "shared" / "Characters.luau"
OUT = ROBLOX / "assets" / "pilots.png"


def read_layout() -> tuple[list[str], dict[str, int]]:
    source = CHARACTERS.read_text(encoding="utf-8")
    keys = re.findall(r'^\tc\("([^"]+)"', source, flags=re.M)
    numbers = {
        name: int(value)
        for name, value in re.findall(r"^Characters\.(SPRITE_SIZE|SHEET_\w+) = (\d+)", source, flags=re.M)
    }
    missing = {"SPRITE_SIZE", "SHEET_COLUMNS", "SHEET_CELL", "SHEET_PAD"} - numbers.keys()
    if not keys or missing:
        sys.exit(f"could not read the layout from {CHARACTERS}: missing {sorted(missing) or 'roster'}")
    return keys, numbers


def build() -> Image.Image:
    keys, n = read_layout()
    size, cols, cell, pad = n["SPRITE_SIZE"], n["SHEET_COLUMNS"], n["SHEET_CELL"], n["SHEET_PAD"]
    if cell < size + 2 * pad:
        sys.exit(f"a {cell} cell cannot hold a {size} sprite with {pad} padding")
    count = len(keys) * 2
    rows = -(-count // cols)
    sheet = Image.new("RGBA", (cols * cell, rows * cell), (0, 0, 0, 0))
    for i, key in enumerate(keys):
        for j, pose in enumerate(("idle", "thrust")):
            path = SPRITES / f"{key}-{pose}.png"
            sprite = Image.open(path).convert("RGBA")
            if sprite.size != (size, size):
                sys.exit(f"{path.name} is {sprite.size}, expected {size}x{size}")
            k = i * 2 + j
            sheet.paste(sprite, ((k % cols) * cell + pad, (k // cols) * cell + pad))
    if max(sheet.size) > 1024:
        sys.exit(f"sheet is {sheet.size}; Roblox scales uploads larger than 1024 down")
    return sheet


def main() -> int:
    sheet = build()
    if "--check" in sys.argv[1:]:
        if not OUT.exists():
            print(f"{OUT.relative_to(ROBLOX)} is missing; run tools/make_atlas.py")
            return 1
        current = Image.open(OUT).convert("RGBA")
        if current.size != sheet.size or ImageChops.difference(current, sheet).getbbox():
            print(f"{OUT.relative_to(ROBLOX)} is stale; run tools/make_atlas.py and re-upload it")
            return 1
        print(f"{OUT.relative_to(ROBLOX)} matches wii/sprites/")
        return 0
    OUT.parent.mkdir(exist_ok=True)
    sheet.save(OUT, optimize=True)
    print(f"wrote {OUT.relative_to(ROBLOX)} ({sheet.size[0]}x{sheet.size[1]})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
