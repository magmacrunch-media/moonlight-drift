# Moonlight Drift — Nintendo Wii Port

**Project**: Port Moonlight Drift (Jetman-style arcade game) to a homebrewed Nintendo Wii  
**Source**: `/Users/jakemccoy/Documents/website/arcade/moonlight-drift/`  
**Timeline**: Few weeks, iterative development  
**Started**: August 16, 2026  

---

## Overview

Moonlight Drift is a side-scrolling obstacle avoidance game built with vanilla JavaScript and HTML5 Canvas. The player holds a button to thrust upward, releases to fall, and navigates through gaps in procedurally generated obstacles. It features 24 unique characters with different physics, dynamic color themes, and a high score system.

The goal is to compile a native Wii homebrew version using devkitPPC (C compiler), libogc (Wii hardware library), and GRRLIB (2D graphics library).

---

## Prerequisites

### 1. Homebrew the Wii
- Use **LetterBomb** exploit (requires SD card, Wii on System Menu 4.3)
- Go to https://please.hackmii.com/, enter Wii MAC address (Wii Settings > Internet > Console Information)
- Download LetterBomb ZIP, copy to SD card root
- Insert SD card, open Wii Message Board, click red letter with bomb icon
- Install **Homebrew Channel** and **BootMii** (for brick backup)
- Create **NAND backup** via BootMii

### 2. Install devkitPPC on MacBook
```bash
# Install devkitPro pacman (macOS installer from https://devkitpro.org/wiki/Installing)
# Then install Wii development packages:
dkp-pacman -S wii-dev

# Set environment variables (add to ~/.zshrc):
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=/opt/devkitpro/devkitPPC
export PATH=$DEVKITPPC/bin:$PATH

# Verify:
ls /opt/devkitpro/devkitPPC/bin/powerpc-eabi-gcc
```

### 3. Install GRRLIB
```bash
# Install dependencies
dkp-pacman -S --needed libfat-ogc ppc-libpng ppc-freetype ppc-libjpeg-turbo

# Clone and build GRRLIB
git clone https://github.com/GRRLIB/GRRLIB.git
cd GRRLIB
make -C GRRLIB clean all install
```

---

## Wii Hardware Specs

| Component | Specification |
|-----------|--------------|
| CPU | IBM "Broadway" (PowerPC 750CL) @ 729 MHz |
| GPU | ATI "Hollywood" @ 243 MHz |
| RAM | 24 MB 1T-SRAM (main) + 64 MB GDDR3 (video) |
| Display | 640x480 (NTSC 480p) |
| Controllers | Wiimote (motion, IR, buttons), Nunchuk, Classic Controller, GameCube |
| Storage | SD/SDHC card (for homebrew) |

---

## Controls

| Action | Wiimote Button |
|--------|---------------|
| Thrust (fly up) | A button (hold) |
| Character selection | D-pad (in menu) |
| Confirm/Start | A button (in menu) |
| Exit to Homebrew Channel | HOME button |

---

## Development Phases

### Phase 1 — Hello Wii (Day 1)
**Goal**: Compile and run a minimal program on the Wii

- [ ] Set up devkitPPC environment
- [ ] Create minimal `main.c` with video init + "Hello Wii" text
- [ ] Build with `make` → produces `boot.dol`
- [ ] Deploy to SD card: `SD:/apps/moonlight-drift/boot.dol`
- [ ] Add `meta.xml` and `icon.png` for Homebrew Channel
- [ ] Test on real Wii via Homebrew Channel
- [ ] Verify Wiimote input works (`WPAD_ScanPads()`)
- [ ] Confirm 640x480 video output

**Minimal main.c template**:
```c
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdlib.h>
#include <stdio.h>

static void *framebuffer = NULL;

int main(int argc, char **argv) {
    VIDEO_Init();
    WPAD_Init();
    
    struct conf_pad_s preferred = VIDEO_GetPreferredMode(NULL);
    framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(preferred));
    console_init(framebuffer, 20, 20, preferred->fbWidth, preferred->xfbHeight,
                 preferred->fbWidth * VI_DISPLAY_PIX_SZ);
    
    VIDEO_Configure(preferred);
    VIDEO_SetNextFramebuffer(framebuffer);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    
    printf("Hello Wii Homebrew World!\n");
    printf("Press HOME to exit.\n");
    
    while (1) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        if (pressed & WPAD_BUTTON_HOME) break;
        VIDEO_WaitVSync();
    }
    return 0;
}
```

