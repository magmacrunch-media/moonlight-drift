/* The projection from the game's 1280x720 world onto the television.
 *
 * This is the one piece of geometry here whose failure is invisible from a
 * desk: a wrong scale does not crash, it plays the game at the wrong size on a
 * video mode you may not own. And it is not only drawing -- pf_world_width()
 * sets the obstacle spawn edge, the cull edge and the starfield spread, so
 * getting it wrong changes how many obstacles are on screen at once.
 *
 * The module's own header describes the pre-projection port as "not a port of
 * the game, it is a different game", because the gap went "from a quarter of
 * the screen to over a third". The headline test below is that sentence turned
 * into a check.
 *
 *   make test-playfield
 */
#include <stdio.h>
#include <string.h>
#include "harness.h"
#include "projection.h"
#include "config.h"

/* Built the way magnolia builds it: ui_safe_x() is fb * pct / 100 in integer
   arithmetic and the safe span is what is left after taking that off both
   edges. Recomputed here rather than linked, because the point is to feed
   projection_compute() the numbers the console really produces. */
static ProjectionInput mode(int fb_w, int fb_h, int widescreen, int overscan_pct) {
    ProjectionInput in;
    in.fb_w = fb_w;
    in.fb_h = fb_h;
    in.safe_x = fb_w * overscan_pct / 100;
    in.safe_y = fb_h * overscan_pct / 100;
    in.safe_w = fb_w - 2 * in.safe_x;
    in.safe_h = fb_h - 2 * in.safe_y;
    in.widescreen = widescreen;
    return in;
}

/* The three the console actually produces, at the shipped overscan. */
static ProjectionInput ntsc_43(void)  { return mode(640, 480, 0, OVERSCAN_PCT); }
static ProjectionInput pal_43(void)   { return mode(640, 528, 0, OVERSCAN_PCT); }
static ProjectionInput ntsc_169(void) { return mode(640, 480, 1, OVERSCAN_PCT); }
static ProjectionInput pal_169(void)  { return mode(640, 528, 1, OVERSCAN_PCT); }

static void check_near(float got, float want, float tol, const char *what) {
    checks++;
    float d = got - want;
    if (d < 0.0f) d = -d;
    if (d > tol) {
        printf("  FAIL: %s (got %f, want %f +/- %f)\n",
               what, (double)got, (double)want, (double)tol);
        failures++;
    }
}

/* The property the whole module exists for: a world length must occupy the same
 * share of the safe area on every video mode. If this holds, the gap is the same
 * fraction of the screen on a PAL CRT as on an NTSC widescreen, which is what
 * "the same game" means here. */
static void test_the_game_is_the_same_size_everywhere(void) {
    printf("playfield: the game is the same size on every mode\n");

    struct { const char *name; ProjectionInput in; } modes[] = {
        { "NTSC 4:3",  ntsc_43()  },
        { "PAL 4:3",   pal_43()   },
        { "NTSC 16:9", ntsc_169() },
        { "PAL 16:9",  pal_169()  }
    };
    const int n = (int)(sizeof(modes) / sizeof(modes[0]));

    /* The gap, as a fraction of the safe area's height. Identical everywhere,
       by construction -- scale_y is derived from safe_h and nothing else. */
    float first = 0.0f;
    for (int i = 0; i < n; i++) {
        Projection p = projection_compute(modes[i].in);
        float gap_px = (float)GAP * p.scale_y;
        float frac   = gap_px / (float)modes[i].in.safe_h;

        if (i == 0) first = frac;

        char what[96];
        snprintf(what, sizeof(what), "%s: the gap is the same share of the screen",
                 modes[i].name);
        check_near(frac, first, 0.0005f, what);
    }

    /* And that share is the one the source game has: 180 of 720. */
    check_near(first, (float)GAP / (float)WORLD_HEIGHT, 0.0005f,
               "and it is the share the 1280x720 canvas gives it");

    /* The whole world height fills the safe area exactly, top to bottom. The
       top and bottom edges are lethal, so this is a gameplay boundary and not a
       margin. */
    for (int i = 0; i < n; i++) {
        Projection p = projection_compute(modes[i].in);
        float bottom = (float)WORLD_HEIGHT * p.scale_y;

        char what[96];
        snprintf(what, sizeof(what), "%s: the world fills the safe height",
                 modes[i].name);
        check_near(bottom, (float)modes[i].in.safe_h, 1.0f, what);
    }
}

