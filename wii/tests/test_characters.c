/* The character roster.
 *
 * This file is data, and data rots quietly: a hitbox typed as 0, a thrust that
 * lost its minus sign, a name left empty when a character was copied from the
 * one above it. None of that crashes. It just produces one character that plays
 * wrong, in a roster of two dozen, which nobody notices until they happen to
 * pick it.
 *
 * The sprite pointers are bin2s symbols that only exist after a console build,
 * so the Makefile generates a stub for them from characters.c itself.
 *
 *   make test-characters
 */
#include <stdio.h>
#include <string.h>
#include "harness.h"
#include "characters.h"

static void test_roster_is_populated(void) {
    printf("characters: the roster exists\n");

    characters_init();
    int n = characters_get_count();

    check(n > 0, "the roster is not empty");
    printf("  %d characters\n", n);

    /* The README and the selector grid both assume a full roster; a truncated
       one would still boot and would still play. */
    check(n >= 24, "the full roster is present, not a truncated one");
}

static void test_every_entry_is_well_formed(void) {
    printf("characters: every entry is usable\n");

    characters_init();
    int n = characters_get_count();

    int bad_id = 0, bad_name = 0, bad_hitbox = 0, bad_origin = 0;
    int bad_thrust = 0, bad_gravity = 0, bad_maxvel = 0, bad_sprite = 0;
    char first[128] = "";

    for (int i = 0; i < n; i++) {
        const CharacterData *c = characters_get_by_index(i);
        if (!c) { bad_id++; continue; }

        if (!c->id || !*c->id) bad_id++;
        if (!c->name || !*c->name) bad_name++;
        if (!c->sprite_idle || !c->sprite_thrust) bad_sprite++;

        /* A zero-width or zero-height hitbox can never collide -- that
           character would be invincible. */
        if (c->hitbox_w <= 0 || c->hitbox_h <= 0) {
            if (!bad_hitbox++ && !first[0]) {
                snprintf(first, sizeof(first), "%s has hitbox %dx%d",
                         c->id ? c->id : "(no id)", c->hitbox_w, c->hitbox_h);
            }
        }

        /* Origins are measured inside a 128x128 export. */
        if (c->sprite_origin_x < 0 || c->sprite_origin_x > 128 ||
            c->sprite_origin_y < 0 || c->sprite_origin_y > 128) bad_origin++;

        /* The signs are the physics. Thrust lifts, gravity pulls, and a
           character with them the same way round only ever falls. */
        if (!(c->thrust < 0.0f)) {
            if (!bad_thrust++ && !first[0]) {
                snprintf(first, sizeof(first), "%s has thrust %f, expected negative",
                         c->id ? c->id : "(no id)", (double)c->thrust);
            }
        }
        if (!(c->gravity > 0.0f)) bad_gravity++;
        if (!(c->maxVelocity > 0.0f)) bad_maxvel++;
    }

    if (first[0]) printf("  first problem: %s\n", first);

    check_int(bad_id, 0, "every character has an id");
    check_int(bad_name, 0, "every character has a display name");
    check_int(bad_sprite, 0, "every character has both sprite frames");
    check_int(bad_hitbox, 0, "every hitbox has positive area");
    check_int(bad_origin, 0, "every sprite origin sits inside its 128x128 export");
    check_int(bad_thrust, 0, "every thrust lifts");
    check_int(bad_gravity, 0, "every gravity pulls down");
    check_int(bad_maxvel, 0, "every terminal velocity is positive");
}

static void test_ids_are_unique(void) {
    printf("characters: ids and names are distinct\n");

    characters_init();
    int n = characters_get_count();

    int dup_id = 0, dup_name = 0;
    char first[128] = "";

    for (int i = 0; i < n; i++) {
        const CharacterData *a = characters_get_by_index(i);
        if (!a || !a->id) continue;

        for (int j = i + 1; j < n; j++) {
            const CharacterData *b = characters_get_by_index(j);
            if (!b || !b->id) continue;

            if (strcmp(a->id, b->id) == 0) {
                if (!dup_id++ && !first[0]) {
                    snprintf(first, sizeof(first), "id \"%s\" appears at %d and %d",
                             a->id, i, j);
                }
            }
            if (a->name && b->name && strcmp(a->name, b->name) == 0) dup_name++;
        }
    }

    if (first[0]) printf("  %s\n", first);
    check_int(dup_id, 0, "no id is used twice");
    check_int(dup_name, 0, "no display name is used twice");
}

static void test_selection(void) {
    printf("characters: selecting one\n");

    characters_init();
    int n = characters_get_count();

    check(characters_get_current() != NULL, "there is always a current character");

    characters_set_current(3);
    check_int(characters_get_current_index(), 3, "selecting by index works");
    const CharacterData *c = characters_get_current();
    const CharacterData *d = characters_get_by_index(3);
    check(c == d, "and the current character is the one at that index");

    /* Out of range must not index off the end of the array. The selector is
       driven by a saved preference, which is a number on an SD card that a
       shorter roster can outlive. */
    int before = characters_get_current_index();
    characters_set_current(n + 50);
    check(characters_get_current_index() >= 0 &&
          characters_get_current_index() < n,
          "an out-of-range selection stays inside the roster");
    check(characters_get_current() != NULL, "and still yields a character");

    characters_set_current(-1);
    check(characters_get_current_index() >= 0 &&
          characters_get_current_index() < n,
          "a negative selection stays inside the roster too");
    (void)before;

    check(characters_get_by_index(n) == NULL, "reading past the end returns nothing");
    check(characters_get_by_index(-1) == NULL, "as does a negative index");
}

int main(void) {
    test_roster_is_populated();
    test_every_entry_is_well_formed();
    test_ids_are_unique();
    test_selection();
    return report();
}
