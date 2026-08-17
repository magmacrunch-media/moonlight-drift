#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
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

int main(int argc, char **argv) {
    srand(time(NULL));

    renderer_init();
    input_init();
    player_init(NULL);
    obstacles_init();
    stars_init();
    scoring_init();

    Player player;
    player_init(&player);

    while (1) {
        input_scan();

        if (input_home_pressed()) break;

        switch (state) {
            case STATE_TITLE:
                renderer_draw_background();
                printf("\x1b[12;25H MOONLIGHT DRIFT");
                printf("\x1b[14;28H Press A to start");
                if (input_start_pressed()) {
                    state = STATE_READY;
                }
                break;

            case STATE_READY:
                renderer_draw_background();
                printf("\x1b[12;28H Hold A to thrust");
                printf("\x1b[14;28H Dodge obstacles!");
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

                renderer_draw_background();
                stars_update();
                renderer_draw_stars();
                renderer_draw_obstacles();
                renderer_draw_player(&player);
                renderer_draw_score(scoring_get());
                break;

            case STATE_GAME_OVER:
                renderer_draw_background();
                printf("\x1b[12;28H GAME OVER");
                printf("\x1b[14;24H Score: %d", scoring_get());
                printf("\x1b[16;24H Press A to retry");
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

        VIDEO_WaitVSync();
    }

    return 0;
}
