#include "pch.h"
#include "scene.h"

Scene::Scene(vulvox::Vulkan_Renderer& renderer) : renderer(&renderer)
{
    glm::vec3 camera_pos{ 0.0f, 0.0f, 0.0f };
    glm::vec3 camera_up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 camera_direction{ 0.0f, 0.0f, 1.0f };

    camera = Camera(camera_pos, camera_up, camera_direction, renderer.get_aspect_ratio(), glm::radians(45.0f), 0.1f, 1000.0f);

    renderer.load_model("Konata", MODEL_PATH);
    renderer.load_texture("Konata", KONATA_MODAL_TEXTURE_PATH);

    konata_matrix = glm::mat4{ 1.0f };

    renderer.load_model("cube", CUBE_MODEL_PATH);
    renderer.load_texture("cube", CUBE_WHITE_TEXTURE_PATH);

    std::vector<std::filesystem::path> texture_paths{ CUBE_WHITE_TEXTURE_PATH, CUBE_BLUE_TEXTURE_PATH };
    renderer.load_texture_array("texture_array_test", texture_paths);

    konata_matrices.reserve(25);
    for (size_t i = 0; i < 25; i++)
    {
        for (size_t j = 0; j < 25; j++)
        {
            vulvox::Instance_Data instance_data;
            instance_data.instance_model_matrix = glm::translate(konata_matrix, glm::vec3(i * 10.f, 125.0f, j * 10.f));
            instance_data.instance_model_matrix = glm::scale(instance_data.instance_model_matrix, glm::vec3(10.f, 10.f, 10.f));

            konata_matrices.push_back(instance_data);
        }
    }


    for (size_t i = 0; i < konata_matrices.size(); i++)
    {
        texture_indices.push_back(i % 2);
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

    if (glfwGetKey(renderer->get_window(), GLFW_KEY_KP_ADD) == GLFW_PRESS)
    {
        const int x_count = 251;
        const int y_count = 251;
        num_layers++;

        konata_matrices.reserve(x_count * y_count * num_layers);
        texture_indices.reserve(x_count * y_count * num_layers);

        for (size_t i = 0; i < y_count; i++)
        {
            for (size_t j = 0; j < x_count; j++)
            {
                vulvox::Instance_Data instance_data;
                instance_data.instance_model_matrix = glm::translate(konata_matrix, glm::vec3(i * 10.f, (num_layers + 1) * 10.0f + 125.0f, j * 10.f));
                instance_data.instance_model_matrix = glm::scale(instance_data.instance_model_matrix, glm::vec3(10.f, 10.f, 10.f));

                konata_matrices.push_back(instance_data);

                texture_indices.push_back(((i * y_count) + j) % 2);
            }
        }
    }


    camera.set_aspect_ratio(renderer->get_aspect_ratio());
    renderer->set_camera(camera.get_mvp());


}

void Scene::draw()
{
    for (size_t i = 0; i < 5; i++)
    {
        for (size_t j = 0; j < 5; j++)
        {
            glm::mat4 pos = glm::translate(konata_matrix, glm::vec3(i * 75.f, 0.0f, j * 75.f));

            renderer->draw_model("Konata", "Konata", pos);
            //renderer->draw_model_with_texture_array("cube", "texture_array_test", 1, pos);
        }
    }



    //renderer->draw_instanced("cube", "cube", konata_matrices);
    renderer->draw_instanced_with_texture_array("cube", "texture_array_test", konata_matrices, texture_indices);
    //renderer->draw_model_with_texture_array("cube", "texture_array_test", 1, konata_matrix);
}