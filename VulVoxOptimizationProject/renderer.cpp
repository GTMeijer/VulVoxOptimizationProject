#include "pch.h"
#include "renderer.h"

namespace vulvox
{
    Renderer::Renderer()
    {
        vulkan_engine = std::make_unique<Vulkan_Engine>();
    }

    Renderer::~Renderer() = default;

    void Renderer::init(uint32_t width, uint32_t height, float field_of_view, float near_plane, float far_plane)
    {
        vulkan_engine->init(width, height);
        vulkan_engine->get_mvp_handler().set_field_of_view(field_of_view);
        vulkan_engine->get_mvp_handler().set_near_plane(near_plane);
        vulkan_engine->get_mvp_handler().set_far_plane(far_plane);
    }

    void Renderer::destroy()
    {
        vulkan_engine->destroy();
    }

    void Renderer::init_imgui()
    {
        if (!vulkan_engine->initialized())
        {
            std::cout << "Initialize the renderer before initializing imgui!" << std::endl;
            return;
        }

        vulkan_engine->init_imgui();
    }

    void Renderer::disable_imgui()
    {
        vulkan_engine->disable_imgui();
    }

    void Renderer::set_dark_theme()
    {
        auto imgui_context = vulkan_engine->get_imgui_context();
        if (imgui_context)
        {
            imgui_context->set_dark_theme();
        }
    }

    void Renderer::set_light_theme()
    {
        auto imgui_context = vulkan_engine->get_imgui_context();
        if (imgui_context)
        {
            imgui_context->set_light_theme();
        }
    }

    void Renderer::set_imgui_callback(std::function<void()> callback)
    {
        auto imgui_context = vulkan_engine->get_imgui_context();
        if (imgui_context)
        {
            imgui_context->set_imgui_callback(callback);
        }
    }

    bool Renderer::should_close() const
    {
        return glfwWindowShouldClose(vulkan_engine->get_glfw_window_ptr());
    }

    void Renderer::start_draw()
    {
        vulkan_engine->start_draw();
    }

    void Renderer::end_draw()
    {
        vulkan_engine->end_draw();
    }

    void Renderer::draw_model(const std::string& model_name, const std::string& texture_name, const glm::mat4& model_matrix)
    {
        vulkan_engine->draw_model(model_name, texture_name, model_matrix);
    }

    void Renderer::draw_model_with_texture_array(const std::string& model_name, const std::string& texture_array_name, const int texture_index, const glm::mat4& model_matrix)
    {
        vulkan_engine->draw_model_with_texture_array(model_name, texture_array_name, texture_index, model_matrix);
    }

    void Renderer::draw_instanced(const std::string& model_name, const std::string& texture_name, const std::vector<glm::mat4>& model_matrices)
    {
        vulkan_engine->draw_instanced(model_name, texture_name, model_matrices);
    }

    void Renderer::draw_instanced_with_texture_array(const std::string& model_name, const std::string& texture_array_name, const std::vector<glm::mat4>& model_matrices, const std::vector<uint32_t>& texture_indices)
    {
        vulkan_engine->draw_instanced_with_texture_array(model_name, texture_array_name, model_matrices, texture_indices);
    }

    void Renderer::draw_planes(const std::string& texture_array_name, const std::vector<glm::mat4>& model_matrices, const std::vector<uint32_t>& texture_indices, const std::vector<glm::vec4>& min_max_uvs)
    {
        vulkan_engine->draw_planes(texture_array_name, model_matrices, texture_indices, min_max_uvs);
    }

    void Renderer::load_model(const std::string& model_name, const std::filesystem::path& path)
    {
        vulkan_engine->load_model(model_name, path);
    }

    void Renderer::load_texture(const std::string& texture_name, const std::filesystem::path& path)
    {
        vulkan_engine->load_texture(texture_name, path);
    }

    void Renderer::load_texture_array(const std::string& texture_name, const std::vector<std::filesystem::path>& paths)
    {
        vulkan_engine->load_texture_array(texture_name, paths);
    }

    void Renderer::unload_model(const std::string& name)
    {
        vulkan_engine->unload_model(name);
    }

    void Renderer::unload_texture(const std::string& name)
    {
        vulkan_engine->unload_texture(name);
    }

    void Renderer::unload_texture_array(const std::string& name)
    {
        vulkan_engine->unload_texture_array(name);
    }

    void Renderer::set_model_matrix(const glm::mat4& new_model_matrix)
    {
        vulkan_engine->get_mvp_handler().set_model_matrix(new_model_matrix);
    }

    void Renderer::set_view_matrix(const glm::mat4& new_view_matrix)
    {
        vulkan_engine->get_mvp_handler().set_view_matrix(new_view_matrix);
    }

    void Renderer::set_field_of_view(float new_field_of_view)
    {
        vulkan_engine->get_mvp_handler().set_field_of_view(new_field_of_view);
    }

    void Renderer::set_aspect_ratio(float new_aspect_ratio)
    {
        vulkan_engine->get_mvp_handler().set_aspect_ratio(new_aspect_ratio);
    }

    void Renderer::set_near_plane(float new_near_plane)
    {
        vulkan_engine->get_mvp_handler().set_near_plane(new_near_plane);
    }

    void Renderer::set_far_plane(float new_far_plane)
    {
        vulkan_engine->get_mvp_handler().set_far_plane(new_far_plane);
    }

    GLFWwindow* Renderer::get_window()
    {
        return vulkan_engine->get_glfw_window_ptr();
    }
    void Renderer::resize_window(const uint32_t new_width, const uint32_t new_height)
    {
        vulkan_engine->resize_window(new_width, new_height);
    }

    float Renderer::get_aspect_ratio() const
    {
        return vulkan_engine->get_mvp_handler().get_aspect_ratio();
    }
    std::string Renderer::get_memory_statistics() const
    {
        return vulkan_engine->get_memory_statistics();
    }
}