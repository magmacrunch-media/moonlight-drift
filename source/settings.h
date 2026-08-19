#ifndef SETTINGS_H
#define SETTINGS_H

/* Player preferences persisted next to the high scores, so a chosen character
   and a muted soundtrack survive a power cycle.
   Kept in the game rather than magnolia: it has exactly one consumer, and the
   engine grows an abstraction when a second game needs it. */

void settings_load(void);
void settings_save(void);

int  settings_get_character(void);
void settings_set_character(int index);

int  settings_get_muted(void);
void settings_set_muted(int muted);

#endif
