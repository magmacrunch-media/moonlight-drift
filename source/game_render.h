#ifndef GAME_RENDER_H
#define GAME_RENDER_H

#include "characters.h"
#include "player.h"

/* Moonlight Drift's own presentation layer. This used to live in the engine,
   which baked a jetman-shaped game (idle/thrust sprite pair, starfield, cyan
   kill-lines) into code meant to be reusable. */

void game_render_load_sprites(const CharacterData *ch);
void game_render_free_sprites(void);
/* Whether the current character's idle+thrust pair actually loaded. */
int  game_render_sprites_loaded(void);

/* Idle portraits for every character, for the selector grid. Loading 24 files
   off SD is slow enough to look like a hang, so this draws a progress frame
   between reads. Safe to call repeatedly -- it returns immediately once loaded.
   Costs roughly 1.5MB of texture memory and is kept alive so reopening the
   selector is instant. */
void game_render_load_portraits(void);
int  game_render_portraits_ready(void);
void game_render_free_portraits(void);

/* Draws character `index`'s portrait with its origin at (x, y). Silently draws
   nothing when that portrait is missing; the caller frames the empty cell. */
void game_draw_portrait(int index, float x, float y, float scale);
int  game_portrait_valid(int index);

void game_draw_background(void);
void game_draw_stars(void);
void game_draw_player(const Player *p, int thrust_active);
void game_draw_score(int score);

/* Minimal fallbacks used when the corresponding GAMEPLAY_* switch is off, so a
   disabled feature still leaves the game playable and readable. */
void game_draw_score_plain(int score);
void game_draw_player_box(const Player *p);

#endif
