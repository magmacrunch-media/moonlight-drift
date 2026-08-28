# Moonlight Drift — terminal version

The third version of the game, alongside `web/` (browser) and `wii/`. Runs on
the [texastoast](https://pypi.org/project/texastoast/) engine's terminal
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

Hold SPACE or ↑ to climb, release to fall, thread the columns. It is also a
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
otherwise. Nothing is sent anywhere: see `texastoast.scores`.

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
