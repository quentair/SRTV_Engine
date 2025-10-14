# Simple Ray Traced Voxel Engine

Ray traced voxel engine made with Vulkan

# Dependencies

- Vulkan SDK version 1.4.328.1
- vk-bootstrap version 1.4.328
- VulkanMemoryAllocator version 3.3.0
- SDL version 3.2.24
- stb_image version 2.30
- GLM version 1.0.1
- fmt version 12.0.0
- ImGui version 1.92.3

# Building

1. Download the repo archive
2. Install the version 1.4.328.1 of the Vulkan SDK (I did it from the [LunarG](https://vulkan.lunarg.com/sdk/home) website)
3. Download the source code of [SDL version 3.2.24](https://github.com/libsdl-org/SDL/releases/tag/release-3.2.24), unzip it and place it with the name "SDL" in the "lib" folder.
4. Download the source code of [fmt version 12.0.0](https://github.com/fmtlib/fmt/releases/tag/12.0.0), unzip it and place it with the name "fmt" in the "lib" folder.
5. Create a build directory at the root of the project (where the first CMakeLists is), you should have something like "SRTVEngine/build"
6. Run CMake at the root directory. The command should look like ```console cmake -B ./build -S .``` (or use CMakeGUI)