#include "pch.h"
#include "vulkan_swap_chain.h"

Vulkan_Swap_Chain::Vulkan_Swap_Chain(Vulkan_Instance* vulkan_instance) : vulkan_instance(vulkan_instance)
{
}

void Vulkan_Swap_Chain::create_swap_chain(GLFWwindow* window, VkSurfaceKHR surface)
{
    //Query supported formats and extends
    Swap_Chain_Support_Details swap_chain_support = vulkan_instance->query_swap_chain_support(surface);

    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats);
    std::cout << "Surface format: " << string_VkFormat(surface_format.format) << std::endl;

    VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
    std::cout << "Present mode: " << string_VkPresentModeKHR(present_mode) << std::endl;

    VkExtent2D extent = choose_swap_extent(swap_chain_support.capabilities, window);
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

    Queue_Family_Indices indices = vulkan_instance->get_queue_families(surface);

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

    if (vkCreateSwapchainKHR(vulkan_instance->device, &create_info, nullptr, &swap_chain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain!");
    }

    //Store the handles to the images inside the swap chains so we can write to them
    vkGetSwapchainImagesKHR(vulkan_instance->device, swap_chain, &image_count, nullptr);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(vulkan_instance->device, swap_chain, &image_count, swap_chain_images.data());

    image_format = surface_format.format;
    this->extent = extent;

    create_image_views();
}


void Vulkan_Swap_Chain::recreate_swap_chain(GLFWwindow* window, VkSurfaceKHR surface)
{
    int new_width = 0;
    int new_height = 0;
    glfwGetFramebufferSize(window, &new_width, &new_height);

    while (new_width == 0 || new_height == 0) {
        glfwGetFramebufferSize(window, &new_width, &new_height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(vulkan_instance->device);

    cleanup_swap_chain();

    create_swap_chain(window, surface);
}

void Vulkan_Swap_Chain::cleanup_swap_chain()
{
    for (auto& framebuffer : framebuffers)
    {
        vkDestroyFramebuffer(vulkan_instance->device, framebuffer, nullptr);
    }

    for (auto& image_view : image_views)
    {
        vkDestroyImageView(vulkan_instance->device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(vulkan_instance->device, swap_chain, nullptr);
}

VkSurfaceFormatKHR Vulkan_Swap_Chain::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) const
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

VkPresentModeKHR Vulkan_Swap_Chain::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) const
{
    for (const auto& available_present_mode : available_present_modes)
    {
        //No VSYNC, just present the most recent image
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return available_present_mode;
        }
    }

    //We default to vertical sync
    return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
}

/// <summary>
/// Pick the appropriate resolution for the swap chain.
/// </summary>
/// <param name="capabilities"></param>
/// <returns></returns>
VkExtent2D Vulkan_Swap_Chain::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) const
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

void Vulkan_Swap_Chain::create_image_views()
{
    image_views.resize(swap_chain_images.size());

    for (size_t i = 0; i < swap_chain_images.size(); i++)
    {
        image_views[i] = create_image_view(vulkan_instance->device, swap_chain_images[i], image_format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}