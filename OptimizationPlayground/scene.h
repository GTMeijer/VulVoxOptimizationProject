#pragma once
class Scene
{
public:
    Scene(vulvox::Vulkan_Renderer& renderer);

    void update(float delta_time, GLFWwindow* window);
    void draw(vulvox::Vulkan_Renderer& renderer);


    static void key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods);

private:

    Camera camera;
};

