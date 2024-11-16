# VulVoxOptimizationProject

How to compile the renderer:

# Windows

Download and install the Vulkan SDK from the [LungarG website](https://vulkan.lunarg.com/).
Open the `.sln` file with Visual Studio.
Point the following properties to the Vulkan SDK install location:
- `C/C++` -> `Additional Include Directories`
- `Librarian` -> `Additional Library Directories`

The renderer project should now compile.

# MacOS

## Install the Vulkan SDK
Download and install the Vulkan SDK from the [LungarG website](https://vulkan.lunarg.com/).
The MacOS version uses MoltenVK internally, which supports up to Vulkan 1.2. This project only uses Vulkan 1.0 features so that shouldn't be a problem.

## Install GLFW
This project uses GLFW to handle window management, install this using homebrew:
``` bash
brew install glfw
```

## Build with cmake

```bash
cmake -S . -B build/ -D CMAKE_BUILD_TYPE=Debug
cmake --build build/
```


```bash
cmake -S . -B build/ -D CMAKE_BUILD_TYPE=Release
cmake --build build/ --config Release
```

# Linux

## Install the Vulkan SDK
Install the Vulkan SDK using you package manager:

``` bash
sudo apt install vulkan-sdk # Ubuntu/Debian
sudo pacman -S vulkan-headers vulkan-tools vulkan-validation-layers # Arch
```
## Install GLFW
This project uses GLFW to handle window management, install this using your package manager:
``` bash
sudo apt install libglfw3-dev # Ubuntu/Debian 
sudo pacman -S glfw-wayland glfw-x11 # Arch
```

## Build with cmake

```bash
cmake -S . -B build/ -D CMAKE_BUILD_TYPE=Debug
cmake --build build/
```


```bash
cmake -S . -B build/ -D CMAKE_BUILD_TYPE=Release
cmake --build build/ --config Release
```
