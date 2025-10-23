# set include directory path

set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/includes")

# add sources

add_executable(SRTVEngine
	main.cpp

	engine.cpp
	${INCLUDE_DIR}/engine.h

	${INCLUDE_DIR}/error_checker.h

	logger.cpp
	${INCLUDE_DIR}/logger.h

	engine_logger.cpp
	${INCLUDE_DIR}/engine_logger.h

	vma_usage.cpp

	resource_deletor.cpp
	${INCLUDE_DIR}/resource_deletor.h

	swapchain.cpp
	${INCLUDE_DIR}/swapchain.h

	${INCLUDE_DIR}/frame_data.h

	image.cpp
	${INCLUDE_DIR}/image.h

	descriptor.cpp
	${INCLUDE_DIR}/descriptor.h

	)

## add include files

target_include_directories(SRTVEngine PUBLIC "${INCLUDE_DIR}")

# set C++ version to 23

set_property(TARGET SRTVEngine PROPERTY CXX_STANDARD 23)

# contrary to OpenGL (-1,1), Vulkan uses depth with range (0,1)

target_compile_definitions(SRTVEngine PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

# link libraries

target_link_libraries(SRTVEngine PUBLIC Vulkan::Vulkan vk-bootstrap vma SDL3::SDL3 glm fmt::fmt stb_image imgui)

# copy the .dll file to the same folder as the executable (SDL3 especially)

add_custom_command(
	TARGET SRTVEngine POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	$<TARGET_RUNTIME_DLLS:SRTVEngine>
	$<TARGET_FILE_DIR:SRTVEngine>
	COMMAND_EXPAND_LISTS
	VERBATIM
	)