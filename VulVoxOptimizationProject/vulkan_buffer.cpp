#include "pch.h"
#include "vulkan_buffer.h"

void Buffer::create(Vulkan_Instance& instance, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags alloc_flags)
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //Only used by graphics queue
    buffer_info.flags = 0;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = alloc_flags;

    ////If the memory is used for something like a staging buffer give the allocator a hint to make writing from host smoother
    //if (usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
    //{
    //    alloc_info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    //}

    if (vmaCreateBuffer(instance.allocator, &buffer_info, &alloc_info, &buffer, &allocation, &allocation_info) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer!");
    }
}

void Buffer::destroy(VmaAllocator allocator)
{
    vmaDestroyBuffer(allocator, buffer, allocation);
}