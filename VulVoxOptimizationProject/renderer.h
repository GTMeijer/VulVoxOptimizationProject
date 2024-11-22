#pragma once

//Use Unique_Ptr vulkan_engine

namespace vulvox
{
    class Vulkan_Engine; //Forward declaration for pimpl

    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        void init(uint32_t width, uint32_t height);
        void destroy();

        /// <summary>
        /// Checks if a close command was given to the window, indicating the program should shutdown.
        /// </summary>
        /// <returns></returns>
        bool should_close() const;

        void start_draw();
        void end_draw();

        void draw_model(const std::string& model_name, const std::string& texture_name, const glm::mat4& model_matrix);
        void draw_model_with_texture_array(const std::string& model_name, const std::string& texture_array_name, const int texture_index, const glm::mat4& model_matrix);
        void draw_instanced(const std::string& model_name, const std::string& texture_name, const std::vector<Instance_Data>& instance_data);
        void draw_instanced_with_texture_array(const std::string& model_name, const std::string& texture_array_name, const std::vector<Instance_Data>& instance_data, const std::vector<uint32_t>& texture_indices);

        void load_model(const std::string& model_name, const std::filesystem::path& path);
        void load_texture(const std::string& texture_name, const std::filesystem::path& path);
        void load_texture_array(const std::string& texture_name, const std::vector<std::filesystem::path>& paths);

        void unload_model(const std::string& name);
        void unload_texture(const std::string& name);
        void unload_texture_array(const std::string& name);

        void set_camera(const MVP& camera_matrix);

        GLFWwindow* get_window();
        void resize_window(const uint32_t new_width, const uint32_t new_height);
        float get_aspect_ratio() const;

    private:

        std::unique_ptr<Vulkan_Engine> vulkan_engine;
    };

}