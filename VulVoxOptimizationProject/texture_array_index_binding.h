#pragma once

namespace vulvox
{
    struct Texture_Array_Index_Binding
    {
        /// <summary>
        /// Returns a description of the input buffer containing the texture array index
        /// </summary>
        /// <returns></returns>
        static VkVertexInputBindingDescription get_binding_description(uint32_t binding)
        {
            VkVertexInputBindingDescription binding_description{};
            binding_description.binding = binding; //Array binding index
            binding_description.stride = sizeof(uint32_t);
            binding_description.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE; //Can be vertex or instance

            return binding_description;
        }

        /// <summary>
        /// Defines an input attribute description for each of the member variables
        /// Each descriptor contains the format and byte offset with respect to the vertex struct
        /// </summary>
        /// <returns></returns>
        static VkVertexInputAttributeDescription get_attribute_description(uint32_t binding)
        {
            VkVertexInputAttributeDescription attribute_description{};

            attribute_description.binding = binding; //Source array binding index
            attribute_description.location = 7; //Location index in shader
            attribute_description.format = VK_FORMAT_R32_UINT;
            attribute_description.offset = 0; //offset in bytes of texture_index in class Instance_Data

            return attribute_description;
        }
    };
}