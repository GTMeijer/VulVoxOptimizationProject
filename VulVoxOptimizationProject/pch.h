#pragma once

//C headers
#include <cstdint>
#include <cstdlib>

//STL
#include <iostream>
#include <stdexcept>
#include <vector>
#include <map>
#include <cstring>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>



//GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#define GLFW_DLL
#include <GLFW/glfw3.h>

//Vulkan
#include <vulkan/vk_enum_string_helper.h> //for error code conversion


#include "utils.h"