#pragma once

namespace vulvox
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 texture_coordinates;

        /// <summary>
        /// Returns a description of the input buffer containing vertices, including the memory stride.
        /// </summary>
        /// <returns></returns>
        static VkVertexInputBindingDescription get_binding_description(uint32_t binding);

        /// <summary>
        /// Defines an input attribute description for each of the member variables
        /// Each descriptor contains the format and byte offset with respect to the vertex struct
        /// </summary>
        /// <returns></returns>
        static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions(uint32_t binding);

        bool operator==(const Vertex& other) const;

    };
}

//Create hash function for vertices (useful for comparing in (unordered) maps)
namespace std
{
    template<> struct hash<vulvox::Vertex>
    {
        size_t operator()(vulvox::Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.position) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texture_coordinates) << 1);
        }
    };
}