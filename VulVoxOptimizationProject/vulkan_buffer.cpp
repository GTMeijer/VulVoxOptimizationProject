#include "pch.h"
#include "vulkan_buffer.h"

namespace vulvox
{
    void Buffer::create(Vulkan_Instance& instance, VkDeviceSize buffer_size, VkBufferUsageFlags usage, VmaAllocationCreateFlags buffer_alloc_flags)
    {
        this->size = buffer_size;
        this->usage_flags = usage;
        this->alloc_flags = buffer_alloc_flags;

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = buffer_size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //Only used by graphics queue
        buffer_info.flags = 0;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = alloc_flags;

        if (vmaCreateBuffer(instance.allocator, &buffer_info, &alloc_info, &buffer, &allocation, &allocation_info) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create buffer!");
        }
    }

    void Buffer::destroy(VmaAllocator allocator)
    {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }

    /// <summary>
    /// Destroy and recreate the buffer with given size.
    /// </summary>
    /// <param name="new_size">The new buffer size in bytes.</param>
    void Buffer::recreate(Vulkan_Instance& instance, VkDeviceSize new_size)
    {
        destroy(instance.allocator);

        create(instance, new_size, usage_flags, alloc_flags);

    }
}