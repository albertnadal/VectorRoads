#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <raylib/raylib.h>
#include <box3d/box3d.h>

#define DEBUG 0
#define ZOOM 1.0f // 1.0f = 1280x800px | 1.09375f = 1400x875px | 1.125f = 1440x900px | 1.5f = 1920×1200px (Full HD) | 2.0f = 2560×1600px (2K) | 2.25f = 2880×1800px | 3.0f = 3840×2400px (4K)

#define MAX_ROAD_OBJECTS_PER_LEVEL 300
#define DISTANCE_BETWEEN_SHIP_AND_CAMERA 15.0f
#define CAMERA_TARGET_Z_DISTANCE 19.0f
#define GRAVITY 3 * -9.80665f
#define TUNNEL_SLICES 9
#define EXPLOSION_SPHERES_COUNT 50
#define INITIAL_SHIP_POSITION (Vector3){5.5f, 3.5f, 10.0f}
#define SHIP_FALL_LIMIT_Y -60.0f
#define SHIP_NEAR_FALL_LIMIT_Y -10.0f
#define SPEED_TEXT_BUFFER_SIZE 12
#define DEFAULT_AVAILABLE_JUMPS_IN_THE_AIR 2
#define TOTAL_LEVELS 8
#define PILOT_CAM_WIDTH 165.0f
#define PILOT_CAM_HEIGHT 108.0f
#define PILOT_CAM_COOL_FACE_SPRITE_OFFSET 0.0f
#define PILOT_CAM_UPSET_FACE_SPRITE_OFFSET 165.0f
#define PILOT_CAM_SCARY_FACE_SPRITE_OFFSET 330.0f
#define PILOT_CAM_TERROR_FACE_SPRITE_OFFSET 495.0f
#define PILOT_CAM_ROAD_COMPLETED_SPRITE_OFFSET 660.0f
#define PILOT_CAM_NO_SIGNAL_SPRITE_OFFSET 831.0f

#define SCR_WIDTH 1280.0f * ZOOM
#define SCR_HEIGHT 800.0f * ZOOM
#define SPEED_TEXT_FONT_SIZE 69.0f * ZOOM
#define SPEED_TEXT_FONT_SPACING 5.0f * ZOOM
#define PRESS_SPACE_TEXT_FONT_SIZE 26.0f * ZOOM
#define PRESS_SPACE_TEXT_FONT_SPACING -1.0f * ZOOM
#define AUTHOR_TEXT_FONT_SIZE 16.0f * ZOOM
#define AUTHOR_TEXT_FONT_SPACING 0.0f * ZOOM
#define VERSION_TEXT_FONT_SIZE 10.0f * ZOOM
#define VERSION_TEXT_FONT_SPACING 0.0f * ZOOM
#define ROAD_COMPLETED_TEXT_FONT_SIZE 26.0f * ZOOM
#define ROAD_COMPLETED_TEXT_FONT_SPACING -1.0f * ZOOM
#define LEVEL_MENU_SELECTOR_WIDTH 225.0f * ZOOM
#define LEVEL_MENU_SELECTOR_HEIGHT 141.0f * ZOOM
#define LEVEL_MENU_SELECTOR_LEFT_MARGIN 192.0f * ZOOM
#define LEVEL_MENU_SELECTOR_MID_MARGIN 661.0f * ZOOM
#define LEVEL_MENU_SELECTOR_TOP_MARGIN 52.0f * ZOOM
#define LEVEL_MENU_SELECTOR_VERTICAL_MARGIN 196.0f * ZOOM

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef enum {
  SR_SCREEN_MAIN_MENU = 0,
  SR_SCREEN_LEVEL_MENU = 1,
  SR_SCREEN_GAME_PLAY = 2
} srGameScreenType;

typedef enum {
  SR_ROAD_OBJECT_NONE = 0,
  SR_ROAD_OBJECT_LANE = 1,
  SR_ROAD_OBJECT_TUNNEL = 2
} srRoadObjectType;

typedef struct {
  Vector3 initialPosition;
  Vector3 size;
  Color color;
  srRoadObjectType type;
  b3BodyId box3DBodyId;
  Model model;
  bool isExit;
} srRoadObject;

typedef struct {
  Vector3 initialPosition;
  float radius;
  Color color;
  unsigned char alpha;
  b3BodyId box3DBodyId;
} srExplosionSphere;

#endif