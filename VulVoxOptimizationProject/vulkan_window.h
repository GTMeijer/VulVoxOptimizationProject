#pragma once


namespace vulvox
{
    class Vulkan_Renderer
    {
    public:

        Vulkan_Renderer() : swap_chain(&vulkan_instance)
        {

        }

        void init(uint32_t width, uint32_t height)
        {
            init_window(width, height);
            init_vulkan();
            is_initialized = true;
        }

        void cleanup();

        void start_draw();
        void end_draw();

        void draw_model(const std::string& model_name, const std::string& texture_name, const glm::mat4& model_matrix);
        void draw_model(const std::string model_name, const std::string& texture_name, const int texture_index, const glm::mat4& model_matrix);
        void draw_instanced(const std::string model_name, const std::string& texture_name, const std::vector<glm::mat4>& model_matrices);
        void draw_instanced(const std::string model_name, const std::string& texture_name, const std::vector<int>& texture_indices, const std::vector<glm::mat4>& model_matrices);

        /// <summary>
        /// Checks if a close command was given to the window, indicating the program should shutdown.
        /// </summary>
        /// <returns></returns>
        bool should_close() const;

        void set_camera(const MVP& camera_matrix);

        GLFWwindow* get_window();
        void resize_window(const uint32_t width, const uint32_t height);
        float get_aspect_ratio() const;

        bool framebuffer_resized = false;

        void load_model(const std::string& name, const std::filesystem::path& path);
        void load_texture(const std::string& name, const std::filesystem::path& path);
        void load_texture_array(const std::string& name, const std::filesystem::path& path);

        void unload_model(const std::string& name);
        void unload_texture(const std::string& name);
        void unload_texture_array(const std::string& name);


    private:

        bool is_initialized = false;
        int frame_count = 0;

        void init_window(uint32_t width, uint32_t height);
        void init_vulkan();

        void update_uniform_buffer(uint32_t current_image);

        //Swap chain recreation functions
        void recreate_swap_chain();
        void cleanup_swap_chain();

        void create_render_pass();
        void create_graphics_pipeline();
        void create_framebuffers();

        //Depth test setup functions
        void create_depth_resources(); //Depth buffer resources

        void create_instance_buffers();
        void create_uniform_buffers();

        void create_descriptor_pool();
        void create_descriptor_set_layout(); //Describes uniform buffers and image samplers
        void create_descriptor_sets();

        void create_sync_objects();

        void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);

        Image create_texture_image(const std::filesystem::path& texture_path);

        //Image creation help functions
        void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

        VkShaderModule create_shader_module(const std::vector<char>& bytecode);

        bool has_stencil_component(VkFormat format) const;

        uint32_t width = 800;
        uint32_t height = 600;
        GLFWwindow* window;

        //Vulkan and device contexts
        Vulkan_Instance vulkan_instance;

        Vulkan_Swap_Chain swap_chain;

        //Command pool and the allocated command buffers that store the commands send to the GPU
        Vulkan_Command_Pool command_pool;

        //Draw state
        VkCommandBuffer current_command_buffer;
        uint32_t current_image_index;

        //Semaphores and fences to synchronize the gpu and host operations
        std::vector<VkSemaphore> image_available_semaphores;
        std::vector<VkSemaphore> render_finished_semaphores;
        std::vector<VkFence> in_flight_fences; //Fence for draw finish

        VkRenderPass render_pass; //Stores how the render images are handeled

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

        glm::mat4 single_konata_matrix = glm::mat4(1.0f);

        struct Instanced_Model
        {
            Image texture;
        };

        Instanced_Model instance_konata;

        std::vector<Instance_Data> konata_instances_data;

        Buffer instance_data_buffer;

        std::unordered_map<std::string, Model> models;
        std::unordered_map<std::string, Image> textures;
        std::unordered_map<std::string, Image> texture_arrays;

        MVP model_view_projection;

        //We don't want to wait for the previous frame to finish while processing the next frame,
        //so we create double the amount of buffers so we can overlap frame processing
        static const int MAX_FRAMES_IN_FLIGHT = 2;
        uint32_t current_frame = 0;
    };

    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
    {
        //Retrieve the object instance the window belongs to
        auto app = reinterpret_cast<Vulkan_Renderer*>(glfwGetWindowUserPointer(window));
        app->framebuffer_resized = true;
    }
}