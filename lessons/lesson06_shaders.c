#include "raylib.h"
#include "raymath.h"
#include <math.h>

// -----------------------------------------------------------------------------
// GLSL SHADER 1: Vertex Wave Deformer + Procedural Gradient
// -----------------------------------------------------------------------------
static const char *vertexWaveVS =
	"#version 330\n"
	"in vec3 vertexPosition;\n"
	"in vec2 vertexTexCoord;\n"
	"in vec3 vertexNormal;\n"
	"in vec4 vertexColor;\n"
	"uniform mat4 mvp;\n"
	"uniform mat4 matModel;\n"
	"uniform mat4 matNormal;\n"
	"uniform float uTime;\n"
	"uniform float uIntensity;\n"
	"out vec2 fragTexCoord;\n"
	"out vec3 fragNormal;\n"
	"out vec3 fragPosition;\n"
	"void main() {\n"
	"    fragTexCoord = vertexTexCoord;\n"
	"    vec3 pos = vertexPosition;\n"
	"    // Vertex Shader modifies geometry on the GPU!\n"
	"    pos.y += sin(pos.x * 2.5 + uTime * 3.0) * cos(pos.z * 2.5 + uTime * 2.0) * (0.4 * uIntensity);\n"
	"    fragNormal = normalize(mat3(matNormal) * vertexNormal);\n"
	"    fragPosition = vec3(matModel * vec4(pos, 1.0));\n"
	"    gl_Position = mvp * vec4(pos, 1.0);\n"
	"}\n";

static const char *vertexWaveFS =
	"#version 330\n"
	"in vec2 fragTexCoord;\n"
	"in vec3 fragNormal;\n"
	"in vec3 fragPosition;\n"
	"uniform float uTime;\n"
	"out vec4 finalColor;\n"
	"void main() {\n"
	"    // Fragment Shader colors pixels based on height and time\n"
	"    float heightFactor = clamp((fragPosition.y + 0.5) / 1.5, 0.0, 1.0);\n"
	"    vec3 bottomColor = vec3(0.1, 0.4, 0.9); // Deep blue\n"
	"    vec3 topColor    = vec3(1.0, 0.3, 0.3); // Vivid coral\n"
	"    vec3 color = mix(bottomColor, topColor, heightFactor);\n"
	"    // Add animated gold ripples\n"
	"    float ripple = sin(fragPosition.y * 10.0 - uTime * 4.0) * 0.5 + 0.5;\n"
	"    color += vec3(1.0, 0.85, 0.2) * (ripple * 0.25);\n"
	"    finalColor = vec4(color, 1.0);\n"
	"}\n";

// -----------------------------------------------------------------------------
// GLSL SHADER 2: Cyberpunk Hologram (Fresnel Rim Lighting + Scanlines)
// -----------------------------------------------------------------------------
static const char *hologramVS =
	"#version 330\n"
	"in vec3 vertexPosition;\n"
	"in vec2 vertexTexCoord;\n"
	"in vec3 vertexNormal;\n"
	"uniform mat4 mvp;\n"
	"uniform mat4 matModel;\n"
	"uniform mat4 matNormal;\n"
	"out vec2 fragTexCoord;\n"
	"out vec3 fragNormal;\n"
	"out vec3 fragPosition;\n"
	"void main() {\n"
	"    fragTexCoord = vertexTexCoord;\n"
	"    fragNormal = normalize(mat3(matNormal) * vertexNormal);\n"
	"    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"
	"    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
	"}\n";

