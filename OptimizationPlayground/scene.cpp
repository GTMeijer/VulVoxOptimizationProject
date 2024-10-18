#include "pch.h"
#include "scene.h"

Scene::Scene(vulvox::Vulkan_Renderer& renderer)
{
    glm::vec3 camera_pos{ 0.0f, 0.0f, 0.0f };
    glm::vec3 camera_up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 camera_direction{ 0.0f, 0.0f, 1.0f };

    camera = Camera(camera_pos, camera_up, camera_direction, renderer.get_aspect_ratio(), glm::radians(45.0f), 0.1f, 1000.0f);
}

void Scene::update(float delta_time, GLFWwindow* window)
{
    //Update camera on key presses
    float camera_speed = 100.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { camera.move_forward(delta_time * camera_speed); }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { camera.move_backward(delta_time * camera_speed); }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) { camera.move_left(delta_time * camera_speed); }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) { camera.move_right(delta_time * camera_speed); }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { camera.rotate_left(camera_speed * delta_time); }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { camera.rotate_right(camera_speed * delta_time); }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) { camera.move_up(delta_time * camera_speed); }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) { camera.move_down(delta_time * camera_speed); }
}

void Scene::draw(vulvox::Vulkan_Renderer& renderer)
{
    renderer.set_camera(camera.get_mvp());

    //renderer.
}

