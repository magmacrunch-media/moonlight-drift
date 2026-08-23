/* The obstacle field: spawning, scrolling, scoring and collision.
 *
 * This is the module where a wrong rule would hide longest. Everything in it is
 * either a flag that must fire exactly once, an array shift that must not lose
 * an element, or a piece of geometry that decides whether the player is dead --
 * and all three look correct while being wrong.
 *
 * The module holds file-static state and calls rand(), so every case calls
 * obstacles_init() first and seeds deterministically where the values matter. A
 * suite that only passes in the order it happens to run is worse than none.
 *
 *   make test-obstacles
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "harness.h"
#include "obstacles.h"
#include "config.h"

/* Spawns n obstacles into a fresh field. */
static void spawn(int n) {
    for (int i = 0; i < n; i++) obstacles_create(WORLD_WIDTH, WORLD_HEIGHT);
}

static Obstacle *at(int i) {
    Obstacle *o = NULL;
    obstacles_get(i, &o);
    return o;
}

static void test_spawning(void) {
    printf("obstacles: spawning\n");
    srand(1u);
    obstacles_init();

    check_int(obstacles_get_count(), 0, "a fresh field is empty");

    obstacles_create(WORLD_WIDTH, WORLD_HEIGHT);
    check_int(obstacles_get_count(), 1, "creating one adds one");

    Obstacle *o = at(0);
    check(o != NULL, "and it is readable");
    if (o) {
        /* Spawned at the right-hand edge of the world it was given, so it
           scrolls in rather than appearing on top of the player. */
        check(o->x == (float)WORLD_WIDTH, "it starts at the spawn edge");
        check(o->top_height >= MIN_OBSTACLE_HEIGHT, "the top is at least the minimum");
        check(o->top_height < MAX_OBSTACLE_HEIGHT, "and below the maximum");
        check_int(o->bottom_y, o->top_height + GAP, "the gap sits directly below the top");
        check_int(o->passed, 0, "and it has not been scored yet");
        check(o->style_index >= 0 && o->style_index < 3, "the style is one of three");
        check(o->seed >= 0.0f && o->seed < 1.0f, "the seed is a unit fraction");
        check(o->roughness >= 0.5f && o->roughness <= 1.0f, "roughness is 0.5..1");
        check(o->asymmetry >= -1.0f && o->asymmetry <= 1.0f, "asymmetry is -1..1");
    }

    /* The gap must always fit on the field, at every random top height, or the
       game becomes unplayable on the spawns where it does not. */
    int gap_off_field = 0;
    for (int i = 0; i < 500; i++) {
        obstacles_init();
        obstacles_create(WORLD_WIDTH, WORLD_HEIGHT);
        Obstacle *ob = at(0);
        if (!ob || ob->bottom_y > WORLD_HEIGHT) gap_off_field++;
    }
    check_int(gap_off_field, 0, "the gap always fits inside the world");

    /* The cap holds, and overflowing it does not scribble past the array. */
    obstacles_init();
    spawn(MAX_OBSTACLES + 10);
    check_int(obstacles_get_count(), MAX_OBSTACLES, "the field caps at MAX_OBSTACLES");
    check(at(MAX_OBSTACLES) == NULL, "and reading past the end returns nothing");
    check(at(-1) == NULL, "as does a negative index");
}

/* The comment in obstacles.c says the source game marks the 10th, 20th ...
 * obstacle and uses the count itself as the label, and that the counter is
 * incremented *before* the test. That comment exists because it was got wrong,
 * which is exactly the kind of thing that should not rely on a comment. */
static void test_milestones(void) {
    printf("obstacles: milestone markers\n");
    srand(2u);
    obstacles_init();

    int wrong_label = 0, wrong_blank = 0;
    char first[96] = "";

    /* Spawn one at a time and read it before the field fills, so each index is
       known. Cull between spawns so the cap never interferes. */
    for (int n = 1; n <= 40; n++) {
        obstacles_create(WORLD_WIDTH, WORLD_HEIGHT);
        Obstacle *o = at(obstacles_get_count() - 1);
        if (!o) continue;

        int expected = (n % THEME_CHANGE_INTERVAL == 0) ? n : 0;
        if (o->milestone != expected) {
            if (expected != 0) wrong_label++; else wrong_blank++;
            if (!first[0]) {
                snprintf(first, sizeof(first),
                         "obstacle %d had milestone %d, wanted %d",
                         n, o->milestone, expected);
            }
        }

        /* Scroll everything off so the next spawn is index 0 again. */
        for (int k = 0; k < 20; k++) obstacles_update(WORLD_WIDTH);
        if (obstacles_get_count() >= MAX_OBSTACLES - 1) {
            for (int k = 0; k < WORLD_WIDTH; k++) obstacles_update(WORLD_WIDTH);
        }
    }

    if (first[0]) printf("  first mismatch: %s\n", first);
    check_int(wrong_label, 0, "every tenth obstacle is labelled with its own count");
    check_int(wrong_blank, 0, "and no other obstacle is labelled at all");
}

