#pragma once

class Buffer
{
public:
    Buffer() = default;

    void create(Vulkan_Instance& instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void destroy(VkDevice device);

    VkBuffer buffer{ VK_NULL_HANDLE };
    VkDeviceMemory device_memory{ VK_NULL_HANDLE };
};

class InstanceBuffer
{
public:
    Buffer buffer;
    size_t size = 0;
    VkDescriptorBufferInfo descriptor{ VK_NULL_HANDLE };
};