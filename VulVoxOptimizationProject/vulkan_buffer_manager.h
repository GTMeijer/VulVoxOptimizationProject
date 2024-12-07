#pragma once

namespace vulvox
{
    class Vulkan_Buffer_Manager
    {
    public:

        Vulkan_Buffer_Manager() = default;

        void init(Vulkan_Instance* vulkan_instance, const uint32_t swap_chain_image_count, const uint32_t growth_factor = 10)
        {
            this->vulkan_instance = vulkan_instance;
            this->swap_chain_image_count = swap_chain_image_count;
            this->growth_factor = growth_factor;

            create_uniform_buffers();
        }

        void set_growth_factor(const uint32_t growth_factor)
        {
            this->growth_factor = growth_factor;
        }

        void destroy()
        {
            // Destroy all buffers
            for (auto& buffer : uniform_buffers)
            {
                buffer.destroy(vulkan_instance->allocator);
            }
            uniform_buffers.clear();

            for (auto& buffer : instance_buffers)
            {
                buffer.destroy(vulkan_instance->allocator);

            }
            instance_buffers.clear();
        }

        void begin_frame()
        {
            instance_buffer_requests = 0;
        }

        Buffer& get_uniform_buffer(const uint32_t current_frame)
        {
            assert(current_frame < uniform_buffers.size());
            return uniform_buffers[current_frame];
        }

        /// <summary>
        /// Get an instance buffer with at least instance_size * instance_count capacity.
        /// </summary>
        /// <param name="current_frame">Index of the current swap_chain image.</param>
        /// <returns>Reference to an available buffer with at least instance_size * instance_count capacity.</returns>
        Buffer& get_instance_buffer(const uint32_t current_frame, const VkDeviceSize instance_size, const VkDeviceSize instance_count)
        {
            //Check if there are buffers left or if we need to create a new set
            uint32_t buffers_per_frame = instance_buffers.size() / swap_chain_image_count;
            uint32_t current_buffer_usage = instance_buffer_requests + 1;

            if (current_buffer_usage > buffers_per_frame)
            {
                expand_instance_buffers(current_buffer_usage, instance_size * instance_count);
            }

            uint32_t buffer_index = instance_buffer_requests * swap_chain_image_count + current_frame;
            instance_buffer_requests++;
            assert(buffer_index < uniform_buffers.size());

            VkDeviceSize buffer_size = instance_size * instance_count;

            //Check if the buffer size has enough capacity 
            if (buffer_size > instance_buffers[buffer_index].size)
            {
                //If not, resize it. Create a few extra so we reduce the chance we have to resize again next frame.
                instance_buffers[buffer_index].recreate(*vulkan_instance, buffer_size + instance_size * growth_factor);
            }

            return instance_buffers[buffer_index];
        }

    private:

        void create_uniform_buffers()
        {
            VkDeviceSize buffer_size = sizeof(MVP);

            uniform_buffers.resize(swap_chain_image_count);

            for (size_t i = 0; i < swap_chain_image_count; i++)
            {
                //Also maps pointer to memory so we can write to the buffer, this pointer is persistant throughout the applications lifetime.
                uniform_buffers[i].create(*vulkan_instance, buffer_size,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
            }
        }

        //Create an instance buffer thats accessable from both the host and device
        Buffer create_instance_buffer(const VkDeviceSize instance_buffer_size)
        {
            VkDeviceSize instance_data_buffer_size = instance_buffer_size;

            Buffer instance_buffer;
            instance_buffer.create(*vulkan_instance, instance_data_buffer_size,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

            return instance_buffer;
        }

        void expand_instance_buffers(uint32_t required_buffers_per_frame, VkDeviceSize instance_buffer_size)
        {
            uint32_t required_buffer_count = required_buffers_per_frame * swap_chain_image_count;

            if (instance_buffers.size() >= required_buffer_count)
            {
                return;
            }

            //Allocate additional buffers
            uint32_t to_allocate = required_buffer_count - instance_buffers.size();

            instance_buffers.reserve(required_buffer_count);
            for (uint32_t i = 0; i < to_allocate; i++)
            {
                instance_buffers.emplace_back(create_instance_buffer(instance_buffer_size));
            }
        }

        VkDevice device = VK_NULL_HANDLE;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        VmaAllocator allocator = VK_NULL_HANDLE;

        Vulkan_Instance* vulkan_instance = nullptr;

        uint32_t swap_chain_image_count = 0;
        uint32_t instance_buffer_requests = 0;
        uint32_t growth_factor = 10;

        std::vector<Buffer> uniform_buffers;
        std::vector<Buffer> instance_buffers;
    };

}