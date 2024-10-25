#pragma once

namespace vulvox
{
    class Buffer
    {
    public:
        Buffer() = default;

        void create(Vulkan_Instance& instance, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags alloc_flags);
        void destroy(VmaAllocator allocator);

        VkBuffer buffer{ VK_NULL_HANDLE };
        VmaAllocation allocation{ VK_NULL_HANDLE };
        VmaAllocationInfo allocation_info;
    };
}