#include <stddef.h>
#include "characters.h"
#include "assets.h"   /* bin2s-generated symbols for every embedded asset */

/* GENERATED -- do not hand-edit. Mirrors the source game:
 *   hitbox_*        <- the `hitbox` block in js/characters/<id>.js
 *   thrust/gravity/ <- js/character-balance.js
 *   maxVelocity
 *   sprite_origin_* <- measured by re-rendering each draw() and locating
 *                      the art inside its 128x128 export; differs per
 *                      character, so it cannot be a shared constant.
 * Sprite bytes are linked into the binary by the Makefile's bin2s rule; the
 * symbols come from assets.h. Roster order matches the web game's load order. */
static const CharacterData all_characters[] = {
    {
        "cat-synth", "Synth Cat",
        cat_synth_idle_png,
        cat_synth_thrust_png,
        42, 30, -1, 18,
        44, 44,
        -0.6f, 0.36f, 10.5f
    },
    {
        "beava", "Beava",
        beava_idle_png,
        beava_thrust_png,
        40, 35, 0, 0,
        37, 48,
        -0.6f, 0.4f, 10.0f
    },
    {
        "vinny-bobarino", "Vinny Bobarino",
        vinny_bobarino_idle_png,
        vinny_bobarino_thrust_png,
        35, 35, 15, 0,
        45, 44,
        -0.6f, 0.4f, 10.0f
    },
    {
        "darius-hodgekins", "Darius Hodgekins",
        darius_hodgekins_idle_png,
        darius_hodgekins_thrust_png,
        40, 35, 0, 0,
        54, 42,
        -0.6f, 0.4f, 10.0f
    },
    {
        "roderick-tron", "Roderick Tron",
        roderick_tron_idle_png,
        roderick_tron_thrust_png,
        40, 35, 0, 0,
        55, 39,
        -0.6f, 0.4f, 10.0f
    },
    {
        "juanito-thompson", "Juanito Thompson",
        juanito_thompson_idle_png,
        juanito_thompson_thrust_png,
        40, 35, 0, 0,
        46, 54,
        -0.6f, 0.4f, 10.0f
    },
    {
        "carl-spatski", "Carl Spatski",
        carl_spatski_idle_png,
        carl_spatski_thrust_png,
        42, 42, -1, -2,
        58, 44,
        -0.65f, 0.4f, 10.5f
    },
    {
        "dspum-balloon", "dspum balloon",
        dspum_balloon_idle_png,
        dspum_balloon_thrust_png,
        30, 38, 5, -3,
        44, 44,
        -0.5f, 0.3f, 9.0f
    },
    {
        "foresters-soul", "Forester's Soul",
        foresters_soul_idle_png,
        foresters_soul_thrust_png,
        40, 40, 0, -2,
        59, 44,
        -0.62f, 0.38f, 11.0f
    },
    {
        "grocery-harrison", "Grocery Harrison",
        grocery_harrison_idle_png,
        grocery_harrison_thrust_png,
        35, 30, 8, 5,
        54, 46,
        -0.6f, 0.4f, 10.0f
    },
    {
        "didgeridoo-man", "Didgeridoo Man",
        didgeridoo_man_idle_png,
        didgeridoo_man_thrust_png,
        45, 35, 0, 0,
        23, 38,
        -0.7f, 0.35f, 9.5f
    },
    {
        "mountain-gnome", "Mountain Gnome",
        mountain_gnome_idle_png,
        mountain_gnome_thrust_png,
        32, 32, 4, 0,
        44, 44,
        -0.55f, 0.32f, 9.5f
    },
    {
        "prince-vince", "Prince Vince",
        prince_vince_idle_png,
        prince_vince_thrust_png,
        27, 38, 7, -3,
        60, 43,
        -0.65f, 0.38f, 10.5f
    },
    {
        "dag-henderson", "Dag Henderson",
        dag_henderson_idle_png,
        dag_henderson_thrust_png,
        40, 35, 0, 0,
        48, 44,
        -0.6f, 0.4f, 10.0f
    },
    {
        "ban-daniel", "BANDANIEL",
        ban_daniel_idle_png,
        ban_daniel_thrust_png,
        38, 38, 1, -2,
        40, 44,
        -0.6f, 0.4f, 10.0f
    },
    {
        "plantain-jane", "PLANTAIN JANE",
        plantain_jane_idle_png,
        plantain_jane_thrust_png,
        38, 38, 1, -2,
        42, 44,
        -0.6f, 0.4f, 10.0f
    },
    {
        "fire-toad", "Fire Toad",
        fire_toad_idle_png,
        fire_toad_thrust_png,
        32, 32, 4, 0,
        35, 44,
        -0.65f, 0.38f, 10.5f
    },
    {
        "backpack-man", "Backpack Man",
        backpack_man_idle_png,
        backpack_man_thrust_png,
        34, 36, 6, -2,
        44, 44,
        -0.6f, 0.4f, 10.0f
    },
    {
        "tollbooth-lady", "Tollbooth Lady",
        tollbooth_lady_idle_png,
        tollbooth_lady_thrust_png,
        40, 35, 0, 0,
        56, 44,
        -0.6f, 0.4f, 10.0f
    },
    {
        "svfp-van", "SVFP Van",
        svfp_van_idle_png,
        svfp_van_thrust_png,
        36, 18, 4, 12,
        52, 42,
        -0.6f, 0.4f, 10.0f
    },
    {
        "tardigrade", "Tardigrade",
        tardigrade_idle_png,
        tardigrade_thrust_png,
        26, 25, 7, 8,
        51, 45,
        -0.7f, 0.35f, 11.0f
    },
    {
        "elektra", "Elektra",
        elektra_idle_png,
        elektra_thrust_png,
        38, 38, 1, 0,
        44, 44,
        -0.58f, 0.38f, 10.5f
    },
    {
        "strawberto", "Strawberto",
        strawberto_idle_png,
        strawberto_thrust_png,
        36, 38, 2, 0,
        44, 37,
        -0.6f, 0.39f, 10.0f
    },
    {
        "carl", "Carl",
        carl_idle_png,
        carl_thrust_png,
        38, 38, 1, 0,
        49, 39,
        -0.62f, 0.37f, 11.0f
    }
};

#define CHARACTER_COUNT (sizeof(all_characters) / sizeof(all_characters[0]))

static int current_index = 0;

void characters_init(void) {
    current_index = 0;
}

const CharacterData *characters_get_current(void) {
    return &all_characters[current_index];
}

void characters_set_current(int index) {
    if (index >= 0 && index < (int)CHARACTER_COUNT) {
        current_index = index;
    }
}

int characters_get_current_index(void) {
    return current_index;
}

int characters_get_count(void) {
    return (int)CHARACTER_COUNT;
}

const CharacterData *characters_get_by_index(int index) {
    if (index >= 0 && index < (int)CHARACTER_COUNT) {
        return &all_characters[index];
    }
    return NULL;
}
