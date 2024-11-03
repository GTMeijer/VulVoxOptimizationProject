#include "pch.h"
#include "model.h"

namespace vulvox
{
    Model::Model(Vulkan_Instance* instance, Vulkan_Command_Pool& command_pool, const std::filesystem::path& path_to_model)
        : vulkan_instance(instance)
    {
        load_model(command_pool, path_to_model);
    }

    void Model::destroy()
    {
        index_buffer.destroy(vulkan_instance->allocator);
        vertex_buffer.destroy(vulkan_instance->allocator);
    }

    void Model::load_model(Vulkan_Command_Pool& command_pool, const std::filesystem::path& path_to_model)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string warn;
        std::string err;
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path_to_model.string().c_str()))
        {
            std::cout << warn << err << std::endl;
            throw std::runtime_error(warn + err);
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        std::unordered_map<Vertex, uint32_t> unique_vertices{};

        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                Vertex vertex{};

                //tiny obj loader packs the vertices as three floats, so multiply index by three
                vertex.position =
                {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                //Same for texture coordinates, but two instead
                vertex.texture_coordinates =
                {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1] //Flip the vertical axis for vulkan standard coordinate system
                };

                vertex.color = { 1.0, 1.0, 1.0f };

                if (!unique_vertices.contains(vertex))
                {
                    unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(unique_vertices[vertex]);
            }
        }

        vertex_buffer_size = sizeof(vertices[0]) * vertices.size();
        index_buffer_size = sizeof(indices[0]) * indices.size();

        vertex_count = static_cast<uint32_t>(vertices.size());
        index_count = static_cast<uint32_t>(indices.size());

        create_vertex_buffer(command_pool, vertices);
        create_index_buffer(command_pool, indices);

        std::cout << "Model loaded containing " << vertices.size() << " vertices and " << indices.size() << " indices." << std::endl;
    }

    void Model::create_vertex_buffer(Vulkan_Command_Pool& command_pool, const std::vector<Vertex>& vertices)
    {
        VkDeviceSize buffer_size = vertices.size() * sizeof(vertices[0]);

        //Create staging buffer that transfers data between the host and device
        Buffer staging_buffer;
        staging_buffer.create(*vulkan_instance, buffer_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

        memcpy(staging_buffer.allocation_info.pMappedData, vertices.data(), buffer_size);

        //Create vertex buffer as device only buffer
        vertex_buffer.create(*vulkan_instance, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);

        //Copy data from host to device
        command_pool.copy_buffer(staging_buffer.buffer, vertex_buffer.buffer, buffer_size);

        //Data on device, cleanup temp buffers
        staging_buffer.destroy(vulkan_instance->allocator);
    }

    void Model::create_index_buffer(Vulkan_Command_Pool& command_pool, const std::vector<uint32_t>& indices)
    {
        VkDeviceSize buffer_size = indices.size() * sizeof(indices[0]);
        Buffer staging_buffer;
        staging_buffer.create(*vulkan_instance, buffer_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

        memcpy(staging_buffer.allocation_info.pMappedData, indices.data(), buffer_size);

        //Create index buffer as device only buffer
        index_buffer.create(*vulkan_instance, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0);

        //Copy data from host to device
        command_pool.copy_buffer(staging_buffer.buffer, index_buffer.buffer, buffer_size);

        //Data on device, cleanup temp buffers
        staging_buffer.destroy(vulkan_instance->allocator);
    }
}