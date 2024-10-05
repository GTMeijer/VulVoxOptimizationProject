#pragma once

class Vulkan_Swap_Chain
{
public:

    explicit Vulkan_Swap_Chain(Vulkan_Instance* vulkan_instance);

    void create_swap_chain(GLFWwindow* window, VkSurfaceKHR surface);

    /// <summary>
    /// In case of a window resize we need to recreate the swap chain so it is compatible again
    /// </summary>
    void recreate_swap_chain(GLFWwindow* window, VkSurfaceKHR surface);

    void cleanup_swap_chain();

    //Swapchain context and information
    VkSwapchainKHR swap_chain = VK_NULL_HANDLE;
    VkFormat image_format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent = { 0,0 };

    //Buffers that hold the target images for renderpass
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkImageView> image_views;

private:

    //Couple this swap chain to a specific vulkan instance (The swap chain will be destroyed before the instance)
    Vulkan_Instance* vulkan_instance;


    //Swap chain creation helper functions
    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) const;
    VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) const;
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) const;

    void create_image_views();

    std::vector<VkImage> swap_chain_images;


};