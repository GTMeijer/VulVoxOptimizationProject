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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//Vulkan
#include <vulkan/vk_enum_string_helper.h> //for error code conversion

//STB image library for image loading
//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "utils.h"
#include "vertex.h"
#include "mvp.h"