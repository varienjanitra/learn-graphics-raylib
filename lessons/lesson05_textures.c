#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <math.h>

// -----------------------------------------------------------------------------
// HELPER: Generate a Procedural Test Pattern Texture in Memory
// -----------------------------------------------------------------------------
Texture2D GenerateTestTexture(int size)
{
	Image img = GenImageColor(size, size, BLANK);

	// Draw a 4-quadrant colored checkerboard with distinct border
	int half = size / 2;
	ImageDrawRectangle(&img, 0, 0, half, half, (Color){ 230, 80, 80, 255 }); // Top-Left: Red
	ImageDrawRectangle(&img, half, 0, half, half, (Color){ 70, 190, 90, 255 }); // Top-Right: Green
	ImageDrawRectangle(&img, 0, half, half, half, (Color){ 60, 140, 240, 255 }); // Bottom-Left: Blue
	ImageDrawRectangle(&img, half, half, half, half, (Color){ 240, 200, 60, 255 }); // Bottom-Right: Yellow

	// Inner subtle checker pattern
	int cells = 8;
	int cellSize = size / cells;
	for (int y = 0; y < cells; y++) {
		for (int x = 0; x < cells; x++) {
			if ((x + y) % 2 == 0) {
				ImageDrawRectangle(&img, x * cellSize, y * cellSize, cellSize, cellSize, (Color){ 255, 255, 255, 60 });
			}
		}
	}

	// High-contrast bright outer border (so CLAMP edge stretching is dramatically visible)
	ImageDrawRectangleLines(&img, (Rectangle){ 0, 0, (float)size, (float)size }, 8, RAYWHITE);
	ImageDrawLine(&img, 0, half, size, half, RAYWHITE);
	ImageDrawLine(&img, half, 0, half, size, RAYWHITE);

	Texture2D tex = LoadTextureFromImage(img);
	UnloadImage(img);
	return tex;
}

// -----------------------------------------------------------------------------
// HELPER: Create a 3D Floor Quad with DYNAMIC UV Buffer
// -----------------------------------------------------------------------------
Mesh CreateDynamicFloorMesh(float size)
{
	Mesh mesh = { 0 };
	mesh.vertexCount = 4;
	mesh.triangleCount = 2;

	mesh.vertices = (float *)MemAlloc(4 * 3 * sizeof(float));
	mesh.texcoords = (float *)MemAlloc(4 * 2 * sizeof(float));
	mesh.normals = (float *)MemAlloc(4 * 3 * sizeof(float));
	mesh.indices = (unsigned short *)MemAlloc(6 * sizeof(unsigned short));

	float half = size / 2.0f;

	// 4 Vertices on the horizontal XZ plane
	// V0: Top-Left
	mesh.vertices[0] = -half;
	mesh.vertices[1] = 0.0f;
	mesh.vertices[2] = -half;
	// V1: Bottom-Left
	mesh.vertices[3] = -half;
	mesh.vertices[4] = 0.0f;
	mesh.vertices[5] = half;
	// V2: Bottom-Right
	mesh.vertices[6] = half;
	mesh.vertices[7] = 0.0f;
	mesh.vertices[8] = half;
	// V3: Top-Right
	mesh.vertices[9] = half;
	mesh.vertices[10] = 0.0f;
	mesh.vertices[11] = -half;

	// Normals pointing straight UP (+Y)
	for (int i = 0; i < 4; i++) {
		mesh.normals[i * 3 + 0] = 0.0f;
		mesh.normals[i * 3 + 1] = 1.0f;
		mesh.normals[i * 3 + 2] = 0.0f;
	}

	// Initial UV coordinates (0 to 1)
	mesh.texcoords[0] = 0.0f;
	mesh.texcoords[1] = 0.0f;
	mesh.texcoords[2] = 0.0f;
	mesh.texcoords[3] = 1.0f;
	mesh.texcoords[4] = 1.0f;
	mesh.texcoords[5] = 1.0f;
	mesh.texcoords[6] = 1.0f;
	mesh.texcoords[7] = 0.0f;

	// 2 Triangles (CCW)
	mesh.indices[0] = 0;
	mesh.indices[1] = 1;
	mesh.indices[2] = 2;
	mesh.indices[3] = 0;
	mesh.indices[4] = 2;
	mesh.indices[5] = 3;

	// Upload with dynamic = true so we can stream UV changes!
	UploadMesh(&mesh, true);
	return mesh;
}

