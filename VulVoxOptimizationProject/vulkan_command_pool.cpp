#include "pch.h"
#include "vulkan_command_pool.h"
namespace vulvox
{
    Vulkan_Command_Pool::Vulkan_Command_Pool(Vulkan_Instance* vulkan_instance, const int frames_in_flight)
        : vulkan_instance(vulkan_instance)
    {
        create_command_pool();
        create_command_buffers(frames_in_flight);
    }

    /// <summary>
    /// Creates the command pool that manages the memory that stores the buffers
    /// </summary>
    void Vulkan_Command_Pool::create_command_pool()
    {
        Queue_Family_Indices queue_family_indices = vulkan_instance->get_queue_families(vulkan_instance->surface);

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.pNext = nullptr;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

        if (vkCreateCommandPool(vulkan_instance->device, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    void Vulkan_Command_Pool::create_command_buffers(int frames_in_flight)
    {
        command_buffers.resize(frames_in_flight);

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.commandPool = command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; //Directly submits to command queue (secondary can be used by primary)
        alloc_info.commandBufferCount = (uint32_t)command_buffers.size(); //Double buffer for concurrent frame processing

        if (vkAllocateCommandBuffers(vulkan_instance->device, &alloc_info, command_buffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
    }

    /// <summary>
    /// Begins a single-use command buffer for short-lived operations.
    /// Allocates a primary command buffer from the specified command pool, 
    /// begins recording with the VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT flag, 
    /// and returns the command buffer.
    /// </summary>
    /// <returns>
    /// The allocated and ready-to-record VkCommandBuffer.
    /// </returns>
    VkCommandBuffer Vulkan_Command_Pool::begin_single_time_commands()
    {
        VkCommandBufferAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandPool = command_pool;
        allocate_info.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(vulkan_instance->device, &allocate_info, &command_buffer);

        //Start recording command buffer
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //We only use this buffer once

        vkBeginCommandBuffer(command_buffer, &begin_info);

        return command_buffer;
    }

    void Vulkan_Command_Pool::end_single_time_commands(VkCommandBuffer command_buffer)
    {
        vkEndCommandBuffer(command_buffer);

        //Execute copy command buffer instantly and wait until transfer is complete
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &command_buffer;

        vkQueueSubmit(vulkan_instance->graphics_queue, 1, &submitInfo, nullptr);
        vkQueueWaitIdle(vulkan_instance->graphics_queue);

        vkFreeCommandBuffers(vulkan_instance->device, command_pool, 1, &command_buffer);
    }

    VkCommandBuffer Vulkan_Command_Pool::reset_command_buffer(int current_frame)
    {
        vkResetCommandBuffer(command_buffers[current_frame], 0); //Remove previous commands and free memory
        return command_buffers[current_frame];
    }

    void Vulkan_Command_Pool::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
    {
        VkCommandBuffer command_buffer = begin_single_time_commands();

        //Record copy
        VkBufferCopy copy_region{};
        copy_region.srcOffset = 0; // Optional
        copy_region.dstOffset = 0; // Optional
        copy_region.size = size;

        vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

        end_single_time_commands(command_buffer);
    }

    void Vulkan_Command_Pool::destroy()
    {
        //Also destroys the command buffers
        vkDestroyCommandPool(vulkan_instance->device, command_pool, nullptr);
    }
}