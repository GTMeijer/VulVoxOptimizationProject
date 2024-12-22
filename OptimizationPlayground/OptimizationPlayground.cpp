#include "pch.h"
#include "scene.h"

#include "imgui.h"

int main()
{
    constexpr uint32_t width = 1024;
    constexpr uint32_t height = 720;

    float slider_value = 0.5f;

    try
    {
        vulvox::Renderer renderer;

        renderer.init(width, height, glm::radians(45.0f), 0.1f, 1000.0f);

        renderer.init_imgui();

        Scene scene(renderer);
        


        auto previous_frame_time = std::chrono::high_resolution_clock::now();
        while (!renderer.should_close())
        {
            auto frame_time = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(frame_time - previous_frame_time).count();
            previous_frame_time = frame_time;

            //Required to update input state
            glfwPollEvents();

            scene.update(delta_time);

            renderer.start_draw();
            renderer.set_imgui_callback([&slider_value]() {
                ImGui::Begin("Demo Window");
                ImGui::Text("Hello, ImGui!");
                ImGui::SliderFloat("Slider", &slider_value, 0.0f, 1.0f);
                ImGui::End();
                });


            scene.draw();
            renderer.end_draw();
        }

        renderer.destroy();
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
}