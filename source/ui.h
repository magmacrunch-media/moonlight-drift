#ifndef UI_H
#define UI_H

#include <grrlib.h>
#include "characters.h"
#include "player.h"

void ui_init(void);
void ui_draw_title(int selected_char_index, int muted);

/* 6x4 grid of every character, cursor on `cursor_index`. Laid out in the
   640x480 design space; ui_map_* projects it into the TV-safe area. */
void ui_draw_character_select(int cursor_index);

#define CHAR_GRID_COLS 6
#define CHAR_GRID_ROWS 4
void ui_draw_ready(void);
void ui_draw_game_over(int score, int is_high_score, int rank);
void ui_draw_initials(int cursor_pos, int selected_letter, const char *initials, int score);
void ui_draw_high_scores(void);
void ui_draw_credits(void);

#endif
