#pragma once

namespace vulvox
{
    class Image
    {
    public:

        Image() = default;

        void create_image(Vulkan_Instance* vulkan_instance, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspect_flags, VmaMemoryUsage memory_usage);
        
        /// <summary>
        /// Creates an image view for a 2d texture
        /// </summary>
        void create_image_view();

        /// <summary>
        /// Creates an image view for a texture array
        /// </summary>
        /// <param name="layer_count">amount of layers in the texture array</param>
        void create_image_view(int layer_count);

        void create_texture_sampler();

        void transition_image_layout(VkCommandBuffer command_buffer, VkImageLayout new_layout);

        void destroy();

        VkImage image;
        VmaAllocation allocation;
        VmaAllocationInfo allocation_info;

        VkImageLayout current_layout;
        VkImageView image_view;

        uint32_t width;
        uint32_t height;

        VkFormat format;
        VkImageAspectFlags aspect_flags;

        VkSampler sampler;

    private:

        Vulkan_Instance* vulkan_instance;


    };

    class Texture
    {

    };

    class TextureArray
    {

    };
}