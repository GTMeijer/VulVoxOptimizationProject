#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vk_enum_string_helper.h> //for error code conversion

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>

#include "utils.h"

class Hello_Triangle_Application
{
public:

    void run()
    {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

private:

    void init_window()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
    }

    void init_vulkan()
    {
        create_instance();
    }

    void create_instance()
    {
        if (enableValidationLayers && !check_validation_layer_support())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Hello Triangle";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        create_info.enabledExtensionCount = glfw_extension_count;
        create_info.ppEnabledExtensionNames = glfw_extensions;

        if (enableValidationLayers)
        {
            create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();
        }
        else
        {
            create_info.enabledLayerCount = 0;
        }

        if (!check_glfw_extension_support())
        {
            throw std::runtime_error("Not all glfw extensions are supported!");
        }

        if (VkResult result = vkCreateInstance(&create_info, nullptr, &instance); result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance! " + std::string(string_VkResult(result)));
        }

        //if VK_ERROR_INCOMPATIBLE_DRIVER on mac, check: https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/01_Instance.html#_encountered_vk_error_incompatible_driver

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        std::cout << "Available extensions: \n";

        for (const auto& extension : extensions)
        {
            std::cout << '\t' << extension.extensionName << '\n';
        }
    }

    bool check_glfw_extension_support() const
    {
        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        std::vector<std::string> available_extension_names;
        for (const auto& extension : extensions)
        {
            available_extension_names.emplace_back(extension.extensionName);
        }

        std::cout << "Required extensions for glfw:\n";

        bool all_supported = true;
        for (int i = 0; i < glfw_extension_count; i++)
        {
            std::string glfw_extension_name = glfw_extensions[i];

            std::cout << "\t" << glfw_extension_name << "\t";

            if (contains(available_extension_names, glfw_extension_name))
            {
                std::cout << "AVAILABLE\n";
            }
            else
            {
                all_supported = false;
                std::cout << "UNAVAILABLE\n";
            }
        }

        std::cout << std::endl;

        return all_supported;
    }

    void main_loop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    bool check_validation_layer_support()
    {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (const char* layer_name : validation_layers)
        {
            bool layer_found = true;

            for (const auto& layerProperties : available_layers)
            {
                if (strcmp(layer_name, layerProperties.layerName) == 0)
                {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found)
            {
                return false;
            }

        }

        return true;
    }

    uint32_t width = 800;
    uint32_t height = 600;
    GLFWwindow* window;
    VkInstance instance;

    const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
};

int main()
{
    Hello_Triangle_Application app;

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