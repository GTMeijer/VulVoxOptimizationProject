#pragma once

namespace vulvox
{
    class Buffer
    {
    public:
        Buffer() = default;

        void create(Vulkan_Instance& instance, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags alloc_flags);
        void destroy(VmaAllocator allocator);

        void recreate(Vulkan_Instance& instance, VkDeviceSize new_size);

        VkDeviceSize size;
        VkBufferUsageFlags usage_flags;
        VmaAllocationCreateFlags alloc_flags;

        VkBuffer buffer{ VK_NULL_HANDLE };
        VmaAllocation allocation{ VK_NULL_HANDLE };
        VmaAllocationInfo allocation_info;
    };
}