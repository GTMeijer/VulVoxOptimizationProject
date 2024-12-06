#pragma once

namespace vulvox
{
    struct Instance_Data
    {
        /// <summary>
        /// Returns a description of the input buffer containing instances
        /// </summary>
        /// <returns></returns>
        static VkVertexInputBindingDescription get_binding_description(uint32_t binding);

        /// <summary>
        /// Defines an input attribute description for each of the member variables
        /// Each descriptor contains the format and byte offset with respect to the vertex struct
        /// </summary>
        /// <returns></returns>
        static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions(uint32_t binding);
    };

    struct Plane_Instance_Data
    {
        /// <summary>
        /// Returns a description of the input buffer containing instances
        /// </summary>
        /// <returns></returns>
        static VkVertexInputBindingDescription get_binding_description(uint32_t binding);

        /// <summary>
        /// Defines an input attribute description for each of the member variables
        /// Each descriptor contains the format and byte offset with respect to the vertex struct
        /// </summary>
        /// <returns></returns>
        static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions(uint32_t binding);
    };
}