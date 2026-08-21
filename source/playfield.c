#include <ogc/conf.h>
#include <stdio.h>
#include "playfield.h"
#include "config.h"
#include "magnolia.h"

static float scale_x = 1.0f;
static float scale_y = 1.0f;
static int   origin_x = 0;
static int   origin_y = 0;
static int   world_w  = WORLD_WIDTH;

void pf_init(void) {
    const int fb_w   = renderer_screen_width();
    const int fb_h   = renderer_screen_height();
    const int safe_w = ui_safe_w();
    const int safe_h = ui_safe_h();

    origin_x = ui_safe_x();
    origin_y = ui_safe_y();

    if (fb_w <= 0 || fb_h <= 0 || safe_w <= 0 || safe_h <= 0) return;

    /* A framebuffer pixel is not square unless the frame's own ratio already
       matches the display's. NTSC 640x480 on a 4:3 set is the one case where it
       does; PAL 640x528 is 1.1 times taller than wide, and any 16:9 mode
       stretches the same frame by 4/3. Deriving the horizontal scale through
       this is what makes both 4:3 modes agree with each other and lets a
       widescreen set show the whole 1280 without a second code path. */
    const float display_aspect =
        (CONF_GetAspectRatio() == CONF_ASPECT_16_9) ? (16.0f / 9.0f)
                                                    : (4.0f / 3.0f);
    const float par = display_aspect / ((float)fb_w / (float)fb_h);

    /* Fill the safe area vertically: the gap and the player keep exactly the
       share of the screen they have on the web, which is what the game feels
       like. The width follows from that, and is simply cropped. */
    scale_y = (float)safe_h / (float)WORLD_HEIGHT;
    scale_x = scale_y / par;

    world_w = (int)((float)safe_w / scale_x);
    /* Never show more than the source game has. 16:9 lands a pixel or two over
       1280 and there is nothing out there to show. */
    if (world_w > WORLD_WIDTH) world_w = WORLD_WIDTH;

    /* One line, once, in the same spirit as the rest of the bring-up reporting:
       every geometry complaint about this game reduces to these numbers, and
       they differ by video mode and by what the TV is set to. Scales are in
       thousandths to keep floats out of printf. */
    printf("playfield: fb %dx%d safe %dx%d+%d+%d par %d/1000 "
           "scale %d/1000 x %d/1000 world %dx%d\n",
           fb_w, fb_h, safe_w, safe_h, origin_x, origin_y,
           (int)(par * 1000.0f), (int)(scale_x * 1000.0f),
           (int)(scale_y * 1000.0f), world_w, WORLD_HEIGHT);
}

float pf_x(float world_x) { return (float)origin_x + world_x * scale_x; }
float pf_y(float world_y) { return (float)origin_y + world_y * scale_y; }
float pf_w(float w)       { return w * scale_x; }
float pf_h(float h)       { return h * scale_y; }

float pf_scale_x(void) { return scale_x; }
float pf_scale_y(void) { return scale_y; }

int pf_world_width(void) { return world_w; }
