#pragma once
class Scene
{
public:
    Scene(vulvox::Vulkan_Renderer& renderer);

    void update(float delta_time);
    void draw(vulvox::Vulkan_Renderer& renderer);

private:

    Camera camera;
};

