#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <math.h>

#define MAX_PARTICLES 40000
#define INITIAL_PARTICLES 12000

typedef struct {
	Vector2 pos;
	Vector2 vel;
	float radius;
	Color color;
} Particle;

// -----------------------------------------------------------------------------
// 1. GLSL SHADER: INSTANCED VERTEX SHADER
// -----------------------------------------------------------------------------
// Uses 'in mat4 instanceTransform' streamed by Raylib's DrawMeshInstanced()
static const char *instancingVS =
	"#version 330\n"
	"in vec3 vertexPosition;\n"
	"in vec2 vertexTexCoord;\n"
	"in mat4 instanceTransform;\n"
	"uniform mat4 mvp;\n"
	"out vec2 fragTexCoord;\n"
	"void main() {\n"
	"    fragTexCoord = vertexTexCoord;\n"
	"    gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);\n"
	"}\n";

// -----------------------------------------------------------------------------
// 2. GLSL SHADER: MODE 2 CIRCLE FRAGMENT SHADER (Discards pixels outside circle)
// -----------------------------------------------------------------------------
static const char *instancedCircleFS =
	"#version 330\n"
	"in vec2 fragTexCoord;\n"
	"out vec4 finalColor;\n"
	"void main() {\n"
	"    float d = length(fragTexCoord - vec2(0.5));\n"
	"    if (d > 0.5) discard;\n"
	"    float alpha = smoothstep(0.5, 0.46, d);\n"
	"    vec3 col = mix(vec3(0.2, 0.6, 1.0), vec3(0.85, 0.35, 1.0), fragTexCoord.y);\n"
	"    finalColor = vec4(col, alpha);\n"
	"}\n";

// -----------------------------------------------------------------------------
// 3. GLSL SHADER: MODE 3 SDF NEON GLOW FRAGMENT SHADER
// -----------------------------------------------------------------------------
static const char *sdfGlowFS =
	"#version 330\n"
	"in vec2 fragTexCoord;\n"
	"out vec4 finalColor;\n"
	"void main() {\n"
	"    float d = length(fragTexCoord - vec2(0.5));\n"
	"    float circle = smoothstep(0.48, 0.44, d);\n"
	"    float glow = exp(-d * 6.5) * 0.75;\n"
	"    vec3 coreColor = vec3(0.3, 0.95, 1.0);\n"
	"    vec3 glowColor = vec3(0.1, 0.35, 0.95);\n"
	"    vec3 col = coreColor * circle + glowColor * glow;\n"
	"    float alpha = clamp(circle + glow, 0.0, 1.0);\n"
	"    finalColor = vec4(col, alpha);\n"
	"}\n";

// -----------------------------------------------------------------------------
// HELPER: Generate a Clean 1x1 Centered Quad Mesh on XY Plane
// -----------------------------------------------------------------------------
Mesh CreateQuadMesh(void)
{
	Mesh mesh = { 0 };
	mesh.vertexCount = 4;
	mesh.triangleCount = 2;

	mesh.vertices = (float *)MemAlloc(4 * 3 * sizeof(float));
	mesh.texcoords = (float *)MemAlloc(4 * 2 * sizeof(float));
	mesh.indices = (unsigned short *)MemAlloc(6 * sizeof(unsigned short));

	// Centered 1x1 quad: from -0.5 to +0.5 on XY plane (Z = 0)
	mesh.vertices[0] = -0.5f;
	mesh.vertices[1] = -0.5f;
	mesh.vertices[2] = 0.0f;
	mesh.vertices[3] = -0.5f;
	mesh.vertices[4] = 0.5f;
	mesh.vertices[5] = 0.0f;
	mesh.vertices[6] = 0.5f;
	mesh.vertices[7] = 0.5f;
	mesh.vertices[8] = 0.0f;
	mesh.vertices[9] = 0.5f;
	mesh.vertices[10] = -0.5f;
	mesh.vertices[11] = 0.0f;

	mesh.texcoords[0] = 0.0f;
	mesh.texcoords[1] = 0.0f;
	mesh.texcoords[2] = 0.0f;
	mesh.texcoords[3] = 1.0f;
	mesh.texcoords[4] = 1.0f;
	mesh.texcoords[5] = 1.0f;
	mesh.texcoords[6] = 1.0f;
	mesh.texcoords[7] = 0.0f;

	mesh.indices[0] = 0;
	mesh.indices[1] = 1;
	mesh.indices[2] = 2;
	mesh.indices[3] = 0;
	mesh.indices[4] = 2;
	mesh.indices[5] = 3;

	UploadMesh(&mesh, false);
	return mesh;
}

