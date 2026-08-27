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
