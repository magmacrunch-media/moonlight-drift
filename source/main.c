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
#include "settings.h"
#include "ui.h"
#include "assets.h"

/* SFX slots, in load order. */
#define SFX_CRASH   0
#define SFX_BUTTON1 1
#define SFX_BUTTON2 2

static int frame_count = 0;

/* The selector is a sub-mode of the title screen rather than an engine state:
   character select is this game's concern, and magnolia only grows an
   abstraction once a second game needs it. */
static int char_select_open = 0;
static int char_cursor = 0;
static int char_top_row = 0;   /* first visible grid row */
static int credits_open = 0;

static void apply_character(Player *p) {
    const CharacterData *ch = characters_get_current();
    printf("apply_character: idx=%d name=%s\n",
           characters_get_current_index(), ch ? ch->name : "(null)");
    player_set_physics(p, ch->thrust, ch->gravity, ch->maxVelocity);
    player_set_hitbox(p, ch->hitbox_w, ch->hitbox_h, ch->hitbox_offset_x, ch->hitbox_offset_y);
    game_render_load_sprites(ch);
}

/* Traced step by step: the crash lands somewhere between selecting a character
   and the first drawn frame, and printf reaches Dolphin's log via OSReport. */
static void start_run(Player *p) {
    printf("start_run: begin\n");
    frame_count = 0;
    shooting_stars_reset();      printf("start_run: shooting stars reset\n");
    scoring_reset();             printf("start_run: scoring reset\n");
    obstacles_init();            printf("start_run: obstacles init\n");
    player_init(p);              printf("start_run: player init\n");
    apply_character(p);          printf("start_run: character applied\n");
    printf("start_run: done\n");
}

/* A degraded start is the difference between real art and placeholder boxes.
   Report every subsystem and wait for input rather than timing out, so one
   photo of the screen explains the whole boot. */
static void show_startup_report(void) {
    const u32 WHITE_C = RGBA(255, 255, 255, 255);
    const u32 GOOD    = RGBA(46, 204, 113, 255);
    const u32 BAD     = RGBA(255, 90, 90, 255);
    const u32 DIM_C   = RGBA(160, 160, 160, 255);

    int sd    = magnolia_sd_mounted();
    int font  = magnolia_fonts_loaded();
    int art   = game_render_sprites_loaded();
    int sound = audio_music_loaded();

    /* Waits for A so the report can be read, but never blocks forever: if no
       Wiimote is connected -- easy to hit in an emulator -- an input-only exit
       would look exactly like a hang. */
    const int timeout_frames = 900;   /* ~15s at 60fps */
    for (int f = 0; f < timeout_frames; f++) {
        game_draw_background();
        ui_draw_border();
        ui_draw_centered_text(60, "STARTUP REPORT", 20, RGBA(255, 215, 0, 255));

        ui_draw_centered_text(120, sd    ? "SD CARD: mounted"    : "SD CARD: NOT MOUNTED", 13, sd    ? GOOD : BAD);
        ui_draw_centered_text(150, font  ? "FONT: loaded"        : "FONT: FAILED",         13, font  ? GOOD : BAD);
        ui_draw_centered_text(180, art   ? "SPRITES: loaded"     : "SPRITES: NOT FOUND",   13, art   ? GOOD : BAD);
        ui_draw_centered_text(210, sound ? "AUDIO: loaded"       : "AUDIO: NOT FOUND",     13, sound ? GOOD : BAD);

        if (!sd) {
            ui_draw_centered_text(262, "Sprites and audio are built into", 11, DIM_C);
            ui_draw_centered_text(280, "the binary, so play is unaffected.", 11, DIM_C);
            ui_draw_centered_text(310, "Without a card, high scores and", 11, DIM_C);
            ui_draw_centered_text(328, "settings will not be saved.", 11, DIM_C);
        }

        ui_draw_centered_text(390, "The game plays normally.", 11, DIM_C);
        char cont[40];
        snprintf(cont, sizeof(cont), "Press A to continue  (%ds)",
                 (timeout_frames - f) / 60);
        ui_draw_centered_text(420, cont, 12, WHITE_C);
        renderer_finish();

        input_scan();
        if (input_start_pressed() || input_home_pressed()) break;
    }
}

/* Grid navigation wraps on both axes, and the visible window follows the
   cursor rather than the cursor being trapped inside the window. */
static void move_cursor(int dcol, int drow) {
    int count = characters_get_count();
    if (count <= 0) return;
    int total_rows = (count + CHAR_GRID_COLS - 1) / CHAR_GRID_COLS;

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
            next = (total_rows - 1) * CHAR_GRID_COLS + col;
            while (next >= count) next -= CHAR_GRID_COLS;
        } else if (next >= count) {
            next = char_cursor % CHAR_GRID_COLS;
        }
        char_cursor = next;
    }

    /* Scroll the window the minimum needed to keep the cursor on screen. */
    int row = char_cursor / CHAR_GRID_COLS;
    if (row < char_top_row) char_top_row = row;
    if (row >= char_top_row + CHAR_GRID_ROWS) char_top_row = row - CHAR_GRID_ROWS + 1;

    int max_top = total_rows - CHAR_GRID_ROWS;
    if (max_top < 0) max_top = 0;
    if (char_top_row > max_top) char_top_row = max_top;
    if (char_top_row < 0) char_top_row = 0;
}

