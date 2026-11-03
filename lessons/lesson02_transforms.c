#include "raylib.h"
#include "raymath.h"
#include <math.h>

int main(void)
{
	// -------------------------------------------------------------------------
	// 1. WINDOW INITIALIZATION
	// -------------------------------------------------------------------------
	const int screenWidth = 900;
	const int screenHeight = 650;

	InitWindow(screenWidth, screenHeight, "Graphics Lab 02: Vectors & Hierarchical Transformations");
	SetTargetFPS(60);

	// -------------------------------------------------------------------------
	// 2. STATE DEFINITIONS
	// -------------------------------------------------------------------------
	// The "Sun" is the root of our hierarchy (World Coordinates)
	Vector2 sunPos = { screenWidth / 2.0f, screenHeight / 2.0f };
	const float sunRadius = 40.0f;

	// Earth's orbit relative to the Sun (Parent = Sun)
	float earthOrbitRadius = 180.0f;
	float earthOrbitAngle = 0.0f;
	float earthOrbitSpeed = 1.0f; // radians per second
	const float earthRadius = 18.0f;

	// Moon's orbit relative to the Earth (Parent = Earth)
	float moonOrbitRadius = 45.0f;
	float moonOrbitAngle = 0.0f;
	float moonOrbitSpeed = 3.5f; // orbits Earth faster
	const float moonRadius = 7.0f;

	bool isPaused = false;
	bool isDraggingSun = false;

	// -------------------------------------------------------------------------
	// 3. MAIN LOOP
	// -------------------------------------------------------------------------
	while (!WindowShouldClose()) {
		float dt = GetFrameTime(); // Delta time: seconds elapsed since last frame (~0.016s)

		// --- STEP A: INPUT & UPDATE ---
		Vector2 mousePos = GetMousePosition();

		// 1. Interactive Dragging of the Sun (Root Parent)
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			if (CheckCollisionPointCircle(mousePos, sunPos, sunRadius + 10.0f)) {
				isDraggingSun = true;
			}
		}
		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
			isDraggingSun = false;
		}
		if (isDraggingSun) {
			sunPos = mousePos; // Move the parent!
		}

		// 2. Speed controls & pause
		if (IsKeyPressed(KEY_SPACE))
			isPaused = !isPaused;
		if (IsKeyDown(KEY_UP))
			earthOrbitSpeed += 0.5f * dt;
		if (IsKeyDown(KEY_DOWN) && earthOrbitSpeed > 0.2f)
			earthOrbitSpeed -= 0.5f * dt;

		// 3. Mathematical Update (Hierarchical Transforms)
		if (!isPaused) {
			earthOrbitAngle += earthOrbitSpeed * dt;
			moonOrbitAngle += moonOrbitSpeed * dt;
		}

		// --- HIERARCHY CALCULATION ---
		// Vector math:
		// Position = Parent_Position + Offset_Vector

		// Earth Offset Vector (Relative to Sun)
		Vector2 earthOffset = {
			cosf(earthOrbitAngle) * earthOrbitRadius,
			sinf(earthOrbitAngle) * earthOrbitRadius
		};
		// Earth World Position = Sun + EarthOffset
		Vector2 earthPos = Vector2Add(sunPos, earthOffset);

		// Moon Offset Vector (Relative to Earth)
		Vector2 moonOffset = {
			cosf(moonOrbitAngle) * moonOrbitRadius,
			sinf(moonOrbitAngle) * moonOrbitRadius
		};
		// Moon World Position = Earth + MoonOffset = (Sun + EarthOffset) + MoonOffset
		Vector2 moonPos = Vector2Add(earthPos, moonOffset);

		// Velocity Vector of Earth (Tangent to orbit circle: derivative of position)
		// Tangent of (cos(a), sin(a)) is (-sin(a), cos(a))
		Vector2 earthVelocityDir = { -sinf(earthOrbitAngle), cosf(earthOrbitAngle) };
		Vector2 earthVelocityArrow = Vector2Add(earthPos, Vector2Scale(earthVelocityDir, 45.0f));

		// --- STEP B: RENDER ---
		BeginDrawing();
		ClearBackground((Color){ 16, 20, 30, 255 }); // Deep space dark blue

		// 1. Draw Coordinate Grid for Reference
		DrawLine(0, (int)sunPos.y, screenWidth, (int)sunPos.y, (Color){ 30, 40, 60, 255 });
		DrawLine((int)sunPos.x, 0, (int)sunPos.x, screenHeight, (Color){ 30, 40, 60, 255 });

		// 2. Draw Orbit Paths (Visualizing radius)
		DrawCircleLines((int)sunPos.x, (int)sunPos.y, earthOrbitRadius, (Color){ 60, 80, 110, 180 });
		DrawCircleLines((int)earthPos.x, (int)earthPos.y, moonOrbitRadius, (Color){ 80, 100, 140, 180 });

		// 3. Draw Connecting Vector Lines (Parent -> Child)
		DrawLineV(sunPos, earthPos, (Color){ 120, 150, 190, 150 });
		DrawLineV(earthPos, moonPos, (Color){ 160, 190, 220, 150 });

		// 4. Draw Bodies (Sun, Earth, Moon)
		// Sun (Root)
		DrawCircleV(sunPos, sunRadius, (Color){ 255, 200, 50, 255 }); // Glowing Yellow
		DrawCircleLines((int)sunPos.x, (int)sunPos.y, sunRadius + 2.0f, ORANGE);

		// Earth (Child of Sun)
		DrawCircleV(earthPos, earthRadius, (Color){ 60, 140, 240, 255 }); // Blue
		DrawCircleLines((int)earthPos.x, (int)earthPos.y, earthRadius + 1.0f, SKYBLUE);

		// Moon (Child of Earth / Grandchild of Sun)
		DrawCircleV(moonPos, moonRadius, (Color){ 200, 210, 225, 255 }); // Silver Gray

		// 5. Draw Earth's Velocity Vector (Green tangent arrow)
		DrawLineEx(earthPos, earthVelocityArrow, 2.5f, GREEN);
		DrawCircleV(earthVelocityArrow, 4.0f, GREEN);

		// 6. Labels & Vectors
		DrawText("Sun (Parent)", (int)sunPos.x - 35, (int)sunPos.y - 10, 14, BLACK);
		DrawText("Earth", (int)earthPos.x + 22, (int)earthPos.y - 8, 14, SKYBLUE);
		DrawText("Moon", (int)moonPos.x + 12, (int)moonPos.y - 6, 12, LIGHTGRAY);
		DrawText("Velocity Vector (Tangent)", (int)earthVelocityArrow.x + 8, (int)earthVelocityArrow.y - 6, 12, GREEN);

		// 7. Interactive HUD
		DrawRectangle(15, 15, 340, 175, (Color){ 10, 14, 22, 220 });
		DrawRectangleLines(15, 15, 340, 175, (Color){ 50, 65, 95, 255 });

		DrawText("LESSON 02: Vectors & Scene Hierarchies", 25, 25, 16, RAYWHITE);
		DrawText("* Click & Drag the Sun to move root space", 25, 52, 13, LIGHTGRAY);
		DrawText("* Earth is relative to Sun: Pos = Sun + V1", 25, 72, 13, LIGHTGRAY);
		DrawText("* Moon is relative to Earth: Pos = Earth + V2", 25, 92, 13, LIGHTGRAY);
		DrawText("* Press SPACE to Pause / Resume orbit", 25, 115, 13, YELLOW);
		DrawText("* Press UP / DOWN arrows to adjust speed", 25, 135, 13, YELLOW);

		DrawText(TextFormat("Earth Speed: %.1fx  |  Status: %s", earthOrbitSpeed, isPaused ? "PAUSED" : "RUNNING"),
			 25, 160, 13, isPaused ? RED : GREEN);

		DrawFPS(screenWidth - 90, 20);

		EndDrawing();
	}

	// -------------------------------------------------------------------------
	// 4. CLEANUP
	// -------------------------------------------------------------------------
	CloseWindow();
	return 0;
}
