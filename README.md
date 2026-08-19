# Moonlight Drift — Nintendo Wii Port

**Project:** Port [Moonlight Drift](https://magmacrunch.com/arcade/moonlight-drift/) (Jetman-style arcade game) to a homebrewed Nintendo Wii
**Engine:** [magnolia](https://github.com/magmacrunchmedia/magnolia) `>= v0.2.0` — shared Wii game engine
**Status:** Phase 7 — all 24 characters, 6×4 selector grid, TV-safe UI, high scores

---

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C (C99) |
| Compiler | devkitPPC (PowerPC cross-compiler) |
| Engine | [magnolia](https://github.com/magmacrunchmedia/magnolia) |
| Graphics | GRRLIB (2D rendering via GX) |
| Font | Press Start 2P (embedded TTF) |
| Storage | SD card (FatFS via libfat-ogc) |
| Build system | GNU Make + devkitPPC wii_rules |

---

## Project Structure

```
moonlight-drift-wii/
├── Makefile                     # Game build config, references ../magnolia
├── meta.xml                     # Homebrew Channel metadata
├── Moonlight-Drift-Wii.md       # Full design document
├── source/
│   ├── main.c                   # Entry point; drives magnolia's GameStateMachine
│   ├── config.h                 # Game constants, APP_NAME, overscan
│   ├── player.c/.h              # Jetman physics (thrust, gravity, boundary)
│   ├── stars.c/.h               # Blinking starfield
│   ├── characters.c/.h          # Character roster, hitboxes, sprite origins
│   ├── obstacles.c/.h           # Cave obstacle system (3 styles)
│   ├── obstacle_renderer.c/.h   # Obstacle drawing + milestone markers
│   ├── game_render.c/.h         # Player/starfield/score/kill-line drawing
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

**Engine code** (core bring-up, rendering, sprites, input, audio, scoring, game state, theme, UI utils, font) lives in [magnolia](https://github.com/magmacrunchmedia/magnolia) and is compiled via `../magnolia` in the Makefile.

Player physics, the starfield and the character roster are **not** in the engine —
they encode this game's decisions, so they live here. See magnolia's README for
the rule on what belongs where.

---

## Building

### Prerequisites

- devkitPro installed at `/opt/devkitpro/`
- magnolia `>= v0.2.0` cloned at `../magnolia` relative to this project.
  There is no version pinning between the two repos yet, so if the engine has
  moved on and the game stops building, check out the matching engine tag:
  `git -C ../magnolia checkout v0.2.0`
- SSH access to MC1 (Windows PC running WSL2 Ubuntu)

### Build Commands

```bash
# SSH into MC1
ssh magma@100.75.220.87

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

### Deploy to SD Card

```
SD Card/
└── apps/
    └── moonlight-drift/
        ├── boot.dol          # Copy from build/moonlight-drift.dol
        ├── meta.xml          # App metadata for Homebrew Channel
        └── sprites/          # Character textures
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
| Exit to HBC | HOME |

---

## Character System

All 24 characters from the web game, selected from a 6×4 grid. Hitboxes, offsets
and physics are generated from `js/characters/*.js` and `js/character-balance.js`
in the source game — `source/characters.c` is generated output, not hand-written.

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
| 6 — Audio | ⬜ | Sound effects + background music |
| 7 — All 24 Characters | ✅ | Full roster + 6×4 selector grid |

---

## Key Differences: Web vs Wii

| Feature | Web Version | Wii Port |
|---------|------------|----------|
| Rendering | HTML5 Canvas | GRRLIB (GX) |
| Input | Keyboard | Wiimote (WPAD) |
| Audio | Web Audio API (OGG) | libogc (PCM16) — planned |
| Storage | WebSocket + localStorage | SD card (JSON) |
| Resolution | 1280×720 | 640×480 |
| Characters | Canvas `draw()` | Pre-rendered PNG sprites |

---

## Resources

| Resource | URL |
|----------|-----|
| magnolia engine | https://github.com/magmacrunchmedia/magnolia |
| Web version | https://magmacrunch.com/arcade/moonlight-drift/ |
| devkitPro | https://devkitpro.org |
| GRRLIB | https://github.com/GRRLIB/GRRLIB |
| WiiBrew | https://wiibrew.org |
