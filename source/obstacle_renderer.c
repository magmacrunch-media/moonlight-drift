#include <grrlib.h>
#include <math.h>
#include <stdio.h>
#include "obstacle_renderer.h"
#include "obstacles.h"
#include "config.h"
#include "renderer.h"
#include "ui_utils.h"

static u32 theme_color(ThemeColor c, u8 alpha) {
    return RGBA(c.r, c.g, c.b, alpha);
}

static void draw_obstacle_top(Obstacle *o) {
    float cx = o->x + OBSTACLE_WIDTH / 2.0f;

    switch (o->style_index) {
        case 0: {
            int stripe_h = 6;
            float w = OBSTACLE_WIDTH;
            for (int y = 0; y < o->top_height; y += stripe_h) {
                float progress = (float)y / (float)o->top_height;
                if (y % 24 < stripe_h) {
                    w = fmaxf(22.0f, OBSTACLE_WIDTH * (1.0f - progress * 0.65f));
                }
                float offset = sinf(o->seed * 100.0f + y * 0.1f) * 6.0f * o->asymmetry;
                float x = cx - w / 2.0f + offset;

                int si = (y / stripe_h) % 3;
                u32 sc;
                if (si == 0) sc = theme_color(o->theme.primary, 255);
                else if (si == 1) sc = theme_color(o->theme.secondary, 255);
                else sc = theme_color(o->theme.accent, 255);

                GRRLIB_Rectangle(x, (f32)y, w, (f32)stripe_h, sc, true);

                u32 hl = theme_color(o->theme.accent, 255);
                float hlw = fmaxf(4.0f, w * 0.2f);
                GRRLIB_Rectangle(x + 3, (f32)y, hlw, (f32)stripe_h, hl, true);
            }
            float tip_y = fmaxf(0.0f, (float)o->top_height - 16.0f);
            float tip_w = w * 0.4f;
            for (int i = 0; i < 16; i += 2) {
                float tw = tip_w * (1.0f - (float)i / 16.0f);
                if (tip_y + i < o->top_height) {
                    u32 tc = theme_color(o->theme.accent, 255);
                    GRRLIB_Rectangle(cx - tw / 2, tip_y + i, tw, 2, tc, true);
                }
            }
            break;
        }
        case 1: {
            int facet_h = 8;
            float w = OBSTACLE_WIDTH;
            for (int y = 0; y < o->top_height; y += facet_h) {
                float progress = (float)y / (float)o->top_height;
                int fi = y / facet_h;
                if (fi % 3 == 0) {
                    w = fmaxf(24.0f, OBSTACLE_WIDTH * (1.0f - progress * 0.6f));
                }
                float angle = sinf(o->seed * 200.0f + fi);
                float offset = angle * 10.0f * o->asymmetry;
                float x = cx - w / 2.0f + offset;

                int cp = (fi + (int)(o->seed * 10)) % 4;
                u32 fc;
                if (cp == 0) fc = theme_color(o->theme.primary, 255);
                else if (cp == 1) fc = theme_color(o->theme.secondary, 255);
                else fc = theme_color(o->theme.accent, 255);

                GRRLIB_Rectangle(x, (f32)y, w, (f32)facet_h, fc, true);

                if (angle > 0.2f) {
                    float hw = w * 0.35f;
                    GRRLIB_Rectangle(x, (f32)y, hw, (f32)facet_h,
                                     theme_color(o->theme.accent, 255), true);
                } else if (angle < -0.2f) {
                    float hw = w * 0.35f;
                    GRRLIB_Rectangle(x + w - hw, (f32)y, hw, (f32)facet_h,
                                     theme_color(o->theme.accent, 255), true);
                }
            }
            float tip_y = fmaxf(0.0f, (float)o->top_height - 12.0f);
            for (int i = 0; i < 12; i += 2) {
                float tw = (w * 0.4f) * (1.0f - (float)i / 12.0f);
                if (tip_y + i < o->top_height) {
                    GRRLIB_Rectangle(cx - tw / 2, tip_y + i, tw, 2,
                                     theme_color(o->theme.accent, 255), true);
                }
            }
            break;
        }
        default: {
            int bh = 10;
            float w = OBSTACLE_WIDTH;
            for (int y = 0; y < o->top_height; y += bh) {
                float progress = (float)y / (float)o->top_height;
                int bi = y / bh;
                if (bi % 2 == 0) {
                    float wv = sinf(o->seed * 300.0f + bi) * 10.0f * o->roughness;
                    w = fmaxf(20.0f, OBSTACLE_WIDTH * (1.0f - progress * 0.65f) + wv);
                }
                float offset = sinf(o->seed * 250.0f + bi * 0.7f) * 12.0f * o->asymmetry;
                float x = cx - w / 2.0f + offset;

                int cs = (int)(o->seed * 1000 + bi * 7) % 3;
                u32 bc;
                if (cs == 0) bc = theme_color(o->theme.primary, 255);
                else if (cs == 1) bc = theme_color(o->theme.secondary, 255);
                else bc = theme_color(o->theme.accent, 255);

                GRRLIB_Rectangle(x, (f32)y, w, (f32)bh, bc, true);

                int has_jut = sinf(o->seed * 400.0f + bi) > 0.6f;
                if (has_jut) {
                    float jw = 6.0f + sinf(o->seed * 500.0f + bi) * 4.0f;
                    if (jw < 2.0f) jw = 2.0f;
                    if (cosf(o->seed * 150.0f + bi) > 0) {
                        GRRLIB_Rectangle(x - jw, y + 2, jw, bh - 4,
                                         theme_color(o->theme.accent, 255), true);
                    } else {
                        GRRLIB_Rectangle(x + w, y + 2, jw, bh - 4,
                                         theme_color(o->theme.accent, 255), true);
                    }
                }

                GRRLIB_Rectangle(x + 2, (f32)y, 4, (f32)bh,
                                 theme_color(o->theme.accent, 255), true);
            }
            float tip_y = fmaxf(0.0f, (float)o->top_height - 14.0f);
            for (int i = 0; i < 14; i += 2) {
                float tw = (w * 0.35f) * (1.0f - (float)i / 14.0f);
                float wb = sinf(o->seed * 100.0f + i) * 2.0f;
                if (tip_y + i < o->top_height) {
                    GRRLIB_Rectangle(cx - tw / 2 + wb, tip_y + i, tw, 2,
                                     theme_color(o->theme.accent, 255), true);
                }
            }
            break;
        }
    }
}

