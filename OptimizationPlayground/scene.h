#pragma once
class Scene
{
public:
    Scene() : renderer(renderer) {};

    void draw(Vulkan_Renderer& renderer);

private:

    Camera camera;
};

