#include <grrlib.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"
#include "magnolia.h"
#include "config.h"
#include "characters.h"
#include "game_render.h"
#include "obstacle_renderer.h"

#define CYAN    RGBA(0, 212, 255, 255)
#define WHITE   RGBA(255, 255, 255, 255)
#define DIM     RGBA(160, 160, 160, 255)
#define YELLOW  RGBA(255, 215, 0, 255)

void ui_init(void) {
}

void ui_draw_title(int selected_char_index) {
    game_draw_background();
    game_draw_stars();
    ui_draw_border();

    ui_draw_centered_text(80, "MOONLIGHT", 40, CYAN);
    ui_draw_centered_text(130, "DRIFT", 40, CYAN);

    const CharacterData *ch = characters_get_by_index(selected_char_index);
    if (ch) {
        ui_draw_centered_text(220, ch->name, 16, WHITE);
    }

    ui_draw_centered_text(320, "Press A: choose character", 14, DIM);

    int sc = scoring_get_count();
    if (sc > 0) {
        ui_draw_centered_text(400, "High Scores", 12, DIM);
    }

    renderer_finish();
}

void ui_draw_character_select(int cursor_index) {
    game_draw_background();
    game_draw_stars();
    ui_draw_border();

    ui_draw_centered_text(24, "SELECT CHARACTER", 18, CYAN);

    const int cell_w = 96, cell_h = 62;
    const int grid_x = 32, grid_y = 62;
    int count = characters_get_count();

    for (int i = 0; i < count && i < CHAR_GRID_COLS * CHAR_GRID_ROWS; i++) {
        int col = i % CHAR_GRID_COLS;
        int row = i / CHAR_GRID_COLS;
        int cx = grid_x + col * cell_w;
        int cy = grid_y + row * cell_h;

        if (i == cursor_index) {
            GRRLIB_Rectangle(ui_map_x(cx), ui_map_y(cy),
                             ui_map_w(cell_w - 4), ui_map_h(cell_h - 4),
                             RGBA(0, 212, 255, 45), true);
            GRRLIB_Rectangle(ui_map_x(cx), ui_map_y(cy),
                             ui_map_w(cell_w - 4), ui_map_h(cell_h - 4),
                             CYAN, false);
        }

        /* Portraits are 128px art shown at ~52px. The sprite origin lands on
           the cell centre, so characters sit consistently regardless of how
           their art is positioned inside its canvas. */
        float px = ui_map_x(cx + (cell_w - 4) / 2);
        float py = ui_map_y(cy + (cell_h - 4) / 2);
        float scale = (float)ui_map_w(52) / 128.0f;

        if (game_portrait_valid(i)) {
            game_draw_portrait(i, px, py, scale);
        } else {
            /* No art: show the slot rather than an unexplained gap. */
            GRRLIB_Rectangle(px - ui_map_w(12), py - ui_map_h(12),
                             ui_map_w(24), ui_map_h(24), DIM, false);
        }
    }

    const CharacterData *ch = characters_get_by_index(cursor_index);
    if (ch) {
        ui_draw_centered_text(322, ch->name, 16, WHITE);

        char stats[64];
        snprintf(stats, sizeof(stats), "HITBOX %dx%d", ch->hitbox_w, ch->hitbox_h);
        ui_draw_centered_text(350, stats, 12, DIM);

        /* No %f: printing floats pulls in a much larger libc formatter, and
           this is the only place the game would need it. */
        int thr = (int)(-ch->thrust * 100.0f + 0.5f);
        int grv = (int)(ch->gravity * 100.0f + 0.5f);
        snprintf(stats, sizeof(stats), "THRUST 0.%02d   GRAVITY 0.%02d", thr, grv);
        ui_draw_centered_text(372, stats, 12, DIM);
    }

    ui_draw_centered_text(408, "D-Pad: move   A: select   B: back", 12, DIM);

    renderer_finish();
}

