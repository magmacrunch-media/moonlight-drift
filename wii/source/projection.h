#ifndef PROJECTION_H
#define PROJECTION_H

/* The arithmetic behind playfield.c, separated from the console it runs on.
 *
 * playfield.c has to ask libogc what video mode is running and magnolia what
 * the safe area is. This does not: hand it those answers and it computes the
 * projection, which makes the one piece of geometry in this game that cannot be
 * checked by looking at a screen checkable on a laptop instead.
 *
 * Worth separating because the failure mode is silent and remote. A wrong scale
 * does not crash; it plays the game at the wrong size on a video mode the author
 * does not own, and pf_world_width() feeds the obstacle spawn edge, the cull
 * edge and the starfield spread, so it changes how the game *plays* and not just
 * how it looks.
 */

typedef struct {
    /* The framebuffer as the video mode reports it: 640x480 on NTSC,
       640x528 on PAL. Not square pixels, which is the whole problem. */
    int fb_w, fb_h;

    /* The TV-safe rectangle inside it, from magnolia's ui_safe_*(). The
       destination is this rather than the raw framebuffer, because the top and
       bottom edges of the world are lethal and drawing them at the framebuffer
       edge puts the kill line behind the bezel. */
    int safe_x, safe_y, safe_w, safe_h;

    /* What the console is set to, not what the framebuffer is shaped like.
       1 for 16:9, 0 for 4:3. */
    int widescreen;
} ProjectionInput;

typedef struct {
    float scale_x, scale_y;
    int   origin_x, origin_y;
    int   world_w;

    /* Pixel-aspect ratio: how much wider than tall a framebuffer pixel is once
       the display has stretched it. 1.0 only on NTSC 4:3. Carried out because
       the bring-up diagnostic prints it, and because it is the single value the
       whole projection turns on -- worth being able to assert directly. */
    float par;

    /* 0 when the input was degenerate -- a framebuffer or safe area with a
       non-positive dimension. The caller keeps the identity projection in that
       case rather than dividing by zero, which is what pf_init() has always
       done; this just says so out loud. */
    int   valid;
} Projection;

Projection projection_compute(ProjectionInput in);

#endif
