#include "pch.h"
#include "mvp_handler.h"

namespace vulvox
{
    MVP_Handler::MVP_Handler() : field_of_view(glm::radians(0.45f)), aspect_ratio(16.f / 9.0f), near_plane(0.1f), far_plane(1000.f)
    {
        update_projection_matrix();
        model_view_projection.model = glm::mat4(1.0f);
    }

    void MVP_Handler::set_model_matrix(const glm::mat4& new_model_matrix)
    {
        model_view_projection.model = new_model_matrix;
    }

    void MVP_Handler::set_view_matrix(const glm::mat4& new_view_matrix)
    {
        model_view_projection.view = new_view_matrix;
    }

    void MVP_Handler::set_field_of_view(float new_field_of_view)
    {
        field_of_view = new_field_of_view;
        update_projection_matrix();
    }

    void MVP_Handler::set_aspect_ratio(float new_aspect_ratio)
    {
        aspect_ratio = new_aspect_ratio;
        update_projection_matrix();
    }

    void MVP_Handler::set_near_plane(float new_near_plane)
    {
        near_plane = new_near_plane;
        update_projection_matrix();
    }

    void MVP_Handler::set_far_plane(float new_far_plane)
    {
        far_plane = new_far_plane;
        update_projection_matrix();
    }

    float MVP_Handler::get_aspect_ratio() const
    {
        return aspect_ratio;
    }

    void MVP_Handler::update_projection_matrix()
    {
        glm::mat4 projection_matrix = glm::perspective(field_of_view, aspect_ratio, near_plane, far_plane);
        projection_matrix[1][1] *= -1; //Invert y-axis so its compatible with Vulkan axes
        model_view_projection.projection = projection_matrix;
    }
}