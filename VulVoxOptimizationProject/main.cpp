#include "pch.h"
#include "Vulkan_Window.h"

int main()
{
    Vulkan_Window app;

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}