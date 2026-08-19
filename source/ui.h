#ifndef UI_H
#define UI_H

#include <grrlib.h>
#include "characters.h"
#include "player.h"

void ui_init(void);
void ui_draw_title(int selected_char_index);
void ui_draw_ready(void);
void ui_draw_game_over(int score, int is_high_score, int rank);
void ui_draw_initials(int cursor_pos, int selected_letter, const char *initials, int score);
void ui_draw_high_scores(void);

#endif
