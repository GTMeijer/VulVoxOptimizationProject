#pragma once

class Buffer
{
public:
    Buffer() = default;

    void create(Vulkan_Instance& instance, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags alloc_flags);
    void destroy(VmaAllocator allocator);

    VkBuffer buffer{ VK_NULL_HANDLE };
    VmaAllocation allocation;
    VmaAllocationInfo allocation_info;
};

class InstanceBuffer
{
public:
    Buffer buffer;
    size_t size = 0;
    VkDescriptorBufferInfo descriptor{ VK_NULL_HANDLE };
};