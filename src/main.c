#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500
/**
 * Real life RPM examples:
 * - Boeing E-3 Sentry: 6 RPM
 * - Air Surveillance Radar: 12 to 15 RPM
 * - En-Route Radar: 4 to 6 RPM
 */
#define RADAR_RPM 12.5f
#define RADAR_CIRCLES_COUNT 4

typedef struct RadarData
{
    int screenWidth;
    int screenHeight;
    Vector2 center;

    int radius;
    int radiusStep;

    // Radians
    float angle;
    // Radians per second
    float angularSpeed;

    // Contains radar circles and center dot
    RenderTexture2D radarTexture;
    // Contains sweep line and phosphor fading effect
    RenderTexture2D sweepEffectTexture;
    // Contains the rotated sweepEffectTexture
    RenderTexture2D displayTexture;

    // Flip texture vertically
    Rectangle sweepSourceRect;
    Rectangle sweepDestRect;
    // Origin for the rotation
    Vector2 sweepOrigin;
} RadarData;

/**
 * Initializes RadarData fields.
 */
void InitializeRadarData(RadarData *that)
{
    that->screenWidth = SCREEN_WIDTH;
    that->screenHeight = SCREEN_HEIGHT;
    that->center = (Vector2){that->screenWidth / 2, that->screenHeight / 2};

    that->radius = that->screenHeight / 2;
    that->radiusStep = that->radius / RADAR_CIRCLES_COUNT;

    that->angle = 0.0f;
    that->angularSpeed = RADAR_RPM * (2.0f * PI / 60.0f);

    that->radarTexture = LoadRenderTexture(that->screenWidth, that->screenHeight);
    that->sweepEffectTexture = LoadRenderTexture(that->screenWidth, that->screenHeight);
    that->displayTexture = LoadRenderTexture(that->screenWidth, that->screenHeight);

    that->sweepSourceRect = (Rectangle){0, 0, (float)that->sweepEffectTexture.texture.width, (float)-that->sweepEffectTexture.texture.height};
    that->sweepDestRect = (Rectangle){that->center.x, that->center.y, (float)that->sweepEffectTexture.texture.width, (float)that->sweepEffectTexture.texture.height};
    that->sweepOrigin = (Vector2){that->sweepEffectTexture.texture.width / 2.0f, that->sweepEffectTexture.texture.height / 2.0f};
}

/**
 * Updates RadarData fields. To be called in main loop.
 */
void UpdateRadarData(RadarData *that, const float deltaTime)
{
    that->angle -= that->angularSpeed * deltaTime;

    // Reset the angle when it completes a full circle (2*PI radians)
    if (that->angle <= 2 * PI)
        that->angle += 2 * PI;
}

/**
 * Updates RadarData fields on window resize.
 */
void UpdateOnResizeRadarData(RadarData *that)
{
    that->screenWidth = GetScreenWidth();
    that->screenHeight = GetScreenHeight();
    that->center = (Vector2){that->screenWidth / 2, that->screenHeight / 2};

    that->radius = that->screenHeight / 2;
    that->radiusStep = that->radius / RADAR_CIRCLES_COUNT;

    UnloadRenderTexture(that->radarTexture);
    UnloadRenderTexture(that->sweepEffectTexture);
    UnloadRenderTexture(that->displayTexture);

    that->radarTexture = LoadRenderTexture(that->screenWidth, that->screenHeight);
    that->sweepEffectTexture = LoadRenderTexture(that->screenWidth, that->screenHeight);
    that->displayTexture = LoadRenderTexture(that->screenWidth, that->screenHeight);

    that->sweepSourceRect = (Rectangle){
        0,
        0,
        (float)that->sweepEffectTexture.texture.width,
        (float)-that->sweepEffectTexture.texture.height,
    }; // Flip texture vertically
    that->sweepDestRect = (Rectangle){
        that->center.x,
        that->center.y,
        (float)that->sweepEffectTexture.texture.width,
        (float)that->sweepEffectTexture.texture.height,
    };
    that->sweepOrigin = (Vector2){
        that->sweepEffectTexture.texture.width / 2.0f,
        that->sweepEffectTexture.texture.height / 2.0f,
    };
}

/**
 * Destroys, cleans, unloads RadarData fields.
 */
void DestroyRadarData(RadarData *that)
{
    UnloadRenderTexture(that->radarTexture);
    UnloadRenderTexture(that->sweepEffectTexture);
    UnloadRenderTexture(that->displayTexture);
}

/**
 * Draws the Sweep effect on sweepEffectTexture.
 */
void DrawSweepEffectTexture(RadarData *radarData)
{
    BeginTextureMode(radarData->sweepEffectTexture);
    ClearBackground(BLANK);

    const float maxAngle = 180 * DEG2RAD;
    const float angleStep = 0.001f; // Low value means no gap between lines

    for (float sweepAngle = 0; sweepAngle <= maxAngle; sweepAngle += angleStep)
    {
        Vector2 sweepEnd = {
            radarData->center.x + radarData->radius * cos(sweepAngle),
            radarData->center.y + radarData->radius * sin(sweepAngle),
        };

        float alpha = 1.0f - (sweepAngle / maxAngle);

        DrawLineV(radarData->center, sweepEnd, Fade(GREEN, alpha));
    }

    EndTextureMode();
}

/**
 * Draws the radar circles on radarTexture.
 */
void DrawRadarTexture(RadarData *radarData)
{
    BeginTextureMode(radarData->radarTexture);
    ClearBackground(BLANK);

    // Draw range circles
    for (int i = 0; i < RADAR_CIRCLES_COUNT; i++)
        DrawCircleLinesV(radarData->center, radarData->radius - radarData->radiusStep * i, GREEN);

    // Draw the circle representing the radar antenna
    DrawCircleV(radarData->center, 6.5, GREEN);

    EndTextureMode();
}

/**
 * Draw the rotated sweepEffectTexture on displayTexture.
 */
void DrawDisplayTexture(RadarData *radarData)
{
    BeginTextureMode(radarData->displayTexture);
    ClearBackground(BLANK);

    DrawTexturePro(
        radarData->sweepEffectTexture.texture,
        radarData->sweepSourceRect,
        radarData->sweepDestRect,
        radarData->sweepOrigin,
        radarData->angle * RAD2DEG,
        WHITE);

    EndTextureMode();
}

/**
 * ToDo:
 * - add planes
 */

int main(void)
{
    const char *TITLE = "Raydar";
    RadarData radarData;

    SetWindowState(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TITLE);

    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    InitializeRadarData(&radarData);
    DrawRadarTexture(&radarData);
    DrawSweepEffectTexture(&radarData);

    while (!WindowShouldClose())
    {
        const float deltaTime = GetFrameTime();

        if (IsWindowResized())
        {
            UpdateOnResizeRadarData(&radarData);
            DrawRadarTexture(&radarData);
            DrawSweepEffectTexture(&radarData);
        }

        UpdateRadarData(&radarData, deltaTime);

        // const Vector2 sweepLineEnd = {
        //     radarData.center.x + radarData.radius * cos(-radarData.angle),
        //     radarData.center.y + radarData.radius * sin(-radarData.angle),
        // };

        DrawDisplayTexture(&radarData);

        BeginDrawing();

        ClearBackground(BLACK);

        DrawTexture(radarData.displayTexture.texture, 0, 0, WHITE);
        DrawTexture(radarData.radarTexture.texture, 0, 0, WHITE);

        DrawFPS(0, 0);

        EndDrawing();
    }

    DestroyRadarData(&radarData);

    CloseWindow();

    return 0;
}
