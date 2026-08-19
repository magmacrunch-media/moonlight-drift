#ifndef OBSTACLES_H
#define OBSTACLES_H

#include "theme.h"

#define MAX_OBSTACLES 20

typedef struct {
    float x;
    int top_height;
    int bottom_y;
    int passed;
    int style_index;
    float seed;   /* 0..1, matches Math.random() in the source game */
    float roughness;
    float asymmetry;
    int milestone;
    Theme theme;
} Obstacle;

void obstacles_init(void);
void obstacles_create(int canvas_width, int canvas_height);
void obstacles_update(int canvas_width);
int obstacles_check_collision(int player_x, int player_y, int player_w, int player_h);
int obstacles_check_score(int player_x);
int obstacles_get_count(void);
void obstacles_get(int index, Obstacle **out);

#endif
