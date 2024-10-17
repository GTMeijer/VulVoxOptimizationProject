#pragma once
class Scene
{
public:
    Scene(Vulkan_Renderer* renderer) : renderer(renderer) {};


private:


    Vulkan_Renderer* renderer;
};

