#ifndef CONFIG_H
#define CONFIG_H

#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

#define CANVAS_WIDTH        640
#define CANVAS_HEIGHT       480

#define GAP                 180
#define OBSTACLE_WIDTH      60
#define OBSTACLE_SPEED      3
#define OBSTACLE_SPAWN_INTERVAL  120

#define PLAYER_X            80
#define PLAYER_Y_INITIAL    200

#define DEFAULT_THRUST      -0.6f
#define DEFAULT_GRAVITY     0.4f
#define DEFAULT_MAX_VELOCITY 10.0f

#define FLAME_HEIGHT        15
#define MIN_OBSTACLE_HEIGHT 50
#define MAX_OBSTACLE_HEIGHT (SCREEN_HEIGHT - GAP - MIN_OBSTACLE_HEIGHT)

#define THEME_CHANGE_INTERVAL   10

#define STAR_COUNT          60

/* Homebrew Channel app directory. The engine derives sd:/apps/<APP_NAME>/...
   from this, so asset and save paths are not repeated around the codebase. */
#define APP_NAME            "moonlight-drift"
#define HIGH_SCORE_COUNT    10

/* Unattended test hooks. All off in a normal build; together they let a scripted
   run reach and hold gameplay with no controller, which is how the entry crash
   was finally cornered -- every suspect feature ran for thousands of frames.
   Reaching gameplay by hand needs three button presses into an emulator window,
   and without a heartbeat a silent log cannot tell a crash from a game sitting
   quietly on a screen with nothing left to say. */
#define AUTOSTART_GAMEPLAY          0   /* boot straight into a run */
#define DEBUG_IMMORTAL              0   /* ignore death, so a run continues unattended */
#define DEBUG_HEARTBEAT_FRAMES      0   /* print frame/obstacles/score every N frames; 0 off */

/* Percent of each screen edge assumed lost to TV overscan. Raise if the border
   or bottom line of text is cut off on your set. */
#define OVERSCAN_PCT        6

#endif
