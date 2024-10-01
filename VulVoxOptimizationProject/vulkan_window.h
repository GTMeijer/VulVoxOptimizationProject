#pragma once

class Vulkan_Window
{
public:

    void init()
    {
        init_window();
        init_vulkan();
        is_initialized = true;
    }

    void run()
    {

        main_loop();
        cleanup();
    }

    bool framebuffer_resized = false;

private:

    bool is_initialized = false;
    int frame_count = 0;

    void init_window();
    void init_vulkan();

    void main_loop();
    void draw_frame();

    void cleanup();
    void cleanup_swap_chain();

    void load_model();

    void update_uniform_buffer(uint32_t current_image);

    //Vulkan and device context creation functions
    void create_surface();

    //Swap chain creation functions
    void create_swap_chain();
    void recreate_swap_chain();

    //Swap chain creation helper functions
    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) const;
    VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) const;
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    void create_image_views();
    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);

    void create_render_pass();
    void create_descriptor_set_layout(); //Describes uniform buffers and image samplers
    void create_graphics_pipeline();
    void create_framebuffers();
    void create_command_pool();

    //Depth test setup functions
    void create_depth_resources(); //Depth buffer resources

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

    void create_texture_image();
    void create_texture_image_view();
    void create_texture_sampler();

    //Image creation help functions
    void create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory);
    void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
    void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);


    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);

    VkShaderModule create_shader_module(const std::vector<char>& bytecode);


    bool has_stencil_component(VkFormat format) const;

    uint32_t width = 800;
    uint32_t height = 600;
    GLFWwindow* window;

    //Vulkan and device contexts
    Vulkan_Instance vulkan_instance;

    //Command pool and the allocated command buffers that store the commands send to the GPU
    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    //Semaphores and fences to synchronize the gpu and host operations
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences; //Fence for draw finish

    VkSurfaceKHR surface;
    
    //Swapchain context and information
    VkSwapchainKHR swap_chain;
    VkFormat swap_chain_image_format;

    std::vector<VkImage> swap_chain_images;
    std::vector<VkImageView> swap_chain_image_views;
    VkExtent2D swap_chain_extent;

    VkRenderPass render_pass; //Stores information about the render images

    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline; //GPU draw state (shaders, rasterization options, depth settings, etc.)
    std::vector<VkFramebuffer> swap_chain_framebuffers; //Target images for renderpass


    //Stuff that gets send to the shaders
    //Vertex and index buffers holding the mesh data
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;

    //Uniform buffers, data available across shaders
    std::vector<VkBuffer> uniform_buffers;
    std::vector<VkDeviceMemory> uniform_buffers_memory;
    std::vector<void*> uniform_buffers_mapped;

    //Descriptor pool and sets, 
    //holds the binding information that connects the shader inputs to the data
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;

    //Image containing the model texture
    VkImage texture_image;
    VkDeviceMemory texture_image_memory;
    VkImageView texture_image_view;
    VkSampler texture_sampler;

    //Image used for depth testing
    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkImageView depth_image_view;

    //We don't want to wait for the previous frame to finish while processing the next frame,
    //so we create double the amount of buffers so we can overlap frame processing
    static const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t current_frame = 0;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;


    //const std::vector<Vertex> vertices =
    //{
    //    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    //    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    //    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    //    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    //    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    //    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    //    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    //    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    //};

    ////16bit for now, >65535 needs 32
    //const std::vector<uint16_t> indices =
    //{
    //    0, 1, 2, 2, 3, 0,
    //    4, 5, 6, 6, 7, 4
    //};
};

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
    //Retrieve the object instance the window belongs to
    auto app = reinterpret_cast<Vulkan_Window*>(glfwGetWindowUserPointer(window));
    app->framebuffer_resized = true;
}