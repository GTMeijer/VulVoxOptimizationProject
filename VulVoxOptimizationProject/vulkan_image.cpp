#include "pch.h"
#include "vulkan_image.h"

namespace vulvox
{
    void Image::create_image(Vulkan_Instance* vulkan_instance,
        uint32_t image_width, uint32_t image_height,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspect_flags, VmaMemoryUsage memory_usage)
    {
        this->vulkan_instance = vulkan_instance;
        this->width = image_width;
        this->height = image_height;
        this->layer_count = 1;
        this->format = format;
        this->aspect_flags = aspect_flags;

        //Descripe image memory format
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = width;
        image_info.extent.height = height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1; //No mip mapping
        image_info.arrayLayers = 1;
        image_info.format = format; //Same as pixel buffers
        image_info.tiling = tiling; //We dont need to access the images memory so no need for linear tiling
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //Discard after first transition
        image_info.usage = usage; //Destination for buffer copy and readable by shader
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //Only one queue family will use the image (graphics)
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.flags = 0; //Optional

        current_layout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo image_alloc_info{};
        image_alloc_info.usage = memory_usage;

        if (VkResult result = vmaCreateImage(vulkan_instance->allocator, &image_info, &image_alloc_info, &image, &allocation, &allocation_info); result != VK_SUCCESS)
        {
            std::string error_string{ string_VkResult(result) };
            throw std::runtime_error("Failed to create image!" + error_string);
        }
    }

    void Image::create_image(Vulkan_Instance* vulkan_instance,
        uint32_t image_width, uint32_t image_height, uint32_t array_layers,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspect_flags, VmaMemoryUsage memory_usage)
    {
        this->vulkan_instance = vulkan_instance;
        this->width = image_width;
        this->height = image_height;
        this->layer_count = array_layers;
        this->format = format;
        this->aspect_flags = aspect_flags;

        //Descripe image memory format
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = width;
        image_info.extent.height = height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1; //No mip mapping
        image_info.arrayLayers = array_layers;
        image_info.format = format; //Same as pixel buffers
        image_info.tiling = tiling; //We dont need to access the images memory so no need for linear tiling
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //Discard after first transition
        image_info.usage = usage; //Destination for buffer copy and readable by shader
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //Only one queue family will use the image (graphics)
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.flags = 0; //Optional

        current_layout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo image_alloc_info{};
        image_alloc_info.usage = memory_usage;

        if (VkResult result = vmaCreateImage(vulkan_instance->allocator, &image_info, &image_alloc_info, &image, &allocation, &allocation_info); result != VK_SUCCESS)
        {
            std::string error_string{ string_VkResult(result) };
            std::cout << error_string << std::endl;
            throw std::runtime_error("Failed to create image!" + error_string);
        }
    }

