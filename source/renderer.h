#ifndef RENDERER_H
#define RENDERER_H

#include "player.h"
#include "obstacles.h"

void renderer_init(void);
void renderer_draw_background(void);
void renderer_draw_stars(void);
void renderer_draw_obstacles(void);
void renderer_draw_player(const Player *p);
void renderer_draw_score(int score);

#endif
