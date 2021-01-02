#include "vulkan_renderer.h"

#include <array>
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
        createGraphicsPipeline();
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
    vkDestroyPipeline(m_device.logical, m_gfxpipeline, nullptr);
    vkDestroyPipelineLayout(m_device.logical, m_pipelineLayout, nullptr),
    vkDestroyRenderPass(m_device.logical, m_renderpass, nullptr);
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

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo shaderInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device.logical, &shaderInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module");
    }
    return shaderModule;
}

void VulkanRenderer::createRenderPass()
{
    // Create color attachment
    VkAttachmentDescription colorAttachment =
    {
        .format = m_surface.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,      // roughly equivalent to glClearColor
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,    // store attachment data after rendering
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // Image data layout before render pass starts
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // Image data layout after render pass (optimized for presentation surface)
    };

    // Create attachment reference to color attachment for subpass
    VkAttachmentReference colorAttachmentRef =
    {
        .attachment = 0, // index of attachment in render pass
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // specify layout to use during subpass
    };

    // Define subpasses
    VkSubpassDescription subpassDesc =
    {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
    };

    // Need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 2> subpassDependencies =
    {
        // the first dependency says that we need to convert from
        // undefined to color_optimal some time between our external code
        // and when the color attachment tries to read or write the image
        VkSubpassDependency{
            // Transition must happen after...
            .srcSubpass = VK_SUBPASS_EXTERNAL,                      // external is code external to the pipeline (i.e. our code)
            .srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,   // bottom of pipeline is when image is rendered
            .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,             // any conversion must happen after external code is done reading image memory
            // But happen before ...
            .dstSubpass = 0,                                                                                // convert image before subpass 0
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,                                  // stage has a color attachment
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,    // do conversions before any read or write by color attachment
            .dependencyFlags = 0,
        },
        VkSubpassDependency{
            .srcSubpass = 0,                                                                                // do conversions after subpass 0
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,                                  // stage has color attachment
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,    // any conversion must happen after stage is done reading/writing image

            .dstSubpass = VK_SUBPASS_EXTERNAL,                      // convert image before external code
            .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,   // stage is the end of pipeline
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,             // do conversions before any memory read by external code
            .dependencyFlags = 0,
        }
    };

    VkRenderPassCreateInfo renderPassInfo =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpassDesc,
        .dependencyCount = static_cast<uint32_t>(subpassDependencies.size()),
        .pDependencies = subpassDependencies.data(),
    };

    if (vkCreateRenderPass(m_device.logical, &renderPassInfo, nullptr, &m_renderpass) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create render pass");
    }
}

void VulkanRenderer::createGraphicsPipeline()
{
    auto vertexShader = readFile("shader/vertex.spv");
    auto fragmentShader = readFile("shader/fragment.spv");

    // Build shaders modules to link to pipeline
    auto vertexModule = createShaderModule(vertexShader);
    auto fragmentModule = createShaderModule(fragmentShader);

    createRenderPass();

    // Vertex Stage Creation Info
    VkPipelineShaderStageCreateInfo vertexStageInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertexModule,
        .pName = "main" // name of entry point function in shader
    };

    // Fragment Stage Creation Info
    VkPipelineShaderStageCreateInfo fragmentStageInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragmentModule,
        .pName = "main" // name of entry point function in shader
    };

    VkPipelineShaderStageCreateInfo shaderStages[] =
    {
        vertexStageInfo,
        fragmentStageInfo
    };

    // Create Vertex Input (TODO: Null for now since vertices are hardcoded in shader)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE // enables option to restart primitives when using strips
    };

    // Viewport and Scissor
    // View port defines what section of window the image should map to
    // We just use the whole window
    VkViewport viewport =
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = float(m_surface.extent.width),
        .height = float(m_surface.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    // the scissor is basically a cropping tool so just use the whole window again
    VkRect2D scissor =
    {
        .offset = {0, 0},
        .extent = m_surface.extent
    };

    VkPipelineViewportStateCreateInfo viewportInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    // Dynamic States (FOR REFERENCE)
    /*std::vector<VkDynamicState> dynamicStateEnables[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT, // enables changing viewport in command buffer with vkCmdSetViewport(cmdbuffer, index:0, count:1, &viewport)
        VK_DYNAMIC_STATE_SCISSOR   // ""      ""       scissor  "" ""      ""     ""   vkCmdSetScissor(cmdbuffer, 1, 0, &scissor)
    };

    VkPipelineDynamicStateCreateInfo dynamicInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size()),
        .pDynamicStates = dynamicStateEnables.data()
    };*/

    // Create Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizerInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE, // if true fragments past far plane are clamped to the far plane rather than clipped (need to enable in deviceFeatures in logical device)
        .rasterizerDiscardEnable = VK_FALSE, // if true discard all data instead of converting to fragments
        .polygonMode = VK_POLYGON_MODE_FILL, // controls how to produce fragments from polygons (need GPU feature for options other than fill)
        .lineWidth = 1, // defines how thick lines are when drawn (need wideLines to support values other than 1)
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE // adds small depth amount to depth value of fragments (useful for shadows)
    };

    // Set up multisampling
    VkPipelineMultisampleStateCreateInfo multisampleInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE, // disable multisampling
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT // number of samples to use per fragment
    };

    // Blending
    VkPipelineColorBlendAttachmentState blendAttachmentInfo =
    {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_TRUE,

        // Blending equation: (srcColorBlend * new color) colorBlendOp (dstColorBlendFactor * old color)
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        // So we end up with (new color alpha * new color) + ((1-new color alpha) * old color)

        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD // doesnt really matter since we have dstAlpha set to 0
        // Alpha blending: (1*srcAlpha)+(0*dstAlpha)
    };

    VkPipelineColorBlendStateCreateInfo blendInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &blendAttachmentInfo,
    };

    // Pipeliane Layout (TODO: Apply descriptor set layouts)
    VkPipelineLayoutCreateInfo layoutInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    // Create Pipeline Layout
    if (vkCreatePipelineLayout(m_device.logical, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // TODO: Set up Depth Stencil testing

    // Finally we can create our graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo =
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,                                // we have two shaders so we have two stages
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pViewportState = &viewportInfo,
        .pDynamicState = nullptr,
        .pRasterizationState = &rasterizerInfo,
        .pMultisampleState = &multisampleInfo,
        .pColorBlendState = &blendInfo,
        .pDepthStencilState = nullptr,
        .layout = m_pipelineLayout,                     // Pipeline will use this layout
        .renderPass = m_renderpass,                     // Renderpass description the pipleine will use
        .subpass = 0,                                   // Subpass that will be used by pipeline

        .basePipelineHandle = VK_NULL_HANDLE,           // Create pipeline and use other pipeline as base
        .basePipelineIndex = -1,                        // Can create multiple pipelines at a time and select one to use as a base for the rest6
    };

    if (vkCreateGraphicsPipelines(m_device.logical, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_gfxpipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create Graphics Pipeline");
    }

    // Can destroy here since we are done with shaders
    // Could keep them around if they will be needed in other pipelines
    vkDestroyShaderModule(m_device.logical, fragmentModule, nullptr);
    vkDestroyShaderModule(m_device.logical, vertexModule, nullptr);
}
