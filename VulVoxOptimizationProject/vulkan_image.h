#pragma once

namespace vulvox
{
    class Image
    {
    public:

        Image() = default;

        /// <summary>
        /// Constructor for a single layered image.
        /// </summary>
        void create_image(Vulkan_Instance* vulkan_instance, uint32_t image_width, uint32_t image_height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspect_flags, VmaMemoryUsage memory_usage);

        /// <summary>
        /// Constructor for an image array.
        /// </summary>
        void create_image(Vulkan_Instance* vulkan_instance, uint32_t image_width, uint32_t image_height, uint32_t array_layers, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspect_flags, VmaMemoryUsage memory_usage);


        /// <summary>
        /// Creates an image view for a 2d texture
        /// </summary>
        void create_image_view();

        /// <summary>
        /// Creates an image view for a texture array
        /// </summary>
        /// <param name="layer_count">amount of layers in the texture array</param>
        void create_image_array_view();

        void create_texture_sampler();

        void transition_image_layout(Vulkan_Command_Pool& command_pool, VkImageLayout new_layout);


        void destroy();

        static void copy_buffer_to_image(Vulkan_Command_Pool& command_pool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        static void copy_buffer_to_image_array(Vulkan_Command_Pool& command_pool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layer_count, VkDeviceSize layer_size);

        static Image create_texture_image(Vulkan_Instance& vulkan_instance, Vulkan_Command_Pool& command_pool, const std::filesystem::path& texture_path);
        static Image create_texture_array_image(Vulkan_Instance& vulkan_instance, Vulkan_Command_Pool& command_pool, const std::vector<std::filesystem::path>& texture_paths);

        VkImage image;
        VmaAllocation allocation;
        VmaAllocationInfo allocation_info;

        VkImageLayout current_layout;
        VkImageView image_view;

        uint32_t width;
        uint32_t height;
        uint32_t layer_count = 1;

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