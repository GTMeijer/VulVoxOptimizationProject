#include "pch.h"
#include "vertex.h"

namespace vulvox
{
    VkVertexInputBindingDescription Vertex::get_binding_description(uint32_t binding)
    {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding = binding; //Array binding index
        binding_description.stride = sizeof(Vertex);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; //Can be vertex or instance

        return binding_description;
    }

    std::vector<VkVertexInputAttributeDescription> Vertex::get_attribute_descriptions(uint32_t binding)
    {
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions{};
        attribute_descriptions.resize(3);

        attribute_descriptions[0].binding = binding; //Source array binding index
        attribute_descriptions[0].location = 0; //Location index in shader
        attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(Vertex, position); //size in bytes of position in class Vertex

        attribute_descriptions[1].binding = binding; //Source array binding index
        attribute_descriptions[1].location = 1; //Location index in shader
        attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(Vertex, color); //size in bytes of color in class Vertex

        attribute_descriptions[2].binding = binding; //Source array binding index
        attribute_descriptions[2].location = 2; //Location index in shader
        attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[2].offset = offsetof(Vertex, texture_coordinates); //size in bytes of texture_coordinates in class Vertex

        return attribute_descriptions;
    }

    bool Vertex::operator==(const Vertex& other) const
    {
        return position == other.position && color == other.color && texture_coordinates == other.texture_coordinates;
    }

}