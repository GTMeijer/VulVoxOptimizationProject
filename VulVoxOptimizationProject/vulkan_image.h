#pragma once

class Image
{
public:

    Image() = default;

    void create_image(Vulkan_Instance* vulkan_instance, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlags aspect_flags);
    void create_image_view();

    void transition_image_layout(VkCommandBuffer command_buffer, VkImageLayout new_layout);

    void destroy();

    VkImage image;
    VkDeviceMemory image_memory;
    VkImageView image_view;

    uint32_t width;
    uint32_t height;

    VkFormat format;
    VkImageAspectFlags aspect_flags;

private:

    Vulkan_Instance* vulkan_instance;

    VkImageLayout current_layout;
};