### Phase 2 — Core Game (Week 1)
**Goal**: Playable game with basic graphics

Port the game logic to C, one module at a time:

| Module | Source File | Porting Difficulty | Notes |
|--------|------------|-------------------|-------|
| Constants | `config.js` | Trivial | `#define` values |
| Physics | `player.js` | Trivial | 3 floats, clamp, comparison |
| Input | `input.js` | Trivial | Single boolean from WPAD |
| Obstacles (gen/move/collide) | `obstacles.js` | Easy | Random + struct + sin/cos math |
| Game loop | `main.js` | Easy | `update(); draw(); VSync();` |
| Score counting | `scoring.js` | Easy | Integer increment |
| Obstacle rendering | `obstacles.js` | Medium | Translate fillRect loops to GRRLIB |

**Start with**:
- 1 default character (Dag Henderson — simplest drawing)
- Simple colored rectangle obstacles (skip 3 fancy styles)
- Basic score display with bitmap font
- Test on Dolphin emulator + real Wii

**Key C structures**:
```c
// Player state
typedef struct {
    float x, y;
    float velocity;
    float thrust;
    float gravity;
    float maxVelocity;
    int width, height;
    int offsetX, offsetY;
} Player;

// Obstacle state
typedef struct {
    float x;
    int topHeight;
    int bottomY;
    int passed;
    int styleIndex;
    int seed;
    float roughness;
    int milestone;
    // Theme colors
    u8 primary_r, primary_g, primary_b;
    u8 secondary_r, secondary_g, secondary_b;
    u8 accent_r, accent_g, accent_b;
} Obstacle;
```

### Phase 3 — Rendering (Week 2)
**Goal**: Full visual fidelity

- [ ] Implement 3 obstacle styles (candy striped, faceted crystal, rough crystal)
- [ ] Port star field (60 blinking pixel stars)
- [ ] Add milestone markers (every 10th obstacle)
- [ ] Implement color theme generation (HSL → RGB)
- [ ] Convert OGG audio to PCM16 for Wii playback
- [ ] Add crash sound effect
- [ ] Add background music loop

**Obstacle rendering approach**:
- Each obstacle style uses loops of `fillRect()` calls
- Translate to GRRLIB `GRRLIB_Rectangle()` or batch to GX
- Pre-compute colors per-frame from theme struct

### Phase 4 — Characters (Week 3)
**Goal**: All 24 characters playable

**Approach**: Pre-render characters as PNG sprite sheets

For each character:
1. Run the web version in a browser
2. Use canvas `toDataURL()` or a headless browser to capture each character's drawing at known positions
3. Export as PNG sprite sheets (idle frame + thrust frame)
4. Load as GRRLIB textures at game start

**Character priority** (start with these 6):
1. Dag Henderson — simplest, good default
2. Gangsta Beaver — popular, simple shapes
3. Cat Synth — wide but simple
4. Tardigrade — smallest hitbox, fan favorite
5. Carl Spatski — largest, visually distinct
6. Vinny Bobarino — medium, recognizable

**Character selector**:
- Canvas-drawn grid menu
- D-pad to navigate, A to select
- Show character name + hitbox size

**Character balance table** (already defined in `character-balance.js`):
```c
typedef struct {
    float thrust;
    float gravity;
    float maxVelocity;
} CharPhysics;

static const CharPhysics char_physics[] = {
    [-1] = { -0.6, 0.4, 10 },   // default
    [TARDIGRADE] = { -0.7, 0.35, 11 },
    [CARL_SPATSKI] = { -0.65, 0.4, 10.5 },
    // ... all 24 characters
};
```

### Phase 5 — UI & Polish (Week 4)
**Goal**: Complete, polished experience

