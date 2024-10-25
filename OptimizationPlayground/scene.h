#pragma once
class Scene
{
public:
    explicit Scene(vulvox::Vulkan_Renderer& renderer);

    void update(float delta_time, GLFWwindow* window);
    void draw(vulvox::Vulkan_Renderer& renderer);

private:

    Camera camera;
};

