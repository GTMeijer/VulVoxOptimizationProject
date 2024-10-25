#include "pch.h"
#include "scene.h"

int main()
{
    constexpr uint32_t width = 1024;
    constexpr uint32_t height = 720;

    vulvox::Vulkan_Renderer renderer;

    renderer.init(width, height);

    Scene scene(renderer);

    auto previous_frame_time = std::chrono::high_resolution_clock::now();
    while (!renderer.should_close())
    {
        auto frame_time = std::chrono::high_resolution_clock::now();
        float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(frame_time - previous_frame_time).count();
        previous_frame_time = frame_time;

        //Required to update input state
        glfwPollEvents();

        scene.update(delta_time, renderer.get_window());
        scene.draw(renderer);

        renderer.draw_frame();
    }

    renderer.cleanup();
}