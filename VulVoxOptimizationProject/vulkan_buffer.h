#pragma once

class Buffer
{
public:
    Buffer() = default;

    void create(Vulkan_Instance& instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void destroy(VkDevice device);

    VkBuffer buffer;
    VkDeviceMemory device_memory;
};