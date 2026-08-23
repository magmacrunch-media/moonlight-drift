/* Jetman physics: thrust against gravity, a velocity clamp, and the two lethal
 * edges of the world.
 *
 * Small enough to read in one sitting, which is exactly why it is worth pinning
 * down -- everything here is one sign or one offset away from a game that plays
 * subtly wrong and never crashes.
 *
 *   make test-player
 */
#include <stdio.h>
#include <string.h>
#include "harness.h"
#include "player.h"
#include "config.h"

/* Rough float comparison: these are accumulated sums, so exact equality would
   be testing the FPU rather than the physics. */
static void check_near(float got, float want, const char *what) {
    checks++;
    float d = got - want;
    if (d < 0.0f) d = -d;
    if (d > 0.001f) {
        printf("  FAIL: %s (got %f, want %f)\n", what, (double)got, (double)want);
        failures++;
    }
}

static void test_defaults(void) {
    printf("player: a freshly placed jetman\n");

    Player p;
    player_init(&p);

    check_near(p.x, (float)PLAYER_X, "starts at the configured x");
    check_near(p.y, (float)PLAYER_Y_INITIAL, "and the configured y");
    check_near(p.velocity, 0.0f, "at rest");
    check_near(p.thrust, DEFAULT_THRUST, "with the default thrust");
    check_near(p.gravity, DEFAULT_GRAVITY, "and the default gravity");

    /* Thrust lifts and gravity pulls, so in screen coordinates -- where y grows
       downward -- thrust must be negative and gravity positive. Getting these
       the same sign gives a jetman that only ever falls. */
    check(p.thrust < 0.0f, "thrust is negative: it lifts");
    check(p.gravity > 0.0f, "gravity is positive: it pulls down");
}

static void test_thrust_and_gravity(void) {
    printf("player: thrust against gravity\n");

    Player p;
    player_init(&p);
    player_set_hitbox(&p, 40, 35, 0, 0);

    /* One frame of falling. */
    player_update(&p, 0, WORLD_HEIGHT);
    check_near(p.velocity, DEFAULT_GRAVITY, "not thrusting accumulates gravity");
    check_near(p.y, (float)PLAYER_Y_INITIAL + DEFAULT_GRAVITY,
               "and the position follows the velocity");

    /* One frame of thrust from rest. */
    player_init(&p);
    player_set_hitbox(&p, 40, 35, 0, 0);
    player_update(&p, 1, WORLD_HEIGHT);
    check_near(p.velocity, DEFAULT_THRUST, "thrusting accumulates thrust");
    check(p.y < (float)PLAYER_Y_INITIAL, "and the jetman rises");

    /* Velocity accumulates rather than being set, which is what makes the
       controls feel like momentum instead of a cursor. */
    player_init(&p);
    player_set_hitbox(&p, 40, 35, 0, 0);
    player_update(&p, 0, WORLD_HEIGHT);
    player_update(&p, 0, WORLD_HEIGHT);
    check_near(p.velocity, DEFAULT_GRAVITY * 2.0f, "gravity accumulates across frames");
}

static void test_velocity_clamp(void) {
    printf("player: the terminal velocity clamp\n");

    Player p;
    player_init(&p);
    player_set_hitbox(&p, 40, 35, 0, 0);
    /* A tall world, so the clamp is what stops it rather than the floor. */
    const int tall = 1000000;

    for (int i = 0; i < 500; i++) player_update(&p, 0, tall);
    check_near(p.velocity, DEFAULT_MAX_VELOCITY, "falling clamps at +maxVelocity");

    player_init(&p);
    player_set_hitbox(&p, 40, 35, 0, 0);
    p.y = 500000.0f;
    for (int i = 0; i < 500; i++) player_update(&p, 1, tall);
    check_near(p.velocity, -DEFAULT_MAX_VELOCITY, "rising clamps at -maxVelocity");

    /* Both directions matter: a clamp applied only to the positive side lets a
       held thrust accelerate without limit. */
    player_init(&p);
    player_set_physics(&p, -1.0f, 1.0f, 3.0f);
    player_set_hitbox(&p, 40, 35, 0, 0);
    p.y = 500000.0f;
    for (int i = 0; i < 50; i++) player_update(&p, 1, tall);
    check(p.velocity >= -3.0f, "a custom maxVelocity is honoured upward too");
}

