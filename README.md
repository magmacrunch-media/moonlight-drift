# Moonlight Drift — Nintendo Wii Port

**Project:** Port [Moonlight Drift](https://magmacrunch.com/arcade/moonlight-drift/) (Jetman-style arcade game) to a homebrewed Nintendo Wii
**Engine:** [magnolia](https://github.com/magmacrunchmedia/magnolia) `>= v0.2.0` — shared Wii game engine
**Status:** Phase 5 complete — styled title screen, high scores, initials entry, character selection with sprites

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
│   └── ui.c/.h                  # Title, ready, game over, initials, high scores
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
| Character select | D-pad Left/Right (title screen) |
| Start / Confirm | A (menus) |
| Initials: change letter | D-pad Left/Right |
| Initials: next position | D-pad Down |
| Initials: save | B or A |
| Exit to HBC | HOME |

---

## Character System

6 playable characters, each with unique hitbox and physics:

| Character | Hitbox | Thrust | Gravity | MaxVel |
|-----------|--------|--------|---------|--------|
| Tardigrade | 26×25 | -0.7 | 0.35 | 11 |
| Dag Henderson | 40×35 | -0.6 | 0.40 | 10 |
| Cat Synth | 42×30 | -0.6 | 0.36 | 10.5 |
| Vinny Bobarino | 35×35 | -0.6 | 0.40 | 10 |
| Carl Spatski | 42×42 | -0.65 | 0.40 | 10.5 |
| Gangsta Beaver | 40×35 | -0.6 | 0.40 | 10 |

Characters are pre-rendered as 80×80 PNG sprites (idle + thrust frames), exported from the web game's canvas.

---

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
| 4 — Characters | ✅ | 6 characters with sprites, D-pad selection |
| 5 — UI & Polish | ✅ | Press Start 2P font, title screen, high scores, initials |
| 6 — Audio | ⬜ | Sound effects + background music |
| 7 — All 24 Characters | ⬜ | Export remaining character sprites |

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