- [ ] Title screen with ASCII art ("MOONLIGHT DRIFT")
- [ ] Ready screen ("PRESS A TO START")
- [ ] Game over screen with score display
- [ ] High score system (save/load from SD card as JSON)
- [ ] Initials entry (D-pad left/right to pick letter, down to confirm)
- [ ] Achievement messages (gold/silver/bronze/cyan/green tiers)
- [ ] Credits screen
- [ ] Character selection persistence (save to SD card)
- [ ] Mute toggle (HOME or specific button combo)

**High score storage**:
```c
// SD card path
#define SCORES_PATH "sd:/apps/moonlight-drift/scores.json"

// Score structure
typedef struct {
    char initials[4];
    int score;
} ScoreEntry;

#define MAX_SCORES 10
static ScoreEntry scores[MAX_SCORES];
```

---

## Project Structure

```
moonlight-drift-wii/
├── Makefile                  # Build configuration
├── source/
│   ├── main.c               # Entry point, game loop, state machine
│   ├── config.h             # Constants (GAP, OBSTACLE_WIDTH, etc.)
│   ├── player.c/.h          # Physics (thrust, gravity, velocity)
│   ├── input.c/.h           # Wiimote input handling
│   ├── obstacles.c/.h       # Obstacle generation, movement, collision, rendering
│   ├── renderer.c/.h        # Draw orchestration
│   ├── stars.c/.h           # Star field rendering
│   ├── scoring.c/.h         # Score tracking, SD card persistence
│   ├── characters.c/.h      # Character registry, physics table
│   ├── audio.c/.h           # Music + SFX playback
│   └── ui.c/.h              # Menus, title screen, game over, initials entry
├── sprites/                  # Pre-rendered character PNGs
│   ├── dag-henderson.png
│   ├── gangsta-beaver.png
│   └── ...
├── audio/                    # Converted PCM audio files
│   ├── music.pcm
│   ├── crash.pcm
│   ├── button1.pcm
│   └── button2.pcm
├── font/                     # Bitmap font (Press Start 2P)
│   └── press-start-2p.bfnt
└── meta.xml                  # Homebrew Channel metadata
```

---

## Makefile Template

```makefile
#---------------------------------------------------------------------------------
.SUFFIXES:
.SECONDARY:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
TARGET      := moonlight-drift
BUILD       := build
SOURCES     := source
DATA        :=
TEXTURES    := sprites audio font
INCLUDES    :=

CFLAGS      = -g -O2 -Wall $(MACHDEP) $(INCLUDE)
CXXFLAGS    = $(CFLAGS)
LDFLAGS     = -g $(MACHDEP) -Wl,-Map,$(notdir $@).map

LIBS        := -lgrrlib -lpngu `$(PREFIX)pkg-config freetype2 libpng libjpeg --libs` -lwiiuse -lbte -logc -lm
LIBDIRS     := $(PORTLIBS)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT   := $(CURDIR)/$(TARGET)
export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR  := $(CURDIR)/$(BUILD)

CFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

ifeq ($(strip $(CPPFILES)),)
    export LD  := $(CC)
else
    export LD  := $(CXX)
endif

export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD) -I$(LIBOGC_INC)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib) -L$(LIBOGC_LIB)

.PHONY: $(BUILD) clean

$(BUILD):
    @[ -d $@ ] || mkdir -p $@
    @$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
    @echo clean ...
    @rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol

run:
    wiiload $(OUTPUT).dol

else

$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

endif
```

---

## Audio Conversion

Convert OGG files to PCM16 for Wii playback:

```bash
# Convert each OGG to raw PCM16 (signed 16-bit, little-endian, stereo, 48000Hz)
ffmpeg -i moonlightdrift-gameloop.ogg -f s16le -acodec pcm_s16le -ar 48000 -ac 2 audio/music.pcm
ffmpeg -i crashsound.ogg -f s16le -acodec pcm_s16le -ar 48000 -ac 2 audio/crash.pcm
ffmpeg -i buttonsound1.ogg -f s16le -acodec pcm_s16le -ar 48000 -ac 2 audio/button1.pcm
ffmpeg -i buttonsound2.ogg -f s16le -acodec pcm_s16le -ar 48000 -ac 2 audio/button2.pcm
```

---

## Character Sprite Generation

To pre-render characters as sprites:

