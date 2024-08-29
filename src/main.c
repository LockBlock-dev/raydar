#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * ToDo:
 *  - planes "own" coordinate system (allows scaling)
 *  - add real planes feed
 */

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500
#define TARGET_FPS 360
/**
 * Real life RPM examples:
 * - Boeing E-3 Sentry: 6 RPM
 * - Air Surveillance Radar: 12 to 15 RPM
 * - En-Route Radar: 4 to 6 RPM
 */
#define RADAR_RPM 12.5f
#define RADAR_CIRCLES_COUNT 4
#define RADAR_FONT_SIZE 20
#define RADAR_MAX_PLANES_COUNT 256
#define PLANE_SIZE 30
#define PLANE_FADE_COEF 0.2f

int min(int a, int b)
{
    return (a < b) ? a : b;
}

int max(int a, int b)
{
    return (a > b) ? a : b;
}

typedef struct Plane
{
    float real_x;
    float real_y;
    float x;
    float y;
    float alpha;
    double lastUpdatedAt;
    // Degrees
    float heading;
    char code[6];
} Plane;

typedef struct RadarData
{
    bool debug;

    int screenWidth;
    int screenHeight;
    Vector2 center;

    int radius;
    int radiusStep;

    // Seconds
    float revolutionTime;

    // Radians
    float angle;
    // Radians per second
    float angularSpeed;

    int planesCount;
    Plane planes[RADAR_MAX_PLANES_COUNT];

    // Contains radar circles and center dot
    RenderTexture2D radarTexture;
    // Contains sweep line and phosphor fading effect
    RenderTexture2D sweepEffectTexture;
    // Contains the rotated sweepEffectTexture
    RenderTexture2D displayTexture;

    // Contains the plane texture
    Texture2D planeTexture;
    float planeTextureRadius;
} RadarData;

/**
 * Checks collisions between a circle and a line segment
 */
bool CheckCircleLineCollision(Vector2 center, float radius, Vector2 lineStart, Vector2 lineEnd)
{
    // Calculate the line segment vector
    Vector2 lineDir = Vector2Subtract(lineEnd, lineStart);
    Vector2 toCircle = Vector2Subtract(center, lineStart);

    // Project vector 'toCircle' onto 'lineDir' to find the closest point
    float lineLengthSquared = Vector2LengthSqr(lineDir);
    float projection = Vector2DotProduct(toCircle, lineDir) / lineLengthSquared;

    // Clamp the projection to the range [0, 1] to ensure it's on the line segment
    projection = Clamp(projection, 0.0f, 1.0f);

    // Calculate the closest point on the line segment
    Vector2 closestPoint = Vector2Add(lineStart, Vector2Scale(lineDir, projection));

    // Check if the closest point is inside the circle
    return CheckCollisionPointCircle(closestPoint, center, radius);
}

/**
 * Initializes RadarData fields.
 */
void InitializeRadarData(RadarData *that)
{
    that->debug = false;

    that->screenWidth = SCREEN_WIDTH;
    that->screenHeight = SCREEN_HEIGHT;
    that->center = (Vector2){that->screenWidth / 2, that->screenHeight / 2};

    that->radius = that->screenHeight / 2;
    that->radiusStep = that->radius / RADAR_CIRCLES_COUNT;

    that->revolutionTime = 60.0f / RADAR_RPM;

    that->angle = 0.0f;
    that->angularSpeed = RADAR_RPM * (2.0f * PI / 60.0f);

    that->planesCount = 0;

    that->radarTexture = LoadRenderTexture(that->screenWidth, that->screenHeight);
    that->sweepEffectTexture = LoadRenderTexture(that->screenWidth, that->screenHeight);
    that->displayTexture = LoadRenderTexture(that->screenWidth, that->screenHeight);

    Image img = LoadImage(TextFormat("%s/assets/plane.png", GetWorkingDirectory()));
    ImageResize(&img, PLANE_SIZE, PLANE_SIZE);

    that->planeTexture = LoadTextureFromImage(img);
    that->planeTextureRadius = sqrt(pow(
                                        that->planeTexture.width / 2,
                                        2) +
                                    pow(
                                        that->planeTexture.height / 2,
                                        2));
    SetTextureFilter(that->planeTexture, TEXTURE_FILTER_TRILINEAR);

    UnloadImage(img);
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
}

