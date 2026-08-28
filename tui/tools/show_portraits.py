"""Print the generated portraits in truecolor, to look at them.

    python tools/show_portraits.py            all of them
    python tools/show_portraits.py fire-toad  just one

Needs a terminal that does 24-bit colour, which is what the portraits need
anyway. Not part of the game and not shipped in the wheel — this is here so a
regeneration can be eyeballed before it is committed, because the only way to
know a portrait reads is to look at it.
"""

from __future__ import annotations

import sys

from drift import characters, portraits


def ansi(hex_colour: str, background: bool = False) -> str:
    r = int(hex_colour[1:3], 16)
    g = int(hex_colour[3:5], 16)
    b = int(hex_colour[5:7], 16)
    return f"\x1b[{48 if background else 38};2;{r};{g};{b}m"


def render(key: str) -> list[str]:
    lines = []
    for row in portraits.PORTRAITS[key]:
        out = ""
        for cell in row:
            if cell is None:
                out += "\x1b[0m "
                continue
            glyph, fg, bg = cell
            out += "\x1b[0m" + ansi(fg)
            if bg:
                out += ansi(bg, background=True)
            out += glyph
        lines.append(out + "\x1b[0m")
    return lines


def main() -> None:
    wanted = sys.argv[1:] or list(portraits.PORTRAITS)
    for key in wanted:
        if key not in portraits.PORTRAITS:
            print(f"no portrait for {key!r}")
            continue
        pilot = characters.get(key)
        accent = portraits.ACCENTS[key]
        print(f"\n  {ansi(accent)}{pilot.glyph}\x1b[0m  {pilot.name}"
              f"   \x1b[2maccent {accent}\x1b[0m")
        for line in render(key):
            print("      " + line)


if __name__ == "__main__":
    main()
