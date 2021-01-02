
#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "utilities.h"

class VulkanRenderer
{
public:
    VulkanRenderer() {}
    virtual ~VulkanRenderer() {}

    int init(GLFWwindow* wnd);
    void destroy();

private:
    GLFWwindow* m_window;

    VkInstance m_instance;

    struct
    {
        VkPhysicalDevice physical;
        VkDevice logical;
    } m_device;

    VkQueue m_gfxQueue, m_presentQueue;

    struct
    {
        VkSurfaceKHR surface;
        VkFormat format;
        VkExtent2D extent;
    } m_surface;

    VkSwapchainKHR m_swapchain;
    std::vector<SwapchainImage> m_swapchainImages;

    VkPipelineLayout m_pipelineLayout;

    VkRenderPass m_renderpass;

    VkPipeline m_gfxpipeline;

    void createInstance();
    bool checkInstanceExtensionSupport(const std::vector<const char*>& tocheck) const;

    void createSurface();

    void getPhysicalDevice();
    bool checkPhysicalDevice(const VkPhysicalDevice& device);
    bool checkDeviceExtensionSupport(const VkPhysicalDevice& dev);

    QueueFamilyIndices getQueueFamilies(const VkPhysicalDevice& dev);

    void createLogicalDevice();

    SwapchainDetails getSwapchainDetails(const VkPhysicalDevice& dev);
    VkSurfaceFormatKHR getBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR getBestPresentMode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D getBestSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void createSwapChain();

    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createRenderPass();
    void createGraphicsPipeline();
};

#endif