
#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include <vulkan/vulkan.h>
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
    VkQueue m_gfxQueue;

    void createInstance();
    bool checkInstanceExtensionSupport(const std::vector<const char*>& tocheck) const;

    void getPhysicalDevice();
    bool checkPhysicalDevice(const VkPhysicalDevice& device);

    QueueFamilyIndicies getQueueFamilies(const VkPhysicalDevice& dev);

    void createLogicalDevice();
};

#endif