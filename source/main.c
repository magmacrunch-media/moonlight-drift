#include <gccore.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "magnolia.h"
#include "config.h"
#include "characters.h"
#include "player.h"
#include "stars.h"
#include "obstacles.h"
#include "obstacle_renderer.h"
#include "game_render.h"
#include "ui.h"

static int frame_count = 0;

/* The selector is a sub-mode of the title screen rather than an engine state:
   character select is this game's concern, and magnolia only grows an
   abstraction once a second game needs it. */
static int char_select_open = 0;
static int char_cursor = 0;

static void apply_character(Player *p) {
    const CharacterData *ch = characters_get_current();
    player_set_physics(p, ch->thrust, ch->gravity, ch->maxVelocity);
    player_set_hitbox(p, ch->hitbox_w, ch->hitbox_h, ch->hitbox_offset_x, ch->hitbox_offset_y);
    game_render_load_sprites(ch);
}

static void start_run(Player *p) {
    frame_count = 0;
    scoring_reset();
    obstacles_init();
    player_init(p);
    apply_character(p);
}

/* A degraded start is the difference between real art and placeholder boxes, so
   say what failed rather than leaving it a mystery on the TV. */
static void show_startup_warning(void) {
    for (int i = 0; i < 240; i++) {
        game_draw_background();
        ui_draw_border();
        ui_draw_centered_text(180, "STARTUP WARNING", 20, RGBA(255, 215, 0, 255));
        if (!magnolia_sd_mounted()) {
            ui_draw_centered_text(220, "SD card not mounted", 14, RGBA(255, 255, 255, 255));
            ui_draw_centered_text(245, "sprites/scores unavailable", 12, RGBA(160, 160, 160, 255));
        }
        if (!magnolia_fonts_loaded()) {
            ui_draw_centered_text(275, "font failed to load", 14, RGBA(255, 255, 255, 255));
        }
        renderer_finish();
        input_scan();
        if (input_start_pressed()) break;
    }
}

/* Grid navigation wraps on both axes: past the right edge moves to the next
   row, past the last cell wraps to the first. */
static void move_cursor(int dcol, int drow) {
    int count = characters_get_count();
    if (count <= 0) return;

    if (dcol) {
        char_cursor += dcol;
        if (char_cursor < 0) char_cursor = count - 1;
        if (char_cursor >= count) char_cursor = 0;
    }
    if (drow) {
        int next = char_cursor + drow * CHAR_GRID_COLS;
        if (next < 0) {
            /* Wrap to the same column on the last populated row. */
            int col = char_cursor % CHAR_GRID_COLS;
            next = ((count - 1) / CHAR_GRID_COLS) * CHAR_GRID_COLS + col;
            while (next >= count) next -= CHAR_GRID_COLS;
        } else if (next >= count) {
            next = char_cursor % CHAR_GRID_COLS;
        }
        char_cursor = next;
    }
}

/* Returns 1 once a character is chosen and the game should advance. */
static int handle_character_select(Player *player) {
    if (input_left_pressed())  move_cursor(-1, 0);
    if (input_right_pressed()) move_cursor(+1, 0);
    if (input_up_pressed())    move_cursor(0, -1);
    if (input_down_pressed())  move_cursor(0, +1);

    if (input_back_pressed()) {
        char_select_open = 0;
        return 0;
    }
    if (input_start_pressed()) {
        characters_set_current(char_cursor);
        apply_character(player);
        char_select_open = 0;
        return 1;
    }
    return 0;
}

static void update_playing(GameStateMachine *gs, Player *player) {
    frame_count++;

    if (player_update(player, input_thrust_pressed(), SCREEN_HEIGHT)) {
        gamestate_end_run(gs, scoring_get());
        return;
    }

    if (frame_count % OBSTACLE_SPAWN_INTERVAL == 0) {
        obstacles_create(CANVAS_WIDTH, CANVAS_HEIGHT);
    }
    obstacles_update(CANVAS_WIDTH);

    if (obstacles_check_collision((int)player->x + player->offsetX,
                                  (int)player->y + player->offsetY,
                                  player->width, player->height)) {
        gamestate_end_run(gs, scoring_get());
        return;
    }

    int score_inc = obstacles_check_score((int)player->x);
    for (int i = 0; i < score_inc; i++) scoring_increment();

    stars_update();
    game_draw_background();
    game_draw_stars();
    obstacle_draw_all();
    game_draw_player(player, input_thrust_pressed());
    game_draw_score(scoring_get());
    renderer_finish();
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    srand(time(NULL));

    const MagnoliaConfig cfg = {
        .app_name     = APP_NAME,
        .max_scores   = HIGH_SCORE_COUNT,
        .overscan_pct = OVERSCAN_PCT
    };
    int init_status = magnolia_init(&cfg);

    input_init();
    obstacles_init();
    stars_init();
    characters_init();
    ui_init();

    Player player;
    player_init(&player);
    apply_character(&player);

    GameStateMachine gs;
    gamestate_init(&gs);

    if (init_status != 0) show_startup_warning();

    while (1) {
        input_scan();
        if (input_home_pressed()) break;

        /* The engine owns every state except play; a return of 1 means it just
           entered GS_PLAYING and the world needs resetting. */
        if (gamestate_current(&gs) != GS_PLAYING) {
            switch (gamestate_current(&gs)) {
                case GS_TITLE:
                    stars_update();
                    if (char_select_open) {
                        ui_draw_character_select(char_cursor);
                    } else {
                        ui_draw_title(characters_get_current_index());
                    }
                    break;
                case GS_READY:
                    stars_update();
                    ui_draw_ready();
                    break;
                case GS_GAME_OVER:
                    ui_draw_game_over(scoring_get(), gs.is_high_score, gs.rank);
                    break;
                case GS_INITIALS:
                    ui_draw_initials(gs.cursor_pos, gs.selected_letter,
                                     gs.initials, scoring_get());
                    break;
                case GS_HIGH_SCORES:
                    ui_draw_high_scores();
                    break;
                default:
                    break;
            }

            /* While the grid is open the engine must not act on A, the same
               way the game withholds GS_PLAYING from it. */
            if (char_select_open) {
                if (handle_character_select(&player)) {
                    gamestate_set(&gs, GS_READY);
                }
            } else if (gamestate_current(&gs) == GS_TITLE && input_start_pressed()) {
                game_render_load_portraits();
                char_cursor = characters_get_current_index();
                char_select_open = 1;
            } else if (gamestate_update(&gs, scoring_get())) {
                start_run(&player);
            }
        } else {
            update_playing(&gs, &player);
        }
    }

    game_render_free_sprites();
    game_render_free_portraits();
    magnolia_shutdown();
    return 0;
}
