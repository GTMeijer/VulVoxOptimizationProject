#pragma once
class Scene
{
public:
    explicit Scene(vulvox::Vulkan_Renderer& renderer);

    void update(float delta_time);
    void draw();

private:

    vulvox::Vulkan_Renderer* renderer;
    Camera camera;
};

