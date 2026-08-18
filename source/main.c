#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <grrlib.h>
#include "config.h"
#include "player.h"
#include "input.h"
#include "obstacles.h"
#include "stars.h"
#include "scoring.h"
#include "renderer.h"

typedef enum {
    STATE_TITLE,
    STATE_READY,
    STATE_PLAYING,
    STATE_GAME_OVER
} GameState;

static GameState state = STATE_TITLE;
static int frame_count = 0;
static int game_running = 0;

static void draw_centered_text(int y, const char *text, unsigned int size, u32 color) {
    int len = 0;
    for (const char *p = text; *p; p++) len++;
    int text_width = len * (int)(size * 0.6f);
    int x = (SCREEN_WIDTH - text_width) / 2;
    if (x < 0) x = 0;
    GRRLIB_PrintfSystemFont(x, y, text, size, color);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    srand(time(NULL));

    renderer_init();
    input_init();
    obstacles_init();
    stars_init();
    scoring_init();

    Player player;
    player_init(&player);

    u32 white = RGBA(255, 255, 255, 255);
    u32 dim   = RGBA(180, 180, 180, 255);

    while (1) {
        input_scan();

        if (input_home_pressed()) break;

        switch (state) {
            case STATE_TITLE:
                renderer_draw_background();
                draw_centered_text(SCREEN_HEIGHT / 2 - 40, "MOONLIGHT DRIFT", 30, white);
                draw_centered_text(SCREEN_HEIGHT / 2 + 10, "Press A to start", 20, dim);
                if (input_start_pressed()) {
                    state = STATE_READY;
                }
                break;

            case STATE_READY:
                renderer_draw_background();
                draw_centered_text(SCREEN_HEIGHT / 2 - 20, "Hold A to thrust", 20, white);
                draw_centered_text(SCREEN_HEIGHT / 2 + 10, "Dodge obstacles!", 20, dim);
                if (input_start_pressed()) {
                    state = STATE_PLAYING;
                    game_running = 1;
                    frame_count = 0;
                    scoring_reset();
                    obstacles_init();
                    player_init(&player);
                }
                break;

            case STATE_PLAYING:
                frame_count++;

                int boundary_hit = player_update(&player, input_thrust_pressed(), SCREEN_HEIGHT);
                if (boundary_hit) {
                    state = STATE_GAME_OVER;
                    break;
                }

                if (frame_count % OBSTACLE_SPAWN_INTERVAL == 0) {
                    obstacles_create(CANVAS_WIDTH, CANVAS_HEIGHT);
                }

                obstacles_update(CANVAS_WIDTH);

                int hit = obstacles_check_collision(
                    (int)player.x + player.offsetX,
                    (int)player.y + player.offsetY,
                    player.width, player.height
                );
                if (hit) {
                    state = STATE_GAME_OVER;
                    break;
                }

                int score_inc = obstacles_check_score((int)player.x);
                for (int i = 0; i < score_inc; i++) {
                    scoring_increment();
                }

                stars_update();
                renderer_draw_background();
                renderer_draw_stars();
                renderer_draw_obstacles();
                renderer_draw_player(&player);
                renderer_draw_score(scoring_get());
                break;

            case STATE_GAME_OVER:
                renderer_draw_background();
                draw_centered_text(SCREEN_HEIGHT / 2 - 40, "GAME OVER", 30, white);

                {
                    char score_buf[32];
                    int sc = scoring_get();
                    int len = 0;
                    if (sc == 0) {
                        score_buf[len++] = '0';
                    } else {
                        char rev[16];
                        int rlen = 0;
                        while (sc > 0) {
                            rev[rlen++] = '0' + (sc % 10);
                            sc /= 10;
                        }
                        for (int i = rlen - 1; i >= 0; i--) {
                            score_buf[len++] = rev[i];
                        }
                    }
                    score_buf[len++] = ' ';
                    score_buf[len++] = 'p';
                    score_buf[len++] = 't';
                    score_buf[len] = '\0';
                    draw_centered_text(SCREEN_HEIGHT / 2 + 0, score_buf, 20, dim);
                }

                draw_centered_text(SCREEN_HEIGHT / 2 + 30, "Press A to retry", 20, dim);
                if (input_start_pressed()) {
                    state = STATE_PLAYING;
                    game_running = 1;
                    frame_count = 0;
                    scoring_reset();
                    obstacles_init();
                    player_init(&player);
                }
                break;
        }

        renderer_finish();
    }

    return 0;
}
