
#define GLFW_INCLUDE_VULKAN // This tells GLFW to #include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define FORCE_DEPTH_ZERO_TO_ONE // specific to Vulkan, OpenGL uses depth as [-1,1] but vulkan is [0,1]
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <cstdint>
#include <iostream>

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow (
        800,
        600,
        "Test Window",
        nullptr,
        nullptr
    );

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::cout << "Extensions: " << extensionCount << std::endl;

    glm::mat4 testMat(1.0f);
    glm::vec4 testVec(1.0f);

    auto testResult = testMat * testVec;

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}