#include "pch.h"
#include "imgui_context.h"

namespace vulvox
{
    ImGui_Context::ImGui_Context(GLFWwindow* glfw_window, vulvox::Vulkan_Instance& vulkan_instance, VkRenderPass render_pass, const int max_swapchain_images)
        : device(vulkan_instance.device)
    {
        std::cout << "Initializing imgui.." << std::endl;

        VkDescriptorPoolSize pool_sizes = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 50 };

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 50;
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes = &pool_sizes;

        vkCreateDescriptorPool(vulkan_instance.device, &pool_info, nullptr, &descriptor_pool);

        //Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; //Enable keyboard controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  //Enable gamepad controls

        //Setup imgui vulkan backend
        ImGui_ImplGlfw_InitForVulkan(glfw_window, true); //Second parameter installs glfw callbacks and chain through existing ones
        ImGui_ImplVulkan_InitInfo imgui_vulkan_init_info{};
        imgui_vulkan_init_info.Instance = vulkan_instance.instance;
        imgui_vulkan_init_info.PhysicalDevice = vulkan_instance.physical_device;
        imgui_vulkan_init_info.Device = vulkan_instance.device;
        imgui_vulkan_init_info.Queue = vulkan_instance.graphics_queue;
        imgui_vulkan_init_info.DescriptorPool = descriptor_pool;
        imgui_vulkan_init_info.RenderPass = render_pass;
        imgui_vulkan_init_info.Subpass = 0;
        imgui_vulkan_init_info.MinImageCount = 2;
        imgui_vulkan_init_info.ImageCount = max_swapchain_images;
        imgui_vulkan_init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        imgui_vulkan_init_info.CheckVkResultFn = check_vk_result;

        ImGui_ImplVulkan_Init(&imgui_vulkan_init_info);
    }

    ImGui_Context::~ImGui_Context()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
    }

    void ImGui_Context::start_imgui_frame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGui_Context::render_ui(VkCommandBuffer current_command_buffer)
    {
        start_imgui_frame();

        if (imgui_callback) {
            imgui_callback();
        }

        end_imgui_frame(current_command_buffer);
    }

    void ImGui_Context::end_imgui_frame(VkCommandBuffer current_command_buffer)
    {
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), current_command_buffer);
    }

    void ImGui_Context::set_imgui_callback(std::function<void()>& callback)
    {
        imgui_callback = callback;
    }

    void ImGui_Context::set_dark_theme()
    {
        ImGui::StyleColorsDark();
    }

    void ImGui_Context::set_light_theme()
    {
        ImGui::StyleColorsLight();
    }
}