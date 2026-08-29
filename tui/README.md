# Moonlight Drift — terminal version

The third version of the game, alongside `web/` (browser) and `wii/`. Runs on
the [magmacrunch.engine](https://pypi.org/project/magmacrunch/) engine's terminal
backend.

```
pipx install magmacrunch-moonlight-drift
moonlight-drift
```

`pipx` rather than `pip` because it puts the command on your PATH in
its own virtualenv; plain `pip install` only reaches your PATH inside an
activated venv. It is also a cabinet in the
[magmacrunch](https://pypi.org/project/magmacrunch/) arcade — `pipx install magmacrunch`
gets this and the other two — and plays identically either way.

For working on it:

```
pip install -e ".[dev]"
python -m drift
```

Tap SPACE or ↑ to climb, hold to keep climbing, release to fall.

**Thrust is a press, not a held key** — a terminal cannot report that a key is
still down, only that it was pressed, and a keyboard goes silent for about half
a second before it starts repeating. Reading held state through that silence
gives a boost that cuts out exactly when you press it. So each press is a hop
whose arc outlasts the repeat delay: holding reads as a continuous climb, and a
single tap is a crisp one. See `drift/config.py`. It is also a
cabinet in the [magmacrunch](https://github.com/magmacrunchmedia/magmacrunch)
arcade, and plays identically either way.

## On the title screen

**Choose a pilot** opens the roster: all 24, with the highlighted one's thrust,
gravity and top speed underneath. They fly differently enough to notice in the
first few seconds, and who you are flying is shown on the title screen so a
choice that went nowhere is obvious.

**How to play** is the browser build's instructions plus the one thing a
terminal has to explain that a browser does not — a keyboard reports presses
and never releases, so thrust is inferred from auto-repeat and lets go about a
tenth of a second after you do.

Both scroll: 24 pilots and the rules are each longer than a standard terminal
is tall.

```
moonlight-drift --list-characters       the roster, without opening the game
moonlight-drift --character fire-toad   fly as someone in particular
```

## Seeing your ship

The sky used to be louder than the ship. A white star sat at 18.3:1 contrast
against the playfield and the dimmest pilot at 2.8:1 — sixty bright marks and
one dim one, the dim one being the thing you steer.

Stars are dimmed on the way to the screen (the palette itself is the browser's
`SNES_STAR_COLORS` and is left alone), the ship is drawn as a filled block with
its glyph punched out of it, and each pilot's flight colour is lifted until it
clears a contrast floor. Ships now sit at 7:1 or better against a brightest
star of 3.8:1, and a test asserts the *hierarchy* rather than any particular
pair of colours.

Lifting is measured, not guessed at lightness: blue contributes 0.0722 to
luminance where green gives 0.7152, so Roderick Tron's perfectly respectable
"light" blue was still dark. Only flight is lifted — the roster keeps each
sprite's true colour beside its portrait.

## Portraits

The roster shows each pilot's actual sprite, generated from the 48 PNGs the Wii
port exported from the browser's canvas `draw()` calls. A cell is two stacked
pixels — `▀` with its own foreground and background — which makes the pixel
grid square in a terminal whose cells are twice as tall as they are wide, so a
14-cell portrait is a real 14×16 image rather than a pictogram.

```
pip install -e ".[tools]"
python tools/make_portraits.py     # rewrites drift/portraits.py
python tools/show_portraits.py     # look at them, in truecolor
```

`drift/portraits.py` is **generated and committed**, so playing needs no image
library and decodes nothing at runtime — Pillow is a tool here, not a
dependency. Regenerate after a character is redrawn and re-exported.

The accent colours come from the same place, saturation-weighted. Picked by eye
first, several were simply wrong: Fire Toad had been coloured from its flames
rather than the toad, and the SVFP Van was cyan when the van is magenta.

Portraits want a truecolor terminal; on 256 colours they approximate. The
in-play glyph stays hand-authored — at two cells wide there is nothing to
downsample a sprite *to*.

## High scores

Kept on disk, so a record outlives the session. **Filed under the same key the
browser build posts to** (`moonlight-drift`), which is what would make a shared
board later a shared board rather than two boards with the same name.

A run that makes the table is asked for three initials, over the wreck rather
than instead of it. One that does not is recorded anyway — it just has not
earned being asked about. Each entry remembers the pilot who flew it.

```
MAGMACRUNCH_DATA_DIR=/somewhere/else moonlight-drift
```

The board lives under your platform's user data directory unless that says
otherwise. Nothing is sent anywhere: see `magmacrunch.engine.scores`.

## What is here

| | |
|---|---|
| `drift/config.py` | the browser's 1280×720 world, and every measurement in it |
| `drift/projection.py` | that world onto character cells |
| `drift/player.py` | gravity, thrust, and the two lethal edges |
| `drift/obstacles.py` | the columns, their tapered silhouettes and collision |
| `drift/characters.py` | 24 pilots — physics, hitboxes, a glyph and a colour |
| `drift/stars.py` | the starfield, twinkling on a four-phase cycle |
| `drift/scenes.py` | the title screen and the run |

None of it imports the engine. `tests/test_physics.py` runs with nothing but
pytest on the machine, which is what proves that claim.

## Drawing a column

The columns taper and wander sideways on a sine. In the browser that wander is
a fine ripple down a 300-pixel column — surface texture. A terminal row covers
about 33 world units, and the sine completes **two and a half turns inside one
of them**, so sampling once per row lands on an arbitrary phase and draws
noise: the edge jitters a cell either way with no relation to the shape.

So a cell asks what the column does across the whole span it covers and takes
the outermost answer. The wobble collapses into a stable envelope and the taper
survives, because the taper varies over the column's length rather than within
a cell. The wobble is not lost to a simplification — it was never resolvable at
this size, and what was being drawn instead was aliasing.

## The two things a terminal changes

**A character cell is about twice as tall as it is wide.** The Wii port has the
same problem from the other direction — non-square framebuffer pixels — and
solves it in `wii/source/projection.c` by deriving a pixel-aspect ratio and
fitting the world's *height* to the safe area. Here that ratio is a constant
`0.5`, and the same fit applies: the gap and the player keep exactly the share
of the screen they have in the browser, and the width is cropped. A short wide
terminal sees less of the 1280 than a tall one, exactly as a 4:3 television
sees less than a 16:9 one.

**The player is about two cells wide and one tall.** So a character cannot
carry its art — on the web each is canvas primitives with hue cycling, on the
Wii a baked PNG, and `draw_image` is a no-op in the terminal backend. What
survives is a glyph, an accent colour from its web palette, and its physics —
which is the part you actually feel.

## Running the tests

```
pip install -e ".[dev]"
python -m pytest -q
```

The simulation suite needs no engine at all:

```
pip install pytest
python -m pytest tests/test_physics.py -q
```

## Licence

PolyForm Noncommercial 1.0.0 — see [LICENSE](LICENSE) and [NOTICE](NOTICE).
