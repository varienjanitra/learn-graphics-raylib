#include "raylib.h"
#include "raymath.h"
#include <stddef.h>
#include <math.h>

// -----------------------------------------------------------------------------
// POST-PROCESSING GLSL FRAGMENT SHADER
// Multi-effect screen shader operating on the rendered framebuffer texture
// -----------------------------------------------------------------------------
static const char *postProcessFS =
	"#version 330\n"
	"in vec2 fragTexCoord;\n"
	"in vec4 fragColor;\n"
	"uniform sampler2D texture0;\n" // Offscreen framebuffer texture
	"uniform int uEffect;\n" // 0: None, 1: Grayscale, 2: CRT, 3: Chromatic, 4: Invert
	"uniform float uTime;\n"
	"uniform vec2 uResolution;\n"
	"out vec4 finalColor;\n"

	// Pseudo-random noise function
	"float rand(vec2 co) {\n"
	"    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);\n"
	"}\n"

	"void main() {\n"
	"    vec2 uv = fragTexCoord;\n"

	// EFFECT 0: Pass-Through (Normal)
	"    if (uEffect == 0) {\n"
	"        finalColor = texture(texture0, uv);\n"
	"        return;\n"
	"    }\n"

	// EFFECT 1: Grayscale & Cinematic Vignette
	"    if (uEffect == 1) {\n"
	"        vec4 col = texture(texture0, uv);\n"
	"        // Standard NTSC Luminance conversion (human eye is most sensitive to green)\n"
	"        float gray = dot(col.rgb, vec3(0.299, 0.587, 0.114));\n"
	"        // High contrast curve\n"
	"        gray = pow(gray, 1.2);\n"
	"        // Vignette: darken corners based on distance from center\n"
	"        float dist = distance(uv, vec2(0.5, 0.5));\n"
	"        float vignette = smoothstep(0.7, 0.25, dist);\n"
	"        finalColor = vec4(vec3(gray) * vignette, 1.0);\n"
	"        return;\n"
	"    }\n"

	// EFFECT 2: Retro CRT Monitor (Curvature, Scanlines, Phosphor glow)
	"    if (uEffect == 2) {\n"
	"        // Barrel curvature distortion\n"
	"        vec2 centered = uv - 0.5;\n"
	"        float r2 = dot(centered, centered);\n"
	"        vec2 crtUV = centered * (1.0 + 0.18 * r2) + 0.5;\n"
	"        if (crtUV.x < 0.0 || crtUV.x > 1.0 || crtUV.y < 0.0 || crtUV.y > 1.0) {\n"
	"            finalColor = vec4(0.02, 0.02, 0.04, 1.0);\n" // Black bezel outside CRT glass
	"            return;\n"
	"        }\n"
	"        vec4 col = texture(texture0, crtUV);\n"
	"        // Scanlines\n"
	"        float scanline = sin(crtUV.y * uResolution.y * 1.5 - uTime * 8.0) * 0.15;\n"
	"        col.rgb -= scanline;\n"
	"        // Vignette\n"
	"        float vignette = smoothstep(0.75, 0.35, length(centered));\n"
	"        col.rgb *= vignette;\n"
	"        // Slight greenish phosphor tint\n"
	"        col.rgb *= vec3(0.9, 1.05, 0.9);\n"
	"        finalColor = col;\n"
	"        return;\n"
	"    }\n"

	// EFFECT 3: Chromatic Aberration & Lens Film Grain
	"    if (uEffect == 3) {\n"
	"        vec2 distFromCenter = uv - 0.5;\n"
	"        float offset = length(distFromCenter) * 0.015;\n"
	"        // Sample R, G, B at slightly different coordinates\n"
	"        float r = texture(texture0, uv + distFromCenter * offset).r;\n"
	"        float g = texture(texture0, uv).g;\n"
	"        float b = texture(texture0, uv - distFromCenter * offset).b;\n"
	"        // Subtle animated film grain\n"
	"        float noise = (rand(uv + fract(uTime)) - 0.5) * 0.08;\n"
	"        finalColor = vec4(vec3(r, g, b) + noise, 1.0);\n"
	"        return;\n"
	"    }\n"

	// EFFECT 4: Color Inversion / Cyber Thermal Vision
	"    if (uEffect == 4) {\n"
	"        vec4 col = texture(texture0, uv);\n"
	"        vec3 inverted = vec3(1.0) - col.rgb;\n"
	"        // Tint with cyan-purple thermal gradient\n"
	"        inverted = mix(inverted, vec3(0.2, 0.9, 0.8), 0.35);\n"
	"        finalColor = vec4(inverted, 1.0);\n"
	"        return;\n"
	"    }\n"
	"}\n";

