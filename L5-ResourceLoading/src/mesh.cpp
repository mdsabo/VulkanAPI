#include "mesh.h"

Mesh::Mesh(VkPhysicalDevice phys, VkDevice log, VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices):
m_physicalDevice(phys), m_logicalDevice(log)
{
    m_vertexCount = vertices->size();
    m_indexCount = indices->size();
    createVertexBuffer(transferQueue, transferCmdPool, vertices);
    createIndexBuffer(transferQueue, transferCmdPool, indices);
}

int Mesh::getVertexCount()
{
    return m_vertexCount;
}
int Mesh::getIndexCount()
{
    return m_indexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
    return m_vertexBuffer;
}

VkBuffer Mesh::getIndexBuffer()
{
    return m_indexBuffer;
}

void Mesh::destroyVertexBuffer()
{
    vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);
    vkFreeMemory(m_logicalDevice, m_vertexBufferMemory, nullptr);
    vkDestroyBuffer(m_logicalDevice, m_indexBuffer, nullptr);
    vkFreeMemory(m_logicalDevice, m_indexBufferMemory, nullptr);
}

void Mesh::createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* vertices)
{
    VkDeviceSize bufferSize = sizeof(Vertex)*vertices->size();

    // Create Staging SRC Buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        m_physicalDevice,
        m_logicalDevice,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer,
        &stagingBufferMemory
    );

    void* data;
    vkMapMemory(
        m_logicalDevice,
        stagingBufferMemory,
        0,
        bufferSize,
        0,
        &data
    );
    memcpy(data, (void*)(vertices->data()), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_logicalDevice, stagingBufferMemory);

    // Create Vertex Buffer (Staging DST Buffer)
    createBuffer(
        m_physicalDevice,
        m_logicalDevice,
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &m_vertexBuffer,
        &m_vertexBufferMemory
    );

    copyBuffer(m_logicalDevice, transferQueue, transferCmdPool, stagingBuffer, m_vertexBuffer, bufferSize);

    vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<uint32_t>* indices)
{
    VkDeviceSize bufferSize = sizeof(uint32_t)*indices->size();

    // Create Staging SRC Buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        m_physicalDevice,
        m_logicalDevice,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer,
        &stagingBufferMemory
    );

    void* data;
    vkMapMemory(
        m_logicalDevice,
        stagingBufferMemory,
        0,
        bufferSize,
        0,
        &data
    );
    memcpy(data, (void*)(indices->data()), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_logicalDevice, stagingBufferMemory);

    // Create Index Buffer (Staging DST Buffer)
    createBuffer(
        m_physicalDevice,
        m_logicalDevice,
        bufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &m_indexBuffer,
        &m_indexBufferMemory
    );

    copyBuffer(m_logicalDevice, transferQueue, transferCmdPool, stagingBuffer, m_indexBuffer, bufferSize);

    vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
}
