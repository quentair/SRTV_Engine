cmake_minimum_required(VERSION 3.30)

project(SRTVEngine)

find_package(Vulkan 1.4.328 REQUIRED)

# libraries

add_subdirectory(lib)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the executable directory automatically.

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

# source files

add_subdirectory(src)

# find glslangValidator to use it for shader file compilation with custom command

find_program (GLSLANG_VALIDATOR "glslangValidator" HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

# Generate a list of files that match the globbing-expressions (here, all shader files in "shaders" directory) and store it into the variable GLSL_SOURCE_FILE
# The GLOB_RECURSE mode will traverse all the subdirectories of the matched directory and match the files

file(GLOB_RECURSE GLSL_SOURCE_FILES CONFIGURE_DEPENDS
	"${PROJECT_SOURCE_DIR}/shaders/*.vert"
	"${PROJECT_SOURCE_DIR}/shaders/*.frag"
	"${PROJECT_SOURCE_DIR}/shaders/*.comp"
	)

# for each glsl shader file, build a spir-v equivalent (example.vert becomes example.vert.spv) and add them to a list

foreach(GLSL_FILE ${GLSL_SOURCE_FILES})
	message(STATUS "BUILDING SHADER")
	
	# format spv file
	get_filename_component(GLSL_FILE_NAME ${GLSL_FILE} NAME)
	set(SPIRV_FILE "${PROJECT_SOURCE_DIR}/shaders/${GLSL_FILE_NAME}.spv")
	message(STATUS "BUILDING ${GLSL_FILE_NAME}")
	
	# building command (glslangvalidator -V shaders/shader.vert -o shaders/shader.vert.spv)
	add_custom_command(
		OUTPUT ${SPIRV_FILE}
		COMMAND ${GLSLANG_VALIDATOR} -V ${GLSL_FILE} -o {SPIRV_FILE}
		DEPENDS ${GLSL_FILE}
	)
	list(APPEND SPIRV_BINARY_FILES ${SPIRV_FILE})
endforeach()

# add shader building command as a new building target
# it will execute the above custom command that create all the spir-v files from glsl files (because of DEPENDS keyword mentioning the previously created list)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

