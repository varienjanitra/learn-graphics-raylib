#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <math.h>

// -----------------------------------------------------------------------------
// HELPER: Generate a custom Indexed 3D Pyramid Mesh from raw C arrays
// -----------------------------------------------------------------------------
Mesh CreatePyramidMesh(void)
{
	Mesh mesh = { 0 };

	mesh.vertexCount = 5;
	mesh.triangleCount = 6;

	// 1. Allocate Vertex Positions (3 floats per vertex: X, Y, Z)
	mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));

	// Vertex 0: Apex (Top tip)
	mesh.vertices[0] = 0.0f;
	mesh.vertices[1] = 2.4f;
	mesh.vertices[2] = 0.0f;
	// Vertex 1: Base Front-Left
	mesh.vertices[3] = -1.6f;
	mesh.vertices[4] = 0.0f;
	mesh.vertices[5] = 1.6f;
	// Vertex 2: Base Front-Right
	mesh.vertices[6] = 1.6f;
	mesh.vertices[7] = 0.0f;
	mesh.vertices[8] = 1.6f;
	// Vertex 3: Base Back-Right
	mesh.vertices[9] = 1.6f;
	mesh.vertices[10] = 0.0f;
	mesh.vertices[11] = -1.6f;
	// Vertex 4: Base Back-Left
	mesh.vertices[12] = -1.6f;
	mesh.vertices[13] = 0.0f;
	mesh.vertices[14] = -1.6f;

	// 2. Allocate Vertex Colors (4 bytes per vertex: R, G, B, A)
	// The GPU hardware interpolates these colors across the triangle faces!
	mesh.colors = (unsigned char *)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));

	// V0: Glowing Gold
	mesh.colors[0] = 255;
	mesh.colors[1] = 215;
	mesh.colors[2] = 0;
	mesh.colors[3] = 255;
	// V1: Crimson Red
	mesh.colors[4] = 235;
	mesh.colors[5] = 60;
	mesh.colors[6] = 60;
	mesh.colors[7] = 255;
	// V2: Emerald Green
	mesh.colors[8] = 50;
	mesh.colors[9] = 210;
	mesh.colors[10] = 80;
	mesh.colors[11] = 255;
	// V3: Electric Blue
	mesh.colors[12] = 40;
	mesh.colors[13] = 140;
	mesh.colors[14] = 240;
	mesh.colors[15] = 255;
	// V4: Vivid Purple
	mesh.colors[16] = 180;
	mesh.colors[17] = 50;
	mesh.colors[18] = 230;
	mesh.colors[19] = 255;

	// 3. Allocate Index Buffer (3 indices per triangle -> 18 indices)
	// Winding Order: All triangles defined in Counter-Clockwise (CCW) order!
	mesh.indices = (unsigned short *)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));

	unsigned short indices[] = {
		// 4 Side Triangles (Apex -> Base Edge)
		0, 1, 2, // Front Face
		0, 2, 3, // Right Face
		0, 3, 4, // Back Face
		0, 4, 1, // Left Face

		// 2 Base Triangles (Floor Quad)
		1, 4, 3, // Base Tri 1
		1, 3, 2 // Base Tri 2
	};

	for (int i = 0; i < mesh.triangleCount * 3; i++) {
		mesh.indices[i] = indices[i];
	}

	// Upload to GPU memory (VBO / EBO)
	UploadMesh(&mesh, false);
	return mesh;
}

// -----------------------------------------------------------------------------
// HELPER: Generate a Dynamic 3D Wave Grid (Procedural Terrain)
// -----------------------------------------------------------------------------
#define GRID_RES 20 // 20x20 = 400 vertices

