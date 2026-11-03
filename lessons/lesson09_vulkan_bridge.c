#include "raylib.h"
#include "raymath.h"
#include <math.h>

// -----------------------------------------------------------------------------
// VULKAN ARCHITECTURE DATA & CODE BLUEPRINTS
// -----------------------------------------------------------------------------
typedef struct {
	const char *title;
	const char *whatYouDidInRaylib;
	const char *whatVulkanDoes;
	const char *vulkanStructs;
	const char *codeSnippet;
	const char *keyInsight;
} VulkanBridgeTopic;

static const VulkanBridgeTopic topics[6] = {
	// -------------------------------------------------------------------------
	// TOPIC 0: BUFFERS & GPU MEMORY
	// -------------------------------------------------------------------------
	{
		.title = "1. Vertex & Index Buffers (Memory Allocation)",
		.whatYouDidInRaylib = "mesh.vertices = malloc(...);\nUploadMesh(&mesh, false);",
		.whatVulkanDoes = "In Vulkan, creating a buffer does NOT allocate memory!\n"
				  "1. Create VkBuffer handle (specifies size and usage: VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)\n"
				  "2. Query VkMemoryRequirements (size and alignment)\n"
				  "3. Allocate VkDeviceMemory from GPU VRAM\n"
				  "4. Bind memory: vkBindBufferMemory(device, buffer, memory, 0)\n"
				  "5. Staging Buffer: Upload via CPU-visible RAM, then GPU DMA copies to fast VRAM!",
		.vulkanStructs = "VkBuffer, VkDeviceMemory, VkBufferCreateInfo, VkMemoryAllocateInfo",
		.codeSnippet =
			"VkBufferCreateInfo bufferInfo = {\n"
			"    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,\n"
			"    .size = sizeof(vertices[0]) * vertexCount,\n"
			"    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,\n"
			"    .sharingMode = VK_SHARING_MODE_EXCLUSIVE\n"
			"};\n"
			"vkCreateBuffer(device, &bufferInfo, NULL, &vertexBuffer);\n"
			"vkAllocateMemory(device, &allocInfo, NULL, &vertexMemory);\n"
			"vkBindBufferMemory(device, vertexBuffer, vertexMemory, 0);",
		.keyInsight = "Vulkan separates the buffer handle from the raw GPU memory allocation. This gives you exact control over VRAM defragmentation and memory pooling." },

	// -------------------------------------------------------------------------
	// TOPIC 1: SHADERS & PIPELINES (PSO)
	// -------------------------------------------------------------------------
	{
		.title = "2. The Pipeline State Object (VkGraphicsPipeline)",
		.whatYouDidInRaylib = "LoadShaderFromMemory(...);\nBeginShaderMode(shader);",
		.whatVulkanDoes = "In OpenGL, shader state and blending could be changed at any time.\n"
				  "In Vulkan, the entire graphics pipeline is IMMUTABLE!\n"
				  "You compile Shaders + Vertex Layout + Depth Testing + Blending + Culling into a single VkPipeline.\n"
				  "The GPU driver pre-compiles this whole state into machine code upfront, eliminating driver stutter!",
		.vulkanStructs = "VkGraphicsPipelineCreateInfo, VkPipelineShaderStageCreateInfo, VkPipelineVertexInputStateCreateInfo",
		.codeSnippet =
			"VkGraphicsPipelineCreateInfo pipelineInfo = {\n"
			"    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,\n"
			"    .stageCount = 2, .pStages = shaderStages,\n"
			"    .pVertexInputState = &vertexInputInfo,\n"
			"    .pInputAssemblyState = &inputAssembly,\n"
			"    .pViewportState = &viewportState,\n"
			"    .pRasterizationState = &rasterizer,\n"
			"    .pMultisampleState = &multisampling,\n"
			"    .pDepthStencilState = &depthStencil,\n"
			"    .pColorBlendState = &colorBlending,\n"
			"    .layout = pipelineLayout, .renderPass = renderPass\n"
			"};\n"
			"vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline);",
		.keyInsight = "No runtime driver re-validation! Once created, switching pipelines is near-instantaneous (vkCmdBindPipeline)." },

	// -------------------------------------------------------------------------
	// TOPIC 2: DESCRIPTORS & UNIFORMS
	// -------------------------------------------------------------------------
	{
		.title = "3. Descriptors & Uniforms (Data Binding)",
		.whatYouDidInRaylib = "SetShaderValue(shader, locTime, &time, SHADER_UNIFORM_FLOAT);\nmodel.materials[0].maps[...].texture = tex;",
		.whatVulkanDoes = "Vulkan does not have individual 'SetUniform' calls.\n"
				  "Instead, resources are grouped into Descriptor Sets:\n"
				  "1. Define a Descriptor Set Layout (the blueprint of what shaders expect: uniform buffers, samplers)\n"
				  "2. Allocate a VkDescriptorSet from a VkDescriptorPool\n"
				  "3. Update descriptor set with buffer / image views (vkUpdateDescriptorSets)\n"
				  "4. Bind in command buffer: vkCmdBindDescriptorSets()",
		.vulkanStructs = "VkDescriptorSetLayout, VkDescriptorPool, VkDescriptorSet, VkWriteDescriptorSet",
		.codeSnippet =
			"VkDescriptorBufferInfo bufferInfo = {\n"
			"    .buffer = uniformBuffer, .offset = 0, .range = sizeof(UniformBufferObject)\n"
			"};\n"
			"VkWriteDescriptorSet descriptorWrite = {\n"
			"    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,\n"
			"    .dstSet = descriptorSet, .dstBinding = 0,\n"
			"    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,\n"
			"    .descriptorCount = 1, .pBufferInfo = &bufferInfo\n"
			"};\n"
			"vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);",
		.keyInsight = "Descriptors allow batching all uniforms and textures into a single pointer block, avoiding dozens of individual state change calls." },

	// -------------------------------------------------------------------------
	// TOPIC 3: RENDER PASSES & FRAMEBUFFERS
	// -------------------------------------------------------------------------
	{
		.title = "4. Render Passes & Swapchain Framebuffers",
		.whatYouDidInRaylib = "BeginDrawing(); ClearBackground(...); ... EndDrawing();\nLoadRenderTexture(...) for FBOs",
		.whatVulkanDoes = "Vulkan makes the render target configuration completely explicit:\n"
				  "1. A VkRenderPass defines what attachments exist (Color, Depth, Resolve),\n"
				  "   how they are loaded (CLEAR vs DONT_CARE), and how they are stored (STORE).\n"
				  "2. A VkFramebuffer binds actual VkImageView attachments to the render pass.\n"
				  "3. Mobile and tile-based GPUs use render pass descriptions to keep tile memory on-chip without writing to slow VRAM!",
		.vulkanStructs = "VkRenderPass, VkFramebuffer, VkAttachmentDescription, VkSubpassDescription",
		.codeSnippet =
			"VkRenderPassBeginInfo renderPassInfo = {\n"
			"    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,\n"
			"    .renderPass = renderPass,\n"
			"    .framebuffer = swapChainFramebuffers[imageIndex],\n"
			"    .renderArea.offset = {0, 0},\n"
			"    .renderArea.extent = swapChainExtent,\n"
			"    .clearValueCount = 2, .pClearValues = clearValues\n"
			"};\n"
			"vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);",
		.keyInsight = "Vulkan tells the GPU ahead of time whether it needs to preserve previous pixels, unlocking massive tile-based caching optimizations." },

	// -------------------------------------------------------------------------
	// TOPIC 4: COMMAND BUFFERS & RECORDING
	// -------------------------------------------------------------------------
	{
		.title = "5. Command Buffers & Queue Submission",
		.whatYouDidInRaylib = "DrawModel(...);\nDrawTriangle(...); (Direct CPU-side calls)",
		.whatVulkanDoes = "In Vulkan, draw calls are NEVER sent directly to the GPU in real time.\n"
				  "Instead, you record commands into a VkCommandBuffer ahead of time:\n"
				  "1. vkBeginCommandBuffer()\n"
				  "2. vkCmdBeginRenderPass(), vkCmdBindPipeline(), vkCmdBindDescriptorSets()\n"
				  "3. vkCmdBindVertexBuffers(), vkCmdDrawIndexed()\n"
				  "4. vkEndCommandBuffer()\n"
				  "Multiple CPU worker threads can record command buffers in parallel!",
		.vulkanStructs = "VkCommandPool, VkCommandBuffer, VkCommandBufferAllocateInfo, VkSubmitInfo",
		.codeSnippet =
			"vkBeginCommandBuffer(cmdBuffer, &beginInfo);\n"
			"    vkCmdBeginRenderPass(cmdBuffer, &passInfo, VK_SUBPASS_CONTENTS_INLINE);\n"
			"        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);\n"
			"        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);\n"
			"        vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);\n"
			"        vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);\n"
			"    vkCmdEndRenderPass(cmdBuffer);\n"
			"vkEndCommandBuffer(cmdBuffer);",
		.keyInsight = "This is why Vulkan gets 10x higher draw-call throughput than legacy OpenGL: recording draw calls can happen across all CPU cores simultaneously!" },

	// -------------------------------------------------------------------------
	// TOPIC 5: SYNCHRONIZATION & PRESENTATION
	// -------------------------------------------------------------------------
	{
		.title = "6. Synchronization: Semaphores, Fences & Presentation",
		.whatYouDidInRaylib = "EndDrawing(); (Automatically waits for VSync and swaps backbuffer)",
		.whatVulkanDoes = "Because the GPU and CPU run asynchronously, you must manually coordinate timing:\n"
				  "1. VkSemaphore: Synchronizes GPU-to-GPU tasks (e.g. 'Don't render until swapchain image is acquired').\n"
				  "2. VkFence: Synchronizes GPU-to-CPU tasks (e.g. 'CPU, wait here until GPU finishes rendering before writing next frame').\n"
				  "3. vkQueueSubmit(): Sends command buffer to the graphics hardware queue.\n"
				  "4. vkQueuePresentKHR(): Hands the finished image to the OS window manager for display.",
		.vulkanStructs = "VkSemaphore, VkFence, VkPresentInfoKHR, VkSubmitInfo",
		.codeSnippet =
			"VkSubmitInfo submitInfo = {\n"
			"    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,\n"
			"    .waitSemaphoreCount = 1, .pWaitSemaphores = &imageAvailableSemaphore,\n"
			"    .commandBufferCount = 1, .pCommandBuffers = &commandBuffer,\n"
			"    .signalSemaphoreCount = 1, .pSignalSemaphores = &renderFinishedSemaphore\n"
			"};\n"
			"vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);\n"
			"vkQueuePresentKHR(presentQueue, &presentInfo);",
		.keyInsight = "No hidden CPU stalls! You explicitly manage how many frames can be 'in flight' concurrently (typically 2 for double-buffering, 3 for triple-buffering)." }
};

