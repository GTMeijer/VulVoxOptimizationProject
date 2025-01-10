#pragma once

#include <functional>

namespace vulvox
{
    class Vulkan_Engine; //Forward declaration for pimpl

    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        void init(uint32_t width, uint32_t height, float field_of_view, float near_plane, float far_plane);
        void destroy();

        /// <summary>
        /// For a basic (debugging) ui you can init Dear ImGui.
        /// This will start an imgui frame when calling start_draw() and render it when calling end_draw().
        /// Make sure you call the imgui functions within these two calls.
        /// Alternatively you can register a callback function that handles all the ui logic (see set_imgui_callback).
        /// </summary>
        void init_imgui();

        /// <summary>
        /// Disables imgui if initialized, destroys all imgui data.
        /// </summary>
        void disable_imgui();

        void set_dark_theme();
        void set_light_theme();

        /// <summary>
        /// Sets a callback function that will automatically be called at the end of the frame.
        /// Use this callback function to handle your ui logic.
        /// </summary>
        /// <param name="callback"></param>
        void set_imgui_callback(std::function<void()> callback);

        /// <summary>
        /// Checks if a close command was given to the window, indicating the program should shutdown.
        /// </summary>
        /// <returns></returns>
        bool should_close() const;

        void start_draw();
        void end_draw();

        void draw_model(const std::string& model_name, const std::string& texture_name, const glm::mat4& model_matrix);
        void draw_model_with_texture_array(const std::string& model_name, const std::string& texture_array_name, const int texture_index, const glm::mat4& model_matrix);
        void draw_instanced(const std::string& model_name, const std::string& texture_name, const std::vector<glm::mat4>& model_matrices);
        void draw_instanced_with_texture_array(const std::string& model_name, const std::string& texture_array_name, const std::vector<glm::mat4>& model_matrices, const std::vector<uint32_t>& texture_indices);
        void draw_planes(const std::string& texture_array_name, const std::vector<glm::mat4>& model_matrices, const std::vector<uint32_t>& texture_indices, const std::vector<glm::vec4>& min_max_uvs);

        void load_model(const std::string& model_name, const std::filesystem::path& path);
        void load_texture(const std::string& texture_name, const std::filesystem::path& path);
        void load_texture_array(const std::string& texture_name, const std::vector<std::filesystem::path>& paths);

        void unload_model(const std::string& name);
        void unload_texture(const std::string& name);
        void unload_texture_array(const std::string& name);

        void set_model_matrix(const glm::mat4& new_model_matrix);
        void set_view_matrix(const glm::mat4& new_view_matrix);

        void set_field_of_view(float new_field_of_view);
        void set_aspect_ratio(float new_aspect_ratio);
        void set_near_plane(float new_near_plane);
        void set_far_plane(float new_far_plane);

        GLFWwindow* get_window();
        void resize_window(const uint32_t new_width, const uint32_t new_height);
        float get_aspect_ratio() const;

        std::string get_memory_statistics() const;

    private:

        //Uses Unique_Ptr to vulkan_engine to hide implementation details (pimpl pattern)
        std::unique_ptr<Vulkan_Engine> vulkan_engine;
    };

}