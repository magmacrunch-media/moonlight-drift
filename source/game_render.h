#ifndef GAME_RENDER_H
#define GAME_RENDER_H

#include "characters.h"
#include "player.h"

/* Moonlight Drift's own presentation layer. This used to live in the engine,
   which baked a jetman-shaped game (idle/thrust sprite pair, starfield, cyan
   kill-lines) into code meant to be reusable. */

void game_render_load_sprites(const CharacterData *ch);
void game_render_free_sprites(void);

void game_draw_background(void);
void game_draw_stars(void);
void game_draw_player(const Player *p, int thrust_active);
void game_draw_score(int score);

#endif
