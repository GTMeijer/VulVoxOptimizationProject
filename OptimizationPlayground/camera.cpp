#include "pch.h"
#include "camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, glm::vec3 direction, float aspect_ratio, float field_of_view, float near_plane, float far_plane)
    : position(position), up(glm::normalize(up)), direction(glm::normalize(direction)), aspect_ratio(aspect_ratio), field_of_view(field_of_view), near_plane(near_plane), far_plane(far_plane)
{

}

vulvox::MVP Camera::get_mvp() const
{
    vulvox::MVP mvp;
    mvp.model = glm::mat4(1.0f); //Identity, no need to transform all models
    mvp.view = get_view_matrix();
    mvp.projection = get_projection_matrix();

    return mvp;
}

glm::mat4 Camera::get_projection_matrix() const
{
    glm::mat4 projection_matrix = glm::perspective(glm::radians(45.0f), aspect_ratio, near_plane, far_plane);;
    projection_matrix[1][1] *= -1; //Invert y-axis so its compatible with Vulkan axes
    return projection_matrix;
}

glm::mat4 Camera::get_view_matrix() const
{
    glm::mat4 view_matrix = glm::lookAt(position, position + direction, up);
    return view_matrix;
}

void Camera::set_position(glm::vec3 new_position)
{
    position = new_position;
}

void Camera::update_position(glm::vec3 position_offset)
{
    position += position_offset;
}

void Camera::set_up(glm::vec3 new_up)
{
    up = glm::normalize(new_up);
}

void Camera::update_up(const glm::mat4& transformation_matrix)
{
    up = transformation_matrix * glm::vec4(up, 1.0f);
    up = glm::normalize(up);
}

void Camera::move_forward(float distance)
{
    update_position(direction * distance);
}

void Camera::move_backward(float distance)
{
    update_position(direction * -distance);
}

void Camera::move_left(float distance)
{
    glm::vec3 side_axis = glm::cross(direction, up);
    update_position(side_axis * -distance);
}

void Camera::move_right(float distance)
{
    glm::vec3 side_axis = glm::cross(direction, up);
    update_position(side_axis * distance);
}

void Camera::move_up(float distance)
{
    update_position(glm::vec3(0.f, distance, 0.f));
}

void Camera::move_down(float distance)
{
    update_position(glm::vec3(0.f, -distance, 0.f));
}

void Camera::rotate_left(float speed)
{
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), speed * glm::radians(1.f), glm::vec3(0.0f, 1.0f, 0.0f));
    direction = rotation * glm::vec4(direction, 1.0f);
}

void Camera::rotate_right(float speed)
{
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), -speed * glm::radians(1.f), glm::vec3(0.0f, 1.0f, 0.0f));
    direction = rotation * glm::vec4(direction, 1.0f);
}

void Camera::set_direction(glm::vec3 new_direction)
{
    direction = glm::normalize(new_direction);
}

void Camera::update_direction(const glm::mat4& transformation_matrix)
{
    direction = transformation_matrix * glm::vec4(direction, 1.0f);
    direction = glm::normalize(direction);
}

void Camera::set_aspect_ratio(float new_aspect_ratio)
{
    aspect_ratio = new_aspect_ratio;
}

void Camera::set_field_of_view(float new_field_of_view)
{
    field_of_view = new_field_of_view;
}

void Camera::set_near_plane(float new_near_plane)
{
    near_plane = new_near_plane;
}

void Camera::set_far_plane(float new_far_plane)
{
    far_plane = new_far_plane;
}
