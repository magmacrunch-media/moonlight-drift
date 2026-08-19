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

static void handle_character_select(Player *player) {
    if (input_left_pressed()) {
        int idx = characters_get_current_index() - 1;
        if (idx < 0) idx = characters_get_count() - 1;
        characters_set_current(idx);
        apply_character(player);
    }
    if (input_right_pressed()) {
        int idx = characters_get_current_index() + 1;
        if (idx >= characters_get_count()) idx = 0;
        characters_set_current(idx);
        apply_character(player);
    }
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
                    ui_draw_title(characters_get_current_index());
                    handle_character_select(&player);
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

            if (gamestate_update(&gs, scoring_get())) {
                start_run(&player);
            }
        } else {
            update_playing(&gs, &player);
        }
    }

    game_render_free_sprites();
    magnolia_shutdown();
    return 0;
}
