#include <math.h>
#include <stdlib.h>
#include "obstacles.h"
#include "config.h"

static Obstacle obstacles[MAX_OBSTACLES];
static int obstacle_count = 0;
static int total_obstacles_created = 0;

void obstacles_init(void) {
    obstacle_count = 0;
    total_obstacles_created = 0;
}

void obstacles_create(int canvas_width, int canvas_height) {
    if (obstacle_count >= MAX_OBSTACLES) return;

    Obstacle *o = &obstacles[obstacle_count];
    o->x = (float)canvas_width;
    o->top_height = MIN_OBSTACLE_HEIGHT + rand() % (MAX_OBSTACLE_HEIGHT - MIN_OBSTACLE_HEIGHT);
    o->bottom_y = o->top_height + GAP;
    o->passed = 0;
    o->style_index = rand() % 3;
    o->seed = rand() % 1000;
    o->roughness = 0.5f + (float)(rand() % 500) / 1000.0f;
    o->asymmetry = (float)(rand() % 2000 - 1000) / 1000.0f;
    o->milestone = (total_obstacles_created % THEME_CHANGE_INTERVAL == 0) ? total_obstacles_created : 0;

    theme_generate(&o->theme);

    obstacle_count++;
    total_obstacles_created++;
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

static float get_obstacle_width_at_y(Obstacle *o, int y_pos, int total_height) {
    float progress = (float)y_pos / (float)total_height;
    float min_w;
    float taper;

    switch (o->style_index) {
        case 0: min_w = 22.0f; taper = 0.65f; break;
        case 1: min_w = 24.0f; taper = 0.60f; break;
        default: min_w = 20.0f; taper = 0.65f; break;
    }

    float width = min_w + (OBSTACLE_WIDTH - min_w) * (1.0f - progress * taper);
    return width;
}

static float get_obstacle_offset(Obstacle *o, int y_pos) {
    switch (o->style_index) {
        case 0: return sinf(o->seed * 100.0f + y_pos * 0.1f) * 6.0f * o->asymmetry;
        case 1: return sinf(o->seed * 50.0f + y_pos * 0.2f) * 4.0f;
        default: return sinf(o->seed * 200.0f + y_pos * 0.5f) * 8.0f * o->roughness;
    }
}

int obstacles_check_collision(int player_x, int player_y, int player_w, int player_h) {
    for (int i = 0; i < obstacle_count; i++) {
        Obstacle *o = &obstacles[i];
        float ox_right = o->x + OBSTACLE_WIDTH;

        if (player_x + player_w < o->x || player_x > ox_right) continue;

        int total_h = o->bottom_y - o->top_height;

        int player_bottom = player_y + player_h;
        int check_y_top = player_y - o->top_height;
        int check_y_bot = player_bottom - o->top_height;

        if (check_y_top >= 0 && check_y_top <= total_h) {
            float w = get_obstacle_width_at_y(o, check_y_top, total_h);
            float offset = get_obstacle_offset(o, check_y_top);
            float center_x = o->x + OBSTACLE_WIDTH / 2.0f;
            float left = center_x - w / 2.0f + offset;
            float right = center_x + w / 2.0f + offset;
            if (player_x < right && player_x + player_w > left) return 1;
        }

        if (check_y_bot >= 0 && check_y_bot <= total_h) {
            float w = get_obstacle_width_at_y(o, check_y_bot, total_h);
            float offset = get_obstacle_offset(o, check_y_bot);
            float center_x = o->x + OBSTACLE_WIDTH / 2.0f;
            float left = center_x - w / 2.0f + offset;
            float right = center_x + w / 2.0f + offset;
            if (player_x < right && player_x + player_w > left) return 1;
        }

        if (player_y < o->top_height && player_bottom > o->top_height) {
            float center_x = o->x + OBSTACLE_WIDTH / 2.0f;
            float left = center_x - OBSTACLE_WIDTH / 2.0f;
            float right = center_x + OBSTACLE_WIDTH / 2.0f;
            if (player_x < right && player_x + player_w > left) return 1;
        }
        if (player_y < o->bottom_y && player_bottom > o->bottom_y) {
            float center_x = o->x + OBSTACLE_WIDTH / 2.0f;
            float left = center_x - OBSTACLE_WIDTH / 2.0f;
            float right = center_x + OBSTACLE_WIDTH / 2.0f;
            if (player_x < right && player_x + player_w > left) return 1;
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
