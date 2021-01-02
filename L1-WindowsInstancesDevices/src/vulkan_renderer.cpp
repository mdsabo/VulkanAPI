#include "vulkan_renderer.h"

#include <iostream>
#include <stdlib.h>

int VulkanRenderer::init(GLFWwindow* wnd)
{
    m_window = wnd;

    try
    {
        createInstance();
        getPhysicalDevice();
        createLogicalDevice();
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
    vkDestroyDevice(m_device.logical, nullptr);
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

    const char* layers[] = {
        //"VK_LAYER_LUNARG_api_dump",
        "VK_LAYER_KHRONOS_validation"
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = layers
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

   QueueFamilyIndicies indices = getQueueFamilies(device);

    return indices.isValid();
}

QueueFamilyIndicies VulkanRenderer::getQueueFamilies(const VkPhysicalDevice& dev)
{
    QueueFamilyIndicies indicies;

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
            if (indicies.isValid()) break;
        }

        i++;
    }

    return indicies;
}

void VulkanRenderer::createLogicalDevice()
{
    QueueFamilyIndicies indices = getQueueFamilies(m_device.physical);

    float priority{1.0f};
    VkDeviceQueueCreateInfo queueInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = static_cast<uint32_t>(indices.graphicsFamily),
        .queueCount = 1,
        .pQueuePriorities = &priority // pQueuePriorities takes an array with length = queueCount
    };

    // Get device features
    VkPhysicalDeviceFeatures devFeatures = {};

    const char* extensionNames[] = { "VK_KHR_portability_subset" }; // From validation layers

    VkDeviceCreateInfo devInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = 1, // the device doesn't care about glfw extensions so this is just 0
        .ppEnabledExtensionNames = extensionNames,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
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
}
