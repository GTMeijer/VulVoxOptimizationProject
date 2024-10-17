#pragma once
class Camera
{
public:

    Camera(glm::vec3 position, glm::vec3 up, glm::vec3 direction, float aspect_ratio, float field_of_view, float near_plane, float far_plane);

    glm::mat4 get_projection_matrix();
    glm::mat4 get_view_matrix();

    void set_position(glm::vec3 new_position);
    void update_position(glm::vec3 position_offset);

    void set_up(glm::vec3 new_up);
    void update_up(glm::mat4 matrix);

    void set_direction(glm::vec3 new_direction);
    void update_direction(glm::mat4 matrix);
    void update_direction(glm::vec3 direction_offset);

    void set_aspect_ratio(float new_aspect_ratio);
    void set_field_of_view(float new_field_of_view);
    void set_near_plane(float new_near_plane);
    void set_far_plane(float new_far_plane);

private:

    glm::vec3 position;
    glm::vec3 up;
    glm::vec3 direction;

    float aspect_ratio;
    float field_of_view;
    float near_plane;
    float far_plane;
};

