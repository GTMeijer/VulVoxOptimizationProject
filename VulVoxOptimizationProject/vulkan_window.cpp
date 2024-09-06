#include "pch.h"
#include "vulkan_window.h"


//TODO: Probably do some lambda stuff here?
void Vulkan_Window::main_loop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}

void Vulkan_Window::init_window()
{
    std::cout << "Init window.." << std::endl;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);

    std::cout << "Window initialized." << std::endl;
}

void Vulkan_Window::init_vulkan()
{
    std::cout << "Init vulkan.." << std::endl;

    create_instance();
    create_surface();
    pick_physical_device();
    create_logical_device();
    create_swap_chain();

    std::cout << "Vulkan initialized." << std::endl;
}

void Vulkan_Window::cleanup()
{
    vkDestroySwapchainKHR(device, swap_chain, nullptr);

    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

/// <summary>
/// Create a vulkan instance and connect it to a glfw window
/// </summary>
void Vulkan_Window::create_instance()
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

    std::cout << std::endl;
}

void Vulkan_Window::create_surface()
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void Vulkan_Window::create_logical_device()
{
    Queue_Family_Indices indices = find_queue_families(physical_device);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

    //Set queue priority, higher values have higher priority
    float queue_priority = 1.0f;

    //Setup the information needed to create the queues.
    for (uint32_t queue_family : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1; //Only one queue is needed, multiple threads can each have a command buffer that feeds into it
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    //Set the required device features
    VkPhysicalDeviceFeatures device_features{}; //No required features for now.

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    //Pass the queue information
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();

    create_info.pEnabledFeatures = &device_features;

    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();

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

    //Store the handles for the graphics and present queues
    vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);

    vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}

void Vulkan_Window::create_swap_chain()
{
    //Query supported formats and extends

    Swap_Chain_Support_Details swap_chain_support = query_swap_chain_support(physical_device);

    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats);
    std::cout << "Surface format: " << string_VkFormat(surface_format.format) << std::endl;

    VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
    std::cout << "Present mode: " << string_VkPresentModeKHR(present_mode) << std::endl;

    VkExtent2D extent = choose_swap_extent(swap_chain_support.capabilities);
    std::cout << "Image buffer extent: " << extent.width << " x " << extent.height << std::endl;

    std::cout << std::endl;

    //Add one to the image buffer count so we have another image buffer to write to when the driver is swapping image buffers
    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

    //Make sure we dont exceed the maximum image buffer count
    if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
    {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;

    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1; //Always one, unless we are working on a stereoscopic 3D application
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //We are writing directly to the image buffer, change when adding post-processing

    Queue_Family_Indices indices = find_queue_families(physical_device);

    std::array<uint32_t, 2> queue_family_indices = { indices.graphics_family.value(), indices.present_family.value() };


    if (indices.graphics_family != indices.present_family)
    {
        //If the graphics and present queues are seperate we default to exclusive mode for simplicity
        //This saves us handling ownership of the images
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices.data();
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform; //Rotates or flips the image in the swapchain (disabled)
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //Disable window alpha blend
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE; //Don't render obscure pixels
    create_info.oldSwapchain = nullptr; //For now, required when recreating the swap chain (e.g. when minimizing)

    if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain!");
    }
}

/// <summary>
/// Select the GPU to use among available devices.
/// </summary>
void Vulkan_Window::pick_physical_device()
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

    std::cout << std::endl;

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

VkSurfaceFormatKHR Vulkan_Window::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) const
{
    for (const auto& available_format : available_formats)
    {
        //We prefer the SRGB format
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return available_format;
        }
    }

    //If our preferred format is not available just use the first available one
    return available_formats[0];
}

VkPresentModeKHR Vulkan_Window::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) const
{
    for (const auto& available_present_mode : available_present_modes)
    {
        //We prefer triple buffering
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return available_present_mode;
        }
    }

    //We default to vertical sync
    return VK_PRESENT_MODE_FIFO_KHR;
}

/// <summary>
/// Pick the appropriate resolution for the swap chain.
/// </summary>
/// <param name="capabilities"></param>
/// <returns></returns>
VkExtent2D Vulkan_Window::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        //Use the recommended (by vulkan) pixel size
        return capabilities.currentExtent;
    }
    else
    {
        //Retrieve the window size in pixels and clamp to the available swap chain size
        //Allows support for screens with high DPI

        int width;
        int height;

        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actual_extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

        actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actual_extent;
    }
}

std::string Vulkan_Window::get_physical_device_name(const VkPhysicalDevice& device) const
{
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    std::string device_name(device_properties.deviceName);

    return device_name;
}

std::string Vulkan_Window::get_physical_device_type(const VkPhysicalDevice& device) const
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
int Vulkan_Window::rate_physical_device(const VkPhysicalDevice& device) const
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

    Queue_Family_Indices indices = find_queue_families(device);

    bool extensions_supported = check_device_extension_support(device);

    bool swap_chain_adequate = false;
    if (extensions_supported)
    {
        Swap_Chain_Support_Details swap_chain_support = query_swap_chain_support(device);
        swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
    }

    if (!indices.is_complete() && !extensions_supported && !swap_chain_adequate)
    {
        return 0;
    }

    return device_score;
}

/// <summary>
/// Checks if all the required extensions needed by glfw are available.
/// </summary>
bool Vulkan_Window::check_glfw_extension_support() const
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
    for (uint32_t i = 0; i < glfw_extension_count; i++)
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

bool Vulkan_Window::check_device_extension_support(const VkPhysicalDevice& device) const
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string, std::less<>> required_extensions(device_extensions.begin(), device_extensions.end());

    for (const auto& extension : available_extensions)
    {
        required_extensions.erase(extension.extensionName);
    }

    //If the required extension set is empty, all required extensions are supported by this device.
    return required_extensions.empty();
}

bool Vulkan_Window::check_validation_layer_support() const
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

Vulkan_Window::Swap_Chain_Support_Details Vulkan_Window::query_swap_chain_support(const VkPhysicalDevice& device) const
{
    Swap_Chain_Support_Details details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());

    if (format_count != 0)
    {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

    if (present_mode_count != 0)
    {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
    }

    return details;
}

Vulkan_Window::Queue_Family_Indices Vulkan_Window::find_queue_families(const VkPhysicalDevice& device) const
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

        //Check if this device supports window system integration (a.k.a. presentation)
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

        if (present_support)
        {
            indices.present_family = i;
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
