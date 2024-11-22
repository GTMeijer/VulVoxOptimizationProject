cmake_minimum_required(VERSION 3.28)

project(VulVoxOptimizationProject VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCE_FILES "*.cpp")

add_library(VulVoxOptimizationProject STATIC ${SOURCE_FILES})

find_package(Vulkan REQUIRED)

set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../includes")

#Platform specific settings
if(WIN32)
    #Windows specific includes and libraries

    set(INCLUDE_DIRS
            "${INCLUDE_DIR}/glfw-3.4/WIN64/include"
            "${INCLUDE_DIR}/glm"
            "${INCLUDE_DIR}/stb-image"
            "${INCLUDE_DIR}/tinyobjloader"
            "${INCLUDE_DIR}/VulkanMemoryAllocator-3.1.0/include"
    )

    set(LIB_DIRS
            "${INCLUDE_DIR}/glfw-3.4/WIN64/lib-vc2022"
    )

    target_link_directories(VulVoxOptimizationProject PUBLIC ${LIB_DIRS})
    
    target_link_libraries(VulVoxOptimizationProject 
        Vulkan::Vulkan 
        "glfw3.lib"
    )


elseif(APPLE)
    # MacOS specific includes and libraries
    find_package(glfw3 REQUIRED) #Use a package manager like Homebrew to install GLFW

    set(INCLUDE_DIRS
            "${INCLUDE_DIR}/glm"
            "${INCLUDE_DIR}/stb-image"
            "${INCLUDE_DIR}/tinyobjloader"
            "${INCLUDE_DIR}/VulkanMemoryAllocator-3.1.0/include"
    )

    target_link_libraries(VulVoxOptimizationProject
        Vulkan::Vulkan
        glfw
        # Required for GLFW on macOS apperantly...
        "-framework Cocoa"        # MacOS window management support
        "-framework IOKit"        # I/O support
        "-framework CoreVideo"    # Frame timing/synchronization
    )
elseif(UNIX)
    # Linux specific includes and libraries
    find_package(glfw3 REQUIRED) #Use a package manager like apt or pacman to install GLFW

    set(INCLUDE_DIRS
            "${INCLUDE_DIR}/glm"
            "${INCLUDE_DIR}/stb-image"
            "${INCLUDE_DIR}/tinyobjloader"
            "${INCLUDE_DIR}/VulkanMemoryAllocator-3.1.0/include"
    )

    target_link_libraries(VulVoxOptimizationProject
        Vulkan::Vulkan
        glfw
        pthread #For threading support
        dl      #Dynamic linking, needed for Vulkan
    )

endif()

target_include_directories(VulVoxOptimizationProject SYSTEM PUBLIC ${INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS})