1. Open the web version of Moonlight Drift in Chrome
2. Open DevTools Console
3. For each character, draw it to an offscreen canvas and export:

```javascript
// Example: export Dag Henderson
const canvas = document.createElement('canvas');
canvas.width = 80;
canvas.height = 80;
const ctx = canvas.getContext('2d');
ctx.fillStyle = '#0a0e27';
ctx.fillRect(0, 0, 80, 80);

// Call the character's draw function
window.characters['dag-henderson'].draw(ctx, 40, 40, false, true, 0);

// Export
const link = document.createElement('a');
link.download = 'dag-henderson-idle.png';
link.href = canvas.toDataURL('image/png');
link.click();

// Repeat with thrustActive = true for thrust frame
```

4. Place exported PNGs in `sprites/` directory
5. Load with `GRRLIB_LoadTextureFromFile("sd:/apps/moonlight-drift/sprites/dag-henderson-idle.png")`

---

## Deployment

### SD Card Structure
```
SD Card/
└── apps/
    └── moonlight-drift/
        ├── boot.dol          # Compiled Wii executable
        ├── meta.xml          # App metadata
        ├── icon.png          # 128x48 icon for Homebrew Channel
        ├── sprites/          # Character textures
        ├── audio/            # PCM audio files
        └── scores.json       # High scores (created at runtime)
```

### meta.xml
```xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<app version="1">
  <name>Moonlight Drift</name>
  <coder>magmacrunch media</coder>
  <version>1.0</version>
  <release_date>20260901000000</release_date>
  <short_description>Jetman-style obstacle avoidance</short_description>
  <long_description>Coast through space. Dodge obstacles. Ride the moon. 24 playable characters with unique physics.</long_description>
</app>
```

### Network Deploy (wiiload)
```bash
# Ensure Wii is on same network and Homebrew Channel is running
wiiload moonlight-drift.dol
```

---

## Source File Reference

| Priority | Source (JS) | Lines | Porting Notes |
|----------|------------|-------|---------------|
| 1 | `config.js` | 30 | Constants → `#define` |
| 2 | `player.js` | 50 | Physics → 3 floats + clamp |
| 3 | `input.js` | 130 | Single boolean from WPAD |
| 4 | `obstacles.js` | 922 | Gen/collision easy, rendering medium |
| 5 | `main.js` | 400 | Game loop + state machine + UI |
| 6 | `renderer.js` | 50 | Draw orchestration |
| 7 | `stars.js` | 100 | 60 pixel stars with blink |
| 8 | `scoring.js` | 352 | Score + SD card + initials entry |
| 9 | `character-balance.js` | 80 | Physics lookup table |
| 10 | `characters/*.js` | ~2500 total | Pre-render to sprites |
| 11 | `adenosine-audio.js` | 258 | Web Audio → libogc audio |

---

## Key Differences: Web vs Wii

| Feature | Web Version | Wii Port |
|---------|------------|----------|
| Rendering | HTML5 Canvas 2D | GRRLIB (GX-based) |
| Drawing | `fillRect()`, `arc()`, `clip()` | `GRRLIB_Rectangle()`, sprites |
| Input | Keyboard/mouse/touch | Wiimote (WPAD) |
| Audio | Web Audio API (OGG) | libogc audio (PCM16) |
| Storage | WebSocket + localStorage | SD card (JSON files) |
| Frame rate | Variable (requestAnimationFrame) | Fixed (VIDEO_WaitVSync) |
| Resolution | 1280x720 (scalable) | 640x480 (fixed) |
| Font | Google Fonts (Press Start 2P) | Bitmap font (embedded) |
| UI | HTML/CSS modals | Canvas-drawn menus |

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

---

## Notes

- The game is frame-rate dependent (obstacles spawn every 120 frames). On Wii at 60fps, this matches the web version. If VSync is different, adjust `OBSTACLE_SPAWN_INTERVAL`.
- GRRLIB doesn't support `ctx.clip()` or `ctx.createRadialGradient()`. Characters using these effects (Carl, Forester's Soul) need simplified sprite versions.
- The Wii's 24MB RAM is tight. Keep sprite textures small (64x64 or 80x80 max per character).
- Test on Dolphin emulator first for faster iteration, then verify on real hardware.
