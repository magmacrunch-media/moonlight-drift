#include <math.h>
#include <stdlib.h>
#include "obstacles.h"
#include "config.h"

static Obstacle obstacles[MAX_OBSTACLES];
static int obstacle_count = 0;
static int total_obstacles_created = 0;

/* One palette is shared by every obstacle on screen and rotates every
   THEME_CHANGE_INTERVAL spawns, so colours move in coherent runs. Generating a
   fresh theme per obstacle (as this previously did) makes every column a
   different random colour, which is the main reason the port did not read like
   the arcade version. Mirrors getCurrentTheme() in js/obstacles.js. */
static Theme current_theme;
static int last_theme_change_count = 0;

static float rand_unit(void) {
    return (float)rand() / ((float)RAND_MAX + 1.0f);
}

static const Theme *theme_current(void) {
    if (total_obstacles_created - last_theme_change_count >= THEME_CHANGE_INTERVAL) {
        theme_generate(&current_theme);
        last_theme_change_count = total_obstacles_created;
    }
    return &current_theme;
}

void obstacles_init(void) {
    obstacle_count = 0;
    total_obstacles_created = 0;
    last_theme_change_count = 0;
    theme_generate(&current_theme);
}

void obstacles_create(int canvas_width, int canvas_height) {
    if (obstacle_count >= MAX_OBSTACLES) return;

    Obstacle *o = &obstacles[obstacle_count];
    o->x = (float)canvas_width;
    o->top_height = MIN_OBSTACLE_HEIGHT + rand() % (MAX_OBSTACLE_HEIGHT - MIN_OBSTACLE_HEIGHT);
    o->bottom_y = o->top_height + GAP;
    o->passed = 0;
    o->style_index = rand() % 3;
    o->seed = rand_unit();
    o->roughness = 0.5f + rand_unit() * 0.5f;
    o->asymmetry = rand_unit() * 2.0f - 1.0f;

    total_obstacles_created++;

    /* Count first, then test: the source game marks the 10th, 20th ... obstacle,
       and uses the count itself as the label. */
    o->milestone = (total_obstacles_created % THEME_CHANGE_INTERVAL == 0)
                 ? total_obstacles_created : 0;

    o->theme = *theme_current();

    obstacle_count++;
}

void obstacles_update(int canvas_width) {
    for (int i = obstacle_count - 1; i >= 0; i--) {
        obstacles[i].x -= OBSTACLE_SPEED;
        if (obstacles[i].x + OBSTACLE_WIDTH < 0) {
            for (int j = i; j < obstacle_count - 1; j++) {
                obstacles[j] = obstacles[j + 1];
            }
            obstacle_count--;
        }
    }
}

static int check_collision_style(Obstacle *o, int player_left, int player_right,
                                  int player_top, int player_bottom) {
    float center_x = o->x + OBSTACLE_WIDTH / 2.0f;

    if (player_right <= o->x || player_left >= o->x + OBSTACLE_WIDTH) return 0;

    if (player_top < o->top_height) {
        int y_pos = player_top;
        if (y_pos < 0) y_pos = 0;
        if (y_pos > o->top_height - 1) y_pos = o->top_height - 1;
        float progress = (float)y_pos / (float)o->top_height;
        float w, offset;

        switch (o->style_index) {
            case 0:
                w = fmaxf(22.0f, OBSTACLE_WIDTH * (1.0f - progress * 0.65f));
                offset = sinf(o->seed * 100.0f + y_pos * 0.1f) * 6.0f * o->asymmetry;
                break;
            case 1:
                w = fmaxf(24.0f, OBSTACLE_WIDTH * (1.0f - progress * 0.6f));
                offset = sinf(o->seed * 50.0f + y_pos * 0.2f) * 4.0f;
                break;
            default:
                w = fmaxf(20.0f, OBSTACLE_WIDTH * (1.0f - progress * 0.65f));
                offset = sinf(o->seed * 200.0f + y_pos * 0.5f) * 8.0f * o->roughness;
                break;
        }

        float left = center_x - w / 2.0f + offset;
        float right = center_x + w / 2.0f + offset;
        if (player_right > left && player_left < right) return 1;
    }

    if (player_bottom > o->bottom_y) {
        float bottom_height = 500.0f;
        int y_pos = player_bottom - o->bottom_y;
        if (y_pos > (int)bottom_height) y_pos = (int)bottom_height;
        float progress = (float)y_pos / bottom_height;
        float w, offset;

        switch (o->style_index) {
            case 0:
                w = fminf(OBSTACLE_WIDTH, 22.0f + OBSTACLE_WIDTH * progress * 0.65f);
                offset = sinf(o->seed * 100.0f + y_pos * 0.1f) * 6.0f * o->asymmetry;
                break;
            case 1:
                w = fminf(OBSTACLE_WIDTH, 24.0f + OBSTACLE_WIDTH * progress * 0.6f);
                offset = sinf(o->seed * 50.0f + y_pos * 0.2f) * 4.0f;
                break;
            default:
                w = fminf(OBSTACLE_WIDTH, 20.0f + OBSTACLE_WIDTH * progress * 0.65f);
                offset = sinf(o->seed * 200.0f + y_pos * 0.5f) * 8.0f * o->roughness;
                break;
        }

        float left = center_x - w / 2.0f + offset;
        float right = center_x + w / 2.0f + offset;
        if (player_right > left && player_left < right) return 1;
    }

    return 0;
}

int obstacles_check_collision(int player_x, int player_y, int player_w, int player_h) {
    int player_top = player_y;
    int player_bottom = player_y + player_h - FLAME_HEIGHT;
    int player_left = player_x;
    int player_right = player_x + player_w;

    for (int i = 0; i < obstacle_count; i++) {
        Obstacle *o = &obstacles[i];
        if (check_collision_style(o, player_left, player_right, player_top, player_bottom)) {
            return 1;
        }
    }
    return 0;
}

int obstacles_check_score(int player_x) {
    int score_increment = 0;
    for (int i = 0; i < obstacle_count; i++) {
        if (!obstacles[i].passed && obstacles[i].x + OBSTACLE_WIDTH < player_x) {
            obstacles[i].passed = 1;
            score_increment++;
        }
    }
    return score_increment;
}

int obstacles_get_count(void) {
    return obstacle_count;
}

void obstacles_get(int index, Obstacle **out) {
    if (index >= 0 && index < obstacle_count) {
        *out = &obstacles[index];
    } else {
        *out = NULL;
    }
}
