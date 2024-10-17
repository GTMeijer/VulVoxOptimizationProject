#include "pch.h"
#include "camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, glm::vec3 direction, float aspect_ratio, float field_of_view, float near_plane, float far_plane)
    : position(position), up(up), direction(direction), aspect_ratio(aspect_ratio), field_of_view(field_of_view), near_plane(near_plane), far_plane(far_plane)
{


}

glm::mat4 Camera::get_projection_matrix()
{
    //mvp.projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
    glm::mat4 projection_matrix = glm::perspective(glm::radians(45.0f), aspect_ratio, near_plane, far_plane);;
    projection_matrix[1][1] *= -1; //Invert y-axis so its compatible with Vulkan axes
    return projection_matrix;
}

glm::mat4 Camera::get_view_matrix()
{
    glm::mat4 view_matrix = glm::lookAt(position, direction, up);
    return view_matrix;
}
