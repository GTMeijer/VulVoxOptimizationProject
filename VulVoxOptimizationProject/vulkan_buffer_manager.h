#pragma once

namespace vulvox
{
    class Vulkan_Buffer_Manager
    {
    public:

        Vulkan_Buffer_Manager() = default;

        void init(Vulkan_Instance* vulkan_instance, const uint32_t swap_chain_image_count, const uint32_t growth_factor = 10);
        void destroy();

        /// <summary>
        /// Set the amount of extra memory (in instance sizes) that is allocated when resizing a buffer.
        /// </summary>
        void set_growth_factor(const uint32_t growth_factor);

        /// <summary>
        /// Call this at the start of every frame to reset the instance buffer usage counter.
        /// </summary>
        void begin_frame();

        /// <summary>
        /// Retrieve the uniform buffer for the current swap chain image.
        /// </summary>
        Buffer& get_uniform_buffer(const uint32_t current_frame);

        /// <summary>
        /// Get index of an instance buffer with at least instance_size * instance_count capacity.
        /// </summary>
        /// <param name="current_frame">Index of the current swap_chain image.</param>
        /// <returns>Reference to an available buffer with at least instance_size * instance_count capacity.</returns>
        size_t get_instance_buffer(const uint32_t current_frame, const VkDeviceSize instance_size, const VkDeviceSize instance_count);

        Buffer& get_instance_buffer(const size_t buffer_index);

        /// <summary>
        /// Copy data to a instance buffer and returns the index of the buffer.
        /// </summary>
        template<typename T>
        size_t copy_to_instance_buffer(Vulkan_Instance& instance, const uint32_t current_frame, const std::vector<T>& data)
        {
            size_t buffer_index = get_instance_buffer(current_frame, sizeof(T), data.size());
            instance_buffers[buffer_index].copy_to_buffer(*vulkan_instance, data);

            return buffer_index;
        }

    private:

        void create_uniform_buffers();

        /// <summary>
        ///Create an instance buffer thats accessable from both the host and device.
        /// </summary>
        /// <param name="instance_buffer_size">Size in sizeof(instance_type) * instance_count.</param>
        Buffer create_instance_buffer(const VkDeviceSize instance_buffer_size);

        /// <summary>
        /// Create extra instance buffers with given buffer size if current amount of instance buffers is less than the requested amount.
        /// </summary>
        void expand_instance_buffers(uint32_t required_buffers_per_frame, VkDeviceSize instance_buffer_size);

        Vulkan_Instance* vulkan_instance = nullptr;

        uint32_t swap_chain_image_count = 0;
        uint32_t instance_buffer_requests = 0;
        uint32_t growth_factor = 10;

        //Uniform buffers, data available across shaders
        std::vector<Buffer> uniform_buffers;

        //Instance buffers, stores data that is part of specific draw calls
        std::vector<Buffer> instance_buffers;
    };

}