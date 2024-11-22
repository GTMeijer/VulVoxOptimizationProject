#pragma once
class Camera
{
public:

    Camera() = default;
    Camera(glm::vec3 position, glm::vec3 up, glm::vec3 direction);

    glm::mat4 get_view_matrix()  const;

    void set_position(glm::vec3 new_position);
    void update_position(glm::vec3 position_offset);

    void set_up(glm::vec3 new_up);
    void update_up(const glm::mat4& transformation_matrix);

    void move_forward(float distance);
    void move_backward(float distance);
    void move_left(float distance);
    void move_right(float distance);
    void move_up(float distance);
    void move_down(float distance);
    void rotate_left(float speed);
    void rotate_right(float speed);

    void set_direction(glm::vec3 new_direction);
    void update_direction(const glm::mat4& transformation_matrix);

private:

    glm::vec3 position;
    glm::vec3 up;
    glm::vec3 direction;
};

