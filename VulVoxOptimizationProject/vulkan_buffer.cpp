#include "pch.h"
#include "vulkan_buffer.h"

void Buffer::create(Vulkan_Instance& instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //Only used by graphics queue
    buffer_info.flags = 0;

    if (vkCreateBuffer(instance.device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    //Query the physical device requirements to use the vertex buffer
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(instance.device, buffer, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;

    //The memory has to be writable from the CPU side
    //Also, ensure the mapped memory matches the content of the allocated memory (no flush needed)
    allocate_info.memoryTypeIndex = instance.find_memory_type(memory_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(instance.device, &allocate_info, nullptr, &device_memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }

    //Bind the buffer to the allocated memory
    vkBindBufferMemory(instance.device, buffer, device_memory, 0);
}

void Buffer::destroy(VkDevice device)
{
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, device_memory, nullptr);
}