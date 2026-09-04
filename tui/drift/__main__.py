"""``python -m drift`` — play Moonlight Drift in a terminal."""

from __future__ import annotations

import argparse


def main() -> None:
    parser = argparse.ArgumentParser(
        prog="moonlight-drift",
        description="Moonlight Drift - terminal version.",
    )
    parser.add_argument(
        "--character", default=None,
        help="who to fly as, by key (see --list-characters)",
    )
    parser.add_argument(
        "--list-characters", action="store_true",
        help="print the roster and exit",
    )
    parser.add_argument(
        "--seed", type=int, default=None,
        help="fix the run, for a reproducible one",
    )
    parser.add_argument(
        "--play", action="store_true",
        help="skip the title screen and start drifting",
    )
    parser.add_argument(
        "--ascii", action="store_true", dest="ascii_only",
        help="draw with plain ASCII instead of block, arrow and suit "
             "glyphs. Detected automatically from the terminal's "
             "encoding; this forces it, for a font that lacks the "
             "pictures. MAGMACRUNCH_ASCII=1 says the same for every "
             "cabinet at once.",
    )
    args = parser.parse_args()

    if args.list_characters:
        # Imported here so --help costs nothing.
        from drift import characters

        for i, c in enumerate(characters.ROSTER):
            print(f"{i:>2}  {c.glyph}  {c.key:<18} {c.name:<20} "
                  f"thrust {c.thrust}  gravity {c.gravity}  max {c.max_velocity}")
        return

    # Imported here, not at module scope, so --help works without the engine
    # or its terminal extra installed.
    from drift.app import run

    run(args.character, args.seed, args.play, args.ascii_only)


if __name__ == "__main__":
    main()
