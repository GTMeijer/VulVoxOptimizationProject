#pragma once

class Vulkan_Window
{
public:

    Vulkan_Window() : swap_chain(&vulkan_instance)
    {

    }

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

    void update_uniform_buffer(uint32_t current_image);

    void create_surface();

    //Swap chain recreation functions
    void recreate_swap_chain();
    void cleanup_swap_chain();

    void create_render_pass();
    void create_graphics_pipeline();
    void create_framebuffers();
    void create_command_pool();

    //Depth test setup functions
    void create_depth_resources(); //Depth buffer resources

    void create_vertex_buffer();
    void create_index_buffer();
    void create_instance_buffers();
    void create_uniform_buffers();

    void create_descriptor_pool();
    void create_descriptor_set_layout(); //Describes uniform buffers and image samplers
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

    VkSurfaceKHR surface;
    Vulkan_Swap_Chain swap_chain;

    //Command pool and the allocated command buffers that store the commands send to the GPU
    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    //Semaphores and fences to synchronize the gpu and host operations
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences; //Fence for draw finish

    VkRenderPass render_pass; //Stores information about the render images

    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout; //Describes the layout of the 'global' data, e.g. uniform buffers

    //GPU draw state (stages, shaders, rasterization options, depth settings, etc.)
    VkPipeline instance_pipeline;
    VkPipeline vertex_pipeline;

    ///Stuff that gets send to the shaders


    //Uniform buffers, data available across shaders
    std::vector<Buffer> uniform_buffers;
    std::vector<void*> uniform_buffers_mapped;

    //Descriptor pool and sets, 
    //holds the binding information that connects the shader inputs to the data
    VkDescriptorPool descriptor_pool;

    struct Descriptor_Sets
    {
        std::vector<VkDescriptorSet> tri_descriptor_set;
        std::vector<VkDescriptorSet> instance_descriptor_set;
    };

    Descriptor_Sets descriptor_sets;

    //Image used for depth testing
    Image depth_image;

    //Texture and mesh for Konata model
    Image texture_image;
    Model konata_model;

    //Vertex and index buffers holding the mesh data
    Buffer vertex_buffer;
    Buffer index_buffer;

    struct Instanced_Model
    {
        Image texture;
        Model model;
    };

    Instanced_Model instance_konata;

    std::vector<Instance_Data> konata_instances_data;

    Buffer instance_vertex_buffer;
    Buffer instance_index_buffer;
    Buffer instance_data_buffer;



    //We don't want to wait for the previous frame to finish while processing the next frame,
    //so we create double the amount of buffers so we can overlap frame processing
    static const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t current_frame = 0;
};

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
    //Retrieve the object instance the window belongs to
    auto app = reinterpret_cast<Vulkan_Window*>(glfwGetWindowUserPointer(window));
    app->framebuffer_resized = true;
}