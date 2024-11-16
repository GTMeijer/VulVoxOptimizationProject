cmake_minimum_required(VERSION 3.28)

project(VulVoxRenderer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCE_FILES "*.cpp")

add_library(VulVoxRenderer STATIC ${SOURCE_FILES})

find_package(Vulkan REQUIRED)

set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../includes")

set(INCLUDE_DIRS
		"${INCLUDE_DIR}/glfw-3.4/WIN64/include"
		"${INCLUDE_DIR}/glm"
		"${INCLUDE_DIR}/stb-image"
		"${INCLUDE_DIR}/tinyobjloader"
		"${INCLUDE_DIR}/VulkanMemoryAllocator-3.1.0/include"
)

target_include_directories(VulVoxRenderer PUBLIC ${INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS})

set(LIB_DIRS
		"${INCLUDE_DIR}/glfw-3.4/WIN64/lib-vc2022"
)

target_link_directories(VulVoxRenderer PUBLIC ${LIB_DIRS})

target_link_libraries(VulVoxRenderer 
	Vulkan::Vulkan 
	"glfw3.lib"
)