static void draw_obstacle_bottom(Obstacle *o) {
    float cx = o->x + OBSTACLE_WIDTH / 2.0f;
    float bot_start = (float)o->bottom_y;
    float bot_len = (float)(CANVAS_HEIGHT - o->bottom_y);

    switch (o->style_index) {
        case 0: {
            int stripe_h = 6;
            float w = 22.0f;
            for (int y = 0; y < (int)bot_len; y += stripe_h) {
                float progress = (float)y / bot_len;
                if (y % 24 < stripe_h) {
                    w = fminf(OBSTACLE_WIDTH, 22.0f + OBSTACLE_WIDTH * progress * 0.65f);
                }
                float offset = sinf(o->seed * 90.0f + y * 0.1f) * 6.0f * o->asymmetry;
                float x = cx - w / 2.0f + offset;

                int si = (y / stripe_h) % 3;
                u32 sc;
                if (si == 0) sc = theme_color(o->theme.primary, 255);
                else if (si == 1) sc = theme_color(o->theme.secondary, 255);
                else sc = theme_color(o->theme.accent, 255);

                GRRLIB_Rectangle(x, bot_start + y, w, (f32)stripe_h, sc, true);

                float hlw = fmaxf(4.0f, w * 0.2f);
                GRRLIB_Rectangle(x + w - hlw - 3, bot_start + y, hlw, (f32)stripe_h,
                                 theme_color(o->theme.accent, 255), true);
            }
            for (int i = 0; i < 16; i += 2) {
                float tw = 22.0f * 0.4f * ((float)i / 16.0f);
                if (i < (int)bot_len) {
                    GRRLIB_Rectangle(cx - tw / 2, bot_start + i, tw, 2,
                                     theme_color(o->theme.accent, 255), true);
                }
            }
            break;
        }
        case 1: {
            int facet_h = 8;
            float w = 24.0f;
            for (int y = 0; y < (int)bot_len; y += facet_h) {
                float progress = (float)y / bot_len;
                int fi = y / facet_h;
                if (fi % 3 == 0) {
                    w = fminf(OBSTACLE_WIDTH, 24.0f + OBSTACLE_WIDTH * progress * 0.6f);
                }
                float angle = cosf(o->seed * 180.0f + fi);
                float offset = angle * 10.0f * o->asymmetry;
                float x = cx - w / 2.0f + offset;

                int cp = (fi + (int)(o->seed * 10)) % 4;
                u32 fc;
                if (cp == 0) fc = theme_color(o->theme.primary, 255);
                else if (cp == 1) fc = theme_color(o->theme.secondary, 255);
                else fc = theme_color(o->theme.accent, 255);

                GRRLIB_Rectangle(x, bot_start + y, w, (f32)facet_h, fc, true);

                if (angle > 0.2f) {
                    float hw = w * 0.35f;
                    GRRLIB_Rectangle(x, bot_start + y, hw, (f32)facet_h,
                                     theme_color(o->theme.accent, 255), true);
                } else if (angle < -0.2f) {
                    float hw = w * 0.35f;
                    GRRLIB_Rectangle(x + w - hw, bot_start + y, hw, (f32)facet_h,
                                     theme_color(o->theme.accent, 255), true);
                }
            }
            for (int i = 0; i < 12; i += 2) {
                float tw = (w * 0.4f) * ((float)i / 12.0f);
                if (i < (int)bot_len) {
                    GRRLIB_Rectangle(cx - tw / 2, bot_start + i, tw, 2,
                                     theme_color(o->theme.accent, 255), true);
                }
            }
            break;
        }
        default: {
            int bh = 10;
            float w = 20.0f;
            for (int y = 0; y < (int)bot_len; y += bh) {
                float progress = (float)y / bot_len;
                int bi = y / bh;
                if (bi % 2 == 0) {
                    float wv = cosf(o->seed * 280.0f + bi) * 10.0f * o->roughness;
                    w = fminf(OBSTACLE_WIDTH, 20.0f + OBSTACLE_WIDTH * progress * 0.65f + wv);
                }
                float offset = cosf(o->seed * 230.0f + bi * 0.6f) * 12.0f * o->asymmetry;
                float x = cx - w / 2.0f + offset;

                int cs = (int)(o->seed * 1000 + bi * 7) % 3;
                u32 bc;
                if (cs == 0) bc = theme_color(o->theme.primary, 255);
                else if (cs == 1) bc = theme_color(o->theme.secondary, 255);
                else bc = theme_color(o->theme.accent, 255);

                GRRLIB_Rectangle(x, bot_start + y, w, (f32)bh, bc, true);

                int has_jut = cosf(o->seed * 380.0f + bi) > 0.6f;
                if (has_jut) {
                    float jw = 6.0f + cosf(o->seed * 480.0f + bi) * 4.0f;
                    if (jw < 2.0f) jw = 2.0f;
                    if (sinf(o->seed * 140.0f + bi) > 0) {
                        GRRLIB_Rectangle(x - jw, bot_start + y + 2, jw, bh - 4,
                                         theme_color(o->theme.accent, 255), true);
                    } else {
                        GRRLIB_Rectangle(x + w, bot_start + y + 2, jw, bh - 4,
                                         theme_color(o->theme.accent, 255), true);
                    }
                }

                GRRLIB_Rectangle(x + w - 6, bot_start + y, 4, (f32)bh,
                                 theme_color(o->theme.accent, 255), true);
            }
            for (int i = 0; i < 14; i += 2) {
                float tw = (w * 0.35f) * ((float)i / 14.0f);
                float wb = cosf(o->seed * 90.0f + i) * 2.0f;
                if (i < (int)bot_len) {
                    GRRLIB_Rectangle(cx - tw / 2 + wb, bot_start + i, tw, 2,
                                     theme_color(o->theme.accent, 255), true);
                }
            }
            break;
        }
    }
}

