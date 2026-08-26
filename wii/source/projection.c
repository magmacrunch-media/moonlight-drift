#include "projection.h"
#include "config.h"

Projection projection_compute(ProjectionInput in) {
    Projection p;

    /* The identity, which is what playfield.c held before it computed anything
       and what it keeps if the input turns out to be unusable. */
    p.scale_x  = 1.0f;
    p.scale_y  = 1.0f;
    p.origin_x = in.safe_x;
    p.origin_y = in.safe_y;
    p.world_w  = WORLD_WIDTH;
    p.par      = 1.0f;
    p.valid    = 0;

    if (in.fb_w <= 0 || in.fb_h <= 0 || in.safe_w <= 0 || in.safe_h <= 0) {
        return p;
    }

    /* A framebuffer pixel is not square unless the frame's own ratio already
       matches the display's. NTSC 640x480 on a 4:3 set is the one case where it
       does; PAL 640x528 is 1.1 times taller than wide, and any 16:9 mode
       stretches the same frame by 4/3. Deriving the horizontal scale through
       this is what makes both 4:3 modes agree with each other and lets a
       widescreen set show the whole 1280 without a second code path. */
    const float display_aspect = in.widescreen ? (16.0f / 9.0f) : (4.0f / 3.0f);
    const float par = display_aspect / ((float)in.fb_w / (float)in.fb_h);
    p.par = par;

    /* Fill the safe area vertically: the gap and the player keep exactly the
       share of the screen they have on the web, which is what the game feels
       like. The width follows from that, and is simply cropped. */
    p.scale_y = (float)in.safe_h / (float)WORLD_HEIGHT;
    p.scale_x = p.scale_y / par;

    p.world_w = (int)((float)in.safe_w / p.scale_x);

    /* Never show more than the source game has. Measured, this never fires: 16:9
       is 1276 on NTSC and 1278 on PAL at the shipped overscan, and even with
       overscan off it stops at 1279, because scale_x lands a hair above 0.5 in
       single precision and the truncating cast takes the rest. Kept anyway --
       one comparison, and the only thing between a future overscan or aspect
       change and a world width that claims more game than exists. */
    if (p.world_w > WORLD_WIDTH) p.world_w = WORLD_WIDTH;

    p.valid = 1;
    return p;
}
