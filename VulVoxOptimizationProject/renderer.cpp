#include "pch.h"
#include "renderer.h"

namespace vulvox
{
    Renderer::Renderer()
    {
        vulkan_engine = std::make_unique<Vulkan_Engine>();
    }

    Renderer::~Renderer() = default;

    void Renderer::init(uint32_t width, uint32_t height)
    {
        vulkan_engine->init(width, height);
    }

    void Renderer::destroy()
    {
        vulkan_engine->destroy();
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

    void Renderer::draw_instanced(const std::string& model_name, const std::string& texture_name, const std::vector<Instance_Data>& instance_data)
    {
        vulkan_engine->draw_instanced(model_name, texture_name, instance_data);
    }

    void Renderer::draw_instanced_with_texture_array(const std::string& model_name, const std::string& texture_array_name, const std::vector<Instance_Data>& instance_data, const std::vector<uint32_t>& texture_indices)
    {
        vulkan_engine->draw_instanced_with_texture_array(model_name, texture_array_name, instance_data, texture_indices);
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

    void Renderer::set_camera(const MVP& camera_matrix)
    {
        vulkan_engine->set_model_view_projection(camera_matrix);
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
        return vulkan_engine->get_aspect_ratio();
    }
}