#include "vulkan_renderer.h"

#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <set>

int VulkanRenderer::init(GLFWwindow* wnd)
{
    m_window = wnd;

    try
    {
        createInstance();
        createSurface();
        getPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
    }
    catch(const std::runtime_error& e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void VulkanRenderer::destroy()
{
    for (auto image : m_swapchainImages)
    {
        vkDestroyImageView(m_device.logical, image.imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_device.logical, m_swapchain, nullptr);
    vkDestroyDevice(m_device.logical, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface.surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

void VulkanRenderer::createInstance()
{

    // Convenience structure for developer info
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan App",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_2 // current is 1.2
    };

    std::vector<const char*> layers = {
        //"VK_LAYER_LUNARG_api_dump",
        "VK_LAYER_KHRONOS_validation"
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data()
    };

    // Create list to hold extensions
    std::vector<const char*> instanceExtensions;
    uint32_t glfwExtensionCount{0};
    const char** glfwExtensions{nullptr};
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Put all of our glfw extensions into the list
    for (int i = 0; i < glfwExtensionCount; i++)
    {
        instanceExtensions.push_back(glfwExtensions[i]);
    }

    if (!checkInstanceExtensionSupport(instanceExtensions))
    {
        throw std::runtime_error("VkInstance does not support required extensions");
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    // disable all validation layers
    #if defined(CFG_RELEASE)
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
    #endif

    // create instance
    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan Instance");
    }
}

void VulkanRenderer::createSurface()
{
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface.surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create GLFW surface");
    };
}

bool VulkanRenderer::checkInstanceExtensionSupport(const std::vector<const char*>& tocheck) const
{
    uint32_t extensionCount{0};
    // Get number of extensions supported
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    // Create list of VkExtensionsProperties using count
    std::vector<VkExtensionProperties> extensions{extensionCount};
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    for (const auto& ext : tocheck)
    {
        bool has_extension = false;
        for (const auto& vkext : extensions)
        {
            if (strcmp(ext, vkext.extensionName) == 0)
            {
                has_extension = true;
                break;
            }
        }
        if (!has_extension)
        {
            std::cout << "Vulkan Extension Unavailable: " << ext << std::endl;
            return false;
        }
    }

    return true;
}

void VulkanRenderer::getPhysicalDevice()
{
    // Enumerate physical devices
    uint32_t physDeviceCount{0};
    vkEnumeratePhysicalDevices(m_instance, &physDeviceCount, nullptr);

    if (physDeviceCount == 0)
    {
        throw std::runtime_error("No Vulkan physical device available");
    }

    std::vector<VkPhysicalDevice> deviceList{physDeviceCount};
    vkEnumeratePhysicalDevices(m_instance, &physDeviceCount, deviceList.data());

    for (const auto& device : deviceList)
    {
        if (checkPhysicalDevice(device))
        {
            m_device.physical = device;
            break;
        }
    }
}

bool VulkanRenderer::checkPhysicalDevice(const VkPhysicalDevice& device)
{
    /* --- FOR FUTURE USE ---
    // Info about the device itself
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    // Info about what the device can do (geom shader, tess shader, etc.)
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);
    */

    bool deviceSuitable = true;

    deviceSuitable &= getQueueFamilies(device).isValid();
    deviceSuitable &= checkDeviceExtensionSupport(device);

    auto swapchainDetails = getSwapchainDetails(device);
    deviceSuitable &= (!swapchainDetails.formats.empty()) && (!swapchainDetails.presentModes.empty());

    return deviceSuitable;
}

bool VulkanRenderer::checkDeviceExtensionSupport(const VkPhysicalDevice& dev)
{
    uint32_t extCount{0};
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);

    if (extCount == 0) return false;

    std::vector<VkExtensionProperties> extensionProperties{extCount};
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, extensionProperties.data());

    for (const auto& ext : DEVICE_EXTENSIONS)
    {
        bool has_extension = false;
        for (const auto& vkext : extensionProperties)
        {
            if (strcmp(ext, vkext.extensionName) == 0)
            {
                has_extension = true;
                break;
            }
        }
        if (!has_extension)
        {
            std::cout << "Vulkan Extension Unavailable: " << ext << std::endl;
            return false;
        }
    }

    return true;
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(const VkPhysicalDevice& dev)
{
    QueueFamilyIndices indicies;

    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyList{queueFamilyCount};
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilyList.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilyList)
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indicies.graphicsFamily = i;
        }

        VkBool32 presentationSupport{VK_FALSE};
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_surface.surface, &presentationSupport);
        if (queueFamily.queueCount > 0 && presentationSupport)
        {
            indicies.presentationFamily = i;
        }

        if (indicies.isValid()) break;

        i++;
    }

    return indicies;
}