#if GAMEPLAY_MILESTONE_MARKERS
/* Port of drawMilestoneMarkers() in js/obstacles.js: a dashed complementary rule
   through the gap, with the obstacle number in a bordered box. */
static void draw_milestone_marker(Obstacle *o) {
    if (!o->milestone) return;

    ThemeColor comp_c = theme_complementary(&o->theme);
    u32 comp = theme_color(comp_c, 255);
    float cx = o->x + OBSTACLE_WIDTH / 2.0f;
    int screen_h = renderer_screen_height();

    /* 8-on/8-off dashes, drawn manually since GX has no line-dash state. */
    for (int y = 0; y < screen_h; y += 16) {
        GRRLIB_Rectangle(cx - 2.0f, (f32)y, 4, 8, comp, true);
    }

    char text[16];
    snprintf(text, sizeof(text), "%d", o->milestone);

    int box_w = 80, box_h = 50;
    float text_y = o->top_height + (o->bottom_y - o->top_height) / 2.0f;
    float box_x = cx - box_w / 2.0f;
    float box_y = text_y - box_h / 2.0f;

    GRRLIB_Rectangle(box_x - 4, box_y - 4, box_w + 8, box_h + 8,
                     RGBA(26, 26, 46, 255), true);
    GRRLIB_Rectangle(box_x, box_y, box_w, box_h, RGBA(26, 26, 46, 230), true);
    GRRLIB_Rectangle(box_x - 2, box_y - 2, box_w + 4, box_h + 4, comp, false);
    GRRLIB_Rectangle(box_x + 2, box_y + 2, box_w - 4, box_h - 4,
                     theme_color(o->theme.secondary, 255), false);

    if (ttf_font) {
        unsigned int size = 20;
        u32 tw = GRRLIB_WidthTTF(ttf_font, text, size);
        float tx = cx - tw / 2.0f;
        float ty = text_y - size / 2.0f;
        GRRLIB_PrintfTTF(tx + 2, ty + 2, ttf_font, text, size, RGBA(0, 0, 0, 255));
        GRRLIB_PrintfTTF(tx, ty, ttf_font, text, size, comp);
    }
}

