
#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <vector>

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

#endif