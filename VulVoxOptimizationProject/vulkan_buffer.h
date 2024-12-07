#pragma once

namespace vulvox
{
    class Buffer
    {
    public:
        Buffer() = default;

        void create(Vulkan_Instance& instance, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags alloc_flags);
        void destroy(VmaAllocator allocator);

        void recreate(Vulkan_Instance& instance, VkDeviceSize new_size);

        template<typename T>
        void copy_to_buffer(Vulkan_Instance& instance, const std::vector<T>& data)
        {
            size_t data_size = data.size() * sizeof(T);

            if (data_size > size)
            {
                recreate(instance, data_size);
            }

            if (allocation_info.pMappedData != nullptr)
            {
                memcpy(allocation_info.pMappedData, data.data(), data_size);
            }
        }

        template<typename T>
        void copy_to_buffer(Vulkan_Instance& instance, const T& data)
        {
            static_assert(!std::is_pointer_v<T>, "copy_to_buffer called with a pointer as data, did you mean to pass the underlying data?");
            size_t data_size = sizeof(T);

            if (data_size > size)
            {
                recreate(instance, data_size);
            }

            if (allocation_info.pMappedData != nullptr)
            {
                memcpy(allocation_info.pMappedData, &data, data_size);
            }
        }

        VkDeviceSize size;
        VkBufferUsageFlags usage_flags;
        VmaAllocationCreateFlags alloc_flags;

        VkBuffer buffer{ VK_NULL_HANDLE };
        VmaAllocation allocation{ VK_NULL_HANDLE };
        VmaAllocationInfo allocation_info;
    };
}