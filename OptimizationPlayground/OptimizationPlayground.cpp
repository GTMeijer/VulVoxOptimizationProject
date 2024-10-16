#include "pch.h"

#include "../VulVoxOptimizationProject/pch.h"
#include "../VulVoxOptimizationProject/vulkan_window.h"

int main()
{
    Vulkan_Renderer renderer;

    renderer.init();
    renderer.run();

    std::cout << "Hello World!\n";

}