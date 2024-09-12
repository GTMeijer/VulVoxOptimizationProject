#pragma once


class Vulkan_Window
{
public:

    void run()
    {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

private:

    /// <summary>
    /// Struct containing the indices of the command queues
    /// </summary>
    struct Queue_Family_Indices
    {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;

        /// <summary>
        /// Check if all queue families are filled.
        /// </summary>
        /// <returns>Returns true if all queue families indices are initialized.</returns>
        bool is_complete()
        {
            return graphics_family.has_value() && present_family.has_value();
        }
    };

    struct Swap_Chain_Support_Details
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    void init_window();
    void init_vulkan();
    void main_loop();
    void cleanup();

    void create_instance();
    void create_surface();
    void create_logical_device();
    void create_swap_chain();
    void create_image_views();
    void create_render_pass();
    void create_graphics_pipeline();
    void create_framebuffers();
    void create_command_pool();
    void create_command_buffer();
    
    void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);

    void pick_physical_device();

    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) const;
    VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) const;
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    std::string get_physical_device_name(const VkPhysicalDevice& device) const;
    std::string get_physical_device_type(const VkPhysicalDevice& device) const;

    int rate_physical_device(const VkPhysicalDevice& device) const;
    bool check_glfw_extension_support() const;
    bool check_device_extension_support(const VkPhysicalDevice& device) const;
    bool check_validation_layer_support() const;
    Swap_Chain_Support_Details query_swap_chain_support(const VkPhysicalDevice& device) const;


    Vulkan_Window::Queue_Family_Indices find_queue_families(const VkPhysicalDevice& device) const;

    VkShaderModule create_shader_module(const std::vector<char>& bytecode);

    uint32_t width = 800;
    uint32_t height = 600;
    GLFWwindow* window;

    VkInstance instance;
    VkPhysicalDevice physical_device = nullptr;
    VkDevice device;

    VkQueue graphics_queue;
    VkQueue present_queue;

    VkSurfaceKHR surface;
    VkSwapchainKHR swap_chain;
    std::vector<VkImage> swap_chain_images;
    std::vector<VkImageView> swap_chain_image_views;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    std::vector<VkFramebuffer> swap_chain_framebuffers;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    //Required device extensions
    const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
};

