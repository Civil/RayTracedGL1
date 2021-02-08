// Copyright (c) 2020-2021 Sultim Tsyrendashiev
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "PhysicalDevice.h"
#include <vector>

using namespace RTGL1;

PhysicalDevice::PhysicalDevice(VkInstance instance, uint32_t selectedPhysDevice)
{
    VkResult r;

    uint32_t physCount = 0;
    r = vkEnumeratePhysicalDevices(instance, &physCount, nullptr);
    assert(physCount > 0);
    assert(selectedPhysDevice < physCount);

    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(physCount);
    r = vkEnumeratePhysicalDevices(instance, &physCount, physicalDevices.data());
    VK_CHECKERROR(r);

    physDevice = physicalDevices[selectedPhysDevice];

    vkGetPhysicalDeviceMemoryProperties(physDevice, &memoryProperties);

    rtPipelineProperties = {};
    rtPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2 deviceProp2 = {};
    deviceProp2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProp2.pNext = &rtPipelineProperties;

    vkGetPhysicalDeviceProperties2(physDevice, &deviceProp2);
}

VkPhysicalDevice PhysicalDevice::Get() const
{
    return physDevice;
}

uint32_t PhysicalDevice::GetMemoryTypeIndex(uint32_t memoryTypeBits, VkFlags requirementsMask) const
{
    // for each memory type available for this device
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        // if type is available
        if ((memoryTypeBits & 1u) == 1)
        {
            if ((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)
            {
                return i;
            }
        }

        memoryTypeBits >>= 1u;
    }

    assert(0);
    return 0;
}

const VkPhysicalDeviceMemoryProperties &PhysicalDevice::GetMemoryProperties() const
{
    return memoryProperties;
}

const VkPhysicalDeviceRayTracingPipelinePropertiesKHR &PhysicalDevice::GetRTPipelineProperties() const
{
    return rtPipelineProperties;
}
