#include "pch.h"
#include "texture_array_index_binding.h"

namespace vulvox
{
    VkVertexInputBindingDescription Texture_Array_Index_Binding::get_binding_description(uint32_t binding)
    {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding = binding; //Array binding index
        binding_description.stride = sizeof(uint32_t);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE; //Can be vertex or instance

        return binding_description;
    }

    VkVertexInputAttributeDescription Texture_Array_Index_Binding::get_attribute_description(uint32_t binding)
    {
        VkVertexInputAttributeDescription attribute_description{};

        attribute_description.binding = binding; //Source array binding index
        attribute_description.location = 7; //Location index in shader
        attribute_description.format = VK_FORMAT_R32_UINT;
        attribute_description.offset = 0; //offset in bytes of texture_index

        return attribute_description;
    }
}