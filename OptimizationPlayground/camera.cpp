#include "pch.h"
#include "camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, glm::vec3 direction)
    : position(position), up(glm::normalize(up)), direction(glm::normalize(direction))
{

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
    set_direction(rotation * glm::vec4(direction, 1.0f));
}

void Camera::rotate_right(float speed)
{
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), -speed * glm::radians(1.f), glm::vec3(0.0f, 1.0f, 0.0f));
    set_direction(rotation * glm::vec4(direction, 1.0f));
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