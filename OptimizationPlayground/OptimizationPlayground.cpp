#include "pch.h"
#include "scene.h"

uint32_t width = 1024;
uint32_t height = 720;

float rotation = 0.f;
float scale = 1.f;
glm::vec3 position;

int main()
{
    vulvox::Vulkan_Renderer renderer;

    renderer.init(width, height);
    
    Scene scene(renderer);

    auto previous_frame_time = std::chrono::high_resolution_clock::now();
    while (!renderer.should_close())
    {
        static auto frame_time = std::chrono::high_resolution_clock::now();
        float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(previous_frame_time - frame_time).count();
        previous_frame_time = frame_time;

        glfwPollEvents();
        //TODO: Poll Events -> Set callback
        //TODO: Update
        //TODO: Draw

        scene.update(delta_time);
        scene.draw(renderer);

        renderer.draw_frame();
    }

    renderer.cleanup();

}