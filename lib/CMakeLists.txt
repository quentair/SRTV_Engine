# SDL2

add_subdirectory(SDL EXCLUDE_FROM_ALL)

# fmt

add_subdirectory(fmt EXCLUDE_FROM_ALL)

# GLM (header only library, so we only need to find its files at compilation time by adding them into a library type target)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

# VMA (header only library, so we only need to find its files at compilation time by adding them into a library type target)

add_library(vma INTERFACE)
target_include_directories(vma INTERFACE vma)

# STB Image (header only library, so we only need to find its files at compilation time by adding them into a library type target)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE stb_image)

# vk-bootstrap (as static via copy of source files only)

add_library(vk-bootstrap STATIC)

target_include_directories(vk-bootstrap PUBLIC vk-bootstrap)

target_sources(vk-bootstrap PRIVATE
	vk-bootstrap/VkBootstrap.cpp
	)

# vk-bootstrap needs to find vulkan headers
target_link_libraries(vk-bootstrap PUBLIC Vulkan::Vulkan)

set_property(TARGET vk-bootstrap PROPERTY CXX_STANDARD 23)

# ImGui (as static via copy of source files only)

add_library(imgui STATIC)

target_include_directories(imgui PUBLIC imgui)

target_sources(imgui PRIVATE
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_impl_vulkan.cpp
	imgui/imgui_impl_sdl3.cpp
	)

# imgui needs to find vulkan and SDL3 headers
target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL3::SDL3)

set_property(TARGET imgui PROPERTY CXX_STANDARD 23)