#include "raylib.h"
#include "raymath.h"
#include <math.h>

// -----------------------------------------------------------------------------
// BLINN-PHONG LIGHTING GLSL SHADER
// -----------------------------------------------------------------------------
static const char *lightingVS =
	"#version 330\n"
	"in vec3 vertexPosition;\n"
	"in vec2 vertexTexCoord;\n"
	"in vec3 vertexNormal;\n"
	"in vec4 vertexColor;\n"
	"uniform mat4 mvp;\n"
	"uniform mat4 matModel;\n"
	"uniform mat4 matNormal;\n"
	"out vec3 fragPosition;\n"
	"out vec3 fragNormal;\n"
	"void main() {\n"
	"    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"
	"    fragNormal = normalize(mat3(matNormal) * vertexNormal);\n"
	"    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
	"}\n";

static const char *lightingFS =
	"#version 330\n"
	"in vec3 fragPosition;\n"
	"in vec3 fragNormal;\n"
	"uniform vec3 uLightPos;\n"
	"uniform vec3 uLightColor;\n"
	"uniform vec3 uViewPos;\n"
	"uniform vec3 uObjectColor;\n"
	"uniform float uAmbientStrength;\n"
	"uniform float uDiffuseStrength;\n"
	"uniform float uSpecularStrength;\n"
	"uniform float uShininess;\n"
	"out vec4 finalColor;\n"
	"void main() {\n"
	"    vec3 norm = normalize(fragNormal);\n"
	"    vec3 lightDir = normalize(uLightPos - fragPosition);\n"
	"    vec3 viewDir  = normalize(uViewPos - fragPosition);\n"
	"    // 1. Ambient Lighting (Baseline indirect light)\n"
	"    vec3 ambient = uAmbientStrength * uLightColor;\n"
	"    // 2. Diffuse Lighting (Lambert's Cosine Law: N dot L)\n"
	"    float diff = max(dot(norm, lightDir), 0.0);\n"
	"    vec3 diffuse = uDiffuseStrength * diff * uLightColor;\n"
	"    // 3. Specular Highlight (Blinn-Phong Halfway Vector: N dot H)\n"
	"    vec3 halfwayDir = normalize(lightDir + viewDir);\n"
	"    float spec = pow(max(dot(norm, halfwayDir), 0.0), uShininess);\n"
	"    vec3 specular = uSpecularStrength * spec * uLightColor;\n"
	"    // Final Lit Color\n"
	"    vec3 result = (ambient + diffuse + specular) * uObjectColor;\n"
	"    finalColor = vec4(result, 1.0);\n"
	"}\n";