int main(void)
{
	// -------------------------------------------------------------------------
	// 1. INITIALIZATION
	// -------------------------------------------------------------------------
	const int screenWidth = 1000;
	const int screenHeight = 680;

	InitWindow(screenWidth, screenHeight, "Graphics Lab 09: The Vulkan Bridge (Architecture Blueprint)");
	SetTargetFPS(60);

	// 3D Camera Setup
	Camera3D camera = { 0 };
	camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	float cameraYaw = 0.8f;
	float cameraPitch = 0.45f;
	float cameraDistance = 11.0f;

	int activeTopic = 0; // 0 to 5
	float sceneRotation = 0.0f;
	bool isPaused = false;

	// 3D Models
	Mesh cubeMesh = GenMeshCube(2.0f, 2.0f, 2.0f);
	Model cubeModel = LoadModelFromMesh(cubeMesh);

	// -------------------------------------------------------------------------
	// 2. MAIN LOOP
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

		// Topic Navigation Hotkeys (1 to 6)
		if (IsKeyPressed(KEY_ONE))
			activeTopic = 0;
		if (IsKeyPressed(KEY_TWO))
			activeTopic = 1;
		if (IsKeyPressed(KEY_THREE))
			activeTopic = 2;
		if (IsKeyPressed(KEY_FOUR))
			activeTopic = 3;
		if (IsKeyPressed(KEY_FIVE))
			activeTopic = 4;
		if (IsKeyPressed(KEY_SIX))
			activeTopic = 5;

		if (IsKeyPressed(KEY_RIGHT) && activeTopic < 5)
			activeTopic++;
		if (IsKeyPressed(KEY_LEFT) && activeTopic > 0)
			activeTopic--;
		if (IsKeyPressed(KEY_SPACE))
			isPaused = !isPaused;

		if (!isPaused) {
			sceneRotation += 35.0f * dt;
		}

		// --- STEP B: RENDER ---
		BeginDrawing();
		ClearBackground((Color){ 12, 15, 22, 255 }); // Obsidian dark background

		// 1. 3D Scene in Upper Right / Center
		BeginMode3D(camera);
		DrawGrid(10, 1.0f);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 2.5f, 0, 0 }, RED);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 2.5f, 0 }, GREEN);
		DrawLine3D((Vector3){ 0, 0, 0 }, (Vector3){ 0, 0, 2.5f }, BLUE);

		Vector3 pos = { 2.5f, 1.2f, 0.0f };
		Vector3 rotAxis = { 0.4f, 1.0f, 0.2f };
		DrawModelEx(cubeModel, pos, rotAxis, sceneRotation, (Vector3){ 1.0f, 1.0f, 1.0f }, (Color){ 230, 80, 60, 255 });
		DrawCubeWires(pos, 2.02f, 2.02f, 2.02f, WHITE);

		float rad = DEG2RAD * sceneRotation;
		Vector3 sat = { pos.x + cosf(rad) * 2.8f, pos.y + sinf(rad * 2.0f) * 0.5f, pos.z + sinf(rad) * 2.8f };
		DrawSphere(sat, 0.45f, (Color){ 50, 160, 240, 255 });
		DrawSphereWires(sat, 0.48f, 6, 6, SKYBLUE);
		EndMode3D();

		// -----------------------------------------------------------------
		// 2D VULKAN BRIDGE DASHBOARD
		// -----------------------------------------------------------------
		const VulkanBridgeTopic *topic = &topics[activeTopic];

		// Header Banner
		DrawRectangle(15, 12, screenWidth - 30, 48, (Color){ 18, 22, 34, 240 });
		DrawRectangleLines(15, 12, screenWidth - 30, 48, (Color){ 60, 75, 110, 255 });
		DrawText("LESSON 09: THE VULKAN BRIDGE - MAPPING GRAPHICS CONCEPTS TO VULKAN", 25, 20, 16, RAYWHITE);
		DrawText("Press [1]-[6] or LEFT/RIGHT to inspect each Vulkan subsystem  |  Drag mouse to rotate 3D preview", 25, 42, 12, LIGHTGRAY);
		DrawFPS(screenWidth - 85, 22);

		// Subsystem Tab Pills
		int tabW = (screenWidth - 30) / 6;
		const char *tabNames[] = { "1. Memory", "2. Pipeline", "3. Descriptors", "4. RenderPass", "5. Commands", "6. Sync" };
		for (int i = 0; i < 6; i++) {
			int tabX = 15 + i * tabW;
			bool isSelected = (i == activeTopic);
			DrawRectangle(tabX, 64, tabW - 4, 26, isSelected ? (Color){ 180, 50, 50, 255 } : (Color){ 25, 30, 45, 200 });
			DrawRectangleLines(tabX, 64, tabW - 4, 26, isSelected ? RED : (Color){ 50, 60, 85, 255 });
			DrawText(tabNames[i], tabX + 12, 70, 12, isSelected ? WHITE : GRAY);
		}

		// Main Technical Details Panel (Left Column: 560px wide)
		DrawRectangle(15, 95, 560, screenHeight - 110, (Color){ 16, 20, 30, 240 });
		DrawRectangleLines(15, 95, 560, screenHeight - 110, (Color){ 50, 65, 95, 255 });

		// Title
		DrawText(topic->title, 30, 110, 17, YELLOW);

		// Section 1: What you did in high-level graphics
		DrawText("What You Learned in Earlier Lessons (High-Level):", 30, 140, 13, GREEN);
		DrawRectangle(30, 158, 530, 42, (Color){ 10, 12, 18, 220 });
		DrawRectangleLines(30, 158, 530, 42, (Color){ 35, 45, 65, 255 });
		DrawText(topic->whatYouDidInRaylib, 38, 164, 12, LIGHTGRAY);

		// Section 2: How Vulkan handles it explicitly
		DrawText("How Vulkan Does It (Explicit Hardware Control):", 30, 212, 13, SKYBLUE);
		DrawText(topic->whatVulkanDoes, 30, 230, 11, RAYWHITE);

		// Section 3: Exact Vulkan C Code Snippet
		DrawText("Vulkan C Implementation Code:", 30, 335, 13, GOLD);
		DrawRectangle(30, 355, 530, 185, (Color){ 8, 10, 16, 250 });
		DrawRectangleLines(30, 355, 530, 185, (Color){ 45, 55, 80, 255 });
		DrawText(topic->codeSnippet, 38, 362, 11, (Color){ 160, 230, 170, 255 });

		// Section 4: Key Architectural Insight
		DrawRectangle(30, 552, 530, 75, (Color){ 25, 30, 45, 220 });
		DrawRectangleLines(30, 552, 530, 75, (Color){ 80, 100, 140, 255 });
		DrawText("KEY ARCHITECTURAL INSIGHT:", 40, 560, 11, ORANGE);
		DrawText(topic->keyInsight, 40, 578, 11, RAYWHITE);

		// Right Panel Legend / Quick Map (Width 390px, Bottom Right)
		DrawRectangle(screenWidth - 395, 390, 380, screenHeight - 405, (Color){ 14, 18, 26, 230 });
		DrawRectangleLines(screenWidth - 395, 390, 380, screenHeight - 405, (Color){ 50, 65, 95, 255 });

		DrawText("VULKAN 1:1 TRANSLATION CHEAT SHEET", screenWidth - 380, 405, 13, YELLOW);
		DrawLine(screenWidth - 380, 424, screenWidth - 30, 424, (Color){ 50, 60, 85, 255 });

		const char *cheatSheet[] = {
			"Mesh Buffer -> VkBuffer + VkDeviceMemory",
			"Shader -> VkShaderModule + SPIR-V",
			"Shader State -> VkGraphicsPipeline (PSO)",
			"Uniforms/Textures -> VkDescriptorSet",
			"FBO / Targets -> VkRenderPass + VkFramebuffer",
			"Draw Calls -> Recorded into VkCommandBuffer",
			"EndDrawing / VSync -> VkSemaphore + vkQueueSubmit"
		};
		for (int i = 0; i < 7; i++) {
			DrawText(cheatSheet[i], screenWidth - 380, 435 + (i * 22), 11, LIGHTGRAY);
		}

		EndDrawing();
	}

	// -------------------------------------------------------------------------
	// 3. CLEANUP
	// -------------------------------------------------------------------------
	UnloadModel(cubeModel);
	CloseWindow();

	return 0;
}
