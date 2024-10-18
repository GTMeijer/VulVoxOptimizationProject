#include "pch.h"
#include "scene.h"

void Scene::draw(Vulkan_Renderer& renderer)
{
    renderer.set_camera(camera.get_mvp());

    renderer.

}