/* The fix recorded in player.c: boundary death is measured against the hitbox,
 * not the sprite box, so a character whose art sits off its origin does not die
 * early. Worth a test precisely because the naive version also "works" -- it
 * just kills some characters a few pixels sooner than others. */
static void test_boundaries_use_the_hitbox(void) {
    printf("player: the lethal edges are measured on the hitbox\n");

    Player p;
    player_init(&p);
    player_set_hitbox(&p, 40, 35, 0, 0);

    /* Comfortably inside: alive. */
    p.y = 300.0f;
    p.velocity = 0.0f;
    check_int(player_update(&p, 0, WORLD_HEIGHT), 0, "mid-field is safe");

    /* Through the ceiling. */
    player_init(&p);
    player_set_hitbox(&p, 40, 35, 0, 0);
    p.y = -50.0f;
    check_int(player_update(&p, 1, WORLD_HEIGHT), 1, "above the ceiling is death");

    /* Through the floor. */
    player_init(&p);
    player_set_hitbox(&p, 40, 35, 0, 0);
    p.y = (float)WORLD_HEIGHT + 10.0f;
    check_int(player_update(&p, 0, WORLD_HEIGHT), 1, "below the floor is death");

    /* The offset case. A character whose hitbox sits 40px below its sprite
       origin is still airborne when the sprite box has crossed the ceiling: at
       y = -30 the sprite is above the edge, but hitbox top is -30 + 40 = 10,
       which is inside. A sprite-box test would kill it here. */
    player_init(&p);
    player_set_hitbox(&p, 40, 35, 0, 40);
    p.y = -30.0f;
    p.velocity = 0.0f;
    check_int(player_update(&p, 1, WORLD_HEIGHT), 0,
              "an offset hitbox survives where the sprite box would not");

    /* And the same offset must still die once the hitbox itself is out. */
    player_init(&p);
    player_set_hitbox(&p, 40, 35, 0, 40);
    p.y = -60.0f;
    p.velocity = 0.0f;
    check_int(player_update(&p, 1, WORLD_HEIGHT), 1,
              "but dies once the hitbox clears the ceiling");

    /* The floor end of the same rule. */
    player_init(&p);
    player_set_hitbox(&p, 40, 35, 0, -20);
    p.y = (float)WORLD_HEIGHT - 20.0f;
    p.velocity = 0.0f;
    check_int(player_update(&p, 1, WORLD_HEIGHT), 0,
              "a hitbox lifted off the sprite survives near the floor");
}

static void test_hitbox_accessor(void) {
    printf("player: reading the hitbox back\n");

    Player p;
    player_init(&p);
    p.x = 100.0f;
    p.y = 200.0f;
    player_set_hitbox(&p, 42, 30, -1, 18);

    int x, y, w, h;
    player_get_hitbox(&p, &x, &y, &w, &h);
    check_int(x, 99, "x carries the offset");
    check_int(y, 218, "y carries the offset");
    check_int(w, 42, "width is the hitbox width");
    check_int(h, 30, "height is the hitbox height");

    player_set_physics(&p, -0.8f, 0.5f, 12.0f);
    check_near(p.thrust, -0.8f, "physics can be replaced per character");
    check_near(p.gravity, 0.5f, "gravity too");
    check_near(p.maxVelocity, 12.0f, "and the terminal velocity");
}

int main(void) {
    test_defaults();
    test_thrust_and_gravity();
    test_velocity_clamp();
    test_boundaries_use_the_hitbox();
    test_hitbox_accessor();
    return report();
}
