#ifndef SETTINGS_H
#define SETTINGS_H

/* Player preferences persisted next to the high scores, so a chosen character
   and a muted soundtrack survive a power cycle.

   The storage moved into magnolia once George Boole needed the same thing; these
   names stay here because which preferences a game has, and what counts as a
   valid value for one, is the game's business. */

void settings_load(void);
void settings_save(void);

int  settings_get_character(void);
void settings_set_character(int index);

int  settings_get_muted(void);
void settings_set_muted(int muted);

#endif
