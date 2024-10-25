#pragma once

namespace vulvox
{
    class Model
    {
    public:

        Model() = default;
        Model(Vulkan_Instance* instance, Vulkan_Command_Pool& command_pool, const std::filesystem::path& path_to_model);

        uint64_t vertex_buffer_size;
        uint64_t index_buffer_size;

        uint32_t vertex_count;
        uint32_t index_count;

        Buffer vertex_buffer;
        Buffer index_buffer;

        void destroy();
        
    private:

        void load_model(Vulkan_Command_Pool& command_pool, const std::filesystem::path& path_to_model);

        /// <summary>
        /// Creates the memory buffer containing the vertex data
        /// </summary>
        void create_vertex_buffer(Vulkan_Command_Pool& command_pool, const std::vector<Vertex>& vertices);
        /// <summary>
        /// Creates the memory buffer containing the index data
        /// </summary>
        void create_index_buffer(Vulkan_Command_Pool& command_pool, const std::vector<uint32_t>& indices);

        Vulkan_Instance* vulkan_instance;
    };
}