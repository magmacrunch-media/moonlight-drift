#ifndef CONFIG_H
#define CONFIG_H

#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

#define CANVAS_WIDTH        640
#define CANVAS_HEIGHT       480

#define GAP                 120
#define OBSTACLE_WIDTH      60
#define OBSTACLE_SPEED      3
#define OBSTACLE_SPAWN_INTERVAL  120

#define PLAYER_X            80
#define PLAYER_Y_INITIAL    200

#define DEFAULT_THRUST      -0.6f
#define DEFAULT_GRAVITY     0.4f
#define DEFAULT_MAX_VELOCITY 10.0f

#define MIN_OBSTACLE_HEIGHT 35
#define MAX_OBSTACLE_HEIGHT (SCREEN_HEIGHT - GAP - MIN_OBSTACLE_HEIGHT)

#define THEME_CHANGE_INTERVAL   10

#define STAR_COUNT          60

#define MAX_SCORES          10
#define SCORES_PATH         "sd:/apps/moonlight-drift/scores.json"

#endif