#endif /* GAMEPLAY_MILESTONE_MARKERS */

void obstacle_draw_all(void) {
    Obstacle *o;
    for (int i = 0; i < obstacles_get_count(); i++) {
        obstacles_get(i, &o);
        if (!o) continue;

        draw_obstacle_top(o);
        draw_obstacle_bottom(o);

        /* Port of drawSparkle() in js/obstacles.js. JS `%` on floats is fmod,
           so seed*200 % 1 is the fractional part -- integer arithmetic here
           collapses it to a constant. */
        if (fmodf(o->seed * 100.0f, 100.0f) < 8.0f) {
            float sx = o->x + o->seed * OBSTACLE_WIDTH;
            float sy;
            if (o->seed * 100.0f < 50.0f) {
                sy = fmodf(o->seed * 200.0f, 1.0f) * (float)o->top_height;
            } else {
                sy = (float)o->bottom_y + fmodf(o->seed * 300.0f, 1.0f) * 100.0f;
            }
            u32 wc = RGBA(255, 255, 255, 255);
            GRRLIB_Rectangle(sx,        sy,        2, 2, wc, true);
            GRRLIB_Rectangle(sx - 3.0f, sy,        2, 2, wc, true);
            GRRLIB_Rectangle(sx + 3.0f, sy,        2, 2, wc, true);
            GRRLIB_Rectangle(sx,        sy - 3.0f, 2, 2, wc, true);
            GRRLIB_Rectangle(sx,        sy + 3.0f, 2, 2, wc, true);
        }
    }

#if GAMEPLAY_MILESTONE_MARKERS
    /* Second pass so markers sit above every obstacle, as in the web renderer
       where drawMilestoneMarkers() runs after drawObstacles(). */
    for (int i = 0; i < obstacles_get_count(); i++) {
        obstacles_get(i, &o);
        if (o) draw_milestone_marker(o);
    }
#endif
}
