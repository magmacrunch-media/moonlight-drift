#include <stdlib.h>
#include <gctypes.h>
#include "stars.h"
#include "config.h"

static Star stars[STAR_COUNT];
static ShootingStar shooters[MAX_SHOOTING_STARS];

/* The web's SNES_STAR_COLORS: white is listed three times so it dominates,
   with pale blue/pink and cyan/gold accents. Weighting matters -- an evenly
   sampled palette reads as noise rather than a starfield. */
static const u8 star_palette[][3] = {
    { 255, 255, 255 },
    { 255, 255, 255 },
    { 255, 255, 255 },
    { 224, 224, 255 },
    { 255, 224, 224 },
    {   0, 212, 255 },
    { 255, 215,   0 }
};
#define STAR_PALETTE_SIZE ((int)(sizeof(star_palette) / sizeof(star_palette[0])))

void stars_init(int world_w, int world_h) {
    if (world_w < 1) world_w = 1;
    if (world_h < 1) world_h = 1;
    shooting_stars_reset();
    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i].x = rand() % world_w;
        stars[i].y = rand() % world_h;
        stars[i].pattern = rand() % STAR_PATTERN_COUNT;
        int c = rand() % STAR_PALETTE_SIZE;
        stars[i].color_r = star_palette[c][0];
        stars[i].color_g = star_palette[c][1];
        stars[i].color_b = star_palette[c][2];
        stars[i].blink_frame = 0;
        stars[i].blink_speed = 30 + rand() % 40;
        stars[i].frame_counter = 0;
        stars[i].visible = 1;
        stars[i].pulse_state = 0;
        stars[i].is_pulsing = (rand() % 100 < 30) ? 1 : 0;
    }
}

void stars_update(void) {
    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i].frame_counter++;
        if (stars[i].frame_counter < stars[i].blink_speed) continue;

        stars[i].frame_counter = 0;
        /* Four-phase cycle, dark only on the last phase: stars are lit 75% of
           the time. Toggling instead gives a 50% duty and reads as flickering. */
        stars[i].blink_frame = (stars[i].blink_frame + 1) % 4;
        if (stars[i].blink_frame == 3) {
            stars[i].visible = 0;
        } else {
            stars[i].visible = 1;
            if (stars[i].is_pulsing && stars[i].blink_frame == 0) {
                stars[i].pulse_state = (stars[i].pulse_state + 1) % 2;
            }
        }
    }
}

void shooting_stars_reset(void) {
    for (int i = 0; i < MAX_SHOOTING_STARS; i++) shooters[i].life = 0;
}

void shooting_stars_update(int canvas_width, int canvas_height) {
    for (int i = 0; i < MAX_SHOOTING_STARS; i++) {
        if (shooters[i].life <= 0) continue;
        shooters[i].x += shooters[i].vx;
        shooters[i].y += shooters[i].vy;
        shooters[i].life--;
        if (shooters[i].x < -20.0f) shooters[i].life = 0;
    }

    /* ~0.3% per frame, as in js/stars.js -- roughly one every five seconds. */
    if (rand() % 1000 >= 3) return;

    for (int i = 0; i < MAX_SHOOTING_STARS; i++) {
        if (shooters[i].life > 0) continue;
        shooters[i].x = (float)canvas_width + 10.0f;
        shooters[i].y = (float)(rand() % (canvas_height * 3 / 5));
        shooters[i].vx = -6.0f - (float)(rand() % 300) / 100.0f;
        shooters[i].vy =  1.0f + (float)(rand() % 200) / 100.0f;
        shooters[i].life = 40;
        shooters[i].color_cyan = rand() % 2;
        break;
    }
}

void shooting_stars_get(int index, ShootingStar **out) {
    *out = (index >= 0 && index < MAX_SHOOTING_STARS) ? &shooters[index] : NULL;
}

int shooting_stars_get_count(void) { return MAX_SHOOTING_STARS; }

void stars_get(int index, Star **out) {
    if (index >= 0 && index < STAR_COUNT) {
        *out = &stars[index];
    } else {
        *out = NULL;
    }
}

int stars_get_count(void) {
    return STAR_COUNT;
}
