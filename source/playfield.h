#ifndef PLAYFIELD_H
#define PLAYFIELD_H

/* Projection from the source game's world onto whatever the console is actually
 * displaying.
 *
 * Moonlight Drift is authored against a 1280x720 canvas -- that is the size the
 * web build's <canvas> declares, and every constant in config.h (GAP,
 * OBSTACLE_WIDTH, OBSTACLE_SPEED, the character hitboxes) is a measurement in
 * that space. Physics, collision and scoring all stay there, unchanged and
 * byte-identical to js/. Only drawing comes through here.
 *
 * Running those constants straight onto a 640x480 framebuffer, which is what
 * this port used to do, is not a port of the game -- it is a different game. The
 * gap goes from a quarter of the screen to over a third, the player from 4.9% of
 * the height to 7.3%, and an obstacle crosses in half the frames while still
 * spawning every 120, so under two are ever on screen instead of nearly four.
 *
 * The destination is the TV-safe rectangle rather than the raw framebuffer. That
 * is not cosmetic: the top and bottom edges of the world are lethal, and drawing
 * them at the framebuffer edge puts the kill line behind the bezel on a CRT and
 * 48 pixels away from where it actually is on PAL.
 */

void pf_init(void);

/* World coordinates and lengths into screen pixels. */
float pf_x(float world_x);
float pf_y(float world_y);
float pf_w(float world_w);
float pf_h(float world_h);

/* The two factors differ whenever the framebuffer's pixels are not square, which
   is every 16:9 mode. Rectangles take both; so does the player sprite. */
float pf_scale_x(void);
float pf_scale_y(void);

/* How much of the 1280-wide world fits across the safe area at the current
   scale -- the spawn edge, the cull edge, and how far the starfield spreads.
   Roughly 960 on any 4:3 set (NTSC and PAL agree), the full 1280 on 16:9. */
int pf_world_width(void);

#endif
