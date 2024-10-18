#include "pch.h"
#include "model.h"

namespace vulvox
{
    Model::Model(const std::filesystem::path& path_to_model)
    {
        load_model(path_to_model);
    }

    uint64_t Model::get_vertices_size()
    {
        if (vertices.empty())
        {
            return 0;
        }

        return sizeof(vertices[0]) * vertices.size();
    }

    uint64_t Model::get_indices_size()
    {
        if (indices.empty())
        {
            return 0;
        }

        return sizeof(indices[0]) * indices.size();
    }

    Vertex* Model::get_vertices_ptr()
    {
        return vertices.data();
    }

    uint32_t* Model::get_indices_ptr()
    {
        return indices.data();
    }


    void Model::load_model(const std::filesystem::path& path_to_model)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path_to_model.string().c_str()))
        {
            throw std::runtime_error(warn + err);
        }

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
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1] //Flip the vertical axis for vulkan standard
                };

                vertex.color = { 1.0, 1.0, 1.0f };

                vertices.push_back(vertex);
                //indices.push_back(indices.size());

                if (unique_vertices.count(vertex) == 0)
                {
                    unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(unique_vertices[vertex]);
            }
        }

        std::cout << "Model loaded containing " << vertices.size() << " vertices." << std::endl;
    }
}