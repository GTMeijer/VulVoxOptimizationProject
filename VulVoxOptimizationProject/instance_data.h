#pragma once

struct Instance_Data
{
    glm::vec3 position;
    glm::vec3 rotation;
    float scale;
    uint32_t texture_index;

    /// <summary>
    /// Returns a description of the input buffer containing instances
    /// </summary>
    /// <returns></returns>
    static VkVertexInputBindingDescription get_binding_description(uint32_t binding)
    {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding = binding; //Array binding index
        binding_description.stride = sizeof(Instance_Data);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE; //Can be vertex or instance

        return binding_description;
    }

    /// <summary>
    /// Defines an input attribute description for each of the member variables
    /// Each descriptor contains the format and byte offset with respect to the vertex struct
    /// </summary>
    /// <returns></returns>
    static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions(uint32_t binding)
    {
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions{};
        attribute_descriptions.resize(3);

        attribute_descriptions[0].binding = binding; //Source array binding index
        attribute_descriptions[0].location = 3; //Location index in shader
        attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(Instance_Data, position); //size in bytes of position in class Instance_Data

        attribute_descriptions[1].binding = binding; //Source array binding index
        attribute_descriptions[1].location = 4; //Location index in shader
        attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(Instance_Data, rotation); //size in bytes of rotation in class Instance_Data

        attribute_descriptions[2].binding = binding; //Source array binding index
        attribute_descriptions[2].location = 5; //Location index in shader
        attribute_descriptions[2].format = VK_FORMAT_R32_SFLOAT;
        attribute_descriptions[2].offset = offsetof(Instance_Data, scale); //size in bytes of scale in class Instance_Data

        attribute_descriptions[2].binding = binding; //Source array binding index
        attribute_descriptions[2].location = 6; //Location index in shader
        attribute_descriptions[2].format = VK_FORMAT_R32_SINT;
        attribute_descriptions[2].offset = offsetof(Instance_Data, texture_index); //size in bytes of texture_index in class Instance_Data

        return attribute_descriptions;
    }
};