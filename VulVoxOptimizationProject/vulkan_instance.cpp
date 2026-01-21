#include "pch.h"
#include "vulkan_instance.h"

namespace vulvox
{
    void Vulkan_Instance::create_instance()
    {
        if (enableValidationLayers && !check_validation_layer_support())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        uint32_t api_version_support;
        vkEnumerateInstanceVersion(&api_version_support);

        std::cout << "Driver supported Vulkan version: " << VK_API_VERSION_MAJOR(api_version_support) << "." << VK_API_VERSION_MINOR(api_version_support) << "\n" << std::endl;

        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Vulkan"; //Application name, doesnt do much (title is in glfw)
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "VulVox";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        //GLFW needs support for window surface creation, we check that here
        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char*> requiredExtensions;

        for (uint32_t i = 0; i < glfw_extension_count; i++) {
            requiredExtensions.emplace_back(glfw_extensions[i]);
        }


#ifdef __APPLE_
        //On MacOS we need to add the following extension check and set the portability flag
        requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

        create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
        create_info.enabledExtensionCount = (uint32_t)requiredExtensions.size();
        create_info.ppEnabledExtensionNames = requiredExtensions.data();

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

    /// <summary>
    /// Select the GPU to use among available physical devices.
    /// </summary>
    void Vulkan_Instance::pick_physical_device(const VkSurfaceKHR surface)
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
        for (auto& device_candidate : devices)
        {
            int device_rating = rate_physical_device(surface, device_candidate);

            std::cout << i << ": " << get_physical_device_name(device_candidate) << "\t" << get_physical_device_type(device_candidate) << "\t Vulkan version: " << get_physical_device_vulkan_support(device_candidate) << "\t Score: " << device_rating << "\n";

            device_candidates.insert(std::make_pair(rate_physical_device(surface, device_candidate), device_candidate));

            i++;
        }

        //Use the highest scoring GPU. If score is 0, no suitable GPU was found.
        if (device_candidates.rbegin()->first > 0)
        {
            physical_device = device_candidates.rbegin()->second;
            std::cout << get_physical_device_name(physical_device) << " selected." << std::endl;
        }
        else
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }

