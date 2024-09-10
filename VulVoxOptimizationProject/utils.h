#pragma once

template <typename R, typename T>
bool contains(R&& range, const T& value) {
    return std::find(std::begin(range), std::end(range), value) != std::end(range);
}

static std::vector<char> read_file(const std::filesystem::path& file_path)
{
    //Start reading at file end to determine buffer size
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file " + file_path.generic_string());
    }

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}