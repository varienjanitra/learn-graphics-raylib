#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>

int main(void)
{
	// -------------------------------------------------------------------------
	// 1. INITIALIZATION
	// -------------------------------------------------------------------------
	const int screenWidth = 960;
	const int screenHeight = 650;

	InitWindow(screenWidth, screenHeight, "Graphics Lab 03: 3D Coordinate Space & The Virtual Camera");
	SetTargetFPS(60);

	// -------------------------------------------------------------------------
	// 2. THE 3D CAMERA SETUP (Spherical Orbit Coordinates)
	// -------------------------------------------------------------------------
	Camera3D camera = { 0 };
	camera.target = (Vector3){ 0.0f, 1.2f, 0.0f }; // Look at center of the scene
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; // +Y is UP
	camera.fovy = 45.0f; // 45 degrees vertical Field-of-View
	camera.projection = CAMERA_PERSPECTIVE; // Start in Perspective mode

	// Explicit orbital angles for total control & transparency
	float cameraYaw = 0.8f; // Horizontal orbit angle (radians)
	float cameraPitch = 0.5f; // Vertical elevation angle (radians)
	float cameraDistance = 12.0f; // Distance from target

	float cubeRotation = 0.0f;
	float sphereOrbit = 0.0f;
	bool isPaused = false;

	// -------------------------------------------------------------------------
	// 3. MAIN LOOP
	// -------------------------------------------------------------------------
	while (!WindowShouldClose()) {
		float dt = GetFrameTime();

		// --- STEP A: INPUT & UPDATE ---

		// 1. Explicit Mouse Orbital Controls (Click & Drag with Left Mouse Button)
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			Vector2 mouseDelta = GetMouseDelta();
			cameraYaw += mouseDelta.x * 0.006f; // Horizontal orbit
			cameraPitch -= mouseDelta.y * 0.006f; // Vertical elevation

			// Clamp pitch to avoid flipping upside down at poles (-85 to +85 degrees)
			if (cameraPitch > 1.48f)
				cameraPitch = 1.48f;
			if (cameraPitch < -1.48f)
				cameraPitch = -1.48f;
		}

		// 2. Mouse Wheel Zoom
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f) {
			cameraDistance -= wheel * 1.0f;
			if (cameraDistance < 3.0f)
				cameraDistance = 3.0f;
			if (cameraDistance > 30.0f)
				cameraDistance = 30.0f;
		}

		// Calculate Camera Position using Spherical Trigonometry
		camera.position.x = camera.target.x + cameraDistance * cosf(cameraPitch) * sinf(cameraYaw);
		camera.position.y = camera.target.y + cameraDistance * sinf(cameraPitch);
		camera.position.z = camera.target.z + cameraDistance * cosf(cameraPitch) * cosf(cameraYaw);

		// 3. Toggle Projection: Perspective vs Orthographic (Press TAB)
		if (IsKeyPressed(KEY_TAB)) {
			if (camera.projection == CAMERA_PERSPECTIVE) {
				camera.projection = CAMERA_ORTHOGRAPHIC;
				camera.fovy = 8.0f; // In Ortho mode, fovy represents the visible box height
			} else {
				camera.projection = CAMERA_PERSPECTIVE;
				camera.fovy = 45.0f;
			}
		}

		// 4. Toggle Pause / Play (Press SPACE)
		if (IsKeyPressed(KEY_SPACE)) {
			isPaused = !isPaused;
		}

		// Update animation angles only when unpaused
		if (!isPaused) {
			cubeRotation += 50.0f * dt; // Rotate cube 50 degrees per second
			sphereOrbit += 1.2f * dt; // Orbit sphere
		}

		// --- STEP B: RENDER ---
		BeginDrawing();
		ClearBackground((Color){ 18, 22, 32, 255 }); // Dark space background

		// =================================================================
		// 3D RENDERING PIPELINE: Activate View & Projection Matrices
		// =================================================================
		BeginMode3D(camera);

		// 1. Ground Grid (14x14 units, 1.0 spacing)
		DrawGrid(14, 1.0f);

		// 2. RGB = XYZ Coordinate Axes at Origin
		// Red = +X (Right), Green = +Y (Up), Blue = +Z (Forward/Depth)
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 3.0f, 0, 0 }, RED);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 3.0f, 0 }, GREEN);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 0, 3.0f }, BLUE);

		// 3. Central 3D Rotating Cube using Matrix Stack (rlgl)
		// rlPushMatrix/rlPopMatrix isolate transformations so they don't affect other objects!
		Vector3 cubePos = { 0.0f, 1.5f, 0.0f };
		rlPushMatrix();
		rlTranslatef(cubePos.x, cubePos.y, cubePos.z);
		rlRotatef(cubeRotation, 0.4f, 1.0f, 0.3f); // Rotate around an arbitrary 3D diagonal axis

		// Draw the rotating cube centered at its local origin
		DrawCube((Vector3){ 0, 0, 0 }, 2.2f, 2.2f, 2.2f, (Color){ 230, 70, 70, 255 }); // Red solid
		DrawCubeWires((Vector3){ 0, 0, 0 }, 2.2f, 2.2f, 2.2f, WHITE); // White edge wireframe
		rlPopMatrix();

		// 4. Orbiting Sphere (Demonstrating Depth Testing / Z-Buffer)
		// Watch how it passes IN FRONT of and BEHIND the spinning cube!
		float orbitRadius = 4.5f;
		Vector3 spherePos = {
			cosf(sphereOrbit) * orbitRadius,
			1.5f + sinf(sphereOrbit * 2.0f) * 0.8f, // Bobbing up and down
			sinf(sphereOrbit) * orbitRadius
		};
		DrawSphere(spherePos, 0.65f, (Color){ 50, 160, 240, 255 });
		DrawSphereWires(spherePos, 0.7f, 8, 8, SKYBLUE);
		DrawLine3D(cubePos, spherePos, (Color){ 100, 140, 200, 90 });

		// 5. Reference Pillars to demonstrate spatial depth and parallax
		DrawCube((Vector3){ -4.0f, 1.5f, -4.0f }, 0.8f, 3.0f, 0.8f, (Color){ 240, 180, 50, 255 });
		DrawCubeWires((Vector3){ -4.0f, 1.5f, -4.0f }, 0.8f, 3.0f, 0.8f, GOLD);

		DrawCube((Vector3){ 4.0f, 1.0f, 4.0f }, 0.8f, 2.0f, 0.8f, (Color){ 50, 200, 120, 255 });
		DrawCubeWires((Vector3){ 4.0f, 1.0f, 4.0f }, 0.8f, 2.0f, 0.8f, GREEN);

		EndMode3D();
		// =================================================================
		// 2D SCREEN SPACE: UI, HUD & Labels
		// =================================================================

		// Interactive HUD Box
		DrawRectangle(15, 15, 410, 205, (Color){ 10, 14, 22, 220 });
		DrawRectangleLines(15, 15, 410, 205, (Color){ 50, 65, 95, 255 });

		DrawText("LESSON 03: 3D Coordinate Space & Camera", 25, 25, 16, RAYWHITE);
		DrawText("1. RGB = XYZ: Red = +X, Green = +Y, Blue = +Z", 25, 50, 13, LIGHTGRAY);
		DrawText("2. Click & Drag LEFT MOUSE: Orbit 3D camera", 25, 70, 13, (IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? GREEN : SKYBLUE));
		DrawText("3. MOUSE WHEEL: Zoom camera in/out", 25, 90, 13, LIGHTGRAY);
		DrawText("4. Press TAB: Toggle Perspective <-> Orthographic", 25, 110, 13, YELLOW);
		DrawText("5. Press SPACE: Pause / Play 3D animations", 25, 130, 13, YELLOW);

		// Real-time animation status banner
		const char *statusStr = isPaused ? "[ PAUSED ] (Press SPACE to Resume)" : "[ PLAYING ] (Press SPACE to Pause)";
		Color statusColor = isPaused ? RED : GREEN;
		DrawText(statusStr, 25, 155, 13, statusColor);

		// Camera status
		const char *projText = (camera.projection == CAMERA_PERSPECTIVE) ? "PERSPECTIVE (FOV: 45 deg)" : "ORTHOGRAPHIC (Isometric)";
		DrawText(TextFormat("Camera: %s | Dist: %.1f", projText, cameraDistance), 25, 178, 12, GRAY);

		DrawFPS(screenWidth - 90, 20);

		EndDrawing();
	}

	// -------------------------------------------------------------------------
	// 4. CLEANUP
	// -------------------------------------------------------------------------
	CloseWindow();
	return 0;
}
