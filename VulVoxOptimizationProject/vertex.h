#pragma once

struct Vertex
{
    glm::vec2 position;
    glm::vec3 color;

    /// <summary>
    /// Returns a description of the input buffer containing vertices
    /// </summary>
    /// <returns></returns>
    static VkVertexInputBindingDescription get_binding_description()
    {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding = 0; //Array binding index
        binding_description.stride = sizeof(Vertex);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; //Can be vertex or instance

        return binding_description;
    }

    /// <summary>
    /// Defines an input attribute description for each of the member variables
    /// Each descriptor contains the format and byte offset with respect to the vertex struct
    /// </summary>
    /// <returns></returns>
    static std::array<VkVertexInputAttributeDescription, 2> get_attribute_descriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{};

        attribute_descriptions[0].binding = 0; //Source array binding index
        attribute_descriptions[0].location = 0; //Location index in shader
        attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(Vertex, position); //byte offset of position in class vertex

        attribute_descriptions[1].binding = 0; //Source array binding index
        attribute_descriptions[1].location = 1; //Location index in shader
        attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(Vertex, color); //byte offset of position in class vertex





        return attribute_descriptions;
    }
};