void ui_draw_ready(void) {
    game_draw_background();
    game_draw_stars();
    ui_draw_border();

    ui_draw_centered_text(180, "HOW TO PLAY", 24, CYAN);
    ui_draw_centered_text(230, "Hold A to thrust", 16, WHITE);
    ui_draw_centered_text(260, "Release to fall", 16, WHITE);
    ui_draw_centered_text(290, "Dodge obstacles!", 16, WHITE);
    ui_draw_centered_text(360, "Press A to start", 16, DIM);

    renderer_finish();
}

void ui_draw_game_over(int score, int is_high_score, int rank) {
    game_draw_background();
    game_draw_stars();
    ui_draw_border();

    ui_draw_centered_text(120, "GAME OVER", 36, WHITE);

    char score_buf[32];
    snprintf(score_buf, sizeof(score_buf), "Score: %d", score);
    ui_draw_centered_text(190, score_buf, 20, CYAN);

    if (is_high_score) {
        char rank_buf[32];
        snprintf(rank_buf, sizeof(rank_buf), "Rank: #%d", rank);
        ui_draw_centered_text(230, rank_buf, 16, YELLOW);
        ui_draw_centered_text(270, "NEW HIGH SCORE!", 18, YELLOW);
        ui_draw_centered_text(310, "Press A to enter initials", 14, DIM);
    } else {
        ui_draw_centered_text(270, "Press A to retry", 16, DIM);
    }

    renderer_finish();
}

void ui_draw_initials(int cursor_pos, int selected_letter, const char *initials, int score) {
    game_draw_background();
    game_draw_stars();
    ui_draw_border();

    ui_draw_centered_text(100, "ENTER YOUR INITIALS", 20, CYAN);

    char score_buf[32];
    snprintf(score_buf, sizeof(score_buf), "Score: %d", score);
    ui_draw_centered_text(140, score_buf, 16, WHITE);

    /* Design-space geometry; ui_map_* projects it into the TV-safe area. */
    int box_w = 120;
    int box_x = (UI_DESIGN_WIDTH - box_w) / 2;
    int box_y = 190;
    GRRLIB_Rectangle(ui_map_x(box_x - 4), ui_map_y(box_y - 4),
                     ui_map_w(box_w + 8), ui_map_h(50), CYAN, false);
    GRRLIB_Rectangle(ui_map_x(box_x), ui_map_y(box_y),
                     ui_map_w(box_w), ui_map_h(42), RGBA(20, 20, 40, 255), true);

    for (int i = 0; i < 3; i++) {
        int lx = box_x + 15 + i * 35;
        char ch[2] = { initials[i], '\0' };

        if (i == cursor_pos) {
            GRRLIB_Rectangle(ui_map_x(lx - 4), ui_map_y(box_y + 4),
                             ui_map_w(30), ui_map_h(34), RGBA(0, 212, 255, 60), true);
            ui_draw_text_shadow(lx + 2, box_y + 8, ch, 24, CYAN);
            char sel[2] = { 'A' + selected_letter, '\0' };
            ui_draw_text_shadow(lx + 2, box_y - 20, sel, 14, YELLOW);
        } else {
            ui_draw_text_shadow(lx + 2, box_y + 8, ch, 24, WHITE);
        }
    }

    ui_draw_centered_text(300, "D-Pad L/R: letter", 12, DIM);
    ui_draw_centered_text(320, "D-Pad Down: next", 12, DIM);
    ui_draw_centered_text(340, "A or B: save", 12, DIM);

    renderer_finish();
}

void ui_draw_high_scores(void) {
    game_draw_background();
    game_draw_stars();
    ui_draw_border();

    ui_draw_centered_text(40, "HIGH SCORES", 24, CYAN);

    int count = scoring_get_count();
    int max_show = count < 10 ? count : 10;

    for (int i = 0; i < max_show; i++) {
        const ScoreEntry *e = scoring_get_entry(i);
        if (!e) break;

        char line[48];
        snprintf(line, sizeof(line), "%2d.  %s   %d", i + 1, e->initials, e->score);
        u32 col = (i == 0) ? YELLOW : WHITE;
        ui_draw_centered_text(90 + i * 30, line, 16, col);
    }

    if (count == 0) {
        ui_draw_centered_text(180, "No scores yet", 16, DIM);
    }

    ui_draw_centered_text(430, "Press A to return", 14, DIM);

    renderer_finish();
}
