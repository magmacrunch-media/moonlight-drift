#include <grrlib.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"
#include "renderer.h"
#include "config.h"
#include "characters.h"
#include "scoring.h"

#define CYAN    RGBA(0, 212, 255, 255)
#define WHITE   RGBA(255, 255, 255, 255)
#define DIM     RGBA(160, 160, 160, 255)
#define YELLOW  RGBA(255, 215, 0, 255)
#define SHADOW  RGBA(0, 0, 0, 255)

extern GRRLIB_ttfFont *ttf_font;

static void draw_text_shadow(int x, int y, const char *text, unsigned int size, u32 color) {
    GRRLIB_PrintfTTF(x + 2, y + 2, ttf_font, text, size, SHADOW);
    GRRLIB_PrintfTTF(x, y, ttf_font, text, size, color);
}

static void draw_centered_text(int y, const char *text, unsigned int size, u32 color) {
    u32 w = GRRLIB_WidthTTF(ttf_font, text, size);
    int x = (SCREEN_WIDTH - (int)w) / 2;
    if (x < 0) x = 0;
    draw_text_shadow(x, y, text, size, color);
}

void ui_init(void) {
}

void ui_draw_border(void) {
    u32 c = CYAN;
    int t = 3;
    GRRLIB_Rectangle(0, 0, SCREEN_WIDTH, t, c, true);
    GRRLIB_Rectangle(0, SCREEN_HEIGHT - t, SCREEN_WIDTH, t, c, true);
    GRRLIB_Rectangle(0, 0, t, SCREEN_HEIGHT, c, true);
    GRRLIB_Rectangle(SCREEN_WIDTH - t, 0, t, SCREEN_HEIGHT, c, true);
}

void ui_draw_title(int selected_char_index) {
    renderer_draw_background();
    renderer_draw_stars();
    ui_draw_border();

    draw_centered_text(80, "MOONLIGHT", 40, CYAN);
    draw_centered_text(130, "DRIFT", 40, CYAN);

    const CharacterData *ch = characters_get_by_index(selected_char_index);
    if (ch) {
        draw_centered_text(220, ch->name, 16, WHITE);
    }

    draw_centered_text(320, "D-Pad: choose", 14, DIM);
    draw_centered_text(345, "Press A: start", 14, DIM);

    int sc = scoring_get_count();
    if (sc > 0) {
        draw_centered_text(400, "High Scores", 12, DIM);
    }

    renderer_finish();
}

void ui_draw_ready(void) {
    renderer_draw_background();
    renderer_draw_stars();
    ui_draw_border();

    draw_centered_text(180, "HOW TO PLAY", 24, CYAN);
    draw_centered_text(230, "Hold A to thrust", 16, WHITE);
    draw_centered_text(260, "Release to fall", 16, WHITE);
    draw_centered_text(290, "Dodge obstacles!", 16, WHITE);
    draw_centered_text(360, "Press A to start", 16, DIM);

    renderer_finish();
}

void ui_draw_game_over(int score, int is_high_score, int rank) {
    renderer_draw_background();
    renderer_draw_stars();
    ui_draw_border();

    draw_centered_text(120, "GAME OVER", 36, WHITE);

    char score_buf[32];
    snprintf(score_buf, sizeof(score_buf), "Score: %d", score);
    draw_centered_text(190, score_buf, 20, CYAN);

    if (is_high_score) {
        char rank_buf[32];
        snprintf(rank_buf, sizeof(rank_buf), "Rank: #%d", rank);
        draw_centered_text(230, rank_buf, 16, YELLOW);
        draw_centered_text(270, "NEW HIGH SCORE!", 18, YELLOW);
        draw_centered_text(310, "Press A to enter initials", 14, DIM);
    } else {
        draw_centered_text(270, "Press A to retry", 16, DIM);
    }

    renderer_finish();
}

void ui_draw_initials(int cursor_pos, int selected_letter, const char *initials, int score) {
    renderer_draw_background();
    renderer_draw_stars();
    ui_draw_border();

    draw_centered_text(100, "ENTER YOUR INITIALS", 20, CYAN);

    char score_buf[32];
    snprintf(score_buf, sizeof(score_buf), "Score: %d", score);
    draw_centered_text(140, score_buf, 16, WHITE);

    int box_w = 120;
    int box_x = (SCREEN_WIDTH - box_w) / 2;
    int box_y = 190;
    GRRLIB_Rectangle(box_x - 4, box_y - 4, box_w + 8, 50, CYAN, false);
    GRRLIB_Rectangle(box_x, box_y, box_w, 42, RGBA(20, 20, 40, 255), true);

    for (int i = 0; i < 3; i++) {
        int lx = box_x + 15 + i * 35;
        char ch[2] = { initials[i], '\0' };

        if (i == cursor_pos) {
            GRRLIB_Rectangle(lx - 4, box_y + 4, 30, 34, RGBA(0, 212, 255, 60), true);
            draw_text_shadow(lx + 2, box_y + 8, ch, 24, CYAN);
            char sel[2] = { 'A' + selected_letter, '\0' };
            draw_text_shadow(lx + 2, box_y - 20, sel, 14, YELLOW);
        } else {
            draw_text_shadow(lx + 2, box_y + 8, ch, 24, WHITE);
        }
    }

    draw_centered_text(300, "D-Pad: change letter", 12, DIM);
    draw_centered_text(320, "A: next position", 12, DIM);
    draw_centered_text(340, "B: confirm & save", 12, DIM);

    renderer_finish();
}

void ui_draw_high_scores(void) {
    renderer_draw_background();
    renderer_draw_stars();
    ui_draw_border();

    draw_centered_text(40, "HIGH SCORES", 24, CYAN);

    int count = scoring_get_count();
    int max_show = count < 10 ? count : 10;

    for (int i = 0; i < max_show; i++) {
        const ScoreEntry *e = scoring_get_entry(i);
        if (!e) break;

        char line[48];
        snprintf(line, sizeof(line), "%2d.  %s   %d", i + 1, e->initials, e->score);
        u32 col = (i == 0) ? YELLOW : WHITE;
        draw_centered_text(90 + i * 30, line, 16, col);
    }

    if (count == 0) {
        draw_centered_text(180, "No scores yet", 16, DIM);
    }

    draw_centered_text(430, "Press A to return", 14, DIM);

    renderer_finish();
}