int main(void)
{
	// -------------------------------------------------------------------------
	// 1. INITIALIZATION
	// -------------------------------------------------------------------------
	const int screenWidth = 1000;
	const int screenHeight = 680;

	InitWindow(screenWidth, screenHeight, "Performance Benchmark: Naive Immediate vs GPU Instanced vs SDF");
	SetTargetFPS(0); // Uncap FPS to measure true rendering throughput!

	// Standard 2D Camera for Screen Space (0,0 is top-left)
	Camera2D cam2d = { 0 };
	cam2d.zoom = 1.0f;

	// 2. Allocate Particles & Instancing Transforms
	Particle *particles = (Particle *)malloc(MAX_PARTICLES * sizeof(Particle));
	Matrix *transforms = (Matrix *)malloc(MAX_PARTICLES * sizeof(Matrix));

	for (int i = 0; i < MAX_PARTICLES; i++) {
		particles[i].pos = (Vector2){ (float)(rand() % screenWidth), (float)(rand() % screenHeight) };
		particles[i].vel = (Vector2){ ((float)(rand() % 240) - 120.0f) * 0.9f, ((float)(rand() % 240) - 120.0f) * 0.9f };
		particles[i].radius = (float)(rand() % 4 + 4); // 4 to 7 px radius
		particles[i].color = (Color){
			(unsigned char)(rand() % 100 + 155),
			(unsigned char)(rand() % 150 + 105),
			(unsigned char)(rand() % 100 + 155),
			240
		};
	}

	// 3. Create Shared 1x1 Quad Mesh for Instanced Drawing
	Mesh quadMesh = CreateQuadMesh();

	// 4. Compile Mode 2 Instanced Shader & Setup Shader Locations
	Shader instancedShader = LoadShaderFromMemory(instancingVS, instancedCircleFS);
	instancedShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(instancedShader, "mvp");
	instancedShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(instancedShader, "instanceTransform");

	Material instancedMat = LoadMaterialDefault();
	instancedMat.shader = instancedShader;

	// 5. Compile Mode 3 SDF Glow Shader & Setup Shader Locations
	Shader sdfShader = LoadShaderFromMemory(instancingVS, sdfGlowFS);
	sdfShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(sdfShader, "mvp");
	sdfShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(sdfShader, "instanceTransform");

	Material sdfMat = LoadMaterialDefault();
	sdfMat.shader = sdfShader;

	int activeCount = INITIAL_PARTICLES;
	int mode = 2; // Default to Mode 2: Modern GPU Instancing
	bool mouseVortex = false;

	// -------------------------------------------------------------------------
	// 6. MAIN BENCHMARK LOOP
	// -------------------------------------------------------------------------
	while (!WindowShouldClose()) {
		float dt = GetFrameTime();
		if (dt > 0.05f)
			dt = 0.05f;

		Vector2 mouse = GetMousePosition();
		mouseVortex = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

		// --- STEP A: SIMULATION (Physics) ---
		for (int i = 0; i < activeCount; i++) {
			if (mouseVortex) {
				Vector2 diff = Vector2Subtract(mouse, particles[i].pos);
				float dist = Vector2Length(diff);
				if (dist > 10.0f) {
					Vector2 force = Vector2Scale(Vector2Normalize(diff), 900.0f);
					particles[i].vel = Vector2Add(particles[i].vel, Vector2Scale(force, dt));
				}
			}

			particles[i].pos.x += particles[i].vel.x * dt;
			particles[i].pos.y += particles[i].vel.y * dt;

			// Bounce off edges
			if (particles[i].pos.x < 0) {
				particles[i].pos.x = 0;
				particles[i].vel.x *= -0.95f;
			}
			if (particles[i].pos.x > screenWidth) {
				particles[i].pos.x = screenWidth;
				particles[i].vel.x *= -0.95f;
			}
			if (particles[i].pos.y < 0) {
				particles[i].pos.y = 0;
				particles[i].vel.y *= -0.95f;
			}
			if (particles[i].pos.y > screenHeight) {
				particles[i].pos.y = screenHeight;
				particles[i].vel.y *= -0.95f;
			}

			// Compute Instanced Transformation Matrix for GPU (Modes 2 & 3)
			// Scale quad by diameter, then translate to particle position on screen!
			float diameter = particles[i].radius * 2.0f;
			transforms[i] = MatrixMultiply(
				MatrixScale(diameter, diameter, 1.0f),
				MatrixTranslate(particles[i].pos.x, particles[i].pos.y, 0.0f));
		}

		// Hotkeys
		if (IsKeyPressed(KEY_ONE))
			mode = 1; // Naive CPU Immediate
		if (IsKeyPressed(KEY_TWO))
			mode = 2; // GPU Instanced
		if (IsKeyPressed(KEY_THREE))
			mode = 3; // SDF Glow Shader

		// Adjust particle count
		if (IsKeyDown(KEY_UP) && activeCount < MAX_PARTICLES)
			activeCount += 500;
		if (IsKeyDown(KEY_DOWN) && activeCount > 1000)
			activeCount -= 500;

		// --- STEP B: RENDER ---
		BeginDrawing();
		ClearBackground((Color){ 10, 12, 18, 255 }); // Dark obsidian

		// -----------------------------------------------------------------
		// ARCHITECTURE 1: NAIVE CPU IMMEDIATE MODE (DrawCircleV in a loop)
		// -----------------------------------------------------------------
		if (mode == 1) {
			for (int i = 0; i < activeCount; i++) {
				DrawCircleV(particles[i].pos, particles[i].radius, particles[i].color);
			}
		}
		// -----------------------------------------------------------------
		// ARCHITECTURE 2: MODERN GPU HARDWARE INSTANCING (Single Draw Call)
		// -----------------------------------------------------------------
		else if (mode == 2) {
			BeginMode2D(cam2d);
			DrawMeshInstanced(quadMesh, instancedMat, transforms, activeCount);
			EndMode2D();
		}
		// -----------------------------------------------------------------
		// ARCHITECTURE 3: MODERN SDF (SIGNED DISTANCE FIELD) SHADER
		// -----------------------------------------------------------------
		else if (mode == 3) {
			// Enable additive blending for glowing particles
			BeginBlendMode(BLEND_ADDITIVE);
			BeginMode2D(cam2d);
			DrawMeshInstanced(quadMesh, sdfMat, transforms, activeCount);
			EndMode2D();
			BeginBlendMode(BLEND_ALPHA); // Restore normal blend mode
		}

		// -----------------------------------------------------------------
		// 2D LIVE BENCHMARK DASHBOARD
		// -----------------------------------------------------------------
		DrawRectangle(15, 15, 470, 255, (Color){ 8, 10, 16, 235 });
		DrawRectangleLines(15, 15, 470, 255, (Color){ 50, 65, 95, 255 });

		DrawText("PRIMITIVE RENDERING ARCHITECTURE BENCHMARK", 25, 25, 15, RAYWHITE);

		const char *modeTitle = (mode == 1) ? "MODE 1: Naive CPU Immediate Mode (DrawCircle)" :
						      ((mode == 2) ? "MODE 2: Modern GPU Instancing (DrawMeshInstanced)" :
								     "MODE 3: Modern SDF Fragment Shader (Neon Glow)");
		Color modeColor = (mode == 1) ? RED : ((mode == 2) ? GREEN : SKYBLUE);
		DrawText(modeTitle, 25, 52, 13, modeColor);

		DrawText("[1] Naive Immediate  |  [2] GPU Instanced  |  [3] SDF Shader", 25, 75, 12, LIGHTGRAY);
		DrawText("[UP / DOWN ARROWS] Change Particle Count (+/- 500)", 25, 95, 12, YELLOW);
		DrawText("[LEFT CLICK & DRAG MOUSE] Attract particles into a vortex", 25, 115, 12, LIGHTGRAY);

		DrawLine(25, 140, 465, 140, (Color){ 40, 50, 75, 255 });

		int currentFPS = GetFPS();
		float frameMs = dt * 1000.0f;
		Color fpsColor = (currentFPS >= 50) ? GREEN : ((currentFPS >= 30) ? ORANGE : RED);

		DrawText(TextFormat("Active Primitives: %d particles", activeCount), 25, 150, 14, RAYWHITE);
		DrawText(TextFormat("FPS: %d  (Frame Time: %.2f ms)", currentFPS, frameMs), 25, 172, 14, fpsColor);

		int drawCalls = (mode == 1) ? activeCount : 1;
		DrawText(TextFormat("GPU Draw Calls: %d  (vs %d in naive)", drawCalls, activeCount), 25, 194, 13, (mode == 1 ? RED : GREEN));

		if (mode == 1) {
			DrawText("BOTTLENECK: CPU BUS STALL (Pushing millions of vertices/sec)", 25, 220, 11, RED);
			DrawText("* Notice how FPS plummets as you press [UP] to add particles!", 25, 238, 11, LIGHTGRAY);
		} else if (mode == 2) {
			DrawText("STATUS: GPU HARDWARE ACCELERATED (Zero CPU vertex churn)", 25, 220, 11, GREEN);
			DrawText("* 1 single mesh drawn 40,000 times in 1 call via DrawMeshInstanced.", 25, 238, 11, LIGHTGRAY);
		} else {
			DrawText("STATUS: MATHEMATICAL SDF (Zero polygon curves, 100% shader math)", 25, 220, 11, SKYBLUE);
			DrawText("* Smooth anti-aliased circles with additive neon glow halos.", 25, 238, 11, LIGHTGRAY);
		}

		DrawFPS(screenWidth - 90, 20);

		EndDrawing();
	}

	// -------------------------------------------------------------------------
	// 7. CLEANUP
	// -------------------------------------------------------------------------
	free(particles);
	free(transforms);
	UnloadMesh(quadMesh);
	UnloadShader(instancedShader);
	UnloadShader(sdfShader);
	CloseWindow();

	return 0;
}
