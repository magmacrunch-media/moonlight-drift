#include <grrlib.h>
#include <stdio.h>
#include "magnolia.h"
#include "game_render.h"
#include "stars.h"

static Sprite spr_idle;
static Sprite spr_thrust;
static int sprites_ready = 0;

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
