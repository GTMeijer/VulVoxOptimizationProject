#include "pch.h"
#include "vulkan_image.h"

namespace vulvox
{
    void Image::create_image(Vulkan_Instance* vulkan_instance, uint32_t image_width, uint32_t image_height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspect_flags, VmaMemoryUsage memory_usage)
    {
        this->vulkan_instance = vulkan_instance;
        this->width = image_width;
        this->height = image_height;
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
        sampler_info.maxLod = 0.0f;

        if (vkCreateSampler(vulkan_instance->device, &sampler_info, nullptr, &sampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    void Image::transition_image_layout(VkCommandBuffer command_buffer, VkImageLayout new_layout)
    {
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
        barrier.subresourceRange.baseArrayLayer = 0; //No array
        barrier.subresourceRange.layerCount = 1;

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
    }

    void Image::destroy()
    {
        vkDestroySampler(vulkan_instance->device, sampler, nullptr);

        vkDestroyImageView(vulkan_instance->device, image_view, nullptr);

        vmaDestroyImage(vulkan_instance->allocator, image, allocation);
    }

}