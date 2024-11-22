#pragma once
namespace vulvox
{
    class MVP_Handler
    {
    public:

        MVP_Handler();

        void set_model_matrix(const glm::mat4& new_model_matrix);
        void set_view_matrix(const glm::mat4& new_view_matrix);

        void set_field_of_view(float new_field_of_view);
        void set_aspect_ratio(float new_aspect_ratio);
        void set_near_plane(float new_near_plane);
        void set_far_plane(float new_far_plane);

        float get_aspect_ratio() const;

        MVP model_view_projection;

    private:

        void update_projection_matrix();

        float field_of_view;
        float aspect_ratio;
        float near_plane;
        float far_plane;

    };
}