int main(void)
{
	// -------------------------------------------------------------------------
	// 1. INITIALIZATION
	// -------------------------------------------------------------------------
	const int screenWidth = 960;
	const int screenHeight = 650;

	InitWindow(screenWidth, screenHeight, "Graphics Lab 08: Framebuffers & Post-Processing");
	SetTargetFPS(60);

	// 3D Camera Setup
	Camera3D camera = { 0 };
	camera.target = (Vector3){ 0.0f, 1.2f, 0.0f };
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	float cameraYaw = 0.8f;
	float cameraPitch = 0.45f;
	float cameraDistance = 12.0f;

	// -------------------------------------------------------------------------
	// 2. CREATE OFFSCREEN FRAMEBUFFER (RenderTexture2D)
	// -------------------------------------------------------------------------
	// Instead of rendering directly to the monitor, we render into this GPU FBO texture!
	RenderTexture2D targetFBO = LoadRenderTexture(screenWidth, screenHeight);
	SetTextureFilter(targetFBO.texture, TEXTURE_FILTER_BILINEAR);

	// Load Post-Processing Fragment Shader
	Shader postShader = LoadShaderFromMemory(NULL, postProcessFS);
	int locEffect = GetShaderLocation(postShader, "uEffect");
	int locTime = GetShaderLocation(postShader, "uTime");
	int locResolution = GetShaderLocation(postShader, "uResolution");

	Vector2 res = { (float)screenWidth, (float)screenHeight };
	SetShaderValue(postShader, locResolution, &res, SHADER_UNIFORM_VEC2);

	int currentEffect = 2; // Default to Retro CRT effect
	float shaderTime = 0.0f;
	float sceneRotation = 0.0f;
	bool isPaused = false;
	bool splitScreen = false;

	// -------------------------------------------------------------------------
	// 3. MAIN LOOP
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

		// Effect selection hotkeys
		if (IsKeyPressed(KEY_ZERO))
			currentEffect = 0; // None
		if (IsKeyPressed(KEY_ONE))
			currentEffect = 1; // Grayscale & Vignette
		if (IsKeyPressed(KEY_TWO))
			currentEffect = 2; // CRT Scanlines
		if (IsKeyPressed(KEY_THREE))
			currentEffect = 3; // Chromatic Aberration
		if (IsKeyPressed(KEY_FOUR))
			currentEffect = 4; // Thermal / Invert
		if (IsKeyPressed(KEY_TAB))
			splitScreen = !splitScreen; // Toggle side-by-side comparison
		if (IsKeyPressed(KEY_SPACE))
			isPaused = !isPaused;

		if (!isPaused) {
			shaderTime += dt;
			sceneRotation += 35.0f * dt;
		}

		// Pass Uniforms to Shader
		SetShaderValue(postShader, locEffect, &currentEffect, SHADER_UNIFORM_INT);
		SetShaderValue(postShader, locTime, &shaderTime, SHADER_UNIFORM_FLOAT);

		// =====================================================================
		// PASS 1: RENDER 3D SCENE TO OFFSCREEN FRAMEBUFFER (FBO)
		// =====================================================================
		BeginTextureMode(targetFBO);
		ClearBackground((Color){ 20, 24, 34, 255 });

		BeginMode3D(camera);
		DrawGrid(14, 1.0f);

		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 3.0f, 0, 0 }, RED);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 3.0f, 0 }, GREEN);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 0, 3.0f }, BLUE);

		// Central spinning shape
		Vector3 pos = { 0.0f, 1.5f, 0.0f };
		DrawCubeWires(pos, 2.4f, 2.4f, 2.4f, WHITE);
		DrawCube(pos, 2.38f, 2.38f, 2.38f, (Color){ 230, 70, 70, 255 });

		// Orbiting satellite shapes
		float rad = DEG2RAD * sceneRotation;
		Vector3 orb1 = { cosf(rad) * 4.0f, 1.5f, sinf(rad) * 4.0f };
		DrawSphere(orb1, 0.7f, (Color){ 60, 160, 240, 255 });
		DrawSphereWires(orb1, 0.72f, 8, 8, SKYBLUE);

		Vector3 orb2 = { cosf(rad + 3.1415f) * 4.0f, 1.5f, sinf(rad + 3.1415f) * 4.0f };
		DrawCube(orb2, 1.2f, 1.2f, 1.2f, (Color){ 240, 180, 50, 255 });
		DrawCubeWires(orb2, 1.22f, 1.22f, 1.22f, GOLD);

		DrawPlane((Vector3){ 0, -0.01f, 0 }, (Vector2){ 14.0f, 14.0f }, (Color){ 30, 40, 55, 255 });

		EndMode3D();
		EndTextureMode();

		// =====================================================================
		// PASS 2: RENDER FULLSCREEN QUAD WITH POST-PROCESSING SHADER TO SCREEN
		// =====================================================================
		BeginDrawing();
		ClearBackground(BLACK);

		if (splitScreen) {
			// Split Screen: Left Half = Raw 3D, Right Half = Post-Processed
			int halfW = screenWidth / 2;

			// 1. Raw Left Half
			Rectangle rawSrc = { 0.0f, 0.0f, (float)halfW, -(float)targetFBO.texture.height };
			Rectangle rawDst = { 0.0f, 0.0f, (float)halfW, (float)screenHeight };
			DrawTexturePro(targetFBO.texture, rawSrc, rawDst, (Vector2){ 0, 0 }, 0.0f, WHITE);

			// 2. Post-Processed Right Half
			BeginShaderMode(postShader);
			Rectangle postSrc = { (float)halfW, 0.0f, (float)halfW, -(float)targetFBO.texture.height };
			Rectangle postDst = { (float)halfW, 0.0f, (float)halfW, (float)screenHeight };
			DrawTexturePro(targetFBO.texture, postSrc, postDst, (Vector2){ 0, 0 }, 0.0f, WHITE);
			EndShaderMode();

			// Divider line
			DrawLine(halfW, 0, halfW, screenHeight, YELLOW);
			DrawText("RAW 3D FRAMEBUFFER", 20, screenHeight - 35, 14, RAYWHITE);
			DrawText("POST-PROCESSED", halfW + 20, screenHeight - 35, 14, YELLOW);
		} else {
			// Fullscreen Post-Processing
			BeginShaderMode(postShader);
			// Note: In OpenGL FBO textures are Y-inverted, so negative height flips it right side up!
			Rectangle sourceRec = { 0.0f, 0.0f, (float)targetFBO.texture.width, -(float)targetFBO.texture.height };
			Rectangle destRec = { 0.0f, 0.0f, (float)screenWidth, (float)screenHeight };
			DrawTexturePro(targetFBO.texture, sourceRec, destRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
			EndShaderMode();
		}

		// -----------------------------------------------------------------
		// 2D HUD OVERLAY
		// -----------------------------------------------------------------
		DrawRectangle(15, 15, 450, 240, (Color){ 10, 14, 22, 230 });
		DrawRectangleLines(15, 15, 450, 240, (Color){ 50, 65, 95, 255 });

		DrawText("LESSON 08: Framebuffers & Post-Processing Shaders", 25, 25, 15, RAYWHITE);

		const char *effectNames[] = {
			"[0] None (Raw FBO Pass-Through)",
			"[1] Grayscale & Vignette (Luminance dot)",
			"[2] Retro CRT Monitor (Curvature + Scanlines)",
			"[3] Chromatic Aberration & Lens Film Grain",
			"[4] Cyber Thermal Vision (Color Invert)"
		};
		for (int i = 0; i < 5; i++) {
			DrawText(effectNames[i], 25, 50 + (i * 18), 12, (currentEffect == i) ? YELLOW : GRAY);
		}

		DrawText(TextFormat("[TAB] Split-Screen Compare: [%s]", splitScreen ? "SPLIT (ON)" : "FULLSCREEN"), 25, 148, 13, SKYBLUE);
		DrawText("[SPACE] Pause / Resume animation", 25, 168, 13, LIGHTGRAY);
		DrawText("LEFT MOUSE: Orbit camera  |  SCROLL: Zoom", 25, 188, 13, LIGHTGRAY);

		DrawLine(25, 210, 440, 210, (Color){ 40, 50, 75, 255 });
		DrawText("PIPELINE: Render 3D -> FBO Texture -> Screen Quad", 25, 220, 12, GREEN);

		DrawFPS(screenWidth - 90, 20);

		EndDrawing();
	}

	// -------------------------------------------------------------------------
	// 4. CLEANUP
	// -------------------------------------------------------------------------
	UnloadRenderTexture(targetFBO);
	UnloadShader(postShader);
	CloseWindow();

	return 0;
}
