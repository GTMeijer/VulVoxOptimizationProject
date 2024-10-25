#pragma once

namespace vulvox
{
    class Vulkan_Command_Pool
    {
    public:
        Vulkan_Command_Pool() = default;
        Vulkan_Command_Pool(Vulkan_Instance* vulkan_instance, const int frames_in_flight);

        void create_command_pool();
        void create_command_buffers(int frames_in_flight);

        VkCommandBuffer begin_single_time_commands();
        void end_single_time_commands(VkCommandBuffer command_buffer);

        VkCommandBuffer reset_command_buffer(int current_frame);

        /// <summary>
        /// Creates a temporary command buffer that transfers data between scr and dst buffers, executed immediately
        /// </summary>
        void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);


        void destroy();

    private:

        Vulkan_Instance* vulkan_instance;

        VkCommandPool command_pool;
        std::vector<VkCommandBuffer> command_buffers;
    };
}