    /// <summary>
    /// Creates an image view for a 2d texture
    /// </summary>
    void Image::create_image_view()
    {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format;

        //Default color channel mapping (SWIZZLE to default)

        //Single layer image, no mipmapping
        view_info.subresourceRange.aspectMask = aspect_flags;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(vulkan_instance->device, &view_info, nullptr, &image_view) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image view!");
        }
    }

    /// <summary>
    /// Creates an image view for a texture array
    /// </summary>
    /// <param name="layer_count">amount of layers in the texture array</param>
    void Image::create_image_array_view()
    {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        view_info.format = format;

        //Default color channel mapping (SWIZZLE to default)

        //Single layer image, no mipmapping
        view_info.subresourceRange.aspectMask = aspect_flags;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = layer_count;

        if (vkCreateImageView(vulkan_instance->device, &view_info, nullptr, &image_view) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image view!");
        }
    }

    void Image::create_texture_sampler()
    {
        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        //Magnification and minification interpolation.
        //Nearest is pixelated, linear is smooth
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;

        //How to handle when the surface is larger than the texture
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        VkPhysicalDeviceProperties properties = vulkan_instance->get_physical_device_properties();

        sampler_info.anisotropyEnable = VK_TRUE;
        sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        //What color to use when ADDRESS MODE is set to border clamp
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

        //Use 0..1 if false or 0..TexWidth/0..TexHeight if true for texture coordinates
        sampler_info.unnormalizedCoordinates = VK_FALSE;

        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

        //No mip mapping at the moment, for later?
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = VK_LOD_CLAMP_NONE;

        if (vkCreateSampler(vulkan_instance->device, &sampler_info, nullptr, &sampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    void Image::transition_image_layout(Vulkan_Command_Pool& command_pool, VkImageLayout new_layout)
    {
        VkCommandBuffer command_buffer = command_pool.begin_single_time_commands();


        //Create barrier to prevent reading before write is done
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = current_layout;
        barrier.newLayout = new_layout;

        //Queue family indices for ownership transfer, we dont want to do this here so set IGNORED
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0; //No mipmapping
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layer_count;

        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags destination_stage;

        if (current_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            //Transfer to write optimal layout
            barrier.srcAccessMask = 0; //No need to wait
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; //

            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; //Start immediately (start of pipeline)
            destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (current_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            //Transfer write optimal to optimal shader read layout
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            throw std::invalid_argument("Unsupported layout transition!");
        }

        //Send command for image barrier
        vkCmdPipelineBarrier(command_buffer,
            source_stage,
            destination_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        current_layout = new_layout;

        command_pool.end_single_time_commands(command_buffer);
    }

    void Image::destroy()
    {
        vkDestroySampler(vulkan_instance->device, sampler, nullptr);

        vkDestroyImageView(vulkan_instance->device, image_view, nullptr);

        vmaDestroyImage(vulkan_instance->allocator, image, allocation);
    }

    void Image::copy_buffer_to_image(Vulkan_Command_Pool& command_pool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        VkCommandBuffer command_buffer = command_pool.begin_single_time_commands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0; //Start of pixel values
        region.bufferRowLength = 0; //Memory layout
        region.bufferImageHeight = 0; //Memory layout

        //Part of the image we want to copy
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0,0,0 };
        region.imageExtent = { width, height, 1 };

        //Enqueue the image copy operation
        vkCmdCopyBufferToImage(
            command_buffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

        command_pool.end_single_time_commands(command_buffer);
    }

    void Image::copy_buffer_to_image_array(Vulkan_Command_Pool& command_pool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layer_count, VkDeviceSize layer_size)
    {
        VkCommandBuffer command_buffer = command_pool.begin_single_time_commands();

        std::vector<VkBufferImageCopy> copy_regions;
        for (uint32_t i = 0; i < layer_count; i++)
        {

            VkBufferImageCopy region{};
            region.bufferOffset = layer_size * i; //Start of pixel values for this layer
            region.bufferRowLength = 0; //Memory layout
            region.bufferImageHeight = 0; //Memory layout

            //Part of the image we want to copy
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = i; //Layer to write
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0,0,0 };
            region.imageExtent = { width, height, 1 };

            copy_regions.push_back(region);
        }

        //Enqueue the image copy operation
        vkCmdCopyBufferToImage(
            command_buffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(copy_regions.size()),
            copy_regions.data());

        command_pool.end_single_time_commands(command_buffer);
    }

    Image Image::create_texture_image(Vulkan_Instance& vulkan_instance, Vulkan_Command_Pool& command_pool, const std::filesystem::path& texture_path)
    {
        int texture_width;
        int texture_height;
        int texture_channels;

        //Load texture image, force alpha channel
        stbi_uc* pixels = stbi_load(texture_path.string().c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
        VkDeviceSize image_size = texture_width * texture_height * 4; //RGBA8 assumed

        if (!pixels)
        {
            throw std::runtime_error("Failed to load texture image!");
        }

        //Setup host visible staging buffer
        Buffer staging_buffer;
        staging_buffer.create(vulkan_instance, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

        //Copy the texture data into the staging buffer
        memcpy(staging_buffer.allocation_info.pMappedData, pixels, image_size);

        //Image in staging buffer, we can free the image data
        stbi_image_free(pixels);

        Image texture_image;
        texture_image.create_image(&vulkan_instance, texture_width, texture_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VMA_MEMORY_USAGE_AUTO);

        //Change layout of target image memory to be optimal for writing destination
        texture_image.transition_image_layout(command_pool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        //Transfer the image data from the staging buffer to the image memory
        copy_buffer_to_image(command_pool, staging_buffer.buffer, texture_image.image, texture_image.width, texture_image.height);

        //Change layout of image memory to be optimal for reading by a shader
        texture_image.transition_image_layout(command_pool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        staging_buffer.destroy(vulkan_instance.allocator);

        texture_image.create_image_view();
        texture_image.create_texture_sampler();

        return texture_image;
    }

    Image Image::create_texture_array_image(Vulkan_Instance& vulkan_instance, Vulkan_Command_Pool& command_pool, const std::vector<std::filesystem::path>& texture_paths)
    {
        if (texture_paths.empty())
        {
            throw std::runtime_error("Failed to load texture image! No texture paths given.");
        }

        std::vector<stbi_uc*> pixel_layers;

        std::vector<uint32_t> texture_widths{};
        std::vector<uint32_t> texture_heights{};

        int texture_width = 0;
        int texture_height = 0;
        int texture_channels = 0;

        for (const auto& texture_path : texture_paths)
        {
            //Load image data from given paths, rgba format is assumed
            stbi_uc* pixels = stbi_load(texture_path.string().c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
            texture_widths.push_back(texture_width);
            texture_heights.push_back(texture_height);

            if (!pixels)
            {
                throw std::runtime_error("Failed to load texture image! Path was: " + texture_path.string());
            }

            pixel_layers.push_back(pixels);
        }

        if (std::ranges::adjacent_find(texture_widths, std::ranges::not_equal_to()) != texture_widths.end() ||
            std::ranges::adjacent_find(texture_heights, std::ranges::not_equal_to()) != texture_heights.end())
        {
            std::cout << "Warning: Given textures for texture array creation are not equal in size! Attempting to use max width and height values." << std::endl;
        }


        uint32_t max_width = *std::ranges::max_element(texture_widths.begin(), texture_widths.end());
        uint32_t max_height = *std::ranges::max_element(texture_heights.begin(), texture_heights.end());
        uint32_t layer_count = static_cast<uint32_t>(pixel_layers.size());

        VkDeviceSize max_layer_size = max_width * max_height * 4; //RGBA8 assumed
        VkDeviceSize buffer_size = max_layer_size * layer_count;

        //Setup host visible staging buffer
        Buffer staging_buffer;
        staging_buffer.create(vulkan_instance, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

        //Copy the texture layers into the staging buffer one by one
        for (int i = 0; i < pixel_layers.size(); i++)
        {
            VkDeviceSize layer_size = texture_widths[i] * texture_heights[i] * 4; //RGBA8 assumed

            memcpy((unsigned char*)staging_buffer.allocation_info.pMappedData + (i * layer_size), pixel_layers[i], layer_size);
        }

        //Image in staging buffer, we can free the image data
        for (auto& pixels : pixel_layers)
        {
            stbi_image_free(pixels);
        }

        pixel_layers.clear();

        Image layered_texture_image;
        layered_texture_image.create_image(&vulkan_instance, max_width, max_height, layer_count,
            VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VMA_MEMORY_USAGE_AUTO);

        //Change layout of target image memory to be optimal for writing destination
        layered_texture_image.transition_image_layout(command_pool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        //Transfer the image data from the staging buffer to the image memory
        copy_buffer_to_image_array(command_pool, staging_buffer.buffer, layered_texture_image.image, layered_texture_image.width, layered_texture_image.height, layered_texture_image.layer_count, max_layer_size);

        //Change layout of image memory to be optimal for reading by a shader
        layered_texture_image.transition_image_layout(command_pool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        staging_buffer.destroy(vulkan_instance.allocator);

        layered_texture_image.create_image_array_view();
        layered_texture_image.create_texture_sampler();

        return layered_texture_image;
    }

}