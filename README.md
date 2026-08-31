# Moonlight Drift

Jetman-style arcade game: hold thrust to rise, release to fall, dodge the cave.
One repo, every version of the game.

| Version | Folder | Engine | Where it runs |
|---------|--------|--------|---------------|
| Browser | [`web/`](web/) | [adenosine](https://github.com/magmacrunch-media/adenosine) | [magmacrunch.com/arcade/moonlight-drift](https://magmacrunch.com/arcade/moonlight-drift/) |
| Wii | [`wii/`](wii/) | [magnolia](https://github.com/magmacrunch-media/magnolia) | Homebrew Channel |
| Terminal | [`tui/`](tui/) | [magmacrunch.engine](https://pypi.org/project/magmacrunch/) | any terminal |

## Layout

- `web/` — the browser version. **This folder is the source of truth**; the
  copy served from the website repo at `arcade/moonlight-drift/` is generated
  from it via `make sync-moonlight-drift` in that repo. Edit here, sync there.
- `wii/` — the Wii port. Builds with devkitPPC and expects the magnolia engine
  checked out beside this repo (`../../magnolia` from inside `wii/`). See
  [`wii/README.md`](wii/README.md).
- `tui/` — the terminal version. Runs on the
  [magmacrunch.engine](https://pypi.org/project/magmacrunch/) engine's terminal backend.
  Install with `pipx install magmacrunch-moonlight-drift` and launch with
  `moonlight-drift`. Also available as part of the
  [magmacrunch](https://pypi.org/project/magmacrunch/) arcade.
  See [`tui/README.md`](tui/README.md).

## Working on the game

A gameplay or balance change usually lands in all three versions: the browser
sources under `web/js/` are the reference implementation, and both the Wii
port's C modules and the TUI's Python scenes cite the `web/js/` file each
behaviour came from. Change `web/` first, then carry the change into
`wii/source/` and `tui/`.

This repo was formed from `moonlight-drift-wii` (whose history it keeps) plus
the browser version imported from the website repo.

## License

[PolyForm Noncommercial License 1.0.0](LICENSE) — read it, learn from it, build
on it, play with it. Any noncommercial purpose is permitted; commercial use is
reserved. The game's name, art, audio and visual design are reserved outright
and are not covered by that licence: see [NOTICE](NOTICE) for the exact
boundary and the third-party components.
