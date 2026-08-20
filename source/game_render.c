#include <grrlib.h>
#include <stdio.h>
#include "magnolia.h"
#include "game_render.h"
#include "stars.h"

#define MAX_PORTRAITS 32

static Sprite spr_idle;
static Sprite spr_thrust;
static int sprites_ready = 0;

static Sprite portraits[MAX_PORTRAITS];
static int portrait_count = 0;
static int portraits_ready = 0;

void game_render_load_sprites(const CharacterData *ch) {
    game_render_free_sprites();
    if (!ch) return;

    /* Character art records where its drawing origin sits inside the PNG; the
       engine places that point at the player position. */
    int ok_idle = sprite_load(&spr_idle,
                              magnolia_asset_path(ch->sprite_idle),
                              ch->sprite_origin_x, ch->sprite_origin_y);
    int ok_thrust = sprite_load(&spr_thrust,
                                magnolia_asset_path(ch->sprite_thrust),
                                ch->sprite_origin_x, ch->sprite_origin_y);
    sprites_ready = ok_idle && ok_thrust;
}

int game_render_sprites_loaded(void) { return sprites_ready; }

void game_render_free_sprites(void) {
    sprite_free(&spr_idle);
    sprite_free(&spr_thrust);
    sprites_ready = 0;
}

static void draw_portrait_progress(int done, int total) {
    game_draw_background();
    game_draw_stars();
    ui_draw_border();
    ui_draw_centered_text(210, "LOADING CHARACTERS", 16, RGBA(0, 212, 255, 255));

    int bar_w = 400, bar_h = 14, bar_x = (UI_DESIGN_WIDTH - bar_w) / 2, bar_y = 250;
    GRRLIB_Rectangle(ui_map_x(bar_x), ui_map_y(bar_y),
                     ui_map_w(bar_w), ui_map_h(bar_h),
                     RGBA(0, 212, 255, 255), false);
    if (total > 0 && done > 0) {
        int fill = (bar_w - 4) * done / total;
        GRRLIB_Rectangle(ui_map_x(bar_x + 2), ui_map_y(bar_y + 2),
                         ui_map_w(fill), ui_map_h(bar_h - 4),
                         RGBA(0, 212, 255, 255), true);
    }
    renderer_finish();
}

void game_render_load_portraits(void) {
    if (portraits_ready) return;

    int total = characters_get_count();
    if (total > MAX_PORTRAITS) total = MAX_PORTRAITS;
    portrait_count = total;

    for (int i = 0; i < total; i++) {
        draw_portrait_progress(i, total);
        const CharacterData *ch = characters_get_by_index(i);
        if (!ch) continue;
        sprite_load(&portraits[i], magnolia_asset_path(ch->sprite_idle),
                    ch->sprite_origin_x, ch->sprite_origin_y);
    }
    portraits_ready = 1;
}

int game_render_portraits_ready(void) { return portraits_ready; }

void game_render_free_portraits(void) {
    for (int i = 0; i < portrait_count; i++) sprite_free(&portraits[i]);
    portrait_count = 0;
    portraits_ready = 0;
}

int game_portrait_valid(int index) {
    return index >= 0 && index < portrait_count && sprite_valid(&portraits[index]);
}

void game_draw_portrait(int index, float x, float y, float scale) {
    if (!game_portrait_valid(index)) return;
    sprite_draw_scaled(&portraits[index], x, y, scale);
}

void game_draw_background(void) {
    renderer_draw_background();

    /* Cyan rules on the lethal top/bottom edges, as drawBackground() does in
       the source game. On a TV these also show where the kill boundary is once
       overscan eats the outer rows. */
    int w = renderer_screen_width();
    int h = renderer_screen_height();
    u32 edge = RGBA(0, 212, 255, 77);
    GRRLIB_Rectangle(0, 0, (f32)w, 2, edge, true);
    GRRLIB_Rectangle(0, (f32)(h - 2), (f32)w, 2, edge, true);
}

static void px(int x, int y, int w, int h, u32 c) {
    GRRLIB_Rectangle((f32)x, (f32)y, (f32)w, (f32)h, c, true);
}

/* The seven STAR_PATTERNS from js/stars.js, drawn as literal pixel runs. The
   chunky, deliberately low-res shapes are the whole point of the look. */