        std::cout << std::endl;
    }

    /// <summary>
    /// Rate a given GPU device based on application requirements.
    /// </summary>
    /// <param name="device"></param>
    /// <returns></returns>
    int Vulkan_Instance::rate_physical_device(const VkSurfaceKHR surface, const VkPhysicalDevice& physical_device_candidate) const
    {
        int device_score = 0;

        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(physical_device_candidate, &device_properties);

        //Discrete GPUs perform better than integrated, fall back to virtual if none are available
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

        Queue_Family_Indices indices = find_queue_families(surface, physical_device_candidate);

        bool extensions_supported = check_device_extension_support(physical_device_candidate);

        bool swap_chain_adequate = false;
        if (extensions_supported)
        {
            Swap_Chain_Support_Details swap_chain_support = query_swap_chain_support(surface, physical_device_candidate);
            swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
        }

        //Anisotropic filtering is required, check capability using the physical device features struct
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(physical_device_candidate, &supportedFeatures);

        if (!indices.is_complete() && !extensions_supported && !swap_chain_adequate && !supportedFeatures.samplerAnisotropy)
        {
            return 0;
        }

        return device_score;
    }

    /// <summary>
    /// Queries the vulkan implementation for glfw extension support.
    /// </summary>
    /// <returns>Returns true if all required GLFW extensions are supported, otherwise false.</returns>
    bool Vulkan_Instance::check_glfw_extension_support() const
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

    /// <summary>
    /// Checks if a given physical device supports the required extensions.
    /// </summary>
    /// <param name="physical_device">The physical device to query.</param>
    /// <returns>True if the physical device supports the required extensions, false otherwise.</returns>
    bool Vulkan_Instance::check_device_extension_support(const VkPhysicalDevice& physical_device) const
    {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());

        std::set<std::string, std::less<>> required_extensions(device_extensions.begin(), device_extensions.end());

        for (const auto& extension : available_extensions)
        {
            required_extensions.erase(extension.extensionName);
        }

        //If the required extension set is empty, all required extensions are supported by this device.
        return required_extensions.empty();
    }

    bool Vulkan_Instance::check_validation_layer_support() const
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

    Swap_Chain_Support_Details Vulkan_Instance::query_swap_chain_support(const VkSurfaceKHR surface, const VkPhysicalDevice& physical_device) const
    {
        Swap_Chain_Support_Details details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities);

        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, details.formats.data());

        if (format_count != 0)
        {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, details.formats.data());
        }

        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);

        if (present_mode_count != 0)
        {
            details.present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, details.present_modes.data());
        }

        return details;
    }

    /// <summary>
    /// Finds the queue families on a given physical device that support both graphics operations 
    /// and presentation to a given surface. It queries the physical device for available queue families,
    /// checks if a family supports graphics operations, and verifies if it supports presentation to the surface.
    /// Returns an object containing the indices of the relevant queue families.
    /// </summary>
    /// <param name="surface">The Vulkan surface to check presentation support against.</param>
    /// <param name="physical_device">The Vulkan physical device to query queue families from.</param>
    /// <returns>Returns a Queue_Family_Indices object containing the indices for graphics and presentation queues.</returns>
    Queue_Family_Indices Vulkan_Instance::find_queue_families(const VkSurfaceKHR surface, const VkPhysicalDevice& physical_device) const
    {
        Queue_Family_Indices indices;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

        int i = 0;
        for (const auto& queue_family : queue_families)
        {
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphics_family = i;
            }

            //Check if this device supports window system integration (a.k.a. presentation)
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);

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

    /// <summary>
    /// Creates the logical device for the selected physical device, setting up the necessary
    /// queues for graphics and presentation. It configures the device features, queue creation info, 
    /// and required extensions. After creating the logical device, it retrieves and stores the 
    /// graphics and presentation queue handles.
    /// </summary>
    /// <param name="surface">The Vulkan surface used to find presentation queue support.</param>
    /// <exception cref="std::runtime_error">Thrown if the logical device creation fails.</exception>
    void Vulkan_Instance::create_logical_device(const VkSurfaceKHR surface)
    {
        Queue_Family_Indices indices = find_queue_families(surface, physical_device);

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
        VkPhysicalDeviceFeatures device_features{};
        device_features.samplerAnisotropy = VK_TRUE; //Device needs to support anisotropic filtering

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

    /// <summary>
    /// Retrieves the name of the given physical device by querying its properties
    /// and returning the device name as a string.
    /// </summary>
    /// <param name="physical_device">The Vulkan physical device whose name is being queried.</param>
    /// <returns>Returns the name of the physical device as a string.</returns>
    std::string Vulkan_Instance::get_physical_device_name(const VkPhysicalDevice& physical_device) const
    {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(physical_device, &device_properties);

        std::string device_name(device_properties.deviceName);

        return device_name;
    }

    /// <summary>
    /// Retrieves the type of the given physical device (e.g., discrete GPU, integrated GPU, CPU, etc.)
    /// by querying its properties and returning a string that represents the device type.
    /// </summary>
    /// <param name="physical_device">The Vulkan physical device whose type is being queried.</param>
    /// <returns>Returns a string representing the type of the physical device.</returns>
    std::string Vulkan_Instance::get_physical_device_type(const VkPhysicalDevice& physical_device) const
    {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(physical_device, &device_properties);

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
    /// Retrieves a string containing the supported Vulkan version
    /// </summary>
    /// <param name="physical_device"></param>
    /// <returns></returns>
    std::string Vulkan_Instance::get_physical_device_vulkan_support(const VkPhysicalDevice& physical_device) const
    {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(physical_device, &device_properties);

        uint32_t deviceApiVersion = device_properties.apiVersion;

        uint32_t major = VK_VERSION_MAJOR(deviceApiVersion);
        uint32_t minor = VK_VERSION_MINOR(deviceApiVersion);

        return std::string(std::to_string(major) + "." + std::to_string(minor));
    }

    void Vulkan_Instance::init_instance()
    {
        create_instance();
    }

    void Vulkan_Instance::init_surface(GLFWwindow* window)
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    void Vulkan_Instance::init_device()
    {
        pick_physical_device(surface);
        create_logical_device(surface);
    }

    void Vulkan_Instance::init_allocator()
    {
        VmaAllocatorCreateInfo allocator_create_info = {};
        allocator_create_info.instance = instance;
        allocator_create_info.physicalDevice = physical_device;
        allocator_create_info.device = device;
        allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_0;

        if (vmaCreateAllocator(&allocator_create_info, &allocator) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create memory allocator!");
        }
    }

    void Vulkan_Instance::cleanup_allocator()
    {
        vmaDestroyAllocator(allocator);
    }

    void Vulkan_Instance::cleanup_instance()
    {
        vkDestroyInstance(instance, nullptr);
    }

    void Vulkan_Instance::cleanup_surface()
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }


    void Vulkan_Instance::cleanup_device()
    {
        vkDestroyDevice(device, nullptr);
    }

    std::string Vulkan_Instance::get_physical_device_name() const
    {
        return get_physical_device_name(physical_device);
    }

    std::string Vulkan_Instance::get_physical_device_type() const
    {
        return get_physical_device_type(physical_device);
    }

    VkPhysicalDeviceProperties Vulkan_Instance::get_physical_device_properties() const
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physical_device, &properties);

        return properties;
    }

    VkPhysicalDeviceMemoryProperties Vulkan_Instance::get_physical_memory_device_properties() const
    {
        VkPhysicalDeviceMemoryProperties memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

        return memory_properties;
    }

    Swap_Chain_Support_Details Vulkan_Instance::query_swap_chain_support(const VkSurfaceKHR surface) const
    {
        return query_swap_chain_support(surface, physical_device);
    }

    Queue_Family_Indices Vulkan_Instance::get_queue_families(const VkSurfaceKHR surface) const
    {
        return find_queue_families(surface, physical_device);
    }

    /// <summary>
    /// Finds and returns a suitable depth format for depth buffering by checking 
    /// for the best-supported format from a list of common depth formats. 
    /// It searches for a format that supports depth-stencil attachment with optimal tiling.
    /// </summary>
    /// <returns>Returns the best supported VkFormat for depth attachments.</returns>
    VkFormat Vulkan_Instance::find_depth_format()
    {
        return find_supported_format
        (
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    /// <summary>
    /// Searches through a list of candidate formats to find one that is supported by the physical device
    /// for the specified image tiling and feature requirements. It checks each format for the required
    /// tiling and format features and returns the first matching format.
    /// </summary>
    /// <param name="candidates">A list of VkFormat candidates to be checked.</param>
    /// <param name="tiling">The desired VkImageTiling (linear or optimal).</param>
    /// <param name="features">The required VkFormatFeatureFlags (e.g., depth-stencil attachment).</param>
    /// <returns>Returns the first VkFormat that supports the requested tiling and features.</returns>
    /// <exception cref="std::runtime_error">Thrown if no supported format is found.</exception>
    VkFormat Vulkan_Instance::find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }


    /// <summary>
    /// Finds and returns a suitable memory type index based on the provided type filter and required memory properties.
    /// It checks the physical device's memory types against the filter and required properties to find a match.
    /// </summary>
    /// <param name="type_filter">A bitmask representing the acceptable memory types.</param>
    /// <param name="properties">The required memory properties (e.g., VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT).</param>
    /// <returns>Returns the index of a suitable memory type.</returns>
    /// <exception cref="std::runtime_error">Thrown if no suitable memory type is found.</exception>
    uint32_t Vulkan_Instance::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const
    {
        VkPhysicalDeviceMemoryProperties memory_properties = get_physical_memory_device_properties();

        for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
        {
            if (type_filter & (1 << i) && //Memory type has to be one of the suitable types
                (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) //Memory has to support all required properties
            {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    std::string Vulkan_Instance::get_memory_statistics() const
    {
        VkPhysicalDeviceMemoryProperties memory_properties = get_physical_memory_device_properties();

        std::vector<VmaBudget> budgets{};
        budgets.resize(memory_properties.memoryHeapCount);

        vmaGetHeapBudgets(allocator, budgets.data());


        // Create a stringstream to build the statistics table
        std::stringstream statistics;
        statistics << std::left << std::setw(15) << "Heap Index"
            << std::setw(15) << "Heap Type"
            << std::setw(20) << "Allocated (MB)"
            << std::setw(20) << "Reserved (MB)"
            << std::setw(20) << "Total (MB)"
            << "\n";

        statistics << std::string(80, '-') << "\n";

        // Loop through all heaps
        for (uint32_t i = 0; i < memory_properties.memoryHeapCount; i++)
        {
            bool isDeviceLocal = memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
            std::string heapType = isDeviceLocal ? "Device Local" : "Host Visible";

            const VmaBudget& budget = budgets[i];

            // Convert values to MB for readability
            float used_memory_MB = (float)budget.statistics.allocationBytes / (1024.0f * 1024.0f);
            float reserved_MB = (float)budget.statistics.blockBytes / (1024.0f * 1024.0f);
            float total_memory_MB = (float)budget.budget / (1024.0f * 1024.0f);

            statistics << std::setw(15) << i
                << std::setw(15) << heapType
                << std::setw(20) << std::fixed << std::setprecision(2) << used_memory_MB
                << std::setw(20) << reserved_MB
                << std::setw(20) << total_memory_MB
                << "\n";
        }

        return statistics.str();
    }

    bool Queue_Family_Indices::is_complete() const
    {
        return graphics_family.has_value() && present_family.has_value();
    }
}