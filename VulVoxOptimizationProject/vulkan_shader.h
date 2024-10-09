#pragma once
class Vulkan_Shader
{
public:

    Vulkan_Shader(VkDevice device, const std::filesystem::path& shader_filepath, const std::string& main_function_name, VkShaderStageFlagBits shader_stage_bit);
    ~Vulkan_Shader();

    VkShaderModule shader_module = VK_NULL_HANDLE;
    std::string main_function_name;
    VkShaderStageFlagBits shader_stage_bit = VK_SHADER_STAGE_ALL;

    VkPipelineShaderStageCreateInfo get_shader_stage_create_info() const;

private:

    VkDevice device = VK_NULL_HANDLE;
    VkShaderModule create_shader_module(VkDevice device, const std::vector<char>& bytecode);

};