/* One palette is shared and rotates every THEME_CHANGE_INTERVAL spawns. The
 * header says generating a fresh theme per obstacle was "the main reason the
 * port did not read like the arcade version", so this is a look-of-the-game
 * rule, not an implementation detail. */
static void test_theme_runs(void) {
    printf("obstacles: colours move in coherent runs\n");
    srand(3u);
    obstacles_init();

    /* The counter is incremented before the palette is consulted, so the change
       fires as the 10th, 20th ... obstacle is built. That makes the first run
       nine long -- it starts from the palette obstacles_init() generated -- and
       every run after it ten. Written out as the boundary rather than as "every
       ten", because the off-by-one is the whole content of the rule. */
    Theme run[41];
    for (int n = 1; n <= 40; n++) {
        obstacles_create(WORLD_WIDTH, WORLD_HEIGHT);
        Obstacle *o = at(obstacles_get_count() - 1);
        if (o) run[n] = o->theme;

        /* Keep the field short so the cap never swallows a spawn. */
        for (int k = 0; k < WORLD_WIDTH / OBSTACLE_SPEED + 2; k++) {
            obstacles_update(WORLD_WIDTH);
        }
    }

    int within_differs = 0, boundary_same = 0;
    for (int n = 2; n <= 40; n++) {
        int same = (memcmp(&run[n - 1], &run[n], sizeof(Theme)) == 0);
        int is_boundary = (n % THEME_CHANGE_INTERVAL == 0);

        if (is_boundary && same) boundary_same++;
        if (!is_boundary && !same) within_differs++;
    }

    check_int(within_differs, 0, "consecutive obstacles inside a run share a palette");
    check_int(boundary_same, 0, "and every tenth obstacle starts a new one");

    /* Guard against the degenerate pass: if theme_generate returned a constant,
       everything above would be satisfied by nothing ever changing. */
    check(memcmp(&run[1], &run[40], sizeof(Theme)) != 0,
          "the palette really has moved across forty obstacles");
}

static void test_scrolling_and_culling(void) {
    printf("obstacles: scrolling and the cull\n");
    srand(4u);
    obstacles_init();
    spawn(3);

    Obstacle *o = at(0);
    float before = o ? o->x : 0.0f;
    obstacles_update(WORLD_WIDTH);
    o = at(0);
    check(o && o->x == before - OBSTACLE_SPEED, "one update moves the field left by the speed");
    check_int(obstacles_get_count(), 3, "and culls nothing that is still on screen");

    /* Drive the leftmost off the edge and confirm the survivors keep their
       order. The cull walks backwards and shifts the tail down, which is where
       an off-by-one would drop or duplicate an element. */
    obstacles_init();
    spawn(3);
    /* Tag them so identity survives the shift. */
    at(0)->top_height = 111;
    at(1)->top_height = 222;
    at(2)->top_height = 333;

    /* Position the first just about to leave, the others well clear. */
    at(0)->x = -OBSTACLE_WIDTH + 1.0f;
    at(1)->x = 400.0f;
    at(2)->x = 800.0f;

    obstacles_update(WORLD_WIDTH);
    check_int(obstacles_get_count(), 2, "an obstacle past the left edge is culled");
    check_int(at(0) ? at(0)->top_height : -1, 222, "the survivors shift down in order");
    check_int(at(1) ? at(1)->top_height : -1, 333, "keeping their relative order");

    /* Everything off at once: the loop must not trip over itself. */
    obstacles_init();
    spawn(5);
    for (int i = 0; i < 5; i++) at(i)->x = -1000.0f;
    obstacles_update(WORLD_WIDTH);
    check_int(obstacles_get_count(), 0, "a whole field can be culled in one update");
}

/* Scoring is a flag, and a flag that fires twice is the classic bug in this
 * shape. Nothing but this test stands between the game and a doubled score. */
