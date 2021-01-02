
#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "utilities.h"
#include "mesh.h"

class VulkanRenderer
{
public:
    VulkanRenderer() {}
    virtual ~VulkanRenderer() {}

    int init(GLFWwindow* wnd);
    void draw();
    void destroy();

private:
    GLFWwindow* m_window;

    VkInstance m_instance;
    int m_currentFrame = 0;

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
    std::vector<VkFramebuffer> m_framebuffers;

    VkPipelineLayout m_pipelineLayout;

    VkRenderPass m_renderpass;

    VkPipeline m_gfxpipeline;

    VkCommandPool m_gfxCommandPool;
    std::vector<VkCommandBuffer> m_commandBuffers; // 1 to 1 with swapchainImages and framebuffers

    std::vector<VkSemaphore> m_imageAvailable, m_renderFinished;
    std::vector<VkFence> m_drawFences;

    std::vector<Mesh> m_meshes;

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

    void createFramebuffers();

    void createCommandPool();
    void allocateCommandBuffers();
    void recordCommands();

    void createSynchronization();
};

#endif