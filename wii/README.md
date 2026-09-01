# Moonlight Drift — Nintendo Wii Port

**Project:** Port [Moonlight Drift](https://magmacrunch.com/arcade/moonlight-drift/) (Jetman-style arcade game) to a homebrewed Nintendo Wii
**Engine:** [magnolia](https://github.com/magmacrunch-media/magnolia) `>= v0.3.0` — shared Wii game engine
**Status:** Phases 1–8 complete — 24 characters, selector grid, audio, TV-safe UI,
persistence, and a world-space playfield that matches the arcade version's geometry

> **A note on `js/` references.** Comments and docs here cite files like
> `js/obstacles.js` or `js/player.js`. Those are the browser version's sources,
> which now live in this repo at [`../web/js/`](../web/js/). The references
> record which function each port came from, so a behaviour can be traced back
> to the original.

---

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C (C99) |
| Compiler | devkitPPC (PowerPC cross-compiler) |
| Engine | [magnolia](https://github.com/magmacrunch-media/magnolia) |
| Graphics | GRRLIB (2D rendering via GX) |
| Font | Press Start 2P (embedded TTF) |
| Storage | SD card (FatFS via libfat-ogc) |
| Build system | GNU Make + devkitPPC wii_rules |

---

## Project Structure

```
moonlight-drift/wii/
├── Makefile                     # Game build config, references ../../magnolia
├── meta.xml                     # Homebrew Channel metadata
├── Moonlight-Drift-Wii.md       # Full design document
├── source/
│   ├── main.c                   # Entry point; drives magnolia's GameStateMachine
│   ├── config.h                 # World constants, APP_NAME, overscan
│   ├── playfield.c/.h           # World (1280x720) -> screen projection
│   ├── player.c/.h              # Jetman physics (thrust, gravity, boundary)
│   ├── stars.c/.h               # Blinking starfield
│   ├── characters.c/.h          # Character roster, hitboxes, sprite origins
│   ├── obstacles.c/.h           # Cave obstacle system (3 styles)
│   ├── obstacle_renderer.c/.h   # Obstacle drawing + milestone markers
│   ├── game_render.c/.h         # Player/starfield/score/kill-line drawing
│   ├── settings.c/.h            # Character + mute preference on SD
│   └── ui.c/.h                  # Title, selector grid, game over, initials, scores
└── sprites/                     # Character PNGs (idle + thrust frames)
    ├── tardigrade-idle.png
    ├── tardigrade-thrust.png
    ├── dag-henderson-idle.png
    ├── dag-henderson-thrust.png
    ├── cat-synth-idle.png
    ├── cat-synth-thrust.png
    ├── vinny-bobarino-idle.png
    ├── vinny-bobarino-thrust.png
    ├── carl-spatski-idle.png
    ├── carl-spatski-thrust.png
    ├── gangsta-beaver-idle.png
    └── gangsta-beaver-thrust.png
```

**Engine code** (core bring-up, rendering, sprites, input, audio, scoring, game state, theme, UI utils, font) lives in [magnolia](https://github.com/magmacrunch-media/magnolia) and is compiled via `../magnolia` in the Makefile.

Player physics, the starfield and the character roster are **not** in the engine —
they encode this game's decisions, so they live here. See magnolia's README for
the rule on what belongs where.

---

## Building

### Prerequisites

- devkitPro installed at `/opt/devkitpro/`
- magnolia `>= v0.3.0` cloned at `../magnolia` relative to this project.
  There is no version pinning between the two repos yet, so if the engine has
  moved on and the game stops building, check out the matching engine tag:
  `git -C ../magnolia checkout v0.3.0`
- A machine with the toolchain on it. The build happens under WSL2 Ubuntu on a
  Windows box here; anything with devkitPro and a POSIX shell will do.

### Build Commands

```bash
# If the toolchain lives on another machine, get a shell on it first
# ssh <you>@<your-build-host>

# Enter WSL2
wsl -d Ubuntu

# Build
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=/opt/devkitpro/devkitPPC
export PATH=$DEVKITPPC/bin:$PATH

cd ~/game_dev/moonlight-drift-wii
make

# Output
ls build/moonlight-drift.dol    # ~2MB Wii executable
```

### Running in Dolphin

One command builds, stages and pushes everything to the folder Dolphin
reads as its SD card:

```bash
make dolphin
```

Then in Dolphin open:

```
C:\Dolphin\sdcard\apps\moonlight-drift\boot.dol
```

That is the whole loop. The `.dol` and its assets come from the same synced tree,
so they can never drift apart.

**Why it matters that both are in sync.** Opening a `.dol` does *not* give
Dolphin the sprites or audio — those resolve through the emulated SD card at
`sd:/apps/moonlight-drift/`. Running a current binary against a stale card (or
vice versa) looks exactly like the game being broken.

**Where Dolphin's SD setting lives**, if it ever needs checking:

```
%APPDATA%\Dolphin Emulator\Config\Dolphin.ini   ->   WiiSDCardPath
```

`make dolphin` defaults to `C:\Dolphin\sdcard` to match. Override it if yours
differs:

```bash
make dolphin DOLPHIN_SD=/mnt/c/some/other/sdcard
```

## Onto a real Wii

Two routes, and they do different jobs.

```bash
make card SD=/mnt/e             # install onto an SD card (permanent)
make wii  WIILOAD=tcp:<wii-ip>  # send this build to a running console (temporary)
```

**`make card`** is the one that installs the game. It needs the card's mount
point because a removable drive's letter moves — a card showing as `E:` in
Windows is `/mnt/e` in WSL. Eject it, put it in the console, and the game appears
in the Homebrew Channel under the name in `meta.xml`.

Unlike `make dolphin`, this **merges**: only `boot.dol` and `meta.xml` are
overwritten, so `scores.json` and `settings.json` on the card survive an update.
Wiping is right for a dev loop against an emulator and wrong when it is somebody's
high scores.

**`make wii`** sends the `.dol` over the network and runs it immediately, without
installing anything. It is the fast loop for real hardware — no card, no ejecting,
a couple of seconds. Open the Homebrew Channel, press Home for the netloader
screen, and use the IP address it displays:

```bash
export WIILOAD=tcp:192.168.1.50   # once per shell, then just `make wii`
```

The console has to be sitting on that netloader screen when you send. Nothing is
written to the card, so the game is gone when you quit it.

The target clears the app directory before copying rather than merging, because
asset filenames have changed across versions and orphans left behind are exactly
what makes a folder ambiguous. It no-ops harmlessly anywhere without `/mnt/c`.

If the card is missing or incomplete the game still boots, and a **startup
report** lists which of SD, font, sprites and audio came up — the fastest way to
tell an asset problem from a code problem.

### Deploy to SD Card

```
SD Card/
└── apps/
    └── moonlight-drift/
        ├── boot.dol          # Copy from build/moonlight-drift.dol
        ├── meta.xml          # App metadata for Homebrew Channel
        ├── sprites/          # 48 character textures (128x128 PNG)
        ├── audio/            # 4 PCM16 assets
        ├── scores.json       # High scores (created at runtime)
        └── settings.json     # Character + mute preference (created at runtime)
```

---

## Game States

```
TITLE → READY → PLAYING → GAME OVER → INITIALS → HIGH SCORES → TITLE
                      ↑                                    |
                      └────────────────────────────────────┘
```

| State | Description |
|-------|-------------|
| `STATE_TITLE` | "MOONLIGHT DRIFT" in Press Start 2P font, D-pad character selection |
| `STATE_READY` | "Hold A to thrust, dodge obstacles!" |
| `STATE_PLAYING` | Active gameplay — physics, obstacles, collision, scoring |
| `STATE_GAME_OVER` | Score display, high score check |
| `STATE_INITIALS` | 3-letter arcade-style initials entry (D-pad to cycle) |
| `STATE_HIGH_SCORES` | Top 10 leaderboard |

---

## Controls

| Action | Wiimote Button |
|--------|---------------|
| Thrust (fly up) | A (hold) |
| Open character grid | A (title screen) |
| Move in grid | D-pad |
| Cancel grid | B |
| Start / Confirm | A (menus) |
| Initials: change letter | D-pad Left/Right |
| Initials: next position | D-pad Down |
| Initials: save | B or A |
| Mute / unmute | 1 (title screen) |
| Credits | 2 (title screen) |
| Exit to HBC | HOME |

---

## Character System

All 24 characters from the web game, selected from a 6×4 grid. Hitboxes, offsets
and physics were generated from the browser version's per-character modules and
its shared balance table — `source/characters.c` is generated output, not
hand-written, and the table below is the authoritative copy of those numbers.

| # | Character | Hitbox | Offset | Thrust | Gravity | MaxVel |
|---|-----------|--------|--------|--------|---------|--------|
| 1 | Synth Cat | 42×30 | -1,+18 | -0.6 | 0.36 | 10.5 |
| 2 | Beava | 40×35 | +0,+0 | -0.6 | 0.4 | 10.0 |
| 3 | Vinny Bobarino | 35×35 | +15,+0 | -0.6 | 0.4 | 10.0 |
| 4 | Darius Hodgekins | 40×35 | +0,+0 | -0.6 | 0.4 | 10.0 |
| 5 | Roderick Tron | 40×35 | +0,+0 | -0.6 | 0.4 | 10.0 |
| 6 | Juanito Thompson | 40×35 | +0,+0 | -0.6 | 0.4 | 10.0 |
| 7 | Carl Spatski | 42×42 | -1,-2 | -0.65 | 0.4 | 10.5 |
| 8 | dspum balloon | 30×38 | +5,-3 | -0.5 | 0.3 | 9.0 |
| 9 | Forester's Soul | 40×40 | +0,-2 | -0.62 | 0.38 | 11.0 |
| 10 | Grocery Harrison | 35×30 | +8,+5 | -0.6 | 0.4 | 10.0 |
| 11 | Didgeridoo Man | 45×35 | +0,+0 | -0.7 | 0.35 | 9.5 |
| 12 | Mountain Gnome | 32×32 | +4,+0 | -0.55 | 0.32 | 9.5 |
| 13 | Prince Vince | 27×38 | +7,-3 | -0.65 | 0.38 | 10.5 |
| 14 | Dag Henderson | 40×35 | +0,+0 | -0.6 | 0.4 | 10.0 |
| 15 | BANDANIEL | 38×38 | +1,-2 | -0.6 | 0.4 | 10.0 |
| 16 | PLANTAIN JANE | 38×38 | +1,-2 | -0.6 | 0.4 | 10.0 |
| 17 | Fire Toad | 32×32 | +4,+0 | -0.65 | 0.38 | 10.5 |
| 18 | Backpack Man | 34×36 | +6,-2 | -0.6 | 0.4 | 10.0 |
| 19 | Tollbooth Lady | 40×35 | +0,+0 | -0.6 | 0.4 | 10.0 |
| 20 | SVFP Van | 36×18 | +4,+12 | -0.6 | 0.4 | 10.0 |
| 21 | Tardigrade | 26×25 | +7,+8 | -0.7 | 0.35 | 11.0 |
| 22 | Elektra | 38×38 | +1,+0 | -0.58 | 0.38 | 10.5 |
| 23 | Strawberto | 36×38 | +2,+0 | -0.6 | 0.39 | 10.0 |
| 24 | Carl | 38×38 | +1,+0 | -0.62 | 0.37 | 11.0 |

**Beava** has no entry in the web game's `character-balance.js` (the table keys it
as `gangsta-beaver`, a different and unloaded character), so it falls back to the
web's default physics. **SVFP Van** is displayed under its initialism.

Sprites are 128×128 RGBA PNGs, idle + thrust per character (48 files). Each
character records a `sprite_origin_x/y` — the point inside its canvas that lands
on the player position — because the art sits differently in every export. The
earlier 80×80 set cropped four thrust frames; 128×128 fits the widest character
(115×102) with margin.

## Audio

Four assets converted from the web game's OGG sources to raw PCM16, 48kHz
stereo — the format `magnolia`'s ASND-backed audio module expects:

```bash
ffmpeg -i in.ogg -f s16le -acodec pcm_s16le -ar 48000 -ac 2 out.pcm
```

| File | Length | Size |
|------|--------|------|
| `music.pcm` | 11.1s | 2.0 MB |
| `crash.pcm` | 1.5s | 276 KB |
| `button1.pcm` / `button2.pcm` | 0.2s / 0.8s | 40 KB / 150 KB |

Clips are held in main RAM (~2.4MB). Converting the music to 32kHz mono would
cut it to ~710KB if that ever matters.

The mix matches the browser version's `loadAudio()` — music at 0.3, effects at
0.4–0.5 — set through the engine before anything plays. At the engine default
every voice runs flat out and an eleven-second music loop simply buries the
crash.

| Voice | Web | `audio_set_*_volume()` |
|-------|-----|------------------------|
| Music | 0.30 | 77 |
| Effects | 0.40–0.50 | 115 |

## Playfield

The web build's canvas is **1280×720**, and that is the only coordinate space
the simulation knows. `GAP`, `OBSTACLE_WIDTH`, `OBSTACLE_SPEED`, every character
hitbox — all of them are measurements in it, so physics, collision and scoring
stay identical to `js/`. `source/playfield.h` projects that world onto the
framebuffer at draw time; nothing else converts between the two.

This used to be different, and it mattered more than anything else in the port.
The web constants were run straight onto a 640×480 framebuffer, which is not a
port of the game so much as a different one:

| | Web (1280×720) | 640×480 field | Now |
|---|---|---|---|
| `GAP` as a share of height | 25% | 37.5% | **25%** |
| Player hitbox as a share of height | 4.9% | 7.3% | **4.9%** |
| Obstacles on screen at once | ~4 | ~2 | **4** |

The projection fills the **TV-safe rectangle** vertically and lets the width
follow. That is what keeps the gap and the player at their web proportions, and
it is also a correctness fix: the top and bottom edges of the world are lethal,
and drawing them at the framebuffer edge put the kill line behind the bezel on a
CRT and 48 pixels away from where it actually was on PAL. Both boundary rules
now come from the same `pf_y()` the death check uses.

The horizontal scale is derived from the pixel aspect ratio rather than assumed,
which is what lets one code path cover every output:

```
par   = display_aspect / (fb_width / fb_height)     /* CONF_GetAspectRatio() */
s_y   = safe_h / 720
s_x   = s_y / par
```

| Output | `par` | scale (x, y) | visible world width |
|---|---|---|---|
| NTSC 4:3 (640×480) | 1.000 | 0.588, 0.588 | 957 |
| PAL 4:3 (640×528) | 1.100 | 0.588, 0.647 | 958 |
| 16:9 (any) | 1.333 | 0.441, 0.588 | 1276 — the whole field |

Both 4:3 modes agree with each other, and a widescreen set gets the arcade
version's full field for nothing. On 4:3 the cropped ~957 means obstacles arrive
with three quarters of the web's warning distance; the alternative was
letterboxing away a third of the screen height.

`pf_init()` prints this line at startup, because every geometry complaint about
this game reduces to those numbers:

```
playfield: fb 640x480 safe 564x424+38+28 par 1000/1000 scale 588/1000 x 588/1000 world 957x720
```

**Stars are the one thing drawn at framebuffer scale.** Their positions move
with the world, but the patterns stay 1:1 — a one-pixel star through a 0.588
scale is a sub-pixel rectangle that renders as nothing, and the chunky shapes
are the entire point of that module.

## Obstacle Styles

Three visual styles with matching tapered collision:

| Style | Visual | Min Width |
|-------|--------|-----------|
| 0 — Candy Striped | 6px horizontal stripes | 22px |
| 1 — Faceted Crystal | 8px angular facets | 24px |
| 2 — Rough Crystal | 10px irregular blocks | 20px |

---

## Development Phases

| Phase | Status | Description |
|-------|--------|-------------|
| 1 — Hello Wii | ✅ | Project scaffold, console text output |
| 2 — Core Game | ✅ | Game loop, physics, obstacles, collision, scoring |
| 3 — GRRLIB Renderer | ✅ | Pixel rendering with GRRLIB rectangles |
| 4 — Characters | ✅ | Character system, sprites, selection |
| 5 — UI & Polish | ✅ | Press Start 2P font, title screen, high scores, initials |
| 6 — Audio | ✅ | PCM16 music loop + crash/button SFX, mute toggle |
| 7 — All 24 Characters | ✅ | Full roster + 6×4 selector grid |
| 8 — Arcade Parity | ✅ | World-space playfield, web audio mix, restored copy |

---

## Key Differences: Web vs Wii

| Feature | Web Version | Wii Port |
|---------|------------|----------|
| Rendering | HTML5 Canvas | GRRLIB (GX) |
| Input | Keyboard | Wiimote (WPAD) |
| Audio | Web Audio API (OGG) | libogc ASND (PCM16) |
| Storage | WebSocket + localStorage | SD card (JSON) |
| Play field | 1280×720 canvas | Same 1280×720 world, projected (see Playfield) |
| Framebuffer | n/a | 640×480 NTSC, 640×528 PAL |
| Characters | Canvas `draw()` | Pre-rendered PNG sprites |

---

## Resources

| Resource | URL |
|----------|-----|
| magnolia engine | https://github.com/magmacrunch-media/magnolia |
| Web version | https://magmacrunch.com/arcade/moonlight-drift/ |
| devkitPro | https://devkitpro.org |
| GRRLIB | https://github.com/GRRLIB/GRRLIB |
| WiiBrew | https://wiibrew.org |

## License

Under the [PolyForm Noncommercial License 1.0.0](../LICENSE), like the rest of
the repository — the physics, the obstacle field, the world-to-screen projection
and the host tests are all there to read and build on for any noncommercial
purpose. Commercial use is reserved.

The 24 characters and their sprites, the audio, the game's name and the visual
design are © 2026 magmacrunch media, all rights reserved, and are **not** covered
by that licence. The music, "Moonlight Drift" by C.P. Rutledge, is a magmacrunch
media record label release and is reserved the same way. See [NOTICE](../NOTICE)
for the exact boundary, the third-party components, and the game's lineage.