void VulkanRenderer::createLogicalDevice()
{
    QueueFamilyIndices indices = getQueueFamilies(m_device.physical);

    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    // If the gfx and presentation family are the same then we only end up with one queue index
    std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

    float priority{1.0f};
    for (auto index : queueFamilyIndices)
    {
        VkDeviceQueueCreateInfo queueInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = static_cast<uint32_t>(index),
            .queueCount = 1,
            .pQueuePriorities = &priority // pQueuePriorities takes an array with length = queueCount
        };

        queueInfos.push_back(queueInfo);
    }

    // Get device features
    VkPhysicalDeviceFeatures devFeatures = {};

    VkDeviceCreateInfo devInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size()),
        .pQueueCreateInfos = queueInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size()), // the device doesn't care about glfw extensions so this is just 0
        .ppEnabledExtensionNames = DEVICE_EXTENSIONS.data(),
        .pEnabledFeatures = &devFeatures
    };

    assert(vkCreateDevice);
    VkResult result = vkCreateDevice(m_device.physical, &devInfo, nullptr, &m_device.logical);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create Vulkan Logic Device");
    }

    // Queues are created when the logical device is created
    // So we just neet to fetch them
    vkGetDeviceQueue(m_device.logical, indices.graphicsFamily, 0, &m_gfxQueue);
    vkGetDeviceQueue(m_device.logical, indices.presentationFamily, 0, &m_presentQueue);
}

SwapchainDetails VulkanRenderer::getSwapchainDetails(const VkPhysicalDevice& dev)
{
    SwapchainDetails details;

    // Surface Capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, m_surface.surface, &details.surfaceCapabilities);

    // Surface Formats
    uint32_t numFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface.surface, &numFormats, nullptr);
    if (numFormats > 0)
    {
        details.formats.resize(numFormats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface.surface, &numFormats, details.formats.data());
    }

    // Presentation Modes
    uint32_t numModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, m_surface.surface, &numModes, nullptr);
    if (numModes > 0)
    {
        details.presentModes.resize(numModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, m_surface.surface, &numModes, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR VulkanRenderer::getBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    // If the format is undefined then every format is available
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const auto& format : formats)
    {
        if (
            (format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    return formats[0]; // None of our desired formats are available so just return something
}

VkPresentModeKHR VulkanRenderer::getBestPresentMode(const std::vector<VkPresentModeKHR>& modes)
{
    for (const auto& mode : modes)
    {
        // Mailbox is prefered mode
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return mode;
        }
    }

    // FIFO has to be available accoding to the Vulkan spec
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::getBestSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else // if the extent is at UINT32_MAX then the value can vary and we have to set it manually
    {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        width = std::clamp(
            static_cast<uint32_t>(width),
            capabilities.maxImageExtent.width,
            capabilities.maxImageExtent.width
        );
        height = std::clamp(
            static_cast<uint32_t>(height),
            capabilities.maxImageExtent.height,
            capabilities.maxImageExtent.height
        );

        return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    }

}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components =
        {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange =
        {
            .aspectMask = aspectFlags,
            .baseArrayLayer = 0,
            .baseMipLevel = 0,
            .levelCount = 1,
            .layerCount = 1
        }
    };

    VkImageView view;
    if (vkCreateImageView(m_device.logical, &viewInfo, nullptr, &view) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VkImageView");
    }
    return view;
}

void VulkanRenderer::createSwapChain()
{
    auto swapchainDetails = getSwapchainDetails(m_device.physical);

    // Steps
    // 1. Choose best surface format
    auto format = getBestSurfaceFormat(swapchainDetails.formats);
    m_surface.format = format.format;
    // 2. Choose best presentation mode
    auto mode = getBestPresentMode(swapchainDetails.presentModes);
    // 3. Choose Image extents
    auto extents = getBestSwapchainExtent(swapchainDetails.surfaceCapabilities);
    m_surface.extent = extents;

    // Min Image Count - shoould get 1 more than minimum to allw for triple buffering
    uint32_t imageCount = swapchainDetails.surfaceCapabilities.minImageCount + 1;
    if (
        swapchainDetails.surfaceCapabilities.maxImageCount != 0 &&
        imageCount > swapchainDetails.surfaceCapabilities.maxImageCount
    ) {
        imageCount = swapchainDetails.surfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_surface.surface,
        .minImageCount = imageCount,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extents,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = swapchainDetails.surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE // used for recycling swap chain (e.g. on window resize)
    };

    auto indices = getQueueFamilies(m_device.physical);
    if (indices.graphicsFamily != indices.presentationFamily)
    {
        uint32_t queueIndices[] =
        {
            uint32_t(indices.graphicsFamily),
            uint32_t(indices.presentationFamily)
        };
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueIndices;
    }

    vkCreateSwapchainKHR(m_device.logical, &swapchainInfo, nullptr, &m_swapchain);

    uint32_t numSwapchainImages = 0;
    vkGetSwapchainImagesKHR(m_device.logical, m_swapchain, &numSwapchainImages, nullptr);
    std::vector<VkImage> images(numSwapchainImages);
    vkGetSwapchainImagesKHR(m_device.logical, m_swapchain, &numSwapchainImages, images.data());

    for (auto& img : images)
    {
        m_swapchainImages.push_back({
            img,
            createImageView(
                img,
                m_surface.format,
                VK_IMAGE_ASPECT_COLOR_BIT
            )
        });
    }
}
