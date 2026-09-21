# Moonlight Drift for Roblox

The fourth version of the game, in Luau. You hold to thrust, let go to fall, and
dodge the cave. It keeps the rules of the other three frame for frame, and it
draws in the GUI as a 2D arcade game. Nobody gets an avatar.

## Layout

| Path | Becomes | What it is |
|------|---------|------------|
| `src/shared/` | `ReplicatedStorage.MoonlightDrift` | The rules: `Config`, `Characters`, `Theme`, `Pilot`, `Obstacles`, `Run`. No Roblox services, so they also run under Lune. |
| `src/client/` | `StarterPlayerScripts.MoonlightDrift` | `Main` (state machine, input, fixed-step loop), `Playfield` (renderer), `Screens` (menus). |
| `src/server/` | `ServerScriptService.MoonlightDrift` | `Scores`: the high-score board and the checks on submitted scores. |
| `tests/run.luau` | nothing | Host tests for `src/shared`, run with Lune. |
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

## Not yet here

- **Art.** The pilots are drawn as their accent colour and glyph, at the size of
  the hitbox. To use the real sprites, upload the PNGs from `wii/sprites/` as
  Roblox images and replace the body in `Playfield.luau` with an `ImageLabel`.
  The sprite origins in `characters.c` say where each hitbox sits inside its
  image.
- **Audio.** `SOUND_IDS` in `Main.client.luau` is empty. Upload
  `web/audio/moonlightdrift-gameloop.ogg`, `crashsound.ogg` and
  `buttonsound1.ogg`, then paste in the asset ids.
- **Shooting stars**, and the web version's character-select portraits.

The game's name, art, audio and characters are reserved; see `../NOTICE`.
Uploading them to Roblox is for the rights holder to do.
