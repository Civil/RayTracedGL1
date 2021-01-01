#pragma once

#include <cassert>
#include <memory>
#include <vulkan/vulkan.h>

#define MAX_FRAMES_IN_FLIGHT 2

#pragma region extension functions

// extension functions' lists
#define VK_INSTANCE_FUNCTION_LIST \
	VK_EXTENSION_FUNCTION(vkCmdBeginDebugUtilsLabelEXT) \
	VK_EXTENSION_FUNCTION(vkCmdEndDebugUtilsLabelEXT) \
	VK_EXTENSION_FUNCTION(vkCreateDebugUtilsMessengerEXT) \
	VK_EXTENSION_FUNCTION(vkDestroyDebugUtilsMessengerEXT)

#define VK_DEVICE_FUNCTION_LIST \
	VK_EXTENSION_FUNCTION(vkDebugMarkerSetObjectNameEXT) \
	VK_EXTENSION_FUNCTION(vkCreateAccelerationStructureKHR) \
	VK_EXTENSION_FUNCTION(vkDestroyAccelerationStructureKHR) \
	VK_EXTENSION_FUNCTION(vkGetRayTracingShaderGroupHandlesKHR) \
	VK_EXTENSION_FUNCTION(vkCreateRayTracingPipelinesKHR) \
	VK_EXTENSION_FUNCTION(vkGetAccelerationStructureDeviceAddressKHR) \
	VK_EXTENSION_FUNCTION(vkGetAccelerationStructureBuildSizesKHR) \
	VK_EXTENSION_FUNCTION(vkCmdBuildAccelerationStructuresKHR) \
	VK_EXTENSION_FUNCTION(vkCmdTraceRaysKHR)


// extension functions' declarations
#define VK_EXTENSION_FUNCTION(fname) extern PFN_##fname s##fname;
    VK_INSTANCE_FUNCTION_LIST
    VK_DEVICE_FUNCTION_LIST
#undef VK_EXTENSION_FUNCTION

void InitInstanceExtensionFunctions(VkInstance instance);
void InitDeviceExtensionFunctions(VkDevice device);

#pragma endregion


#define VK_CHECKERROR(x) assert(x == VK_SUCCESS)


#ifndef _NDEBUG
    #define SET_DEBUG_NAME(device, obj, type, name) AddDebugName(device, obj, type, name)
#else
    #define SET_DEBUG_NAME(device, obj, type, name) do{}while(0)
#endif


void AddDebugName(VkDevice device, uint64_t obj, VkDebugReportObjectTypeEXT type, const char *name);


// TODO: remove VBProperties from here 
struct VBProperties
{
    bool vertexArrayOfStructs;
    uint32_t positionStride;
    uint32_t normalStride;
    uint32_t texCoordStride;
    uint32_t colorStride;
};