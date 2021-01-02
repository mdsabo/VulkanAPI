
#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <vector>
#include <fstream>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

const int MAX_FRAME_DRAWS = 2;  // allow at most 2 images on the queue at once

const std::vector<const char*> DEVICE_EXTENSIONS =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset"
};

struct QueueFamilyIndices
{
    int graphicsFamily{-1};
    int presentationFamily{-1};

    bool isValid()
    {
        return graphicsFamily >= 0 && presentationFamily >= 0;
    }
};

struct SwapchainDetails
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct SwapchainImage
{
    VkImage image;
    VkImageView imageView;
};

static std::vector<char> readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary|std::ios::ate); // ate starts the buffer at the end of the file
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file");
    }

    size_t fsize = size_t(file.tellg());
    std::vector<char> contents(fsize);
    file.seekg(0); // go to start of file
    file.read(contents.data(), fsize); // read in all the data to our vector
    file.close();

    return contents;
}

// Vertex Data Representation
struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
};

static uint32_t findMemoryTypeIndex(VkPhysicalDevice physical, uint32_t allowedTypes, VkMemoryPropertyFlags flags)
{
    VkPhysicalDeviceMemoryProperties physicalMemoryProps;
    vkGetPhysicalDeviceMemoryProperties(physical, &physicalMemoryProps);

    for (uint32_t i = 0; i < physicalMemoryProps.memoryTypeCount; i++)
    {
        if ((allowedTypes & (1ul << i)) && (physicalMemoryProps.memoryTypes[i].propertyFlags & flags) == flags)
        {
            return i;
        }
    }
    return -1;
}

static void createBuffer(
    VkPhysicalDevice physical,
    VkDevice logical,
    VkDeviceSize bufferSize,
    VkBufferUsageFlags usageFlags,
    VkMemoryPropertyFlags bufferProperties,
    VkBuffer* buffer,
    VkDeviceMemory* memory
) {
    VkBufferCreateInfo bufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bufferSize,
        .usage = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    if (vkCreateBuffer(logical, &bufferInfo, nullptr, buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VkBuffer");
    }

    VkMemoryRequirements memoryReqs;
    vkGetBufferMemoryRequirements(logical, *buffer, &memoryReqs);

    VkMemoryAllocateInfo memoryAllocateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryReqs.size,
        .memoryTypeIndex = findMemoryTypeIndex(physical, memoryReqs.memoryTypeBits, bufferProperties)
    };

    if (vkAllocateMemory(logical, &memoryAllocateInfo, nullptr, memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Vertex Buffer memory");
    }

    vkBindBufferMemory(logical, *buffer, *memory, 0);
}

static void copyBuffer(
    VkDevice device,
    VkQueue transferQueue,
    VkCommandPool transferCmdPool,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize bufferSize
) {
    VkCommandBuffer transferCmdBuffer;

    VkCommandBufferAllocateInfo allocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = transferCmdPool,
        .commandBufferCount = 1
    };

    vkAllocateCommandBuffers(device, &allocInfo, &transferCmdBuffer);

    VkCommandBufferBeginInfo beginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(transferCmdBuffer, &beginInfo);

    VkBufferCopy bufferCopy =
    {
        .size = bufferSize,
        .srcOffset = 0,
        .dstOffset = 0,
    };

    vkCmdCopyBuffer(transferCmdBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);

    vkEndCommandBuffer(transferCmdBuffer);

    VkSubmitInfo submitInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &transferCmdBuffer
    };
    vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(transferQueue); // wait for trasnfer to finish (could be a problem if loading many, many meshes)

    vkFreeCommandBuffers(device, transferCmdPool, 1, &transferCmdBuffer);
}

#endif