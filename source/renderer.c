#include <gccore.h>
#include <stdio.h>
#include "renderer.h"
#include "config.h"
#include "stars.h"
#include "obstacles.h"

static void *framebuffer = NULL;
static GXRModeObj *rmode = NULL;

void renderer_init(void) {
    VIDEO_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(framebuffer, 20, 20, rmode->fbWidth, rmode->xfbHeight,
                 rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(framebuffer);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

void renderer_draw_background(void) {
    printf("\x1b[2J");
    printf("\x1b[0;0H");
}

void renderer_draw_stars(void) {
    Star *s;
    for (int i = 0; i < stars_get_count(); i++) {
        stars_get(i, &s);
        if (s && s->visible) {
            int brightness = s->is_pulsing ? (s->pulse_state ? 255 : 128) : 255;
            printf("\x1b[%d;%dH\x1b[38;2;%d;%d;%dm*\x1b[0m",
                   s->y / 8 + 1, s->x / 8 + 1,
                   brightness, brightness, brightness);
        }
    }
}

void renderer_draw_obstacles(void) {
    Obstacle *o;
    for (int i = 0; i < obstacles_get_count(); i++) {
        obstacles_get(i, &o);
        if (o) {
            int row_top = o->top_height / 8;
            int row_bot = o->bottom_y / 8;
            int col = (int)(o->x) / 8;

            for (int r = 0; r < row_top; r++) {
                printf("\x1b[%d;%dH\x1b[48;2;%d;%d;%dm \x1b[0m",
                       r + 1, col + 1,
                       o->theme.primary.r, o->theme.primary.g, o->theme.primary.b);
            }
            for (int r = row_bot; r < SCREEN_HEIGHT / 8; r++) {
                printf("\x1b[%d;%dH\x1b[48;2;%d;%d;%dm \x1b[0m",
                       r + 1, col + 1,
                       o->theme.secondary.r, o->theme.secondary.g, o->theme.secondary.b);
            }
        }
    }
}

void renderer_draw_player(const Player *p) {
    int row = (int)p->y / 8;
    int col = (int)p->x / 8;
    printf("\x1b[%d;%dH\x1b[48;2;255;255;255m  \x1b[0m", row + 1, col + 1);
    printf("\x1b[%d;%dH\x1b[48;2;255;255;255m  \x1b[0m", row + 2, col + 1);
}

void renderer_draw_score(int score) {
    printf("\x1b[1;1H SCORE: %d", score);
}