static void draw_star_pattern(int pattern, int x, int y, u32 c) {
    switch (pattern) {
        case 0:                                    /* single pixel */
            px(x, y, 1, 1, c);
            break;
        case 1:                                    /* 2x2 */
            px(x, y, 2, 2, c);
            break;
        case 2:                                    /* 3x3 */
            px(x, y, 3, 3, c);
            break;
        case 3:                                    /* plus */
            px(x + 1, y, 1, 3, c);
            px(x, y + 1, 3, 1, c);
            break;
        case 4:                                    /* large plus */
            px(x + 2, y, 1, 5, c);
            px(x, y + 2, 5, 1, c);
            break;
        case 5:                                    /* diamond */
            px(x + 2, y,     1, 1, c);
            px(x + 1, y + 1, 3, 1, c);
            px(x,     y + 2, 5, 1, c);
            px(x + 1, y + 3, 3, 1, c);
            px(x + 2, y + 4, 1, 1, c);
            break;
        default:                                   /* 8-point star */
            px(x + 2, y + 2, 2, 2, c);
            px(x + 2, y,     2, 1, c);
            px(x + 2, y + 5, 2, 1, c);
            px(x,     y + 2, 1, 2, c);
            px(x + 5, y + 2, 1, 2, c);
            px(x + 1, y + 1, 1, 1, c);
            px(x + 4, y + 1, 1, 1, c);
            px(x + 1, y + 4, 1, 1, c);
            px(x + 4, y + 4, 1, 1, c);
            break;
    }
}

void game_draw_stars(void) {
    Star *s;
    for (int i = 0; i < stars_get_count(); i++) {
        stars_get(i, &s);
        if (!s || !s->visible) continue;

        /* Pulsing stars dim via alpha, matching the web's '88' suffix. */
        u8 alpha = (s->is_pulsing && s->pulse_state == 0) ? 136 : 255;
        draw_star_pattern(s->pattern, s->x, s->y,
                          RGBA(s->color_r, s->color_g, s->color_b, alpha));
    }

    ShootingStar *sh;
    for (int i = 0; i < shooting_stars_get_count(); i++) {
        shooting_stars_get(i, &sh);
        if (!sh || sh->life <= 0) continue;

        u8 r = sh->color_cyan ?   0 : 255;
        u8 g = sh->color_cyan ? 212 : 255;
        u8 b = 255;

        for (int t = 0; t < 8; t++) {
            int a = (int)((1.0f - (float)t / 8.0f) * 255.0f);
            if (a <= 50) continue;
            int tx = (int)(sh->x + t * 2.0f);
            int ty = (int)(sh->y + t * 0.4f);
            px(tx, ty, 2, 2, RGBA(r, g, b, (u8)a));
            if (t < 3) {                            /* glow on the leading end */
                px(tx - 1, ty, 1, 2, RGBA(r, g, b, 68));
                px(tx + 2, ty, 1, 2, RGBA(r, g, b, 68));
            }
        }
    }
}

void game_draw_player(const Player *p, int thrust_active) {
    if (sprites_ready) {
        sprite_draw(thrust_active ? &spr_thrust : &spr_idle, p->x, p->y);
    } else {
        GRRLIB_Rectangle(p->x, p->y, (f32)p->width, (f32)p->height,
                         RGBA(255, 255, 255, 255), true);
    }
}

/* SNES-style score plate, matching drawScoreOnCanvas() in the source game:
   a blue-grey box with chunky light/dark borders and gold text outlined in
   black. Drawn in design space so it holds position across video modes. */
void game_draw_score(int score) {
    if (!ttf_font) return;

    const int bx = 10, by = 10, bw = 180, bh = 50;
    int x = ui_map_x(bx), y = ui_map_y(by);
    int w = ui_map_w(bw), h = ui_map_h(bh);

    GRRLIB_Rectangle(x, y, w, h, RGBA(0x3a, 0x44, 0x66, 255), true);
    /* Two nested outlines stand in for the web's 3px and 2px strokes. */
    GRRLIB_Rectangle(x, y, w, h, RGBA(0x6a, 0x7a, 0x9a, 255), false);
    GRRLIB_Rectangle(x + 1, y + 1, w - 2, h - 2, RGBA(0x6a, 0x7a, 0x9a, 255), false);
    GRRLIB_Rectangle(x + 4, y + 4, w - 8, h - 8, RGBA(0x2a, 0x3a, 0x5a, 255), false);

    char buf[24];
    snprintf(buf, sizeof(buf), "score: %d", score);

    int tx = ui_map_x(bx + 15), ty = ui_map_y(by + 17);
    unsigned int size = ui_map_size(14);
    /* Black outline, drawn as four offsets, then the gold fill. */
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            GRRLIB_PrintfTTF(tx + dx, ty + dy, ttf_font, buf, size, RGBA(0, 0, 0, 255));
        }
    }
    GRRLIB_PrintfTTF(tx, ty, ttf_font, buf, size, RGBA(255, 215, 0, 255));
}
