#pragma once

namespace vulvox
{
    class ImGui_Context
    {
    public:
        ImGui_Context(GLFWwindow* glfw_window, vulvox::Vulkan_Instance& vulkan_instance, VkRenderPass render_pass, const int max_swapchain_images);
        ~ImGui_Context();

        void render_ui(VkCommandBuffer current_command_buffer);

        void set_imgui_callback(std::function<void()>& callback);

        void set_dark_theme();
        void set_light_theme();

    private:

        void start_imgui_frame();
        void end_imgui_frame(VkCommandBuffer current_command_buffer);

        std::function<void()> imgui_callback;

        //Allocating a seperate descriptor pool for imgui is just way easier to manage
        VkDescriptorPool descriptor_pool;

        //Device that created this imgui context
        VkDevice device = VK_NULL_HANDLE;

    };
}