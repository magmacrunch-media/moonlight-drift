#include "magnolia.h"
#include "characters.h"
#include "settings.h"

/* Thin naming layer over the engine's preference store. What used to be here --
   a hand-rolled reader and writer for settings.json -- moved into magnolia when
   a second game needed the same thing, which is the rule this file's header
   asked for. What stays is the part that is actually this game's: which
   preferences exist, and that a character index is only valid against the
   current roster. */

#define KEY_CHARACTER "character"
#define KEY_MUTED     "muted"

void settings_load(void) {
    /* prefs are loaded by magnolia_init(); nothing to do but validate. */
    int v = prefs_get_int(KEY_CHARACTER, 0);
    if (v < 0 || v >= characters_get_count()) {
        prefs_set_int(KEY_CHARACTER, 0);
    }
}

void settings_save(void) {
    prefs_save();
}

int settings_get_character(void) {
    int v = prefs_get_int(KEY_CHARACTER, 0);
    return (v >= 0 && v < characters_get_count()) ? v : 0;
}

void settings_set_character(int index) {
    prefs_set_int(KEY_CHARACTER, index);
}

int settings_get_muted(void) {
    return prefs_get_int(KEY_MUTED, 0) ? 1 : 0;
}

void settings_set_muted(int m) {
    prefs_set_int(KEY_MUTED, m ? 1 : 0);
}
