#include "pch.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define VMA_IMPLEMENTATION
#ifdef DEBUG
    #define VMA_DEBUG_LOG_FORMAT(format, ...) do { \
           printf((format), __VA_ARGS__); \
        printf("\n"); \
    } while(false)
#endif // DEBUG

#include <vk_mem_alloc.h>