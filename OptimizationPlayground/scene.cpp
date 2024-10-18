#include "pch.h"
#include "scene.h"

Scene::Scene(vulvox::Vulkan_Renderer& renderer)
{
    glm::vec3 camera_pos{ 20.0f, 20.0f, 20.0f };
    glm::vec3 camera_up{ 0.0f, 0.0f, 1.0f };
    glm::vec3 camera_direction{ 0.0f, 0.0f, 15.0f };

    camera = Camera(camera_pos, camera_up, camera_direction, renderer.get_aspect_ratio(), glm::radians(45.0f), 0.1f, 100.0f);
}

void Scene::update(float delta_time)
{
    //Rotate around the z-axis
    //mvp.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 0.25f, 0.25f)) * glm::rotate(glm::mat4(1.0f), delta_time * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f));




}

void Scene::draw(vulvox::Vulkan_Renderer& renderer)
{
    renderer.set_camera(camera.get_mvp());

    //renderer.
}