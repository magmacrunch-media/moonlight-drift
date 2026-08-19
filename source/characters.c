#include <stddef.h>
#include "characters.h"

/* GENERATED -- do not hand-edit. Mirrors the source game:
 *   hitbox_*        <- the `hitbox` block in js/characters/<id>.js
 *   thrust/gravity/ <- js/character-balance.js
 *   maxVelocity
 *   sprite_origin_* <- measured by re-rendering each draw() and locating
 *                      the art inside its 128x128 export; differs per
 *                      character, so it cannot be a shared constant.
 * Sprite paths are relative to the app's SD directory and resolved by
 * magnolia_asset_path(). Roster order matches the web game's load order. */
static const CharacterData all_characters[] = {
    {
        "cat-synth", "Synth Cat",
        "sprites/cat-synth-idle.png",
        "sprites/cat-synth-thrust.png",
        42, 30, -1, 18,
        44, 44,
        -0.6f, 0.36f, 10.5f
    },
    {
        "beava", "Beava",
        "sprites/beava-idle.png",
        "sprites/beava-thrust.png",
        40, 35, 0, 0,
        37, 48,
        -0.6f, 0.4f, 10.0f
    },
    {
        "vinny-bobarino", "Vinny Bobarino",
        "sprites/vinny-bobarino-idle.png",
        "sprites/vinny-bobarino-thrust.png",
        35, 35, 15, 0,
        45, 44,
        -0.6f, 0.4f, 10.0f
    },
    {
        "darius-hodgekins", "Darius Hodgekins",
        "sprites/darius-hodgekins-idle.png",
        "sprites/darius-hodgekins-thrust.png",
        40, 35, 0, 0,
        54, 42,
        -0.6f, 0.4f, 10.0f
    },
    {
        "roderick-tron", "Roderick Tron",
        "sprites/roderick-tron-idle.png",
        "sprites/roderick-tron-thrust.png",
        40, 35, 0, 0,
        55, 39,
        -0.6f, 0.4f, 10.0f
    },
    {
        "juanito-thompson", "Juanito Thompson",
        "sprites/juanito-thompson-idle.png",
        "sprites/juanito-thompson-thrust.png",
        40, 35, 0, 0,
        46, 54,
        -0.6f, 0.4f, 10.0f
    },
    {
        "carl-spatski", "Carl Spatski",
        "sprites/carl-spatski-idle.png",
        "sprites/carl-spatski-thrust.png",
        42, 42, -1, -2,
        58, 44,
        -0.65f, 0.4f, 10.5f
    },
    {
        "dspum-balloon", "dspum balloon",
        "sprites/dspum-balloon-idle.png",
        "sprites/dspum-balloon-thrust.png",
        30, 38, 5, -3,
        44, 44,
        -0.5f, 0.3f, 9.0f
    },
    {
        "foresters-soul", "Forester's Soul",
        "sprites/foresters-soul-idle.png",
        "sprites/foresters-soul-thrust.png",
        40, 40, 0, -2,
        59, 44,
        -0.62f, 0.38f, 11.0f
    },
    {
        "grocery-harrison", "Grocery Harrison",
        "sprites/grocery-harrison-idle.png",
        "sprites/grocery-harrison-thrust.png",
        35, 30, 8, 5,
        54, 46,
        -0.6f, 0.4f, 10.0f
    },
    {
        "didgeridoo-man", "Didgeridoo Man",
        "sprites/didgeridoo-man-idle.png",
        "sprites/didgeridoo-man-thrust.png",
        45, 35, 0, 0,
        23, 38,
        -0.7f, 0.35f, 9.5f
    },
    {
        "mountain-gnome", "Mountain Gnome",
        "sprites/mountain-gnome-idle.png",
        "sprites/mountain-gnome-thrust.png",
        32, 32, 4, 0,
        44, 44,
        -0.55f, 0.32f, 9.5f
    },
    {
        "prince-vince", "Prince Vince",
        "sprites/prince-vince-idle.png",
        "sprites/prince-vince-thrust.png",
        27, 38, 7, -3,
        60, 43,
        -0.65f, 0.38f, 10.5f
    },
    {
        "dag-henderson", "Dag Henderson",
        "sprites/dag-henderson-idle.png",
        "sprites/dag-henderson-thrust.png",
        40, 35, 0, 0,
        48, 44,
        -0.6f, 0.4f, 10.0f
    },
    {
        "ban-daniel", "BANDANIEL",
        "sprites/ban-daniel-idle.png",
        "sprites/ban-daniel-thrust.png",
        38, 38, 1, -2,
        40, 44,
        -0.6f, 0.4f, 10.0f
    },
    {
        "plantain-jane", "PLANTAIN JANE",
        "sprites/plantain-jane-idle.png",
        "sprites/plantain-jane-thrust.png",
        38, 38, 1, -2,
        42, 44,
        -0.6f, 0.4f, 10.0f
    },
    {
        "fire-toad", "Fire Toad",
        "sprites/fire-toad-idle.png",
        "sprites/fire-toad-thrust.png",
        32, 32, 4, 0,
        35, 44,
        -0.65f, 0.38f, 10.5f
    },
    {
        "backpack-man", "Backpack Man",
        "sprites/backpack-man-idle.png",
        "sprites/backpack-man-thrust.png",
        34, 36, 6, -2,
        44, 44,
        -0.6f, 0.4f, 10.0f
    },
    {
        "tollbooth-lady", "Tollbooth Lady",
        "sprites/tollbooth-lady-idle.png",
        "sprites/tollbooth-lady-thrust.png",
        40, 35, 0, 0,
        56, 44,
        -0.6f, 0.4f, 10.0f
    },
    {
        "svfp-van", "SVFP Van",
        "sprites/svfp-van-idle.png",
        "sprites/svfp-van-thrust.png",
        36, 18, 4, 12,
        52, 42,
        -0.6f, 0.4f, 10.0f
    },
    {
        "tardigrade", "Tardigrade",
        "sprites/tardigrade-idle.png",
        "sprites/tardigrade-thrust.png",
        26, 25, 7, 8,
        51, 45,
        -0.7f, 0.35f, 11.0f
    },
    {
        "elektra", "Elektra",
        "sprites/elektra-idle.png",
        "sprites/elektra-thrust.png",
        38, 38, 1, 0,
        44, 44,
        -0.58f, 0.38f, 10.5f
    },
    {
        "strawberto", "Strawberto",
        "sprites/strawberto-idle.png",
        "sprites/strawberto-thrust.png",
        36, 38, 2, 0,
        44, 37,
        -0.6f, 0.39f, 10.0f
    },
    {
        "carl", "Carl",
        "sprites/carl-idle.png",
        "sprites/carl-thrust.png",
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
