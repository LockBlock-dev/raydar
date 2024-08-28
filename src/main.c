#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500

void DrawSweepLineFade(RenderTexture2D *sweepTexture, float sweepWidth, Vector2 center, int radius)
{
    BeginTextureMode(*sweepTexture);

    ClearBackground(BLANK);

    // Draw a series of lines to fill the sector
    for (float sweepAngle = -sweepWidth / 2; sweepAngle <= sweepWidth / 2; sweepAngle += 0.001f)
    {
        Vector2 sweepEnd = {
            center.x + radius * cos(sweepAngle),
            center.y + radius * sin(sweepAngle),
        };

        DrawLineV(center, sweepEnd, GREEN);
    }

    EndTextureMode();
}

/**
 * ToDo:
 * - use delta time for phosphor fade effect
 * - add planes
 * - (put all vars into a struct? at least find an elegant way to update things when screen is resized)
 */

int main(void)
{
    const char *TITLE = "Raydar";
    int screenWidth = SCREEN_WIDTH;
    int screenHeight = SCREEN_HEIGHT;

    SetWindowState(FLAG_MSAA_4X_HINT);

    InitWindow(screenWidth, screenHeight, TITLE);

    /**
     * Real life RPM examples:
     * - Boeing E-3 Sentry: 6 RPM
     * - Air Surveillance Radar: 12 to 15 RPM
     * - En-Route Radar: 4 to 6 RPM
     */
    const float RADAR_RPM = 12.5f;

    const int RADAR_CIRCLES_COUNT = 4;

    // RPM to radians per second
    const float angularSpeed = RADAR_RPM * (2.0f * PI / 60.0f);
    float angle = 0.0f;

    // Width of the radar sweep in radians (adjust for more/less overlap)
    const float SWEEP_WIDTH = 0.1f;

    Vector2 center = {screenWidth / 2, screenHeight / 2};
    int radius = screenHeight / 2;
    int radiusStep = radius / RADAR_CIRCLES_COUNT;

    // Create a RenderTexture2D to draw the radar sweep effect
    RenderTexture2D radarTexture = LoadRenderTexture(screenWidth, screenHeight);
    RenderTexture2D sweepTexture = LoadRenderTexture(screenWidth, screenHeight);

    SetWindowState(FLAG_WINDOW_RESIZABLE);

    SetTargetFPS(60);

    DrawSweepLineFade(&sweepTexture, SWEEP_WIDTH, center, radius);

    while (!WindowShouldClose())
    {
        const float deltaTime = GetFrameTime();

        if (IsWindowResized())
        {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
            center = (Vector2){screenWidth / 2, screenHeight / 2};
            radius = screenHeight / 2;
            radiusStep = radius / RADAR_CIRCLES_COUNT;

            UnloadRenderTexture(radarTexture);
            radarTexture = LoadRenderTexture(screenWidth, screenHeight);

            UnloadRenderTexture(sweepTexture);
            sweepTexture = LoadRenderTexture(screenWidth, screenHeight);

            DrawSweepLineFade(&sweepTexture, SWEEP_WIDTH, center, radius);

            UnloadRenderTexture(radarTexture);
            radarTexture = LoadRenderTexture(screenWidth, screenHeight);
        }

        angle -= angularSpeed * deltaTime;

        // Reset the angle when it completes a full circle (2*PI radians)
        if (angle <= 2 * PI)
            angle += 2 * PI;

        const Vector2 sweepLineEnd = {
            center.x + radius * cos(-angle),
            center.y + radius * sin(-angle),
        };

        //////////////////
        // Texture mode //
        //////////////////

        BeginTextureMode(radarTexture);

        // Fade effect over the entire texture
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.01f));

        Rectangle sourceRec = {0, 0, (float)sweepTexture.texture.width, (float)-sweepTexture.texture.height}; // Flip texture vertically
        Rectangle destRec = {center.x, center.y, (float)sweepTexture.texture.width, (float)sweepTexture.texture.height};
        Vector2 origin = {sweepTexture.texture.width / 2.0f, sweepTexture.texture.height / 2.0f};

        DrawTexturePro(sweepTexture.texture, sourceRec, destRec, origin, angle * RAD2DEG, WHITE);

        EndTextureMode();

        //////////////////
        // Drawing mode //
        //////////////////

        BeginDrawing();
        ClearBackground(BLACK);

        // // Draw sweep line + fade effect
        DrawTexture(radarTexture.texture, 0, 0, WHITE);

        DrawFPS(0, 0);

        // Draw range circles
        for (int i = 0; i < RADAR_CIRCLES_COUNT; i++)
            DrawCircleLinesV(center, radius - radiusStep * i, GREEN);

        // Draw the circle representing the radar antenna
        DrawCircleV(center, 6.5, GREEN);

        EndDrawing();
    }

    UnloadRenderTexture(radarTexture);
    UnloadRenderTexture(sweepTexture);

    CloseWindow();

    return 0;
}
