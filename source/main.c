#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "magnolia.h"
#include "config.h"
#include "obstacles.h"
#include "obstacle_renderer.h"
#include "ui.h"

typedef enum {
    STATE_TITLE,
    STATE_READY,
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_INITIALS,
    STATE_HIGH_SCORES
} GameState;

static GameState state = STATE_TITLE;
static int frame_count = 0;
static int game_running = 0;

static int is_high = 0;
static int rank = 0;

static int cursor_pos = 0;
static int selected_letter = 0;
static char initials[4] = "AAA";

static void apply_character(Player *p) {
    const CharacterData *ch = characters_get_current();
    player_set_physics(p, ch->thrust, ch->gravity, ch->maxVelocity);
    player_set_hitbox(p, ch->hitbox_w, ch->hitbox_h, ch->hitbox_offset_x, ch->hitbox_offset_y);
    renderer_load_sprites(ch);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    srand(time(NULL));

    int init_status = renderer_init();
    input_init();
    obstacles_init();
    stars_init();
    scoring_init();
    characters_init();
    ui_init();

    Player player;
    player_init(&player);
    apply_character(&player);

    /* A missing SD mount is the difference between real character art and white
       placeholder boxes, so say so on screen rather than leaving it a mystery. */
    if (init_status != 0) {
        for (int i = 0; i < 240; i++) {
            renderer_draw_background();
            ui_draw_border();
            ui_draw_centered_text(180, "STARTUP WARNING", 20, RGBA(255, 215, 0, 255));
            if (!renderer_sd_mounted()) {
                ui_draw_centered_text(220, "SD card not mounted", 14, RGBA(255, 255, 255, 255));
                ui_draw_centered_text(245, "sprites/scores unavailable", 12, RGBA(160, 160, 160, 255));
            }
            if (!renderer_fonts_loaded()) {
                ui_draw_centered_text(275, "font failed to load", 14, RGBA(255, 255, 255, 255));
            }
            renderer_finish();
            input_scan();
            if (input_start_pressed()) break;
        }
    }

    while (1) {
        input_scan();

        if (input_home_pressed()) break;

        switch (state) {
            case STATE_TITLE:
                stars_update();
                ui_draw_title(characters_get_current_index());

                if (input_left_pressed()) {
                    int idx = characters_get_current_index() - 1;
                    if (idx < 0) idx = characters_get_count() - 1;
                    characters_set_current(idx);
                    apply_character(&player);
                }
                if (input_right_pressed()) {
                    int idx = characters_get_current_index() + 1;
                    if (idx >= characters_get_count()) idx = 0;
                    characters_set_current(idx);
                    apply_character(&player);
                }
                if (input_start_pressed()) {
                    state = STATE_READY;
                }
                break;

            case STATE_READY:
                stars_update();
                ui_draw_ready();
                if (input_start_pressed()) {
                    state = STATE_PLAYING;
                    game_running = 1;
                    frame_count = 0;
                    scoring_reset();
                    obstacles_init();
                    player_init(&player);
                    apply_character(&player);
                }
                break;

            case STATE_PLAYING:
                frame_count++;

                int boundary_hit = player_update(&player, input_thrust_pressed(), SCREEN_HEIGHT);
                if (boundary_hit) {
                    is_high = scoring_is_high_score(scoring_get());
                    rank = is_high ? scoring_get_rank(scoring_get()) : 0;
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
                    is_high = scoring_is_high_score(scoring_get());
                    rank = is_high ? scoring_get_rank(scoring_get()) : 0;
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
                obstacle_draw_all();
                renderer_draw_player(&player, input_thrust_pressed());
                renderer_draw_score(scoring_get());
                renderer_finish();
                break;

            case STATE_GAME_OVER:
                ui_draw_game_over(scoring_get(), is_high, rank);

                if (input_start_pressed()) {
                    if (is_high) {
                        cursor_pos = 0;
                        selected_letter = 0;
                        initials[0] = 'A';
                        initials[1] = 'A';
                        initials[2] = 'A';
                        state = STATE_INITIALS;
                    } else {
                        state = STATE_PLAYING;
                        game_running = 1;
                        frame_count = 0;
                        scoring_reset();
                        obstacles_init();
                        player_init(&player);
                        apply_character(&player);
                    }
                }
                break;

            case STATE_INITIALS:
                ui_draw_initials(cursor_pos, selected_letter, initials, scoring_get());

                if (input_left_pressed()) {
                    selected_letter--;
                    if (selected_letter < 0) selected_letter = 25;
                }
                if (input_right_pressed()) {
                    selected_letter++;
                    if (selected_letter > 25) selected_letter = 0;
                }

                if (input_up_pressed()) {
                    initials[cursor_pos] = 'A' + selected_letter;
                }
                if (input_down_pressed()) {
                    initials[cursor_pos] = 'A' + selected_letter;
                    if (cursor_pos < 2) {
                        cursor_pos++;
                        selected_letter = initials[cursor_pos] - 'A';
                        if (selected_letter < 0 || selected_letter > 25) selected_letter = 0;
                    }
                }

                if (input_start_pressed()) {
                    initials[cursor_pos] = 'A' + selected_letter;
                    scoring_add_entry(initials, scoring_get());
                    state = STATE_HIGH_SCORES;
                }

                if (input_back_pressed()) {
                    initials[cursor_pos] = 'A' + selected_letter;
                    scoring_add_entry(initials, scoring_get());
                    state = STATE_HIGH_SCORES;
                }
                break;

            case STATE_HIGH_SCORES:
                ui_draw_high_scores();
                if (input_start_pressed()) {
                    state = STATE_TITLE;
                }
                break;
        }

    }

    renderer_shutdown();
    return 0;
}
