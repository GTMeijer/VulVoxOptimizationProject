#pragma once

namespace vulvox
{
    class Vulkan_Engine
    {
    public:
        Vulkan_Engine();
        ~Vulkan_Engine();

        void init_window(uint32_t width, uint32_t height);
        void init_vulkan();

        void init(uint32_t width, uint32_t height);

        void init_imgui();
        void disable_imgui();
        ImGui_Context* get_imgui_context() const;

        void destroy();

        GLFWwindow* get_glfw_window_ptr();
        MVP_Handler& get_mvp_handler();

        void resize_window(const uint32_t new_width, const uint32_t new_height);

        void load_model(const std::string& model_name, const std::filesystem::path& path);
        void load_texture(const std::string& texture_name, const std::filesystem::path& path);
        void load_texture_array(const std::string& texture_name, const std::vector<std::filesystem::path>& paths);

        void unload_model(const std::string& name);
        void unload_texture(const std::string& name);
        void unload_texture_array(const std::string& name);

        void start_draw();
        void end_draw();

        void draw_model(const std::string& model_name, const std::string& texture_name, const glm::mat4& model_matrix);
        void draw_model_with_texture_array(const std::string& model_name, const std::string& texture_array_name, const int texture_index, const glm::mat4& model_matrix);
        void draw_instanced(const std::string& model_name, const std::string& texture_name, const std::vector<glm::mat4>& model_matrices);
        void draw_instanced_with_texture_array(const std::string& model_name, const std::string& texture_array_name, const std::vector<glm::mat4>& model_matrices, const std::vector<uint32_t>& texture_indices);
        void draw_planes(const std::string& texture_array_name, const std::vector<glm::mat4>& model_matrices, const std::vector<uint32_t>& texture_indices, const std::vector<glm::vec4>& min_max_uvs);

        bool initialized() const;

        bool framebuffer_resized = false;

        std::string get_memory_statistics() const;

    private:

        void update_uniform_buffer();

        //Swap chain recreation functions
        void recreate_swap_chain();
        void cleanup_swap_chain();

        void create_render_pass();
        void create_graphics_pipeline();
        void create_framebuffers();

        //Depth test setup functions
        void create_depth_resources();

        void create_descriptor_pool();
        void create_mvp_descriptor_set_layout(); //Describes mvp uniform buffers
        void create_texture_descriptor_set_layout(); //Describes uniform image samplers
        void create_descriptor_sets();

        VkDescriptorSet create_texture_descriptor_set(const Image& texture);

        void create_sync_objects();

        void start_record_command_buffer();
        void end_record_command_buffer();

        VkShaderModule create_shader_module(const std::vector<char>& bytecode);

        bool has_stencil_component(VkFormat format) const;

        bool is_initialized = false;

        int frame_count = 0;

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

        VkDescriptorSetLayout mvp_descriptor_set_layout;
        VkDescriptorSetLayout texture_descriptor_set_layout;
        VkPipelineLayout pipeline_layout; //Describes the layout of the 'global' data, e.g. uniform buffers

        //GPU draw state (stages, shaders, rasterization options, depth settings, etc.)
        VkPipeline instance_pipeline;
        VkPipeline instance_tex_array_pipeline;
        VkPipeline vertex_pipeline;
        VkPipeline instance_plane_pipeline;

        ///Stuff that gets send to the shaders



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

        //Manages all the uniform and instance buffers
        Vulkan_Buffer_Manager buffer_manager;

        std::unordered_map<std::string, Model> models;
        std::unordered_map<std::string, Image> textures;
        std::unordered_map<std::string, VkDescriptorSet> texture_descriptor_sets;
        std::unordered_map<std::string, VkDescriptorSet> texture_array_descriptor_sets;
        std::unordered_map<std::string, Image> texture_arrays;

        MVP_Handler mvp_handler;

        //Optional user interface
        std::unique_ptr<ImGui_Context> imgui_context;

        //We don't want to wait for the previous frame to finish while processing the next frame,
        //so we create double the amount of buffers so we can overlap frame processing
        static const int MAX_FRAMES_IN_FLIGHT;
        uint32_t current_frame = 0;
    };


    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
    {
        //Retrieve the object instance the window belongs to
        auto app = reinterpret_cast<Vulkan_Engine*>(glfwGetWindowUserPointer(window));
        app->framebuffer_resized = true;
    }
}