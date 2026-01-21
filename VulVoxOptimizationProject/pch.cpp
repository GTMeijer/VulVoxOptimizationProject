#include "pch.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define VMA_IMPLEMENTATION

#ifdef VULKAN_VERSION_COMPATABILITY
    //VMA uses some vulkan 1.3 features, older PCs might not support this.
    //In compat mode we set the vulkan version to 1.2 so VMA doesn't use this.
    #define VMA_VULKAN_VERSION 1002000
#endif

#ifdef DEBUG
    #define VMA_DEBUG_LOG_FORMAT(format, ...) do { \
           printf((format), __VA_ARGS__); \
        printf("\n"); \
    } while(false)
#endif // DEBUG

#include <vk_mem_alloc.h>