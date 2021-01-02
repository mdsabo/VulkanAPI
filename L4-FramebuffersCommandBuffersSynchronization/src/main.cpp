
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <thread>

#include "vulkan_renderer.h"

GLFWwindow* initWindow(
    const std::string& name = "Window",
    const unsigned int width = 800,
    const unsigned int height = 600
);

int main()
{
    GLFWwindow* window = initWindow();

    VulkanRenderer vkrender = VulkanRenderer();
    if (vkrender.init(window) == EXIT_FAILURE) return EXIT_FAILURE;

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        vkrender.draw();

        // Needed to prevent simultaneous use of command buffers
        // Seems like image isn't being locked until some time after it is acquired
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }

    vkrender.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

GLFWwindow* initWindow(const std::string& name, const unsigned int width, const unsigned int height)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);


    return glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
}