static void test_pixel_aspect(void) {
    printf("playfield: square and non-square pixels\n");

    /* NTSC 4:3 is the one mode whose framebuffer ratio already matches the
       display, so its pixels are square and the two scales agree. */
    Projection n43 = projection_compute(ntsc_43());
    check_near(n43.par, 1.0f, 0.001f, "NTSC 4:3 has square pixels");
    check_near(n43.scale_x, n43.scale_y, 0.001f, "so both scales agree");

    /* PAL's frame is 1.1 times taller than wide for the same picture. */
    Projection p43 = projection_compute(pal_43());
    check_near(p43.par, 1.1f, 0.005f, "PAL 4:3 pixels are 1.1 times tall");
    check(p43.scale_x < p43.scale_y, "so the horizontal scale is the smaller one");

    /* Widescreen stretches the same frame by 4/3. */
    Projection n169 = projection_compute(ntsc_169());
    check_near(n169.par, 4.0f / 3.0f, 0.005f, "NTSC 16:9 stretches by 4/3");
    check(n169.scale_x < n169.scale_y, "so the horizontal scale shrinks");

    /* The direction matters and is the single most invertible line in the
       module: dividing by par where you should multiply stretches the game the
       wrong way on every widescreen set. A wider display must fit MORE world
       across, not less. */
    check(n169.scale_x < n43.scale_x,
          "widescreen uses a smaller horizontal scale than 4:3");
    check(n169.world_w > n43.world_w,
          "and therefore shows more of the world, not less");
}

static void test_world_width(void) {
    printf("playfield: how much of the world fits across\n");

    Projection n43  = projection_compute(ntsc_43());
    Projection p43  = projection_compute(pal_43());
    Projection n169 = projection_compute(ntsc_169());
    Projection p169 = projection_compute(pal_169());

    printf("  NTSC 4:3 %d   PAL 4:3 %d   NTSC 16:9 %d   PAL 16:9 %d\n",
           n43.world_w, p43.world_w, n169.world_w, p169.world_w);

    /* The header's claim, checked as written: "NTSC and PAL agree". */
    int diff = n43.world_w - p43.world_w;
    if (diff < 0) diff = -diff;
    check(diff <= 2, "the two 4:3 modes agree on world width to within a pixel or two");

    /* "Roughly 960 on any 4:3 set." Loose, because the header says roughly. */
    check(n43.world_w > 900 && n43.world_w < 1000, "NTSC 4:3 shows roughly 960");
    check(p43.world_w > 900 && p43.world_w < 1000, "PAL 4:3 shows roughly 960");

    /* Widescreen approaches the full canvas. The header says "the full 1280";
       it is a few pixels short of that, which is recorded below rather than
       rounded away. */
    check(n169.world_w > 1200, "16:9 shows nearly the whole 1280-wide world");
    check(p169.world_w > 1200, "on PAL as well as NTSC");

    /* Never more than the source game has, on any input. */
    check(n43.world_w  <= WORLD_WIDTH && p43.world_w  <= WORLD_WIDTH &&
          n169.world_w <= WORLD_WIDTH && p169.world_w <= WORLD_WIDTH,
          "no mode claims more world than exists");

    /* Exact values, recorded from the implementation rather than derived
       independently -- so a change that moves them has to move them on purpose.
       Confirmed against a real Dolphin run at 16:9, which reported
       "world 1276x720" from the same arithmetic. */
    check_int(n43.world_w,  957,  "NTSC 4:3 is 957 (recorded)");
    check_int(p43.world_w,  958,  "PAL 4:3 is 958 (recorded)");
    check_int(n169.world_w, 1276, "NTSC 16:9 is 1276 (recorded, matches Dolphin)");
    check_int(p169.world_w, 1278, "PAL 16:9 is 1278 (recorded)");
}

