#include "pch.h"
#include "vulkan_shader.h"

Vulkan_Shader::Vulkan_Shader(VkDevice device, const std::filesystem::path& shader_filepath, const std::string& main_function_name, VkShaderStageFlagBits shader_stage_bit)
    : device(device), main_function_name(main_function_name), shader_stage_bit(shader_stage_bit)
{
    //Load compiled SPIR-V shader files 
    auto shader_code = read_file(shader_filepath);
    std::cout << "Shader file " << shader_filepath.filename() << " read, size is " << shader_code.size() << " bytes" << std::endl;

    if (std::filesystem::file_size(shader_filepath) != shader_code.size())
    {
        throw std::runtime_error("Failed to load shader, size mismatch!");
    }

    //Wrap the shader byte code in the shader modules for use in the pipeline
    shader_module = create_shader_module(device, shader_code);
}

Vulkan_Shader::~Vulkan_Shader()
{
    if (device != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(device, shader_module, nullptr);
        main_function_name.clear();
        shader_stage_bit = VK_SHADER_STAGE_ALL;
        device = VK_NULL_HANDLE;
    }
}

VkPipelineShaderStageCreateInfo Vulkan_Shader::get_shader_stage_create_info() const
{
    VkPipelineShaderStageCreateInfo shader_stage_info{};
    shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_info.stage = shader_stage_bit;
    shader_stage_info.module = shader_module;
    shader_stage_info.pName = main_function_name.c_str();
    shader_stage_info.pSpecializationInfo = nullptr; //Use this for control flow flags
    return shader_stage_info;
}

VkShaderModule Vulkan_Shader::create_shader_module(VkDevice device, const std::vector<char>& bytecode)
{
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = bytecode.size();
    create_info.pCode = reinterpret_cast<const uint32_t*> (bytecode.data());

    VkShaderModule new_shader_module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &create_info, nullptr, &new_shader_module) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }

    return new_shader_module;
}