/**
 * Updates planes
 */
void UpdatePlanes(RadarData *that, const float deltaTime)
{
    const Vector2 sweepLine = {
        that->center.x + that->radius * cos(-that->angle),
        that->center.y + that->radius * sin(-that->angle),
    };

    for (size_t i = 0; i < that->planesCount; i++)
    {
        float speed = GetRandomValue(5, 10);
        float speedX = speed * cos(that->planes[i].heading * DEG2RAD);
        float speedY = -speed * sin(that->planes[i].heading * DEG2RAD);

        that->planes[i].real_x += speedX * deltaTime;
        that->planes[i].real_y += speedY * deltaTime;
        that->planes[i].alpha -= PLANE_FADE_COEF * deltaTime;

        double now = GetTime();
        bool shouldUpdate = that->planes[i].lastUpdatedAt < now - that->revolutionTime / 4;

        if (CheckCircleLineCollision(
                (Vector2){that->planes[i].real_x, that->planes[i].real_y},
                that->planeTextureRadius,
                that->center,
                sweepLine) &&
            shouldUpdate)
        {
            that->planes[i].x = that->planes[i].real_x;
            that->planes[i].y = that->planes[i].real_y;

            that->planes[i].alpha = 1.0f;

            that->planes[i].lastUpdatedAt = now;
        }
    }
}

/**
 * Destroys, cleans, unloads RadarData fields.
 */
