
#ifndef MESH_H_
#define MESH_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "utilities.h"

class Mesh
{
public:
    Mesh() {}
    Mesh(
        VkPhysicalDevice phys,
        VkDevice log,
        VkQueue transferQueue,
        VkCommandPool transferCmdPool,
        std::vector<Vertex>* vertices,
        std::vector<uint32_t>* indices
    );

    int getVertexCount();
    int getIndexCount();
    VkBuffer getVertexBuffer();
    VkBuffer getIndexBuffer();

    void destroyVertexBuffer();
private:
    int m_vertexCount, m_indexCount;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_logicalDevice;
    VkBuffer m_vertexBuffer, m_indexBuffer;
    VkDeviceMemory m_vertexBufferMemory, m_indexBufferMemory;

    void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* vertices);
    void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<uint32_t>* indices);
};

#endif