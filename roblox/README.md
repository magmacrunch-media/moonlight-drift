# Moonlight Drift for Roblox

The fourth version of the game, in Luau. You hold to thrust, let go to fall, and
dodge the cave. It keeps the rules of the other three frame for frame, and it
draws in the GUI as a 2D arcade game. Nobody gets an avatar.

## Layout

| Path | Becomes | What it is |
|------|---------|------------|
| `src/shared/` | `ReplicatedStorage.MoonlightDrift` | The rules: `Config`, `Characters`, `Theme`, `Pilot`, `Obstacles`, `Run`. No Roblox services, so they also run under Lune. |
| `src/client/` | `StarterPlayerScripts.MoonlightDrift` | `Main` (state machine, input, sound, fixed-step loop), `Playfield` (renderer), `Screens` (menus), `Assets` (uploaded asset ids). |
| `src/server/` | `ServerScriptService.MoonlightDrift` | `Scores`: the high-score board and the checks on submitted scores. |
| `tests/run.luau` | nothing | Host tests for `src/shared`, run with Lune. |
| `tools/make_atlas.py` | `assets/pilots.png` | Packs the 48 sprites in `wii/sprites/` into one sheet to upload. |
| `default.project.json` | the place | The Rojo project. It also declares the three remotes and sets `Players.CharacterAutoLoads = false`. |

## Where the rules come from

As with the terminal port, the rules come from `wii/source/`, where they are
already separate from any renderer. Each shared module names the C file it
mirrors: `Pilot` is `player.c`, `Obstacles` is `obstacles.c`, `Theme` is
magnolia's `theme.c`, and `Run.step` is `update_playing()` in `main.c`. The
roster's hitboxes and physics are copied from `characters.c`. The glyphs and
accent colours come from the terminal port.

The world is the source game's 1280x720 at 60 frames a second. The client
runs `Run.step` on a fixed 1/60 s step and letterboxes the world into any
screen at 16:9.

The obstacles are drawn from `Obstacles.profileTop` and `profileBottom`, which
are also the functions collision uses. So the cave you see is the cave that
hits you.

A gameplay change still starts in `web/js/`, as `../AGENTS.md` says. Carry it
into `src/shared/` along with `wii/source/` and `tui/`.

## Building

The toolchain is pinned in `rokit.toml`:

```bash
rokit install
```

Serve it live into Studio with the Rojo plugin:

```bash
rojo serve
```

Or build a place file to open or publish:

```bash
rojo build -o moonlight-drift.rbxl
```

Run the tests from this folder:

```bash
lune run tests/run
```

Type-check `src/` against Roblox's API with
[luau-lsp](https://github.com/JohnnyMorganz/luau-lsp). This is the only check
short of Studio that notices a misspelt property or enum. It needs a sourcemap
and Roblox's type definitions (`scripts/globalTypes.d.luau` in that repo):

```bash
rojo sourcemap -o sourcemap.json
luau-lsp analyze --platform=roblox --definitions=globalTypes.d.luau --sourcemap=sourcemap.json src
```

CI's `roblox` job runs all three on every push: the tests, `rojo build` and
the type check.

## High scores

Each run is simulated on the player's own client, so the server never sees the
flight. The server does see the clock. `StartRun` has to come before
`SubmitRun`, and a score is refused if the run was too short to have earned it.
The limit is `Run.maxPlausibleScore`: the n-th column cannot count before frame
`120n + 420`, plus 5% and one second of slack. This does not make cheating
impossible, but a cheated score can be no higher than an honest run of the same
length would have scored.

Each player's best is kept in the OrderedDataStore `MoonlightDriftBest_v1`,
keyed by UserId, and it also shows as `Best` in the player list. Bump the suffix
to wipe the board. In Studio, DataStores only work once **Game Settings >
Security > Enable Studio Access to API Services** is on. Until then the board
keeps scores for the session only and labels them "(this server only)".

## Art and sound

Roblox serves images and audio only by asset id, and an id only exists once
somebody uploads the file. The ids go in `src/client/Assets.luau`. Any id left
empty is skipped: the pilots fall back to their accent colour and glyph, and
that sound stays silent.

| Id | Upload |
|----|--------|
| `pilotSheet` | `roblox/assets/pilots.png` |
| `music` | `web/audio/moonlightdrift-gameloop.ogg` |
| `crash` | `web/audio/crashsound.ogg` |
| `button` | `web/audio/buttonsound1.ogg` |
| `start` | `web/audio/buttonsound2.ogg` |

In Studio: **View > Asset Manager > Bulk Import**, then right-click each asset
and choose **Copy ID**.

The pilots are the Wii port's sprites, all 48 in one 910x910 sheet, so there is
one image to upload instead of 48. Each sprite is drawn at world scale with the
character's origin on the pilot's position, exactly as `game_render.c` does it.
The sheet's layout is defined in `Characters.luau`, and the tool reads it from
there. A sprite change means re-running `python tools/make_atlas.py`,
uploading the new sheet and pasting its new id. An upload is never updated in
place. CI runs `make_atlas.py --check`, so a stale sheet fails the build.

The sound follows `web/js/main.js`. The music starts on the first menu press,
fades in over two seconds to volume 0.3, and loops for the rest of the session,
crashes included. SOUND ON/OFF on the title screen mutes everything.

## What matches the website, and what does not

The screens carry the website's own words and art: its ASCII logo on the title
card, its how-to-play panel on the ready screen, "GAME OVER! / final score: /
too cheap for ya?!", the two-column score board, the credits, and the ASCII
star field that fills whatever a screen leaves around the 16:9 playfield. The
score plate, the cyan boundary lines, the three obstacle styles (candy stripe,
faceted crystal, rough crystal) with their band heights, colour cycles and
highlights, and the milestone markers with their complementary dashed line all
follow `web/js/`. A press anywhere on the title starts a run, and choosing a
pilot leads to the ready screen, as closing the website's character modal does.
Pilot select shows each character's description from `character-balance.js`.

Three things deliberately differ.

- **The columns' outlines are collision-true.** The website steps a drawn
  column's width every few bands while collision uses the smooth formula, so
  its picture and its hitbox disagree by a few units, and its rough style juts
  blocks out past its own collision width. Here every band is measured with the
  functions collision reads, and decoration is clamped inside that outline.
- **Scores are per account, not initials.** Roblox knows who is playing, so
  there is no initials prompt; the board shows names and each player's best.
- **Buttons are on the screen.** The website puts its controls in HTML around
  the canvas and in modals; there is nowhere here to put them but the screen.

Not here: shooting stars (`web/js/stars.js` defines them and the website's
renderer never calls them), and the extra characters in
`web/js/characters/more/`, which the website does not load either.

The game's name, art, audio and characters are reserved; see `../NOTICE`.
Uploading them to Roblox is for the rights holder to do.
