# Moonlight Drift — agent brief

One game, two versions, one repo:

- `web/` — browser version (adenosine engine, plain JS). Source of truth for
  gameplay and balance. Deployed by the website repo: run
  `make sync-moonlight-drift` there to copy `web/` into `arcade/moonlight-drift/`.
  Never edit the website repo's copy directly — it gets overwritten.
- `wii/` — Wii port (magnolia engine, C99). Has its own `AGENTS.md` with build
  and porting detail. Expects magnolia checked out beside this repo.
- `tui/` — terminal version (the `magmacrunch.engine` TUI engine, Python).
  Has its own `README.md`. `moonlight-drift`, or seated by the magmacrunch arcade.

A gameplay change is not done until all three versions have it (or the commit
says why one is skipped). `web/js/` is the reference the Wii port's comments
cite, and the terminal port was taken from `wii/source/` for the same reason
George Boole's was — it is the same rules already separated from a renderer.

## Cache-buster stamps in `web/index.html`

Every `?v=` in `web/index.html` is the first eight hex of SHA-256 over the file
it stamps, with newlines normalised to LF. Get one wrong and visitors keep
serving the cached old bytes, so the change reaches nobody and the page still
loads.

`../shared/adenosine-audio.js` is the one to watch: it names a file that does
not exist in this repo at all. It resolves only once `web/` has been copied
into the website's `arcade/`, and it goes stale when *that* repo updates the
shared bundle — which nothing here can notice.

The website's `.githooks/pre-commit` recomputes stale stamps in its copy of
this page, but that repair never travels back here, and
`make sync-moonlight-drift` copies `web/` over `arcade/moonlight-drift/`
verbatim. So a stamp corrected there is reverted by the next sync, silently,
on a game nobody touched. **The fix belongs in this repo.** Recompute one from
a website checkout, and check the whole site after any sync:

```
node scripts/check-cache-busters.mjs --digest arcade/shared/adenosine-audio.js
npm run check:cachebust
```

## Where the rules live in each version

| | rules | rendering |
|---|---|---|
| `web/` | `js/player.js`, `js/obstacles.js` | same files, tangled with canvas |
| `wii/` | `source/player.c`, `obstacles.c`, `projection.c` | `game_render.c`, `obstacle_renderer.c`, `ui.c` |
| `tui/` | `drift/player.py`, `obstacles.py`, `projection.py` — no engine import | `drift/scenes.py` |

**`wii/source/` is the reference to port from**, not `web/js/` — it is the same
rules already separated from a renderer, and it was checked against the web
build when it was written.

## `tui/LICENSE` and `tui/NOTICE` are copies

The originals are at the repo root, where they cover `web/` and `wii/` too. The
copies exist because the wheel is built from `tui/` and PolyForm requires the
notice to travel with the distribution — a wheel built from a subdirectory
cannot reach a file above it. **Relicensing means changing both.**
