#pragma once

class Model
{
public:

    Model() = default;
    Model(const std::filesystem::path& path_to_model);

    uint64_t get_vertices_size();
    uint64_t get_indices_size();
    
    Vertex* get_vertices_ptr();
    uint32_t* get_indices_ptr();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

private:

    void load_model(const std::filesystem::path& path_to_model);


};