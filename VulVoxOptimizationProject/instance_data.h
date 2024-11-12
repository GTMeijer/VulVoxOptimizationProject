#pragma once

namespace vulvox
{
    struct Instance_Data
    {
        glm::mat4 instance_model_matrix = glm::mat4{ 1.0f };

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
    };
}