Mesh CreateWaveGridMesh(float size)
{
	Mesh mesh = { 0 };

	mesh.vertexCount = GRID_RES * GRID_RES;
	mesh.triangleCount = (GRID_RES - 1) * (GRID_RES - 1) * 2;

	mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
	mesh.colors = (unsigned char *)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));
	mesh.indices = (unsigned short *)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));

	float step = size / (GRID_RES - 1);
	float halfSize = size / 2.0f;

	// 1. Generate Vertices in a 2D planar grid (X, Z)
	for (int z = 0; z < GRID_RES; z++) {
		for (int x = 0; x < GRID_RES; x++) {
			int idx = (z * GRID_RES + x) * 3;
			mesh.vertices[idx + 0] = (x * step) - halfSize;
			mesh.vertices[idx + 1] = 0.0f; // Height (Y) will be animated
			mesh.vertices[idx + 2] = (z * step) - halfSize;

			// Color gradient based on coordinates
			int cIdx = (z * GRID_RES + x) * 4;
			mesh.colors[cIdx + 0] = (unsigned char)((float)x / GRID_RES * 200 + 40);
			mesh.colors[cIdx + 1] = 120;
			mesh.colors[cIdx + 2] = (unsigned char)((float)z / GRID_RES * 200 + 55);
			mesh.colors[cIdx + 3] = 255;
		}
	}

	// 2. Generate Index Buffer (Connect 4 corner vertices of each quad into 2 triangles)
	int triIdx = 0;
	for (int z = 0; z < GRID_RES - 1; z++) {
		for (int x = 0; x < GRID_RES - 1; x++) {
			unsigned short topLeft = (unsigned short)(z * GRID_RES + x);
			unsigned short bottomLeft = (unsigned short)((z + 1) * GRID_RES + x);
			unsigned short bottomRight = (unsigned short)((z + 1) * GRID_RES + (x + 1));
			unsigned short topRight = (unsigned short)(z * GRID_RES + (x + 1));

			// Triangle 1 (CCW)
			mesh.indices[triIdx++] = topLeft;
			mesh.indices[triIdx++] = bottomLeft;
			mesh.indices[triIdx++] = bottomRight;

			// Triangle 2 (CCW)
			mesh.indices[triIdx++] = topLeft;
			mesh.indices[triIdx++] = bottomRight;
			mesh.indices[triIdx++] = topRight;
		}
	}

	// Upload with dynamic = true (allows updating vertex buffer every frame)
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

	InitWindow(screenWidth, screenHeight, "Graphics Lab 04: Meshes, Vertices & Index Buffers");
	SetTargetFPS(60);

	// Camera setup (Spherical Orbit)
	Camera3D camera = { 0 };
	camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	float cameraYaw = 0.6f;
	float cameraPitch = 0.45f;
	float cameraDistance = 11.0f;

	// 2. Create Meshes & Models
	Mesh pyramidMesh = CreatePyramidMesh();
	Model pyramidModel = LoadModelFromMesh(pyramidMesh);

	Mesh waveMesh = CreateWaveGridMesh(8.0f);
	Model waveModel = LoadModelFromMesh(waveMesh);

	int currentMode = 1; // 1 = Pyramid, 2 = Dynamic Terrain
	bool wireframe = false;
	float waveTime = 0.0f;
	bool isPaused = false;

	// -------------------------------------------------------------------------
	// 3. MAIN LOOP
	// -------------------------------------------------------------------------
	while (!WindowShouldClose()) {
		float dt = GetFrameTime();

		// --- STEP A: INPUT & CAMERA UPDATE ---
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

		// Spherical Orbit Math
		camera.position.x = camera.target.x + cameraDistance * cosf(cameraPitch) * sinf(cameraYaw);
		camera.position.y = camera.target.y + cameraDistance * sinf(cameraPitch);
		camera.position.z = camera.target.z + cameraDistance * cosf(cameraPitch) * cosf(cameraYaw);

		// Keyboard hotkeys
		if (IsKeyPressed(KEY_ONE))
			currentMode = 1;
		if (IsKeyPressed(KEY_TWO))
			currentMode = 2;
		if (IsKeyPressed(KEY_W))
			wireframe = !wireframe;
		if (IsKeyPressed(KEY_SPACE))
			isPaused = !isPaused;

		// --- STEP B: DYNAMIC MESH STREAMING (Mode 2) ---
		if (currentMode == 2 && !isPaused) {
			waveTime += dt * 3.0f;

			// Recalculate Y (height) for each vertex using wave functions
			float step = 8.0f / (GRID_RES - 1);
			float halfSize = 4.0f;

			for (int z = 0; z < GRID_RES; z++) {
				for (int x = 0; x < GRID_RES; x++) {
					int idx = (z * GRID_RES + x) * 3;
					float posX = (x * step) - halfSize;
					float posZ = (z * step) - halfSize;

					// Superimpose two sine waves
					float dist = sqrtf(posX * posX + posZ * posZ);
					waveMesh.vertices[idx + 1] = sinf(dist * 1.5f - waveTime) * 0.55f +
								     cosf(posX * 1.0f + waveTime * 0.7f) * 0.25f;
				}
			}

			// Stream updated vertex buffer directly to GPU VRAM (buffer 0 = positions)
			UpdateMeshBuffer(waveMesh, 0, waveMesh.vertices, waveMesh.vertexCount * 3 * sizeof(float), 0);
		}

		// --- STEP C: RENDER ---
		BeginDrawing();
		ClearBackground((Color){ 16, 20, 30, 255 });

		BeginMode3D(camera);
		DrawGrid(12, 1.0f);

		// Coordinate origin axes
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 2.5f, 0, 0 }, RED);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 2.5f, 0 }, GREEN);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 0, 2.5f }, BLUE);

		Vector3 modelPos = { 0.0f, 0.5f, 0.0f };

		if (currentMode == 1) {
			// Mode 1: Custom Indexed Pyramid
			if (wireframe) {
				DrawModelWires(pyramidModel, modelPos, 1.0f, WHITE);
			} else {
				DrawModel(pyramidModel, modelPos, 1.0f, WHITE);
				DrawModelWires(pyramidModel, modelPos, 1.001f, (Color){ 20, 20, 20, 180 }); // Accent wireframe
			}

			// Visualize the 5 individual vertices as glowing points
			for (int i = 0; i < pyramidMesh.vertexCount; i++) {
				Vector3 vPos = {
					pyramidMesh.vertices[i * 3 + 0] + modelPos.x,
					pyramidMesh.vertices[i * 3 + 1] + modelPos.y,
					pyramidMesh.vertices[i * 3 + 2] + modelPos.z
				};
				DrawSphere(vPos, 0.08f, YELLOW);
			}
		} else {
			// Mode 2: Dynamic Wave Terrain
			if (wireframe) {
				DrawModelWires(waveModel, modelPos, 1.0f, SKYBLUE);
			} else {
				DrawModel(waveModel, modelPos, 1.0f, WHITE);
				DrawModelWires(waveModel, modelPos, 1.002f, (Color){ 30, 40, 60, 120 });
			}
		}

		EndMode3D();

		// -----------------------------------------------------------------
		// 2D HUD OVERLAY
		// -----------------------------------------------------------------
		DrawRectangle(15, 15, 430, 230, (Color){ 10, 14, 22, 230 });
		DrawRectangleLines(15, 15, 430, 230, (Color){ 50, 65, 95, 255 });

		DrawText("LESSON 04: Meshes & Index Buffers", 25, 25, 16, RAYWHITE);
		DrawText("Press [1]: Custom Colored Pyramid (5 vertices, 6 triangles)", 25, 50, 13, (currentMode == 1 ? YELLOW : GRAY));
		DrawText("Press [2]: Procedural Wave Grid (400 vertices, dynamic GPU)", 25, 70, 13, (currentMode == 2 ? YELLOW : GRAY));
		DrawText(TextFormat("Press [W]: Toggle Wireframe Mode  [%s]", wireframe ? "WIREFRAME" : "SOLID"), 25, 95, 13, SKYBLUE);
		DrawText("Press [SPACE]: Pause / Play wave animation", 25, 115, 13, LIGHTGRAY);
		DrawText("LEFT MOUSE: Orbit camera  |  SCROLL: Zoom", 25, 135, 13, LIGHTGRAY);

		// Technical Architecture stats
		DrawLine(25, 160, 420, 160, (Color){ 40, 50, 75, 255 });
		if (currentMode == 1) {
			DrawText("PYRAMID TOPOLOGY STATS:", 25, 170, 12, GREEN);
			DrawText("* Unique Vertices: 5 (vs 18 unindexed = 72% memory saved!)", 25, 188, 12, RAYWHITE);
			DrawText("* Index Buffer: 18 uint16s stitching 6 CCW triangles", 25, 206, 12, RAYWHITE);
		} else {
			DrawText("TERRAIN TOPOLOGY STATS:", 25, 170, 12, GREEN);
			DrawText(TextFormat("* Grid Resolution: %dx%d = %d vertices", GRID_RES, GRID_RES, waveMesh.vertexCount), 25, 188, 12, RAYWHITE);
			DrawText(TextFormat("* Indices: %d uint16s stitching %d triangles", waveMesh.triangleCount * 3, waveMesh.triangleCount), 25, 206, 12, RAYWHITE);
		}

		DrawFPS(screenWidth - 90, 20);

		EndDrawing();
	}

	// -------------------------------------------------------------------------
	// 4. CLEANUP (Free CPU & GPU buffers)
	// -------------------------------------------------------------------------
	UnloadModel(pyramidModel); // Automatically unloads pyramidMesh VBOs/VAO
	UnloadModel(waveModel); // Automatically unloads waveMesh VBOs/VAO
	CloseWindow();

	return 0;
}
