/*
================================================================================
LESSON 09 COMPANION: RAW VULKAN INITIALIZATION BLUEPRINT (PURE C)
================================================================================
This file serves as your reference guide for writing raw Vulkan code in C.
Every concept you learned in Lessons 1 through 8 maps directly to this sequence:

1. Instance Creation:       Initializes the Vulkan driver library.
2. Physical Device (GPU):   Queries and selects your dedicated GPU hardware.
3. Logical Device:          Creates your software interface to the GPU.
4. Graphics Queue:          The hardware queue where draw commands are submitted.
5. Command Pool:            Allocates memory for recording command buffers.
================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// When you link against Vulkan (or include volk.h), you include:
// #include <vulkan/vulkan.h>

// Pseudocode walkthrough of the exact Vulkan C setup sequence:

/*
int main(void) {
    // -------------------------------------------------------------------------
    // STEP 1: CREATE VULKAN INSTANCE
    // Corresponds to: InitWindow() / Raylib initialization
    // -------------------------------------------------------------------------
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "My First Raw Vulkan App",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Custom Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_2,
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = 0, // In real apps, includes window surface extensions (e.g., VK_KHR_surface)
        .enabledLayerCount = 0,     // In debug mode, enable "VK_LAYER_KHRONOS_validation"
    };

    VkInstance instance;
    if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance!\n");
        return 1;
    }
    printf("1. Vulkan Instance successfully created!\n");

    // -------------------------------------------------------------------------
    // STEP 2: SELECT PHYSICAL DEVICE (GPU)
    // Corresponds to: Raylib auto-detecting GPU hardware
    // -------------------------------------------------------------------------
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        fprintf(stderr, "No GPU with Vulkan support found!\n");
        return 1;
    }

    VkPhysicalDevice physicalDevices[deviceCount];
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices);
    VkPhysicalDevice physicalDevice = physicalDevices[0]; // Choose first GPU

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    printf("2. Selected GPU: %s\n", deviceProperties.deviceName);

    // -------------------------------------------------------------------------
    // STEP 3: FIND GRAPHICS QUEUE FAMILY
    // Corresponds to: The GPU channel that executes drawing commands
    // -------------------------------------------------------------------------
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    int graphicsFamily = -1;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
            break;
        }
    }

    // -------------------------------------------------------------------------
    // STEP 4: CREATE LOGICAL DEVICE & RETRIEVE QUEUE HANDLE
    // -------------------------------------------------------------------------
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queueCreateInfo,
        .queueCreateInfoCount = 1,
    };

    VkDevice device;
    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device!\n");
        return 1;
    }

    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    printf("3. Logical device and Graphics Queue created successfully!\n");

    // -------------------------------------------------------------------------
    // WHAT HAPPENS NEXT (The concepts from Lessons 4-8):
    // 1. Swapchain:        Negotiates double/triple-buffering presentation images.
    // 2. Render Pass:      Configures color/depth attachments (Lesson 8).
    // 3. VkPipeline (PSO): Bakes vertex layout, shaders, and blending (Lesson 6).
    // 4. VkBuffer:         Allocates VBO / IBO for vertex data (Lesson 4).
    // 5. Descriptors:      Binds textures and uniform buffers (Lessons 5 & 6).
    // 6. Command Buffers:  Records draw commands (Lesson 1 & 3).
    // -------------------------------------------------------------------------

    // CLEANUP
    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);
    printf("4. Vulkan successfully cleaned up!\n");

    return 0;
}
*/
int main(void)
{
	printf("=== VULKAN INITIALIZATION BLUEPRINT ===\n");
	printf("Open 'lessons/vulkan_starter_blueprint.c' to inspect the exact C structs\n");
	printf("for initializing Vulkan instances, physical devices, and queue families!\n");
	return 0;
}
