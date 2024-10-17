#pragma once
class Camera
{
public:

    Camera(glm::vec3 position, glm::vec3 up, glm::vec3 look_at, float aspect_ratio, float field_of_view, float near_plane, float far_plane);

    glm::mat4 get_projection_matrix();
    glm::mat4 get_view_matrix();

private:

    glm::vec3 position;
    glm::vec3 up;
    glm::vec3 look_at;

    float aspect_ratio;
    float field_of_view;
    float near_plane;
    float far_plane;
};

