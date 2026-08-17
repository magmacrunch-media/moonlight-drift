# Moonlight Drift — Nintendo Wii Port

**Project:** Port [Moonlight Drift](https://magmacrunch.com/arcade/moonlight-drift/) (Jetman-style arcade game) to a homebrewed Nintendo Wii
**Source repo:** [magmacrunchmedia/magmacrunch.com](https://github.com/magmacrunchmedia/magmacrunch.com) — `arcade/moonlight-drift/`
**Status:** Phase 1 complete — builds and links successfully, ready for GRRLIB renderer

---

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C (C99) |
| Compiler | devkitPPC (PowerPC cross-compiler) |
| Hardware lib | libogc (Wii video, input, memory, filesystem) |
| Graphics | GRRLIB (2D rendering via GX) — planned |
| Audio | libogc ASND/aesnd — planned |
| Storage | FatFS (SD card read/write via libfat-ogc) |
| Build system | GNU Make + devkitPPC wii_rules |

---

## Project Structure

```
moonlight-drift-wii/
├── Makefile                  # devkitPPC build config
├── meta.xml                  # Homebrew Channel metadata
├── source/
│   ├── main.c               # Entry point, game loop, state machine
│   ├── config.h             # All game constants (#define)
│   ├── player.c/.h          # Player physics (thrust, gravity, velocity, boundary collision)
│   ├── input.c/.h           # Wiimote input via WPAD
│   ├── obstacles.c/.h       # Obstacle generation, movement, tapered collision, themes
│   ├── renderer.c/.h        # Draw orchestration (currently console text, GRRLIB planned)
│   ├── stars.c/.h           # Star field (60 blinking/pulsing pixel stars)
│   ├── scoring.c/.h         # Score tracking, SD card JSON persistence
│   └── theme.c/.h           # HSL→RGB color theme generation
├── sprites/                  # Pre-rendered character PNGs (empty — Phase 4)
├── audio/                    # PCM16 audio files (empty — Phase 5)
└── font/                     # Bitmap font (empty — Phase 5)
```

---

## Building

### Prerequisites

- devkitPro installed at `/opt/devkitpro/` (includes devkitPPC + libogc)
- SSH access to MC1 (Windows PC running WSL2 Ubuntu)

### Build Commands

```bash
# SSH into MC1
ssh magma@100.75.220.87

# Enter WSL2
wsl -d Ubuntu -u root

# Build
cd ~/game_dev/moonlight-drift-wii
make

# Clean
make clean

# Output
ls build/moonlight-drift.dol    # ~336KB Wii executable
```

### Deploy to SD Card

```
SD Card/
└── apps/
    └── moonlight-drift/
        ├── boot.dol          # Copy from build/moonlight-drift.dol
        ├── meta.xml          # App metadata for Homebrew Channel
        ├── sprites/          # Character textures (Phase 4)
        ├── audio/            # PCM audio files (Phase 5)
        └── scores.json       # High scores (created at runtime)
```

---

## Wii Hardware Reference

| Component | Specification |
|-----------|--------------|
| CPU | IBM "Broadway" (PowerPC 750CL) @ 729 MHz |
| GPU | ATI "Hollywood" @ 243 MHz |
| RAM | 24 MB 1T-SRAM (main) + 64 MB GDDR3 (video) |
| Display | 640x480 (NTSC 480p) |
| Controllers | Wiimote (motion, IR, buttons), Nunchuk, Classic Controller, GameCube |
| Storage | SD/SDHC card (for homebrew) |

---

## Game Architecture

### State Machine

```
[LOADING] → [TITLE SCREEN] → [READY SCREEN] → [GAME RUNNING] → [GAME OVER]
                                      ↑                            |
                                      └────────────────────────────┘
                                                      (restart)
```

Implemented as a `GameState` enum in `main.c`:
- `STATE_TITLE` — "MOONLIGHT DRIFT" title, press A to start
- `STATE_READY` — Instructions, hold A to thrust
- `STATE_PLAYING` — Active gameplay with physics, obstacles, collision
- `STATE_GAME_OVER` — Final score, press A to retry

### Game Loop

Runs at 60fps locked via `VIDEO_WaitVSync()`:

```
1. Input: WPAD_ScanPads() → thrustActive boolean
2. Physics: player_update() — thrust/gravity, velocity clamp, boundary check
3. Obstacles: spawn every 120 frames (2 sec at 60fps), move left, remove off-screen
4. Collision: tapered obstacle collision (3 styles), boundary collision
5. Scoring: increment when player passes obstacle right edge
6. Render: clear, stars, obstacles, player, score display
7. VSync: wait for next frame
```

### Module Breakdown

| Module | JS Source | C Module | Purpose |
|--------|----------|----------|---------|
| Constants | `config.js` | `config.h` | 15 `#define` values (GAP, OBSTACLE_SPEED, etc.) |
| Physics | `player.js` | `player.c/.h` | Player struct, thrust/gravity, velocity clamp, boundary collision |
| Input | `input.js` | `input.c/.h` | `WPAD_ScanPads()` → `thrustActive` boolean |
| Obstacles | `obstacles.js` | `obstacles.c/.h` | Generation, movement, tapered collision (3 styles), theme snapshots |
| Rendering | `renderer.js` | `renderer.c/.h` | Draw orchestration (console stub → GRRLIB) |
| Stars | `stars.js` | `stars.c/.h` | 60 stars with blink/pulse animation |
| Scoring | `scoring.js` | `scoring.c/.h` | Score counter, SD card JSON read/write |
| Themes | (inline in obstacles.js) | `theme.c/.h` | HSL→RGB, random palette generation |
| Main | `main.js` | `main.c` | Entry point, game loop, state machine |

---

## Key Porting Decisions (Web → Wii)

| Feature | Web Version | Wii Port |
|---------|------------|----------|
| Rendering | HTML5 Canvas 2D (`fillRect`, `arc`, `clip`) | GRRLIB (GX-based rectangles, sprites) |
| Input | Keyboard/mouse/touch | Wiimote A button (WPAD) |
| Audio | Web Audio API (OGG) | libogc audio (PCM16) |
| Storage | WebSocket + localStorage | SD card (FatFS, JSON files) |
| Frame rate | Variable (`requestAnimationFrame`) | Fixed 60fps (`VIDEO_WaitVSync`) |
| Resolution | 1280x720 (scalable) | 640x480 (fixed NTSC) |
| Font | Google Fonts (Press Start 2P) | Bitmap font or GRRLIB text |
| Characters | JS `draw()` with Canvas API | Pre-rendered PNG sprites (64x64) |
| Obstacle rendering | `fillRect()` loops with stripe/facet/block patterns | GRRLIB rectangles or GX batch calls |
| Collision | Tapered width + sine offset (3 style-specific functions) | Same math in C (`sinf()` calls) |

---

## Character System

### Balance Table (from JS `character-balance.js`)

All 24 characters have unique physics:

| Character | Hitbox | Thrust | Gravity | MaxVel | Area |
|-----------|--------|--------|---------|--------|------|
| tardigrade | 26x25 | -0.7 | 0.35 | 11 | 650 |
| prince-vince | 27x38 | -0.65 | 0.38 | 10.5 | 1026 |
| fire-toad | 32x32 | -0.65 | 0.38 | 10.5 | 1024 |
| vinny-bobarino | 35x35 | -0.6 | 0.4 | 10 | 1225 |
| cat-synth | 42x30 | -0.6 | 0.36 | 10.5 | 1260 |
| dag-henderson | 40x35 | -0.6 | 0.4 | 10 | 1400 |
| carl-spatski | 42x42 | -0.65 | 0.4 | 10.5 | 1764 |

### Character Sprite Strategy

Since the JS drawing code uses `arc()`, `ellipse()`, `clip()`, and `quadraticCurveTo()` — none of which GRRLIB supports — characters will be **pre-rendered as PNG sprite sheets**:

1. Run the web game in Chrome
2. Use DevTools canvas `toDataURL()` to export each character at known positions
3. Export idle frame + thrust frame as 64x64 or 80x80 PNGs
4. Load with `GRRLIB_LoadTextureFromFile()` on Wii

**Priority characters (Phase 4):**
1. Dag Henderson — simplest, good default
2. Gangsta Beaver — popular, simple shapes
3. Cat Synth — wide but simple
4. Tardigrade — smallest hitbox, fan favorite
5. Carl Spatski — largest, visually distinct
6. Vinny Bobarino — medium, recognizable

---

## Obstacle Styles

Three visual styles, each with matching tapered collision:

| Style | Visual | Collision Taper | Min Width |
|-------|--------|----------------|-----------|
| 0 — Candy Striped | 6px horizontal stripes, alternating colors | `width = max(22, 60 * (1 - progress * 0.65))` | 22px |
| 1 — Faceted Crystal | 8px angular facets with light detection | `width = max(24, 60 * (1 - progress * 0.6))` | 24px |
| 2 — Rough Crystal | 10px irregular blocks with protrusions | `width = max(20, 60 * (1 - progress * 0.65))` | 20px |

Each uses sine-wave offsets for horizontal wobble:
- Style 0: `sin(seed*100 + yPos*0.1) * 6 * asymmetry`
- Style 1: `sin(seed*50 + yPos*0.2) * 4`
- Style 2: `sin(seed*200 + yPos*0.5) * 8 * roughness`

---

## Controls

| Action | Wiimote Button |
|--------|---------------|
| Thrust (fly up) | A button (hold) |
| Start game | A button (in menus) |
| Exit to HBC | HOME button |

---

## Development Phases

| Phase | Status | Description |
|-------|--------|-------------|
| 1 — Hello Wii | ✅ Done | Project scaffold, builds successfully, console text output |
| 2 — Core Game | ✅ Done | Game loop, physics, obstacles, collision, scoring, state machine |
| 3 — GRRLIB Renderer | ⬜ Next | Replace console text with GRRLIB pixel rendering |
| 4 — Characters | ⬜ | Pre-render 6 priority characters as PNG sprites |
| 5 — UI & Polish | ⬜ | Title screen, game over, high scores, audio, credits |
| 6 — All 24 Characters | ⬜ | Export and load remaining character sprites |

---

## Current Status

**Phase 1 complete.** The game compiles and links successfully (`moonlight-drift.dol`, ~336KB). All core modules are implemented:

- Game loop runs at 60fps with 4-state machine
- Player physics match the JS version (thrust/gravity/velocity)
- Obstacle generation, movement, and tapered collision detection ported
- 3 obstacle styles with style-specific collision math
- Color theme generation (HSL→RGB)
- Star field with blink/pulse
- Score tracking with SD card JSON persistence
- Wiimote input (A button for thrust, HOME to exit)

**Currently uses console text for rendering** (ANSI escape codes to the Wii's text console). Next step is replacing this with GRRLIB for actual pixel rendering.

---

## Resources

| Resource | URL |
|----------|-----|
| devkitPro | https://devkitpro.org |
| libogc | https://github.com/devkitPro/libogc |
| GRRLIB | https://github.com/GRRLIB/GRRLIB |
| WiiBrew Wiki | https://wiibrew.org |
| LetterBomb | https://please.hackmii.com/ |
| devkitPro Forum | https://forums.devkitpro.org |
| Wii examples | https://github.com/devkitPro/wii-examples |
| Source game | https://magmacrunch.com/arcade/moonlight-drift/ |