/* Returns 1 once a character is chosen and the game should advance. */
static int handle_character_select(Player *player) {
    int moved = input_left_pressed() || input_right_pressed()
             || input_up_pressed()   || input_down_pressed();
    if (input_left_pressed())  move_cursor(-1, 0);
    if (input_right_pressed()) move_cursor(+1, 0);
    if (input_up_pressed())    move_cursor(0, -1);
    if (input_down_pressed())  move_cursor(0, +1);
    if (moved) audio_play_sfx(SFX_BUTTON1);

    if (input_back_pressed()) {
        char_select_open = 0;
        return 0;
    }
    if (input_start_pressed()) {
        printf("select: choosing %d\n", char_cursor);
        characters_set_current(char_cursor);
        printf("select: saving settings\n");
        settings_set_character(char_cursor);
        printf("select: applying character\n");
        apply_character(player);
        printf("select: sfx\n");
        audio_play_sfx(SFX_BUTTON2);
        printf("select: done\n");
        char_select_open = 0;
        return 1;
    }
    return 0;
}

static void update_playing(GameStateMachine *gs, Player *player) {
    frame_count++;
    if (frame_count == 1) printf("update_playing: first frame entered\n");

    if (player_update(player, input_thrust_pressed(), SCREEN_HEIGHT)) {
#if DEBUG_IMMORTAL
        player->y = PLAYER_Y_INITIAL;
        player->velocity = 0.0f;
#else
        audio_play_sfx(SFX_CRASH);
        gamestate_end_run(gs, scoring_get());
        return;
#endif
    }

    if (frame_count % OBSTACLE_SPAWN_INTERVAL == 0) {
        obstacles_create(CANVAS_WIDTH, CANVAS_HEIGHT);
    }
    obstacles_update(CANVAS_WIDTH);

    if (obstacles_check_collision((int)player->x + player->offsetX,
                                  (int)player->y + player->offsetY,
                                  player->width, player->height)) {
#if !DEBUG_IMMORTAL
        audio_play_sfx(SFX_CRASH);
        gamestate_end_run(gs, scoring_get());
        return;
#endif
    }

    int score_inc = obstacles_check_score((int)player->x);
    for (int i = 0; i < score_inc; i++) scoring_increment();

    stars_update();
    shooting_stars_update(CANVAS_WIDTH, CANVAS_HEIGHT);

    game_draw_background();
    game_draw_stars();
    obstacle_draw_all();
    game_draw_player(player, input_thrust_pressed());
    game_draw_score(scoring_get());
    renderer_finish();

#if DEBUG_HEARTBEAT_FRAMES
    if (frame_count % DEBUG_HEARTBEAT_FRAMES == 0) {
        printf("heartbeat: frame=%d obstacles=%d score=%d\n",
               frame_count, obstacles_get_count(), scoring_get());
    }
#endif
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
    /* Sends printf to the OSReport channel, which Dolphin captures in its log.
       Unbuffered, or the line written immediately before a crash -- the one that
       matters most -- dies in the buffer with it. */
    SYS_STDIO_Report(true);
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("=== moonlight drift starting ===\n");

    int init_status = magnolia_init(&cfg);
    /* -2 means video never came up; every draw call below would be undefined. */
    if (init_status == -2) return 1;

    input_init();
    obstacles_init();
    stars_init();
    characters_init();
    ui_init();

    audio_init();
    audio_load_sfx_mem(SFX_CRASH,   crash_pcm,   crash_pcm_size);
    audio_load_sfx_mem(SFX_BUTTON1, button1_pcm, button1_pcm_size);
    audio_load_sfx_mem(SFX_BUTTON2, button2_pcm, button2_pcm_size);

    /* Restore the player's last character and mute preference before anything
       reads them, so the title screen comes up in the state they left it. */
    settings_load();
    audio_set_muted(settings_get_muted());
    characters_set_current(settings_get_character());

    audio_play_music_mem(music_pcm, music_pcm_size);

    Player player;
    player_init(&player);
    apply_character(&player);

    GameStateMachine gs;
    gamestate_init(&gs);

#if AUTOSTART_GAMEPLAY
    printf("autostart: skipping menus, entering gameplay directly\n");
    start_run(&player);
    gamestate_set(&gs, GS_PLAYING);
#endif

    if (!game_render_sprites_loaded() || !magnolia_fonts_loaded()) show_startup_report();

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
                        ui_draw_character_select(char_cursor, char_top_row);
                    } else if (credits_open) {
                        ui_draw_credits();
                    } else {
                        ui_draw_title(audio_get_muted());
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
            } else if (credits_open) {
                if (input_back_pressed() || input_start_pressed()) {
                    credits_open = 0;
                    audio_play_sfx(SFX_BUTTON2);
                }
            } else if (gamestate_current(&gs) == GS_TITLE && input_button1_pressed()) {
                int m = !audio_get_muted();
                audio_set_muted(m);
                settings_set_muted(m);
                if (!m) audio_play_sfx(SFX_BUTTON2);
            } else if (gamestate_current(&gs) == GS_TITLE && input_button2_pressed()) {
                credits_open = 1;
                audio_play_sfx(SFX_BUTTON2);
            } else if (gamestate_current(&gs) == GS_TITLE && input_start_pressed()) {
                game_render_load_portraits();
                char_cursor = characters_get_current_index();
                char_top_row = 0;
                move_cursor(0, 0);   /* scrolls the window onto the cursor */
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
    audio_shutdown();
    magnolia_shutdown();
    return 0;
}
