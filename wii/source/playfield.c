#include <ogc/conf.h>
#include <stdio.h>
#include "playfield.h"
#include "projection.h"
#include "config.h"
#include "magnolia.h"

static float scale_x = 1.0f;
static float scale_y = 1.0f;
static int   origin_x = 0;
static int   origin_y = 0;
static int   world_w  = WORLD_WIDTH;

/* Everything this function does is ask the console what it is, hand the answers
   to projection_compute(), and report. The arithmetic lives in projection.c so
   that it can be run somewhere other than a Wii -- a wrong scale here does not
   crash, it plays the game at the wrong size on a video mode the author may not
   own, and pf_world_width() feeds the obstacle spawn and cull edges as well as
   the drawing. */
void pf_init(void) {
    ProjectionInput in;
    in.fb_w   = renderer_screen_width();
    in.fb_h   = renderer_screen_height();
    in.safe_x = ui_safe_x();
    in.safe_y = ui_safe_y();
    in.safe_w = ui_safe_w();
    in.safe_h = ui_safe_h();
    in.widescreen = (CONF_GetAspectRatio() == CONF_ASPECT_16_9);

    const Projection p = projection_compute(in);

    scale_x  = p.scale_x;
    scale_y  = p.scale_y;
    origin_x = p.origin_x;
    origin_y = p.origin_y;
    world_w  = p.world_w;

    /* One line, once, in the same spirit as the rest of the bring-up reporting:
       every geometry complaint about this game reduces to these numbers, and
       they differ by video mode and by what the TV is set to. Scales are in
       thousandths to keep floats out of printf. */
    printf("playfield: fb %dx%d safe %dx%d+%d+%d par %d/1000 "
           "scale %d/1000 x %d/1000 world %dx%d\n",
           in.fb_w, in.fb_h, in.safe_w, in.safe_h, origin_x, origin_y,
           (int)(p.par * 1000.0f), (int)(scale_x * 1000.0f),
           (int)(scale_y * 1000.0f), world_w, WORLD_HEIGHT);
}

float pf_x(float world_x) { return (float)origin_x + world_x * scale_x; }
float pf_y(float world_y) { return (float)origin_y + world_y * scale_y; }
float pf_w(float w)       { return w * scale_x; }
float pf_h(float h)       { return h * scale_y; }

float pf_scale_x(void) { return scale_x; }
float pf_scale_y(void) { return scale_y; }

int pf_world_width(void) { return world_w; }