static const char *hologramFS =
	"#version 330\n"
	"in vec2 fragTexCoord;\n"
	"in vec3 fragNormal;\n"
	"in vec3 fragPosition;\n"
	"uniform float uTime;\n"
	"uniform vec3 uCameraPos;\n"
	"out vec4 finalColor;\n"
	"void main() {\n"
	"    // 1. Animated Scanlines across the surface\n"
	"    float scanline = sin(fragPosition.y * 25.0 - uTime * 6.0) * 0.5 + 0.5;\n"
	"    // 2. Fresnel Rim Lighting (Glancing angle glow)\n"
	"    vec3 viewDir = normalize(uCameraPos - fragPosition);\n"
	"    vec3 norm = normalize(fragNormal);\n"
	"    float rim = 1.0 - max(dot(viewDir, norm), 0.0);\n"
	"    rim = pow(rim, 2.2);\n"
	"    // 3. Cyberpunk Cyan/Blue Hologram Palette\n"
	"    vec3 holoColor = vec3(0.05, 0.85, 1.0);\n"
	"    vec3 color = holoColor * (0.25 + 0.35 * scanline + 1.2 * rim);\n"
	"    // Glitch flicker\n"
	"    float flicker = sin(uTime * 45.0) > 0.96 ? 0.4 : 1.0;\n"
	"    finalColor = vec4(color * flicker, 0.85);\n"
	"}\n";

