#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vk_enum_string_helper.h> //for error code conversion

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <map>
#include <cstring>
#include <optional>

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
        pick_physical_device();
        create_logical_device();
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

    /// <summary>
    /// Select the GPU to use among available devices.
    /// </summary>
    void pick_physical_device()
    {
        uint32_t device_count = 0;

        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

        if (device_count == 0)

        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());


        std::multimap<int, VkPhysicalDevice> device_candidates;

        std::cout << "Found devices:\n";
        int i = 1;
        for (auto& device : devices)
        {
            int device_rating = rate_physical_device(device);

            std::cout << i << ": " << get_physical_device_name(device) << "\t" << get_physical_device_type(device) << "\t" << "Score: " << device_rating << "\n";

            device_candidates.insert(std::make_pair(rate_physical_device(device), device));

            i++;
        }

        std::cout << std::flush;

        //Use the highest scoring GPU. If score is 0, no suitable GPU was found.
        if (device_candidates.rbegin()->first > 0)
        {
            physical_device = device_candidates.rbegin()->second;
        }
        else
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    std::string get_physical_device_name(const VkPhysicalDevice& device) const
    {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(device, &device_properties);

        std::string device_name(device_properties.deviceName);

        return device_name;
    }

    std::string get_physical_device_type(const VkPhysicalDevice& device) const
    {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(device, &device_properties);

        switch (device_properties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return "VK_PHYSICAL_DEVICE_TYPE_CPU";
            break;
        default:
            return "UNKNOWN";
            break;
        }
    }

    /// <summary>
    /// Rate a given GPU device based on application requirements.
    /// </summary>
    /// <param name="device"></param>
    /// <returns></returns>
    int rate_physical_device(const VkPhysicalDevice& device) const
    {
        int device_score = 0;

        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(device, &device_properties);

        //Discrete GPUs perform better than intergrated, fall back to virtual if none are available
        if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            device_score += 1000;
        }
        else if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            device_score += 100;
        }
        else if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
        {
            device_score += 1;
        }

        device_score += device_properties.limits.maxImageDimension2D;

        if (Queue_Family_Indices indices = find_queue_families(device); !indices.is_complete())
        {
            return 0;
        }

        return device_score;
    }

    void create_logical_device()
    {
        Queue_Family_Indices indices = find_queue_families(physical_device);

        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = indices.graphics_family.value();
        queue_create_info.queueCount = 1; //Only one queue is needed, multiple threads can each have a command buffer that feeds into it

        //Set queue priority, higher values have higher priority
        float queuePriority = 1.0f;
        queue_create_info.pQueuePriorities = &queuePriority;

        //Set the required device features
        VkPhysicalDeviceFeatures device_features{}; //No required features for now.

        VkDeviceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        create_info.pQueueCreateInfos = &queue_create_info;
        create_info.queueCreateInfoCount = 1;

        create_info.pEnabledFeatures = &device_features;

        create_info.enabledExtensionCount = 0;

        if (enableValidationLayers)
        {
            create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();
        }
        else
        {
            create_info.enabledLayerCount = 0;
        }

        //Setup device specific extensions
            //Nothing for now.

        if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device!");
        }

        //Store the handle for the graphics queue
        vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
    }

    struct Queue_Family_Indices
    {
        std::optional<uint32_t> graphics_family;

        /// <summary>
        /// Check if all queue families are filled.
        /// </summary>
        /// <returns></returns>
        bool is_complete()
        {
            return graphics_family.has_value();
        }
    };

    Queue_Family_Indices find_queue_families(const VkPhysicalDevice& device) const
    {
        Queue_Family_Indices indices;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        int i = 0;
        for (const auto& queue_family : queue_families)
        {
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphics_family = i;
            }

            //Early out if all queue families are found.
            if (indices.is_complete())
            {
                break;
            }

            i++;
        }


        return indices;
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
        vkDestroyDevice(device, nullptr);

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
    VkPhysicalDevice physical_device = nullptr;
    VkDevice device;

    VkQueue graphics_queue;


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