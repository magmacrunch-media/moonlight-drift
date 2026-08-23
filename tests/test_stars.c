/* The starfield and the shooting stars.
 *
 * Background, and the least likely place for a bug that ruins a run -- but the
 * field is seeded across the *visible world*, which changes with the video
 * mode, and a star placed outside it is either invisible or drawn off the safe
 * area. The bounds are the part worth holding.
 *
 * Both modules hold file-static state, so every case re-seeds and re-inits.
 *
 *   make test-stars
 */
#include <stdio.h>
#include <stdlib.h>
#include "harness.h"
#include "stars.h"
#include "config.h"

static void test_field_is_seeded(void) {
    printf("stars: the field is seeded inside the world it is given\n");
    srand(11u);

    const int w = 960, h = WORLD_HEIGHT;
    stars_init(w, h);

    int n = stars_get_count();
    check_int(n, STAR_COUNT, "the field holds STAR_COUNT stars");

    int out_of_bounds = 0, bad_pattern = 0, bad_colour = 0, bad_blink = 0;
    char first[112] = "";

    for (int i = 0; i < n; i++) {
        Star *s = NULL;
        stars_get(i, &s);
        if (!s) { out_of_bounds++; continue; }

        if (s->x < 0 || s->x >= w || s->y < 0 || s->y >= h) {
            if (!out_of_bounds++ && !first[0]) {
                snprintf(first, sizeof(first), "star %d at (%d,%d) is outside %dx%d",
                         i, s->x, s->y, w, h);
            }
        }
        if (s->pattern < 0 || s->pattern >= STAR_PATTERN_COUNT) bad_pattern++;
        if (s->color_r < 0 || s->color_r > 255 ||
            s->color_g < 0 || s->color_g > 255 ||
            s->color_b < 0 || s->color_b > 255) bad_colour++;
        /* The header says frame 3 is the invisible one, so the counter has to
           stay inside 0..3 or a star blinks on a frame that does not exist. */
        if (s->blink_frame < 0 || s->blink_frame > 3) bad_blink++;
    }

    if (first[0]) printf("  %s\n", first);
    check_int(out_of_bounds, 0, "every star lands inside the world");
    check_int(bad_pattern, 0, "every pattern index is valid");
    check_int(bad_colour, 0, "every colour channel is a byte");
    check_int(bad_blink, 0, "every blink frame is 0..3");

    /* A narrower world -- what a 4:3 set actually shows of the 1280 field --
       must reseed rather than leaving stars off the right-hand edge. */
    stars_init(640, 480);
    int outside = 0;
    for (int i = 0; i < stars_get_count(); i++) {
        Star *s = NULL;
        stars_get(i, &s);
        if (s && (s->x >= 640 || s->y >= 480)) outside++;
    }
    check_int(outside, 0, "re-initialising to a smaller world reseeds inside it");

    /* Degenerate sizes are clamped rather than producing a divide or a
       negative coordinate. */
    stars_init(0, 0);
    check_int(stars_get_count(), STAR_COUNT, "a zero-sized world still yields a field");
    stars_init(-100, -100);
    check_int(stars_get_count(), STAR_COUNT, "so does a negative one");

    check(stars_get_count() > 0, "the field is never empty");
    Star *past = NULL;
    stars_get(STAR_COUNT + 5, &past);
    check(past == NULL, "reading past the end returns nothing");
}

static void test_field_updates_stay_in_range(void) {
    printf("stars: updating never walks a star out of range\n");
    srand(12u);

    const int w = 960, h = WORLD_HEIGHT;
    stars_init(w, h);

    int strayed = 0, bad_blink = 0;
    for (int frame = 0; frame < 600; frame++) {
        stars_update();
        for (int i = 0; i < stars_get_count(); i++) {
            Star *s = NULL;
            stars_get(i, &s);
            if (!s) continue;
            if (s->x < 0 || s->x >= w || s->y < 0 || s->y >= h) strayed++;
            if (s->blink_frame < 0 || s->blink_frame > 3) bad_blink++;
        }
    }
    check_int(strayed, 0, "no star drifts outside the world over 600 frames");
    check_int(bad_blink, 0, "and the blink frame stays in range throughout");
}