static void test_scoring(void) {
    printf("obstacles: scoring exactly once\n");
    srand(5u);
    obstacles_init();
    spawn(3);

    const int player_x = 200;

    /* All still ahead of the player: nothing scores. */
    for (int i = 0; i < 3; i++) at(i)->x = (float)(player_x + 100 + i * 100);
    check_int(obstacles_check_score(player_x), 0, "an obstacle ahead of the player scores nothing");

    /* Move one fully behind the player. */
    at(0)->x = (float)(player_x - OBSTACLE_WIDTH - 1);
    check_int(obstacles_check_score(player_x), 1, "passing one scores one");
    check_int(at(0)->passed, 1, "and marks it passed");

    /* The whole point: it must never score again. */
    check_int(obstacles_check_score(player_x), 0, "the same obstacle never scores twice");
    check_int(obstacles_check_score(player_x), 0, "no matter how often it is asked");

    /* Two more crossing at once are both counted. */
    at(1)->x = (float)(player_x - OBSTACLE_WIDTH - 1);
    at(2)->x = (float)(player_x - OBSTACLE_WIDTH - 1);
    check_int(obstacles_check_score(player_x), 2, "two crossing together score two");
    check_int(obstacles_check_score(player_x), 0, "and then stop scoring");

    /* Partially past is not past: the trailing edge has to clear the player. */
    obstacles_init();
    spawn(1);
    at(0)->x = (float)(player_x - OBSTACLE_WIDTH + 1);
    check_int(obstacles_check_score(player_x), 0, "an obstacle still overlapping has not been passed");

    /* A fresh field forgets everything. */
    obstacles_init();
    spawn(1);
    at(0)->x = (float)(player_x - OBSTACLE_WIDTH - 1);
    check_int(obstacles_check_score(player_x), 1, "a new field scores its own obstacles");
}

static void test_collision(void) {
    printf("obstacles: collision against the gap\n");
    srand(6u);
    obstacles_init();
    spawn(1);

    Obstacle *o = at(0);
    o->x = 100.0f;
    o->top_height = 200;
    o->bottom_y = 200 + GAP;
    o->style_index = 1;
    o->seed = 0.0f;
    o->roughness = 0.5f;
    o->asymmetry = 0.0f;

    const int pw = 40, ph = 35;

    /* Horizontally clear of the column: never a collision, wherever vertically. */
    check_int(obstacles_check_collision(0, 0, pw, ph), 0,
              "a player left of the column is safe at the top");
    check_int(obstacles_check_collision(0, 600, pw, ph), 0,
              "and safe at the bottom");
    check_int(obstacles_check_collision(500, 0, pw, ph), 0,
              "so is one right of the column");

    /* Squarely inside the gap, overlapping the column horizontally. */
    int gap_middle = 200 + GAP / 2 - ph / 2;
    check_int(obstacles_check_collision(100, gap_middle, pw, ph), 0,
              "a player inside the gap passes through");

    /* Into the top block. */
    check_int(obstacles_check_collision(100, 10, pw, ph), 1,
              "a player in the top block collides");

    /* Into the bottom block. The hitbox excludes FLAME_HEIGHT, so push well
       past the lip rather than sitting on it. */
    check_int(obstacles_check_collision(100, 200 + GAP + 60, pw, ph), 1,
              "a player in the bottom block collides");

    /* The flame is not part of the player. A body whose sprite box reaches into
       the bottom block, but whose hitbox does not once the flame is removed,
       must survive -- that exclusion is deliberate. */
    obstacles_init();
    spawn(1);
    o = at(0);
    o->x = 100.0f;
    o->top_height = 200;
    o->bottom_y = 200 + GAP;
    o->style_index = 1;
    o->seed = 0.0f;
    o->roughness = 0.5f;
    o->asymmetry = 0.0f;

    int just_above = o->bottom_y - ph + FLAME_HEIGHT - 1;
    check_int(obstacles_check_collision(100, just_above, pw, ph), 0,
              "the flame does not count as part of the hitbox");

    /* Sweeping the gap at every horizontal overlap: the gap must be passable
       everywhere, in every style, or some spawns are unwinnable. */
    int blocked = 0;
    char first[96] = "";
    for (int style = 0; style < 3; style++) {
        obstacles_init();
        spawn(1);
        o = at(0);
        o->top_height = 200;
        o->bottom_y = 200 + GAP;
        o->style_index = style;
        o->seed = 0.37f;
        o->roughness = 0.8f;
        o->asymmetry = 0.6f;

        for (int x = 40; x <= 200; x += 4) {
            o->x = (float)x;
            /* Centre of the gap, allowing for the flame exclusion. */
            int y = 200 + GAP / 2 - ph / 2;
            if (obstacles_check_collision(100, y, pw, ph)) {
                if (!blocked++) {
                    snprintf(first, sizeof(first),
                             "style %d at column x=%d blocks the middle of the gap",
                             style, x);
                }
            }
        }
    }
    if (blocked) printf("  first blockage: %s\n", first);
    check_int(blocked, 0, "the middle of the gap is passable in every style");
}

int main(void) {
    test_spawning();
    test_milestones();
    test_theme_runs();
    test_scrolling_and_culling();
    test_scoring();
    test_collision();
    return report();
}
