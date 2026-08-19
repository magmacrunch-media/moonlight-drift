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

void game_draw_stars(void) {
    Star *s;
    for (int i = 0; i < stars_get_count(); i++) {
        stars_get(i, &s);
        if (!s || !s->visible) continue;

        u8 brightness = s->is_pulsing ? (s->pulse_state ? 255 : 128) : 255;
        u32 color = RGBA(s->color_r, s->color_g, s->color_b, brightness);

        int size;
        switch (s->pattern % 4) {
            case 0:  size = 1; break;
            case 1:  size = 2; break;
            case 2:  size = 3; break;
            default: size = 2; break;
        }
        GRRLIB_Rectangle((f32)s->x, (f32)s->y, (f32)size, (f32)size, color, true);
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

void game_draw_score(int score) {
    if (!ttf_font) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", score);
    GRRLIB_PrintfTTF(20, 10, ttf_font, "SCORE:", 14, RGBA(255, 255, 255, 255));
    GRRLIB_PrintfTTF(120, 10, ttf_font, buf, 14, RGBA(255, 255, 255, 255));
}
