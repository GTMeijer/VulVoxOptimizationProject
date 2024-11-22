#pragma once

namespace vulvox
{
    struct Texture_Array_Index_Binding
    {
        /// <summary>
        /// Returns a description of the input buffer containing the texture array index and memory stride.
        /// </summary>
        /// <returns></returns>
        static VkVertexInputBindingDescription get_binding_description(uint32_t binding);

        /// <summary>
        /// Defines an input attribute description for each of the member variables
        /// Each descriptor contains the format and byte offset with respect to the vertex struct
        /// </summary>
        /// <returns></returns>
        static VkVertexInputAttributeDescription get_attribute_description(uint32_t binding);
    };
}