void DestroyRadarData(RadarData *that)
{
    UnloadRenderTexture(that->radarTexture);
    UnloadRenderTexture(that->sweepEffectTexture);
    UnloadRenderTexture(that->displayTexture);

    UnloadTexture(that->planeTexture);
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
 * Draws the rotated sweepEffectTexture on displayTexture.
 */
void DrawDisplayTexture(RadarData *radarData)
{
    // Define the source rectangle (the entire texture)
    Rectangle sourceRect = {
        0,
        0,
        radarData->sweepEffectTexture.texture.width,
        -radarData->sweepEffectTexture.texture.height, // Flip texture vertically
    };

    // Define the destination rectangle (center and size on the screen)
    Rectangle destRect = {
        radarData->center.x,
        radarData->center.y,
        radarData->sweepEffectTexture.texture.width,
        radarData->sweepEffectTexture.texture.height,
    };

    // Define the origin of rotation (center of the texture)
    Vector2 origin = {
        radarData->sweepEffectTexture.texture.width / 2,
        radarData->sweepEffectTexture.texture.height / 2,
    };

    BeginTextureMode(radarData->displayTexture);
    ClearBackground(BLANK);

    DrawTexturePro(
        radarData->sweepEffectTexture.texture,
        sourceRect,
        destRect,
        origin,
        radarData->angle * RAD2DEG,
        WHITE);

    EndTextureMode();
}

/**
 * Draws the planes
 */
void DrawPlanes(RadarData *radarData)
{
    // Define the source rectangle (the entire texture)
    Rectangle sourceRec = {
        0.0f,
        0.0f,
        radarData->planeTexture.width,
        radarData->planeTexture.height,
    };

    // Define the origin of rotation (center of the texture)
    Vector2 origin = {
        radarData->planeTexture.width / 2,
        radarData->planeTexture.height / 2,
    };

    for (size_t i = 0; i < radarData->planesCount; i++)
    {
        // Define the destination rectangle (position and size on the screen)
        Rectangle destRec = {
            radarData->planes[i].x,
            radarData->planes[i].y,
            radarData->planeTexture.width,
            radarData->planeTexture.height,
        };

        // Account for the plane asset facing 90°
        float rotationAngle = fmodf(90.0f - radarData->planes[i].heading, 360.0f);

        DrawTexturePro(radarData->planeTexture,
                       sourceRec,
                       destRec,
                       origin,
                       rotationAngle,
                       Fade(WHITE, radarData->planes[i].alpha));

        DrawText(radarData->planes[i].code,
                 radarData->planes[i].x + radarData->planeTextureRadius + RADAR_FONT_SIZE / 2,
                 radarData->planes[i].y - RADAR_FONT_SIZE / 2,
                 RADAR_FONT_SIZE,
                 Fade(WHITE, radarData->planes[i].alpha));

        if (radarData->debug)
        {
            DrawCircleLines(radarData->planes[i].x,
                            radarData->planes[i].y,
                            radarData->planeTextureRadius,
                            RED);

            DrawCircleLines(radarData->planes[i].real_x,
                            radarData->planes[i].real_y,
                            radarData->planeTextureRadius,
                            YELLOW);
        }
    }
}

int main(void)
{
    const char *TITLE = "Raydar";
    const char *DEBUG_TEXT = "DEBUG";
    RadarData radarData;

    SetWindowState(FLAG_MSAA_4X_HINT);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TITLE);

    SetTargetFPS(TARGET_FPS);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    InitializeRadarData(&radarData);
    DrawRadarTexture(&radarData);
    DrawSweepEffectTexture(&radarData);

    const int DEBUG_TEXT_SIZE = MeasureText(DEBUG_TEXT, RADAR_FONT_SIZE);

    // DEMO PLANES

    radarData.planes[0] = (Plane){
        .real_x = radarData.center.x + 10,
        .real_y = radarData.center.y - 10,
        .x = radarData.center.x,
        .y = radarData.center.y,
        .alpha = 1.0f,
        .code = "ABC123",
        .heading = GetRandomValue(0, 360),
        .lastUpdatedAt = -1.0f,
    };
    radarData.planesCount++;

    radarData.planes[1] = (Plane){
        .real_x = radarData.center.x - radarData.center.x / 2,
        .real_y = radarData.center.y,
        .x = radarData.center.x - radarData.center.x / 2,
        .y = radarData.center.y,
        .alpha = 1.0f,
        .code = "DEF456",
        .heading = GetRandomValue(0, 360),
        .lastUpdatedAt = -1.0f,
    };
    radarData.planesCount++;

    // END DEMO PLANES

    while (!WindowShouldClose())
    {
        const float deltaTime = GetFrameTime();

        if (IsKeyPressed(KEY_HOME))
        {
            radarData.debug = !radarData.debug;
        }

        if (IsWindowResized())
        {
            UpdateOnResizeRadarData(&radarData);
            DrawRadarTexture(&radarData);
            DrawSweepEffectTexture(&radarData);
        }

        UpdateRadarData(&radarData, deltaTime);
        UpdatePlanes(&radarData, deltaTime);

        DrawDisplayTexture(&radarData);

        BeginDrawing();

        ClearBackground(BLACK);

        DrawTexture(radarData.displayTexture.texture, 0, 0, WHITE);
        DrawTexture(radarData.radarTexture.texture, 0, 0, WHITE);

        DrawPlanes(&radarData);

        if (radarData.debug)
        {
            const Vector2 sweepLine = {
                radarData.center.x + radarData.radius * cos(-radarData.angle),
                radarData.center.y + radarData.radius * sin(-radarData.angle),
            };

            DrawLineV(radarData.center, sweepLine, RED);

            DrawText(DEBUG_TEXT, radarData.screenWidth - DEBUG_TEXT_SIZE, 0, RADAR_FONT_SIZE, WHITE);
        }

        DrawFPS(0, 0);

        EndDrawing();
    }

    DestroyRadarData(&radarData);

    CloseWindow();

    return 0;
}
