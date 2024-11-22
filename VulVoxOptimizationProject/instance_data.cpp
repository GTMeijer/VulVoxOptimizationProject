#include "pch.h"
#include "instance_data.h"

namespace vulvox
{
    VkVertexInputBindingDescription Instance_Data::get_binding_description(uint32_t binding)
    {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding = binding; //Array binding index
        binding_description.stride = sizeof(glm::mat4);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE; //Can be vertex or instance

        return binding_description;
    }

    std::vector<VkVertexInputAttributeDescription> Instance_Data::get_attribute_descriptions(uint32_t binding)
    {
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions{};
        attribute_descriptions.resize(4);

        //A mat4 uses four locations
        for (uint32_t i = 0; i < 4; i++)
        {
            attribute_descriptions[i].binding = binding; //Source array binding index
            attribute_descriptions[i].location = 3 + i; //Location index in shader
            attribute_descriptions[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attribute_descriptions[i].offset = i * sizeof(glm::vec4); //Byte offset relative to the start of the object
        }

        return attribute_descriptions;
    }
}