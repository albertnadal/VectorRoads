#include <assert.h>
#include <stdio.h>
#include "common.h"
#include "level.h"
#include "lane.h"
#include "tunnel.h"
#include "explosion.h"

int totalRoadObjects;
srRoadObject roadObjects[MAX_ROAD_OBJECTS_PER_LEVEL] = {0};
srExplosionSphere explosionSpheres[EXPLOSION_SPHERES_COUNT] = {0};
bool shipIsExploding = false;
bool shipReachedExit = false;
bool shipOnGround = false;
bool quitGame = false;
int availableJumpsInTheAir = DEFAULT_AVAILABLE_JUMPS_IN_THE_AIR;
int loadedLevel = 0;
int selectedLevel = 0;
srGameScreenType currentGameScreen;

b3BodyId createShipBody(b3WorldId worldId, Vector3 shipPos, Vector3 shipSize) {
  b3BodyDef shipBodyDef = b3DefaultBodyDef();
  shipBodyDef.type = b3_dynamicBody;
  shipBodyDef.position = (b3Pos){shipPos.x, shipPos.y, shipPos.z};

  b3BodyId shipBodyId = b3CreateBody(worldId, &shipBodyDef);
  b3MotionLocks shipBodyLocks = {0};

  shipBodyLocks.angularX = true;
  shipBodyLocks.angularY = true;
  shipBodyLocks.angularZ = true;

  b3Body_SetMotionLocks(shipBodyId, shipBodyLocks);
  b3ShapeDef shipShapeDef = b3DefaultShapeDef();

  shipShapeDef.enableContactEvents = true;
  shipShapeDef.enableHitEvents = true;
  shipShapeDef.baseMaterial.restitution = 0.0f;
  shipShapeDef.baseMaterial.friction = 0.2f;
  shipShapeDef.density = 200.0f;
  shipShapeDef.enableSensorEvents = true;

  b3BoxHull shipBox = b3MakeBoxHull(shipSize.x * 0.5f, shipSize.y * 0.5f, shipSize.z * 0.5f);
  b3CreateHullShape(shipBodyId, &shipShapeDef, &shipBox.base);

  float sphereRadius = 0.15f;
  float offsetX = shipSize.x * 0.30f;
  float offsetZ = shipSize.z * 0.30f;
  float offsetY = -shipSize.y * 0.50f;

  b3Sphere sphere;
  sphere.radius = sphereRadius;
  sphere.center = (b3Vec3){-offsetX, offsetY, -offsetZ};
  b3CreateSphereShape(shipBodyId, &shipShapeDef, &sphere);

  sphere.center = (b3Vec3){offsetX, offsetY, -offsetZ};
  b3CreateSphereShape(shipBodyId, &shipShapeDef, &sphere);

  sphere.center = (b3Vec3){-offsetX, offsetY, offsetZ};
  b3CreateSphereShape(shipBodyId, &shipShapeDef, &sphere);

  sphere.center = (b3Vec3){offsetX, offsetY, offsetZ};
  b3CreateSphereShape(shipBodyId, &shipShapeDef, &sphere);

  return shipBodyId;
}

void playLevel(int level, b3WorldId worldId, b3BodyId shipBodyId, Texture2D *bg, Rectangle *bgSize, b3Vec3 *shipEngineForce, b3Vec3 *shipLateralForce) {
  assert(level >= 0 && "Cannot play a level with invalid id.");

  if ((loadedLevel != level) && (bg->id != 0)) {
    UnloadTexture(*bg);
  }

  char filename[64];
  snprintf(filename, sizeof(filename), "images/level%d.png", level);
  *bg = LoadTexture(filename);
  *bgSize = (Rectangle){0, 0, (float)bg->width, (float)bg->height};
  loadedLevel = level;
  Vector3 shipPos = INITIAL_SHIP_POSITION;
  *shipEngineForce = (b3Vec3){0.0f, 0.0f, 0.0f},
  *shipLateralForce = (b3Vec3){0.0f, 0.0f, 0.0f};
  b3Body_SetLinearVelocity(shipBodyId, (b3Vec3){0.0f, 0.0f, 0.0f});
  b3Body_SetAngularVelocity(shipBodyId, (b3Vec3){0.0f, 0.0f, 0.0f});
  b3Body_SetTransform(shipBodyId, (b3Pos){shipPos.x, shipPos.y, shipPos.z}, b3Body_GetRotation(shipBodyId));

  if(shipIsExploding) {
    destroyExplosionSpheres(explosionSpheres);
  }

  loadLevel(level, worldId, roadObjects, &totalRoadObjects);
  shipIsExploding = false;
  shipReachedExit = false;
}

