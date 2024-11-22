#pragma once

#include <iostream>
#include <chrono>

//GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//GLM
//Force depth range from 0.0 to 1.0 (Vulkan standard), instead of -1.0 to 1.0
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <filesystem>
#include "../VulVoxOptimizationProject/mvp.h"
#include "../VulVoxOptimizationProject/instance_data.h"
#include "../VulVoxOptimizationProject/renderer.h"


#include "camera.h"


const std::string MODEL_PATH = "../models/konata.obj";
const std::string KONATA_MODAL_TEXTURE_PATH = "../textures/konata_texture.png";
const std::string KONATA_TEXTURE_PATH = "../textures/konata.png";

const std::string CUBE_MODEL_PATH = "../models/cube-tex.obj";
const std::string CUBE_WHITE_TEXTURE_PATH = "../textures/cube-tex.png";
const std::string CUBE_BLUE_TEXTURE_PATH = "../textures/cube-tex-blue.png";