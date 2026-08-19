#ifndef STARS_H
#define STARS_H

/* Chunky SNES-style pixel starfield, ported from the source game's js/stars.js.
   Patterns and palette are deliberately limited -- the look depends on stars
   being a handful of distinct shapes in a few high-contrast colours, not smooth
   gradients. */

#define STAR_PATTERN_COUNT 7

typedef struct {
    int x, y;
    int pattern;                 /* 0..STAR_PATTERN_COUNT-1 */
    int color_r, color_g, color_b;
    int blink_frame;             /* 0..3; invisible only on frame 3 */
    int blink_speed;
    int frame_counter;
    int visible;
    int pulse_state;
    int is_pulsing;
} Star;

void stars_init(void);
void stars_update(void);
void stars_get(int index, Star **out);
int  stars_get_count(void);

/* Rare shooting stars with a fading pixel trail. Update during play; the web
   spawns them at roughly 0.3% per frame. */
#define MAX_SHOOTING_STARS 4

typedef struct {
    float x, y;
    float vx, vy;
    int   life;                  /* frames remaining; <= 0 means unused */
    int   color_cyan;            /* white otherwise */
} ShootingStar;

void shooting_stars_reset(void);
void shooting_stars_update(int canvas_width, int canvas_height);
void shooting_stars_get(int index, ShootingStar **out);
int  shooting_stars_get_count(void);

#endif
