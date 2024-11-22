#pragma once

namespace vulvox
{
    /// <summary>
    /// Struct containing the indices of the command queues we require
    /// For this program we need a graphics and present capable queue.
    /// The families supporting these queues are stored in this class
    /// </summary>
    struct Queue_Family_Indices
    {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;

        /// <summary>
        /// Check if all queue families are filled.
        /// </summary>
        /// <returns>Returns true if all queue families indices are initialized.</returns>
        bool is_complete() const;
    };

    struct Swap_Chain_Support_Details
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    class Vulkan_Instance
    {
    public:

        Vulkan_Instance() = default;

        void init_instance();
        void init_surface(GLFWwindow* window);
        void init_device();
        void init_allocator();

        void cleanup_allocator();
        void cleanup_instance();
        void cleanup_surface();
        void cleanup_device();

        std::string get_physical_device_name() const;
        std::string get_physical_device_type() const;

        VkPhysicalDeviceProperties get_physical_device_properties() const;
        VkPhysicalDeviceMemoryProperties get_physical_memory_device_properties() const;

        Swap_Chain_Support_Details query_swap_chain_support(const VkSurfaceKHR surface) const;
        Queue_Family_Indices get_queue_families(const VkSurfaceKHR surface) const;

        VkFormat find_depth_format();

        VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const;

        //Vulkan and device contexts
        VkInstance instance = VK_NULL_HANDLE; //Vulkan context (driver access)
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE; //Physical GPU
        VkDevice device = VK_NULL_HANDLE; //Logical GPU context

        //Memory allocation helper
        VmaAllocator allocator;

        //Queues that send commands to the command buffers
        VkQueue graphics_queue = VK_NULL_HANDLE;
        VkQueue present_queue = VK_NULL_HANDLE;

    private:

        //Creates a vulkan instance so we can access the driver
        void create_instance();

        //Select the GPU to use among available physical devices.
        void pick_physical_device(const VkSurfaceKHR surface);
        int rate_physical_device(const VkSurfaceKHR surface, const VkPhysicalDevice& physical_device_candidate) const;
        bool check_glfw_extension_support() const;
        bool check_device_extension_support(const VkPhysicalDevice& physical_device) const;
        bool check_validation_layer_support() const;

        Swap_Chain_Support_Details query_swap_chain_support(const VkSurfaceKHR surface, const VkPhysicalDevice& physical_device) const;

        Queue_Family_Indices find_queue_families(const VkSurfaceKHR surface, const VkPhysicalDevice& physical_device) const;

        //Create the handle to the selected physical device
        void create_logical_device(const VkSurfaceKHR surface);

        std::string get_physical_device_name(const VkPhysicalDevice& physical_device) const;
        std::string get_physical_device_type(const VkPhysicalDevice& physical_device) const;

        //Required device extensions
        const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
        const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif
    };
}