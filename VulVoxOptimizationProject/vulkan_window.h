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

    bool framebuffer_resized = false;

private:

    /// <summary>
    /// Struct containing the indices of the command queues we require
    /// For this program we need a graphics and present capable queue.
    /// The families supporting these queues are stored in this class
    /// </summary>
    struct Queue_Family_Indices
    {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;

        /// <summary>
        /// Check if all queue families are filled.
        /// </summary>
        /// <returns>Returns true if all queue families indices are initialized.</returns>
        bool is_complete() const
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
    void draw_frame();

    void update_uniform_buffer(uint32_t current_image);

    void cleanup();
    void cleanup_swap_chain();

    void create_instance();
    void create_surface();
    void create_logical_device();
    void create_swap_chain();
    void create_image_views();
    void create_render_pass();
    void create_descriptor_set_layout(); //Describes uniform buffers
    void create_graphics_pipeline();
    void create_framebuffers();
    void create_command_pool();

    void create_vertex_buffer();
    void create_index_buffer();
    void create_uniform_buffers();
    void create_descriptor_pool();
    void create_descriptor_sets();

    void create_command_buffer();
    void create_sync_objects();


    void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
    void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

    void recreate_swap_chain();

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

    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);

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
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    std::vector<VkFramebuffer> swap_chain_framebuffers;
    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;

    std::vector<VkBuffer> uniform_buffers;
    std::vector<VkDeviceMemory> uniform_buffers_memory;
    std::vector<void*> uniform_buffers_mapped;
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;

    //Semaphores and fences to synchronize the gpu and host operations
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences; //Fence for draw finish

    //Required device extensions
    const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };

    //We don't want to wait for the previous frame to finish while processing the next frame,
    //so we create double the amount of buffers so we can overlap frame processing
    static const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t current_frame = 0;

    const std::vector<Vertex> vertices =
    {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    //16bit for now, >65535 needs 32
    const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
};

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
    //Retrieve the object instance the window belongs to
    auto app = reinterpret_cast<Vulkan_Window*>(glfwGetWindowUserPointer(window));
    app->framebuffer_resized = true;
}