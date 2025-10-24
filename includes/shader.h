#pragma once

#include "vulkan/vulkan.h"

namespace srtv_engine {

bool loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);

} // namespace srtv_engine