void updatePilotCamImage(Rectangle* pilotCamSource, float speed, bool exploding, bool reachedExit, float verticalPosition) {
  if (reachedExit) {
    pilotCamSource->x = PILOT_CAM_ROAD_COMPLETED_SPRITE_OFFSET;
  } else if (exploding || (verticalPosition < SHIP_NEAR_FALL_LIMIT_Y)) {
    pilotCamSource->x = PILOT_CAM_NO_SIGNAL_SPRITE_OFFSET;
  } else if (speed < 60.0f) {
    pilotCamSource->x = PILOT_CAM_COOL_FACE_SPRITE_OFFSET;
  } else if (speed < 110.0f) {
    pilotCamSource->x = PILOT_CAM_UPSET_FACE_SPRITE_OFFSET;
  } else if (speed < 160.0f) {
    pilotCamSource->x = PILOT_CAM_SCARY_FACE_SPRITE_OFFSET;
  } else if (speed < 200.0f) {
    pilotCamSource->x = PILOT_CAM_TERROR_FACE_SPRITE_OFFSET;
  }
}

int main() {
#if !DEBUG
  SetTraceLogLevel(LOG_NONE);
#endif
  static const char pressSpaceText[] = "Press SPACE to continue...";
  static const char authorText[] = "Albert Nadal Garriga (2026)";
  static const char versionText[] = "v0.0.1";
  static const char roadCompletedText[] = "Road Completed";

  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(SCR_WIDTH, SCR_HEIGHT, "VectorRoads");
  SetExitKey(KEY_NULL);
  SetTargetFPS(60);
  InitAudioDevice();

  srGameScreenType currentGameScreen = SR_SCREEN_MAIN_MENU;
#if !DEBUG
  Music menuMusic = LoadMusicStream("audio/menu.mp3");
  Music gameplayMusic = LoadMusicStream("audio/gameplay.mp3");
  Music explosionFx = LoadMusicStream("audio/explosion.mp3");
  Music clickFx = LoadMusicStream("audio/click.mp3");
  menuMusic.looping = true;
  gameplayMusic.looping = true;
  explosionFx.looping = false;
  clickFx.looping = false;
#endif

  RenderTexture2D target = LoadRenderTexture(SCR_WIDTH, SCR_HEIGHT);
  SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

  Texture2D mainMenuBg = LoadTexture("images/main_menu.png");
  Texture2D levelMenuBg = LoadTexture("images/level_menu.png");
  Texture2D bg = (Texture2D){0};
  Rectangle bgSize = (Rectangle){0};
  Rectangle mainMenuSize = (Rectangle){0, 0, (float)mainMenuBg.width, (float)mainMenuBg.height};
  Rectangle levelMenuSize = (Rectangle){0, 0, (float)levelMenuBg.width, (float)levelMenuBg.height};
  Rectangle resSize = (Rectangle){0, 0, SCR_WIDTH, SCR_HEIGHT};
  Texture2D hudPanel = LoadTexture("images/hud_panel.png");
  Texture2D pilotCam = LoadTexture("images/pilot_cam.png");
  Rectangle pilotCamSource = {0.0f, 0.0f, PILOT_CAM_WIDTH, PILOT_CAM_HEIGHT};
  Font digitalFont = LoadFont("fonts/digital.ttf");
  Font retroFont = LoadFont("fonts/retro.ttf");
  char speedText[SPEED_TEXT_BUFFER_SIZE];
  Color textFontColor = (Color){83, 244, 65, 255};
  Color levelSelectorColor = (Color){83, 244, 65, 255};

  b3WorldDef worldDef = b3DefaultWorldDef();
  worldDef.gravity = (b3Vec3){0.0f, GRAVITY, 0.0f};
  worldDef.restitutionThreshold = 0.1f;
  b3WorldId worldId = b3CreateWorld(&worldDef);

  Vector3 shipPos = INITIAL_SHIP_POSITION;
  Vector3 shipSize = (Vector3){1.33f, 0.5f, 0.7f};
  b3BodyId shipBodyId = createShipBody(worldId, shipPos, shipSize);
  b3Pos shipPosition;
  b3Vec3 shipSpeed, prevShipSpeed;
  b3Vec3 shipEngineForce = {0.0f, 0.0f, 0.0f},
         shipLateralForce = {0.0f, 0.0f, 0.0f};
  Model shipModel = LoadModel("models/ship.glb");
  b3ContactEvents events;
  b3SensorEvents sensorEvents;

  // Prepare for simulation. Typically we use a time step of 1/60 of a
  // second (60Hz) and 4 sub-steps. This provides a high quality simulation
  // in most game scenarios.
  float timeStep = 1.0f / 60.0f;
  int subStepCount = 4;

  Camera3D camera = {0};
  camera.position = (Vector3){5.5f, 7.5f, shipPos.z + DISTANCE_BETWEEN_SHIP_AND_CAMERA};
  camera.target = (Vector3){5.5f, 2.5f, camera.position.z - CAMERA_TARGET_Z_DISTANCE};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 40.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  bool ignoreEscape = false;
#if !DEBUG
  PlayMusicStream(menuMusic);
#endif

  while (!WindowShouldClose() && !quitGame) {

    if (currentGameScreen == SR_SCREEN_MAIN_MENU) {
#if !DEBUG
      UpdateMusicStream(menuMusic);
#endif
      BeginTextureMode(target);
      ClearBackground(BLACK);
      DrawTexturePro(mainMenuBg, mainMenuSize, resSize, (Vector2){0,0}, 0.0f, WHITE);

      if (!ignoreEscape && IsKeyPressed(KEY_ESCAPE)) {
        quitGame = true;
        continue;
      } else if (!IsKeyDown(KEY_ESCAPE)) {
        ignoreEscape = false;
      }

      if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
        currentGameScreen = SR_SCREEN_LEVEL_MENU;
      }

      if (((int)(GetTime() * 2.0)) % 2 == 0) {
        Vector2 textSize = MeasureTextEx(
          retroFont,
          pressSpaceText,
          PRESS_SPACE_TEXT_FONT_SIZE,
          PRESS_SPACE_TEXT_FONT_SPACING
        );

        DrawTextEx(
          retroFont,
          pressSpaceText,
          (Vector2){
            (SCR_WIDTH - textSize.x) / 2.0f,
            SCR_HEIGHT - (SCR_HEIGHT / 2.5f)
          },
          PRESS_SPACE_TEXT_FONT_SIZE,
          PRESS_SPACE_TEXT_FONT_SPACING,
          textFontColor
        );
      }

      Vector2 authorTextSize = MeasureTextEx(
        retroFont,
        authorText,
        AUTHOR_TEXT_FONT_SIZE,
        AUTHOR_TEXT_FONT_SPACING
      );

      DrawTextEx(
        retroFont,
        authorText,
        (Vector2){
          (SCR_WIDTH - authorTextSize.x) / 2,
          SCR_HEIGHT - 25.0f * ZOOM
        },
        AUTHOR_TEXT_FONT_SIZE,
        AUTHOR_TEXT_FONT_SPACING,
        WHITE
      );

      Vector2 versionTextSize = MeasureTextEx(
        retroFont,
        versionText,
        VERSION_TEXT_FONT_SIZE,
        VERSION_TEXT_FONT_SPACING
      );

      DrawTextEx(
        retroFont,
        versionText,
        (Vector2){
          SCR_WIDTH - versionTextSize.x - 10.0f * ZOOM,
          SCR_HEIGHT - 20.0f * ZOOM
        },
        VERSION_TEXT_FONT_SIZE,
        VERSION_TEXT_FONT_SPACING,
        WHITE
      );
      EndTextureMode();
    } else if (currentGameScreen == SR_SCREEN_GAME_PLAY) {
#if !DEBUG
      UpdateMusicStream(gameplayMusic);
      UpdateMusicStream(explosionFx);
#endif

      if (IsKeyPressed(KEY_ESCAPE)) {
        if(shipIsExploding) {
          destroyExplosionSpheres(explosionSpheres);
        }
        shipIsExploding = false;
        shipReachedExit = false;
        unloadCurrentLevel(roadObjects, &totalRoadObjects);
#if !DEBUG
        StopMusicStream(gameplayMusic);
        StopMusicStream(explosionFx);
        SeekMusicStream(menuMusic, 0.0);
        PlayMusicStream(menuMusic);
#endif
        ignoreEscape = true;
        currentGameScreen = SR_SCREEN_LEVEL_MENU;
        continue;
      }

      b3World_Step(worldId, timeStep, subStepCount);
      shipSpeed = b3Body_GetLinearVelocity(shipBodyId);

      if (shipReachedExit) {
        shipPosition = b3Body_GetPosition(shipBodyId);
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
          if(shipIsExploding) {
            destroyExplosionSpheres(explosionSpheres);
          }
          shipIsExploding = false;
          unloadCurrentLevel(roadObjects, &totalRoadObjects);
#if !DEBUG
          StopMusicStream(gameplayMusic);
          StopMusicStream(explosionFx);
          SeekMusicStream(menuMusic, 0.0);
          PlayMusicStream(menuMusic);
#endif
          currentGameScreen = SR_SCREEN_LEVEL_MENU;
        }

      } else if (!shipIsExploding) {
#if DEBUG
        if (IsKeyDown(KEY_W)) {
          b3Body_SetTransform(shipBodyId, (b3Pos){shipPosition.x, shipPosition.y, shipPosition.z - 25.0f}, b3Body_GetRotation(shipBodyId));
        } else if (IsKeyDown(KEY_S)) {
          b3Body_SetTransform(shipBodyId, (b3Pos){shipPosition.x, shipPosition.y, shipPosition.z + 25.0f}, b3Body_GetRotation(shipBodyId));
        }
#endif

        if (IsKeyDown(KEY_LEFT)) {
          shipLateralForce.x = -1500.0f;
        } else if (IsKeyDown(KEY_RIGHT)) {
          shipLateralForce.x = 1500.0f;
        } else {
          shipLateralForce.x = 0.0f;
        }

        if (IsKeyDown(KEY_UP)) {
          shipEngineForce.z = -2000.0f;
        } else if (IsKeyDown(KEY_DOWN)) {
          shipEngineForce.z = 200.0f;
        }

        if (IsKeyReleased(KEY_UP) || IsKeyReleased(KEY_DOWN)) {
          shipEngineForce.z = 0.0f;
        }

        if (IsKeyPressed(KEY_SPACE) && (availableJumpsInTheAir > 0)) {
          b3Body_ApplyForceToCenter(shipBodyId, (b3Pos){0.0f, 90000.0f, 0.0f}, true);
          availableJumpsInTheAir--;
        }

        b3Body_ApplyForceToCenter(shipBodyId, shipEngineForce, true);
        b3Body_ApplyForceToCenter(shipBodyId, shipLateralForce, true);

        sensorEvents = b3World_GetSensorEvents(worldId);

        for(int i = 0; i < sensorEvents.beginCount && !shipIsExploding && !shipReachedExit; i++) {
          b3SensorBeginTouchEvent *touch = &sensorEvents.beginEvents[i];
          b3BodyId body = b3Shape_GetBody(touch->sensorShapeId);
          if (b3Body_IsValid(body) ) {
            srRoadObject *roadObject = b3Body_GetUserData(body);
            if ((roadObject != NULL) && roadObject->isExit) {
#if DEBUG
              printf("SHIP REACHED EXIT\n");
#endif
              shipReachedExit = true;
              break;
            }
          }
        }

        events = b3World_GetContactEvents(worldId);

        for(int i = 0; i < events.hitCount && !shipIsExploding && !shipReachedExit; i++) {
          b3ContactHitEvent *hit = &events.hitEvents[i];
          if(1.0f - hit->normal.z <= 0.01f) {
#if DEBUG
            printf("FRONTAL COLLISION\n");
#endif
            shipIsExploding = true;
#if !DEBUG
            SeekMusicStream(explosionFx, 0.0);
            PlayMusicStream(explosionFx);
#endif
            float explosionMagnitude = 0.0000001f + ((-prevShipSpeed.z * 0.0003f) / 100.0f);
            createExplosionSpheres(worldId, explosionSpheres, (Vector3){shipPosition.x, shipPosition.y, shipPosition.z}, shipSize, 0.05f, 0.1f, explosionMagnitude);
          }
        }

        for(int i = 0; i < events.beginCount; i++) {
          b3ContactBeginTouchEvent *event = &events.beginEvents[i];
          if (b3Contact_IsValid(event->contactId)) {
            b3ContactData data = b3Contact_GetData(event->contactId);
            for(int j = 0; j < data.manifoldCount; j++) {
              if(data.manifolds[j].normal.y == 1.0f) {
#if DEBUG
                printf("SHIP IS ON THE GROUND\n");
#endif
                shipOnGround = true;
                availableJumpsInTheAir = DEFAULT_AVAILABLE_JUMPS_IN_THE_AIR;
              }
            }
          }
        }

        for(int i = 0; i < events.endCount; i++) {
          b3ContactEndTouchEvent *event = &events.endEvents[i];
          if (b3Shape_IsValid(event->shapeIdA) && b3Shape_IsValid(event->shapeIdB)) {
            b3AABB aabbA = b3Shape_GetAABB(event->shapeIdA);
            b3AABB aabbB = b3Shape_GetAABB(event->shapeIdB);

            if((aabbA.upperBound.y < aabbB.lowerBound.y) ||
              ((aabbA.upperBound.y > aabbB.lowerBound.y) && (aabbA.lowerBound.z > aabbB.upperBound.z)) ||
              ((aabbA.upperBound.y > aabbB.lowerBound.y) && (aabbA.lowerBound.x > aabbB.upperBound.x)) ||
              ((aabbA.upperBound.y > aabbB.lowerBound.y) && (aabbA.upperBound.x < aabbB.lowerBound.x))) {
#if DEBUG
                printf("SHIP IS NOT ON THE GROUND\n");
#endif
              shipOnGround = false;
            }
          }
        }

        shipPosition = b3Body_GetPosition(shipBodyId);
        if (shipPosition.y < SHIP_FALL_LIMIT_Y) {
          playLevel(selectedLevel, worldId, shipBodyId, &bg, &bgSize, &shipEngineForce, &shipLateralForce); // Restart level
        }
      }
      prevShipSpeed = shipSpeed;
      snprintf(speedText, sizeof(speedText), "%.0f", fabsf(shipSpeed.z));

      BeginTextureMode(target);
      ClearBackground(BLACK);
      DrawTexturePro(bg, bgSize, resSize, (Vector2){0,0}, 0.0f, WHITE);

      BeginMode3D(camera);

      if (!shipIsExploding && !shipReachedExit) {
        DrawModelEx(
          shipModel,
          (Vector3){shipPosition.x, shipPosition.y - 0.25f, shipPosition.z},
          (Vector3){0.0f, 0.0f, 0.0f},
          0.0f,
          (Vector3){0.3f, 0.3f, 0.3f},
          WHITE
        );
      }

#if DEBUG
      DrawCubeWires((Vector3){shipPosition.x, shipPosition.y, shipPosition.z}, shipSize.x, shipSize.y, shipSize.z, BLACK);
#endif
      camera.position.z = shipPosition.z + DISTANCE_BETWEEN_SHIP_AND_CAMERA;
      camera.target.z = camera.position.z - CAMERA_TARGET_Z_DISTANCE;

      for (int j = 0; j < totalRoadObjects; j++) {
        srRoadObject *obj = &roadObjects[j];
#if !DEBUG
        if(!obj->isExit) {
#endif
          b3Pos pos = b3Body_GetPosition(obj->box3DBodyId);

          if(obj->type == SR_ROAD_OBJECT_LANE) {
            DrawCube((Vector3){pos.x, pos.y, pos.z}, obj->size.x, obj->size.y, obj->size.z, obj->color);
            DrawCubeWires((Vector3){pos.x, pos.y, pos.z}, obj->size.x, obj->size.y, obj->size.z, BLACK);
          } else if (obj->type == SR_ROAD_OBJECT_TUNNEL) {
            DrawModel(obj->model, (Vector3){pos.x, pos.y, pos.z}, 1.0f, obj->color);
            drawTunnelWires((Vector3){pos.x, pos.y, pos.z}, (Vector3){obj->size.x, obj->size.y, obj->size.z}, BLACK);
          }
#if !DEBUG
        }
#endif
      }

      if (shipIsExploding) {
        for (int i = 0; i < EXPLOSION_SPHERES_COUNT; i++) {
          if (explosionSpheres[i].alpha == 0.0f) {
            continue;
          }

          if (explosionSpheres[i].alpha > 1) {
            explosionSpheres[i].alpha -= 1.0f;
            b3Pos pos = b3Body_GetPosition(explosionSpheres[i].box3DBodyId);
            explosionSpheres[i].color.a = explosionSpheres[i].alpha;
            DrawSphere((Vector3){pos.x, pos.y, pos.z}, explosionSpheres[i].radius, explosionSpheres[i].color);
          } else {
            explosionSpheres[i].alpha = 0.0f;
            playLevel(selectedLevel, worldId, shipBodyId, &bg, &bgSize, &shipEngineForce, &shipLateralForce); // Restart level
            break;
          }
        }
      }

      EndMode3D();
#if DEBUG
      DrawFPS(16, 16);
#endif

      updatePilotCamImage(&pilotCamSource, fabsf(shipSpeed.z), shipIsExploding, shipReachedExit, shipPosition.y);
      int hudX = (SCR_WIDTH - hudPanel.width * ZOOM) / 2;
      int hudY = SCR_HEIGHT - hudPanel.height * ZOOM - 20 * ZOOM;
      DrawTexturePro(pilotCam, pilotCamSource, (Rectangle){hudX + hudPanel.width * ZOOM - PILOT_CAM_WIDTH * ZOOM - 40 * ZOOM, hudY + 22 * ZOOM, PILOT_CAM_WIDTH * ZOOM, PILOT_CAM_HEIGHT * ZOOM}, (Vector2){0, 0}, 0.0f, WHITE);
      DrawTextureEx(hudPanel, (Vector2){hudX, hudY}, 0.0f, ZOOM, WHITE);

      Vector2 textSize = MeasureTextEx(
        digitalFont,
        speedText,
        SPEED_TEXT_FONT_SIZE,
        SPEED_TEXT_FONT_SPACING
      );

      DrawTextEx(
        digitalFont,
        speedText,
        (Vector2){
          hudX + (hudPanel.width * ZOOM / 2) - textSize.x - 50.0f * ZOOM ,
          hudY + (hudPanel.height * ZOOM / 4) - 1
        },
        SPEED_TEXT_FONT_SIZE,
        SPEED_TEXT_FONT_SPACING,
        textFontColor
      );

      if (shipReachedExit) {
        textSize = MeasureTextEx(
          retroFont,
          roadCompletedText,
          ROAD_COMPLETED_TEXT_FONT_SIZE,
          ROAD_COMPLETED_TEXT_FONT_SPACING
        );

        DrawTextEx(
          retroFont,
          roadCompletedText,
          (Vector2){
            (SCR_WIDTH - textSize.x) / 2,
            SCR_HEIGHT - (SCR_HEIGHT / 2)
          },
          ROAD_COMPLETED_TEXT_FONT_SIZE,
          ROAD_COMPLETED_TEXT_FONT_SPACING,
          textFontColor
        );
      }

      EndTextureMode();

    } else if (currentGameScreen == SR_SCREEN_LEVEL_MENU) {
#if !DEBUG
      UpdateMusicStream(menuMusic);
      UpdateMusicStream(clickFx);
#endif
      BeginTextureMode(target);
      ClearBackground(BLACK);
      DrawTexturePro(levelMenuBg, levelMenuSize, resSize, (Vector2){0,0}, 0.0f, WHITE);
      bool arrowKeyPressed = false;

      if (!ignoreEscape && IsKeyPressed(KEY_ESCAPE)) {
        ignoreEscape = true;
        currentGameScreen = SR_SCREEN_MAIN_MENU;
        continue;
      } else if (!IsKeyDown(KEY_ESCAPE)) {
        ignoreEscape = false;
      }

      if (IsKeyPressed(KEY_LEFT)) {
        selectedLevel = ((selectedLevel - (TOTAL_LEVELS / 2)) + TOTAL_LEVELS) % TOTAL_LEVELS;
        arrowKeyPressed = true;
      } else if (IsKeyPressed(KEY_RIGHT)) {
        selectedLevel = ((TOTAL_LEVELS / 2) + selectedLevel) % TOTAL_LEVELS;
        arrowKeyPressed = true;
      }

      if (IsKeyPressed(KEY_UP)) {
        selectedLevel = (selectedLevel - 1 + TOTAL_LEVELS) % TOTAL_LEVELS;
        arrowKeyPressed = true;
      } else if (IsKeyPressed(KEY_DOWN)) {
        selectedLevel = (selectedLevel + 1) % TOTAL_LEVELS;
        arrowKeyPressed = true;
      }

#if !DEBUG
      if (arrowKeyPressed) {
        SeekMusicStream(clickFx, 0.0);
        PlayMusicStream(clickFx);
      }
#endif

      if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
#if !DEBUG
        StopMusicStream(menuMusic);
        StopMusicStream(clickFx);
        SeekMusicStream(gameplayMusic, 0.0);
        PlayMusicStream(gameplayMusic);
#endif
        currentGameScreen = SR_SCREEN_GAME_PLAY;
        playLevel(selectedLevel, worldId, shipBodyId, &bg, &bgSize, &shipEngineForce, &shipLateralForce);
      }

      Rectangle levelSelectorRect = (Rectangle){LEVEL_MENU_SELECTOR_LEFT_MARGIN + ((selectedLevel < (TOTAL_LEVELS / 2)) ? 0 : LEVEL_MENU_SELECTOR_MID_MARGIN), LEVEL_MENU_SELECTOR_TOP_MARGIN + (selectedLevel % (TOTAL_LEVELS / 2) * LEVEL_MENU_SELECTOR_VERTICAL_MARGIN), LEVEL_MENU_SELECTOR_WIDTH, LEVEL_MENU_SELECTOR_HEIGHT};
      DrawRectangleLinesEx(levelSelectorRect, 5.0f, levelSelectorColor);
      EndTextureMode();
    }

    BeginDrawing();
    DrawTexturePro(
        target.texture,
        (Rectangle){0, 0, SCR_WIDTH, -SCR_HEIGHT},
        (Rectangle){0, 0, SCR_WIDTH, SCR_HEIGHT},
        (Vector2){0,0},
        0.0f,
        WHITE
    );
    EndDrawing();
  }

  b3DestroyWorld(worldId);
  UnloadModel(shipModel);
  UnloadTexture(bg);
  UnloadTexture(hudPanel);
  UnloadTexture(pilotCam);
  UnloadTexture(levelMenuBg);
  UnloadTexture(mainMenuBg);
  UnloadFont(digitalFont);
  UnloadFont(retroFont);
#if !DEBUG
  UnloadMusicStream(menuMusic);
  UnloadMusicStream(gameplayMusic);
#endif
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
