#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "magnolia.h"
#include "characters.h"
#include "settings.h"

#define SETTINGS_FILE "settings.json"

static int sel_character = 0;
static int muted = 0;

/* Deliberately the same shape as the scores file: a tiny hand-parsed JSON
   object, readable from a PC if someone wants to inspect their SD card. */
void settings_load(void) {
    FILE *f = fopen(magnolia_asset_path(SETTINGS_FILE), "r");
    if (!f) return;

    char buf[192];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';

    const char *p = strstr(buf, "\"character\":");
    if (p) {
        int v = atoi(p + 12);
        if (v >= 0 && v < characters_get_count()) sel_character = v;
    }
    p = strstr(buf, "\"muted\":");
    if (p) muted = atoi(p + 8) ? 1 : 0;
}

void settings_save(void) {
    FILE *f = fopen(magnolia_asset_path(SETTINGS_FILE), "w");
    if (!f) return;
    fprintf(f, "{\"character\":%d,\"muted\":%d}", sel_character, muted);
    fclose(f);
}

int  settings_get_character(void)       { return sel_character; }
void settings_set_character(int index)  { sel_character = index; settings_save(); }
int  settings_get_muted(void)           { return muted; }
void settings_set_muted(int m)          { muted = m ? 1 : 0; settings_save(); }