/* How many slots currently hold a live star.
 *
 * shooting_stars_get_count() reports the size of the pool, not how many are in
 * flight -- a slot is live only when life > 0, and game_render.c skips the dead
 * ones itself. Worth stating in the test, because "get_count" reads like the
 * other one and a caller that trusts the name draws four stuck streaks. */
static int live_shooters(void) {
    int live = 0;
    for (int i = 0; i < shooting_stars_get_count(); i++) {
        ShootingStar *s = NULL;
        shooting_stars_get(i, &s);
        if (s && s->life > 0) live++;
    }
    return live;
}

static void test_shooting_stars(void) {
    printf("stars: shooting stars\n");
    srand(13u);

    check_int(shooting_stars_get_count(), MAX_SHOOTING_STARS,
              "the pool is MAX_SHOOTING_STARS slots wide");

    shooting_stars_reset();
    check_int(live_shooters(), 0, "a reset pool has none in flight");

    /* The web spawns them at roughly 0.3% a frame, so a few thousand frames
       produce some without the test being flaky about exactly when. */
    int most = 0, over_cap = 0;
    for (int frame = 0; frame < 20000; frame++) {
        shooting_stars_update(WORLD_WIDTH, WORLD_HEIGHT);
        int live = live_shooters();
        if (live > most) most = live;
        if (live > MAX_SHOOTING_STARS) over_cap++;
    }

    printf("  most in flight at once: %d of %d slots\n", most, MAX_SHOOTING_STARS);
    check(most > 0, "shooting stars do eventually appear");
    check_int(over_cap, 0, "and never exceed the pool");

    /* They travel leftward and expire, rather than accumulating: a star that
       never dies would pin a slot forever and thin the field over a long run. */
    shooting_stars_reset();
    srand(14u);
    int spawned_at = -1;
    for (int frame = 0; frame < 20000 && spawned_at < 0; frame++) {
        shooting_stars_update(WORLD_WIDTH, WORLD_HEIGHT);
        if (live_shooters() > 0) spawned_at = frame;
    }
    check(spawned_at >= 0, "one spawns within a reasonable run");

    if (spawned_at >= 0) {
        ShootingStar *s = NULL;
        for (int i = 0; i < shooting_stars_get_count(); i++) {
            shooting_stars_get(i, &s);
            if (s && s->life > 0) break;
            s = NULL;
        }
        check(s != NULL, "and is readable");
        if (s) {
            check(s->vx < 0.0f, "it travels leftward across the field");
            float x0 = s->x;
            shooting_stars_update(WORLD_WIDTH, WORLD_HEIGHT);
            check(s->x < x0, "and actually moves");
        }

        /* Run long enough for every star to expire, with spawning effectively
           off, and the pool must drain rather than filling up. */
        for (int frame = 0; frame < 500; frame++) {
            shooting_stars_update(WORLD_WIDTH, WORLD_HEIGHT);
        }
        check(live_shooters() <= MAX_SHOOTING_STARS, "the pool never overfills");
    }

    /* Reset clears them mid-flight -- it runs at the start of every run. */
    shooting_stars_reset();
    check_int(live_shooters(), 0, "reset empties the pool mid-flight");

    ShootingStar *past = NULL;
    shooting_stars_get(MAX_SHOOTING_STARS + 1, &past);
    check(past == NULL, "reading past the end returns nothing");
    shooting_stars_get(-1, &past);
    check(past == NULL, "as does a negative index");
}

int main(void) {
    test_field_is_seeded();
    test_field_updates_stay_in_range();
    test_shooting_stars();
    return report();
}
