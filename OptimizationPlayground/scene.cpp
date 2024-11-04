#include "pch.h"
#include "scene.h"

Scene::Scene(vulvox::Vulkan_Renderer& renderer) : renderer(&renderer)
{
    glm::vec3 camera_pos{ 0.0f, 0.0f, 0.0f };
    glm::vec3 camera_up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 camera_direction{ 0.0f, 0.0f, 1.0f };

    camera = Camera(camera_pos, camera_up, camera_direction, renderer.get_aspect_ratio(), glm::radians(45.0f), 0.1f, 1000.0f);

    renderer.load_model("Konata", MODEL_PATH);
    renderer.load_texture("Konata", TEXTURE_PATH);

    konata_matrix = glm::mat4{ 1.0f };

    renderer.load_model("cube", CUBE_MODEL_PATH);
    renderer.load_texture("cube", CUBE_TEXTURE_PATH);

    konata_matrices.reserve(25);
    for (size_t i = 0; i < 25; i++)
    {
        for (size_t j = 0; j < 25; j++)
        {
            vulvox::Instance_Data instance_data;
            instance_data.instance_model_matrix = glm::translate(konata_matrix, glm::vec3(i * 75.f, 125.0f, j * 75.f));

            konata_matrices.push_back(instance_data);
        }
    }

}

void Scene::update(float delta_time)
{
    //Update camera on key presses
    float camera_speed = 100.0f;
    if (glfwGetKey(renderer->get_window(), GLFW_KEY_W) == GLFW_PRESS) { camera.move_forward(delta_time * camera_speed); }
    if (glfwGetKey(renderer->get_window(), GLFW_KEY_S) == GLFW_PRESS) { camera.move_backward(delta_time * camera_speed); }
    if (glfwGetKey(renderer->get_window(), GLFW_KEY_Q) == GLFW_PRESS) { camera.move_left(delta_time * camera_speed); }
    if (glfwGetKey(renderer->get_window(), GLFW_KEY_E) == GLFW_PRESS) { camera.move_right(delta_time * camera_speed); }
    if (glfwGetKey(renderer->get_window(), GLFW_KEY_A) == GLFW_PRESS) { camera.rotate_left(camera_speed * delta_time); }
    if (glfwGetKey(renderer->get_window(), GLFW_KEY_D) == GLFW_PRESS) { camera.rotate_right(camera_speed * delta_time); }
    if (glfwGetKey(renderer->get_window(), GLFW_KEY_SPACE) == GLFW_PRESS) { camera.move_up(delta_time * camera_speed); }
    if (glfwGetKey(renderer->get_window(), GLFW_KEY_Z) == GLFW_PRESS) { camera.move_down(delta_time * camera_speed); }

    camera.set_aspect_ratio(renderer->get_aspect_ratio());
    renderer->set_camera(camera.get_mvp());

    for (auto& konata_matrix : konata_matrices)
    {
        konata_matrix.instance_model_matrix = glm::rotate(konata_matrix.instance_model_matrix, delta_time * glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
    }
}

void Scene::draw()
{
    for (size_t i = 0; i < 5; i++)
    {
        for (size_t j = 0; j < 5; j++)
        {
            glm::mat4 pos = glm::translate(konata_matrix, glm::vec3(i * 75.f, 0.0f, j * 75.f));

            renderer->draw_model("Konata", "Konata", pos);
        }
    }

    renderer->draw_instanced("Konata", "Konata", konata_matrices);
}