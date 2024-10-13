#pragma once

//C headers
#include <cstdint>
#include <cstdlib>

//STL
#include <string>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <cstring>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <chrono>


//GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#define GLFW_DLL
#include <GLFW/glfw3.h>

//GLM
//Force depth range from 0.0 to 1.0 (Vulkan standard), instead of -1.0 to 1.0
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//Include hash function for comparing vertices in an unordered map structure
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

//Vulkan
#include <vulkan/vk_enum_string_helper.h> //for error code conversion

//STB image library for image loading
//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

//Tiny object loader, library for model loading
#include <tiny_obj_loader.h>

//Vulkan memory allocator for easier memory management
#include <vk_mem_alloc.h>

#include "utils.h"
#include "vertex.h"
#include "mvp.h"
#include "instance_data.h"

#include "vulkan_instance.h"
#include "vulkan_image.h"
#include "vulkan_swap_chain.h"
#include "vulkan_buffer.h"
#include "model.h"
#include "vulkan_shader.h"

const std::string MODEL_PATH = "models/konata.obj";
const std::string TEXTURE_PATH = "textures/konata_texture.png";