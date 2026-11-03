#include "raylib.h"
#include <math.h>

int main(void) {
    // -------------------------------------------------------------------------
    // 1. INITIALIZATION
    // -------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Graphics Lab 01: The Rendering Loop & Coordinates");
    SetTargetFPS(60); // Match monitor refresh rate (~60 FPS)

    float angle = 0.0f;

    // -------------------------------------------------------------------------
    // 2. THE MAIN LOOP (Runs ~60 times per second)
    // -------------------------------------------------------------------------
    while (!WindowShouldClose()) {

        // --- STEP A: UPDATE STATE (Math, Physics, Input) ---
        angle += 0.02f; // Increment rotation angle

        // Read mouse position (Screen space coordinates: (0,0) is top-left)
        Vector2 mousePos = GetMousePosition();

        // Calculate a rotating triangle centered in the window
        float centerX = screenWidth / 2.0f;
        float centerY = screenHeight / 2.0f;
        float radius = 120.0f;

        Vector2 p1 = { centerX + cosf(angle) * radius, centerY + sinf(angle) * radius };
        Vector2 p2 = { centerX + cosf(angle + 2.094f) * radius, centerY + sinf(angle + 2.094f) * radius }; // +120 deg
        Vector2 p3 = { centerX + cosf(angle + 4.188f) * radius, centerY + sinf(angle + 4.188f) * radius }; // +240 deg

        // --- STEP B: RENDER (Double Buffering) ---
        BeginDrawing();

            // 1. Always clear the screen at the start of a frame
            ClearBackground((Color){ 24, 28, 38, 255 }); // Dark slate background

            // 2. Draw Screen Space Coordinate Grid
            // Screen (0, 0) is top-left: X increases right, Y increases down!
            DrawLine(0, centerY, screenWidth, centerY, (Color){ 45, 55, 75, 255 });
            DrawLine(centerX, 0, centerX, screenHeight, (Color){ 45, 55, 75, 255 });

            // 3. Draw Rotating Triangle (The atom of computer graphics)
            DrawTriangle(p1, p2, p3, (Color){ 235, 87, 87, 255 }); // Red-orange

            // Draw vertex points
            DrawCircleV(p1, 6.0f, YELLOW);
            DrawCircleV(p2, 6.0f, GREEN);
            DrawCircleV(p3, 6.0f, SKYBLUE);

            // 4. Interactive Element: Draw circle at mouse cursor
            DrawCircleLines((int)mousePos.x, (int)mousePos.y, 20.0f, (Color){ 80, 220, 100, 200 });
            DrawCircle((int)mousePos.x, (int)mousePos.y, 4.0f, (Color){ 80, 220, 100, 255 });

            // 5. Informational HUD
            DrawText("LESSON 1: The Rendering Loop & Coordinate Space", 20, 20, 20, RAYWHITE);
            DrawText("1. Screen Origin (0,0) is at TOP-LEFT", 20, 50, 16, LIGHTGRAY);
            DrawText("2. Move mouse: watch the green cursor track input", 20, 72, 16, LIGHTGRAY);
            DrawText("3. Rotating shape: calculated with trigonometry each frame", 20, 94, 16, LIGHTGRAY);

            // Real-time cursor coordinates
            DrawText(TextFormat("Mouse Pos: (X: %.0f, Y: %.0f)", mousePos.x, mousePos.y), 20, screenHeight - 35, 16, GREEN);
            DrawFPS(screenWidth - 100, 20);

        EndDrawing();
    }

    // -------------------------------------------------------------------------
    // 3. CLEANUP & SHUTDOWN
    // -------------------------------------------------------------------------
    CloseWindow();
    return 0;
}
