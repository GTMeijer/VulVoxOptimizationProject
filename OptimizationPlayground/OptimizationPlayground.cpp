#include "pch.h"


uint32_t width = 1024;
uint32_t height = 720;

float rotation = 0.f;
float scale = 1.f;
glm::vec3 position;



void update(float delta_time)
{
    //Rotate around the z-axis
    MVP mvp{};
    mvp.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 0.25f, 0.25f)) * glm::rotate(glm::mat4(1.0f), time * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f));
    mvp.view = glm::lookAt(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mvp.projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    mvp.projection[1][1] *= -1; //Invert y-axis so its compatible with Vulkan axes
}

int main()
{
    Vulkan_Renderer renderer;

    renderer.init(width, height);


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

        update(delta_time);

        renderer.draw_frame();
    }

    renderer.cleanup();

}