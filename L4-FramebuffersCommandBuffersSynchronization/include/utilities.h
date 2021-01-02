
#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <vector>
#include <fstream>
#include <iostream>

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

#endif