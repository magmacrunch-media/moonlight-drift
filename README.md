# Moonlight Drift

Jetman-style arcade game: hold thrust to rise, release to fall, dodge the cave.
One repo, every version of the game.

| Version | Folder | Engine | Where it runs |
|---------|--------|--------|---------------|
| Browser | [`web/`](web/) | [adenosine](https://github.com/magmacrunchmedia/adenosine) | [magmacrunch.com/arcade/moonlight-drift](https://magmacrunch.com/arcade/moonlight-drift/) |
| Wii | [`wii/`](wii/) | [magnolia](https://github.com/magmacrunchmedia/magnolia) | Homebrew Channel |

## Layout

- `web/` — the browser version. **This folder is the source of truth**; the
  copy served from the website repo at `arcade/moonlight-drift/` is generated
  from it via `make sync-moonlight-drift` in that repo. Edit here, sync there.
- `wii/` — the Wii port. Builds with devkitPPC and expects the magnolia engine
  checked out beside this repo (`../../magnolia` from inside `wii/`). See
  [`wii/README.md`](wii/README.md).

## Working on the game

A gameplay or balance change usually lands in both versions: the browser
sources under `web/js/` are the reference implementation, and the Wii port's C
modules cite the `web/js/` file each behaviour came from. Change `web/` first,
then carry the change into `wii/source/`.

This repo was formed from `moonlight-drift-wii` (whose history it keeps) plus
the browser version imported from the website repo.