int main(void)
{
	// -------------------------------------------------------------------------
	// 1. INITIALIZATION
	// -------------------------------------------------------------------------
	const int screenWidth = 960;
	const int screenHeight = 650;

	InitWindow(screenWidth, screenHeight, "Graphics Lab 05: Textures & UV Coordinates");
	SetTargetFPS(60);

	// Spherical Orbit Camera
	Camera3D camera = { 0 };
	camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	float cameraYaw = 0.8f;
	float cameraPitch = 0.5f;
	float cameraDistance = 12.0f;

	// 2. Generate Texture
	Texture2D texture = GenerateTestTexture(256);

	// 3. Create Models
	Mesh cubeMesh = GenMeshCube(2.2f, 2.2f, 2.2f);
	Model cubeModel = LoadModelFromMesh(cubeMesh);
	cubeModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

	Mesh floorMesh = CreateDynamicFloorMesh(10.0f);
	Model floorModel = LoadModelFromMesh(floorMesh);
	floorModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

	// Interactive settings
	int filterMode = 0; // 0 = Point (Nearest), 1 = Bilinear
	int wrapMode = 0; // 0 = Repeat, 1 = Clamp, 2 = Mirror Repeat
	float uvScale = 3.0f; // Start with UVs > 1.0 so wrap mode is immediately visible!
	float prevUvScale = 0.0f;
	float rotation = 0.0f;
	bool autoRotate = true;

	// Initialize wrap mode
	SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
	SetTextureFilter(texture, TEXTURE_FILTER_POINT);

	// -------------------------------------------------------------------------
	// 4. MAIN LOOP
	// -------------------------------------------------------------------------
	while (!WindowShouldClose()) {
		float dt = GetFrameTime();

		// --- STEP A: INPUT & CAMERA ---
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			Vector2 delta = GetMouseDelta();
			cameraYaw += delta.x * 0.006f;
			cameraPitch -= delta.y * 0.006f;
			if (cameraPitch > 1.48f)
				cameraPitch = 1.48f;
			if (cameraPitch < -1.48f)
				cameraPitch = -1.48f;
		}

		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f) {
			cameraDistance -= wheel * 1.0f;
			if (cameraDistance < 3.0f)
				cameraDistance = 3.0f;
			if (cameraDistance > 30.0f)
				cameraDistance = 30.0f;
		}

		camera.position.x = camera.target.x + cameraDistance * cosf(cameraPitch) * sinf(cameraYaw);
		camera.position.y = camera.target.y + cameraDistance * sinf(cameraPitch);
		camera.position.z = camera.target.z + cameraDistance * cosf(cameraPitch) * cosf(cameraYaw);

		if (IsKeyPressed(KEY_SPACE))
			autoRotate = !autoRotate;
		if (autoRotate)
			rotation += 25.0f * dt;

		// Toggle Filter: Point vs Bilinear
		if (IsKeyPressed(KEY_F)) {
			filterMode = (filterMode + 1) % 2;
			SetTextureFilter(texture, (filterMode == 0) ? TEXTURE_FILTER_POINT : TEXTURE_FILTER_BILINEAR);
		}

		// Toggle Wrap Mode: 0 = Repeat, 1 = Clamp, 2 = Mirror Repeat
		if (IsKeyPressed(KEY_R)) {
			wrapMode = (wrapMode + 1) % 3;
			if (wrapMode == 0)
				SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
			else if (wrapMode == 1)
				SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
			else
				SetTextureWrap(texture, TEXTURE_WRAP_MIRROR_REPEAT);
		}

		// Adjust UV scale
		if (IsKeyDown(KEY_UP))
			uvScale += 1.5f * dt;
		if (IsKeyDown(KEY_DOWN))
			uvScale -= 1.5f * dt;
		if (uvScale < 0.5f)
			uvScale = 0.5f;
		if (uvScale > 6.0f)
			uvScale = 6.0f;

		// --- STEP B: STREAM UPDATED UV COORDINATES TO GPU ---
		if (uvScale != prevUvScale) {
			prevUvScale = uvScale;

			// Scale the vertex UV coordinates beyond [0, 1]!
			// This is what forces the GPU texture sampler to trigger WRAP logic!
			floorMesh.texcoords[0] = 0.0f;
			floorMesh.texcoords[1] = 0.0f;
			floorMesh.texcoords[2] = 0.0f;
			floorMesh.texcoords[3] = uvScale;
			floorMesh.texcoords[4] = uvScale;
			floorMesh.texcoords[5] = uvScale;
			floorMesh.texcoords[6] = uvScale;
			floorMesh.texcoords[7] = 0.0f;

			// Stream new UV buffer to GPU VRAM (buffer index 1 = texcoords)
			UpdateMeshBuffer(floorMesh, 1, floorMesh.texcoords, 4 * 2 * sizeof(float), 0);
		}

		// --- STEP C: RENDER ---
		BeginDrawing();
		ClearBackground((Color){ 16, 20, 28, 255 });

		BeginMode3D(camera);
		DrawGrid(12, 1.0f);

		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 3.0f, 0, 0 }, RED);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 3.0f, 0 }, GREEN);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 0, 3.0f }, BLUE);

		// 1. Draw Central Rotating Cube (Standard UV: [0, 1])
		Vector3 cubePos = { 0.0f, 1.4f, 0.0f };
		Vector3 axis = { 0.2f, 1.0f, 0.0f };
		DrawModelEx(cubeModel, cubePos, axis, rotation, (Vector3){ 1.0f, 1.0f, 1.0f }, WHITE);
		DrawCubeWires(cubePos, 2.22f, 2.22f, 2.22f, (Color){ 255, 255, 255, 80 });

		// 2. Draw 3D Floor with Scaled UV Coordinates
		DrawModel(floorModel, (Vector3){ 0.0f, -0.01f, 0.0f }, 1.0f, WHITE);

		EndMode3D();

		// -----------------------------------------------------------------
		// 2D HUD OVERLAY & VRAM PREVIEW
		// -----------------------------------------------------------------
		DrawRectangle(15, 15, 430, 235, (Color){ 10, 14, 22, 230 });
		DrawRectangleLines(15, 15, 430, 235, (Color){ 50, 65, 95, 255 });

		DrawText("LESSON 05: Textures & UV Coordinates", 25, 25, 16, RAYWHITE);

		const char *wrapNames[] = { "REPEAT (Tiles across surface)", "CLAMP (Edge pixels stretch)", "MIRROR_REPEAT (Mirrored tiles)" };
		Color wrapColors[] = { GREEN, ORANGE, SKYBLUE };
		DrawText(TextFormat("[R] Wrap Mode:   %s", wrapNames[wrapMode]), 25, 52, 13, wrapColors[wrapMode]);

		DrawText(TextFormat("[F] Filter Mode: %s", filterMode == 0 ? "POINT (Nearest / Pixelated)" : "BILINEAR (Smooth Interpolation)"), 25, 72, 13, YELLOW);
		DrawText(TextFormat("[UP/DOWN] Floor UV Range: [0.0, %.2f]", uvScale), 25, 95, 13, GREEN);
		DrawText("[SPACE] Pause / Play cube rotation", 25, 115, 13, LIGHTGRAY);
		DrawText("LEFT MOUSE: Orbit camera  |  SCROLL: Zoom", 25, 135, 13, LIGHTGRAY);

		DrawLine(25, 160, 420, 160, (Color){ 40, 50, 75, 255 });
		DrawText("WHY WRAP MODE MATTERS:", 25, 170, 12, SKYBLUE);
		DrawText("* Wrap mode ONLY takes effect when UVs exceed 1.0!", 25, 188, 12, RAYWHITE);
		if (wrapMode == 0) {
			DrawText("* REPEAT: At UV > 1.0, texels wrap back to 0.0 (tiling).", 25, 206, 12, GREEN);
		} else if (wrapMode == 1) {
			DrawText("* CLAMP: At UV > 1.0, GPU locks onto the outer edge pixel.", 25, 206, 12, ORANGE);
		} else {
			DrawText("* MIRROR: At UV > 1.0, every alternate tile flips reverse.", 25, 206, 12, SKYBLUE);
		}

		// 2D Texture Preview
		DrawRectangle(15, screenHeight - 160, 140, 145, (Color){ 10, 14, 22, 230 });
		DrawRectangleLines(15, screenHeight - 160, 140, 145, (Color){ 50, 65, 95, 255 });
		DrawText("2D Source VRAM:", 22, screenHeight - 152, 11, LIGHTGRAY);
		DrawTexturePro(texture,
			       (Rectangle){ 0, 0, (float)texture.width, (float)texture.height },
			       (Rectangle){ 25, screenHeight - 135, 120, 120 },
			       (Vector2){ 0, 0 }, 0.0f, WHITE);

		DrawFPS(screenWidth - 90, 20);

		EndDrawing();
	}

	// -------------------------------------------------------------------------
	// 5. CLEANUP
	// -------------------------------------------------------------------------
	UnloadModel(cubeModel);
	UnloadModel(floorModel);
	UnloadTexture(texture);
	CloseWindow();

	return 0;
}