/* The clamp at WORLD_WIDTH is dead code in every configuration the console can
 * produce, and this test exists to say so rather than to defend it.
 *
 * At the shipped 6% overscan both widescreen modes land several pixels short.
 * Turning overscan off entirely gets 16:9 to 1279 -- in exact arithmetic that
 * case is 640 / 0.5 = 1280, but scale_x comes out a hair above 0.5 in single
 * precision and the truncating cast takes the rest. So nothing short of a
 * contrived safe area reaches the cap at all.
 *
 * That is worth knowing and not worth removing: it costs one comparison and it
 * is the only thing standing between a future overscan or aspect change and a
 * world width that claims more game than exists. */
static void test_the_clamp(void) {
    printf("playfield: the world-width clamp\n");

    check(projection_compute(ntsc_169()).world_w < WORLD_WIDTH,
          "at the shipped overscan the clamp does not fire on NTSC 16:9");
    check(projection_compute(pal_169()).world_w < WORLD_WIDTH,
          "nor on PAL 16:9");

    /* The closest any real configuration gets. Recorded, not derived: exact
       arithmetic says 1280 and single precision says 1279. */
    Projection full = projection_compute(mode(640, 480, 1, 0));
    check_int(full.world_w, 1279, "even at zero overscan 16:9 stops one short of 1280");
    check(full.world_w <= WORLD_WIDTH, "and so still never exceeds the world");

    /* Only a contrived safe area reaches the cap, which is the one case that
       proves the comparison still works. */
    ProjectionInput wide = mode(640, 480, 1, 0);
    wide.safe_w = 2000;
    check_int(projection_compute(wide).world_w, WORLD_WIDTH,
              "an oversized safe area is clamped to the world that exists");
}

static void test_origin_and_degenerate_input(void) {
    printf("playfield: origin, and inputs that make no sense\n");

    ProjectionInput in = ntsc_43();
    Projection p = projection_compute(in);
    check_int(p.origin_x, in.safe_x, "the origin is the safe area's corner");
    check_int(p.origin_y, in.safe_y, "on both axes");
    check(p.valid, "a real mode produces a valid projection");

    /* Every degenerate dimension must fall back rather than divide by zero.
       The fallback is the identity that pf_init() has always kept. */
    const struct { const char *what; ProjectionInput in; } bad[] = {
        { "zero framebuffer width",   mode(0, 480, 0, OVERSCAN_PCT) },
        { "zero framebuffer height",  mode(640, 0, 0, OVERSCAN_PCT) },
        { "negative framebuffer",     mode(-640, -480, 0, OVERSCAN_PCT) },
        { "zero safe area",           mode(640, 480, 0, 50) }
    };

    for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        Projection d = projection_compute(bad[i].in);
        char what[96];

        snprintf(what, sizeof(what), "%s is rejected", bad[i].what);
        check(!d.valid, what);

        snprintf(what, sizeof(what), "%s leaves the identity scale", bad[i].what);
        check(d.scale_x == 1.0f && d.scale_y == 1.0f, what);

        snprintf(what, sizeof(what), "%s leaves the full world width", bad[i].what);
        check_int(d.world_w, WORLD_WIDTH, what);
    }
}

int main(void) {
    test_the_game_is_the_same_size_everywhere();
    test_pixel_aspect();
    test_world_width();
    test_the_clamp();
    test_origin_and_degenerate_input();
    return report();
}