int main(void)
{
	// -------------------------------------------------------------------------
	// 1. INITIALIZATION
	// -------------------------------------------------------------------------
	const int screenWidth = 960;
	const int screenHeight = 650;

	InitWindow(screenWidth, screenHeight, "Graphics Lab 07: Lighting & Surface Normals (Blinn-Phong)");
	SetTargetFPS(60);

	// Camera setup
	Camera3D camera = { 0 };
	camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	float cameraYaw = 0.8f;
	float cameraPitch = 0.45f;
	float cameraDistance = 12.0f;

	// 2. Compile Lighting Shader
	Shader shader = LoadShaderFromMemory(lightingVS, lightingFS);

	// Get Uniform Locations
	int locLightPos = GetShaderLocation(shader, "uLightPos");
	int locLightColor = GetShaderLocation(shader, "uLightColor");
	int locViewPos = GetShaderLocation(shader, "uViewPos");
	int locObjectColor = GetShaderLocation(shader, "uObjectColor");
	int locAmbStrength = GetShaderLocation(shader, "uAmbientStrength");
	int locDiffStrength = GetShaderLocation(shader, "uDiffuseStrength");
	int locSpecStrength = GetShaderLocation(shader, "uSpecularStrength");
	int locShininess = GetShaderLocation(shader, "uShininess");

	// Default Lighting Parameters
	Vector3 lightColor = { 1.0f, 1.0f, 0.95f }; // Warm white light
	float ambientStrength = 0.15f;
	float diffuseStrength = 0.85f;
	float specularStrength = 0.65f;
	float shininess = 32.0f; // High = tight glossy spot, Low = wide matte reflection

	bool enableAmbient = true;
	bool enableDiffuse = true;
	bool enableSpecular = true;
	bool isPaused = false;
	float lightOrbitAngle = 0.0f;

	// 3. Create Models
	Mesh sphereMesh = GenMeshSphere(1.2f, 32, 32);
	Model sphereModel = LoadModelFromMesh(sphereMesh);
	sphereModel.materials[0].shader = shader;

	Mesh cubeMesh = GenMeshCube(1.8f, 1.8f, 1.8f);
	Model cubeModel = LoadModelFromMesh(cubeMesh);
	cubeModel.materials[0].shader = shader;

	Mesh floorMesh = GenMeshPlane(14.0f, 14.0f, 1, 1);
	Model floorModel = LoadModelFromMesh(floorMesh);
	floorModel.materials[0].shader = shader;

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

		// Toggle Lighting Components (A, D, S)
		if (IsKeyPressed(KEY_A))
			enableAmbient = !enableAmbient;
		if (IsKeyPressed(KEY_D))
			enableDiffuse = !enableDiffuse;
		if (IsKeyPressed(KEY_S))
			enableSpecular = !enableSpecular;
		if (IsKeyPressed(KEY_SPACE))
			isPaused = !isPaused;

		// Adjust Shininess Exponent
		if (IsKeyDown(KEY_UP))
			shininess *= 1.04f;
		if (IsKeyDown(KEY_DOWN))
			shininess *= 0.96f;
		if (shininess < 2.0f)
			shininess = 2.0f;
		if (shininess > 256.0f)
			shininess = 256.0f;

		// Move the 3D Point Light in orbit
		if (!isPaused)
			lightOrbitAngle += 1.2f * dt;
		float lightRad = 4.8f;
		Vector3 lightPos = {
			cosf(lightOrbitAngle) * lightRad,
			3.0f + sinf(lightOrbitAngle * 2.0f) * 0.8f,
			sinf(lightOrbitAngle) * lightRad
		};

		// --- STEP B: PASS LIGHTING UNIFORMS TO GPU ---
		float curAmb = enableAmbient ? ambientStrength : 0.0f;
		float curDiff = enableDiffuse ? diffuseStrength : 0.0f;
		float curSpec = enableSpecular ? specularStrength : 0.0f;

		SetShaderValue(shader, locLightPos, &lightPos, SHADER_UNIFORM_VEC3);
		SetShaderValue(shader, locLightColor, &lightColor, SHADER_UNIFORM_VEC3);
		SetShaderValue(shader, locViewPos, &camera.position, SHADER_UNIFORM_VEC3);
		SetShaderValue(shader, locAmbStrength, &curAmb, SHADER_UNIFORM_FLOAT);
		SetShaderValue(shader, locDiffStrength, &curDiff, SHADER_UNIFORM_FLOAT);
		SetShaderValue(shader, locSpecStrength, &curSpec, SHADER_UNIFORM_FLOAT);
		SetShaderValue(shader, locShininess, &shininess, SHADER_UNIFORM_FLOAT);

		// --- STEP C: RENDER ---
		BeginDrawing();
		ClearBackground((Color){ 14, 18, 26, 255 });

		BeginMode3D(camera);
		DrawGrid(14, 1.0f);

		// Origin axes
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 3.0f, 0, 0 }, RED);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 3.0f, 0 }, GREEN);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 0, 3.0f }, BLUE);

		// 1. Draw Glowing Light Source Bulb
		DrawSphere(lightPos, 0.22f, YELLOW);
		DrawSphereWires(lightPos, 0.25f, 6, 6, ORANGE);
		// Draw light ray line downwards
		DrawLine3D(lightPos, (Vector3){ lightPos.x, 0, lightPos.z }, (Color){ 255, 255, 100, 60 });

		// 2. Draw Floor Plane (Deep Slate Blue)
		Vector3 floorColor = { 0.2f, 0.25f, 0.35f };
		SetShaderValue(shader, locObjectColor, &floorColor, SHADER_UNIFORM_VEC3);
		DrawModel(floorModel, (Vector3){ 0, -0.01f, 0 }, 1.0f, WHITE);

		// 3. Draw 3D Sphere (Vivid Red - Curved surface shows smooth specular highlights)
		Vector3 sphereColor = { 0.95f, 0.25f, 0.25f };
		SetShaderValue(shader, locObjectColor, &sphereColor, SHADER_UNIFORM_VEC3);
		DrawModel(sphereModel, (Vector3){ -1.8f, 1.2f, 0.0f }, 1.0f, WHITE);

		// 4. Draw 3D Cube (Emerald Green - Flat surfaces show per-face diffuse shading)
		Vector3 cubeColor = { 0.15f, 0.85f, 0.35f };
		SetShaderValue(shader, locObjectColor, &cubeColor, SHADER_UNIFORM_VEC3);
		DrawModel(cubeModel, (Vector3){ 1.8f, 0.9f, 0.0f }, 1.0f, WHITE);

		EndMode3D();

		// -----------------------------------------------------------------
		// 2D HUD OVERLAY
		// -----------------------------------------------------------------
		DrawRectangle(15, 15, 430, 245, (Color){ 10, 14, 22, 230 });
		DrawRectangleLines(15, 15, 430, 245, (Color){ 50, 65, 95, 255 });

		DrawText("LESSON 07: Lighting & Surface Normals (Blinn-Phong)", 25, 25, 15, RAYWHITE);
		DrawText(TextFormat("[A] Ambient:  %s (Base indirect light)", enableAmbient ? "ON" : "OFF"), 25, 52, 13, enableAmbient ? GREEN : RED);
		DrawText(TextFormat("[D] Diffuse:  %s (Lambertian N dot L)", enableDiffuse ? "ON" : "OFF"), 25, 72, 13, enableDiffuse ? GREEN : RED);
		DrawText(TextFormat("[S] Specular: %s (Blinn-Phong N dot H)", enableSpecular ? "ON" : "OFF"), 25, 92, 13, enableSpecular ? GREEN : RED);
		DrawText(TextFormat("[UP/DOWN] Shininess Exponent: %.1f", shininess), 25, 115, 13, YELLOW);
		DrawText("[SPACE] Pause / Play orbiting light source", 25, 135, 13, LIGHTGRAY);
		DrawText("LEFT MOUSE: Orbit camera  |  SCROLL: Zoom", 25, 155, 13, LIGHTGRAY);

		DrawLine(25, 180, 420, 180, (Color){ 40, 50, 75, 255 });
		DrawText("LIGHTING MATH FORMULA:", 25, 190, 12, SKYBLUE);
		DrawText("* Ambient  = Ka * LightColor", 25, 206, 12, RAYWHITE);
		DrawText("* Diffuse  = Kd * max(dot(N, L), 0.0) * LightColor", 25, 222, 12, RAYWHITE);

		DrawFPS(screenWidth - 90, 20);

		EndDrawing();
	}

	// -------------------------------------------------------------------------
	// 5. CLEANUP
	// -------------------------------------------------------------------------
	UnloadModel(sphereModel);
	UnloadModel(cubeModel);
	UnloadModel(floorModel);
	UnloadShader(shader);
	CloseWindow();

	return 0;
}