int main(void)
{
	// -------------------------------------------------------------------------
	// 1. INITIALIZATION
	// -------------------------------------------------------------------------
	const int screenWidth = 960;
	const int screenHeight = 650;

	InitWindow(screenWidth, screenHeight, "Graphics Lab 06: The Modern GPU Pipeline & Shaders");
	SetTargetFPS(60);

	// Spherical Orbit Camera
	Camera3D camera = { 0 };
	camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	float cameraYaw = 0.8f;
	float cameraPitch = 0.4f;
	float cameraDistance = 11.0f;

	// 2. Compile GLSL Shaders directly from memory
	Shader waveShader = LoadShaderFromMemory(vertexWaveVS, vertexWaveFS);
	int waveLocTime = GetShaderLocation(waveShader, "uTime");
	int waveLocIntensity = GetShaderLocation(waveShader, "uIntensity");

	Shader holoShader = LoadShaderFromMemory(hologramVS, hologramFS);
	int holoLocTime = GetShaderLocation(holoShader, "uTime");
	int holoLocCameraPos = GetShaderLocation(holoShader, "uCameraPos");

	// 3. Create 3D Models
	// A high-tessellation sphere and torus to show off vertex/fragment shaders
	Mesh sphereMesh = GenMeshSphere(1.8f, 36, 36);
	Model sphereModel = LoadModelFromMesh(sphereMesh);

	Mesh torusMesh = GenMeshTorus(0.7f, 1.8f, 32, 32);
	Model torusModel = LoadModelFromMesh(torusMesh);

	int activeShader = 1; // 1 = Wave Deformer, 2 = Cyberpunk Hologram
	float shaderTime = 0.0f;
	float waveIntensity = 1.0f;
	bool isPaused = false;
	float modelRotation = 0.0f;

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

		// Hotkeys
		if (IsKeyPressed(KEY_ONE))
			activeShader = 1;
		if (IsKeyPressed(KEY_TWO))
			activeShader = 2;
		if (IsKeyPressed(KEY_SPACE))
			isPaused = !isPaused;

		if (IsKeyDown(KEY_UP))
			waveIntensity += 1.0f * dt;
		if (IsKeyDown(KEY_DOWN))
			waveIntensity -= 1.0f * dt;
		if (waveIntensity < 0.0f)
			waveIntensity = 0.0f;
		if (waveIntensity > 3.0f)
			waveIntensity = 3.0f;

		if (!isPaused) {
			shaderTime += dt;
			modelRotation += 20.0f * dt;
		}

		// --- STEP B: PASS UNIFORMS FROM CPU TO GPU ---
		if (activeShader == 1) {
			SetShaderValue(waveShader, waveLocTime, &shaderTime, SHADER_UNIFORM_FLOAT);
			SetShaderValue(waveShader, waveLocIntensity, &waveIntensity, SHADER_UNIFORM_FLOAT);
			torusModel.materials[0].shader = waveShader;
			sphereModel.materials[0].shader = waveShader;
		} else {
			SetShaderValue(holoShader, holoLocTime, &shaderTime, SHADER_UNIFORM_FLOAT);
			SetShaderValue(holoShader, holoLocCameraPos, &camera.position, SHADER_UNIFORM_VEC3);
			torusModel.materials[0].shader = holoShader;
			sphereModel.materials[0].shader = holoShader;
		}

		// --- STEP C: RENDER ---
		BeginDrawing();
		ClearBackground((Color){ 14, 16, 24, 255 });

		BeginMode3D(camera);
		DrawGrid(14, 1.0f);

		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 3.0f, 0, 0 }, RED);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 3.0f, 0 }, GREEN);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 0, 3.0f }, BLUE);

		Vector3 pos = { 0.0f, 1.5f, 0.0f };
		Vector3 rotAxis = { 0.4f, 1.0f, 0.2f };

		if (activeShader == 1) {
			// Draw Torus with Vertex Deformer Shader
			DrawModelEx(torusModel, pos, rotAxis, modelRotation, (Vector3){ 1.0f, 1.0f, 1.0f }, WHITE);
		} else {
			// Draw Sphere + Torus with Hologram Shader
			DrawModelEx(sphereModel, pos, rotAxis, modelRotation, (Vector3){ 1.0f, 1.0f, 1.0f }, WHITE);
		}

		EndMode3D();

		// -----------------------------------------------------------------
		// 2D HUD OVERLAY
		// -----------------------------------------------------------------
		DrawRectangle(15, 15, 440, 230, (Color){ 10, 14, 22, 230 });
		DrawRectangleLines(15, 15, 440, 230, (Color){ 50, 65, 95, 255 });

		DrawText("LESSON 06: The GPU Pipeline & Custom GLSL Shaders", 25, 25, 16, RAYWHITE);
		DrawText("[1] Vertex Wave Deformer Shader (CPU -> Vertex GPU)", 25, 52, 13, activeShader == 1 ? YELLOW : GRAY);
		DrawText("[2] Cyberpunk Hologram Shader (Fresnel Rim + Scanlines)", 25, 72, 13, activeShader == 2 ? YELLOW : GRAY);
		DrawText(TextFormat("[UP/DOWN] Wave Intensity: %.2fx", waveIntensity), 25, 95, 13, activeShader == 1 ? GREEN : DARKGRAY);
		DrawText("[SPACE] Pause / Play shader time", 25, 115, 13, LIGHTGRAY);
		DrawText("LEFT MOUSE: Orbit camera  |  SCROLL: Zoom", 25, 135, 13, LIGHTGRAY);

		DrawLine(25, 160, 430, 160, (Color){ 40, 50, 75, 255 });
		DrawText("PROGRAMMABLE PIPELINE CONCEPTS:", 25, 170, 12, SKYBLUE);
		if (activeShader == 1) {
			DrawText("* Vertex Shader executes on EVERY vertex to deform space.", 25, 188, 12, RAYWHITE);
			DrawText("* Uniforms ('uTime', 'uIntensity') sent from CPU every frame.", 25, 206, 12, RAYWHITE);
		} else {
			DrawText("* Fragment Shader calculates Fresnel Rim dot(ViewDir, Normal).", 25, 188, 12, RAYWHITE);
			DrawText("* Camera world position uniform feeds GPU specular glow.", 25, 206, 12, RAYWHITE);
		}

		DrawFPS(screenWidth - 90, 20);

		EndDrawing();
	}

	// -------------------------------------------------------------------------
	// 5. CLEANUP
	// -------------------------------------------------------------------------
	UnloadModel(sphereModel);
	UnloadModel(torusModel);
	UnloadShader(waveShader);
	UnloadShader(holoShader);
	CloseWindow();

	return 0;
}
